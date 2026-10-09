/*
 * SPDX-FileCopyrightText: 2025 fcitx5-enhint contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _ENHINT_ENHINT_H_
#define _ENHINT_ENHINT_H_

#include "cedict.h"

#include <fcitx-config/configuration.h>
#include <fcitx-config/iniparser.h>
#include <fcitx-utils/handlertable.h>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/signals.h>
#include <fcitx/action.h>
#include <fcitx/addoninstance.h>
#include <fcitx/instance.h>
#include <atomic>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

FCITX_CONFIGURATION(
    EnhintConfig,
    fcitx::Option<bool> enabled{
        this, "Enabled", _("Show English hints above candidates"), true};
    fcitx::Option<int, fcitx::IntConstrain> maxGlosses{
        this, "MaxGlosses", _("Maximum number of English glosses"), 2,
        fcitx::IntConstrain(1, 8)};
    fcitx::Option<std::string> separator{
        this, "Separator", _("Separator between glosses"), ", "};
    fcitx::Option<std::vector<std::string>> skipPrograms{
        this, "SkipPrograms",
        _("Programs that must not be decorated (they may insert the hint)"),
        {}};
    fcitx::Option<fcitx::KeyList> hotkey{
        this, "Hotkey", _("Toggle key"), {}};);

namespace fcitx {
class InputContext;
class Text;
} // namespace fcitx

namespace enhint {

class Enhint final : public fcitx::AddonInstance {
    class ToggleAction : public fcitx::Action {
    public:
        ToggleAction(Enhint *parent) : parent_(parent) {}

        std::string shortText(fcitx::InputContext * /*ic*/) const override {
            return parent_->config_.enabled.value()
                       ? _("English hints: on")
                       : _("English hints: off");
        }

        std::string icon(fcitx::InputContext * /*ic*/) const override {
            return parent_->config_.enabled.value() ? "fcitx-enhint-on"
                                                    : "fcitx-enhint-off";
        }

        void activate(fcitx::InputContext *ic) override {
            parent_->toggle(ic);
        }

    private:
        Enhint *parent_;
    };

public:
    explicit Enhint(fcitx::Instance *instance);
    ~Enhint() override;

    void reloadConfig() override;
    void save() override;
    const fcitx::Configuration *getConfig() const override { return &config_; }
    void setConfig(const fcitx::RawConfig &config) override;

private:
    void loadDictionary();
    bool isCandidateText(fcitx::InputContext *ic,
                         const fcitx::Text &text) const;
    bool anySiblingHasHint(fcitx::InputContext *ic,
                           const Cedict &dictionary) const;
    void toggle(fcitx::InputContext *ic);

    fcitx::Instance *instance_;
    EnhintConfig config_;
    // Built on a background thread and published once ready, so that loading
    // the ~200k entry dictionary does not block fcitx5 startup.
    std::atomic<std::shared_ptr<const Cedict>> cedict_{};
    std::thread loaderThread_;
    std::set<std::string> clientSideLogged_;
    fcitx::ScopedConnection outputFilterConn_;
    std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>>
        eventHandler_;
    ToggleAction toggleAction_{this};
};

} // namespace enhint

#endif // _ENHINT_ENHINT_H_
