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
#include "entities/account.h"
#include "entities/asset.h"
#include "entities/id.h"
#include "persistence/contractdb.h"
#include "config/configuration.h"
#include "config/txbase.h"
#include "config/scoin.h"

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

using namespace std;

class CCacheWrapper;
class CValidationState;
class CContractDBCache;

typedef uint256 TxID;

string GetTxType(const TxType txType);
bool GetTxMinFee(const TxType nTxType, int height, const TokenSymbol &symbol, uint64_t &feeOut);

class CBaseTx {
public:
    static uint64_t nMinTxFee;
    static uint64_t nMinRelayTxFee;
    static uint64_t nDustAmountThreshold;
    static const int32_t CURRENT_VERSION = INIT_TX_VERSION;

    int32_t nVersion;
    TxType nTxType;
    mutable CUserID txUid;
    int32_t valid_height;
    TokenSymbol fee_symbol; // fee symbol, default is WICC, some tx (MAJOR_VER_R1) not serialize this field
    uint64_t llFees;
    UnsignedCharArray signature;

    uint64_t nRunStep;     //!< only in memory
    int32_t nFuelRate;     //!< only in memory
    mutable TxID sigHash;  //!< only in memory

public:
    CBaseTx(int32_t nVersionIn, TxType nTxTypeIn, CUserID txUidIn, int32_t nValidHeightIn, uint64_t llFeesIn) :
        nVersion(nVersionIn), nTxType(nTxTypeIn), txUid(txUidIn), valid_height(nValidHeightIn),
        fee_symbol(SYMB::WICC), llFees(llFeesIn), nRunStep(0), nFuelRate(0) {}

    CBaseTx(TxType nTxTypeIn, CUserID txUidIn, int32_t nValidHeightIn, TokenSymbol feeSymbolIn, uint64_t llFeesIn) :
        nVersion(CURRENT_VERSION), nTxType(nTxTypeIn), txUid(txUidIn), valid_height(nValidHeightIn),
        fee_symbol(feeSymbolIn), llFees(llFeesIn), nRunStep(0), nFuelRate(0) {}

    CBaseTx(TxType nTxTypeIn, CUserID txUidIn, int32_t nValidHeightIn, uint64_t llFeesIn) :
        nVersion(CURRENT_VERSION), nTxType(nTxTypeIn), txUid(txUidIn), valid_height(nValidHeightIn),
        fee_symbol(SYMB::WICC), llFees(llFeesIn), nRunStep(0), nFuelRate(0) {}

    CBaseTx(int32_t nVersionIn, TxType nTxTypeIn) :
        nVersion(nVersionIn), nTxType(nTxTypeIn), valid_height(0), fee_symbol(SYMB::WICC), llFees(0), nRunStep(0),
        nFuelRate(0) {}

    CBaseTx(TxType nTxTypeIn) :
        nVersion(CURRENT_VERSION), nTxType(nTxTypeIn), valid_height(0), fee_symbol(SYMB::WICC), llFees(0), nRunStep(0),
        nFuelRate(0) {}

    virtual ~CBaseTx() {}

    virtual std::pair<TokenSymbol, uint64_t> GetFees() const { return std::make_pair(SYMB::WICC, llFees); }
    virtual TxID GetHash() const { return ComputeSignatureHash(); }
    virtual uint32_t GetSerializeSize(int32_t nType, int32_t nVersion) const { return 0; }

    virtual uint64_t GetFuel(uint32_t nFuelRate);
    uint32_t GetFuelRate(CContractDBCache &contractCache);
    virtual double GetPriority() const {
        return kTransactionPriorityCeiling / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION);
    }
    virtual TxID ComputeSignatureHash(bool recalculate = false) const = 0;
    virtual std::shared_ptr<CBaseTx> GetNewInstance() const           = 0;
    virtual string ToString(CAccountDBCache &accountCache)           = 0;
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state)                  = 0;
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) = 0;

    bool IsValidHeight(int32_t nCurHeight, int32_t nTxCacheHeight) const;

    // If the sender has no regid before, geneate a regid for the sender.
    bool GenerateRegID(CAccount &account, CCacheWrapper &cw, CValidationState &state, const int32_t height,
                       const int32_t index);

    bool IsBlockRewardTx() { return nTxType == BLOCK_REWARD_TX || nTxType == UCOIN_BLOCK_REWARD_TX; }
    bool IsMedianPriceTx() { return nTxType == PRICE_MEDIAN_TX; }
    bool IsPriceFeedTx() { return nTxType == PRICE_FEED_TX; }

protected:
    bool CheckTxFeeSufficient(const TokenSymbol &feeSymbol, const uint64_t llFees, const int32_t height) const;
    bool CheckSignatureSize(const vector<unsigned char> &signature) const;
    bool CheckCoinRange(const TokenSymbol &symbol, const int64_t amount) const;

    static bool AddInvolvedKeyIds(vector<CUserID> uids, CCacheWrapper &cw, set<CKeyID> &keyIds);
};

class CPricePoint {
public:
    CoinPricePair coin_price_pair;
    uint64_t price;     // boosted by 10^4

public:
    CPricePoint() {}

