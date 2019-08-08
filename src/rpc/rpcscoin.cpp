// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcscoin.h"

#include "commons/base58.h"
#include "config/const.h"
#include "rpc/core/rpcserver.h"
#include "rpc/core/rpccommons.h"
#include "init.h"
#include "net.h"
#include "miner/miner.h"
#include "commons/util.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#include "tx/cdptx.h"
#include "tx/dextx.h"
#include "tx/pricefeedtx.h"
#include "tx/assettx.h"

Value submitpricefeedtx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 2 || params.size() > 3) {
        throw runtime_error(
            "submitpricefeedtx {price_feeds_json} [fee]\n"
            "\nsubmit a Price Feed Tx.\n"
            "\nArguments:\n"
            "1. \"address\" :                   (string, required) Price Feeder's address\n"
            "2. \"pricefeeds\":                 (string, required) A json array of pricefeeds\n"
            " [\n"
            "   {\n"
            "      \"coin\": \"WICC|WGRT\",     (string, required) The coin type\n"
            "      \"currency\": \"USD|CNY\"    (string, required) The currency type\n"
            "      \"price\":                   (number, required) The price (boosted by 10^4) \n"
            "   }\n"
            "       ,...\n"
            " ]\n"
            "3.\"fee\":                         (numeric, optional) fee pay for miner, default is 10000\n"
            "\nResult:\n"
            "\"txid\"                           (string) The transaction id.\n"
            "\nExamples:\n" +
            HelpExampleCli("submitpricefeedtx",
                           "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" "
                           "\"[{\\\"coin\\\": \\\"WICC\\\", \\\"currency\\\": \\\"USD\\\", \\\"price\\\": 2500}]\"\n") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("submitpricefeedtx",
                           "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"[{\"coin\": \"WICC\", \"currency\": \"USD\", "
                           "\"price\": 2500}]\"\n"));
    }

    RPCTypeCheck(params, boost::assign::list_of(str_type)(array_type)(int_type));

    auto feedUid = CUserID::ParseUserId(params[0].get_str());
    if (!feedUid) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid addr");
    }

    Array arrPricePoints = params[1].get_array();
    vector<CPricePoint> pricePoints;
    for (auto objPp : arrPricePoints) {
        const Value& coinValue = find_value(objPp.get_obj(), "coin");
        const Value& currencyValue = find_value(objPp.get_obj(), "currency");
        const Value& priceValue = find_value(objPp.get_obj(), "price");
        if (    coinValue.type() == null_type
            ||  currencyValue.type() == null_type
            ||  priceValue.type() == null_type ) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "null type not allowed!");
        }

        string coinStr = coinValue.get_str();
        if (!kCoinTypeSet.count(coinStr))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid coin symbol: %s", coinStr));

        string currencyStr = currencyValue.get_str();
        if (!kCurrencyTypeSet.count(currencyStr))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid currency type: %s", currencyStr));

        uint64_t price = priceValue.get_int64();

        CoinPricePair cpp(coinStr, currencyStr);
        CPricePoint pp(cpp, price);
        pricePoints.push_back(pp);
    }

    uint64_t minFee = GetTxMinFee(PRICE_FEED_TX, chainActive.Height());
    uint64_t fees = minFee;
    if (params.size() == 3) {
        fees = AmountToRawValue(params[2]);
        if (fees < minFee)
            throw JSONRPCError(RPC_WALLET_ERROR,
                               strprintf("Tx fee given is too small: %d < %d", fees, minFee));
    }

    int32_t validHeight = chainActive.Height();
    CPriceFeedTx tx(*feedUid, validHeight, fees, pricePoints);
    return SubmitTx(*feedUid, tx);
}

