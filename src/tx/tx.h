// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_BASETX_H
#define COIN_BASETX_H

#include "commons/serialize.h"
#include "commons/uint256.h"
#include "entities/account.h"
#include "entities/asset.h"
#include "entities/id.h"
#include "config/configuration.h"
#include "config/txbase.h"
#include "config/scoin.h"

#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

using namespace std;

enum class TxExecuteContextType {
    NULL_TYPE               = 0,
    VALIDATE_MEMPOOL        = 1,
    CONNECT_BLOCK           = 2,
    PRODUCE_BLOCK           = 3,

};

static const unordered_map<TxExecuteContextType, string> kTxExecuteContextTypeNameMap {
    { TxExecuteContextType::VALIDATE_MEMPOOL,   "VALIDATE_MEMPOOL" },
    { TxExecuteContextType::CONNECT_BLOCK,      "CONNECT_BLOCK"    },
    { TxExecuteContextType::PRODUCE_BLOCK,      "PRODUCE_BLOCK"    },
};

class CCacheWrapper;
class CValidationState;

static const std::unordered_map<TxType, AccountPermType> kTxTypePermMap = {
    { BCOIN_TRANSFER_TX,            AccountPermType::PERM_SEND_COIN  },
    { DELEGATE_VOTE_TX,             AccountPermType::PERM_SEND_VOTE  },
    { UCOIN_TRANSFER_TX,            AccountPermType::PERM_SEND_COIN  },

    { UCOIN_STAKE_TX,               AccountPermType::PERM_STAKE_COIN },

    { UTXO_TRANSFER_TX,             AccountPermType::PERM_SEND_UTXO  },
    { UTXO_PASSWORD_PROOF_TX,       AccountPermType::PERM_SEND_UTXO  },

    { LCONTRACT_DEPLOY_TX,          AccountPermType::PERM_DEPLOY_SC  },
    { LCONTRACT_INVOKE_TX,          AccountPermType::PERM_INVOKE_SC  },
    { UCONTRACT_DEPLOY_R2_TX,          AccountPermType::PERM_DEPLOY_SC  },
    { UCONTRACT_INVOKE_R2_TX,          AccountPermType::PERM_INVOKE_SC  },
    { UNIVERSAL_CONTRACT_TX,             AccountPermType::PERM_INVOKE_SC  },
    { PRICE_FEED_TX,                AccountPermType::PERM_FEED_PRICE },

    { CDP_STAKE_TX,                 AccountPermType::PERM_CDP        },
    { CDP_REDEEM_TX,                AccountPermType::PERM_CDP        },
    { CDP_LIQUIDATE_TX,             AccountPermType::PERM_CDP        },

    { DEX_LIMIT_BUY_ORDER_TX,       AccountPermType::PERM_DEX        },
    { DEX_LIMIT_SELL_ORDER_TX,      AccountPermType::PERM_DEX        },
    { DEX_MARKET_BUY_ORDER_TX,      AccountPermType::PERM_DEX        },
    { DEX_MARKET_SELL_ORDER_TX,     AccountPermType::PERM_DEX        },
    { DEX_CANCEL_ORDER_TX,          AccountPermType::PERM_DEX        },
    { DEX_ORDER_TX,                 AccountPermType::PERM_DEX        },
    { DEX_OPERATOR_ORDER_TX,        AccountPermType::PERM_DEX        },
    { DEX_OPERATOR_UPDATE_TX,       AccountPermType::PERM_DEX        },
    { DEX_OPERATOR_REGISTER_TX,     AccountPermType::PERM_DEX        },
    { DEX_OPERATOR_REGISTER_TX,     AccountPermType::PERM_DEX        },
    { DEX_TRADE_SETTLE_TX,          AccountPermType::PERM_DEX        },

    { PROPOSAL_REQUEST_TX,          AccountPermType::PERM_PROPOSE    },
};

string GetTxType(const TxType txType);
bool GetTxMinFee(CCacheWrapper &cw, const TxType nTxType, int height, const TokenSymbol &symbol, uint64_t &feeOut);

inline const string& GetTxTypeName(TxType txType) {
    auto it = kTxFeeTable.find(txType);
    if (it != kTxFeeTable.end())
        return std::get<0>(it->second);
    return EMPTY_STRING;
}

class CTxExecuteContext {
public:
    int32_t                       height;
    int32_t                       index;
    uint32_t                      fuel_rate;
    uint32_t                      block_time;
    uint32_t                      prev_block_time;
    CCacheWrapper*                pCw;
    CValidationState*             pState;
    TxExecuteContextType          context_type;

