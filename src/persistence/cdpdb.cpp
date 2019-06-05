// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdpdb.h"
#include "dbconf.h"
#include "leveldbwrapper.h"

bool CCdpCache::SetStakeBcoins(CUserID txUid, uint64_t bcoinsToStake, uint64_t collateralRatio,
                                uint64_t mintedScoins, int blockHeight, CDbOpLog &cdpDbOpLog) {

    string key = dbk::GenDbKey(dbk::CDP, txUid);
    CUserCdp lastCdp;
    if (mapCdps.count(txUid.ToString())) {
        if (!GetData(key, lastCdp)) {
            return ERRORMSG("CCdpCache::SetStakeBcoins : GetData failed.");
        }
    }

    CUserCdp cdp        = lastCdp;
    cdp.lastBlockHeight = cdp.blockHeight;
    cdp.lastOwedScoins  += cdp.totalOwedScoins;
    cdp.blockHeight     = blockHeight;
    cdp.collateralRatio = collateralRatio;
    cdp.mintedScoins    = mintedScoins;
    cdp.totalOwedScoins += cdp.mintedScoins;

    if (!SetData(key, cdp)) {
        return ERRORMSG("CCdpCache::SetStakeBcoins : SetData failed.");
    }

    // TODO: serialize to string
    // cdpDbOpLog.Reset(key, lastCdp);
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

bool CCdpCache::SetData(const string &key, const CUserCdp &value) {
    pBase->SetData(dbk::GenDbKey(dbk::CDP, key), value);
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

bool CCdpDb::GetData(const string &key, CUserCdp &value) {
    //TODO
    return true;
}

bool CCdpDb::SetData(const string &key, const CUserCdp &value) {
    //TODO
    return true;
}
bool CCdpDb::BatchWrite(const map<string, CUserCdp > &mapContractDb) {
    //TODO
    return true;
}
bool CCdpDb::EraseKey(const string &key) {
    //TODO
    return true;
}
bool CCdpDb::HaveData(const string &key) {
    //TODO
    return true;
}