Value submitstakefcointx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 2 || params.size() > 3) {
        throw runtime_error(
            "submitstakefcointx \"addr\" \"fcoin amount\" [fee]\n"
            "\nstake fcoins\n"
            "\nArguments:\n"
            "1.\"addr\":            (string, required)\n"
            "2.\"fcoin amount\":    (numeric, required) amount of fcoins to stake\n"
            "3.\"fee\":             (numeric, optional) fee pay for miner, default is 10000\n"
            "\nResult:\n"
            "\"txid\"               (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitstakefcointx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" 200000000\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitstakefcointx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" 200000000\n")
        );
    }

    const CUserID &userId = RPC_PARAM::GetUserId(params[0].get_str());

    int64_t stakeAmount = params[1].get_int64();
    uint64_t minFee = GetTxMinFee(FCOIN_STAKE_TX, chainActive.Height());
    uint64_t fees = minFee;
    if (params.size() == 3) {
        fees = AmountToRawValue(params[2]);
        if (fees < minFee)
            throw JSONRPCError(RPC_WALLET_ERROR, strprintf("Tx fee given is too small: %d < %d",
                            fees, minFee));
    }
    int32_t validHeight = chainActive.Tip()->height;

    BalanceOpType stakeType = stakeAmount >= 0 ? BalanceOpType::STAKE : BalanceOpType::UNSTAKE;
    CFcoinStakeTx tx(userId, validHeight, fees, stakeType, std::abs(stakeAmount));
    return SubmitTx(userId, tx);
}

/*************************************************<< CDP >>**************************************************/
Value submitstakecdptx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 3 || params.size() > 6) {
        throw runtime_error(
            "submitstakecdptx \"addr\" stake_combo_money mint_combo_money [\"cdp_id\"] [symbol:fee:unit]\n"
            "\nsubmit a CDP Staking Tx.\n"
            "\nArguments:\n"
            "1. \"addr\": CDP Staker's account address\n"
            "2. \"stake_combo_money\":  (symbol:amount:unit, required) Combo Money to stake into the CDP,"
                                                                      " default symbol=WICC, default unit=sawi\n"
            "3. \"mint_combo_money\":   (symbol:amount:unit, required), Combo Money to mint from the CDP,"
                                                                      " default symbol=WUSD, default unit=sawi\n"
            "4. \"cdp_id\":         (string, optional) CDP ID (tx hash of the first CDP Stake Tx)\n"
            "5. \"symbol:fee:unit\": (symbol:amount:unit, optional) fee paid to miner, default is WICC:100000:sawi\n"
            "\nResult:\n"
            "\"txid\"               (string) The transaction id.\n"
            "\nExamples:\n" +
            HelpExampleCli("submitstakecdptx",
                           "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" 20000000000 3000000 "
                           "\"b850d88bf1bed66d43552dd724c18f10355e9b6657baeae262b3c86a983bee71\" \"WICC:1000000:sawi\"\n") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("submitstakecdptx",
                           "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\", 2000000000, 3000000, "
                           "\"b850d88bf1bed66d43552dd724c18f10355e9b6657baeae262b3c86a983bee71\", \"WICC:1000000:sawi\"\n"));
    }

    const CUserID &cdpUid = RPC_PARAM::GetUserId(params[0]);

    ComboMoney cmBcoinsToStake, cmScoinsToMint;
    if (!ParseRpcInputMoney(params[1].get_str(), cmBcoinsToStake, SYMB::WICC))
        throw JSONRPCError(RPC_INVALID_PARAMETER, "bcoinsToStake ComboMoney format error");

    if (!ParseRpcInputMoney(params[2].get_str(), cmScoinsToMint, SYMB::WUSD))
        throw JSONRPCError(RPC_INVALID_PARAMETER, "scoinsToMint ComboMoney format error");

    int validHeight = chainActive.Tip()->height;

    uint256 cdpId;
    if (params.size() > 3) {
        // TODO: check the txid format
        cdpId.SetHex(params[3].get_str());
    }

    const ComboMoney &cmFee = RPC_PARAM::GetFee(params, 4, CDP_STAKE_TX);

    if (cdpId.IsEmpty()) { // new stake cdp
        if (cmBcoinsToStake.amount == 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Error: stake_amount is zero!");

        if (cmScoinsToMint.amount == 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Error: mint_amount is zero!");
    }

    CCDPStakeTx tx(cdpUid, validHeight, cdpId, cmFee, cmBcoinsToStake, cmScoinsToMint);
    return SubmitTx(cdpUid, tx);
}

Value submitredeemcdptx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 4 || params.size() > 5) {
        throw runtime_error(
            "submitredeemcdptx \"addr\" \"cdp_id\" repay_amount redeem_amount [\"symbol:fee:unit\"]\n"
            "\nsubmit a CDP Redemption Tx\n"
            "\nArguments:\n"
            "1. \"addr\" : (string) CDP redemptor's address\n"
            "2. \"cdp_id\": (string) ID of existing CDP (tx hash of the first CDP Stake Tx)\n"
            "3. \"repay_amount\": (numeric required) scoins (E.g. WUSD) to stake into the CDP, boosted by 10^8\n"
            "4. \"redeem_amount\": (numeric required) bcoins (E.g. WICC) to stake into the CDP, boosted by 10^8\n"
            "5. \"symbol:fee:unit\": (string:numeric:string, optional) fee paid to miner, default is WICC:100000:sawi\n"
            "\nResult:\n"
            "\"txid\" (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitredeemcdptx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"b850d88bf1bed66d43552dd724c18f10355e9b6657baeae262b3c86a983bee71\"  20000000000 30000 \"1000000\"\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitredeemcdptx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\", \"b850d88bf1bed66d43552dd724c18f10355e9b6657baeae262b3c86a983bee71\", 2000000000, 30000, \"1000000\"\n")
        );
    }
    EnsureWalletIsUnlocked();

    const CUserID &cdpUid = RPC_PARAM::GetUserId(params[0].get_str());

    uint256 cdpTxId     = uint256S(params[1].get_str());
    uint64_t repayAmount = params[2].get_uint64();
    uint64_t redeemAmount = params[3].get_uint64();

    const ComboMoney &cmFee = RPC_PARAM::GetFee(params, 4, CDP_STAKE_TX);

    int validHeight = chainActive.Tip()->height;

    CCDPRedeemTx tx(cdpUid, cmFee, validHeight, cdpTxId, repayAmount, redeemAmount);
    return SubmitTx(cdpUid, tx);
}

