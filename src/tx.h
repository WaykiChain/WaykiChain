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

#include "chainparams.h"
#include "hash.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "key.h"
#include "serialize.h"
#include "uint256.h"

using namespace json_spirit;
using namespace std;

class CTxUndo;
class CValidationState;
class CAccountViewCache;
class CScriptDB;
class CBlock;
class CTransactionDBCache;
class CScriptDBViewCache;
class CRegID;
class CID;
class CAccountLog;
class COperVoteFund;
class CVoteFund;

static const int nTxVersion1 = 1;
static const int nTxVersion2 = 2;

typedef vector<unsigned char> vector_unsigned_char;

#define SCRIPT_ID_SIZE (6)

enum TxType {
    REWARD_TX   = 1,    //!< Miner Reward Tx
    REG_ACCT_TX = 2,    //!< Register Account Tx
    COMMON_TX   = 3,    //!< Base Coin Transfer Tx
    CONTRACT_TX = 4,    //!< Contract Tx
    REG_CONT_TX = 5,    //!< Register Contract Tx
    DELEGATE_TX = 6,    //!< Vote Delegate Tx

    /******** Begin of Stable Coin TX Type Enums ********/
    CDP_OPEN_TX             = 11, //!< CDP Collateralize Tx
    CDP_REFUEL_TX           = 12, //!< CDP Refuel Tx
    CDP_REDEMP_TX           = 13, //!< CDP Redemption Tx (partial or full)
    CDP_LIQUIDATE_TX        = 14, //!< CDP Liquidation Tx (partial or full)

    PRICE_FEED_WICC_TX      = 21, //!< Price Feed Tx: WICC/USD
    PRICE_FEED_MICC_TX      = 22, //!< Price Feed Tx: MICC/USD
    PRICE_FEED_WUSD_TX      = 23, //!< Price Feed Tx: WUSD/USD

    SFC_PARAM_MTX           = 31, //!< StableCoin Fund Committee invokes Param Set/Update MulSigTx
    SFC_GLOBAL_HALT_MTX     = 32, //!< StableCoin Fund Committee invokes Global Halt CDP Operations MulSigTx
    SFC_GLOBAL_SETTLE_MTX   = 33, //!< StableCoin Fund Committee invokes Global Settle Operation MulSigTx

    WUSD_TRANSFER_TX        = 41, //!< StableCoin WUSD Transfer Tx
    MICC_TRANSFER_TX        = 42, //!< FundCoin MICC Transfer Tx

    DEX_WICC_FOR_MICC_TX    = 51, //!< DEX: owner sells WICC for MICC Tx
    DEX_MICC_FOR_WICC_TX    = 52, //!< DEX: owner sells MICC for WICC Tx
    DEX_WICC_FOR_WUSD_TX    = 53, //!< DEX: owner sells WICC for WUSD Tx
    DEX_WUSD_FOR_WICC_TX    = 54, //!< DEX: owner sells WUSD for WICC Tx
    DEX_MICC_FOR_WUSD_TX    = 55, //!< DEX: owner sells MICC for WUSD Tx
    DEX_WUSD_FOR_MICC_TX    = 56, //!< DEX: owner sells WUSD for MICC Tx
    /******** End of Stable Coin Enums ********/

    NULL_TX = 0//!< NULL_TX
};

/**
 * brief:   kinds of fund type
 */
enum FundType {
    FREEDOM = 1,    //!< FREEDOM
    REWARD_FUND,    //!< REWARD_FUND
    NULL_FUNDTYPE,  //!< NULL_FUNDTYPE
};

enum OperType {
    ADD_FREE   = 1,  //!< add money to freedom
    MINUS_FREE = 2,  //!< minus money from freedom
    NULL_OPERTYPE,   //!< invalid operate type
};

enum VoteOperType {
    ADD_FUND   = 1,  //!< add operate
    MINUS_FUND = 2,  //!< minus operate
    NULL_OPER,       //!< invalid
};

class CNullID {
public:
    friend bool operator==(const CNullID &a, const CNullID &b) { return true; }
    friend bool operator<(const CNullID &a, const CNullID &b) { return true; }
};

