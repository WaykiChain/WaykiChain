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
    CSysParamDBCache(CDBAccess *pDbAccess) : sysParamCache(pDbAccess),
                                             minerFeeCache(pDbAccess),
                                             cdpParamCache(pDbAccess),
                                             cdpInterestParamChangesCache(pDbAccess),
                                             currentBpCountCache(pDbAccess),
                                             newBpCountCache(pDbAccess){}

    CSysParamDBCache(CSysParamDBCache *pBaseIn) : sysParamCache(pBaseIn->sysParamCache),
                                                  minerFeeCache(pBaseIn->minerFeeCache),
                                                  cdpParamCache(pBaseIn->cdpParamCache),
                                                  cdpInterestParamChangesCache(pBaseIn->cdpInterestParamChangesCache),
                                                  currentBpCountCache(pBaseIn->currentBpCountCache),
                                                  newBpCountCache(pBaseIn->newBpCountCache){}

    bool GetParam(const SysParamType &paramType, uint64_t& paramValue) {
        if (SysParamTable.count(paramType) == 0)
            return false;

        auto iter = SysParamTable.find(paramType);
        string keyPostfix = std::get<0>(iter->second);
        CVarIntValue<uint64_t > value ;
        if (!sysParamCache.GetData(paramType, value)) {
            paramValue = std::get<1>(iter->second);
        } else{
            paramValue = value.get();
        }

        return true;
    }

    bool GetCdpParam(const CCdpCoinPair& coinPair, const CdpParamType &paramType, uint64_t& paramValue) {
        if (CdpParamTable.count(paramType) == 0)
            return false;

        auto iter = CdpParamTable.find(paramType);
        auto key = std::make_pair(coinPair, paramType);
        CVarIntValue<uint64_t > value ;
        if (!cdpParamCache.GetData(key, value)) {
            paramValue = std::get<1>(iter->second);
        } else{
            paramValue = value.get();
        }

        return true;
    }

    bool Flush() {
        sysParamCache.Flush();
        minerFeeCache.Flush();
        return true;
    }

    uint32_t GetCacheSize() const { return sysParamCache.GetCacheSize() + minerFeeCache.GetCacheSize(); }

    void SetBaseViewPtr(CSysParamDBCache *pBaseIn) {
        sysParamCache.SetBase(&pBaseIn->sysParamCache);
        minerFeeCache.SetBase(&pBaseIn->minerFeeCache);
        cdpParamCache.SetBase(&pBaseIn->cdpParamCache);
        cdpInterestParamChangesCache.SetBase(&pBaseIn->cdpInterestParamChangesCache);
        currentBpCountCache.SetBase(&pBaseIn->currentBpCountCache);
        newBpCountCache.SetBase(&pBaseIn->newBpCountCache);
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        sysParamCache.SetDbOpLogMap(pDbOpLogMapIn);
        minerFeeCache.SetDbOpLogMap(pDbOpLogMapIn);
        cdpParamCache.SetDbOpLogMap(pDbOpLogMapIn);
        cdpInterestParamChangesCache.SetDbOpLogMap(pDbOpLogMapIn);
        currentBpCountCache.SetDbOpLogMap(pDbOpLogMapIn);
        newBpCountCache.SetDbOpLogMap(pDbOpLogMapIn);

    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        sysParamCache.RegisterUndoFunc(undoDataFuncMap);
        minerFeeCache.RegisterUndoFunc(undoDataFuncMap);
        cdpParamCache.RegisterUndoFunc(undoDataFuncMap);
        cdpInterestParamChangesCache.RegisterUndoFunc(undoDataFuncMap);
        currentBpCountCache.RegisterUndoFunc(undoDataFuncMap);
        newBpCountCache.RegisterUndoFunc(undoDataFuncMap);
    }
    bool SetParam(const SysParamType& key, const uint64_t& value){
        return sysParamCache.SetData(key, CVarIntValue(value)) ;
    }

    bool SetCdpParam(const CCdpCoinPair& coinPair, const CdpParamType& paramkey, const uint64_t& value) {
        auto key = std::make_pair(coinPair, paramkey);
        return cdpParamCache.SetData(key, value);
    }
    bool SetMinerFee( const TxType txType, const string feeSymbol, const uint64_t feeSawiAmount) {

        auto pa = std::make_pair(txType, feeSymbol) ;
        return minerFeeCache.SetData(pa , CVarIntValue(feeSawiAmount)) ;

    }

    bool SetCdpInterestParam(CCdpCoinPair& coinPair, CdpParamType paramType, int32_t height , uint64_t value){

        CCdpInterestParamChangeMap changeMap;
        cdpInterestParamChangesCache.GetData(coinPair, changeMap);
        auto &item = changeMap[CVarIntValue(height)];
        if (paramType == CdpParamType::CDP_INTEREST_PARAM_A) {
            item.param_a = value;
        } else if (paramType == CdpParamType::CDP_INTEREST_PARAM_A) {
            item.param_b = value;
        } else {
            assert(false && "must be param_a || param_b");
            return false;
        }
        return cdpInterestParamChangesCache.SetData(coinPair, changeMap) ;
    }

    bool GetCdpInterestParamChanges(const CCdpCoinPair& coinPair, int32_t beginHeight, int32_t endHeight,
            list<CCdpInterestParamChange> &changes) {
        // must validate the coinPair before call this func
        changes.clear();
        CCdpInterestParamChangeMap changeMap;
        cdpInterestParamChangesCache.GetData(coinPair, changeMap);
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
        } else { // found
            changes.push_back({
                beginHeight,
                0,  // will be set later
                beginChangeIt->second.param_a,
                beginChangeIt->second.param_b
            });
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

        auto pa = std::make_pair(txType, feeSymbol) ;
        CVarIntValue<uint64_t > value ;
        bool result =  minerFeeCache.GetData(pa , value) ;

        if(result)
            feeSawiAmount = value.get();
        return result ;
    }


