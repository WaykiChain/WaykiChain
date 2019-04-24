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
    REWARD_TX        = 1,  //!< Miner Reward Tx
    REG_ACCT_TX      = 2,  //!< Register Account Tx
    COMMON_TX        = 3,  //!< Base Coin Transfer Tx
    CONTRACT_TX      = 4,  //!< Contract Tx
    REG_CONT_TX      = 5,  //!< Register Contract Tx
    DELEGATE_TX      = 6,  //!< Vote Delegate Tx
    COMMON_MULSIG_TX = 7,  //!< Multisig Tx

    /******** Begin of Stable Coin TX Type Enums ********/
    CDP_OPEN_TX      = 11,  //!< CDP Collateralize Tx
    CDP_REFUEL_TX    = 12,  //!< CDP Refuel Tx
    CDP_REDEMP_TX    = 13,  //!< CDP Redemption Tx (partial or full)
    CDP_LIQUIDATE_TX = 14,  //!< CDP Liquidation Tx (partial or full)

    PRICE_FEED_WICC_TX = 21,  //!< Price Feed Tx: WICC/USD
    PRICE_FEED_MICC_TX = 22,  //!< Price Feed Tx: MICC/USD
    PRICE_FEED_WUSD_TX = 23,  //!< Price Feed Tx: WUSD/USD

    SFC_PARAM_MTX = 31,  //!< StableCoin Fund Committee invokes Param Set/Update MulSigTx
    SFC_GLOBAL_HALT_MTX =
        32,  //!< StableCoin Fund Committee invokes Global Halt CDP Operations MulSigTx
    SFC_GLOBAL_SETTLE_MTX =
        33,  //!< StableCoin Fund Committee invokes Global Settle Operation MulSigTx

    WUSD_TRANSFER_TX = 41,  //!< StableCoin WUSD Transfer Tx
    MICC_TRANSFER_TX = 42,  //!< FundCoin MICC Transfer Tx

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
    { REWARD_TX,            "REWARD_TX" },
    { REG_ACCT_TX,          "REG_ACCT_TX" },
    { COMMON_TX,            "COMMON_TX" },
    { CONTRACT_TX,          "CONTRACT_TX" },
    { REG_CONT_TX,          "REG_CONT_TX" },
    { DELEGATE_TX,          "DELEGATE_TX" },
    { COMMON_MULSIG_TX,     "COMMON_MULSIG_TX"},
    { CDP_OPEN_TX,          "CDP_OPEN_TX" },
    { CDP_REFUEL_TX,        "CDP_REFUEL_TX" },
    { CDP_REDEMP_TX,        "CDP_REDEMP_TX" },
    { CDP_LIQUIDATE_TX,     "CDP_LIQUIDATE_TX" },
    { PRICE_FEED_WICC_TX,   "PRICE_FEED_WICC_TX" },
    { PRICE_FEED_MICC_TX,   "PRICE_FEED_MICC_TX" },
    { PRICE_FEED_WUSD_TX,   "PRICE_FEED_WUSD_TX" },
    { SFC_PARAM_MTX,        "SFC_PARAM_MTX" },
    { SFC_GLOBAL_HALT_MTX,  "SFC_GLOBAL_HALT_MTX" },
    { SFC_GLOBAL_SETTLE_MTX,"SFC_GLOBAL_SETTLE_MTX" },
    { WUSD_TRANSFER_TX,     "WUSD_TRANSFER_TX" },
    { MICC_TRANSFER_TX,     "MICC_TRANSFER_TX" },
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
    static int64_t nMinRelayTxFee;
    static uint64_t nDustAmountThreshold;
    static const int CURRENT_VERSION = nTxVersion1;

    int nVersion;
    unsigned char nTxType;
    int nValidHeight;
    uint64_t llFees;
    uint64_t nRunStep;  //!< only in memory
    int nFuelRate;      //!< only in memory
    uint256 sigHash;    //!< only in memory

