// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_CDPDB_H
#define PERSIST_CDPDB_H

#include "commons/uint256.h"
#include "commons/leb128.h"
#include "entities/cdp.h"
#include "dbaccess.h"
#include "dbiterator.h"

#include <map>
#include <set>
#include <string>
#include <cstdint>

using namespace std;

/*  CCompositeKVCache     prefixType        key                  value           variable  */
/*  ----------------   --------------      -----------------    --------------   -----------*/
// cdpr{$Ratio}{$height}{$cdpid} -> CUserCDP
// height: allows data of the same ratio to be sorted by height, from low to high
typedef CCompositeKVCache<dbk::CDP_RATIO_INDEX, tuple<CCdpCoinPair, CFixedUInt64, CFixedUInt64, uint256>, CUserCDP>  CCdpRatioIndexCache;
// cdpr{$height}{$cdpid} -> CUserCDP
// cdp sort by height, from low to high
typedef CCompositeKVCache<dbk::CDP_HEIGHT_INDEX, tuple<CCdpCoinPair, CFixedUInt64, uint256>, CUserCDP> CCdpHeightIndexCache;

class CDBCdpHeightIndexIt: public CDbIterator<CCdpHeightIndexCache> {
public:
    typedef CDbIterator<CCdpHeightIndexCache> Base;
    using Base::Base;

    uint64_t GetHeight() const {
        return std::get<1>(GetKey()).value;
    }
    const uint256& GetCdpId() const {
        return std::get<2>(GetKey());
    }
};

class CCdpDBCache {
public:
    CCdpDBCache() {}
    CCdpDBCache(CDBAccess *pDbAccess);
    CCdpDBCache(CCdpDBCache *pBaseIn);

    bool NewCDP(const int32_t blockHeight, CUserCDP &cdp);
    bool EraseCDP(const CUserCDP &oldCDP, const CUserCDP &cdp);
    bool UpdateCDP(const CUserCDP &oldCDP, const CUserCDP &newCDP);

    bool UserHaveCdp(const CRegID &regId, const TokenSymbol &assetSymbol, const TokenSymbol &scoinSymbol);
    bool GetCDPList(const CRegID &regId, vector<CUserCDP> &cdpList);
    bool GetCDP(const uint256 cdpid, CUserCDP &cdp);

    bool GetCdpListByCollateralRatio(const CCdpCoinPair &cdpCoinPair, const uint64_t collateralRatio,
            const uint64_t bcoinMedianPrice, CCdpRatioIndexCache::Map &userCdps);


    shared_ptr<CDBCdpHeightIndexIt> CreateCdpHeightIndexIt() {
        return make_shared<CDBCdpHeightIndexIt>(cdp_height_index_cache);
    }

    inline uint64_t GetGlobalStakedBcoins() const;
    inline uint64_t GetGlobalOwedScoins() const;
    CCdpGlobalData GetCdpGlobalData(const CCdpCoinPair &cdpCoinPair) const;

    bool GetCdpBcoin(const TokenSymbol &bcoinSymbol, CCdpBcoinDetail &cdpBcoin);
    bool IsCdpBcoinActivated(const TokenSymbol &bcoinSymbol);
    bool SetCdpBcoin(const TokenSymbol &bcoinSymbol, const CCdpBcoinDetail &cdpBcoin);

    void SetBaseViewPtr(CCdpDBCache *pBaseIn);
    void SetDbOpLogMap(CDBOpLogMap * pDbOpLogMapIn);

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        cdp_global_data_cache.RegisterUndoFunc(undoDataFuncMap);
        cdp_cache.RegisterUndoFunc(undoDataFuncMap);
        bcoin_status_cache.RegisterUndoFunc(undoDataFuncMap);
        user_cdp_cache.RegisterUndoFunc(undoDataFuncMap);
        cdp_ratio_index_cache.RegisterUndoFunc(undoDataFuncMap);
        cdp_height_index_cache.RegisterUndoFunc(undoDataFuncMap);
    }

    uint32_t GetCacheSize() const;
    bool Flush();
private:
    bool SaveCDPToDB(const CUserCDP &cdp);
    bool EraseCDPFromDB(const CUserCDP &cdp);

    bool SaveCdpIndexData(const CUserCDP &userCdp);
    bool EraseCdpIndexData(const CUserCDP &userCdp);

    CCdpRatioIndexCache::KeyType MakeCdpRatioIndexKey(const CUserCDP &cdp);
    CCdpHeightIndexCache::KeyType MakeCdpHeightIndexKey(const CUserCDP &cdp);