typedef boost::variant<CNullID, CRegID, CKeyID, CPubKey> CUserID;

/*CRegID 是地址激活后，分配的账户ID*/
class CRegID {
private:
    uint32_t nHeight;
    uint16_t nIndex;
    mutable vector<unsigned char> vRegID;
    void SetRegIDByCompact(const vector<unsigned char> &vIn);
    void SetRegID(string strRegID);

public:
    friend class CID;
    CRegID(string strRegID);
    CRegID(const vector<unsigned char> &vIn);
    CRegID(uint32_t nHeight = 0, uint16_t nIndex = 0);

    const vector<unsigned char> &GetVec6() const {
        assert(vRegID.size() == 6);
        return vRegID;
    }
    void SetRegID(const vector<unsigned char> &vIn);
    CKeyID GetKeyID(const CAccountViewCache &view) const;
    uint32_t GetHeight() const { return nHeight; }
    bool operator==(const CRegID &co) const { return (this->nHeight == co.nHeight && this->nIndex == co.nIndex); }
    bool operator!=(const CRegID &co) const { return (this->nHeight != co.nHeight || this->nIndex != co.nIndex); }
    static bool IsSimpleRegIdStr(const string &str);
    static bool IsRegIdStr(const string &str);
    static bool GetKeyID(const string &str, CKeyID &keyId);
    bool IsEmpty() const { return (nHeight == 0 && nIndex == 0); };
    bool Clean();
    string ToString() const;

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(nHeight));
        READWRITE(VARINT(nIndex));
        if (fRead) {
            vRegID.clear();
            vRegID.insert(vRegID.end(), BEGIN(nHeight), END(nHeight));
            vRegID.insert(vRegID.end(), BEGIN(nIndex), END(nIndex));
        })
};

/*CID是一个vector 存放CRegID, CKeyID, CPubKey*/
class CID {
private:
    vector_unsigned_char vchData;

public:
    const vector_unsigned_char &GetID() { return vchData; }
    static const vector_unsigned_char &UserIDToVector(const CUserID &userid) { return CID(userid).GetID(); }
    bool Set(const CRegID &id);
    bool Set(const CKeyID &id);
    bool Set(const CPubKey &id);
    bool Set(const CNullID &id);
    bool Set(const CUserID &userid);
    CID() {}
    CID(const CUserID &dest) { Set(dest); }
    CUserID GetUserId();
    IMPLEMENT_SERIALIZE(
        READWRITE(vchData);)
};

class CIDVisitor : public boost::static_visitor<bool> {
private:
    CID *pId;

public:
    CIDVisitor(CID *pIdIn) : pId(pIdIn) {}
    bool operator()(const CRegID &id) const { return pId->Set(id); }
    bool operator()(const CKeyID &id) const { return pId->Set(id); }
    bool operator()(const CPubKey &id) const { return pId->Set(id); }
    bool operator()(const CNullID &no) const { return true; }
};

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
    uint64_t nRunStep;  // only in memory
    int nFuelRate;      // only in memory
    uint256 sigHash;    // only in memory