    CPricePoint(const CoinPricePair &coinPricePair, const uint64_t priceIn)
        : coin_price_pair(coinPricePair), price(priceIn) {}

    CPricePoint(const CPricePoint& other) { *this = other; }

    ~CPricePoint() {}

public:
    uint64_t GetPrice() const { return price; }
    CoinPricePair GetCoinPricePair() const { return coin_price_pair; }

    string ToString() {
        return strprintf("coin_price_pair:%s:%s, price:%lld",
                        coin_price_pair.first, coin_price_pair.second, price);
    }

    json_spirit::Object ToJson() const {
        json_spirit::Object obj;

        obj.push_back(json_spirit::Pair("coin_symbol",      coin_price_pair.first));
        obj.push_back(json_spirit::Pair("price_symbol",     coin_price_pair.second));
        obj.push_back(json_spirit::Pair("price",            price));

        return obj;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(coin_price_pair);
        READWRITE(VARINT(price));)

    CPricePoint& operator=(const CPricePoint& other) {
        if (this == &other)
            return *this;

        this->coin_price_pair   = other.coin_price_pair;
        this->price             = other.price;

        return *this;
    }
};

#define IMPLEMENT_CHECK_TX_MEMO                                                                    \
    if (memo.size() > MAX_COMMON_TX_MEMO_SIZE)                                                     \
        return state.DoS(100, ERRORMSG("%s, memo's size too large", __FUNCTION__), REJECT_INVALID, \
                         "memo-size-toolarge");

#define IMPLEMENT_CHECK_TX_ARGUMENTS                                                                    \
    if (arguments.size() > MAX_CONTRACT_ARGUMENT_SIZE)                                                  \
        return state.DoS(100, ERRORMSG("%s, arguments's size too large, __FUNCTION__"), REJECT_INVALID, \
                         "arguments-size-toolarge");

#define IMPLEMENT_CHECK_TX_FEE                                                                         \
    if (!CheckBaseCoinRange(llFees))                                                                   \
        return state.DoS(100, ERRORMSG("%s, tx fee out of range", __FUNCTION__), REJECT_INVALID,       \
                         "bad-tx-fee-toolarge");                                                       \
     if (!kFeeSymbolSet.count(fee_symbol))                                                             \
        return state.DoS(100, ERRORMSG("%s, not support fee symbol=%s, only supports:%s",              \
            __FUNCTION__, fee_symbol, GetFeeSymbolSetStr()), REJECT_INVALID, "bad-tx-fee-symbol");     \
    if (!CheckTxFeeSufficient(fee_symbol, llFees, height)) {                                           \
        return state.DoS(100, ERRORMSG("%s, tx fee too small(height: %d, fee symbol: %s, fee: %llu",   \
            __FUNCTION__, height, fee_symbol, llFees), REJECT_INVALID, "bad-tx-fee-toosmall");         \
    }

#define IMPLEMENT_CHECK_TX_REGID(txUidType)                                                            \
    if (txUidType != typeid(CRegID)) {                                                                 \
        return state.DoS(100, ERRORMSG("%s, txUid must be CRegID", __FUNCTION__), REJECT_INVALID,      \
            "txUid-type-error");                                                                       \
    }

#define IMPLEMENT_CHECK_TX_APPID(appUidType)                                                       \
    if (appUidType != typeid(CRegID)) {                                                            \
        return state.DoS(100, ERRORMSG("%s, appUid must be CRegID", __FUNCTION__), REJECT_INVALID, \
                         "appUid-type-error");                                                     \
    }

#define IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUidType)                                                        \
    if ((txUidType != typeid(CRegID)) && (txUidType != typeid(CPubKey))) {                                   \
        return state.DoS(100, ERRORMSG("%s, txUid must be CRegID or CPubKey", __FUNCTION__), REJECT_INVALID, \
                         "txUid-type-error");                                                                \
    }

#define IMPLEMENT_CHECK_TX_REGID_OR_KEYID(toUidType)                                                        \
    if ((toUidType != typeid(CRegID)) && (toUidType != typeid(CKeyID))) {                                   \
        return state.DoS(100, ERRORMSG("%s, toUid must be CRegID or CKeyID", __FUNCTION__), REJECT_INVALID, \
                         "toUid-type-error");                                                               \
    }

#define IMPLEMENT_CHECK_TX_SIGNATURE(signatureVerifyPubKey)                                                          \
    if (!CheckSignatureSize(signature)) {                                                                            \
        return state.DoS(100, ERRORMSG("%s, tx signature size invalid", __FUNCTION__), REJECT_INVALID,               \
                         "bad-tx-sig-size");                                                                         \
    }                                                                                                                \
    uint256 sighash = ComputeSignatureHash();                                                                        \
    if (!VerifySignature(sighash, signature, signatureVerifyPubKey)) {                                               \
        return state.DoS(100, ERRORMSG("%s, tx signature error", __FUNCTION__), REJECT_INVALID, "bad-tx-signature"); \
    }


#endif //COIN_BASETX_H
