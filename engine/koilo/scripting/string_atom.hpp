// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file string_atom.hpp
 * @brief String interning table for fast identifier comparison.
 *
 * Interns all identifier strings (variable names, method names, field names)
 * so comparison is integer equality instead of hash+strcmp. Each unique string
 * is stored once and referenced by a 32-bit atom ID.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include "../registry/reflect_macros.hpp"

namespace koilo {
namespace scripting {

using StringAtom = uint32_t;
static constexpr StringAtom INVALID_ATOM = UINT32_MAX;

class AtomTable {
public:
    StringAtom Intern(const std::string& s) {
        auto it = table_.find(s);
        if (it != table_.end()) return it->second;
        StringAtom id = static_cast<StringAtom>(strings_.size());
        strings_.push_back(s);
        table_[s] = id;
        return id;
    }

    const std::string& GetString(StringAtom atom) const {
        static const std::string empty;
        if (atom >= strings_.size()) return empty;
        return strings_[atom];
    }

    bool Contains(const std::string& s) const {
        return table_.count(s) > 0;
    }

    StringAtom Find(const std::string& s) const {
        auto it = table_.find(s);
        return (it != table_.end()) ? it->second : INVALID_ATOM;
    }

    size_t Size() const { return strings_.size(); }

private:
    std::unordered_map<std::string, StringAtom> table_;
    std::vector<std::string> strings_;

    KL_BEGIN_FIELDS(AtomTable)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(AtomTable)
        KL_METHOD_AUTO(AtomTable, Intern, "Intern"),
        KL_METHOD_AUTO(AtomTable, GetString, "Get string"),
        KL_METHOD_AUTO(AtomTable, Contains, "Contains"),
        KL_METHOD_AUTO(AtomTable, Find, "Find"),
        KL_METHOD_AUTO(AtomTable, Size, "Size")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AtomTable)
        /* No reflected ctors. */
    KL_END_DESCRIBE(AtomTable)

};

} // namespace scripting
} // namespace koilo