private:
    static unordered_map<unsigned char, string> txTypeMap;
    static void InitializeTxTypeMap() {
        txTypeMap[REWARD_TX]            = "REWARD_TX";
        txTypeMap[REG_ACCT_TX]          = "REG_ACCT_TX";
        txTypeMap[COMMON_TX]            = "COMMON_TX";
        txTypeMap[CONTRACT_TX]          = "CONTRACT_TX";
        txTypeMap[REG_CONT_TX]          = "REG_CONT_TX";
        txTypeMap[DELEGATE_TX]          = "DELEGATE_TX";
        txTypeMap[CDP_OPEN_TX]          = "CDP_OPEN_TX";
        txTypeMap[CDP_REFUEL_TX]        = "CDP_REFUEL_TX";
        txTypeMap[CDP_REDEMP_TX]        = "CDP_REDEMP_TX";
        txTypeMap[CDP_LIQUIDATE_TX]     = "CDP_LIQUIDATE_TX";
        txTypeMap[PRICE_FEED_WICC_TX]   = "PRICE_FEED_WICC_TX";
        txTypeMap[PRICE_FEED_MICC_TX]   = "PRICE_FEED_MICC_TX";
        txTypeMap[PRICE_FEED_WUSD_TX]   = "PRICE_FEED_WUSD_TX";
        txTypeMap[SFC_PARAM_MTX]        = "SFC_PARAM_MTX";
        txTypeMap[SFC_GLOBAL_HALT_MTX]  = "SFC_GLOBAL_HALT_MTX";
        txTypeMap[SFC_GLOBAL_SETTLE_MTX]= "SFC_GLOBAL_SETTLE_MTX";
        txTypeMap[WUSD_TRANSFER_TX]     = "WUSD_TRANSFER_TX";
        txTypeMap[MICC_TRANSFER_TX]     = "MICC_TRANSFER_TX";
        txTypeMap[DEX_WICC_FOR_MICC_TX] = "DEX_WICC_FOR_MICC_TX";
        txTypeMap[DEX_MICC_FOR_WICC_TX] = "DEX_MICC_FOR_WICC_TX";
        txTypeMap[DEX_WICC_FOR_WUSD_TX] = "DEX_WICC_FOR_WUSD_TX";
        txTypeMap[DEX_WUSD_FOR_WICC_TX] = "DEX_WUSD_FOR_WICC_TX";
        txTypeMap[DEX_MICC_FOR_WUSD_TX] = "DEX_MICC_FOR_WUSD_TX";
        txTypeMap[DEX_WUSD_FOR_MICC_TX] = "DEX_WUSD_FOR_MICC_TX";
        txTypeMap[NULL_TX]              = "NULL_TX";
    }

public:
    CBaseTx(const CBaseTx &other) { *this = other; InitializeTxTypeMap(); }

    CBaseTx(int nVersionIn, unsigned char nTxTypeIn, int nValidHeightIn, uint64_t llFeesIn) :
        nVersion(nVersionIn), nTxType(nTxTypeIn), nValidHeight(nValidHeightIn), llFees(llFeesIn), nRunStep(0), nFuelRate(0)
        { InitializeTxTypeMap(); }

    CBaseTx(unsigned char nTxTypeIn, int nValidHeightIn, uint64_t llFeesIn) :
        nVersion(CURRENT_VERSION), nTxType(nTxTypeIn), nValidHeight(nValidHeightIn), llFees(llFeesIn), nRunStep(0), nFuelRate(0)
        { InitializeTxTypeMap(); }

    CBaseTx(int nVersionIn, unsigned char nTxTypeIn) :
        nVersion(nVersionIn), nTxType(nTxTypeIn), nValidHeight(0), llFees(0), nRunStep(0), nFuelRate(0)
        { InitializeTxTypeMap(); }

    CBaseTx(unsigned char nTxTypeIn) :
        nVersion(CURRENT_VERSION), nTxType(nTxTypeIn), nValidHeight(0), llFees(0), nRunStep(0), nFuelRate(0)
        { InitializeTxTypeMap(); }

    virtual ~CBaseTx() {}
    virtual unsigned int GetSerializeSize(int nType, int nVersion) const = 0;
    virtual uint256 GetHash() const = 0;
    virtual uint64_t GetFee() const = 0;
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
    virtual uint64_t GetFuel(int nfuelRate);
    int GetFuelRate(CScriptDBViewCache &scriptDB);
    string GetTxType(unsigned char nTxType) const { return txTypeMap[nTxType]; };

