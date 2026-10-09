/*
 * SPDX-FileCopyrightText: 2025 fcitx5-enhint contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _ENHINT_CEDICT_H_
#define _ENHINT_CEDICT_H_

#include <cstddef>
#include <string>
#include <unordered_map>

namespace enhint {

/**
 * A minimal reader for the CC-CEDICT dictionary.
 *
 * Only the mapping from a Chinese word to its (raw) English gloss is kept in
 * memory. The formatting into a short, human friendly string is done lazily on
 * lookup so that config changes take effect immediately.
 */
class Cedict {
public:
    /**
     * Parse the CC-CEDICT file at @p path.
     *
     * @return true on success, false if the file can not be opened.
     */
    bool load(const std::string &path);

    bool loaded() const { return loaded_; }
    std::size_t size() const { return entries_.size(); }
    const std::string &path() const { return path_; }

    /**
     * Return a short translation for @p word, or an empty string if the word
     * is unknown.
     *
     * @param word the Chinese word (UTF-8).
     * @param maxGlosses maximum number of glosses to keep.
     * @param separator separator inserted between glosses.
     */
    std::string lookup(const std::string &word, int maxGlosses,
                       const std::string &separator) const;

private:
    void insert(const std::string &word, const std::string &rawGloss);

    std::unordered_map<std::string, std::string> entries_;
    std::string path_;
    bool loaded_ = false;
};

} // namespace enhint

#endif // _ENHINT_CEDICT_H_
