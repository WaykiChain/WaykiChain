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
    { ACCOUNT_REGISTER_TX,       "ACCOUNT_REGISTER_TX" },
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
    mutable CUserID txUid; //FIXME: check whether mutable is needed or not

    static uint64_t nMinTxFee;
    static uint64_t nMinRelayTxFee;
    static uint64_t nDustAmountThreshold;
    static const int CURRENT_VERSION = nTxVersion1;
    int nVersion;
    unsigned char nTxType;
    int nValidHeight;
    uint64_t llFees;
    vector_unsigned_char signature;

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
    virtual uint256 GetHash() const                                      = 0;
    virtual uint64_t GetFee() const                                      = 0;
    virtual uint64_t GetFuel(int nfuelRate);
    virtual uint64_t GetValue() const { return 0; }
    virtual double GetPriority() const                                = 0;
    virtual uint256 SignatureHash(bool recalculate = false) const     = 0;
    virtual std::shared_ptr<CBaseTx> GetNewInstance()                 = 0;
    virtual string ToString(CAccountViewCache &view) const            = 0;
    virtual Object ToJson(const CAccountViewCache &AccountView) const = 0;
    virtual bool GetAddress(std::set<CKeyID> &vAddr, CAccountViewCache &view,
                            CScriptDBViewCache &scriptDB)             = 0;
    virtual bool IsValidHeight(int nCurHeight, int nTxCacheHeight) const;
    bool IsCoinBase() { return (nTxType == BLOCK_REWARD_TX); }
    virtual bool ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                           CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                           CScriptDBViewCache &scriptDB)     = 0;
    virtual bool UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                               CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                               CScriptDBViewCache &scriptDB) = 0;
    virtual bool CheckTx(CValidationState &state, CAccountViewCache &view,
                         CScriptDBViewCache &scriptDB)       = 0;

    int GetFuelRate(CScriptDBViewCache &scriptDB);

protected:
    bool CheckMinTxFee(const uint64_t llFees) const;
    bool CheckSignatureSize(const vector<unsigned char> &signature) const ;
};

class CRegisterAccountTx : public CBaseTx {
public:
    mutable CUserID minerUid;  // miner pubkey

public:
    CRegisterAccountTx(const CBaseTx *pBaseTx): CBaseTx(ACCOUNT_REGISTER_TX) {
        assert(ACCOUNT_REGISTER_TX == pBaseTx->nTxType);
        *this = *(CRegisterAccountTx *)pBaseTx;
    }
    CRegisterAccountTx(const CUserID &uId, const CUserID &minerUidIn, int64_t feeIn, int validHeightIn) :
        CBaseTx(ACCOUNT_REGISTER_TX, validHeightIn, feeIn) {
        txUid       = uId;
        minerUid    = minerUidIn;
    }
    CRegisterAccountTx(): CBaseTx(ACCOUNT_REGISTER_TX) {}

    ~CRegisterAccountTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(minerUid);
        READWRITE(VARINT(llFees));
        READWRITE(signature);)

    uint64_t GetFee() const { return llFees; }
    uint64_t GetValue() const { return 0; }

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid << minerUid
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

class CBaseCoinTransferTx : public CBaseTx {
public:
    mutable CUserID toUid;  // Recipient Regid or Keyid
    uint64_t bcoins;      // transfer amount
    vector_unsigned_char memo;

public:
    CBaseCoinTransferTx(): CBaseTx(BCOIN_TRANSFER_TX) { }

    CBaseCoinTransferTx(const CBaseTx *pBaseTx): CBaseTx(BCOIN_TRANSFER_TX) {
        assert(BCOIN_TRANSFER_TX == pBaseTx->nTxType);
        *this = *(CBaseCoinTransferTx *)pBaseTx;
    }

    CBaseCoinTransferTx(const CUserID &txUidIn, CUserID toUidIn, uint64_t feeIn, uint64_t valueIn,
              int validHeightIn, vector_unsigned_char &descriptionIn) :
              CBaseTx(BCOIN_TRANSFER_TX, validHeightIn, feeIn) {

        //FIXME: need to support public key
        if (txUidIn.type() == typeid(CRegID))
            assert(!txUidIn.get<CRegID>().IsEmpty());

        if (toUidIn.type() == typeid(CRegID))
            assert(!toUidIn.get<CRegID>().IsEmpty());

        txUid   = txUidIn;
        toUid   = toUidIn;
        bcoins  = valueIn;
        memo    = descriptionIn;
    }