public:
    CBaseTx(const CBaseTx &other) { *this = other; }

    CBaseTx(int nVersionIn, TxType nTxTypeIn, int nValidHeightIn, uint64_t llFeesIn) :
        nVersion(nVersionIn), nTxType(nTxTypeIn), nValidHeight(nValidHeightIn), llFees(llFeesIn),
        nRunStep(0), nFuelRate(0) {}

    CBaseTx(TxType nTxTypeIn, int nValidHeightIn, uint64_t llFeesIn) :
        nVersion(CURRENT_VERSION), nTxType(nTxTypeIn), nValidHeight(nValidHeightIn), llFees(llFeesIn),
        nRunStep(0), nFuelRate(0) {}

    CBaseTx(int nVersionIn, TxType nTxTypeIn) :
        nVersion(nVersionIn), nTxType(nTxTypeIn),
        nValidHeight(0), llFees(0), nRunStep(0), nFuelRate(0) {}

    CBaseTx(TxType nTxTypeIn) :
        nVersion(CURRENT_VERSION), nTxType(nTxTypeIn),
        nValidHeight(0), llFees(0), nRunStep(0), nFuelRate(0) {}

    virtual ~CBaseTx() {}
    virtual unsigned int GetSerializeSize(int nType, int nVersion) const = 0;
    virtual uint256 GetHash() const = 0;
    virtual uint64_t GetFee() const = 0;
    virtual uint64_t GetFuel(int nfuelRate);
    virtual uint64_t GetValue() const { return 0; }
    virtual double GetPriority() const = 0;
    virtual uint256 SignatureHash(bool recalculate = false) const = 0;
    virtual std::shared_ptr<CBaseTx> GetNewInstance() = 0;
    virtual string ToString(CAccountViewCache &view) const = 0;
    virtual Object ToJson(const CAccountViewCache &AccountView) const = 0;
    virtual bool GetAddress(std::set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB) = 0;
    virtual bool IsValidHeight(int nCurHeight, int nTxCacheHeight) const;
    bool IsCoinBase() { return (nTxType == REWARD_TX); }
    virtual bool ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
                        int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB) = 0;
    virtual bool UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
                        int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB);
    virtual bool CheckTx(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB) = 0;

    int GetFuelRate(CScriptDBViewCache &scriptDB);

protected:
    bool CheckMinTxFee(const uint64_t llFees) const;
    bool CheckSignatureSize(const vector<unsigned char> &signature) const ;
};

class CRegisterAccountTx : public CBaseTx {
public:
    mutable CUserID userId;   // pubkey
    mutable CUserID minerId;  // miner pubkey
    vector<unsigned char> signature;

public:
    CRegisterAccountTx(const CBaseTx *pBaseTx): CBaseTx(REG_ACCT_TX) {
        assert(REG_ACCT_TX == pBaseTx->nTxType);
        *this = *(CRegisterAccountTx *)pBaseTx;
    }
    CRegisterAccountTx(const CUserID &uId, const CUserID &minerID, int64_t feeIn, int validHeightIn) :
        CBaseTx(REG_ACCT_TX, validHeightIn, feeIn) {
        userId       = uId;
        minerId      = minerID;
    }
    CRegisterAccountTx(): CBaseTx(REG_ACCT_TX) {}

    ~CRegisterAccountTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(userId);
        READWRITE(minerId);
        READWRITE(VARINT(llFees));
        READWRITE(signature);)

    uint64_t GetFee() const { return llFees; }
    uint64_t GetValue() const { return 0; }

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << userId << minerId
               << VARINT(llFees);
            // Truly need to write the sigHash.
            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }

        return sigHash;
    }
    uint256 GetHash() const { return SignatureHash(); }
    double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CRegisterAccountTx>(this); }
    bool GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
    string ToString(CAccountViewCache &view) const;
    Object ToJson(const CAccountViewCache &AccountView) const;
    bool ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo, int nHeight,
                   CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB);
    bool UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo, int nHeight,
                       CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB);
    bool CheckTx(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
};

class CCommonTx : public CBaseTx {
public:
    mutable CUserID srcUserId;  // regid or pubkey
    mutable CUserID desUserId;  // regid or keyid
    uint64_t bcoinBalance;      // transfer amount
    vector_unsigned_char memo;
    vector_unsigned_char signature;

public:
    CCommonTx(): CBaseTx(COMMON_TX) { }