protected:
    bool CheckMinTxFee(uint64_t llFees);
    bool CheckSignatureSize(vector<unsigned char> &signature);
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
        CID id(userId);
        READWRITE(id);
        CID mMinerid(minerId);
        READWRITE(mMinerid);
        if (fRead) {
            userId  = id.GetUserId();
            minerId = mMinerid.GetUserId();
        }
        READWRITE(VARINT(llFees));
        READWRITE(signature);)

    uint64_t GetFee() const { return llFees; }
    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            CID userPubkey(userId);
            CID minerPubkey(minerId);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << userPubkey << minerPubkey
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
    uint64_t bcoinBalance;          // transfer amount
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
            assert(!boost::get<CRegID>(srcUserIdIn).IsEmpty());

        if (desUserIdIn.type() == typeid(CRegID))
            assert(!boost::get<CRegID>(desUserIdIn).IsEmpty());

        srcUserId   = srcUserIdIn;
        desUserId   = desUserIdIn;
        bcoinBalance    = valueIn;
        memo        = descriptionIn;
    }

    CCommonTx(const CUserID &srcUserIdIn, CUserID desUserIdIn, uint64_t feeIn, uint64_t valueIn,
              int validHeightIn): CBaseTx(COMMON_TX, validHeightIn, feeIn) {
        if (srcUserIdIn.type() == typeid(CRegID))
            assert(!boost::get<CRegID>(srcUserIdIn).IsEmpty());

        if (desUserIdIn.type() == typeid(CRegID))
            assert(!boost::get<CRegID>(desUserIdIn).IsEmpty());

        srcUserId    = srcUserIdIn;
        desUserId    = desUserIdIn;
        bcoinBalance     = valueIn;
    }

    ~CCommonTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        CID srcId(srcUserId);
        READWRITE(srcId);
        CID desId(desUserId);
        READWRITE(desId);
        READWRITE(VARINT(llFees));
        READWRITE(VARINT(bcoinBalance));
        READWRITE(memo);
        READWRITE(signature);
        if (fRead) {
            srcUserId  = srcId.GetUserId();
            desUserId = desId.GetUserId();
        })

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            CID srcId(srcUserId);
            CID desId(desUserId);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << srcId << desId
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
    uint64_t bcoinBalance;          // transfer amount
    vector_unsigned_char arguments;
    vector_unsigned_char signature;

public:
    CContractTx(): CBaseTx(CONTRACT_TX) {}

    CContractTx(const CBaseTx *pBaseTx): CBaseTx(CONTRACT_TX) {
        assert(CONTRACT_TX == pBaseTx->nTxType);
        *this = *(CContractTx *)pBaseTx;
    }

    CContractTx(const CUserID &srcRegIdIn, CUserID desUserIdIn, uint64_t feeIn,
                uint64_t valueIn, int validHeightIn, vector_unsigned_char &argumentsIn):
                CBaseTx(CONTRACT_TX, validHeightIn, feeIn) {
        if (srcRegIdIn.type() == typeid(CRegID))
            assert(!boost::get<CRegID>(srcRegIdIn).IsEmpty());

        if (desUserIdIn.type() == typeid(CRegID))
            assert(!boost::get<CRegID>(desUserIdIn).IsEmpty());

        srcRegId     = srcRegIdIn;
        desUserId    = desUserIdIn;
        bcoinBalance     = valueIn;
        arguments    = argumentsIn;
    }

    CContractTx(const CUserID &srcRegIdIn, CUserID desUserIdIn, uint64_t feeIn,
                uint64_t valueIn, int validHeightIn):
                CBaseTx(CONTRACT_TX, validHeightIn, feeIn) {
        if (srcRegIdIn.type() == typeid(CRegID))
            assert(!boost::get<CRegID>(srcRegIdIn).IsEmpty());

        if (desUserIdIn.type() == typeid(CRegID))
            assert(!boost::get<CRegID>(desUserIdIn).IsEmpty());

        srcRegId     = srcRegIdIn;
        desUserId    = desUserIdIn;
        bcoinBalance     = valueIn;
    }

    ~CContractTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight)); CID srcId(srcRegId);
        READWRITE(srcId);
        CID desId(desUserId);
        READWRITE(desId);
        READWRITE(VARINT(llFees));
        READWRITE(VARINT(bcoinBalance));
        READWRITE(arguments);
        READWRITE(signature);
        if (fRead) {
            srcRegId  = srcId.GetUserId();
            desUserId = desId.GetUserId();
        })

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            CID srcId(srcRegId);
            CID desId(desUserId);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << srcId << desId
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
        CID acctId(account);
        READWRITE(acctId);
        if (fRead) {
            account = acctId.GetUserId();
        }
        READWRITE(VARINT(rewardValue));
        READWRITE(VARINT(nHeight));)

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            CID accId(account);
            ss << VARINT(nVersion) << nTxType << accId << VARINT(rewardValue) << VARINT(nHeight);
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
        CID regId(regAcctId);
        READWRITE(regId);
        if (fRead) {
            regAcctId = regId.GetUserId();
        }
        READWRITE(script);
        READWRITE(VARINT(llFees));
        READWRITE(signature);)

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            CID regAccId(regAcctId);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << regAccId << script
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
    CDelegateTx(const vector_unsigned_char &accountIn, vector<COperVoteFund> &operVoteFundsIn,
                const uint64_t feeIn, const int validHeightIn):
                CBaseTx(DELEGATE_TX, validHeightIn, feeIn) {
        if (accountIn.size() > 6) {
            userId = CPubKey(accountIn);
        } else {
            userId = CRegID(accountIn);
        }
        operVoteFunds = operVoteFundsIn;
    }
    CDelegateTx(const CUserID &userIdIn, uint64_t feeIn, const vector<COperVoteFund> &operVoteFundsIn,
                const int validHeightIn):
                CBaseTx(DELEGATE_TX, validHeightIn, feeIn) {
        if (userIdIn.type() == typeid(CRegID))
            assert(!boost::get<CRegID>(userIdIn).IsEmpty());

        userId        = userIdIn;
        operVoteFunds = operVoteFundsIn;
    }
    CDelegateTx(): CBaseTx(DELEGATE_TX) {}
    ~CDelegateTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        CID ID(userId);
        READWRITE(ID);
        READWRITE(operVoteFunds);
        READWRITE(VARINT(llFees));
        READWRITE(signature);
        if (fRead) {
            userId = ID.GetUserId();
        })

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            CID accId(userId);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << accId << operVoteFunds
               << VARINT(llFees);
            // Truly need to write the sigHash.
            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }

        return sigHash;
    }

    uint256 GetHash() const { return SignatureHash(); }
    uint64_t GetFee() const { return llFees; }
    double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDelegateTx>(this); }
    string ToString(CAccountViewCache &view) const;
    Object ToJson(const CAccountViewCache &accountView) const;
    bool GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
    bool ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo, int nHeight,
                   CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB);
    bool CheckTx(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
};

