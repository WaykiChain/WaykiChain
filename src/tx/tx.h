// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef TX_H
#define TX_H

#include <boost/variant.hpp>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "accounts/accounts.h"
#include "commons/uint256.h"
#include "commons/serialize.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace std;

class CTxUndo;
class CValidationState;
class CAccountViewCache;
class CScriptDB;
class CBlock;
class CTransactionDBCache;
class CScriptDBViewCache;
class COperVoteFund;

static const int nTxVersion1 = 1;
static const int nTxVersion2 = 2;

#define SCRIPT_ID_SIZE (6)

enum TxType : unsigned char {
    BLOCK_REWARD_TX     = 1,  //!< Miner Block Reward Tx
    ACCOUNT_REGISTER_TX = 2,  //!< Account Registeration Tx
    BCOIN_TRANSFER_TX   = 3,  //!< BaseCoin Transfer Tx
    CONTRACT_INVOKE_TX  = 4,  //!< Contract Invocation Tx
    CONTRACT_DEPLOY_TX  = 5,  //!< Contract Deployment Tx
    DELEGATE_VOTE_TX    = 6,  //!< Vote Delegate Tx
    COMMON_MTX          = 7,  //!< Multisig Tx

    /******** Begin of Stable Coin TX Type Enums ********/
    CDP_OPEN_TX         = 11,  //!< CDP Collateralize Tx
    CDP_REFUEL_TX       = 12,  //!< CDP Refuel Tx
    CDP_REDEMP_TX       = 13,  //!< CDP Redemption Tx (partial or full)
    CDP_LIQUIDATE_TX    = 14,  //!< CDP Liquidation Tx (partial or full)

    PRICE_FEED_TX       = 21,  //!< Price Feed Tx: WICC/USD | MICC/WUSD | WUSD/USD

    SFC_PARAM_MTX       = 31,  //!< StableCoin Fund Committee invokes Param Set/Update MulSigTx
    SFC_GLOBAL_HALT_MTX = 32,  //!< StableCoin Fund Committee invokes Global Halt CDP Operations MulSigTx
    SFC_GLOBAL_SETTLE_MTX =33,  //!< StableCoin Fund Committee invokes Global Settle Operation MulSigTx

    SCOIN_TRANSFER_TX   = 41,  //!< StableCoin Transfer Tx
    FCOIN_TRANSFER_TX   = 42,  //!< FundCoin Transfer Tx

    DEX_WICC_FOR_MICC_TX = 51,  //!< DEX: owner sells WICC for MICC Tx
    DEX_MICC_FOR_WICC_TX = 52,  //!< DEX: owner sells MICC for WICC Tx
    DEX_WICC_FOR_WUSD_TX = 53,  //!< DEX: owner sells WICC for WUSD Tx
    DEX_WUSD_FOR_WICC_TX = 54,  //!< DEX: owner sells WUSD for WICC Tx
    DEX_MICC_FOR_WUSD_TX = 55,  //!< DEX: owner sells MICC for WUSD Tx
    DEX_WUSD_FOR_MICC_TX = 56,  //!< DEX: owner sells WUSD for MICC Tx
    /******** End of Stable Coin Enums ********/

    NULL_TX = 0 //!< NULL_TX
};

static const unordered_map<unsigned char, string> kTxTypeMap = {
    { BLOCK_REWARD_TX,      "BLOCK_REWARD_TX" },
    { ACCOUNT_REGISTER_TX,  "ACCOUNT_REGISTER_TX" },
    { BCOIN_TRANSFER_TX,    "BCOIN_TRANSFER_TX" },
    { CONTRACT_INVOKE_TX,   "CONTRACT_INVOKE_TX" },
    { CONTRACT_DEPLOY_TX,   "CONTRACT_DEPLOY_TX" },
    { DELEGATE_VOTE_TX,     "DELEGATE_VOTE_TX" },
    { COMMON_MTX,           "COMMON_MTX"},
    { CDP_OPEN_TX,          "CDP_OPEN_TX" },
    { CDP_REFUEL_TX,        "CDP_REFUEL_TX" },
    { CDP_REDEMP_TX,        "CDP_REDEMP_TX" },
    { CDP_LIQUIDATE_TX,     "CDP_LIQUIDATE_TX" },
    { PRICE_FEED_TX,        "PRICE_FEED_TX" },
    { SFC_PARAM_MTX,        "SFC_PARAM_MTX" },
    { SFC_GLOBAL_HALT_MTX,  "SFC_GLOBAL_HALT_MTX" },
    { SFC_GLOBAL_SETTLE_MTX,"SFC_GLOBAL_SETTLE_MTX" },
    { DEX_WICC_FOR_MICC_TX, "DEX_WICC_FOR_MICC_TX" },
    { DEX_MICC_FOR_WICC_TX, "DEX_MICC_FOR_WICC_TX" },
    { DEX_WICC_FOR_WUSD_TX, "DEX_WICC_FOR_WUSD_TX" },
    { DEX_WUSD_FOR_WICC_TX, "DEX_WUSD_FOR_WICC_TX" },
    { DEX_MICC_FOR_WUSD_TX, "DEX_MICC_FOR_WUSD_TX" },
    { DEX_WUSD_FOR_MICC_TX, "DEX_WUSD_FOR_MICC_TX" },
    { NULL_TX,              "NULL_TX" },
};

