// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "commons/serialize.h"
#include "persistence/dbaccess.h"
#include "persistence/dbconf.h"

#include <cstdint>
#include <unordered_map>
#include <string>
#include <cstdint>
#include "config/sysparams.h"

using namespace std;

struct CCdpInterestParams {
    uint64_t param_a = 0;
    uint64_t param_b = 0;

    IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(param_a));
            READWRITE(VARINT(param_b));
    )

    string ToString() const {
        return strprintf("param_a=%llu", param_a) + ", " +
        strprintf("param_a=%llu", param_a);

    }
};

typedef map<CVarIntValue<int32_t>, CCdpInterestParams> CCdpInterestParamChangeMap;

struct CCdpInterestParamChange {
    int32_t begin_height = 0;
    int32_t end_height = 0;
    uint64_t param_a = 0;
    uint64_t param_b = 0;
};

class CSysParamDBCache {
public:
    CSysParamDBCache() {}
    CSysParamDBCache(CDBAccess *pDbAccess) : sys_param_chache(pDbAccess),
                                             miner_fee_cache(pDbAccess),
                                             cdp_param_cache(pDbAccess),
                                             cdp_interest_param_changes_cache(pDbAccess),
                                             current_total_bps_size_cache(pDbAccess),
                                             new_total_bps_size_cache(pDbAccess){}

    CSysParamDBCache(CSysParamDBCache *pBaseIn) : sys_param_chache(pBaseIn->sys_param_chache),
                                                  miner_fee_cache(pBaseIn->miner_fee_cache),
                                                  cdp_param_cache(pBaseIn->cdp_param_cache),
                                                  cdp_interest_param_changes_cache(pBaseIn->cdp_interest_param_changes_cache),
                                                  current_total_bps_size_cache(pBaseIn->current_total_bps_size_cache),
                                                  new_total_bps_size_cache(pBaseIn->new_total_bps_size_cache){}

    bool GetParam(const SysParamType &paramType, uint64_t& paramValue) {
        if (kSysParamTable.count(paramType) == 0)
            return false;

        auto iter = kSysParamTable.find(paramType);
        CVarIntValue<uint64_t > value;
        if (!sys_param_chache.GetData(paramType, value)) {
            paramValue = std::get<0>(iter->second);
        } else{
            paramValue = value.get();
        }

        return true;
    }

    bool GetCdpParam(const CCdpCoinPair& coinPair, const CdpParamType &paramType, uint64_t& paramValue) {
        if (kCdpParamTable.count(paramType) == 0)
            return false;

        auto iter = kCdpParamTable.find(paramType);
        auto key = std::make_pair(coinPair, paramType);
        CVarIntValue<uint64_t > value;
        if (!cdp_param_cache.GetData(key, value)) {
            paramValue = std::get<0>(iter->second);
        } else{
            paramValue = value.get();
        }

        return true;
    }

    bool Flush() {
        sys_param_chache.Flush();
        miner_fee_cache.Flush();
        cdp_param_cache.Flush();
        cdp_interest_param_changes_cache.Flush();
        current_total_bps_size_cache.Flush();
        new_total_bps_size_cache.Flush();
        return true;
    }

    uint32_t GetCacheSize() const { return sys_param_chache.GetCacheSize() +
                                    miner_fee_cache.GetCacheSize() +
                cdp_interest_param_changes_cache.GetCacheSize() +
                                    current_total_bps_size_cache.GetCacheSize() +
                                    new_total_bps_size_cache.GetCacheSize(); }

