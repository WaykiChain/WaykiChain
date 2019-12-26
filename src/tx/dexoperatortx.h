// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_DEX_OPERATOR_TX_H
#define TX_DEX_OPERATOR_TX_H

#include "entities/asset.h"
#include "entities/dexorder.h"
#include "tx.h"
#include "persistence/dexdb.h"


class CDEXOperatorRegisterTx: public CBaseTx {
public:
    struct Data {
        CUserID owner_uid;                   // owner uid of exchange
        CUserID match_uid;                   // match uid
        string name              = "";       // domain name
        string portal_url        = "";
        uint64_t maker_fee_ratio = 0;
        uint64_t taker_fee_ratio = 0;
        string memo              = "";

        IMPLEMENT_SERIALIZE(
            READWRITE(owner_uid);
            READWRITE(match_uid);
            READWRITE(name);
            READWRITE(portal_url);
            READWRITE(VARINT(maker_fee_ratio));
            READWRITE(VARINT(taker_fee_ratio));
            READWRITE(memo);
        )
    };
public:
    Data data;
public:
    CDEXOperatorRegisterTx() : CBaseTx(DEX_OPERATOR_REGISTER_TX) {};

    CDEXOperatorRegisterTx(const CUserID &txUidIn, int32_t validHeightIn,
                           const TokenSymbol &feeSymbol, uint64_t fees,
                           const Data &dataIn)
        : CBaseTx(DEX_OPERATOR_REGISTER_TX, txUidIn, validHeightIn, feeSymbol, fees), data(dataIn) {
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(data);
        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
               << data;
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXOperatorRegisterTx>(*this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};


class CDEXOperatorUpdateData{

public:
    enum UpdateField: uint8_t{
        UPDATE_NONE     = 0 ,
        MATCH_UID       = 1 ,
        NAME            = 2 ,
        PORTAL_URL      = 3 ,
        MAKER_FEE_RATIO = 4 ,
        TAKER_FEE_RATIO = 5 ,
        OWNER_UID       = 6 ,
        MEMO            = 7
    };

public:
    int32_t dexId = -1 ;
    uint8_t field = UPDATE_NONE  ;
    string value = "";

    bool IsEmpty(){ return dexId == -1 || field == UPDATE_NONE || field > MEMO ; }

    IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(dexId));
            READWRITE(field);
            READWRITE(value);
            )

    bool Check(string& errmsg, string& errcode, uint32_t currentHeight) ;

    bool UpdateToDexOperator(DexOperatorDetail& detail,CCacheWrapper& cw) ;
    bool GetRegID(CCacheWrapper& cw,CRegID& regid) ;

};

class CDEXOperatorUpdateTx: public CBaseTx {

public:
    CDEXOperatorUpdateData update_data ;
public:
    CDEXOperatorUpdateTx() : CBaseTx(DEX_OPERATOR_UPDATE_TX) {}

    CDEXOperatorUpdateTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbolIn,
                   uint64_t feesIn, const CDEXOperatorUpdateData &updateData)
            : CBaseTx(DEX_OPERATOR_UPDATE_TX, txUidIn, validHeightIn, feeSymbolIn, feesIn),
            update_data(updateData) {}

    ~CDEXOperatorUpdateTx() {}

    IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(this->nVersion));
            nVersion = this->nVersion;
            READWRITE(VARINT(valid_height));
            READWRITE(txUid);
            READWRITE(fee_symbol);
            READWRITE(VARINT(llFees));
            READWRITE(update_data);
            READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
               << update_data;
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual TxID GetHash() const { return ComputeSignatureHash(); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CDEXOperatorUpdateTx>(*this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};




#endif  // TX_DEX_OPERATOR_TX_H