class CVoteFund {
public:
    CPubKey pubKey;   //!< delegates public key
    uint64_t value;   //!< amount of vote
    uint256 sigHash;  // only in memory

public:
    CVoteFund() {
        value  = 0;
        pubKey = CPubKey();
    }
    CVoteFund(uint64_t valueIn) {
        value  = valueIn;
        pubKey = CPubKey();
    }
    CVoteFund(uint64_t valueIn, CPubKey pubKeyIn) {
        value  = valueIn;
        pubKey = pubKeyIn;
    }
    CVoteFund(const CVoteFund &fund) {
        value  = fund.value;
        pubKey = fund.pubKey;
    }
    CVoteFund &operator=(const CVoteFund &fund) {
        if (this == &fund) {
            return *this;
        }
        this->value  = fund.value;
        this->pubKey = fund.pubKey;
        return *this;
    }
    ~CVoteFund() {}

    uint256 GetHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(value) << pubKey;
            // Truly need to write the sigHash.
            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }

        return sigHash;
    }

    friend bool operator<(const CVoteFund &fa, const CVoteFund &fb) {
        if (fa.value <= fb.value)
            return true;
        else
            return false;
    }
    friend bool operator>(const CVoteFund &fa, const CVoteFund &fb) {
        return !operator<(fa, fb);
    }
    friend bool operator==(const CVoteFund &fa, const CVoteFund &fb) {
        if (fa.pubKey != fb.pubKey)
            return false;
        if (fa.value != fb.value)
            return false;
        return true;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(pubKey);
        READWRITE(VARINT(value));
    )

    string ToString(bool isAddress = false) const {
        string str("");
        str += "pubKey:";
        if (isAddress) {
            str += pubKey.GetKeyID().ToAddress();
        } else {
            str += pubKey.ToString();
        }
        str += " value:";
        str += strprintf("%s", value);
        str += "\n";
        return str;
    }
    Object ToJson(bool isAddress = false) const;
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
    friend bool operator<(const CScriptDBOperLog &log1, const CScriptDBOperLog &log2) { return log1.vKey < log2.vKey; }
};

