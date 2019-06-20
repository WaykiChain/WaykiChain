// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_BASETX_H
#define COIN_BASETX_H

#include "commons/serialize.h"
#include "commons/uint256.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "accounts/account.h"
#include "accounts/id.h"
#include "persistence/contractdb.h"

#include <boost/variant.hpp>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

using namespace std;

class CCacheWrapper;
class CValidationState;
class CContractCache;

static const int32_t nTxVersion1 = 1;
static const int32_t nTxVersion2 = 2;

static const bool kGlobalStableCoinLockIsOn         = false;    // when true, CDP cannot be added but can be closed.
                                                                // scoins cannot be sold in DEX
static const uint16_t kDefaultOpenLiquidateRatio    = 15000;    // 150% * 10000
static const uint16_t kDefaultForcedLiquidateRatio  = 10000;    // 100% * 10000
static const uint16_t kDefaultCdpLoanInterest       = 350;      // 3.5% * 10000
static const uint16_t kDefaultCdpPenaltyFeeRatio    = 1300;     // 13% * 10000

static const uint32_t kTotalFundCoinAmount = 21000000;              // 21 million MICC
static const uint32_t kDefaultPriceFeedStakedFcoinsMin = 210000;    // 1%: min 210K fcoins deposited to be a price feeder
static const uint16_t kDefaultPriceFeedDeviateAcceptLimit = 3000;   // 30% * 10000, above than that will be penalized
static const uint16_t kDefaultPriceFeedDeviatePenalty = 1000;       // 1000 bcoins deduction as penalty
static const uint16_t kDefaultPriceFeedContinuousDeviateTimesLimit = 10;  // after 10 times continuous deviate limit penetration all deposit be deducted
static const uint16_t kDefaultPriceFeedTxFee = 10000;   // 10000 sawi


#define SCRIPT_ID_SIZE (6)

enum TxType : unsigned char {
    BLOCK_REWARD_TX     = 1,  //!< Miner Block Reward Tx
    ACCOUNT_REGISTER_TX = 2,  //!< Account Registration Tx
    BCOIN_TRANSFER_TX   = 3,  //!< BaseCoin Transfer Tx
    CONTRACT_INVOKE_TX  = 4,  //!< Contract Invocation Tx
    CONTRACT_DEPLOY_TX  = 5,  //!< Contract Deployment Tx
    DELEGATE_VOTE_TX    = 6,  //!< Vote Delegate Tx
    COMMON_MTX          = 7,  //!< Multisig Tx

    BLOCK_PRICE_MEDIAN_TX = 8, // Block Median Price Tx
    /******** Begin of Stable Coin TX Type Enums ********/
    CDP_OPEN_TX         = 11,   //!< CDP Open Tx
    CDP_REFUEL_TX       = 12,   //!< CDP refuel Tx
    CDP_REDEEMP_TX       = 13,   //!< CDP Redemption Tx (partial or full)
    CDP_LIQUIDATE_TX    = 14,   //!< CDP Liquidation Tx (partial or full)
    CDP_STAKE_TX        = 15,   //!< CDP Staking/Restaking Tx

    PRICE_FEED_TX = 22,  //!< Price Feed Tx: WICC/USD | MICC/WUSD | WUSD/USD

    SFC_PARAM_MTX         = 31,  //!< StableCoin Fund Committee invokes Param Set/Update MulSigTx
    SFC_GLOBAL_HALT_MTX   = 32,  //!< StableCoin Fund Committee invokes Global Halt CDP Operations MulSigTx
    SFC_GLOBAL_SETTLE_MTX = 33,  //!< StableCoin Fund Committee invokes Global Settle Operation MulSigTx

    SCOIN_TRANSFER_TX = 41,  //!< StableCoin Transfer Tx
    FCOIN_TRANSFER_TX = 42,  //!< FundCoin Transfer Tx
    FCOIN_STAKE_TX    = 43,  //!< Stake Fund Coin in order to become a price feeder

    DEX_BUY_ORDER_TX        = 51,
    DEX_SELL_ORDER_TX       = 52,
    DEX_SETTLE_TX           = 53,