    CBaseCoinTransferTx(const CUserID &txUidIn, CUserID toUidIn, uint64_t feeIn, uint64_t valueIn,
              int validHeightIn): CBaseTx(BCOIN_TRANSFER_TX, validHeightIn, feeIn) {
        if (txUidIn.type() == typeid(CRegID))
            assert(!txUidIn.get<CRegID>().IsEmpty());

        if (toUidIn.type() == typeid(CRegID))
            assert(!toUidIn.get<CRegID>().IsEmpty());

        txUid  = txUidIn;
        toUid  = toUidIn;
        bcoins = valueIn;
    }

    ~CBaseCoinTransferTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(toUid);
        READWRITE(VARINT(llFees));
        READWRITE(VARINT(bcoins));
        READWRITE(memo);
        READWRITE(signature);
    )

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid << toUid
               << VARINT(llFees) << VARINT(bcoins) << memo;
            // Truly need to write the sigHash.
            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }

        return sigHash;
    }

    uint64_t GetValue() const { return bcoins; }
    uint256 GetHash() const { return SignatureHash(); }
    uint64_t GetFee() const { return llFees; }
    double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CBaseCoinTransferTx>(this); }
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

class CContractInvokeTx : public CBaseTx {
public:
    mutable CUserID appUid;  // app regid or address
    uint64_t bcoins;         // transfer amount
    vector_unsigned_char arguments; // arguments to invoke a contract method

public:
    CContractInvokeTx() : CBaseTx(CONTRACT_INVOKE_TX) {}

    CContractInvokeTx(const CBaseTx *pBaseTx): CBaseTx(CONTRACT_INVOKE_TX) {
        assert(CONTRACT_INVOKE_TX == pBaseTx->nTxType);
        *this = *(CContractInvokeTx *)pBaseTx;
    }

    CContractInvokeTx(const CUserID &txUidIn, CUserID appUidIn, uint64_t feeIn,
                uint64_t bcoinsIn, int validHeightIn, vector_unsigned_char &argumentsIn):
                CBaseTx(CONTRACT_INVOKE_TX, validHeightIn, feeIn) {
        if (txUidIn.type() == typeid(CRegID))
            assert(!txUidIn.get<CRegID>().IsEmpty()); //FIXME: shouldnot be using assert here, throw an error instead.

        if (appUidIn.type() == typeid(CRegID))
            assert(!appUidIn.get<CRegID>().IsEmpty());

        txUid  = txUidIn;
        appUid = appUidIn;
        bcoins = bcoinsIn;
        arguments = argumentsIn;
    }

    CContractInvokeTx(const CUserID &txUidIn, CUserID appUidIn, uint64_t feeIn, uint64_t bcoinsIn, int validHeightIn):
                CBaseTx(CONTRACT_INVOKE_TX, validHeightIn, feeIn) {
        if (txUidIn.type() == typeid(CRegID))
            assert(!txUidIn.get<CRegID>().IsEmpty());

        if (appUidIn.type() == typeid(CRegID))
            assert(!appUidIn.get<CRegID>().IsEmpty());

        txUid  = txUidIn;
        appUid = appUidIn;
        bcoins = bcoinsIn;
    }

    ~CContractInvokeTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(appUid);
        READWRITE(VARINT(llFees));
        READWRITE(VARINT(bcoins));
        READWRITE(arguments);
        READWRITE(signature);
    )

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid << appUid
               << VARINT(llFees) << VARINT(bcoins) << arguments;
            // Truly need to write the sigHash.
            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }
        return sigHash;
    }

    uint64_t GetValue() const { return bcoins; }
    uint256 GetHash() const { return SignatureHash(); }
    uint64_t GetFee() const { return llFees; }
    double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    std::shared_ptr<CBaseTx> GetNewInstance() {
        return std::make_shared<CContractInvokeTx>(this);
    }
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

