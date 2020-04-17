// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "commons/base58.h"
#include "config/const.h"
#include "rpc/core/rpccommons.h"
#include "rpc/rpcapi.h"
#include "init.h"
#include "net.h"
#include "commons/util/util.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#include "tx/dextx.h"
#include "tx/dexoperatortx.h"

using namespace dex;

// TODO: move to json.h
namespace json {

#define DEFINE_SINGLE_TO_JSON(type) \
    inline Value ToJson(const type &val) { return Value(val); }


    DEFINE_SINGLE_TO_JSON(int32_t)
    DEFINE_SINGLE_TO_JSON(uint8_t)
    DEFINE_SINGLE_TO_JSON(uint16_t)
    DEFINE_SINGLE_TO_JSON(uint32_t)
    DEFINE_SINGLE_TO_JSON(uint64_t)
    DEFINE_SINGLE_TO_JSON(string)
    DEFINE_SINGLE_TO_JSON(bool)

    // vector
    template<typename T, typename A> Value ToJson(const vector<T, A>& val);

    //optional
    template<typename T> Value ToJson(const std::optional<T>& val);

    // set
    template<typename K, typename Pred, typename A> Value ToJson(const set<K, Pred, A>& val);

    // map
    template<typename K, typename T, typename Pred, typename A> Value ToJson(const map<K, T, Pred, A>& val);

    // CVarIntValue
    template<typename I> Value ToJson(const CVarIntValue<I>& val);

    // common Object Type, must support T.IsEmpty() and T.SetEmpty()
    template<typename T> Value ToJson(const T& val);

    //optional
    template<typename T> Value ToJson(const std::optional<T>& val) { return val ? ToJson(val.value()) : Value(); }

    // vector
    template<typename T, typename A> Value ToJson(const vector<T, A>& val) {
        Array arr;
        for (const auto &item : val) {
            arr.push_back(ToJson(item));
        }
        return arr;
    }

    // set
    template<typename K, typename Pred, typename A> Value ToJson(const set<K, Pred, A>& val) {
        Array arr;
        for (const auto &item : val) {
            arr.push_back(ToJson(item));
        }
        return arr;
    }

    // CVarIntValue
    template<typename I> Value ToJson(const CVarIntValue<I>& val) {
        return ToJson(val.get());
    }

    // common Object Type, must support T.IsEmpty() and T.SetEmpty()
    template<typename T> Value ToJson(const T& val) {
        return val.ToJson();
    }

}

static Object DexOperatorToJson(const CAccountDBCache &accountCache, const DexOperatorDetail &dexOperator) {
    Object result;
    CKeyID ownerKeyid;
    accountCache.GetKeyId(dexOperator.owner_regid, ownerKeyid);
    CKeyID feeReceiverKeyId;
    accountCache.GetKeyId(dexOperator.fee_receiver_regid, feeReceiverKeyId);
    result.push_back(Pair("owner_regid",   dexOperator.owner_regid.ToString()));
    result.push_back(Pair("owner_addr",     ownerKeyid.ToAddress()));
    result.push_back(Pair("fee_receiver_regid", dexOperator.fee_receiver_regid.ToString()));
    result.push_back(Pair("fee_receiver_addr",   feeReceiverKeyId.ToAddress()));
    result.push_back(Pair("name",           dexOperator.name));
    result.push_back(Pair("portal_url",     dexOperator.portal_url));
    result.push_back(Pair("order_open_mode", kOpenModeHelper.GetName(dexOperator.order_open_mode)));
    result.push_back(Pair("maker_fee_ratio", dexOperator.maker_fee_ratio));
    result.push_back(Pair("taker_fee_ratio", dexOperator.taker_fee_ratio));
    result.push_back(Pair("order_open_dexop_list", json::ToJson(dexOperator.order_open_dexop_set)));
    result.push_back(Pair("memo",           dexOperator.memo));
    result.push_back(Pair("memo_hex",       HexStr(dexOperator.memo)));
    result.push_back(Pair("activated",       dexOperator.activated));
    return result;
}

namespace RPC_PARAM {

    OrderType GetOrderType(const Value &jsonValue) {
        OrderType ret = OrderType::ORDER_TYPE_NULL;
        if (!kOrderTypeHelper.Parse(jsonValue.get_str(), ret))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("order_type=%s is invalid",
                jsonValue.get_str()));
        return ret;
    }

    OrderSide GetOrderSide(const Value &jsonValue) {
        OrderSide ret = OrderSide::ORDER_SIDE_NULL;
        if (!kOrderSideHelper.Parse(jsonValue.get_str(), ret))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("order_side=%s is invalid",
                jsonValue.get_str()));
        return ret;
    }

    DexID GetDexId(const Value &jsonValue) {
        return RPC_PARAM::GetUint32(jsonValue);
    }

    DexID GetDexId(const Array& params, const size_t index) {
        return params.size() > index? GetDexId(params[index]) : DEX_RESERVED_ID;
    }

    uint64_t GetOperatorFeeRatio(const Value &jsonValue) {

        uint64_t ratio = RPC_PARAM::GetUint64(jsonValue);
        if (ratio > DEX_OPERATOR_FEE_RATIO_MAX)
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("match fee ratio=%llu is large than %llu",
                ratio, DEX_OPERATOR_FEE_RATIO_MAX));
        return ratio;
    }


    uint64_t GetOperatorFeeRatio(const Array& params, const size_t index) {
        return params.size() > index ? GetOperatorFeeRatio(params[index]) : 0;
    }

    string GetMemo(const Array& params, const size_t index) {
        if (params.size() > index) {
            string memo = params[index].get_str();
            if (memo.size() > MAX_COMMON_TX_MEMO_SIZE)
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("memo.size=%u is large than %llu",
                    memo.size(), MAX_COMMON_TX_MEMO_SIZE));
            return memo;
        }
        return "";
    }

    OpenMode GetOrderOpenMode(const Value &jsonValue) {
        OpenMode ret = OpenMode::PRIVATE;
        if (!kOpenModeHelper.Parse(jsonValue.get_str(), ret))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("order_open_mode=%s is invalid",
                jsonValue.get_str()));
        return ret;
    }

    OpenMode GetOrderOpenMode(const Array &params, const size_t index) {

        return params.size() > index ? GetOrderOpenMode(params[index].get_str()) : OpenMode::PUBLIC;
    }

    DexOperatorDetail GetDexOperator(const DexID &dexId) {
        DexOperatorDetail operatorDetail;
        if (!pCdMan->pDexCache->GetDexOperator(dexId, operatorDetail))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("the dex operator does not exist! dex_id=%u",
                dexId));
        return operatorDetail;
    }

    void CheckOrderAmount(const TokenSymbol &symbol, const uint64_t amount, const char *pSymbolSide) {

        if (amount < (int64_t)MIN_DEX_ORDER_AMOUNT)
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                strprintf("%s amount is too small, symbol=%s, amount=%llu, min_amount=%llu",
                          pSymbolSide, symbol, amount, MIN_DEX_ORDER_AMOUNT));

        CheckTokenAmount(symbol, amount);
    }

    uint64_t CalcCoinAmount(uint64_t assetAmount, const uint64_t price) {
        uint128_t coinAmount = assetAmount * (uint128_t)price / PRICE_BOOST;
        if (coinAmount > ULLONG_MAX)
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                strprintf("the calculated coin amount out of range, asset_amount=%llu, price=%llu",
                          assetAmount, price));
        return (uint64_t)coinAmount;
    }

    ComboMoney GetOperatorTxFee(const Array& params, const size_t index) {
        ComboMoney fee;
        if (params.size() > index) {
            return GetComboMoney(params[index], SYMB::WICC);
        } else {
            return {SYMB::WICC, 0, COIN_UNIT::SAWI};
        }
    }

    DexOpIdValueList GetOrderOpenDexopList(const Value& jsonValue) {
        DexOpIdValueList ret;
        const Array &arr = jsonValue.get_array();
        for (auto &item : arr) {
            ret.push_back(CVarIntValue(item.get_uint64()));
        }
        return ret;
    }


    void CheckOrderFee(const CAccount &txAccount, uint64_t txFee, uint64_t minFee,
        CAccount *pOperatorAccount = nullptr, uint64_t operatorTxFee = 0) {

        HeightType height = chainActive.Height() + 1;
        auto spErr = dex::CheckOrderMinFee(height, txAccount, txFee, minFee, pOperatorAccount,
                                           operatorTxFee);
        if (spErr) {
            throw JSONRPCError(RPC_INVALID_PARAMS, *spErr);
        }
    }

} // namespace RPC_PARAM

