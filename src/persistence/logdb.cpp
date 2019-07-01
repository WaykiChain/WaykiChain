// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "logdb.h"

bool CLogDBCache::SetExecuteFail(const int32_t blockHeight, const uint256 txid, const uint8_t errorCode,
                                 const string &errorMessage) {
    return executeFailCache.SetData(std::to_string(blockHeight) + "_" + txid.GetHex(),
                                    std::make_pair(errorCode, errorMessage));
}

bool CLogDBCache::GetExecuteFail(const int32_t blockHeight, vector<std::tuple<uint256, uint8_t, string> > &result) {
    map<string, std::pair<uint8_t, string> > elements;
    if (!executeFailCache.GetAllElements(std::to_string(blockHeight) + "_", elements)) {
        return false;
    }

    size_t prefixLen = string(std::to_string(blockHeight) + "_").size();
    for (const auto &item : elements) {
        result.push_back(std::make_tuple(uint256S(item.first.substr(prefixLen)) /* txid */,
                                         std::get<0>(item.second) /* error code */,
                                         std::get<1>(item.second) /* error message */));
    }

    return true;
}

void CLogDBCache::Flush() {
    executeFailCache.Flush();
}