class CBlockRewardTx : public CBaseTx {
public:
    uint64_t rewardValue;
    int nHeight;

public:
    CBlockRewardTx(): CBaseTx(BLOCK_REWARD_TX) { rewardValue = 0; }
    CBlockRewardTx(const CBaseTx *pBaseTx): CBaseTx(BLOCK_REWARD_TX) {
        assert(BLOCK_REWARD_TX == pBaseTx->nTxType);
        *this = *(CBlockRewardTx *)pBaseTx;
    }
    CBlockRewardTx(const vector_unsigned_char &accountIn, const uint64_t rewardValueIn, const int nHeightIn):
        CBaseTx(BLOCK_REWARD_TX) {
        if (accountIn.size() > 6) {
            txUid = CPubKey(accountIn);
        } else {
            txUid = CRegID(accountIn);
        }
        rewardValue = rewardValueIn;
        nHeight     = nHeightIn;
    }
    ~CBlockRewardTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(txUid);
        READWRITE(VARINT(rewardValue));
        READWRITE(VARINT(nHeight));)

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << txUid << VARINT(rewardValue) << VARINT(nHeight);
            // Truly need to write the sigHash.
            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }

        return sigHash;
    }

    uint64_t GetValue() const { return rewardValue; }
    uint256 GetHash() const { return SignatureHash(); }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CBlockRewardTx>(this); }
    uint64_t GetFee() const { return 0; }
    double GetPriority() const { return 0.0f; }
    string ToString(CAccountViewCache &view) const;
    Object ToJson(const CAccountViewCache &AccountView) const;
    bool GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
    bool ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
                   int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB);
    bool UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                       CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                       CScriptDBViewCache &scriptDB);
    bool CheckTx(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {
        return true;
    }
};

class CContractDeployTx : public CBaseTx {
public:
    vector_unsigned_char contractScript;  // contract script content

public:
    CContractDeployTx(const CBaseTx *pBaseTx): CBaseTx(CONTRACT_DEPLOY_TX) {
        assert(CONTRACT_DEPLOY_TX == pBaseTx->nTxType);
        *this = *(CContractDeployTx *)pBaseTx;
    }
    CContractDeployTx(): CBaseTx(CONTRACT_DEPLOY_TX) {}
    ~CContractDeployTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(contractScript);
        READWRITE(VARINT(llFees));
        READWRITE(signature);)

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid << contractScript
               << VARINT(llFees);
            // Truly need to write the sigHash.
            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }

        return sigHash;
    }

    uint256 GetHash() const { return SignatureHash(); }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CContractDeployTx>(this); }
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

class CDelegateVoteTx : public CBaseTx {
public:
    vector<COperVoteFund> operVoteFunds;  //!< oper delegate votes, max length is Delegates number

public:
    CDelegateVoteTx(const CBaseTx *pBaseTx): CBaseTx(DELEGATE_VOTE_TX) {
        assert(DELEGATE_VOTE_TX == pBaseTx->nTxType);
        *this = *(CDelegateVoteTx *)pBaseTx;
    }
    CDelegateVoteTx(const vector_unsigned_char &accountIn, const vector<COperVoteFund> &operVoteFundsIn,
                const uint64_t feeIn, const int validHeightIn)
        : CBaseTx(DELEGATE_VOTE_TX, validHeightIn, feeIn) {
        if (accountIn.size() > 6) {
            txUid = CPubKey(accountIn);
        } else {
            txUid = CRegID(accountIn);
        }
        operVoteFunds = operVoteFundsIn;
    }
    CDelegateVoteTx(const CUserID &userIdIn, const uint64_t feeIn,
                const vector<COperVoteFund> &operVoteFundsIn, const int validHeightIn)
        : CBaseTx(DELEGATE_VOTE_TX, validHeightIn, feeIn) {
        if (userIdIn.type() == typeid(CRegID)) assert(!userIdIn.get<CRegID>().IsEmpty());

        txUid        = userIdIn;
        operVoteFunds = operVoteFundsIn;
    }
    CDelegateVoteTx(): CBaseTx(DELEGATE_VOTE_TX) {}
    ~CDelegateVoteTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(operVoteFunds);
        READWRITE(VARINT(llFees));
        READWRITE(signature);
    )

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid << operVoteFunds
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
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDelegateVoteTx>(this); }
    string ToString(CAccountViewCache &view) const;
    Object ToJson(const CAccountViewCache &accountView) const;
    bool GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
    bool ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
                   int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB);
    bool UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                       CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                       CScriptDBViewCache &scriptDB);
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
        : CBaseTx(COMMON_MTX, validHeightIn, feeIn) {
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
        : CBaseTx(COMMON_MTX, validHeightIn, feeIn) {
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

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight);
            // Do NOT add item.signature.
            for (const auto &item : signaturePairs) {
                ss << item.regId;
            }
            ss << desUserId << VARINT(llFees) << VARINT(bcoins) << VARINT(required) << memo;
            // Truly need to write the sigHash.
            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }
        return sigHash;
    }

    uint64_t GetValue() const { return bcoins; }
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