Object SubmitOrderTx(const CKeyID &txKeyid, const DexOperatorDetail &operatorDetail,
    shared_ptr<CDEXOrderBaseTx> &pOrderBaseTx) {

    if (!pWalletMain->HasKey(txKeyid)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "tx user address not found in wallet");
    }
    const uint256 txHash = pOrderBaseTx->GetHash();
    if (!pWalletMain->Sign(txKeyid, txHash, pOrderBaseTx->signature)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
    }

    shared_ptr<CBaseTx> pBaseTx = static_pointer_cast<CBaseTx>(pOrderBaseTx);
    if (pOrderBaseTx->has_operator_config) {
        CAccount operatorAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, operatorDetail.fee_receiver_regid);
        const CKeyID &operatorKeyid = operatorAccount.keyid;
        if (!pWalletMain->HasKey(operatorKeyid)) {
            CDataStream ds(SER_DISK, CLIENT_VERSION);
            ds << pBaseTx;
            throw JSONRPCError(RPC_WALLET_ERROR, strprintf("dex operator address not found in wallet! "
                "tx_raw_with_sign=%s", HexStr(ds.begin(), ds.end())));
        }
        if (!pWalletMain->Sign(operatorKeyid, txHash, pOrderBaseTx->operator_signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }
    }

    string retMsg;
    if (!pWalletMain->CommitTx(pBaseTx.get(), retMsg))
        throw JSONRPCError(RPC_WALLET_ERROR, strprintf("SubmitTx failed: txid=%s, %s", pBaseTx->GetHash().GetHex(), retMsg));

    Object obj;
    obj.push_back( Pair("txid", retMsg) );

    return obj;
}

static void CheckDexId0BeforeV3(uint64_t dexId, int32_t height) {
    if (dexId != 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("dex_id must be 0 before v3! tip_height=%d, v3_height=%d",
            height, SysCfg().GetVer3ForkHeight()));
}

static void CheckMemoEmptyBeforeV3(const string &memo, int32_t height) {
    if (!memo.empty())
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("memo must be empty before v3! tip_height=%d, v3_height=%d",
            height, SysCfg().GetVer3ForkHeight()));
}

/*************************************************<< DEX >>**************************************************/
Value submitdexbuylimitordertx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 4 || params.size() > 8) {
        throw runtime_error(
            "submitdexbuylimitordertx \"addr\" \"coin_symbol\" \"symbol:asset_amount:unit\"  price"
                " [dex_id] [symbol:fee:unit] \"[memo]\"\n"
            "\nsubmit a dex buy limit price order tx.\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) order owner address\n"
            "2.\"coin_symbol\": (string required) coin type to pay\n"
            "3.\"symbol:asset_amount:unit\",(string:numeric:string,required) the target amount to buy \n "
            "   default symbol is WICC, default unit is sawi.\n"
            "4.\"price\": (numeric, required) bidding price willing to buy\n"
            "5.\"dex_id\": (numeric, optional) Decentralized Exchange(DEX) ID, default is 0\n"
            "6.\"symbol:fee:unit\":(string:numeric:string, optional) fee paid for miner, default is WICC:10000:sawi\n"
            "7.\"memo\": (string, optional) memo\n"
            "\nResult:\n"
            "\"txid\" (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitdexbuylimitordertx", "\"10-3\" \"WUSD\" \"WICC:1000000000:sawi\""
                             " 100000000 1\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitdexbuylimitordertx", "\"10-3\", \"WUSD\", \"WICC:1000000000:sawi\","
                             " 100000000, 1\n")
        );
    }

    EnsureWalletIsUnlocked();
    int32_t validHeight = chainActive.Height();
    FeatureForkVersionEnum version = GetFeatureForkVersion(validHeight);
    const TxType txType = version  < MAJOR_VER_R3 ? DEX_LIMIT_BUY_ORDER_TX : DEX_ORDER_TX;

    const CUserID& userId          = RPC_PARAM::GetUserId(params[0], true);
    const TokenSymbol& coinSymbol  = RPC_PARAM::GetOrderCoinSymbol(params[1]);
    ComboMoney assetInfo           = RPC_PARAM::GetComboMoney(params[2], SYMB::WICC);
    uint64_t price                 = RPC_PARAM::GetPrice(params[3]);
    DexID dexId                    = RPC_PARAM::GetDexId(params, 4);
    ComboMoney cmFee               = RPC_PARAM::GetFee(params, 5, txType);
    string memo                    = RPC_PARAM::GetMemo(params, 6);

    RPC_PARAM::CheckOrderSymbols(__func__, coinSymbol, assetInfo.symbol);
    // Get account for checking balance
    CAccount txAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, userId);
    RPC_PARAM::CheckAccountBalance(txAccount, cmFee.symbol, SUB_FREE, cmFee.GetAmountInSawi());
    uint64_t coinAmount = CDEXOrderBaseTx::CalcCoinAmount(assetInfo.GetAmountInSawi(), price);
    RPC_PARAM::CheckAccountBalance(txAccount, coinSymbol, FREEZE, coinAmount);

    DexOperatorDetail operatorDetail = RPC_PARAM::GetDexOperator(dexId);

    shared_ptr<CDEXOrderBaseTx> pOrderBaseTx;
    if (version < MAJOR_VER_R3) {
        CheckDexId0BeforeV3(dexId, validHeight);
        CheckMemoEmptyBeforeV3(memo, validHeight);
        pOrderBaseTx = make_shared<CDEXBuyLimitOrderTx>(
            userId, validHeight, cmFee.symbol, cmFee.GetAmountInSawi(), coinSymbol, assetInfo.symbol,
            assetInfo.GetAmountInSawi(), price);
    } else {
        pOrderBaseTx =
            make_shared<CDEXOrderTx>(userId, validHeight, cmFee.symbol, cmFee.GetAmountInSawi(),
                                     ORDER_LIMIT_PRICE, ORDER_BUY, coinSymbol, assetInfo.symbol,
                                     0, assetInfo.GetAmountInSawi(), price, dexId, memo);
    }
    return SubmitOrderTx(txAccount.keyid, operatorDetail, pOrderBaseTx);
}

