// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_PRICE_MEDIAN_H
#define TX_PRICE_MEDIAN_H

#include "tx.h"
#include "entities/cdp.h"
#include "entities/price.h"
#include "persistence/cdpdb.h"
#include "commons/compat/endian.h"

template<typename MapType, typename ItemSerializer>
class CMapSerializer {
public:
//typedef std::map<PriceCoinPair, uint64_t> PriceMap;
    MapType &map_obj;
    typedef typename MapType::value_type ItemType;

    CMapSerializer(MapType &mapObjIn): map_obj(mapObjIn) {}

    inline uint32_t GetSerializeSize(int nType, int nVersion) const {
        uint32_t sz = GetSizeOfCompactSize(map_obj.size());
        for (auto &item : map_obj) {
            ItemSerializer itemSer(REF(item.first), item.second);
            sz += ::GetSerializeSize(itemSer, nType, nVersion);
        }
        return sz;
    }

    template <typename Stream>
    void Serialize(Stream &s, int nType, int nVersion) const {
        WriteCompactSize(s, map_obj.size());
        for (auto& item : map_obj) {
            ItemSerializer itemSer(REF(item.first), item.second);
            ::Serialize(s, itemSer, nType, nVersion);
        }
    }

    template <typename Stream>
    void Unserialize(Stream &s, int nType, int nVersion) {
        map_obj.clear();
        uint32_t nSize = ReadCompactSize(s);
        for (uint32_t i = 0; i < nSize; i++)
        {
            typename MapType::key_type key;
            typename MapType::mapped_type data;
            ItemSerializer itemSer(key, data);
            ::Unserialize(s, itemSer, nType, nVersion);
            auto retPair = map_obj.emplace(key, data);
            if (!retPair.second) {
                throw runtime_error(strprintf("duplicate key in map when unserialize! key=%s",
                    db_util::ToString(key)));
            }
        }
    }
};

template<typename MapType>
class CMapItemRef {
public:
    typedef typename MapType::key_type key_type;
    typedef typename MapType::mapped_type mapped_type;
    key_type &key;
    mapped_type &value;
    CMapItemRef( key_type &keyIn, mapped_type &valueIn) :
        key(keyIn), value(valueIn) {}
};

using CPriceMapItemRef = CMapItemRef<PriceMap>;

// common serializer for PriceMapItem.
class CPriceMapItemCommonSerializer: public CPriceMapItemRef {
public:
    using CPriceMapItemRef::CPriceMapItemRef;
    IMPLEMENT_SERIALIZE(
        READWRITE(key); // PriceCoinPair
        READWRITE(VARINT(value)); // priceValue
    )
};

// serialize the price value in LE(little-endian) format for compatible old data before V2
class CPriceMapItemLESerializer: public CPriceMapItemRef {
public:
    using CPriceMapItemRef::CPriceMapItemRef;

    IMPLEMENT_SERIALIZE(
        READWRITE(key); // PriceCoinPair
        uint64_t leValue = htole64(value); // priceValue
        READWRITE(leValue);
        if (fRead)
            value = le64toh(leValue);
    )
};

using CPriceMapCommonSerializer = CMapSerializer<PriceMap, CPriceMapItemCommonSerializer>;
using CPriceMapLESerializer = CMapSerializer<PriceMap, CPriceMapItemLESerializer>;

class CBlockPriceMedianTx: public CBaseTx  {
public:
    mutable PriceMap median_prices;

public:
    CBlockPriceMedianTx() : CBaseTx(PRICE_MEDIAN_TX) {}
    CBlockPriceMedianTx(const int32_t validHeight) : CBaseTx(PRICE_MEDIAN_TX) { valid_height = validHeight; }

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        if (GetFeatureForkVersion(valid_height) >= MAJOR_VER_R3) {
            READWRITE(REF(CPriceMapCommonSerializer(median_prices)));
        } else {
            // compatible with old data before V3
            READWRITE(REF(CPriceMapLESerializer(median_prices)));
        }
        // no signature;
    )

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid;
        if (GetFeatureForkVersion(valid_height) >= MAJOR_VER_R3) {
            hw << CPriceMapCommonSerializer(median_prices);
        } else {
            // compatible with old data before V3
            hw << CPriceMapLESerializer(median_prices);
        }
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CBlockPriceMedianTx>(*this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(CCacheWrapper &cw) const;

    bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) { return true; }

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);

public:
    void SetMedianPrices(PriceMap &mapMedianPricesIn) {
        median_prices = mapMedianPricesIn;
    }
private:
    bool ForceLiquidateCdps(CTxExecuteContext &context, PriceDetailMap &priceDetails);
    bool EqualToCalculatedPrices(const PriceDetailMap &calcPrices);
};

class CCdpCoinPairDetail {
public:
    CCdpCoinPair coin_pair;
    bool is_price_active = false;
    bool is_staked_perm = false;
    uint64_t bcoin_price = 0;
    CTxCord init_tx_cord;

    friend bool operator<(const CCdpCoinPairDetail &a, const CCdpCoinPairDetail &b);
};


bool GetCdpCoinPairDetails(CCacheWrapper &cw, HeightType height, const PriceDetailMap &priceDetails, set<CCdpCoinPairDetail> &cdpCoinPairSet);

#endif //TX_PRICE_MEDIAN_H