    CCommonTx(const CBaseTx *pBaseTx): CBaseTx(COMMON_TX) {
        assert(COMMON_TX == pBaseTx->nTxType);
        *this = *(CCommonTx *)pBaseTx;
    }

    CCommonTx(const CUserID &srcUserIdIn, CUserID desUserIdIn, uint64_t feeIn, uint64_t valueIn,
              int validHeightIn, vector_unsigned_char &descriptionIn) :
              CBaseTx(COMMON_TX, validHeightIn, feeIn) {

        //FIXME: need to support public key
        if (srcUserIdIn.type() == typeid(CRegID))
            assert(!srcUserIdIn.get<CRegID>().IsEmpty());

        if (desUserIdIn.type() == typeid(CRegID))
            assert(!desUserIdIn.get<CRegID>().IsEmpty());

        srcUserId    = srcUserIdIn;
        desUserId    = desUserIdIn;
        bcoinBalance = valueIn;
        memo         = descriptionIn;
    }

    CCommonTx(const CUserID &srcUserIdIn, CUserID desUserIdIn, uint64_t feeIn, uint64_t valueIn,
              int validHeightIn): CBaseTx(COMMON_TX, validHeightIn, feeIn) {
        if (srcUserIdIn.type() == typeid(CRegID))
            assert(!srcUserIdIn.get<CRegID>().IsEmpty());

        if (desUserIdIn.type() == typeid(CRegID))
            assert(!desUserIdIn.get<CRegID>().IsEmpty());

        srcUserId    = srcUserIdIn;
        desUserId    = desUserIdIn;
        bcoinBalance = valueIn;
    }

    ~CCommonTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(srcUserId);
        READWRITE(desUserId);
        READWRITE(VARINT(llFees));
        READWRITE(VARINT(bcoinBalance));
        READWRITE(memo);
        READWRITE(signature);
    )

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << srcUserId << desUserId
               << VARINT(llFees) << VARINT(bcoinBalance) << memo;
            // Truly need to write the sigHash.
            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }

        return sigHash;
    }

    uint64_t GetValue() const { return bcoinBalance; }
    uint256 GetHash() const { return SignatureHash(); }
    uint64_t GetFee() const { return llFees; }
    double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    std::shared_ptr<CBaseTx> GetNewInstance() {
        return std::make_shared<CCommonTx>(this);
    }
    string ToString(CAccountViewCache &view) const;
    Object ToJson(const CAccountViewCache &AccountView) const;
    bool GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
    bool ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
                   int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB);
    bool CheckTx(CValidationState &state, CAccountViewCache &view,
                          CScriptDBViewCache &scriptDB);
};

class CContractTx : public CBaseTx {
public:
    mutable CUserID srcRegId;   // src regid
    mutable CUserID desUserId;  // keyid or app regid
    uint64_t bcoinBalance;      // transfer amount
    vector_unsigned_char arguments;
    vector_unsigned_char signature;

public:
    CContractTx() : CBaseTx(CONTRACT_TX) {}

    CContractTx(const CBaseTx *pBaseTx): CBaseTx(CONTRACT_TX) {
        assert(CONTRACT_TX == pBaseTx->nTxType);
        *this = *(CContractTx *)pBaseTx;
    }

    CContractTx(const CUserID &srcRegIdIn, CUserID desUserIdIn, uint64_t feeIn,
                uint64_t valueIn, int validHeightIn, vector_unsigned_char &argumentsIn):
                CBaseTx(CONTRACT_TX, validHeightIn, feeIn) {
        if (srcRegIdIn.type() == typeid(CRegID))
            assert(!srcRegIdIn.get<CRegID>().IsEmpty());

        if (desUserIdIn.type() == typeid(CRegID))
            assert(!desUserIdIn.get<CRegID>().IsEmpty());

        srcRegId     = srcRegIdIn;
        desUserId    = desUserIdIn;
        bcoinBalance = valueIn;
        arguments    = argumentsIn;
    }