string GetTxType(unsigned char txType);

class CBaseTx {
public:
    static uint64_t nMinTxFee;
    static uint64_t nMinRelayTxFee;
    static uint64_t nDustAmountThreshold;
    static const int CURRENT_VERSION = nTxVersion1;

    int nVersion;
    unsigned char nTxType;
    mutable CUserID txUid;
    int nValidHeight;
    uint64_t llFees;
    vector_unsigned_char signature;

    uint64_t nRunStep;        //!< only in memory
    int nFuelRate;            //!< only in memory
    mutable uint256 sigHash;  //!< only in memory

public:
    CBaseTx(const CBaseTx &other) { *this = other; }

    CBaseTx(int nVersionIn, TxType nTxTypeIn, CUserID txUidIn, int nValidHeightIn, uint64_t llFeesIn) :
        nVersion(nVersionIn), nTxType(nTxTypeIn), txUid(txUidIn), nValidHeight(nValidHeightIn), llFees(llFeesIn),
        nRunStep(0), nFuelRate(0) {}

    CBaseTx(TxType nTxTypeIn, CUserID txUidIn, int nValidHeightIn, uint64_t llFeesIn) :
        nVersion(CURRENT_VERSION), nTxType(nTxTypeIn), txUid(txUidIn), nValidHeight(nValidHeightIn), llFees(llFeesIn),
        nRunStep(0), nFuelRate(0) {}

    CBaseTx(int nVersionIn, TxType nTxTypeIn) :
        nVersion(nVersionIn), nTxType(nTxTypeIn),
        nValidHeight(0), llFees(0), nRunStep(0), nFuelRate(0) {}

    CBaseTx(TxType nTxTypeIn) :
        nVersion(CURRENT_VERSION), nTxType(nTxTypeIn),
        nValidHeight(0), llFees(0), nRunStep(0), nFuelRate(0) {}

    virtual ~CBaseTx() {}

    virtual uint64_t GetFee() const { return llFees; }
    virtual uint64_t GetFuel(int nfuelRate);
    virtual uint256 GetHash() const { return ComputeSignatureHash(); };
    virtual double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); };
    virtual uint64_t GetValue() const { return 0; };

    virtual unsigned int GetSerializeSize(int nType, int nVersion) const    = 0;
    virtual bool GetAddress(std::set<CKeyID> &vAddr,
                        CAccountViewCache &view,
                        CScriptDBViewCache &scriptDB)                       = 0;
    virtual uint256 ComputeSignatureHash(bool recalculate = false) const    = 0;
    virtual std::shared_ptr<CBaseTx> GetNewInstance()                       = 0;
    virtual string ToString(CAccountViewCache &view) const                  = 0;
    virtual Object ToJson(const CAccountViewCache &AccountView) const       = 0;
    virtual bool CheckTx(CValidationState &state,
                        CAccountViewCache &view,
                        CScriptDBViewCache &scriptDB)                       = 0;
    virtual bool ExecuteTx(int nIndex, CAccountViewCache &view,
                        CValidationState &state,
                        CTxUndo &txundo, int nHeight,
                        CTransactionDBCache &txCache,
                        CScriptDBViewCache &scriptDB)                       = 0;
    virtual bool UndoExecuteTx(int nIndex, CAccountViewCache &view,
                        CValidationState &state,
                        CTxUndo &txundo, int nHeight,
                        CTransactionDBCache &txCache,
                        CScriptDBViewCache &scriptDB)                       = 0;

    int GetFuelRate(CScriptDBViewCache &scriptDB);
    bool IsValidHeight(int nCurHeight, int nTxCacheHeight) const;
    bool IsCoinBase() { return (nTxType == BLOCK_REWARD_TX); }

protected:
    bool CheckMinTxFee(const uint64_t llFees) const;
    bool CheckSignatureSize(const vector<unsigned char> &signature) const ;
};


class CScriptDBOperLog {
public:
    vector<unsigned char> vKey;
    vector<unsigned char> vValue;

    CScriptDBOperLog(const vector<unsigned char> &vKeyIn, const vector<unsigned char> &vValueIn) {
        vKey   = vKeyIn;
        vValue = vValueIn;
    }
    CScriptDBOperLog() {
        vKey.clear();
        vValue.clear();
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(vKey);
        READWRITE(vValue);)