Value submitdexselllimitordertx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 4 || params.size() > 8) {
        throw runtime_error(
            "submitdexselllimitordertx \"addr\" \"coin_symbol\" \"asset\" price"
                " [dex_id] [symbol:fee:unit] \"[memo]\"\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) order owner address\n"
            "2.\"coin_symbol\": (string required) coin type to pay\n"
            "3.\"asset_symbol:asset_amount:unit\",(comboMoney,required) the target amount to sell. "
            "   default symbol is WICC, default unit is sawi.\n"
            "4.\"price\": (numeric, required) bidding price willing to buy\n"
            "5.\"dex_id\": (numeric, optional) Decentralized Exchange(DEX) ID, default is 0\n"
            "6.\"symbol:fee:unit\":(string:numeric:string, optional) fee paid for miner, default is WICC:10000:sawi\n"
            "7.\"memo\": (string, optional) memo\n"
            "\nResult:\n"
            "\"txid\" (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitdexselllimitordertx", "\"10-3\" \"WUSD\" \"WICC:1000000000:sawi\""
                             " 100000000 1\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitdexselllimitordertx", "\"10-3\", \"WUSD\", \"WICC:1000000000:sawi\","
                             " 100000000, 1\n")
        );
    }

    EnsureWalletIsUnlocked();
    int32_t validHeight = chainActive.Height();
    FeatureForkVersionEnum version = GetFeatureForkVersion(validHeight);
    const TxType txType = version  < MAJOR_VER_R3 ? DEX_LIMIT_SELL_ORDER_TX : DEX_ORDER_TX;

    const CUserID& userId          = RPC_PARAM::GetUserId(params[0], true);
    const TokenSymbol& coinSymbol  = RPC_PARAM::GetOrderCoinSymbol(params[1]);
    ComboMoney assetInfo           = RPC_PARAM::GetComboMoney(params[2], SYMB::WICC);
    uint64_t price                 = RPC_PARAM::GetPrice(params[3]);
    DexID dexId                    = RPC_PARAM::GetDexId(params, 4);
    ComboMoney cmFee               = RPC_PARAM::GetFee(params, 5, txType);
    string memo                    = RPC_PARAM::GetMemo(params, 6);

    RPC_PARAM::CheckOrderSymbols(__FUNCTION__, coinSymbol, assetInfo.symbol);
    // Get account for checking balance
    CAccount txAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, userId);
    RPC_PARAM::CheckAccountBalance(txAccount, cmFee.symbol, SUB_FREE, cmFee.GetAmountInSawi());
    RPC_PARAM::CheckAccountBalance(txAccount, assetInfo.symbol, FREEZE, assetInfo.GetAmountInSawi());

    DexOperatorDetail operatorDetail = RPC_PARAM::GetDexOperator(dexId);

    shared_ptr<CDEXOrderBaseTx> pOrderBaseTx;
    if (version < MAJOR_VER_R3) {
        CheckDexId0BeforeV3(dexId, validHeight);
        CheckMemoEmptyBeforeV3(memo, validHeight);
        pOrderBaseTx = make_shared<CDEXSellLimitOrderTx>(
            userId, validHeight, cmFee.symbol, cmFee.GetAmountInSawi(), coinSymbol, assetInfo.symbol,
            assetInfo.GetAmountInSawi(), price);
    } else {
        pOrderBaseTx =
            make_shared<CDEXOrderTx>(userId, validHeight, cmFee.symbol, cmFee.GetAmountInSawi(),
                                     ORDER_LIMIT_PRICE, ORDER_SELL, coinSymbol, assetInfo.symbol,
                                     0, assetInfo.GetAmountInSawi(), price, dexId, memo);
    }

    return SubmitOrderTx(txAccount.keyid, operatorDetail, pOrderBaseTx);
}

Value submitdexbuymarketordertx(const Array& params, bool fHelp) {
     if (fHelp || params.size() < 3 || params.size() > 7) {
        throw runtime_error(
            "submitdexbuymarketordertx \"addr\" \"coin_symbol\" coin_amount \"asset_symbol\" "
                " [dex_id] [symbol:fee:unit] \"[memo]\"\n"
            "\nsubmit a dex buy market price order tx.\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) order owner address\n"
            "2.\"coin_symbol:coin_amount:unit\",(comboMoney,required) the target coin amount for buying asset \n "
            "   default symbol is WUSD, default unit is sawi.\n"
            "3.\"asset_symbol\": (string required), asset type to buy\n"
            "4.\"dex_id\": (numeric, optional) Decentralized Exchange(DEX) ID, default is 0\n"
            "5.\"symbol:fee:unit\":(string:numeric:string, optional) fee paid for miner, default is WICC:10000:sawi\n"
            "6.\"memo\": (string, optional) memo\n"
            "\nResult:\n"
            "\"txid\" (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitdexbuymarketordertx", "\"10-3\" \"WUSD:200000000:sawi\"  \"WICC\""
                             " 1\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitdexbuymarketordertx", "\"10-3\", \"WUSD:200000000:sawi\", \"WICC\","
                             " 1\n")
        );
    }

    EnsureWalletIsUnlocked();
    int32_t validHeight = chainActive.Height();
    FeatureForkVersionEnum version = GetFeatureForkVersion(validHeight);
    const TxType txType = version  < MAJOR_VER_R3 ? DEX_MARKET_BUY_ORDER_TX : DEX_ORDER_TX;

    const CUserID& userId          = RPC_PARAM::GetUserId(params[0], true);
    ComboMoney coinInfo            = RPC_PARAM::GetComboMoney(params[1], SYMB::WUSD);
    const TokenSymbol& assetSymbol = RPC_PARAM::GetOrderAssetSymbol(params[2]);
    DexID dexId                    = RPC_PARAM::GetDexId(params, 3);
    ComboMoney cmFee               = RPC_PARAM::GetFee(params, 4, txType);
    string memo                    = RPC_PARAM::GetMemo(params, 5);

    RPC_PARAM::CheckOrderSymbols(__FUNCTION__, coinInfo.symbol, assetSymbol);
    // Get account for checking balance
    CAccount txAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, userId);
    RPC_PARAM::CheckAccountBalance(txAccount, cmFee.symbol, SUB_FREE, cmFee.GetAmountInSawi());
    RPC_PARAM::CheckAccountBalance(txAccount, coinInfo.symbol, FREEZE, coinInfo.GetAmountInSawi());

    DexOperatorDetail operatorDetail = RPC_PARAM::GetDexOperator(dexId);

    shared_ptr<CDEXOrderBaseTx> pOrderBaseTx;
    if (version < MAJOR_VER_R3) {
        CheckDexId0BeforeV3(dexId, validHeight);
        CheckMemoEmptyBeforeV3(memo, validHeight);
        pOrderBaseTx = make_shared<CDEXBuyMarketOrderTx>(userId, validHeight, cmFee.symbol,
                                                         cmFee.GetAmountInSawi(), coinInfo.symbol,
                                                         assetSymbol, coinInfo.GetAmountInSawi());
    } else {
        pOrderBaseTx = make_shared<CDEXOrderTx>(
            userId, validHeight, cmFee.symbol, cmFee.GetAmountInSawi(), ORDER_MARKET_PRICE, ORDER_BUY,
            coinInfo.symbol, assetSymbol, coinInfo.GetAmountInSawi(), 0, 0, dexId, memo);
    }

    return SubmitOrderTx(txAccount.keyid, operatorDetail, pOrderBaseTx);
}