    void SetBaseViewPtr(CSysParamDBCache *pBaseIn) {
        sys_param_chache.SetBase(&pBaseIn->sys_param_chache);
        miner_fee_cache.SetBase(&pBaseIn->miner_fee_cache);
        cdp_param_cache.SetBase(&pBaseIn->cdp_param_cache);
        cdp_interest_param_changes_cache.SetBase(&pBaseIn->cdp_interest_param_changes_cache);
        current_total_bps_size_cache.SetBase(&pBaseIn->current_total_bps_size_cache);
        new_total_bps_size_cache.SetBase(&pBaseIn->new_total_bps_size_cache);
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        sys_param_chache.SetDbOpLogMap(pDbOpLogMapIn);
        miner_fee_cache.SetDbOpLogMap(pDbOpLogMapIn);
        cdp_param_cache.SetDbOpLogMap(pDbOpLogMapIn);
        cdp_interest_param_changes_cache.SetDbOpLogMap(pDbOpLogMapIn);
        current_total_bps_size_cache.SetDbOpLogMap(pDbOpLogMapIn);
        new_total_bps_size_cache.SetDbOpLogMap(pDbOpLogMapIn);

    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        sys_param_chache.RegisterUndoFunc(undoDataFuncMap);
        miner_fee_cache.RegisterUndoFunc(undoDataFuncMap);
        cdp_param_cache.RegisterUndoFunc(undoDataFuncMap);
        cdp_interest_param_changes_cache.RegisterUndoFunc(undoDataFuncMap);
        current_total_bps_size_cache.RegisterUndoFunc(undoDataFuncMap);
        new_total_bps_size_cache.RegisterUndoFunc(undoDataFuncMap);
    }
    bool SetParam(const SysParamType& key, const uint64_t& value){
        return sys_param_chache.SetData(key, CVarIntValue(value));
    }

    bool SetCdpParam(const CCdpCoinPair& coinPair, const CdpParamType& paramkey, const uint64_t& value) {
        auto key = std::make_pair(coinPair, paramkey);
        return cdp_param_cache.SetData(key, value);
    }
    bool SetMinerFee( const TxType txType, const string feeSymbol, const uint64_t feeSawiAmount) {

        auto pa = std::make_pair(txType, feeSymbol);
        return miner_fee_cache.SetData(pa , CVarIntValue(feeSawiAmount));

    }

    bool SetCdpInterestParam(CCdpCoinPair& coinPair, CdpParamType paramType, int32_t height , uint64_t value){

        CCdpInterestParamChangeMap changeMap;
        cdp_interest_param_changes_cache.GetData(coinPair, changeMap);
        auto &item = changeMap[CVarIntValue(height)];
        if (paramType == CdpParamType::CDP_INTEREST_PARAM_A) {
            item.param_a = value;
            uint64_t param_b;
            if(!GetCdpParam(coinPair,CDP_INTEREST_PARAM_B, param_b))
                return false;
            item.param_b = param_b;
        } else if (paramType == CdpParamType::CDP_INTEREST_PARAM_B) {
            item.param_b = value;
            uint64_t param_a;
            if(!GetCdpParam(coinPair,CDP_INTEREST_PARAM_A, param_a))
                return false;
            item.param_a = param_a;
        } else {
            assert(false && "must be param_a || param_b");
            return false;
        }
        return cdp_interest_param_changes_cache.SetData(coinPair, changeMap);
    }

    bool GetCdpInterestParamChanges(const CCdpCoinPair& coinPair, int32_t beginHeight, int32_t endHeight,
            list<CCdpInterestParamChange> &changes) {
        // must validate the coinPair before call this func
        changes.clear();
        CCdpInterestParamChangeMap changeMap;
        cdp_interest_param_changes_cache.GetData(coinPair, changeMap);

        auto it = changeMap.begin();
        auto beginChangeIt = changeMap.end();
        // Find out which change the beginHeight should belong to
        for (; it != changeMap.end(); it++) {
            if (it->first.get() > beginHeight) {
                break;
            }
            beginChangeIt = it;
        }
        // add the first change to the change list, make sure the change list not empty
        if (beginChangeIt == changeMap.end()) { // not found, use default value
            changes.push_back({
                beginHeight,
                0, // will be set later
                GetCdpParamDefaultValue(CDP_INTEREST_PARAM_A),
                GetCdpParamDefaultValue(CDP_INTEREST_PARAM_B)
            });
            beginChangeIt = changeMap.begin();
        } else { // found
            changes.push_back({
                beginHeight,
                0,  // will be set later
                beginChangeIt->second.param_a,
                beginChangeIt->second.param_b
            });

            beginChangeIt++;
        }

        for (it = beginChangeIt; it != changeMap.end(); it++) {
            if (it->first.get() > endHeight)
                break;
            assert(!changes.empty());
            changes.back().end_height = it->first.get() - 1;
            changes.push_back({
                it->first.get(),
                0, // will be set later
                it->second.param_a,
                it->second.param_b
            });
        }
        changes.back().end_height = endHeight;

        return true;
    }

