// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdpdb.h"

bool CCdpCache::SetStakeBcoins(CUserID txUid, uint64_t bcoinsToStake, int blockHeight, CDbOpLog &cdpDbOpLog) {
    //TODO
    return true;
}

bool CCdpCache::GetUnderLiquidityCdps(vector<CUserCdp> & userCdps) {
    //TODO
    return true;
}

bool CCdpCache::GetData(const string &key, CUserCdp &value) {
    if (mapCdps.count(key) > 0) {
        if (!mapCdps[key].IsEmpty()) {
            value = mapCdps[key];
            return true;
        } else {
            return false;
        }
    }

    if (!pBase->GetData(key, value)) {
        return false;
    }

    mapCdps[key] = value;  //cache it here for speed in-mem access
    return true;
}

bool CCdpCache::SetData(const string &vKey, const CUserCdp &value) {
    //TODO
    return true;
}

bool CCdpCache::BatchWrite(const map<string, CUserCdp> &mapContractDb) {
    //TODO
    return true;
}

bool CCdpCache::EraseKey(const string &vKey) {
    //TODO
    return true;
}

bool CCdpCache::HaveData(const string &vKey) {
    //TODO
    return true;
}


CCdpDb::CCdpDb(const string &name, size_t nCacheSize, bool fMemory, bool fWipe)
    : db(GetDataDir() / "blocks" / name, nCacheSize, fMemory, fWipe) {}

CCdpDb::CCdpDb(size_t nCacheSize, bool fMemory, bool fWipe)
    : db(GetDataDir() / "blocks" / "cdp", nCacheSize, fMemory, fWipe) {}