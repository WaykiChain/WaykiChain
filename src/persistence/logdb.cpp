// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "logdb.h"
#include "config/chainparams.h"

bool CLogDBCache::SetExecuteFail(const int32_t blockHeight, const uint256 txid, const uint8_t errorCode,
                                 const string &errorMessage) {
    if (!SysCfg().IsLogFailures())
        return true;
    return executeFailCache.SetData(make_pair(CFixedUInt32(blockHeight), txid),
                                    std::make_pair(errorCode, errorMessage));
}

void CLogDBCache::Flush() { executeFailCache.Flush(); }