Value submitliquidatecdptx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 3 || params.size() > 4) {
        throw runtime_error(
            "submitliquidatecdptx \"addr\" \"cdp_id\" liquidate_amount [symbol:fee:unit]\n"
            "\nsubmit a CDP Liquidation Tx\n"
            "\nArguments:\n"
            "1. \"addr\" : (string required) CDP liquidator's address\n"
            "2. \"cdp_id\": (string required) ID of existing CDP (tx hash of the first CDP Stake Tx)\n"
            "3. \"liquidate_amount\": (numeric required) WUSD coins to repay to CDP, boosted by 10^8 (penalty fees deducted separately from sender account)\n"
            "4. \"symbol:fee:unit\": (string:numeric:string, optional) fee paid to miner, default is WICC:100000:sawi\n"
            "\nResult:\n"
            "\"txid\" (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitliquidatecdptx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\"  \"b850d88bf1bed66d43552dd724c18f10355e9b6657baeae262b3c86a983bee71\" 20000000000 \"WICC:1000000\"\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitliquidatecdptx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\", \"b850d88bf1bed66d43552dd724c18f10355e9b6657baeae262b3c86a983bee71\", 2000000000, \"WICC:1000000\"\n")
        );
    }
    const CUserID &userId = RPC_PARAM::GetUserId(params[0]);
    const uint256 &cdpTxId  = RPC_PARAM::GetTxid(params[1]);
    uint64_t liquidateAmount  = AmountToRawValue(params[2]);
    const ComboMoney &cmFee = RPC_PARAM::GetFee(params, 3, CDP_STAKE_TX);

    int validHeight = chainActive.Tip()->height;
    CCDPLiquidateTx tx(userId, cmFee, validHeight, cdpTxId, liquidateAmount);
    return SubmitTx(userId, tx);
}

Value getmedianprice(const Array& params, bool fHelp){
    if (fHelp) {
        throw runtime_error(
            "getmedianprice [height]\n"
            "\nget current median price or query at specified height.\n"
            "\nArguments:\n"
            "1.\"height\": (numeric, optional), specified height. If not provided, use the tip block height in chainActive\n\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("getmedianprice","")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("getmedianprice","")
        );
    }

    int height = chainActive.Tip()->height;
    if (params.size() > 0){
        height = params[0].get_int();
        if (height < 0 || height > chainActive.Height())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range.");
    }

    CBlock block;
    CBlockIndex* pBlockIndex = chainActive[height];
    if (!ReadBlockFromDisk(pBlockIndex, block)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Can't read block from disk");
    }

    Array prices;
    for (auto& item : block.GetBlockMedianPrice()) {
        Object price;
        price.push_back(Pair("coin_symbol",     item.first.first));
        price.push_back(Pair("price_symbol",    item.first.second));
        price.push_back(Pair("price",           (double)item.second / kPercentBoost));
        prices.push_back(price);
    }

    Object obj;
    obj.push_back(Pair("median_price", prices));
    return obj;
}