//global overloadding fun
template <typename Stream>
void Serialize(Stream &os, const std::shared_ptr<CBaseTx> &pa, int nType, int nVersion) {
    unsigned char nTxType = pa->nTxType;
    Serialize(os, nTxType, nType, nVersion);
    if (pa->nTxType == ACCOUNT_REGISTER_TX) {
        Serialize(os, *((CRegisterAccountTx *)(pa.get())), nType, nVersion);
    } else if (pa->nTxType == BCOIN_TRANSFER_TX) {
        Serialize(os, *((CBaseCoinTransferTx *)(pa.get())), nType, nVersion);
    } else if (pa->nTxType == CONTRACT_INVOKE_TX) {
        Serialize(os, *((CContractInvokeTx *)(pa.get())), nType, nVersion);
    } else if (pa->nTxType == BLOCK_REWARD_TX) {
        Serialize(os, *((CBlockRewardTx *)(pa.get())), nType, nVersion);
    } else if (pa->nTxType == CONTRACT_DEPLOY_TX) {
        Serialize(os, *((CContractDeployTx *)(pa.get())), nType, nVersion);
    } else if (pa->nTxType == DELEGATE_VOTE_TX) {
        Serialize(os, *((CDelegateVoteTx *)(pa.get())), nType, nVersion);
    } else if (pa->nTxType == COMMON_MTX) {
        Serialize(os, *((CMulsigTx *)(pa.get())), nType, nVersion);
    } else {
        string sTxType(1, nTxType);
        throw ios_base::failure("Serialize: nTxType (" + sTxType + ") value error.");
    }
}

//global overloadding fun
template <typename Stream>
void Unserialize(Stream &is, std::shared_ptr<CBaseTx> &pa, int nType, int nVersion) {
    unsigned char nTxType;
    is.read((char *)&(nTxType), sizeof(nTxType));
    if (nTxType == ACCOUNT_REGISTER_TX) {
        pa = std::make_shared<CRegisterAccountTx>();
        Unserialize(is, *((CRegisterAccountTx *)(pa.get())), nType, nVersion);
    } else if (nTxType == BCOIN_TRANSFER_TX) {
        pa = std::make_shared<CBaseCoinTransferTx>();
        Unserialize(is, *((CBaseCoinTransferTx *)(pa.get())), nType, nVersion);
    } else if (nTxType == CONTRACT_INVOKE_TX) {
        pa = std::make_shared<CContractInvokeTx>();
        Unserialize(is, *((CContractInvokeTx *)(pa.get())), nType, nVersion);
    } else if (nTxType == BLOCK_REWARD_TX) {
        pa = std::make_shared<CBlockRewardTx>();
        Unserialize(is, *((CBlockRewardTx *)(pa.get())), nType, nVersion);
    } else if (nTxType == CONTRACT_DEPLOY_TX) {
        pa = std::make_shared<CContractDeployTx>();
        Unserialize(is, *((CContractDeployTx *)(pa.get())), nType, nVersion);
    } else if (nTxType == DELEGATE_VOTE_TX) {
        pa = std::make_shared<CDelegateVoteTx>();
        Unserialize(is, *((CDelegateVoteTx *)(pa.get())), nType, nVersion);
    } else if (nTxType == COMMON_MTX) {
        pa = std::make_shared<CMulsigTx>();
        Unserialize(is, *((CMulsigTx *)(pa.get())), nType, nVersion);
    } else {
        string sTxType(1, nTxType);
        throw ios_base::failure("Unserialize: nTxType (" + sTxType + ") value error.");
    }
    pa->nTxType = nTxType;
}

#endif
