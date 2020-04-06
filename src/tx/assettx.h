// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_ASSET_H
#define TX_ASSET_H

#include "tx.h"

struct CUserIssuedAsset {
    TokenSymbol asset_symbol;       //asset symbol, E.g WICC | WUSD
    CUserID     owner_uid;          //creator or owner user id of the asset
    TokenName   asset_name;         //asset long name, E.g WaykiChain coin
    uint64_t    total_supply;       //boosted by 10^8 for the decimal part, max is 90 billion.
    bool        mintable;           //whether this token can be minted in the future.

    CUserIssuedAsset(): total_supply(0), mintable(false) {}

    CUserIssuedAsset(const TokenSymbol& assetSymbol, const CUserID& ownerUid, const TokenName& assetName,
                    uint64_t totalSupply, bool mintableIn) :
            asset_symbol(assetSymbol),
            owner_uid(ownerUid),
            asset_name(assetName),
            total_supply(totalSupply),
            mintable(mintableIn) {};

    IMPLEMENT_SERIALIZE(
        READWRITE(asset_symbol);
        READWRITE(owner_uid);
        READWRITE(asset_name);
        READWRITE(mintable);
        READWRITE(VARINT(total_supply));

    )

    string ToString() const {
        return strprintf("asset_symbol=%s, asset_name=%s, owner_uid=%s, total_supply=%llu, mintable=%d",
                        asset_symbol, asset_name, owner_uid.ToString(), total_supply, mintable);
    }
};

Object AssetToJson(const CAccountDBCache &accountCache, const CAsset &asset);
Object AssetToJson(const CAccountDBCache &accountCache, const CUserIssuedAsset &asset);

/**
 * Issue a new asset onto Chain
 */
class CUserIssueAssetTx: public CBaseTx {
public:
    CUserIssuedAsset  asset;          // UIA asset
    
public:
    CUserIssueAssetTx() : CBaseTx(ASSET_ISSUE_TX) {};

    CUserIssueAssetTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbol,
                  uint64_t fees, const CUserIssuedAsset &assetIn)
        : CBaseTx(ASSET_ISSUE_TX, txUidIn, validHeightIn, feeSymbol, fees),
          asset(assetIn){}

    ~CUserIssueAssetTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(asset);
        READWRITE(signature);
    )

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid
                   << fee_symbol << VARINT(llFees) << asset;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CUserIssueAssetTx>(*this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};

class CUserUpdateAsset {
public:
    class CNullData {
    public:
        friend bool operator==(const CNullData &a, const CNullData &b) { return true; }
        friend bool operator<(const CNullData &a, const CNullData &b) { return true; }
    };
public:
    enum UpdateType: uint8_t {
        UPDATE_NONE = 0,
        OWNER_UID = 1,
        NAME = 2,
        MINT_AMOUNT = 3,
    };
    typedef boost::variant<CNullData,
        CUserID, // owner_uid
        string,  // name
        uint64_t // mint_amount
    > ValueType;
private:
    UpdateType type; // update type
    ValueType value; // update value

public:
    static std::shared_ptr<UpdateType> ParseUpdateType(const string& str);

    static const string& GetUpdateTypeName(UpdateType type);
public:
    CUserUpdateAsset(): type(UPDATE_NONE), value(CNullData()) {}

    void Set(const CUserID &ownerUid);

    void Set(const string &name);

    void Set(const uint64_t &mintAmount);

    UpdateType GetType() const { return type; }

    template <typename T_Value>
    T_Value &get() {
        return boost::get<T_Value>(value);
    }
    template <typename T_Value>
    const T_Value &get() const {
        return boost::get<T_Value>(value);
    }

public:

    inline unsigned int GetSerializeSize(int serializedType, int nVersion) const {
        switch (type) {
            case OWNER_UID:     return sizeof(uint8_t) + get<CUserID>().GetSerializeSize(serializedType, nVersion);
            case NAME:          return sizeof(uint8_t) + ::GetSerializeSize(get<string>(), serializedType, nVersion);
            case MINT_AMOUNT:   return sizeof(uint8_t) + ::GetSerializeSize(VARINT(get<uint64_t>()), serializedType, nVersion);
            default: break;
        }
        return 0;
    }

    template <typename Stream>
    void Serialize(Stream &s, int serializedType, int nVersion) const {
        s << (uint8_t)type;
        switch (type) {
            case OWNER_UID:     s << get<CUserID>(); break;
            case NAME:          s << get<string>(); break;
            case MINT_AMOUNT:   s << VARINT(get<uint64_t>()); break;
            default: {
                LogPrint(BCLog::ERROR, "CUserUpdateAsset::Serialize(), Invalid Asset update type=%d\n", type);
                throw ios_base::failure("Invalid Asset update type");
            }
        }
    }

    template <typename Stream>
    void Unserialize(Stream &s, int serializedType, int nVersion) {
        s >> ((uint8_t&)type);
        switch (type) {
            case OWNER_UID: {
                CUserID uid;
                s >> uid;
                value = uid;
                break;
            }
            case NAME: {
                string name;
                s >> name;
                value = name;
                break;
            }
            case MINT_AMOUNT: {
                uint64_t mintAmount;
                s >> VARINT(mintAmount);
                value = mintAmount;
                break;
            }
            default: {
                LogPrint(BCLog::ERROR, "CUserUpdateAsset::Unserialize(), Invalid Asset update type=%d\n", type);
                throw ios_base::failure("Invalid Asset update type");
            }
        }
    }

    string ValueToString() const;

    string ToString(const CAccountDBCache &accountCache) const;

    Object ToJson(const CAccountDBCache &accountCache) const;
};

/**
 * Update an existing asset from Chain
 */
class CUserUpdateAssetTx: public CBaseTx {
public:
    TokenSymbol asset_symbol;       // symbol of asset that needs to be updated
    CUserUpdateAsset update_data;   // update data(type, value)
    
public:
    CUserUpdateAssetTx() : CBaseTx(UIA_UPDATE_TX) {}

    CUserUpdateAssetTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbolIn,
                   uint64_t feesIn, const TokenSymbol &assetSymbolIn, const CUserUpdateAsset &updateData)
        : CBaseTx(UIA_UPDATE_TX, txUidIn, validHeightIn, feeSymbolIn, feesIn),
          asset_symbol(assetSymbolIn),
          update_data(updateData) {}

    ~CUserUpdateAssetTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(asset_symbol);
        READWRITE(update_data);
        READWRITE(signature);
    )

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid
                   << fee_symbol << VARINT(llFees) << asset_symbol << update_data;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CUserUpdateAssetTx>(*this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);

};

#endif //TX_ASSET_H