Value listcdps(const Array& params, bool fHelp);
Value listcdpstoliquidate(const Array& params, bool fHelp);

Value getusercdp(const Array& params, bool fHelp){
    if (fHelp || params.size() < 1 || params.size() > 2) {
        throw runtime_error(
            "getusercdp \"addr\"\n"
            "\nget account's cdp.\n"
            "\nArguments:\n"
            "1.\"addr\": (string, required) CDP owner's account addr\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("getusercdp", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\"\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("getusercdp", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\"\n")
        );
    }

    auto pUserId = CUserID::ParseUserId(params[0].get_str());
    if (!pUserId) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid addr");
    }

    CAccount txAccount;
    if (!pCdMan->pAccountCache->GetAccount(*pUserId, txAccount)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                            strprintf("The account not exists! userId=%s", pUserId->ToString()));
    }
    assert(!txAccount.regid.IsEmpty());

    int height = chainActive.Tip()->height;
    uint64_t bcoinMedianPrice = pCdMan->pPpCache->GetBcoinMedianPrice(height);

    Array cdps;
    vector<CUserCDP> userCdps;
    if (pCdMan->pCdpCache->GetCDPList(txAccount.regid, userCdps)) {
        for (auto& cdp : userCdps) {
            cdps.push_back(cdp.ToJson(bcoinMedianPrice));
        }
    }

    Object obj;
    obj.push_back(Pair("user_cdps", cdps));
    return obj;
}

Value getcdp(const Array& params, bool fHelp){
    if (fHelp || params.size() < 1 || params.size() > 2) {
        throw runtime_error(
            "getcdp \"cdp_id\"\n"
            "\nget CDP by its CDP_ID\n"
            "\nArguments:\n"
            "1.\"cdp_id\": (string, required) cdp_id\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("getcdp", "\"c01f0aefeeb25fd6afa596f27ee3a1e861b657d2e1c341bfd1c412e87d9135c8\"\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("getcdp", "\"c01f0aefeeb25fd6afa596f27ee3a1e861b657d2e1c341bfd1c412e87d9135c8\"\n")
        );
    }

    int32_t height = chainActive.Tip()->height;
    uint64_t bcoinMedianPrice = pCdMan->pPpCache->GetBcoinMedianPrice(height);

    uint256 cdpTxId(uint256S(params[0].get_str()));
    CUserCDP cdp;
    if (!pCdMan->pCdpCache->GetCDP(cdpTxId, cdp)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("CDP (%s) does not exist!", cdpTxId.GetHex()));
    }

    Object obj;
    obj.push_back(Pair("cdp", cdp.ToJson(bcoinMedianPrice)));
    return obj;
}