    CContractTx(const CUserID &srcRegIdIn, CUserID desUserIdIn, uint64_t feeIn,
                uint64_t valueIn, int validHeightIn):
                CBaseTx(CONTRACT_TX, validHeightIn, feeIn) {
        if (srcRegIdIn.type() == typeid(CRegID))
            assert(!srcRegIdIn.get<CRegID>().IsEmpty());

        if (desUserIdIn.type() == typeid(CRegID))
            assert(!desUserIdIn.get<CRegID>().IsEmpty());

        srcRegId     = srcRegIdIn;
        desUserId    = desUserIdIn;
        bcoinBalance = valueIn;
    }

    ~CContractTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(srcRegId);
        READWRITE(desUserId);
        READWRITE(VARINT(llFees));
        READWRITE(VARINT(bcoinBalance));
        READWRITE(arguments);
        READWRITE(signature);
    )

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << srcRegId << desUserId
               << VARINT(llFees) << VARINT(bcoinBalance) << arguments;
            // Truly need to write the sigHash.
            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }
        return sigHash;
    }

    uint64_t GetValue() const { return bcoinBalance; }
    uint256 GetHash() const { return SignatureHash(); }
    uint64_t GetFee() const { return llFees; }
    double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    std::shared_ptr<CBaseTx> GetNewInstance() {
        return std::make_shared<CContractTx>(this);
    }
    string ToString(CAccountViewCache &view) const;
    Object ToJson(const CAccountViewCache &AccountView) const;
    bool GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
    bool ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
                   int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB);
    bool CheckTx(CValidationState &state, CAccountViewCache &view,
                          CScriptDBViewCache &scriptDB);
};

class CRewardTx : public CBaseTx {
public:
    mutable CUserID account;  // pubkey in genesis block, otherwise accountId
    uint64_t rewardValue;
    int nHeight;

public:
    CRewardTx(): CBaseTx(REWARD_TX) { rewardValue = 0; }
    CRewardTx(const CBaseTx *pBaseTx): CBaseTx(REWARD_TX) {
        assert(REWARD_TX == pBaseTx->nTxType);
        *this = *(CRewardTx *)pBaseTx;
    }
    CRewardTx(const vector_unsigned_char &accountIn, const uint64_t rewardValueIn, const int nHeightIn):
        CBaseTx(REWARD_TX) {
        if (accountIn.size() > 6) {
            account = CPubKey(accountIn);
        } else {
            account = CRegID(accountIn);
        }
        rewardValue = rewardValueIn;
        nHeight     = nHeightIn;
    }
    ~CRewardTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(account);
        READWRITE(VARINT(rewardValue));
        READWRITE(VARINT(nHeight));)

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << account << VARINT(rewardValue) << VARINT(nHeight);
            // Truly need to write the sigHash.
            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }

        return sigHash;
    }

    uint64_t GetValue() const { return rewardValue; }
    uint256 GetHash() const { return SignatureHash(); }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CRewardTx>(this); }
    uint64_t GetFee() const { return 0; }
    double GetPriority() const { return 0.0f; }
    string ToString(CAccountViewCache &view) const;
    Object ToJson(const CAccountViewCache &AccountView) const;
    bool GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
    bool ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo, int nHeight,
                   CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB);
    bool CheckTx(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB) { return true; }
};

class CRegisterContractTx : public CBaseTx {
public:
    mutable CUserID regAcctId;    // contract publisher regid
    vector_unsigned_char script;  // contract script content
    vector_unsigned_char signature;

public:
    CRegisterContractTx(const CBaseTx *pBaseTx): CBaseTx(REG_CONT_TX) {
        assert(REG_CONT_TX == pBaseTx->nTxType);
        *this = *(CRegisterContractTx *)pBaseTx;
    }
    CRegisterContractTx(): CBaseTx(REG_CONT_TX) {}
    ~CRegisterContractTx() { }

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(regAcctId);
        READWRITE(script);
        READWRITE(VARINT(llFees));
        READWRITE(signature);)

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << regAcctId << script
               << VARINT(llFees);
            // Truly need to write the sigHash.
            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }

        return sigHash;
    }

    uint256 GetHash() const { return SignatureHash(); }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CRegisterContractTx>(this); }
    uint64_t GetFee() const { return llFees; }
    uint64_t GetValue() const { return 0; }
    double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    string ToString(CAccountViewCache &view) const;
    Object ToJson(const CAccountViewCache &AccountView) const;
    bool GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
    bool ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo, int nHeight,
                   CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB);
    bool UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo, int nHeight,
                       CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB);
    bool CheckTx(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
};

