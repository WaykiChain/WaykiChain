
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef UTIL_ENUM_HELPER_H
#define UTIL_ENUM_HELPER_H

#include "commons/types.h"
#include "commons/util/util.h"

template<typename EnumType, typename EnumBase>
struct EnumHelper {
    EnumTypeMap<EnumType, std::string, EnumBase> enum_map;
    std::unordered_map<std::string, EnumType> name_map;
    bool ignore_case = true;

    EnumHelper(const std::vector<std::pair<EnumType, std::string>> enumPairs, bool ignoreCase = true) {
        ignore_case = ignoreCase;
        for (auto enumPair : enumPairs) {
            std::string name = ignore_case ? StrToUpper(enumPair.second) : enumPair.second;

            assert(enum_map.find(enumPair.first) == enum_map.end());
            enum_map[enumPair.first] = name;

            assert(name_map.find(name) == name_map.end());
            name_map[name] = enumPair.first;
        }
    }

    inline constexpr bool CheckEnum(EnumType enumValue) const {
        return enum_map.find(enumValue) != enum_map.end();
    }

    inline constexpr const std::string& GetName(EnumType enumValue) const {
        auto it = enum_map.find(enumValue);
        if (it != enum_map.end())
            return it->second;
        return EMPTY_STRING;
    }

    inline constexpr bool Parse(const std::string &name, EnumType &enumValue) const {
        typename decltype(name_map)::const_iterator it;
        if (ignore_case) {
            it = name_map.find(StrToUpper(name));

        } else {
            it = name_map.find(name);
        }
        if (it == name_map.end())
            return false;

        enumValue = it->second;
        return true;
    }
};

#endif //UTIL_ENUM_HELPER_H