/*************************************************<< DEX >>**************************************************/
Value submitdexbuylimitordertx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 5 || params.size() > 6) {
        throw runtime_error(
            "submitdexbuylimitordertx \"addr\" \"coin_symbol\" \"asset_symbol\" asset_amount price [symbol:fee:unit]\n"
            "\nsubmit a dex buy limit price order tx.\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) order owner address\n"
            "2.\"coin_symbol\": (string required) coin type to pay\n"
            "3.\"asset_symbol\": (string required), asset type to buy\n"
            "4.\"asset_amount\": (numeric, required) amount of target asset to buy\n"
            "5.\"price\": (numeric, required) bidding price willing to buy\n"
            "6.\"symbol:fee:unit\":(string:numeric:string, optional) fee paid for miner, default is WICC:10000:sawi\n"
            "\nResult:\n"
            "\"txid\" (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitdexbuylimitordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"WUSD\" \"WICC\" 1000000 200000000\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitdexbuylimitordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"WUSD\" \"WICC\" 1000000 200000000\n")
        );
    }
    const CUserID &userId = RPC_PARAM::GetUserId(params[0]);
    const TokenSymbol& coinSymbol  = RPC_PARAM::GetOrderCoinSymbol(params[1]);
    const TokenSymbol& assetSymbol = RPC_PARAM::GetOrderAssetSymbol(params[2]);
    uint64_t assetAmount  = AmountToRawValue(params[3]);
    uint64_t price        = RPC_PARAM::GetPrice(params[4]); // TODO: need to check price?
    ComboMoney fee = RPC_PARAM::GetFee(params, 5, DEX_LIMIT_BUY_ORDER_TX);

    // Get account for checking balance
    CAccount txAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, userId);
    RPC_PARAM::CheckAccountBalance(txAccount, fee.symbol, SUB_FREE, fee.GetSawiAmount());
    uint64_t coinAmount = CDEXOrderBaseTx::CalcCoinAmount(assetAmount, price);
    RPC_PARAM::CheckAccountBalance(txAccount, coinSymbol, FREEZE, coinAmount);

    int validHeight = chainActive.Height();
    CDEXBuyLimitOrderTx tx(userId, validHeight, fee.symbol, fee.GetSawiAmount(), coinSymbol,
                           assetSymbol, assetAmount, price);
    return SubmitTx(userId, tx);
}

Value submitdexselllimitordertx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 5 || params.size() > 6) {
        throw runtime_error(
            "submitdexselllimitordertx \"addr\" \"coin_symbol\" \"asset_symbol\" asset_amount price [symbol:fee:unit]\n"
            "\nsubmit a dex buy limit price order tx.\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) order owner address\n"
            "2.\"coin_symbol\": (string required) coin type to pay\n"
            "3.\"asset_symbol\": (string required), asset type to buy\n"
            "4.\"asset_amount\": (numeric, required) amount of target asset to buy\n"
            "5.\"price\": (numeric, required) bidding price willing to buy\n"
            "6.\"symbol:fee:unit\":(string:numeric:string, optional) fee paid for miner, default is WICC:10000:sawi\n"
            "\nResult:\n"
            "\"txid\" (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitdexselllimitordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"WICC\" \"WUSD\" 1000000 200000000\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitdexselllimitordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"WICC\" \"WUSD\" 1000000 200000000\n")
        );
    }

    const CUserID &userId = RPC_PARAM::GetUserId(params[0]);
    const TokenSymbol& coinSymbol  = RPC_PARAM::GetOrderCoinSymbol(params[1]);
    const TokenSymbol& assetSymbol = RPC_PARAM::GetOrderAssetSymbol(params[2]);
    uint64_t assetAmount  = AmountToRawValue(params[3]);
    uint64_t price        = RPC_PARAM::GetPrice(params[4]);
    ComboMoney fee = RPC_PARAM::GetFee(params, 5, DEX_LIMIT_SELL_ORDER_TX);

    // Get account for checking balance
    CAccount txAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, userId);
    RPC_PARAM::CheckAccountBalance(txAccount, fee.symbol, SUB_FREE, fee.GetSawiAmount());
    RPC_PARAM::CheckAccountBalance(txAccount, assetSymbol, FREEZE, assetAmount);

    int validHeight = chainActive.Height();
    CDEXSellLimitOrderTx tx(userId, validHeight, fee.symbol, fee.GetSawiAmount(), coinSymbol, assetSymbol, assetAmount, price);
    return SubmitTx(userId, tx);
}

