// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef COMMONS_TYPES_H
#define COMMONS_TYPES_H

#include <utility>
#include <unordered_set>
#include <unordered_map>

// pair<> hash for unordered_set and unordered_map
template <typename T1, typename T2>
struct UnorderedPairHash {
    size_t operator()(const std::pair<T1, T2> &value) const {
        return std::hash<T1>()(std::get<0>(value)) ^ std::hash<T2>()(std::get<1>(value));
    }
};

// T1 and T2 must be the basic type(int, string ...)
template <class T1, class T2, class _Hash = UnorderedPairHash<T1, T2>>
using UnorderedPairSet = std::unordered_set<std::pair<T1, T2>, _Hash>;

template <typename T>
struct EnumTypeHash {
    size_t operator()(const T& type) const noexcept { return std::hash<int>{}(type); }
};


template <typename EnumType, typename ValueType>
using EnumTypeMap = std::unordered_map<EnumType, ValueType, EnumTypeHash<EnumType>>;

namespace container {
    template<typename Container>
    void Append(Container dest, Container appended) {
        dest.insert(dest.end(), std::begin(appended), std::end(appended));
    }
};

#endif //COMMONS_TYPES_H