class CDelegateTx : public CBaseTx {
public:
    mutable CUserID userId;
    vector<COperVoteFund> operVoteFunds;  //!< oper delegate votes, max length is Delegates number
    vector_unsigned_char signature;

public:
    CDelegateTx(const CBaseTx *pBaseTx): CBaseTx(DELEGATE_TX) {
        assert(DELEGATE_TX == pBaseTx->nTxType);
        *this = *(CDelegateTx *)pBaseTx;
    }
    CDelegateTx(const vector_unsigned_char &accountIn, const vector<COperVoteFund> &operVoteFundsIn,
                const uint64_t feeIn, const int validHeightIn)
        : CBaseTx(DELEGATE_TX, validHeightIn, feeIn) {
        if (accountIn.size() > 6) {
            userId = CPubKey(accountIn);
        } else {
            userId = CRegID(accountIn);
        }
        operVoteFunds = operVoteFundsIn;
    }
    CDelegateTx(const CUserID &userIdIn, const uint64_t feeIn,
                const vector<COperVoteFund> &operVoteFundsIn, const int validHeightIn)
        : CBaseTx(DELEGATE_TX, validHeightIn, feeIn) {
        if (userIdIn.type() == typeid(CRegID)) assert(!userIdIn.get<CRegID>().IsEmpty());

        userId        = userIdIn;
        operVoteFunds = operVoteFundsIn;
    }
    CDelegateTx(): CBaseTx(DELEGATE_TX) {}
    ~CDelegateTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(userId);
        READWRITE(operVoteFunds);
        READWRITE(VARINT(llFees));
        READWRITE(signature);
    )

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << userId << operVoteFunds
               << VARINT(llFees);
            // Truly need to write the sigHash.
            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }

        return sigHash;
    }

    uint256 GetHash() const { return SignatureHash(); }
    uint64_t GetFee() const { return llFees; }
    uint64_t GetValue() const { return 0; }
    double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDelegateTx>(this); }
    string ToString(CAccountViewCache &view) const;
    Object ToJson(const CAccountViewCache &accountView) const;
    bool GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
    bool ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo, int nHeight,
                   CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB);
    bool CheckTx(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
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
        string str("");
        str += "vKey:";
        str += HexStr(vKey);
        str += "\n";
        str += "vValue:";
        str += HexStr(vValue);
        str += "\n";
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
    vector<CSignaturePair> signaturePairs;  //!< signature pair
    mutable CUserID desUserId;              //!< keyid or regid
    uint64_t bcoinBalance;                  //!< transfer amount
    uint8_t required;                       //!< required keys
    vector_unsigned_char memo;              //!< memo

    CKeyID keyId;  //!< only in memory

public:
    CMulsigTx() : CBaseTx(COMMON_MULSIG_TX) {}

    CMulsigTx(const CBaseTx *pBaseTx) : CBaseTx(COMMON_MULSIG_TX) {
        assert(COMMON_MULSIG_TX == pBaseTx->nTxType);
        *this = *(CMulsigTx *)pBaseTx;
    }

    CMulsigTx(const vector<CSignaturePair> &signaturePairsIn, const CUserID &desUserIdIn,
                uint64_t feeIn, const uint64_t valueIn, const int validHeightIn,
                const uint8_t requiredIn, const vector_unsigned_char &memoIn)
        : CBaseTx(COMMON_MULSIG_TX, validHeightIn, feeIn) {
        if (desUserIdIn.type() == typeid(CRegID))
            assert(!desUserIdIn.get<CRegID>().IsEmpty());

        signaturePairs = signaturePairsIn;
        desUserId      = desUserIdIn;
        bcoinBalance   = valueIn;
        required       = requiredIn;
        memo           = memoIn;
    }

    CMulsigTx(const vector<CSignaturePair> &signaturePairsIn, const CUserID &desUserIdIn,
                uint64_t feeIn, const uint64_t valueIn, const int validHeightIn,
                const uint8_t requiredIn)
        : CBaseTx(COMMON_MULSIG_TX, validHeightIn, feeIn) {
        if (desUserIdIn.type() == typeid(CRegID))
            assert(!desUserIdIn.get<CRegID>().IsEmpty());

        signaturePairs = signaturePairsIn;
        desUserId      = desUserIdIn;
        bcoinBalance   = valueIn;
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
        READWRITE(VARINT(bcoinBalance));
        READWRITE(VARINT(required));
        READWRITE(memo);
    )
 
    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight);
            // Do NOT add item.signature.
            for (const auto &item : signaturePairs) {
                ss << item.regId;
            }
            ss << desUserId << VARINT(llFees) << VARINT(bcoinBalance) << VARINT(required) << memo;
            // Truly need to write the sigHash.
            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }
        return sigHash;
    }

    uint64_t GetValue() const { return bcoinBalance; }
    uint256 GetHash() const { return SignatureHash(); }
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