Value submitdexsellmarketordertx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 3 || params.size() > 7) {
        throw runtime_error(
            "submitdexsellmarketordertx \"addr\" \"coin_symbol\" \"asset_symbol\" asset_amount "
                " [dex_id] [symbol:fee:unit] \"[memo]\"\n"
            "\nsubmit a dex sell market price order tx.\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) order owner address\n"
            "2.\"coin_symbol\": (string required) coin type to pay\n"
            "3.\"asset_symbol:asset_amount:unit\",(comboMoney,required) the target amount to sell, "
                                                  "default symbol is WICC, default unit is sawi.\n"
            "4.\"dex_id\": (numeric, optional) Decentralized Exchange(DEX) ID, default is 0\n"
            "5.\"symbol:fee:unit\":(string:numeric:string, optional) fee paid for miner, default is WICC:10000:sawi\n"
            "6.\"memo\": (string, optional) memo\n"
            "\nResult:\n"
            "\"txid\" (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitdexsellmarketordertx", "\"10-3\" \"WUSD\" \"WICC:200000000:sawi\""
                             " 1\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitdexsellmarketordertx", "\"10-3\", \"WUSD\", \"WICC:200000000:sawi\","
                             " 1\n")
        );
    }

    EnsureWalletIsUnlocked();
    int32_t validHeight = chainActive.Height();
    FeatureForkVersionEnum version = GetFeatureForkVersion(validHeight);
    const TxType txType = version  < MAJOR_VER_R3 ? DEX_MARKET_SELL_ORDER_TX : DEX_ORDER_TX;

    const CUserID& userId          = RPC_PARAM::GetUserId(params[0], true);
    const TokenSymbol& coinSymbol  = RPC_PARAM::GetOrderCoinSymbol(params[1]);
    ComboMoney assetInfo           = RPC_PARAM::GetComboMoney(params[2], SYMB::WICC);
    DexID dexId                    = RPC_PARAM::GetDexId(params, 3);
    ComboMoney cmFee               = RPC_PARAM::GetFee(params, 4, txType);
    string memo                    = RPC_PARAM::GetMemo(params, 5);

    RPC_PARAM::CheckOrderSymbols(__FUNCTION__, coinSymbol, assetInfo.symbol);
    // Get account for checking balance
    CAccount txAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, userId);
    RPC_PARAM::CheckAccountBalance(txAccount, cmFee.symbol, SUB_FREE, cmFee.GetAmountInSawi());
    RPC_PARAM::CheckAccountBalance(txAccount, assetInfo.symbol, FREEZE, assetInfo.GetAmountInSawi());

    DexOperatorDetail operatorDetail = RPC_PARAM::GetDexOperator(dexId);

    shared_ptr<CDEXOrderBaseTx> pOrderBaseTx;
    if (version < MAJOR_VER_R3) {
        CheckDexId0BeforeV3(dexId, validHeight);
        CheckMemoEmptyBeforeV3(memo, validHeight);
        pOrderBaseTx = make_shared<CDEXSellMarketOrderTx>(
            userId, validHeight, cmFee.symbol, cmFee.GetAmountInSawi(), coinSymbol, assetInfo.symbol,
            assetInfo.GetAmountInSawi());
    } else {
        pOrderBaseTx =
            make_shared<CDEXOrderTx>(userId, validHeight, cmFee.symbol, cmFee.GetAmountInSawi(),
                                     ORDER_MARKET_PRICE, ORDER_SELL, coinSymbol, assetInfo.symbol,
                                     0, assetInfo.GetAmountInSawi(), 0, dexId, memo);
    }

    return SubmitOrderTx(txAccount.keyid, operatorDetail, pOrderBaseTx);
}