public:
    /*  CCompositeKVCache  prefixType       key                            value             variable  */
    /*  ---------------- --------------   ------------                --------------    ----- --------*/
    // cdpCoinPair -> total staked assets
    CCompositeKVCache<  dbk::CDP_GLOBAL_DATA, CCdpCoinPair,   CCdpGlobalData>    cdp_global_data_cache;
    // cdp{$cdpid} -> CUserCDP
    CCompositeKVCache<  dbk::CDP,       uint256,                    CUserCDP>           cdp_cache;
    // cbca{$bcoin_symbol} -> $cdpBcoinDetail
    CCompositeKVCache<  dbk::CDP_BCOIN, TokenSymbol,    CCdpBcoinDetail>           bcoin_status_cache;
    // ucdp${CRegID}{$cdpCoinPair} -> set<cdpid>
    CCompositeKVCache<  dbk::USER_CDP, pair<CRegIDKey, CCdpCoinPair>, optional<uint256>> user_cdp_cache;
    // cdpr{Ratio}{$cdpid} -> CUserCDP
    CCdpRatioIndexCache          cdp_ratio_index_cache;
    CCdpHeightIndexCache         cdp_height_index_cache;
};

enum CDPCloseType: uint8_t {
    BY_REDEEM = 0,
    BY_MANUAL_LIQUIDATE,
    BY_FORCE_LIQUIDATE
};

string GetCdpCloseTypeName(const CDPCloseType type);

class CClosedCdpDBCache {
public:
    CClosedCdpDBCache() {}

    CClosedCdpDBCache(CDBAccess *pDbAccess) : closedCdpTxCache(pDbAccess), closedTxCdpCache(pDbAccess) {}

    CClosedCdpDBCache(CClosedCdpDBCache *pBaseIn)
        : closedCdpTxCache(&pBaseIn->closedCdpTxCache), closedTxCdpCache(&pBaseIn->closedTxCdpCache) {}

public:
    bool AddClosedCdpIndex(const uint256& closedCdpId, const uint256& closedCdpTxId, CDPCloseType closeType) {
        return closedCdpTxCache.SetData(closedCdpId, {closedCdpTxId, (uint8_t)closeType});
    }

    bool AddClosedCdpTxIndex(const uint256& closedCdpTxId, const uint256& closedCdpId, CDPCloseType closeType) {
        return  closedTxCdpCache.SetData(closedCdpTxId, {closedCdpId, closeType});
    }

    bool GetClosedCdpById(const uint256& closedCdpId, std::pair<uint256, uint8_t>& cdp) {
        return closedCdpTxCache.GetData(closedCdpId, cdp);
    }

    bool GetClosedCdpByTxId(const uint256& closedCdpTxId, std::pair<uint256, uint8_t>& cdp) {
        return closedTxCdpCache.GetData(closedCdpTxId, cdp);
    }

    uint32_t GetCacheSize() const { return closedCdpTxCache.GetCacheSize() + closedTxCdpCache.GetCacheSize(); }

    void SetBaseViewPtr(CClosedCdpDBCache *pBaseIn) {
        closedCdpTxCache.SetBase(&pBaseIn->closedCdpTxCache);
        closedTxCdpCache.SetBase(&pBaseIn->closedTxCdpCache);
    }

    void Flush() {
        closedCdpTxCache.Flush();
        closedTxCdpCache.Flush();
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        closedCdpTxCache.SetDbOpLogMap(pDbOpLogMapIn);
        closedTxCdpCache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        closedCdpTxCache.RegisterUndoFunc(undoDataFuncMap);
        closedTxCdpCache.RegisterUndoFunc(undoDataFuncMap);
    }
public:
    /*  CCompositeKVCache     prefixType     key               value             variable  */
    /*  ----------------   --------------   ------------   --------------    ----- --------*/
    // ccdp${closed_cdpid} -> <closedCdpTxId, closeType>
    CCompositeKVCache< dbk::CLOSED_CDP_TX, uint256, std::pair<uint256, uint8_t> > closedCdpTxCache;
    // ctx${$closed_cdp_txid} -> <closedCdpId, closeType> (no-force-liquidation)
    CCompositeKVCache< dbk::CLOSED_TX_CDP, uint256, std::pair<uint256, uint8_t> > closedTxCdpCache;
};

#endif  // PERSIST_CDPDB_H