class COperVoteFund {
public:
    static string voteOperTypeArray[3];

public:
    unsigned char operType;  //!<1:ADD_FUND 2:MINUS_FUND
    CVoteFund fund;

    IMPLEMENT_SERIALIZE(
        READWRITE(operType);
        READWRITE(fund);)
public:
    COperVoteFund() {
        operType = NULL_OPER;
    }
    COperVoteFund(unsigned char nType, const CVoteFund &operFund) {
        operType = nType;
        fund     = operFund;
    }
    string ToString(bool isAddress = false) const;
    Object ToJson(bool isAddress = false) const;
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

class CAccount {
public:
    CRegID regID;                   //!< regID of the account
    CKeyID keyID;                   //!< keyID of the account
    CPubKey pubKey;                 //!< public key of the account
    CPubKey minerPubKey;            //!< miner public key of the account
    uint64_t bcoinBalance;          //!< BaseCoin balance
    uint64_t scoinBalance;          //!< StableCoin balance
    uint64_t fcoinBalance;          //!< StabeFundCoin balance
    uint64_t nVoteHeight;           //!< account vote block height
    vector<CVoteFund> vVoteFunds;   //!< account delegate votes order by vote value
    uint64_t llVotes;               //!< votes received
    bool hasOpenCdp;                //!< Whether the account has open CDP or not. If true, it exists in a map container

    uint256 sigHash;                // only in memory

public:
    /**
     * @brief operate account
     * @param type: operate type
     * @param values
     * @param nCurHeight:  the tip block height
     * @return returns true if successful, otherwise false
     */
    bool OperateAccount(OperType type, const uint64_t &values, const uint64_t nCurHeight);
    bool UndoOperateAccount(const CAccountLog &accountLog);
    bool ProcessDelegateVote(vector<COperVoteFund> &operVoteFunds, const uint64_t nCurHeight);
    bool OperateVote(VoteOperType type, const uint64_t &values);

public:
    CAccount(CKeyID &keyId, CPubKey &pubKey) : 
        keyID(keyId), pubKey(pubKey), bcoinBalance(0), scoinBalance(0), fcoinBalance(0), nVoteHeight(0), llVotes(0) {
        minerPubKey = CPubKey();
        vVoteFunds.clear();
        regID.Clean();
    }

    CAccount() : keyID(uint160()), bcoinBalance(0), scoinBalance(0), fcoinBalance(0), nVoteHeight(0), llVotes(0) {
        pubKey      = CPubKey();
        minerPubKey = CPubKey();
        vVoteFunds.clear();
        regID.Clean();
    }

    CAccount(const CAccount &other) {
        this->regID       = other.regID;
        this->keyID       = other.keyID;
        this->pubKey      = other.pubKey;
        this->minerPubKey = other.minerPubKey;
        this->bcoinBalance = other.bcoinBalance;
        this->scoinBalance = other.scoinBalance;
        this->fcoinBalance = other.fcoinBalance;
        this->nVoteHeight = other.nVoteHeight;
        this->vVoteFunds  = other.vVoteFunds;
        this->llVotes     = other.llVotes;
    }

    CAccount &operator=(const CAccount &other) {
        if (this == &other)
            return *this;

        this->regID       = other.regID;
        this->keyID       = other.keyID;
        this->pubKey      = other.pubKey;
        this->minerPubKey = other.minerPubKey;
        this->bcoinBalance = other.bcoinBalance;
        this->scoinBalance = other.scoinBalance;
        this->fcoinBalance = other.fcoinBalance;
        this->nVoteHeight = other.nVoteHeight;
        this->vVoteFunds  = other.vVoteFunds;
        this->llVotes     = other.llVotes;

        return *this;
    }