Value gendexoperatorordertx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 10 || params.size() > 13) {
        throw runtime_error(
            "gendexoperatorordertx \"addr\" \"order_type\" \"order_side\" \"symbol:coins:unit\" \"symbol:assets:unit\""
                " price dex_id \"open_mode\" taker_fee_ratio maker_fee_ratio \"[symbol:fee:unit]\""
                " \"[symbol:operator_tx_fee:unit]\" \"[memo]\"\n"
            "\ngenerator an operator dex order tx, support operator config, and must be signed by operator before sumiting.\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) order owner address\n"
            "2.\"order_type\": (string required) order type, must be in (LIMIT_PRICE, MARKET_PRICE)\n"
            "3.\"order_side\": (string required) order side, must be in (BUY, SELL)\n"
            "4.\"symbol:coins:unit\": (string:numeric:string, required) the coins(money) of order, coins=0 if not market buy order, \n"
                                                  "default symbol is WUSD, default unit is sawi.\n"
            "5.\"symbol:assets:unit\",(string:numeric:string, required) the assets of order, assets=0 if market buy order"
                                                  "default symbol is WICC, default unit is sawi.\n"
            "6.\"price\": (numeric, required) expected price of order, boost 100000000\n"
            "7.\"dex_id\": (numeric, required) Decentralized Exchange(DEX) ID\n"
            "8.\"open_mode\": (string, required) indicate the order is PUBLIC or PRIVATE\n"
            "9.\"taker_fee_ratio\": (numeric, required) taker fee ratio config by operator, boost 100000000\n"
            "10.\"maker_fee_ratio\": (numeric, required) maker fee ratio config by operator, boost 100000000\n"
            "11.\"symbol:fee:unit\":(string:numeric:string, optional) fee pay by tx user to miner, default is WICC:10000:sawi\n"
            "12.\"symbol:operator_tx_fee:unit\":(string:numeric:string, optional) tx fee pay by operator to miner,"
                                            "symbol must equal to fee, default is WICC:0:sawi\n"
            "13.\"memo\": (string, optional) memo\n"
            "\nResult:\n"
            "\"txid\" (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("gendexoperatorordertx", "\"10-3\" \"LIMIT_PRICE\" \"BUY\" \"WUSD:0\" \"WICC:2000000000:sawi\""
                             " 100000000 0 \"PUBLIC\" 80000 40000\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("gendexoperatorordertx", "\"10-3\", \"LIMIT_PRICE\", \"BUY\", \"WUSD:0\", \"WICC:2000000000:sawi\","
                             " 100000000, 0, \"PUBLIC\", 80000, 40000\n")
        );
    }

    EnsureWalletIsUnlocked();
    int32_t validHeight = chainActive.Height();
    FeatureForkVersionEnum version = GetFeatureForkVersion(validHeight);
    const TxType txType = DEX_OPERATOR_ORDER_TX;

    if (version < MAJOR_VER_R3) {
        throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("unsupport to call %s() before R3 fork height=%d",
            __func__, SysCfg().GetVer3ForkHeight()));
    }

    const CUserID &userId               = RPC_PARAM::GetUserId(params[0], true);
    OrderType orderType                 = RPC_PARAM::GetOrderType(params[1]);
    OrderSide orderSide                 = RPC_PARAM::GetOrderSide(params[2]);
    const ComboMoney &coins             = RPC_PARAM::GetComboMoney(params[3]);
    const ComboMoney &assets            = RPC_PARAM::GetComboMoney(params[4], SYMB::WICC);
    uint64_t price                      = RPC_PARAM::GetPrice(params[5]);
    DexID dexId                         = RPC_PARAM::GetDexId(params[6]);
    OpenMode openMode                   = RPC_PARAM::GetOrderOpenMode(params[7]);
    uint64_t takerFeeRatio              = RPC_PARAM::GetOperatorFeeRatio(params[8]);
    uint64_t makerFeeRatio              = RPC_PARAM::GetOperatorFeeRatio(params[9]);
    ComboMoney fee;
    uint64_t minFee;
    RPC_PARAM::ParseTxFee(params, 10, txType, fee, minFee);
    ComboMoney operatorTxFee = RPC_PARAM::GetOperatorTxFee(params, 11);
    string memo                         = RPC_PARAM::GetMemo(params, 12);

    RPC_PARAM::CheckOrderSymbols(__func__, coins.symbol, assets.symbol);

    static_assert(MIN_DEX_ORDER_AMOUNT < INT64_MAX, "minimum dex order amount out of range");
    if (orderType == ORDER_MARKET_PRICE && orderSide == ORDER_BUY) {
        if (assets.GetAmountInSawi() != 0)
            throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("asset amount=%llu must be 0 when "
                "order_type=%s, order_side=%s", assets.GetAmountInSawi(), kOrderTypeHelper.GetName(orderType),
                kOrderSideHelper.GetName(orderSide)));
        RPC_PARAM::CheckOrderAmount(coins.symbol, coins.GetAmountInSawi(), "coin");
    } else {
        if (coins.GetAmountInSawi() != 0)
            throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("coin amount=%llu must be 0 when "
                "order_type=%s, order_side=%s", coins.GetAmountInSawi(), kOrderTypeHelper.GetName(orderType),
                kOrderSideHelper.GetName(orderSide)));

        RPC_PARAM::CheckOrderAmount(assets.symbol, assets.GetAmountInSawi(), "asset");
    }

    if (orderType == ORDER_MARKET_PRICE) {
        if (price != 0)
            throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("price must be 0 when order_type=%s",
                    price, kOrderTypeHelper.GetName(orderType)));
    } else {
        // TODO: should check the price range??
        if (price <= 0)
            throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("price=%llu out of range, order_type=%s",
                    price, kOrderTypeHelper.GetName(orderType)));
    }

    DexOperatorDetail operatorDetail = RPC_PARAM::GetDexOperator(dexId);

    CAccount txAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, userId);
    CAccount operatorAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, operatorDetail.fee_receiver_regid);

    if (fee.symbol != operatorTxFee.symbol)
        throw JSONRPCError(RPC_INVALID_PARAMS,
            strprintf("operator_tx_fee_symbol=%s does not euqal to fee_symbol=%s",
                fee.symbol, operatorTxFee.symbol));

    RPC_PARAM::CheckOrderFee(txAccount, fee.GetAmountInSawi(), minFee, &operatorAccount, operatorTxFee.GetAmountInSawi());

    RPC_PARAM::CheckAccountBalance(txAccount, fee.symbol, SUB_FREE,
            fee.GetAmountInSawi() + operatorTxFee.GetAmountInSawi());

    if (orderSide == ORDER_BUY) {
        uint64_t coinAmount = coins.GetAmountInSawi();
        if (orderType == ORDER_LIMIT_PRICE && orderSide == ORDER_BUY)
            coinAmount = RPC_PARAM::CalcCoinAmount(assets.amount, price);
        RPC_PARAM::CheckAccountBalance(txAccount, coins.symbol, FREEZE, coinAmount);
    } else {
        assert(orderSide == ORDER_SELL);
        RPC_PARAM::CheckAccountBalance(txAccount, assets.symbol, FREEZE, assets.GetAmountInSawi());
    }

    shared_ptr<CDEXOrderBaseTx> pOrderBaseTx = make_shared<CDEXOperatorOrderTx>(
        userId, validHeight, fee.symbol, fee.GetAmountInSawi(), orderType, orderSide, coins.symbol,
        assets.symbol, coins.GetAmountInSawi(), assets.GetAmountInSawi(), price, dexId, openMode,
        makerFeeRatio, takerFeeRatio, operatorDetail.fee_receiver_regid,
        operatorTxFee.GetAmountInSawi(), memo);

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << pOrderBaseTx;

    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
    return obj;
}

Value submitdexcancelordertx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 2 || params.size() > 3) {
        throw runtime_error(
            "submitdexcancelordertx \"addr\" \"txid\" [symbol:fee:unit]\n"
            "\nsubmit a dex cancel order tx.\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) order owner address\n"
            "2.\"txid\": (string required) order tx want to cancel\n"
            "3.\"symbol:fee:unit\":(string:numeric:string, optional) fee paid for miner, default is WICC:10000:sawi\n"
            "\nResult:\n"
            "\"txid\" (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitdexcancelordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" "
                             "\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\" ")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitdexcancelordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\", "\
                             "\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\"")
        );
    }

    EnsureWalletIsUnlocked();

    const CUserID& userId = RPC_PARAM::GetUserId(params[0], true);
    const uint256& txid   = RPC_PARAM::GetTxid(params[1], "txid");
    ComboMoney cmFee      = RPC_PARAM::GetFee(params, 2, DEX_CANCEL_ORDER_TX);

    // Get account for checking balance
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, userId);
    RPC_PARAM::CheckAccountBalance(account, cmFee.symbol, SUB_FREE, cmFee.GetAmountInSawi());

    // check active order tx
    RPC_PARAM::CheckActiveOrderExisted(*pCdMan->pDexCache, txid);

    int32_t validHeight = chainActive.Height();
    CDEXCancelOrderTx tx(userId, validHeight, cmFee.symbol, cmFee.GetAmountInSawi(), txid);
    return SubmitTx(account.keyid, tx);
}

Value submitdexsettletx(const Array& params, bool fHelp) {
     if (fHelp || params.size() < 2 || params.size() > 3) {
        throw runtime_error(
            "submitdexsettletx \"addr\" \"deal_items\" [symbol:fee:unit]\n"
            "\nsubmit a dex settle tx.\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) settle owner address\n"
            "2.\"deal_items\": (string required) deal items in json format\n"
            " [\n"
            "   {\n"
            "      \"buy_order_id\":\"txid\", (string, required) order txid of buyer\n"
            "      \"sell_order_id\":\"txid\", (string, required) order txid of seller\n"
            "      \"deal_price\":n (numeric, required) deal price\n"
            "      \"deal_coin_amount\":n (numeric, required) deal amount of coin\n"
            "      \"deal_asset_amount\":n (numeric, required) deal amount of asset\n"
            "   }\n"
            "       ,...\n"
            " ]\n"
            "3.\"symbol:fee:unit\":(string:numeric:string, optional) fee paid for miner, default is WICC:10000:sawi\n"
            "\nResult:\n"
            "\"txid\" (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitdexsettletx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" "
                           "\"[{\\\"buy_order_id\\\":\\\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\\\", "
                           "\\\"sell_order_id\\\":\\\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8a1\\\", "
                           "\\\"deal_price\\\":100000000,"
                           "\\\"deal_coin_amount\\\":100000000,"
                           "\\\"deal_asset_amount\\\":100000000}]\" ")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitdexsettletx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\", "\
                           "[{\"buy_order_id\":\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\", "
                           "\"sell_order_id\":\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8a1\", "
                           "\"deal_price\":100000000,"
                           "\"deal_coin_amount\":100000000,"
                           "\"deal_asset_amount\":100000000}]")
        );
    }

    EnsureWalletIsUnlocked();

    const CUserID &userId = RPC_PARAM::GetUserId(params[0]);
    Array dealItemArray = params[1].get_array();
    ComboMoney fee = RPC_PARAM::GetFee(params, 2, DEX_TRADE_SETTLE_TX);

    vector<CDEXSettleTx::DealItem> dealItems;
    for (auto dealItemObj : dealItemArray) {
        CDEXSettleTx::DealItem dealItem;
        const Value& buy_order_id      = JSON::GetObjectFieldValue(dealItemObj, "buy_order_id");
        dealItem.buyOrderId            = RPC_PARAM::GetTxid(buy_order_id, "buy_order_id");
        const Value& sell_order_id     = JSON::GetObjectFieldValue(dealItemObj, "sell_order_id");
        dealItem.sellOrderId           = RPC_PARAM::GetTxid(sell_order_id.get_str(), "sell_order_id");
        const Value& deal_price        = JSON::GetObjectFieldValue(dealItemObj, "deal_price");
        dealItem.dealPrice             = RPC_PARAM::GetPrice(deal_price);
        const Value& deal_coin_amount  = JSON::GetObjectFieldValue(dealItemObj, "deal_coin_amount");
        dealItem.dealCoinAmount        = AmountToRawValue(deal_coin_amount);
        const Value& deal_asset_amount = JSON::GetObjectFieldValue(dealItemObj, "deal_asset_amount");
        dealItem.dealAssetAmount       = AmountToRawValue(deal_asset_amount);
        dealItems.push_back(dealItem);
    }

    // Get account for checking balance
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, userId);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());

    int32_t validHeight = chainActive.Height();
    CDEXSettleTx tx(userId, validHeight, fee.symbol, fee.GetAmountInSawi(), dealItems);
    return SubmitTx(account.keyid, tx);
}

