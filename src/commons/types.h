// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef COMMONS_TYPES_H
#define COMMONS_TYPES_H

// hash for unordered_set and unordered_map
template <typename TT>
struct UnorderedHash {
    size_t operator()(TT const& tt) const { return std::hash<TT>()(tt); }
};

#endif //COMMONS_TYPES_H