Value submitdexbuymarketordertx(const Array& params, bool fHelp) {
     if (fHelp || params.size() < 4 || params.size() > 5) {
        throw runtime_error(
            "submitdexbuymarketordertx \"addr\" \"coin_symbol\" coin_amount \"asset_symbol\" [symbol:fee:unit]\n"
            "\nsubmit a dex buy market price order tx.\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) order owner address\n"
            "2.\"coin_symbol\": (string required) coin type to pay\n"
            "3.\"coin_amount\": (numeric, required) amount of target coin to buy\n"
            "4.\"asset_symbol\": (string required), asset type to buy\n"
            "5.\"symbol:fee:unit\":(string:numeric:string, optional) fee paid for miner, default is WICC:10000:sawi\n"
            "\nResult:\n"
            "\"txid\" (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitdexbuymarketordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"WICC\" 200000000 \"WUSD\"\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitdexbuymarketordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"WICC\" 200000000 \"WUSD\"\n")
        );
    }

    const CUserID &userId = RPC_PARAM::GetUserId(params[0]);
    const TokenSymbol& coinSymbol  = RPC_PARAM::GetOrderCoinSymbol(params[1]);
    uint64_t coinAmount  = AmountToRawValue(params[2]);
    const TokenSymbol& assetSymbol = RPC_PARAM::GetOrderAssetSymbol(params[3]);
    ComboMoney fee = RPC_PARAM::GetFee(params, 4, DEX_MARKET_BUY_ORDER_TX);

    // Get account for checking balance
    CAccount txAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, userId);
    RPC_PARAM::CheckAccountBalance(txAccount, fee.symbol, SUB_FREE, fee.GetSawiAmount());
    RPC_PARAM::CheckAccountBalance(txAccount, coinSymbol, FREEZE, coinAmount);

    int validHeight = chainActive.Height();
    CDEXBuyMarketOrderTx tx(userId, validHeight, fee.symbol, fee.GetSawiAmount(), coinSymbol, assetSymbol, coinAmount);
    return SubmitTx(userId, tx);
}

Value submitdexsellmarketordertx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 4 || params.size() > 5) {
        throw runtime_error(
            "submitdexsellmarketordertx \"addr\" \"coin_symbol\" \"asset_symbol\" asset_amount [symbol:fee:unit]\n"
            "\nsubmit a dex sell market price order tx.\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) order owner address\n"
            "2.\"coin_symbol\": (string required) coin type to pay\n"
            "3.\"asset_symbol\": (string required), asset type to buy\n"
            "4.\"asset_amount\": (numeric, required) amount of target asset to buy\n"
            "5.\"symbol:fee:unit\":(string:numeric:string, optional) fee paid for miner, default is WICC:10000:sawi\n"
            "\nResult:\n"
            "\"txid\" (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitdexsellmarketordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"WUSD\" \"WICC\" 200000000\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitdexsellmarketordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"WUSD\" \"WICC\" 200000000\n")
        );
    }

    const CUserID &userId = RPC_PARAM::GetUserId(params[0]);
    const TokenSymbol& coinSymbol  = RPC_PARAM::GetOrderCoinSymbol(params[1]);
    const TokenSymbol& assetSymbol = RPC_PARAM::GetOrderAssetSymbol(params[2]);
    uint64_t assetAmount  = AmountToRawValue(params[3]);
    ComboMoney fee = RPC_PARAM::GetFee(params, 4, DEX_MARKET_SELL_ORDER_TX);

    // Get account for checking balance
    CAccount txAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, userId);
    RPC_PARAM::CheckAccountBalance(txAccount, fee.symbol, SUB_FREE, fee.GetSawiAmount());
    RPC_PARAM::CheckAccountBalance(txAccount, assetSymbol, FREEZE, assetAmount);

    int validHeight = chainActive.Height();
    CDEXSellMarketOrderTx tx(userId, validHeight, fee.symbol, fee.GetSawiAmount(), coinSymbol, assetSymbol, assetAmount);
    return SubmitTx(userId, tx);
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
            + HelpExampleRpc("submitdexcancelordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" "\
                             "\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\"")
        );
    }

    const CUserID &userId = RPC_PARAM::GetUserId(params[0]);
    const uint256 &txid = RPC_PARAM::GetTxid(params[1]);
    ComboMoney fee = RPC_PARAM::GetFee(params, 2, DEX_MARKET_SELL_ORDER_TX);

    // Get account for checking balance
    CAccount txAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, userId);
    RPC_PARAM::CheckAccountBalance(txAccount, fee.symbol, SUB_FREE, fee.GetSawiAmount());

    // check active order tx
    RPC_PARAM::CheckActiveOrderExisted(*pCdMan->pDexCache, txid);

    int validHeight = chainActive.Height();
    CDEXCancelOrderTx tx(userId, validHeight, fee.symbol, fee.GetSawiAmount(), txid);
    return SubmitTx(userId, tx);
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
            "      \"buy_order_txid\":\"txid\", (string, required) order txid of buyer\n"
            "      \"sell_order_txid\":\"txid\", (string, required) order txid of seller\n"
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
                           "\"[{\\\"buy_order_txid\\\":\\\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\\\", "
                           "\\\"sell_order_txid\\\":\\\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8a1\\\", "
                           "\\\"deal_price\\\":100000000,"
                           "\\\"deal_coin_amount\\\":100000000,"
                           "\\\"deal_asset_amount\\\":100000000}]\" ")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitdexsettletx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" "\
                           "[{\"buy_order_txid\":\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\", "
                           "\"sell_order_txid\":\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8a1\", "
                           "\"deal_price\":100000000,"
                           "\"deal_coin_amount\":100000000,"
                           "\"deal_asset_amount\":100000000}]")
        );
    }
    const CUserID &userId = RPC_PARAM::GetUserId(params[0]);
    Array dealItemArray = params[1].get_array();
    ComboMoney fee = RPC_PARAM::GetFee(params, 2, DEX_LIMIT_BUY_ORDER_TX);

    vector<DEXDealItem> dealItems;
    for (auto dealItemObj : dealItemArray) {
        DEXDealItem dealItem;
        const Value& buy_order_txid = JSON::GetObjectFieldValue(dealItemObj, "buy_order_txid");
        dealItem.buyOrderId = RPC_PARAM::GetTxid(buy_order_txid);
        const Value& sell_order_txid = JSON::GetObjectFieldValue(dealItemObj, "sell_order_txid");
        dealItem.sellOrderId = RPC_PARAM::GetTxid(sell_order_txid.get_str());
        const Value& deal_price = JSON::GetObjectFieldValue(dealItemObj, "deal_price");
        dealItem.dealPrice = RPC_PARAM::GetPrice(deal_price);
        const Value& deal_coin_amount = JSON::GetObjectFieldValue(dealItemObj, "deal_coin_amount");
        dealItem.dealCoinAmount = AmountToRawValue(deal_coin_amount);
        const Value& deal_asset_amount = JSON::GetObjectFieldValue(dealItemObj, "deal_asset_amount");
        dealItem.dealAssetAmount = AmountToRawValue(deal_asset_amount);
        dealItems.push_back(dealItem);
    }

    // Get account for checking balance
    CAccount txAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, userId);
    RPC_PARAM::CheckAccountBalance(txAccount, fee.symbol, SUB_FREE, fee.GetSawiAmount());

    int validHeight = chainActive.Height();
    CDEXSettleTx tx(userId, validHeight, fee.symbol, fee.GetSawiAmount(), dealItems);
    return SubmitTx(userId, tx);
}

