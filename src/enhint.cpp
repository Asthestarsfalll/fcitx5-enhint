/*
 * SPDX-FileCopyrightText: 2025 fcitx5-enhint contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "enhint.h"

#include <fcitx-config/iniparser.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/standardpaths.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/event.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputmethodentry.h>
#include <fcitx/statusarea.h>
#include <fcitx/text.h>
#include <fcitx/userinterfacemanager.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace enhint {

Enhint::Enhint(fcitx::Instance *instance) : instance_(instance) {
    reloadConfig();
    if (!instance_) {
        return;
    }

    instance_->userInterfaceManager().registerAction("enhint", &toggleAction_);

    outputFilterConn_ = instance_->connect<fcitx::Instance::OutputFilter>(
        [this](fcitx::InputContext *inputContext, fcitx::Text &text) {
            auto dictionary = cedict_.load();
            if (!config_.enabled.value() || !dictionary || text.empty()) {
                return;
            }
            // Programs on the skip list receive candidate text verbatim and
            // may insert it, so never decorate them.
            const auto &program = inputContext->program();
            const auto &skip = config_.skipPrograms.value();
            if (std::find(skip.begin(), skip.end(), program) != skip.end()) {
                return;
            }
            if (inputContext->capabilityFlags().test(
                    fcitx::CapabilityFlag::ClientSideInputPanel)) {
                // The client draws its own candidate window. Most clients
                // commit through SelectCandidate and work fine, but some insert
                // the string. Log once per program so a bad one can be added to
                // SkipPrograms.
                if (clientSideLogged_.insert(program).second) {
                    FCITX_INFO()
                        << "enhint: program '" << program
                        << "' uses a client-side candidate window; add it to "
                           "SkipPrograms if it inserts the hint";
                }
            }
            // The input panel preedit is displayed inline by the client and is
            // committed verbatim (e.g. browsers that commit the preedit on
            // composition end), so decorating it would leak the hint into the
            // text field. This is easy to trigger accidentally because the
            // preedit often coincides with a candidate: a raw/spell candidate
            // (typing "wo" offers a "wo" candidate) or the committed sentence
            // in CommitPreview mode. Only decorate real candidates.
            if (isPreeditText(inputContext, text)) {
                return;
            }
            if (!isCandidateText(inputContext, text)) {
                return;
            }
            const auto word = text.toString();
            auto gloss = dictionary->lookup(word, config_.maxGlosses.value(),
                                            config_.separator.value());
            // Keep the hint line for candidates without a translation, as long
            // as some sibling candidate has one, so that all candidates stay
            // aligned on two lines.
            if (gloss.empty() &&
                !anySiblingHasHint(inputContext, *dictionary)) {
                return;
            }

            // Rebuild the text as two lines: English hint on top, the original
            // candidate below. classicui renders multi line candidate text
            // stacked vertically.
            fcitx::Text newText;
            newText.append(gloss + "\n", fcitx::TextFormatFlag::Italic);
            for (size_t i = 0, e = text.size(); i < e; ++i) {
                newText.append(text.stringAt(i), text.formatAt(i));
            }
            text = std::move(newText);
        });

    eventHandler_ = instance_->watchEvent(
        fcitx::EventType::InputContextKeyEvent,
        fcitx::EventWatcherPhase::Default, [this](fcitx::Event &event) {
            auto &keyEvent = static_cast<fcitx::KeyEvent &>(event);
            if (keyEvent.isRelease()) {
                return;
            }
            auto *inputContext = keyEvent.inputContext();
            const auto hotkey = config_.hotkey.value();
            if (!hotkey.empty() && keyEvent.key().checkKeyList(hotkey)) {
                toggle(inputContext);
                keyEvent.filterAndAccept();
            }
        });

    loadDictionary();
}

Enhint::~Enhint() {
    if (loaderThread_.joinable()) {
        loaderThread_.join();
    }
}

void Enhint::loadDictionary() {
    loaderThread_ = std::thread([this] {
        static const std::vector<std::string> kRelativePaths = {
            "enhint/cedict_ts.u8",
            "enhint/cedict.u8",
            "enhint/cedict.txt",
        };
        for (const auto &relative : kRelativePaths) {
            auto path = fcitx::StandardPaths::global().locate(
                fcitx::StandardPathsType::PkgData, relative);
            if (path.empty()) {
                continue;
            }
            auto dictionary = std::make_shared<Cedict>();
            if (dictionary->load(path.string())) {
                FCITX_INFO() << "enhint: loaded " << dictionary->size()
                             << " entries from " << path;
                cedict_.store(std::move(dictionary));
                return;
            }
        }
        FCITX_WARN() << "enhint: no CC-CEDICT dictionary found. Drop "
                        "cedict_ts.u8 into ~/.local/share/fcitx5/enhint/";
    });
}

bool Enhint::isCandidateText(fcitx::InputContext *inputContext,
                             const fcitx::Text &text) const {
    const auto *entry = instance_->inputMethodEntry(inputContext);
    if (!entry || entry->addon() != "pinyin") {
        return false;
    }
    auto candidateList = inputContext->inputPanel().candidateList();
    if (!candidateList) {
        return false;
    }
    const auto string = text.toString();
    // Already decorated (or otherwise multi line); never double process.
    if (string.find('\n') != std::string::npos) {
        return false;
    }
    for (int i = 0, e = candidateList->size(); i < e; ++i) {
        const auto &candidate = candidateList->candidate(i);
        if (candidate.isPlaceHolder()) {
            continue;
        }
        if (candidate.text().toString() == string) {
            return true;
        }
    }
    return false;
}

bool Enhint::isPreeditText(fcitx::InputContext *inputContext,
                           const fcitx::Text &text) const {
    const auto &panel = inputContext->inputPanel();
    const auto string = text.toString();
    return (!panel.preedit().empty() && panel.preedit().toString() == string) ||
           (!panel.clientPreedit().empty() &&
            panel.clientPreedit().toString() == string);
}

bool Enhint::anySiblingHasHint(fcitx::InputContext *inputContext,
                               const Cedict &dictionary) const {
    auto candidateList = inputContext->inputPanel().candidateList();
    if (!candidateList) {
        return false;
    }
    for (int i = 0, e = candidateList->size(); i < e; ++i) {
        const auto &candidate = candidateList->candidate(i);
        if (candidate.isPlaceHolder()) {
            continue;
        }
        if (!dictionary.lookup(candidate.text().toString(), 1, " ").empty()) {
            return true;
        }
    }
    return false;
}

void Enhint::toggle(fcitx::InputContext *inputContext) {
    config_.enabled.setValue(!config_.enabled.value());
    save();
    toggleAction_.update(inputContext);
    inputContext->updateUserInterface(
        fcitx::UserInterfaceComponent::InputPanel, true);
    inputContext->updatePreedit();
}

void Enhint::reloadConfig() {
    readAsIni(config_, "conf/enhint.conf");
    toggleAction_.setHotkey(config_.hotkey.value());
}

void Enhint::save() { safeSaveAsIni(config_, "conf/enhint.conf"); }

void Enhint::setConfig(const fcitx::RawConfig &config) {
    config_.load(config, true);
    save();
    toggleAction_.setHotkey(config_.hotkey.value());
}

} // namespace enhint

class EnhintModuleFactory : public fcitx::AddonFactory {
    fcitx::AddonInstance *create(fcitx::AddonManager *manager) override {
        fcitx::registerDomain("fcitx5-enhint", FCITX_INSTALL_LOCALEDIR);
        return new enhint::Enhint(manager->instance());
    }
};

FCITX_ADDON_FACTORY_V2(enhint, EnhintModuleFactory);
