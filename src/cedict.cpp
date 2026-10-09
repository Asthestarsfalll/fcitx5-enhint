/*
 * SPDX-FileCopyrightText: 2025 fcitx5-enhint contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "cedict.h"

#include <fstream>
#include <string_view>
#include <vector>

namespace enhint {

namespace {

constexpr std::string_view kNoisePrefixes[] = {
    "CL:",          "variant of ", "old variant of ", "see ",
    "surname ",     "used in ",    "abbr. for ",      "erhua variant of ",
    "Taiwan pr. ",  "Japanese variant of ",
};

bool isNoise(std::string_view gloss) {
    for (auto prefix : kNoisePrefixes) {
        if (gloss.size() >= prefix.size() &&
            gloss.compare(0, prefix.size(), prefix) == 0) {
            return true;
        }
    }
    return false;
}

std::string_view trim(std::string_view sv) {
    while (!sv.empty() && (sv.front() == ' ' || sv.front() == '\t' ||
                           sv.front() == '\r')) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && (sv.back() == ' ' || sv.back() == '\t' ||
                           sv.back() == '\r')) {
        sv.remove_suffix(1);
    }
    return sv;
}

// Split the "/gloss1/gloss2/.../" payload into individual glosses. Within a
// single definition group, the mdbg export uses "; " between near synonyms.
std::vector<std::string_view> splitGlosses(std::string_view raw) {
    std::vector<std::string_view> result;
    auto push = [&result](std::string_view gloss) {
        gloss = trim(gloss);
        if (!gloss.empty()) {
            result.push_back(gloss);
        }
    };
    std::size_t start = 0;
    while (start <= raw.size()) {
        auto end = raw.find('/', start);
        if (end == std::string_view::npos) {
            end = raw.size();
        }
        auto group = raw.substr(start, end - start);
        std::size_t sub = 0;
        while (sub <= group.size()) {
            auto subEnd = group.find(';', sub);
            if (subEnd == std::string_view::npos) {
                subEnd = group.size();
            }
            push(group.substr(sub, subEnd - sub));
            sub = subEnd + 1;
        }
        start = end + 1;
    }
    return result;
}

} // namespace

void Cedict::insert(const std::string &word, const std::string &rawGloss) {
    if (word.empty() || rawGloss.empty()) {
        return;
    }
    // First definition wins, which tends to be the most common one.
    entries_.emplace(word, rawGloss);
}

bool Cedict::load(const std::string &path) {
    loaded_ = false;
    entries_.clear();
    path_.clear();

    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) {
        return false;
    }

    std::string line;
    std::size_t parsed = 0;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        // Format: Traditional Simplified [pin1 yin1] /gloss/gloss/...
        auto firstSpace = line.find(' ');
        if (firstSpace == std::string::npos) {
            continue;
        }
        auto secondSpace = line.find(' ', firstSpace + 1);
        if (secondSpace == std::string::npos) {
            continue;
        }
        auto glossStart = line.find('/', secondSpace + 1);
        if (glossStart == std::string::npos) {
            continue;
        }
        const std::string traditional = line.substr(0, firstSpace);
        const std::string simplified =
            line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
        const std::string rawGloss = line.substr(glossStart);

        insert(simplified, rawGloss);
        if (traditional != simplified) {
            insert(traditional, rawGloss);
        }
        ++parsed;
    }

    if (parsed == 0) {
        entries_.clear();
        return false;
    }

    path_ = path;
    loaded_ = true;
    return true;
}

std::string Cedict::lookup(const std::string &word, int maxGlosses,
                           const std::string &separator) const {
    if (!loaded_ || word.empty()) {
        return {};
    }
    auto iter = entries_.find(word);
    if (iter == entries_.end()) {
        return {};
    }

    auto glosses = splitGlosses(iter->second);
    std::string result;
    int count = 0;
    for (auto gloss : glosses) {
        if (isNoise(gloss)) {
            continue;
        }
        if (!result.empty()) {
            result.append(separator);
        }
        result.append(gloss);
        ++count;
        if (count >= maxGlosses) {
            break;
        }
    }
    return result;
}

} // namespace enhint