    CTxExecuteContext()
        : height(0),
          index(0),
          fuel_rate(0),
          block_time(0),
          prev_block_time(0),
          pCw(nullptr),
          pState(nullptr),
          context_type(TxExecuteContextType::CONNECT_BLOCK){}

    CTxExecuteContext(const int32_t heightIn, const int32_t indexIn, const uint32_t fuelRateIn,
                      const uint32_t blockTimeIn, const uint32_t preBlockTimeIn,
                      CCacheWrapper *pCwIn, CValidationState *pStateIn,
                      const TxExecuteContextType contextType = TxExecuteContextType::CONNECT_BLOCK)
        : height(heightIn),
          index(indexIn),
          fuel_rate(fuelRateIn),
          block_time(blockTimeIn),
          prev_block_time(preBlockTimeIn),
          pCw(pCwIn),
          pState(pStateIn),
          context_type(contextType) {}
};

class CBaseTx {
public:
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
    CAccount txAccount;    //!< only in memory
    ReceiptList receipts;  //!< not persisted within Tx Cache

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

    virtual std::pair<TokenSymbol, uint64_t> GetFees() const { return std::make_pair(fee_symbol, llFees); }

    virtual TxID GetHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter hashWriter(SER_GETHASH, 0);
            SerializeForHash(hashWriter);
            sigHash = hashWriter.GetHash();
        }
        return sigHash;
    }

    virtual uint32_t GetSerializeSize(int32_t nType, int32_t nVersion) const { return 0; }

    virtual uint64_t GetFuelFee(int32_t height, uint32_t nFuelRate);
    virtual double GetPriority() const {
        return TRANSACTION_PRIORITY_CEILING / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION);
    }
    virtual void SerializeForHash(CHashWriter &hw) const = 0;
    virtual std::shared_ptr<CBaseTx> GetNewInstance() const           = 0;
    virtual string ToString(CAccountDBCache &accountCache)            = 0;
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    bool CheckBaseTx(CTxExecuteContext &context);
    virtual bool CheckTx(CTxExecuteContext &context) = 0;
    virtual bool ExecuteTx(CTxExecuteContext &context) = 0;
    bool ExecuteFullTx(CTxExecuteContext &context);

    inline bool CheckAndExecuteTx(CTxExecuteContext& context) {
        return CheckBaseTx(context) && CheckTx(context) && ExecuteFullTx(context);
    }

    bool IsValidHeight(int32_t nCurHeight, int32_t nTxCacheHeight) const;

    // If the sender has no regid before, generate a regid for the sender.
    bool RegisterAccountPubKey(CTxExecuteContext &context);

    bool IsBlockRewardTx()  { return nTxType == BLOCK_REWARD_TX || nTxType == UCOIN_BLOCK_REWARD_TX; }
    bool IsPriceMedianTx()  { return nTxType == PRICE_MEDIAN_TX; }
    bool IsPriceFeedTx()    { return nTxType == PRICE_FEED_TX; }
    bool IsCoinMintTx()     { return nTxType == UCOIN_MINT_TX; }
    bool IsRelayForbidden() { return kForbidRelayTxSet.count(nTxType) > 0; }

    const string& GetTxTypeName() const { return ::GetTxTypeName(nTxType); }


public:
    static unsigned int GetSerializePtrSize(const std::shared_ptr<CBaseTx> &pBaseTx, int nType, int nVersion){
        return pBaseTx->GetSerializeSize(nType, nVersion) + 1;
    }

    template<typename Stream>
    static void SerializePtr(Stream& os, const std::shared_ptr<CBaseTx> &pBaseTx, int nType, int nVersion);

    template<typename Stream>
    static void UnserializePtr(Stream& is, std::shared_ptr<CBaseTx> &pBaseTx, int nType, int nVersion);
    bool GetTxAccount(CTxExecuteContext &context, CAccount &account);
    bool CheckFee(CTxExecuteContext &context, function<bool(CTxExecuteContext&, uint64_t)> = nullptr) const;
    bool CheckMinFee(CTxExecuteContext &context, uint64_t minFee) const;

    bool CheckTxAvailableFromVer(CTxExecuteContext &context, FeatureForkVersionEnum ver);

    bool VerifySignature(CTxExecuteContext &context, const CPubKey &pubkey);
protected:
    bool CheckTxFeeSufficient(CCacheWrapper &cw, const TokenSymbol &feeSymbol,
                              const uint64_t llFees, const int32_t height) const;
    bool CheckSignatureSize(const vector<unsigned char> &signature) const;


    static bool AddInvolvedKeyIds(vector<CUserID> uids, CCacheWrapper &cw, set<CKeyID> &keyIds);
};