///////////////////////////////////////////////////////////////////////////////
// asset tx rpc

Value submitassetissuetx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 6 || params.size() > 7) {
        throw runtime_error(
            "submitassetissuetx \"addr\" \"asset_symbol\" \"asset_owner_addr\" \"asset_name\" total_supply mintable [symbol:fee:unit]\n"
            "\nsubmit an asset issue tx.\n"
            "\nthe tx creator must have enough WICC for issued fee(550 WICC).\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) tx owner address\n"
            "2.\"asset_symbol\": (string required) asset symbol, E.g WICC | WUSD\n"
            "3.\"asset_owner_addr\": (string required) asset owner address, can be same as tx owner address\n"
            "4.\"asset_name\": (string required) asset long name, E.g WaykiChain coin\n"
            "5.\"total_supply\": (numeric required) asset total supply\n"
            "6.\"mintable\": (boolean required) whether this asset token can be minted in the future\n"
            "7.\"symbol:fee:unit\":(string:numeric:string, optional) fee paid for miner, default is WICC:10000:sawi\n"
            "\nResult:\n"
            "\"txid\" (string) The new transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitassetissuetx", "\"10-2\" \"CNY\" \"10-2\" \"RMB\" 1000000000000000 true")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitassetissuetx", "\"10-2\" \"CNY\" \"10-2\" \"RMB\" 1000000000000000 true")
        );
    }
    const CUserID &uid = RPC_PARAM::GetUserId(params[0]);
    const TokenSymbol &assetSymbol = RPC_PARAM::GetAssetIssueSymbol(params[1]);
    const CUserID &assetOwnerUid = RPC_PARAM::GetUserId(params[2]);
    const TokenName &assetName = RPC_PARAM::GetAssetName(params[3]);
    int64_t totalSupply = params[4].get_int64();
    if (totalSupply <= 0 || (uint64_t)totalSupply > MAX_ASSET_TOTAL_SUPPLY) 
        throw JSONRPCError(RPC_INVALID_PARAMS,
                           strprintf("asset total_supply=%lld can not <= 0 or > %llu", totalSupply, MAX_ASSET_TOTAL_SUPPLY));
    bool mintable = params[5].get_bool();
    ComboMoney fee = RPC_PARAM::GetFee(params, 6, DEX_MARKET_SELL_ORDER_TX);

    // Get account for checking balance
    CAccount txAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, uid);
    RPC_PARAM::CheckAccountBalance(txAccount, SYMB::WICC, SUB_FREE, ASSET_ISSUE_FEE);
    RPC_PARAM::CheckAccountBalance(txAccount, fee.symbol, SUB_FREE, fee.GetSawiAmount());

    CAsset asset(assetSymbol, assetOwnerUid, assetName, (uint64_t)totalSupply, mintable);
    int validHeight = chainActive.Height();
    CAssetIssueTx tx(uid, validHeight, fee.symbol, fee.GetSawiAmount(), asset);
    return SubmitTx(uid, tx);
}

