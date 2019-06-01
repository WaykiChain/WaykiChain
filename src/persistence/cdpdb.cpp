// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdpdb.h"

bool CCdpCache::SetStakeBcoins(uint64_t bcoinsToStake, int blockHeight, CDBOpLog &cdpDbOpLog) {
    //TODO
    return true;
}

bool CCdpCache::GetLiquidityCdpItems(vector<CdpItem> & cdpItems) {
    //TODO
    return true;
}

bool CCdpCache::GetData(const string &key, vector<unsigned char> &value) {
    if (mapCdps.count(key) > 0) {
        if (!mapCdps[key].empty()) {
            vValue = mapCdps[vKey];
            return true;
        } else {
            return false;
        }
    }

    if (!pBase->GetData(vKey, vValue)) {
        return false;
    }
    mapCdps[vKey] = vValue;  //cache it here for speed in-mem access
    return true;
}

bool CCdpCache::SetData(const string &vKey, const vector<unsigned char> &vValue) {
    //TODO
    return true;
}

bool CCdpCache::BatchWrite(const map<string, vector<unsigned char> > &mapContractDb) {
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