Value getdexorder(const Array& params, bool fHelp) {
     if (fHelp || params.size() != 1) {
        throw runtime_error(
            "getdexorder \"order_id\"\n"
            "\nget dex order detail.\n"
            "\nArguments:\n"
            "1.\"order_id\":    (string, required) order txid\n"
            "\nResult: object of order detail\n"
            "\nExamples:\n"
            + HelpExampleCli("getdexorder", "\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("getdexorder", "\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\"")
        );
    }
    const uint256 &orderId = RPC_PARAM::GetTxid(params[0], "order_id");

    auto pDexCache = pCdMan->pDexCache;
    CDEXOrderDetail orderDetail;
    if (!pDexCache->GetActiveOrder(orderId, orderDetail))
        throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("The order not exists or inactive! order_id=%s", orderId.ToString()));

    Object obj;
    DEX_DB::OrderToJson(orderId, orderDetail, obj);
    return obj;
}

extern Value listdexsysorders(const Array& params, bool fHelp) {
     if (fHelp || params.size() > 1) {
        throw runtime_error(
            "listdexsysorders [\"height\"]\n"
            "\nget dex system-generated active orders by block height.\n"
            "\nArguments:\n"
            "1.\"height\":  (numeric, optional) block height, default is current tip block height\n"
            "\nResult:\n"
            "\"height\"     (string) the specified block height.\n"
            "\"orders\"     (string) a list of system-generated DEX orders.\n"
            "\nExamples:\n"
            + HelpExampleCli("listdexsysorders", "10")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("listdexsysorders", "10")
        );
    }

    int64_t tipHeight = chainActive.Height();
    int64_t height    = tipHeight;
    if (params.size() > 0)
        height = params[0].get_int64();

    if (height < 0 || height > tipHeight) {
        throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("height=%d must >= 0 and <= tip_height=%d", height, tipHeight));
    }

    auto pGetter = pCdMan->pDexCache->CreateSysOrdersGetter();
    if (!pGetter->Execute(height)) {
        throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("get system-generated orders error! height=%d", height));
    }

    Object obj;
    obj.push_back(Pair("height", height));
    pGetter->ToJson(obj);

    return obj;
}

extern Value listdexorders(const Array& params, bool fHelp) {
     if (fHelp || params.size() > 4) {
        throw runtime_error(
            "listdexorders [\"begin_height\"] [\"end_height\"] [\"max_count\"] [\"last_pos_info\"]\n"
            "\nget dex all active orders by block height range.\n"
            "\nArguments:\n"
            "1.\"begin_height\":    (numeric, optional) the begin block height, default is 0\n"
            "2.\"end_height\":      (numeric, optional) the end block height, default is current tip block height\n"
            "3.\"max_count\":       (numeric, optional) the max order count to get, default is 500\n"
            "4.\"last_pos_info\":   (string, optional) the last position info to get more orders, default is empty\n"
            "\nResult:\n"
            "\"begin_height\"       (numeric) the begin block height of returned orders.\n"
            "\"end_height\"         (numeric) the end block height of returned orders.\n"
            "\"has_more\"           (bool) has more orders in db.\n"
            "\"last_pos_info\"      (string) the last position info to get more orders.\n"
            "\"count\"              (numeric) the count of returned orders.\n"
            "\"orders\"             (string) a list of system-generated DEX orders.\n"
            "\nExamples:\n"
            + HelpExampleCli("listdexorders", "0 100 500")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("listdexorders", "0, 100, 500")
        );
    }

    int64_t tipHeight = chainActive.Height();
    int64_t beginHeight = 0;
    if (params.size() > 0)
        beginHeight = params[0].get_int64();
    if (beginHeight < 0 || beginHeight > tipHeight) {
        throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("begin_height=%d must >= 0 and <= tip_height=%d", beginHeight, tipHeight));
    }

    int64_t endHeight = tipHeight;
    if (params.size() > 1)
        endHeight = params[1].get_int64();
    if (endHeight < beginHeight || endHeight > tipHeight) {
        throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("end_height=%d must >= begin_height=%d and <= tip_height=%d",
            endHeight, beginHeight, tipHeight));
    }


    int64_t maxCount = 500;
    if (params.size() > 2) {
        maxCount = params[2].get_int64();
        if (maxCount < 0)
            throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("max_count=%d must >= 0", maxCount));
    }

    DEXBlockOrdersCache::KeyType lastKey;
    if (params.size() > 3) {
        string lastPosInfo = RPC_PARAM::GetBinStrFromHex(params[3], "last_pos_info");
        auto err = DEX_DB::ParseLastPos(lastPosInfo, lastKey);
        if (err)
            throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("Invalid last_pos_info! %s", *err));
        uint32_t lastHeight = DEX_DB::GetHeight(lastKey);
        if (lastHeight < beginHeight || lastHeight > endHeight)
            throw JSONRPCError(RPC_INVALID_PARAMS,
                               strprintf("Invalid last_pos_info! height of last_pos_info is not in "
                                         "range(begin=%d,end=%d) ",
                                         beginHeight, endHeight));
    }

    auto pGetter = pCdMan->pDexCache->CreateOrdersGetter();
    if (!pGetter->Execute(beginHeight, endHeight, maxCount, lastKey)) {
        throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("get all active orders error! begin_height=%d, end_height=%d",
            beginHeight, endHeight));
    }

    string newLastPosInfo;
    if (pGetter->has_more) {
        auto err = DEX_DB::MakeLastPos(pGetter->last_key, newLastPosInfo);
        if (err)
            throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("Make new last_pos_info error! %s", *err));
    }
    Object obj;
    obj.push_back(Pair("begin_height", (int64_t)pGetter->begin_height));
    obj.push_back(Pair("end_height", (int64_t)pGetter->end_height));
    obj.push_back(Pair("has_more", pGetter->has_more));
    obj.push_back(Pair("last_pos_info", HexStr(newLastPosInfo)));
    pGetter->ToJson(obj);
    return obj;
}