Value submitassetupdatetx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 5 || params.size() > 6) {
        throw runtime_error(
            "submitassetupdatetx \"addr\" \"asset_symbol\" \"asset_owner_addr\" \"asset_name\" mint_amount [symbol:fee:unit]\n"
            "\nsubmit an asset issue tx.\n"
            "\nthe tx creator must have enough WICC for issued fee(550 WICC).\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) tx owner address\n"
            "2.\"asset_symbol\": (string required) asset symbol, E.g WICC | WUSD\n"
            "3.\"asset_owner_addr\": (string required) asset owner address, can be same as tx owner address\n"
            "4.\"asset_name\": (string required) asset long name, E.g WaykiChain coin\n"
            "5.\"mint_amount\": (numeric required) mint amount\n"
            "6.\"symbol:fee:unit\":(string:numeric:string, optional) fee paid for miner, default is WICC:10000:sawi\n"
            "\nResult:\n"
            "\"txid\" (string) The new transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitassetupdatetx", "\"10-2\" \"CNY\" \"10-2\" \"RMB\" 100000000")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitassetupdatetx", "\"10-2\" \"CNY\" \"10-2\" \"RMB\" 100000000")
        );
    }

    const CUserID &uid = RPC_PARAM::GetUserId(params[0]);
    const TokenSymbol &assetSymbol = RPC_PARAM::GetAssetIssueSymbol(params[1]);
    const CUserID &assetOwnerUid = RPC_PARAM::GetUserId(params[2]);
    const TokenName &assetName = RPC_PARAM::GetAssetName(params[3]);
    int64_t mintAmount = params[4].get_int64();
    if (mintAmount < 0 || (uint64_t)mintAmount > MAX_ASSET_TOTAL_SUPPLY) 
        throw JSONRPCError(RPC_INVALID_PARAMS,
                           strprintf("asset min_amount=%lld can not < 0 or > %llu", mintAmount, MAX_ASSET_TOTAL_SUPPLY));
    ComboMoney fee = RPC_PARAM::GetFee(params, 5, DEX_MARKET_SELL_ORDER_TX);

    // Get account for checking balance
    CAccount txAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, uid);
    RPC_PARAM::CheckAccountBalance(txAccount, SYMB::WICC, SUB_FREE, ASSET_UPDATE_FEE);
    RPC_PARAM::CheckAccountBalance(txAccount, fee.symbol, SUB_FREE, fee.GetSawiAmount());

    int validHeight = chainActive.Height();
    CAssetUpdateTx tx(uid, validHeight, fee.symbol, fee.GetSawiAmount(), assetSymbol,
                      assetOwnerUid, assetName, mintAmount);

    return SubmitTx(uid, tx);
}