/**################################ Universal Coin Transfer ########################################**/

struct SingleTransfer {
    CUserID to_uid;
    TokenSymbol coin_symbol = SYMB::WICC;
    uint64_t coin_amount    = 0;

    SingleTransfer() {}

    SingleTransfer(const CUserID &toUidIn, const TokenSymbol &coinSymbol, const uint64_t coinAmount)
        : to_uid(toUidIn), coin_symbol(coinSymbol), coin_amount(coinAmount) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(to_uid);
        READWRITE(coin_symbol);
        READWRITE(VARINT(coin_amount));
    )
    string ToString(const CAccountDBCache &accountCache) const;

    Object ToJson(const CAccountDBCache &accountCache) const;
};

class CSignaturePair {
public:
    CRegID regid;                 //!< regid only
    UnsignedCharArray signature;  //!< signature

    IMPLEMENT_SERIALIZE(
        READWRITE(regid);
        READWRITE(signature);)

public:
    CSignaturePair() {}

    CSignaturePair(const CRegID &regidIn, const UnsignedCharArray &signatureIn)
        : regid(regidIn), signature(signatureIn) {}

    CSignaturePair(const CRegID &regidIn): regid(regidIn) {}

    string ToString() const;
    Object ToJson() const;
};

#define TX_ERR_TITLE (GetTxTypeName())
#define TX_OBJ_ERR_TITLE(tx) (tx.GetTxTypeName())

#define IMPLEMENT_DEFINE_CW_STATE                                                                  \
    CCacheWrapper &cw       = *context.pCw;                                                        \
    CValidationState &state = *context.pState;

#define IMPLEMENT_CHECK_TX_MEMO                                                                    \
    if (memo.size() > MAX_COMMON_TX_MEMO_SIZE)                                                     \
        return state.DoS(100, ERRORMSG("memo's size too large"), REJECT_INVALID,                   \
                         "memo-size-toolarge");

#define IMPLEMENT_CHECK_TX_ARGUMENTS                                                               \
    if (arguments.size() > MAX_CONTRACT_ARGUMENT_SIZE)                                             \
        return state.DoS(100, ERRORMSG("arguments's size too large"), REJECT_INVALID,              \
                         "arguments-size-toolarge");

#define IMPLEMENT_CHECK_TX_REGID(txUid)                                                            \
    if (!txUid.is<CRegID>()) {                                                                     \
        return state.DoS(100, ERRORMSG("txUid must be CRegID"), REJECT_INVALID,                    \
                         "txUid-type-error");                                                      \
    }

#define IMPLEMENT_CHECK_TX_APPID(appUid)                                                           \
    if (!appUid.is<CRegID>()) {                                                                    \
        return state.DoS(100, ERRORMSG("appUid must be CRegID"), REJECT_INVALID,                   \
                         "appUid-type-error");                                                     \
    }

#define IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(uid)                                                    \
    if (GetFeatureForkVersion(context.height) == MAJOR_VER_R1 && !uid.is<CRegID>()) {              \
        return state.DoS(100,                                                                      \
                         ERRORMSG("txUid must be CRegID pre-stable coin release! tx=%s",           \
                                  __FUNCTION__, GetTxTypeName()),                                  \
                         REJECT_INVALID, "txUid-type-error");                                      \
    }                                                                                              \
    if ((!uid.is<CRegID>()) && (!uid.is<CPubKey>())) {                                             \
        return state.DoS(100, ERRORMSG("txUid must be CRegID or CPubKey! tx=%s", GetTxTypeName()), \
                         REJECT_INVALID, "txUid-type-error");                                      \
    }

#define IMPLEMENT_CHECK_TX_CANDIDATE_REGID_OR_PUBKEY(candidateUid)                                 \
    if ((!candidateUid.is<CRegID>()) && (!candidateUid.is<CPubKey>())) {                           \
        return state.DoS(100, ERRORMSG("candidateUid must be CRegID or CPubKey"), REJECT_INVALID,  \
                         "candidateUid-type-error");                                               \
    }

#define IMPLEMENT_CHECK_TX_REGID_OR_KEYID(toUid)                                                   \
    if ((!toUid.is<CRegID>()) && (!toUid.is<CKeyID>())) {                                          \
        return state.DoS(100, ERRORMSG("toUid must be CRegID or CKeyID"), REJECT_INVALID,          \
                         "toUid-type-error");                                                      \
    }

#endif //COIN_BASETX_H