public:
    bool SetNewBpCount(uint8_t newBpCount, uint32_t launchHeight) {
        return newBpCountCache.SetData(std::make_pair(CVarIntValue(launchHeight), newBpCount)) ;
    }
    bool SetCurrentBpCount(uint8_t bpCount) {

        return currentBpCountCache.SetData(bpCount) ;
    }
    uint8_t GetBpCount(uint32_t height) {

        pair<CVarIntValue<uint32_t>,uint8_t> value ;
        if(newBpCountCache.GetData(value)){
            auto launchHeight = std::get<0>(value);
            if(height >= launchHeight.get()) {
                return std::get<1>(value) ;
            }
        }

        uint8_t bpCount ;
        if(currentBpCountCache.GetData(bpCount)){
            return bpCount;
        }

        return 11 ;

    }

public:


/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// SysParamDB
    // order tx id -> active order
    CCompositeKVCache< dbk::SYS_PARAM,     uint8_t,      CVarIntValue<uint64_t> >              sysParamCache;
    CCompositeKVCache< dbk::MINER_FEE,     pair<uint8_t, string>,  CVarIntValue<uint64_t> >              minerFeeCache;
    CCompositeKVCache< dbk::CDP_PARAM,     pair<CCdpCoinPair,uint8_t>, CVarIntValue<uint64_t> >      cdpParamCache;
    // [prefix]cdp_coin_pair -> cdp_interest_param_changes (contain all changes)
    CCompositeKVCache< dbk::CDP_INTEREST_PARAMS, CCdpCoinPair, CCdpInterestParamChangeMap> cdpInterestParamChangesCache;
    CSimpleKVCache<dbk:: BP_COUNT, uint8_t>             currentBpCountCache ;
    CSimpleKVCache<dbk:: NEW_BP_COUNT, pair<CVarIntValue<uint32_t>,uint8_t>>  newBpCountCache ;
};