void CheckAccountRegId(const CUserID uid , const string fieldName){

    if(!uid.is<CRegID>() || !uid.get<CRegID>().IsMature(chainActive.Height())){
        throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("%s must be a matured regid!", fieldName));
    }
    CAccount account ;

    if(!pCdMan->pAccountCache->GetAccount(uid, account))
        throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("the account of %s doest not exist, uid=%s",
                fieldName, uid.ToDebugString()));
}

Value submitdexoperatorregtx(const Array& params, bool fHelp){

    if (fHelp || params.size() < 9  || params.size() > 11){
        throw runtime_error(
            "submitdexoperatorregtx  \"addr\" \"owner_uid\" \"fee_receiver_uid\" \"dex_name\" \"portal_url\" "
            "\"open_mode\" maker_fee_ratio taker_fee_ratio [\"fees\"] [\"memo\"]\n"
            "\nregister a dex operator\n"
            "\nArguments:\n"
            "1.\"addr\":            (string, required) the dex creator's address\n"
            "2.\"owner_uid\":       (string, required) the dexoperator 's owner account \n"
            "3.\"fee_receiver_uid\":(string, required) the dexoperator 's fee receiver account \n"
            "4.\"dex_name\":        (string, required) dex operator's name \n"
            "5.\"portal_url\":      (string, required) the dex operator's website url \n"
            "6.\"open_mode\":     (string, required) indicate the order is PUBLIC or PRIVATE\n"
            "7.\"maker_fee_ratio\": (number, required) range is 0 ~ 50000000, 50000000 stand for 50% \n"
            "8.\"taker_fee_ratio\": (number, required) range is 0 ~ 50000000, 50000000 stand for 50% \n"
            "9.\"order_open_dexop_list\": (array of number, required) order open dexop list, max size is 500\n"
            "10.\"fee\":             (symbol:fee:unit, optional) tx fee,default is the min fee for the tx type  \n"
            "11 \"memo\":            (string, optional) dex memo \n"
            "\nResult:\n"
            "\"txHash\"             (string) The transaction id.\n"

            "\nExamples:\n"
            + HelpExampleCli("submitdexoperatorregtx", "\"0-1\" \"0-1\" \"0-2\" \"wayki-dex\""
                            "\"http://www.wayki-dex.com\" \"PRIVATE\" 2000000 2000000 '[0,1]'")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitdexoperatorregtx", "\"0-1\", \"0-1\", \"0-2\", \"wayki-dex\", "
                            "\"http://www.wayki-dex.com\", \"PRIVATE\", 2000000, 2000000, [0, 1]")

            ) ;
    }

    EnsureWalletIsUnlocked();
    const CUserID &userId = RPC_PARAM::GetUserId(params[0].get_str(),true);
    CDEXOperatorRegisterTx::Data data ;
    data.owner_uid = RPC_PARAM::GetUserId(params[1].get_str());
    data.fee_receiver_uid = RPC_PARAM::GetUserId(params[2].get_str());
    CheckAccountRegId(data.owner_uid, "owner_uid");
    CheckAccountRegId(data.fee_receiver_uid, "fee_receiver_uid");
    data.name = params[3].get_str();
    data.portal_url = params[4].get_str();
    data.order_open_mode    = RPC_PARAM::GetOrderOpenMode(params[5]);
    data.maker_fee_ratio = AmountToRawValue(params[6]);
    data.taker_fee_ratio = AmountToRawValue(params[7]);
    data.order_open_dexop_list = RPC_PARAM::GetOrderOpenDexopList(params[8]);
    ComboMoney fee  = RPC_PARAM::GetFee(params, 9, DEX_OPERATOR_REGISTER_TX);
    data.memo = RPC_PARAM::GetMemo(params, 10);

    // Get account for checking balance
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, userId);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());
    int32_t validHeight = chainActive.Height();

    CDEXOperatorRegisterTx tx(userId, validHeight, fee.symbol, fee.GetAmountInSawi(), data);
    return SubmitTx(account.keyid, tx);
}

Value submitdexopupdatetx(const Array& params, bool fHelp){

    if(fHelp ||params.size()< 4 || params.size() > 5 ){
        throw runtime_error(
                "submitdexopupdatetx  \"tx_uid\" \"dex_id\" \"update_field\" \"value\" \"fee\" \n"
                "\n register a dex operator\n"
                "\nArguments:\n"
                "1.\"tx_uid\":          (string, required) the tx sender, must be the dexoperaor's owner regid\n"
                "2.\"dex_id\":          (number, required) dex operator's id \n"
                "3.\"update_field\":    (nuber, required) the dexoperator field to update\n"
                "                       1: owner_regid (string) the dexoperator 's owner account\n"
                "                       2: fee_receiver_regid: (string) the dexoperator 's fee receiver account\n"
                "                       3: dex_name: (string) dex operator's name\n"
                "                       4: portal_url: (string) the dex operator's website url\n"
                "                       5: open_mode: (string) indicate the order is PUBLIC or PRIVATE\n"
                "                       6: maker_fee_ratio: (number) range is 0 ~ 50000000, 50000000 stand for 50%\n"
                "                       7: taker_fee_ratio (number) range is 0 ~ 50000000, 50000000 stand for 50%\n"
                "                       8: order_open_devop_list (Array of number) order open dexop list, max size is 500\n"
                "                       9: memo\n"
                "4.\"value\":           (string, required) updated value \n"
                "5.\"fee\":             (symbol:fee:unit, optional) tx fee,default is the min fee for the tx type  \n"
                "\nResult:\n"
                "\"txHash\"             (string) The transaction id.\n"
                "\nExamples:\n"
                + HelpExampleCli("submitdexopupdatetx", "0-1 1 1 0-3")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("submitdexopupdatetx", "0-1 1 1 0-3")

                ) ;
    }

    EnsureWalletIsUnlocked();
    const CUserID &userId = RPC_PARAM::GetUserId(params[0].get_str(),true);
    CDEXOperatorUpdateData updateData ;
    updateData.dexId = RPC_PARAM::GetDexId(params[1]);
    updateData.field = CDEXOperatorUpdateData::UpdateField(params[2].get_int());
    const Value & updatedValue = params[3];
    switch (updateData.field){
        case CDEXOperatorUpdateData::UpdateField::OWNER_UID :
        case CDEXOperatorUpdateData::UpdateField::FEE_RECEIVER_UID : {
            updateData.value = CUserID(RPC_PARAM::GetRegId(updatedValue));
            break;
        }
        case CDEXOperatorUpdateData::UpdateField::NAME :
        case CDEXOperatorUpdateData::UpdateField::PORTAL_URL :
        case CDEXOperatorUpdateData::UpdateField::MEMO :
            updateData.value = updatedValue.get_str();
            break;
        case CDEXOperatorUpdateData::UpdateField::OPEN_MODE:
            updateData.value = RPC_PARAM::GetOrderOpenMode(updatedValue);
            break;
        case CDEXOperatorUpdateData::UpdateField::MAKER_FEE_RATIO:
        case CDEXOperatorUpdateData::UpdateField::TAKER_FEE_RATIO:{
            if (updatedValue.type() == json_spirit::Value_type::int_type) {
                updateData.value  = AmountToRawValue(updatedValue);
            } else if (updatedValue.type() == json_spirit::Value_type::str_type) {
                uint64_t ratio = 0;
                if (!ParseUint64(updatedValue.get_str(), ratio)) {
                    throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("Parse update_field=%s as uint64_t type error",
                        updatedValue.get_str()));
                }
                updateData.value = ratio;
            } else {
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid json value type=%s of update_field, "
                    "expect int or string type",
                    JSON::GetValueTypeName(updatedValue.type())));
            }
            break;
        }
        case CDEXOperatorUpdateData::UpdateField::ORDER_OPEN_DEVOP_LIST:{
            if (updatedValue.type() == json_spirit::Value_type::array_type) {
                updateData.value  = RPC_PARAM::GetOrderOpenDexopList(updatedValue);
            } else if (updatedValue.type() == json_spirit::Value_type::str_type) {
                Value devOpArr;
                if (!json_spirit::read_string(updatedValue.get_str(), devOpArr) ||
                        devOpArr.type() == json_spirit::Value_type::array_type)
                    throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("Parse update_field=\"%s\" as array of number type error",
                        updatedValue.get_str()));
                updateData.value = RPC_PARAM::GetOrderOpenDexopList(devOpArr);
            } else {
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid json value type=%s of update_field, "
                    "expect array of number type", JSON::GetValueTypeName(updatedValue.type())));
            }
            break;
        }

        default:
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("unsupport dex update_field=%d", updateData.field));
    }

    string errmsg ;
    string errcode ;
    shared_ptr<CCacheWrapper> spCw = make_shared<CCacheWrapper>(pCdMan);
    if(!updateData.Check(*spCw, errmsg,errcode,chainActive.Height())){
        throw JSONRPCError(RPC_INVALID_PARAMS, errmsg);
    }
    ComboMoney fee = RPC_PARAM::GetFee(params,4, DEX_OPERATOR_UPDATE_TX) ;

    // Get account for checking balance
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, userId);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());
    int32_t validHeight = chainActive.Height();

    CDEXOperatorUpdateTx tx(userId, validHeight, fee.symbol, fee.GetAmountInSawi(), updateData);
    return SubmitTx(account.keyid, tx);

}