    string ToString() const {
        string str;
        str += strprintf("vKey: %s, vValue: %s", HexStr(vKey), HexStr(vValue));
        return str;
    }

    friend bool operator<(const CScriptDBOperLog &log1, const CScriptDBOperLog &log2) {
        return log1.vKey < log2.vKey;
    }
};

class CTxUndo {
public:
    uint256 txHash;
    vector<CAccountLog> vAccountLog;
    vector<CScriptDBOperLog> vScriptOperLog;
    IMPLEMENT_SERIALIZE(
        READWRITE(txHash);
        READWRITE(vAccountLog);
        READWRITE(vScriptOperLog);)

public:
    bool GetAccountOperLog(const CKeyID &keyId, CAccountLog &accountLog);
    void Clear() {
        txHash = uint256();
        vAccountLog.clear();
        vScriptOperLog.clear();
    }
    string ToString() const;
};

class CSignaturePair {
public:
    CRegID regId;  //!< regid only
    vector_unsigned_char signature;

    IMPLEMENT_SERIALIZE(
        READWRITE(regId);
        READWRITE(signature);)

public:
    CSignaturePair(const CSignaturePair &signaturePair) {
        regId     = signaturePair.regId;
        signature = signaturePair.signature;
    }

    CSignaturePair(const CRegID &regIdIn, const vector_unsigned_char &signatureIn) {
        regId     = regIdIn;
        signature = signatureIn;
    }

    CSignaturePair() {}

    string ToString() const;
    Object ToJson() const;
};

class CMulsigTx : public CBaseTx {
public:
    mutable CUserID desUserId;              //!< keyid or regid
    uint64_t bcoins;                        //!< transfer amount
    uint8_t required;                       //!< required keys
    vector_unsigned_char memo;              //!< memo
    vector<CSignaturePair> signaturePairs;  //!< signature pair

    CKeyID keyId;  //!< only in memory

public:
    CMulsigTx() : CBaseTx(COMMON_MTX) {}

    CMulsigTx(const CBaseTx *pBaseTx) : CBaseTx(COMMON_MTX) {
        assert(COMMON_MTX == pBaseTx->nTxType);
        *this = *(CMulsigTx *)pBaseTx;
    }

    CMulsigTx(const vector<CSignaturePair> &signaturePairsIn, const CUserID &desUserIdIn,
                uint64_t feeIn, const uint64_t valueIn, const int validHeightIn,
                const uint8_t requiredIn, const vector_unsigned_char &memoIn)
        : CBaseTx(COMMON_MTX, CNullID(), validHeightIn, feeIn) {
        if (desUserIdIn.type() == typeid(CRegID))
            assert(!desUserIdIn.get<CRegID>().IsEmpty());

        signaturePairs = signaturePairsIn;
        desUserId      = desUserIdIn;
        bcoins         = valueIn;
        required       = requiredIn;
        memo           = memoIn;
    }

    CMulsigTx(const vector<CSignaturePair> &signaturePairsIn, const CUserID &desUserIdIn,
                uint64_t feeIn, const uint64_t valueIn, const int validHeightIn,
                const uint8_t requiredIn)
        : CBaseTx(COMMON_MTX, CNullID(), validHeightIn, feeIn) {
        if (desUserIdIn.type() == typeid(CRegID))
            assert(!desUserIdIn.get<CRegID>().IsEmpty());

        signaturePairs = signaturePairsIn;
        desUserId      = desUserIdIn;
        bcoins         = valueIn;
        required       = requiredIn;
    }

    ~CMulsigTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(signaturePairs);
        READWRITE(desUserId);
        READWRITE(VARINT(llFees));
        READWRITE(VARINT(bcoins));
        READWRITE(VARINT(required));
        READWRITE(memo);
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight);
            // Do NOT add item.signature.
            for (const auto &item : signaturePairs) {
                ss << item.regId;
            }
            ss << desUserId << VARINT(llFees) << VARINT(bcoins) << VARINT(required) << memo;
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    uint64_t GetValue() const { return bcoins; }
    uint256 GetHash() const { return ComputeSignatureHash(); }
    uint64_t GetFee() const { return llFees; }
    double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CMulsigTx>(this); }
    string ToString(CAccountViewCache &view) const;
    Object ToJson(const CAccountViewCache &AccountView) const;
    bool GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
    bool ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
                   int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB);
    bool UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                       CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                       CScriptDBViewCache &scriptDB);
    bool CheckTx(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
};

inline unsigned int GetSerializeSize(const std::shared_ptr<CBaseTx> &pa, int nType, int nVersion) {
    return pa->GetSerializeSize(nType, nVersion) + 1;
}

#endif