    DEX_WICC_FOR_MICC_TX = 61,  //!< DEX: owner sells WICC for MICC Tx
    DEX_MICC_FOR_WICC_TX = 62,  //!< DEX: owner sells MICC for WICC Tx
    DEX_WICC_FOR_WUSD_TX = 63,  //!< DEX: owner sells WICC for WUSD Tx
    DEX_WUSD_FOR_WICC_TX = 64,  //!< DEX: owner sells WUSD for WICC Tx
    DEX_MICC_FOR_WUSD_TX = 65,  //!< DEX: owner sells MICC for WUSD Tx
    DEX_WUSD_FOR_MICC_TX = 66,  //!< DEX: owner sells WUSD for MICC Tx
    /******** End of Stable Coin Enums ********/

    NULL_TX = 0  //!< NULL_TX
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
    { CDP_REDEEMP_TX,       "CDP_REDEEMP_TX" },
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
    static const int32_t CURRENT_VERSION = nTxVersion1;

    int32_t nVersion;
    unsigned char nTxType;
    mutable CUserID txUid;
    int32_t nValidHeight;
    uint64_t llFees;
    vector_unsigned_char signature;

    uint64_t nRunStep;        //!< only in memory
    int32_t nFuelRate;            //!< only in memory
    mutable uint256 sigHash;  //!< only in memory

public:
    CBaseTx(const CBaseTx &other) { *this = other; }

    CBaseTx(int32_t nVersionIn, TxType nTxTypeIn, CUserID txUidIn, int32_t nValidHeightIn, uint64_t llFeesIn) :
        nVersion(nVersionIn), nTxType(nTxTypeIn), txUid(txUidIn), nValidHeight(nValidHeightIn), llFees(llFeesIn),
        nRunStep(0), nFuelRate(0) {}

    CBaseTx(TxType nTxTypeIn, CUserID txUidIn, int32_t nValidHeightIn, uint64_t llFeesIn) :
        nVersion(CURRENT_VERSION), nTxType(nTxTypeIn), txUid(txUidIn), nValidHeight(nValidHeightIn), llFees(llFeesIn),
        nRunStep(0), nFuelRate(0) {}

    CBaseTx(int32_t nVersionIn, TxType nTxTypeIn) :
        nVersion(nVersionIn), nTxType(nTxTypeIn),
        nValidHeight(0), llFees(0), nRunStep(0), nFuelRate(0) {}

    CBaseTx(TxType nTxTypeIn) :
        nVersion(CURRENT_VERSION), nTxType(nTxTypeIn),
        nValidHeight(0), llFees(0), nRunStep(0), nFuelRate(0) {}

    virtual ~CBaseTx() {}

    virtual uint64_t GetFee() const { return llFees; }
    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual uint32_t GetSerializeSize(int32_t nType, int32_t nVersion) const { return 0; }

    virtual uint64_t GetFuel(int32_t nFuelRate);
    virtual double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    virtual uint64_t GetValue() const { return 0; }

    virtual uint256 ComputeSignatureHash(bool recalculate = false) const = 0;
    virtual std::shared_ptr<CBaseTx> GetNewInstance()                    = 0;

    virtual string ToString(CAccountCache &view)                           = 0;
    virtual Object ToJson(const CAccountCache &view) const                 = 0;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) = 0;

    virtual bool CheckTx(int32_t nHeight, CCacheWrapper &cw, CValidationState &state)                       = 0;
    virtual bool ExecuteTx(int32_t nHeight, int32_t nIndex, CCacheWrapper &cw, CValidationState &state)     = 0;
    virtual bool UndoExecuteTx(int32_t nHeight, int32_t nIndex, CCacheWrapper &cw, CValidationState &state) = 0;

    int32_t GetFuelRate(CContractCache &scriptDB);
    bool IsValidHeight(int32_t nCurHeight, int32_t nTxCacheHeight) const;
    bool IsCoinBase() { return (nTxType == BLOCK_REWARD_TX); }

protected:
    bool CheckMinTxFee(const uint64_t llFees, const int32_t nHeight) const;
    bool CheckSignatureSize(const vector<unsigned char> &signature) const ;
protected:
    static bool SaveTxAddresses(int32_t nHeight, int32_t nIndex, CCacheWrapper &cw, const vector<CUserID> &userIds);
};

class CCoinPriceType {
public:
    unsigned char coinType;
    unsigned char priceType;

    CCoinPriceType(CoinType coinTypeIn, PriceType priceTypeIn) :
        coinType(coinTypeIn), priceType(priceTypeIn) {}

    string ToString() { return strprintf("%u%u", coinType, priceType); }

    IMPLEMENT_SERIALIZE(
        READWRITE(coinType);
        READWRITE(priceType);)
};