template <typename Stream>
void Serialize(Stream &os, const std::shared_ptr<CBaseTx> &pa, int nType, int nVersion) {
    unsigned char nTxType = pa->nTxType;
    Serialize(os, nTxType, nType, nVersion);
    if (pa->nTxType == REG_ACCT_TX) {
        Serialize(os, *((CRegisterAccountTx *)(pa.get())), nType, nVersion);
    } else if (pa->nTxType == COMMON_TX) {
        Serialize(os, *((CCommonTx *)(pa.get())), nType, nVersion);
    } else if (pa->nTxType == CONTRACT_TX) {
        Serialize(os, *((CContractTx *)(pa.get())), nType, nVersion);
    } else if (pa->nTxType == REWARD_TX) {
        Serialize(os, *((CRewardTx *)(pa.get())), nType, nVersion);
    } else if (pa->nTxType == REG_CONT_TX) {
        Serialize(os, *((CRegisterContractTx *)(pa.get())), nType, nVersion);
    } else if (pa->nTxType == DELEGATE_TX) {
        Serialize(os, *((CDelegateTx *)(pa.get())), nType, nVersion);
    } else if (pa->nTxType == COMMON_MULSIG_TX) {
        Serialize(os, *((CMulsigTx *)(pa.get())), nType, nVersion);
    } else {
        string sTxType(1, nTxType);
        throw ios_base::failure("Serialize: nTxType (" + sTxType + ") value error.");
    }
}

template <typename Stream>
void Unserialize(Stream &is, std::shared_ptr<CBaseTx> &pa, int nType, int nVersion) {
    unsigned char nTxType;
    is.read((char *)&(nTxType), sizeof(nTxType));
    if (nTxType == REG_ACCT_TX) {
        pa = std::make_shared<CRegisterAccountTx>();
        Unserialize(is, *((CRegisterAccountTx *)(pa.get())), nType, nVersion);
    } else if (nTxType == COMMON_TX) {
        pa = std::make_shared<CCommonTx>();
        Unserialize(is, *((CCommonTx *)(pa.get())), nType, nVersion);
    } else if (nTxType == CONTRACT_TX) {
        pa = std::make_shared<CContractTx>();
        Unserialize(is, *((CContractTx *)(pa.get())), nType, nVersion);
    } else if (nTxType == REWARD_TX) {
        pa = std::make_shared<CRewardTx>();
        Unserialize(is, *((CRewardTx *)(pa.get())), nType, nVersion);
    } else if (nTxType == REG_CONT_TX) {
        pa = std::make_shared<CRegisterContractTx>();
        Unserialize(is, *((CRegisterContractTx *)(pa.get())), nType, nVersion);
    } else if (nTxType == DELEGATE_TX) {
        pa = std::make_shared<CDelegateTx>();
        Unserialize(is, *((CDelegateTx *)(pa.get())), nType, nVersion);
    } else if (nTxType == COMMON_MULSIG_TX) {
        pa = std::make_shared<CMulsigTx>();
        Unserialize(is, *((CMulsigTx *)(pa.get())), nType, nVersion);
    } else {
        string sTxType(1, nTxType);
        throw ios_base::failure("Unserialize: nTxType (" + sTxType + ") value error.");
    }
    pa->nTxType = nTxType;
}

#endif
