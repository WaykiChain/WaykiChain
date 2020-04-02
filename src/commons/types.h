// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMONS_TYPES_H
#define COMMONS_TYPES_H

#include <string>
#include <vector>
#include <utility>
#include <unordered_set>
#include <unordered_map>

using namespace std;

typedef std::string TokenSymbol;     //8 chars max, E.g. WICC, WCNY, WICC-01D
typedef std::string TokenName;       //32 chars max, E.g. WaykiChain Coins
typedef std::string CoinUnitName;    //defined in coin unit type table

typedef std::pair<TokenSymbol, TokenSymbol> TradingPair;
typedef std::string AssetSymbol;     //8 chars max, E.g. WICC
typedef std::string PriceSymbol;     //8 chars max, E.g. USD, CNY, EUR, BTC
typedef std::pair<TokenSymbol, TokenSymbol> PriceCoinPair;

using uint128_t = unsigned __int128;

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

template <typename T, typename HashType = int32_t>
struct EnumTypeHash {
    size_t operator()(const T& type) const noexcept { return std::hash<HashType>{}((HashType)type); }
};


template <typename EnumType, typename ValueType, typename HashType = int32_t>
using EnumTypeMap = std::unordered_map<EnumType, ValueType, EnumTypeHash<EnumType, HashType>>;

template <typename EnumType, typename HashType = int32_t>
using EnumUnorderedSet = std::unordered_set<EnumType, EnumTypeHash<EnumType, HashType>>;

namespace container {
    template<typename Container>
    void Append(Container &dest, const Container &appended) {
        dest.insert(dest.end(), std::begin(appended), std::end(appended));
    }
};

typedef uint32_t HeightType;
typedef uint32_t TxIndex;

#endif //COMMONS_TYPES_H