class CPricePoint {
private:
    CCoinPriceType coinPriceType;
    uint64_t price;

public:
    CPricePoint(CCoinPriceType coinPriceTypeIn, uint64_t priceIn)
        : coinPriceType(coinPriceTypeIn), price(priceIn) {}

    CPricePoint(CoinType coinTypeIn, PriceType priceTypeIn, uint64_t priceIn)
        : coinPriceType(coinTypeIn, priceTypeIn), price(priceIn) {}

    uint64_t GetPrice() { return price; }
    CCoinPriceType GetCoinPriceType() { return coinPriceType; }

    string ToString() {
        return strprintf("coinType:%u, priceType:%u, price:%lld",
                        coinPriceType.coinType, coinPriceType.priceType, price);
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(coinPriceType);
        READWRITE(VARINT(price));)
};

#define IMPLEMENT_CHECK_TX_MEMO                                                                             \
    if (memo.size() > kCommonTxMemoMaxSize)                                                                 \
        return state.DoS(100, ERRORMSG("%s::CheckTx, memo's size too large", __FUNCTION__), REJECT_INVALID, \
                         "memo-size-toolarge");

#define IMPLEMENT_CHECK_TX_ARGUMENTS                                                                             \
    if (arguments.size() > kContractArgumentMaxSize)                                                             \
        return state.DoS(100, ERRORMSG("%s::CheckTx, arguments's size too large, __FUNCTION__"), REJECT_INVALID, \
                         "arguments-size-toolarge");

#define IMPLEMENT_CHECK_TX_FEE                                                                                     \
    if (!CheckBaseCoinRange(llFees))                                                                               \
        return state.DoS(100, ERRORMSG("%s::CheckTx, tx fee out of range", __FUNCTION__), REJECT_INVALID,          \
                         "bad-tx-fee-toolarge");                                                                   \
                                                                                                                   \
    if (!CheckMinTxFee(llFees, nHeight)) {                                                                         \
        return state.DoS(100, ERRORMSG("%s::CheckTx, tx fee smaller than MinTxFee", __FUNCTION__), REJECT_INVALID, \
                         "bad-tx-fee-toosmall");                                                                   \
    }

#define IMPLEMENT_CHECK_TX_REGID(txUidType)                                                                \
    if (txUidType != typeid(CRegID)) {                                                                     \
        return state.DoS(100, ERRORMSG("%s::CheckTx, txUid must be CRegID", __FUNCTION__), REJECT_INVALID, \
                         "txUid-type-error");                                                              \
    }

#define IMPLEMENT_CHECK_TX_APPID(appUidType)                                                                \
    if (appUidType != typeid(CRegID)) {                                                                     \
        return state.DoS(100, ERRORMSG("%s::CheckTx, appUid must be CRegID", __FUNCTION__), REJECT_INVALID, \
                         "appUid-type-error");                                                              \
    }

#define IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUidType)                                                                 \
    if ((txUidType != typeid(CRegID)) && (txUidType != typeid(CPubKey))) {                                            \
        return state.DoS(100, ERRORMSG("%s::CheckTx, txUid must be CRegID or CPubKey", __FUNCTION__), REJECT_INVALID, \
                         "txUid-type-error");                                                                         \
    }

#define IMPLEMENT_CHECK_TX_REGID_OR_KEYID(toUidType)                                                                 \
    if ((toUidType != typeid(CRegID)) && (toUidType != typeid(CKeyID))) {                                            \
        return state.DoS(100, ERRORMSG("%s::CheckTx, toUid must be CRegID or CKeyID", __FUNCTION__), REJECT_INVALID, \
                         "toUid-type-error");                                                                        \
    }

#define IMPLEMENT_CHECK_TX_SIGNATURE(signatureVerifyPubKey)                                                     \
    if (!CheckSignatureSize(signature)) {                                                                       \
        return state.DoS(100, ERRORMSG("%s::CheckTx, tx signature size invalid", __FUNCTION__), REJECT_INVALID, \
                         "bad-tx-sig-size");                                                                    \
    }                                                                                                           \
    uint256 sighash = ComputeSignatureHash();                                                                   \
    if (!VerifySignature(sighash, signature, signatureVerifyPubKey)) {                                          \
        return state.DoS(100, ERRORMSG("%s::CheckTx, tx signature error", __FUNCTION__), REJECT_INVALID,        \
                         "bad-tx-signature");                                                                   \
    }

#endif //COIN_BASETX_H