    std::shared_ptr<CAccount> GetNewInstance() const { return std::make_shared<CAccount>(*this); }
    bool IsRegistered() const { return (pubKey.IsFullyValid() && pubKey.GetKeyID() == keyID); }
    bool SetRegId(const CRegID &regID) {
        this->regID = regID;
        return true;
    };
    bool GetRegId(CRegID &regID) const {
        regID = this->regID;
        return !regID.IsEmpty();
    };
    uint64_t GetRawBalance();
    uint64_t GetTotalBalance();
    uint64_t GetFrozenBalance();
    uint64_t GetAccountProfit(uint64_t prevBlockHeight);
    string ToString(bool isAddress = false) const;
    Object ToJsonObj(bool isAddress = false) const;
    bool IsEmptyValue() const { return !(bcoinBalance > 0); }

    uint256 GetHash(bool recalculate = false) {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            //FIXME: need to check block soft-fork height here
            ss << regID << keyID << pubKey << minerPubKey 
                << VARINT(bcoinBalance) << VARINT(scoinBalance) << VARINT(fcoinBalance)
                << VARINT(nVoteHeight)
                << vVoteFunds << llVotes;
        
            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }

        return sigHash;
    }

    bool UpDateAccountPos(int nCurHeight);

    IMPLEMENT_SERIALIZE(
        READWRITE(regID);
        READWRITE(keyID);
        READWRITE(pubKey);
        READWRITE(minerPubKey);
        READWRITE(VARINT(bcoinBalance));
        READWRITE(VARINT(nVoteHeight));
        READWRITE(vVoteFunds);
        READWRITE(llVotes);)

    uint64_t GetReceiveVotes() const { return llVotes; }

private:
    bool IsMoneyOverflow(uint64_t nAddMoney);
};

class CAccountLog {
public:
    CKeyID keyID;
    uint64_t bcoinBalance;             //!< freedom money which coinage greater than 30 days
    uint64_t nVoteHeight;          //!< account vote height
    vector<CVoteFund> vVoteFunds;  //!< delegate votes
    uint64_t llVotes;              //!< votes received

    IMPLEMENT_SERIALIZE(
        READWRITE(keyID);
        READWRITE(VARINT(bcoinBalance));
        READWRITE(VARINT(nVoteHeight));
        READWRITE(vVoteFunds);
        READWRITE(llVotes);)

public:
    CAccountLog(const CAccount &acct) {
        keyID       = acct.keyID;
        bcoinBalance= acct.bcoinBalance;
        nVoteHeight = acct.nVoteHeight;
        vVoteFunds  = acct.vVoteFunds;
        llVotes     = acct.llVotes;
    }
    CAccountLog(CKeyID &keyId) {
        keyID       = keyId;
        bcoinBalance    = 0;
        nVoteHeight = 0;
        llVotes     = 0;
    }
    CAccountLog() {
        keyID       = uint160();
        bcoinBalance  = 0;
        nVoteHeight = 0;
        vVoteFunds.clear();
        llVotes = 0;
    }
    void SetValue(const CAccount &acct) {
        keyID       = acct.keyID;
        bcoinBalance = acct.bcoinBalance;
        nVoteHeight = acct.nVoteHeight;
        llVotes     = acct.llVotes;
        vVoteFunds  = acct.vVoteFunds;
    }
    string ToString() const;
};

inline unsigned int GetSerializeSize(const std::shared_ptr<CBaseTx> &pa, int nType, int nVersion) {
    return pa->GetSerializeSize(nType, nVersion) + 1;
}

template <typename Stream>
void Serialize(Stream &os, const std::shared_ptr<CBaseTx> &pa, int nType, int nVersion) {
    unsigned char ntxType = pa->nTxType;
    Serialize(os, ntxType, nType, nVersion);
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
    } else
        throw ios_base::failure("seiralize tx type value error, must be ranger(1...6)");
}

template <typename Stream>
void Unserialize(Stream &is, std::shared_ptr<CBaseTx> &pa, int nType, int nVersion) {
    char nTxType;
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
    } else {
        string sTxType(1, nTxType);
        throw ios_base::failure("Unserialize: nTxType (" + sTxType + ") value error, must be within range (1...6)");
    }
    pa->nTxType = nTxType;
}

#endif