extern Value getdexoperator(const Array& params, bool fHelp) {
     if (fHelp || params.size() != 1) {
        throw runtime_error(
            "getdexoperator dex_id\n"
            "\nget dex operator by dex_id.\n"
            "\nArguments:\n"
            "1.\"dex_id\":  (numeric, required) dex id\n"
            "\nResult: dex_operator detail\n"
            "\nExamples:\n"
            + HelpExampleCli("getdexoperator", "10")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("getdexoperator", "10")
        );
    }

    uint32_t dexOrderId = params[0].get_int();
    DexOperatorDetail dexOperator;
    if (!pCdMan->pDexCache->GetDexOperator(dexOrderId, dexOperator))
        throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("dex operator does not exist! dex_id=%lu", dexOrderId));

    Object obj = DexOperatorToJson(*pCdMan->pAccountCache, dexOperator);
    obj.insert(obj.begin(), Pair("id", (uint64_t)dexOrderId));
    return obj;
}

extern Value getdexoperatorbyowner(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "getdexoperatorbyowner owner_addr\n"
            "\nget dex operator by dex operator owner.\n"
            "\nArguments:\n"
            "1.\"owner_addr\":  (string, required) owner address\n"
            "\nResult: dex_operator detail\n"
            "\nExamples:\n"
            + HelpExampleCli("getdexoperatorbyowner", "10-1")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("getdexoperatorbyowner", "10-1")
        );
    }

    const CUserID &userId = RPC_PARAM::GetUserId(params[0]);

    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, userId);
    if (!account.IsRegistered())
        throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("account not registered! uid=%s", userId.ToDebugString()));

    DexOperatorDetail dexOperator;
    uint32_t dexOrderId = 0;
    if (!pCdMan->pDexCache->GetDexOperatorByOwner(account.regid, dexOrderId, dexOperator))
        throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("the owner account dos not have a dex operator! uid=%s", userId.ToDebugString()));

    Object obj = DexOperatorToJson(*pCdMan->pAccountCache, dexOperator);
    obj.insert(obj.begin(), Pair("id", (uint64_t)dexOrderId));
    return obj;
}

extern Value getdexorderfee(const Array& params, bool fHelp) {
     if (fHelp || params.size() < 2 || params.size() > 3) {
        throw runtime_error(
            "getdexorderfee \"addr\" \"tx_type\" [\"fee_symbol\"]\n"
            "\nget dex order fee by account.\n"
            "\nArguments:\n"
            "1.\"addr\":    (string, required) account address\n"
            "2.\"tx_type\": (string, required) tx type, support:\n"
            "                DEX_LIMIT_BUY_ORDER_TX,\n"
            "                DEX_LIMIT_SELL_ORDER_TX,\n"
            "                DEX_MARKET_BUY_ORDER_TX,\n"
            "                DEX_MARKET_SELL_ORDER_TX,\n"
            "                DEX_ORDER_TX,\n"
            "                DEX_OPERATOR_ORDER_TX,\n"
            "                DEX_CANCEL_ORDER_TX\n"
            "3.\"fee_symbol\":  (string, optional) fee symbol, support:\n"
            "                WICC,\n"
            "                WUSD\n"
            "\nResult: dex order fee info\n"
            "\nExamples:\n"
            + HelpExampleCli("getdexorderfee", "\"10-1\" \"DEX_LIMIT_BUY_ORDER_TX\" \"WICC\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("getdexorderfee", "\"10-1\", \"DEX_LIMIT_BUY_ORDER_TX\", \"WICC\"")
        );
    }

    const CUserID& userId   = RPC_PARAM::GetUserId(params[0], true);
    TxType txType = RPC_PARAM::ParseTxType(params[1]);
    TokenSymbol feeSymbol = params.size() > 2 ? params[2].get_str() : SYMB::WICC;

    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, userId);

    int32_t height = chainActive.Height();
    uint64_t defaultMinFee = 0;
    Object obj;
    auto spCw = make_shared<CCacheWrapper>(pCdMan);

    if (DEX_ORDER_TX_SET.count(txType) == 0) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("unsupport order tx_type=%s", GetTxTypeName(txType)));
    }
    if (kFeeSymbolSet.count(feeSymbol) == 0) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("unsupport fee_symbol=%s", feeSymbol));
    }

    if (!GetTxMinFee(*spCw, txType, height, feeSymbol, defaultMinFee))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("get default min fee of tx failed! "
            "tx=%s, height=%d, symbol=%s", GetTxTypeName(txType), height, feeSymbol));

    auto stakedWiccAmount = account.GetToken(SYMB::WICC).staked_amount;
    uint64_t actualMinFee = dex::CalcOrderMinFee(defaultMinFee, stakedWiccAmount);

    Object accountObj;
    accountObj.push_back(Pair("addr",  account.keyid.ToAddress()));
    accountObj.push_back(Pair("regid", account.regid.ToString()));
    accountObj.push_back(Pair("staked_wicc_amount", stakedWiccAmount));

    obj.push_back(Pair("actual_min_fee", actualMinFee));
    obj.push_back(Pair("default_min_fee", defaultMinFee));
    obj.push_back(Pair("min_fee_for_pubkey", defaultMinFee * 2));
    obj.push_back(Pair("block_height", height));
    obj.push_back(Pair("account", accountObj));

    return obj;
}