    bool GetMinerFee( const uint8_t txType, const string feeSymbol, uint64_t& feeSawiAmount) {

        auto pa = std::make_pair(txType, feeSymbol);
        CVarIntValue<uint64_t > value;
        bool result =  miner_fee_cache.GetData(pa , value);

        if(result)
            feeSawiAmount = value.get();
        return result;
    }

    bool GetAxcSwapGwRegId(CRegID& regid) {
        uint64_t param;
        if(GetParam(SysParamType::AXC_SWAP_GATEWAY_REGID, param)) {
            regid = CRegID(param);
            return !regid.IsEmpty();
        }
        return false;
    }

    CRegID GetDexMatchSvcRegId() {
        uint64_t param;
        CRegID regid;
        if(GetParam(SysParamType::DEX_MATCH_SVC_REGID, param)) {
            regid = CRegID(param);
            if(regid.IsEmpty()){
                regid = SysCfg().GetDexMatchSvcRegId();
            }
        }
        return regid;
    }
public:
    bool SetNewTotalBpsSize(uint8_t newTotalBpsSize, uint32_t effectiveHeight) {
        return new_total_bps_size_cache.SetData(std::make_pair(CVarIntValue(effectiveHeight), newTotalBpsSize));
    }
    bool SetCurrentTotalBpsSize(uint8_t totalBpsSize) {

        return current_total_bps_size_cache.SetData(totalBpsSize);
    }

    uint8_t GetTotalBpsSize(uint32_t height) {

        pair<CVarIntValue<uint32_t>,uint8_t> value;
        if(new_total_bps_size_cache.GetData(value)){
            auto effectiveHeight = std::get<0>(value);
            if(height >= effectiveHeight.get()) {
                return std::get<1>(value);
            }
        }
        uint8_t totalBpsSize;
        if(current_total_bps_size_cache.GetData(totalBpsSize)){
            return totalBpsSize;
        }

        return IniCfg().GetTotalDelegateNum();
    }

public:


/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// SysParamDB
    // order tx id -> active order
    CCompositeKVCache< dbk::SYS_PARAM,     uint8_t,      CVarIntValue<uint64_t> >               sys_param_chache;
    CCompositeKVCache< dbk::MINER_FEE,     pair<uint8_t, string>,  CVarIntValue<uint64_t> >     miner_fee_cache;
    CCompositeKVCache< dbk::CDP_PARAM,     pair<CCdpCoinPair,uint8_t>, CVarIntValue<uint64_t> > cdp_param_cache;
    // [prefix]cdpCoinPair -> cdp_interest_param_changes (contain all changes)
    CCompositeKVCache< dbk::CDP_INTEREST_PARAMS, CCdpCoinPair, CCdpInterestParamChangeMap>      cdp_interest_param_changes_cache;
    CSimpleKVCache<dbk:: TOTAL_BPS_SIZE, uint8_t>                                               current_total_bps_size_cache;
    CSimpleKVCache<dbk:: NEW_TOTAL_BPS_SIZE, pair<CVarIntValue<uint32_t>,uint8_t>>              new_total_bps_size_cache;
};