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

class CSysParamDBCache {
public:
    CSysParamDBCache() {}
    CSysParamDBCache(CDBAccess *pDbAccess) : sysParamCache(pDbAccess),
                                             minerFeeCache(pDbAccess),
                                             cdpParamCache(pDbAccess),
                                             cdpABParamChangeCache(pDbAccess){}
    CSysParamDBCache(CSysParamDBCache *pBaseIn) : sysParamCache(pBaseIn->sysParamCache),
                                                  minerFeeCache(pBaseIn->minerFeeCache),
                                                  cdpParamCache(pBaseIn->cdpParamCache),
                                                  cdpABParamChangeCache(){}

    bool GetParam(const SysParamType &paramType, uint64_t& paramValue) {
        if (SysParamTable.count(paramType) == 0)
            return false;

        auto iter = SysParamTable.find(paramType);
        string keyPostfix = std::get<0>(iter->second);
        CVarIntValue<uint64_t > value ;
        if (!sysParamCache.GetData(keyPostfix, value)) {
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
        string keyPostfix = std::get<0>(iter->second);
        auto key = std::make_pair(coinPair, keyPostfix);
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
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        sysParamCache.SetDbOpLogMap(pDbOpLogMapIn);
        minerFeeCache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        sysParamCache.RegisterUndoFunc(undoDataFuncMap);
        minerFeeCache.RegisterUndoFunc(undoDataFuncMap);
    }
    bool SetParam(const string& key, const uint64_t& value){
        return sysParamCache.SetData(key, CVarIntValue(value)) ;
    }

    bool SetCdpParam(const CCdpCoinPair& coinPair, const string& paramkey, const uint64_t& value) {
        auto key = std::make_pair(coinPair,paramkey);
        return cdpParamCache.SetData(key, value);
    }
    bool SetMinerFee( const uint8_t txType, const string feeSymbol, const uint64_t feeSawiAmount) {

        auto pa = std::make_pair(txType, feeSymbol) ;
        return minerFeeCache.SetData(pa , CVarIntValue(feeSawiAmount)) ;

    }

    bool SetABParamChange(CCdpCoinPair& coinPair, string paramkey, int32_t& height , uint64_t& value){
        auto vvalue = CVarIntValue(value) ;
        auto key  = std::make_pair(coinPair, paramkey) ;
        map<CVarIntValue<int32_t>, CVarIntValue<uint64_t>> heightValueMap ;
        cdpABParamChangeCache.GetData(key, heightValueMap);
        heightValueMap[ CVarIntValue(height)] = CVarIntValue(value) ;
        return cdpABParamChangeCache.SetData(key, heightValueMap) ;
    }

    bool GetCdpABParamMap(CCdpCoinPair& coinPair, const string paramKey, map<CVarIntValue<int32_t>,CVarIntValue<uint64_t>>& heightValueMap){

        auto key = std::make_pair(coinPair, paramKey) ;
        return cdpABParamChangeCache.GetData(key, heightValueMap) ;
    }

    bool GetMinerFee( const uint8_t txType, const string feeSymbol, uint64_t& feeSawiAmount) {

        auto pa = std::make_pair(txType, feeSymbol) ;
        CVarIntValue<uint64_t > value ;
        bool result =  minerFeeCache.GetData(pa , value) ;

        if(result)
            feeSawiAmount = value.get();
        return result ;
    }


private:
/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// SysParamDB
    // order tx id -> active order
    CCompositeKVCache< dbk::SYS_PARAM,     string,      CVarIntValue<uint64_t> >              sysParamCache;
    CCompositeKVCache< dbk::MINER_FEE,     pair<uint8_t, string>,  CVarIntValue<uint64_t> >              minerFeeCache;
    CCompositeKVCache< dbk::CDP_PARAM,     pair<CCdpCoinPair,string>, CVarIntValue<uint64_t> >      cdpParamCache;
    CCompositeKVCache< dbk::INTEREST_HISTORY, pair<CCdpCoinPair,string>,
                                     map<CVarIntValue<int32_t>,CVarIntValue<uint64_t>>> cdpABParamChangeCache ;

};