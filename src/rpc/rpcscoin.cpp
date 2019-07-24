// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcscoin.h"

#include "commons/base58.h"
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

Value submitpricefeedtx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 2 || params.size() > 3) {
        throw runtime_error(
            "submitpricefeedtx {price_feeds_json} [fee]\n"
            "\nsubmit a Price Feed Tx.\n"
            "\nthe execution include registercontracttx and callcontracttx.\n"
            "\nArguments:\n"
            "1. \"address\" : Price Feeder's address\n"
            "2. \"pricefeeds\"    (string, required) A json array of pricefeeds\n"
            " [\n"
            "   {\n"
            "      \"coin\": \"WICC|WGRT\", (string, required) The coin type\n"
            "      \"currency\": \"USD|CNY\" (string, required) The currency type\n"
            "      \"price\": (number, required) The price (boosted by 10^4) \n"
            "   }\n"
            "       ,...\n"
            " ]\n"
            "3.\"fee\": (numeric, optional) fee pay for miner, default is 10000\n"
            "\nResult:\n"
            "\"txid\" (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitpricefeedtx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" "
                            "\"[{\\\"coin\\\", WICC, \\\"currency\\\": \\\"USD\\\", \\\"price\\\": 0.28}]\"\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitpricefeedtx","\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"[{\"coin\", WICC, \"currency\": \"USD\", \"price\": 0.28}]\"\n"));
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
        CoinType coinType;
        if (coinStr == "WICC") {
            coinType = CoinType::WICC;
        } else if (coinStr == "WUSD") {
            coinType = CoinType::WUSD;
        } else if (coinStr == "WGRT") {
            coinType = CoinType::WGRT;
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid coin type: %s", coinStr));
        }

        string currencyStr = currencyValue.get_str();
        PriceType currencyType;
        if (currencyStr == "USD") {
            currencyType = PriceType::USD;
        } else if (currencyStr == "CNY") {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "CNY stablecoin not supported yet");
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid currency type: %s", currencyStr));
        }

        uint64_t price = priceValue.get_int64();
        CCoinPriceType cpt(coinType, currencyType);
        CPricePoint pp(cpt, price);
        pricePoints.push_back(pp);
    }

    int64_t fees = params.size() == 3 ? params[2].get_int64() : 0;
    if (fees < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid fees: %ld", fees));
    }

    int32_t validHeight = chainActive.Height();
    CPriceFeedTx tx(*feedUid, validHeight, fee, pricePoints);
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

    auto pUserId = CUserID::ParseUserId(params[0].get_str());
    if (!pUserId) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid addr");
    }

    int64_t stakeAmount = params[1].get_int64();
    int64_t fees        = params.size() > 2 ? params[2].get_int64() : 0;
    if (fees < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid fees");
    }
    int32_t validHeight = chainActive.Tip()->nHeight;

    BalanceOpType stakeType = stakeAmount >= 0 ? BalanceOpType::STAKE : BalanceOpType::UNSTAKE;
    CFcoinStakeTx tx(*pUserId, validHeight, fees, stakeType, std::abs(stakeAmount));
    return SubmitTx(*pUserId, tx);
}

/*************************************************<< CDP >>**************************************************/
Value submitstakecdptx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 3 || params.size() > 6) {
        throw runtime_error(
            "submitstakecdptx \"addr\" stake_amount collateral_ratio [\"cdp_id\"] [fee]\n"
            "\nsubmit a CDP Staking Tx.\n"
            "\nArguments:\n"
            "1. \"address\" : CDP staker's address\n"
            "2. \"stake_amount\":   (numeric, required) WICC coins to stake into the CDP, boosted by 10^8\n"
            "3. \"mint_amount\":    (numberic, required), WUSD amount to mint\n"
            "4. \"cdp_id\":         (string, optional) ID of existing CDP (tx hash of the first CDP Stake Tx)\n"
            "5. \"fee\":            (numeric, optional) fee pay for miner, default is 10000\n"
            "\nResult:\n"
            "\"txid\"               (string) The transaction id.\n"
            "\nExamples:\n" +
            HelpExampleCli("submitstakecdptx",
                           "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" 20000000000 3000000 "
                           "\"b850d88bf1bed66d43552dd724c18f10355e9b6657baeae262b3c86a983bee71\" 1000000\n") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("submitstakecdptx",
                           "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" 2000000000 3000000 "
                           "\"b850d88bf1bed66d43552dd724c18f10355e9b6657baeae262b3c86a983bee71\" 1000000\n"));
    }
    uint64_t stakeAmount = params[1].get_uint64();
    uint64_t mintAmount  = params[2].get_uint64();

    int validHeight = chainActive.Tip()->nHeight;
    uint64_t fee    = 0;
    uint256 cdpTxId;
    if (params.size() >= 4) {
        cdpTxId = uint256S(params[3].get_str());
    }
    if (params.size() == 5) {
        fee = params[4].get_uint64();  // real type, 0 if empty and thence minFee
    }

    auto cdpUid = CUserID::ParseUserId(params[0].get_str());
    if (!cdpUid) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid addr");
    }

    CCDPStakeTx tx(*cdpUid, fee, validHeight, cdpTxId, stakeAmount, mintAmount);
    return SubmitTx(*cdpUid, tx);
}

Value submitredeemcdptx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 4 || params.size() > 5) {
        throw runtime_error(
            "submitredeemcdptx \"addr\" \"cdp_id\" repay_amount redeem_amount [fee]\n"
            "\nsubmit a CDP Redemption Tx\n"
            "\nArguments:\n"
            "1. \"address\" : CDP redemptor's address\n"
            "2. \"cdp_id\": (string) ID of existing CDP (tx hash of the first CDP Stake Tx)\n"
            "3. \"repay_amount\": (numeric required) WUSD coins to stake into the CDP, boosted by 10^8\n"
            "4. \"redeem_amount\": (numeric required) WICC coins to stake into the CDP, boosted by 10^8\n"
            "5. \"fee\": (numeric, optional) fee pay for miner, default is 10000\n"
            "\nResult:\n"
            "\"txid\" (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitredeemcdptx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"b850d88bf1bed66d43552dd724c18f10355e9b6657baeae262b3c86a983bee71\"  20000000000 30000 1000000\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitredeemcdptx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"b850d88bf1bed66d43552dd724c18f10355e9b6657baeae262b3c86a983bee71\" 2000000000 30000 1000000\n")
        );
    }
    EnsureWalletIsUnlocked();

    auto cdpUid = CUserID::ParseUserId(params[0].get_str());
    if (!cdpUid) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid addr");
    }

    uint256 cdpTxId     = uint256S(params[1].get_str());
    uint64_t repayAmount = params[2].get_uint64();
    uint64_t redeemAmount = params[3].get_uint64();
    uint64_t fee        = 0;
    if (params.size() == 5) {
        fee = params[4].get_uint64();  // real type, 0 if empty and thence minFee
    }

    int validHeight = chainActive.Tip()->nHeight;

    CCDPRedeemTx tx(*cdpUid, fee, validHeight, cdpTxId, repayAmount, redeemAmount);
    return SubmitTx(*cdpUid, tx);
}

Value submitliquidatecdptx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 3 || params.size() > 4) {
        throw runtime_error(
            "submitliquidatecdptx \"addr\" liquidate_amount collateral_ratio [\"cdp_id\"] [interest] [fee]\n"
            "\nsubmit a CDP Liquidation Tx\n"
            "\nArguments:\n"
            "1. \"address\" : (string required) CDP liquidator's address\n"
            "2. \"cdp_id\": (string required) ID of existing CDP (tx hash of the first CDP Stake Tx)\n"
            "3. \"liquidate_amount\": (numeric required) WICC coins to stake into the CDP, including penalty fees, boosted by 10^8\n"
            "4. \"fee\": (numeric, optional) fee pay for miner, default is 10000\n"
            "\nResult:\n"
            "\"txid\" (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitliquidatecdptx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\"  \"b850d88bf1bed66d43552dd724c18f10355e9b6657baeae262b3c86a983bee71\" 20000000000 1000000\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitliquidatecdptx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"b850d88bf1bed66d43552dd724c18f10355e9b6657baeae262b3c86a983bee71\" 2000000000 1000000\n")
        );
    }

    auto cdpUid = CUserID::ParseUserId(params[0].get_str());
    if (!cdpUid) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid addr");
    }

    uint256 cdpTxId     = uint256S(params[1].get_str());
    uint64_t liquidateAmount = params[2].get_uint64();

    uint64_t fee        = 0;
     if (params.size() ==6 ) {
        fee = params[5].get_uint64();  // real type, 0 if empty and thence minFee
    }

    int validHeight = chainActive.Tip()->nHeight;
    CCDPLiquidateTx tx(*cdpUid, fee, validHeight, cdpTxId, liquidateAmount);
    return SubmitTx(*cdpUid, tx);
}

Value getmedianprice(const Array& params, bool fHelp){
    if (fHelp) {
        throw runtime_error(
            "getmedianprice \"coin_type\" \"asset_type\" [height]\n"
            "\nget current median price or query at specified height.\n"
            "\nArguments:\n"
            "1.\"height\": (numeric, optional), specified height. If not provide use the tip block height in chainActive\n\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("getmedianprice","")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("getmedianprice","")
        );
    }

    int height = chainActive.Tip()->nHeight;
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
    if (block.vptx.size() > 1 && block.vptx[1]->nTxType == BLOCK_PRICE_MEDIAN_TX) {
        map<CCoinPriceType, uint64_t> mapMedianPricePoints = ((CBlockPriceMedianTx*)block.vptx[1].get())->GetMedianPrice();
        for (auto &item : mapMedianPricePoints) {
            Object price;
            price.push_back(Pair("coin_type",   item.first.coinType));
            price.push_back(Pair("price_type",  item.first.priceType));
            price.push_back(Pair("price",       item.second));
            prices.push_back(price);
        }
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
            "getusercdp \"addr\" \"cdp_id\" [height]\n"
            "\nget account's cdp.\n"
            "\nArguments:\n"
            "1.\"addr\": (string, required) CDP owner addr\n"
            "2.\"cdb_id\": (string, optional) CDP TxId\n"
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

    Array cdps;

    if(params.size() > 1) {
        uint256 cdpTxId(uint256S(params[1].get_str()));
        CUserCDP cdp(txAccount.regid, cdpTxId);
        if (pCdMan->pCdpCache->GetCdp(cdp)) {
            cdps.push_back(cdp.ToJson());
        } else {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                            strprintf("The cdp not exists! cdpId=%s", params[1].get_str()));
        }
    }
    else {
        vector<CUserCDP> userCdps;
        if (pCdMan->pCdpCache->GetCdpList(txAccount.regid, userCdps)) {
            for (auto& cdp : userCdps) {
                cdps.push_back(cdp.ToJson());
            }
        }
    }

    Object obj;
    obj.push_back(Pair("cdp", cdps));
    return obj;
}

/*************************************************<< DEX >>**************************************************/
Value submitdexbuylimitordertx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 5 || params.size() > 6) {
        throw runtime_error(
            "submitdexbuylimitordertx \"addr\" \"coin_type\" \"asset_type\" asset_amount price [fee]\n"
            "\nsubmit a dex buy limit price order tx.\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) order owner address\n"
            "2.\"coin_type\": (string required) coin type to pay\n"
            "3.\"asset_type\": (string required), asset type to buy\n"
            "4.\"asset_amount\": (numeric, required) amount of target asset to buy\n"
            "5.\"price\": (numeric, required) bidding price willing to buy\n"
            "6.\"fee\": (numeric, optional) fee pay for miner, default is 10000\n"
            "\nResult:\n"
            "\"txid\" (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitdexbuylimitordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"WUSD\" \"WICC\" 1000000 200000000\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitdexbuylimitordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"WUSD\" \"WICC\" 1000000 200000000\n")
        );
    }

    EnsureWalletIsUnlocked();

    auto pUserId = CUserID::ParseUserId(params[0].get_str());
    if (!pUserId) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid addr");
    }

    CoinType coinType;
    if (!ParseCoinType(params[1].get_str(), coinType)) {
        throw JSONRPCError(RPC_COIN_TYPE_INVALID, "Invalid coin_type");
    }

    AssetType assetType;
    if (!ParseAssetType(params[2].get_str(), assetType)) {
        throw JSONRPCError(RPC_ASSET_TYPE_INVALID, "Invalid asset_type");
    }

    uint64_t assetAmount = AmountToRawValue(params[3]);
    uint64_t price = AmountToRawValue(params[4]);

    uint64_t fee = 0;
    if (params.size() > 5) {
        fee = params[5].get_uint64();
    }

    CAccount txAccount;
    if (!pCdMan->pAccountCache->GetAccount(*pUserId, txAccount)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                            strprintf("The account not exists! userId=%s", pUserId->ToString()));
    }
    assert(!txAccount.keyid.IsEmpty());

    // TODO: need to support fee coin type
    uint64_t amount = assetAmount;
    if (txAccount.GetToken("WICC").free_amount< amount + fee) {
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Account does not have enough coins");
    }

    int validHeight = chainActive.Height();
    CDEXBuyLimitOrderTx tx(*pUserId, validHeight, fee, coinType, assetType, assetAmount, price);
    return SubmitTx(*pUserId, tx);
}

Value submitdexselllimitordertx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 5 || params.size() > 6) {
        throw runtime_error(
            "submitdexselllimitordertx \"addr\" \"coin_type\" \"asset_type\" asset_amount price [fee]\n"
            "\nsubmit a dex buy limit price order tx.\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) order owner address\n"
            "2.\"coin_type\": (string required) coin type to pay\n"
            "3.\"asset_type\": (string required), asset type to buy\n"
            "4.\"asset_amount\": (numeric, required) amount of target asset to buy\n"
            "5.\"price\": (numeric, required) bidding price willing to buy\n"
            "6.\"fee\": (numeric, optional) fee pay for miner, default is 10000\n"
            "\nResult:\n"
            "\"txid\" (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitdexselllimitordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"WICC\" \"WUSD\" 1000000 200000000\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitdexselllimitordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"WICC\" \"WUSD\" 1000000 200000000\n")
        );
    }

    EnsureWalletIsUnlocked();

    // 1. addr
    auto pUserId = CUserID::ParseUserId(params[0].get_str());
    if (!pUserId) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid addr");
    }

    CoinType coinType;
    if (!ParseCoinType(params[1].get_str(), coinType)) {
        throw JSONRPCError(RPC_COIN_TYPE_INVALID, "Invalid coin_type");
    }

    AssetType assetType;
    if (!ParseAssetType(params[2].get_str(), assetType)) {
        throw JSONRPCError(RPC_ASSET_TYPE_INVALID, "Invalid asset_type");
    }

    uint64_t assetAmount = AmountToRawValue(params[3]);
    uint64_t price = AmountToRawValue(params[4]);

    uint64_t fee = 0;
    if (params.size() > 5) {
        fee = params[5].get_uint64();
    }

    CAccount txAccount;
    if (!pCdMan->pAccountCache->GetAccount(*pUserId, txAccount)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                            strprintf("The account not exists! userId=%s", pUserId->ToString()));
    }
    assert(!txAccount.keyid.IsEmpty());

    // TODO: need to support fee coin type
    if (txAccount.GetToken("WICC").free_amount < fee) {
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Account does not have enough WICC");
    }

    if (txAccount.GetToken("WUSD").free_amount < assetAmount) {
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Account does not have enough WUSD");
    }

    int validHeight = chainActive.Height();
    CDEXSellLimitOrderTx tx(*pUserId, validHeight, fee, coinType, assetType, assetAmount, price);
    return SubmitTx(*pUserId, tx);
}

Value submitdexbuymarketordertx(const Array& params, bool fHelp) {
     if (fHelp || params.size() < 4 || params.size() > 5) {
        throw runtime_error(
            "submitdexbuymarketordertx \"addr\" \"coin_type\" \"asset_type\" asset_amount [fee]\n"
            "\nsubmit a dex buy market price order tx.\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) order owner address\n"
            "2.\"coin_type\": (string required) coin type to pay\n"
            "3.\"asset_type\": (string required), asset type to buy\n"
            "4.\"coin_amount\": (numeric, required) amount of target coin to buy\n"
            "5.\"fee\": (numeric, optional) fee pay for miner, default is 10000\n"
            "\nResult:\n"
            "\"txid\" (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitdexbuymarketordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"WICC\" \"WUSD\" 200000000\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitdexbuymarketordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"WICC\" \"WUSD\" 200000000\n")
        );
    }

    EnsureWalletIsUnlocked();

    auto pUserId = CUserID::ParseUserId(params[0].get_str());
    if (!pUserId) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid addr");
    }

    CoinType coinType;
    if (!ParseCoinType(params[1].get_str(), coinType)) {
        throw JSONRPCError(RPC_COIN_TYPE_INVALID, "Invalid coin_type");
    }

    AssetType assetType;
    if (!ParseAssetType(params[2].get_str(), assetType)) {
        throw JSONRPCError(RPC_ASSET_TYPE_INVALID, "Invalid asset_type");
    }

    uint64_t coinAmount = AmountToRawValue(params[3]);
    uint64_t fee = 0;
    if (params.size() > 4) {
        fee = AmountToRawValue(params[4]);
    }

    CAccount txAccount;
    if (!pCdMan->pAccountCache->GetAccount(*pUserId, txAccount)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                            strprintf("The account not exists! userId=%s", pUserId->ToString()));
    }

    // TODO: need to support fee coin type
    uint64_t amount = coinAmount;
    if (txAccount.GetToken("WICC").free_amount< amount + fee) {
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Account does not have enough coins");
    }

    int validHeight = chainActive.Height();
    CDEXBuyMarketOrderTx tx(*pUserId, validHeight, fee, coinType, assetType, coinAmount);
    return SubmitTx(*pUserId, tx);
}

Value submitdexsellmarketordertx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 4 || params.size() > 5) {
        throw runtime_error(
            "submitdexsellmarketordertx \"addr\" \"coin_type\" \"asset_type\" asset_amount [fee]\n"
            "\nsubmit a dex sell market price order tx.\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) order owner address\n"
            "2.\"coin_type\": (string required) coin type to pay\n"
            "3.\"asset_type\": (string required), asset type to buy\n"
            "4.\"asset_amount\": (numeric, required) amount of target asset to buy\n"
            "5.\"fee\": (numeric, optional) fee pay for miner, default is 10000\n"
            "\nResult:\n"
            "\"txid\" (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitdexsellmarketordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"WUSD\" \"WICC\" 200000000\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitdexsellmarketordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"WUSD\" \"WICC\" 200000000\n")
        );
    }

    EnsureWalletIsUnlocked();

    // 1. addr
    auto pUserId = CUserID::ParseUserId(params[0].get_str());
    if (!pUserId) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid addr");
    }

    CoinType coinType;
    if (!ParseCoinType(params[1].get_str(), coinType)) {
        throw JSONRPCError(RPC_COIN_TYPE_INVALID, "Invalid coin_type");
    }

    AssetType assetType;
    if (!ParseAssetType(params[2].get_str(), assetType)) {
        throw JSONRPCError(RPC_ASSET_TYPE_INVALID, "Invalid asset_type");
    }

    uint64_t assetAmount = AmountToRawValue(params[3]);
    uint64_t fee = 0;
    if (params.size() > 4) {
        fee = params[4].get_uint64();
    }

    CAccount txAccount;
    if (!pCdMan->pAccountCache->GetAccount(*pUserId, txAccount)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                            strprintf("The account not exists! userId=%s", pUserId->ToString()));
    }
    assert(!txAccount.keyid.IsEmpty());

    // TODO: need to support fee coin type
    if (txAccount.GetToken("WICC").free_amount < fee) {
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Account does not have enough WICC");
    }

    if (txAccount.GetToken("WUSD").free_amount < assetAmount) {
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Account does not have enough WUSD");
    }

    int validHeight = chainActive.Height();
    CDEXSellMarketOrderTx tx(*pUserId, validHeight, fee, coinType, assetType, assetAmount);
    return SubmitTx(*pUserId, tx);
}

Value submitdexcancelordertx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 2 || params.size() > 3) {
        throw runtime_error(
            "submitdexcancelordertx \"addr\" \"txid\"\n"
            "\nsubmit a dex cancel order tx.\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) order owner address\n"
            "2.\"txid\": (string required) order tx want to cancel\n"
            "3.\"fee\": (numeric, optional) fee pay for miner, default is 10000\n"
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

    EnsureWalletIsUnlocked();

    auto pUserId = CUserID::ParseUserId(params[0].get_str());
    if (!pUserId) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid addr");
    }

    uint256 txid(uint256S(params[1].get_str()));

    uint64_t fee = 0;
    if (params.size() > 2) {
        fee = params[2].get_uint64();
    }

    int validHeight = chainActive.Height();
    CDEXCancelOrderTx tx(*pUserId, validHeight, fee, txid);
    return SubmitTx(*pUserId, tx);
}

static const Value& JsonFindValue(Value jsonObj, const string &name) {

    const Value& jsonValue = find_value(jsonObj.get_obj(), name);
    if (jsonValue.type() == null_type || jsonValue == null_type) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("field %s not found in json object", name));
    }
    return jsonValue;
}

Value submitdexsettletx(const Array& params, bool fHelp) {
     if (fHelp || params.size() < 2 || params.size() > 3) {
        throw runtime_error(
            "submitdexsettletx \"addr\" \"deal_items\"\n"
            "\nsubmit a dex settle tx.\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) settle owner address\n"
            "2.\"deal_items\": (string required) deal items in json format\n"
            " [\n"
            "   {\n"
            "      \"buy_order_txid\":\"txid\", (string, required) order txid of buyer\n"
            "      \"sell_order_txid\":\"txid\", (string, required) order txid of seller\n"
            "      \"deal_price\":n (numeric, required) deal price "
            "      \"deal_coin_amount\":n (numeric, required) deal amount of coin\n"
            "      \"deal_asset_amount\":n (numeric, required) deal amount of asset\n"
            "   }\n"
            "       ,...\n"
            " ]\n"
            "3.\"fee\": (numeric, optional) fee pay for miner, default is 10000\n"
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

    EnsureWalletIsUnlocked();

    // 1. addr
    auto pUserId = CUserID::ParseUserId(params[0].get_str());
    if (!pUserId) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid addr");
    }

    vector<DEXDealItem> dealItems;

    Array dealItemArray = params[1].get_array();

    for (auto dealItemObj : dealItemArray) {
        DEXDealItem dealItem;
        const Value& buy_order_txid = JsonFindValue(dealItemObj, "buy_order_txid");
        dealItem.buyOrderId.SetHex(buy_order_txid.get_str());
        const Value& sell_order_txid = find_value(dealItemObj.get_obj(), "sell_order_txid");
        dealItem.sellOrderId.SetHex(sell_order_txid.get_str());
        const Value& deal_price = JsonFindValue(dealItemObj, "deal_price");
        dealItem.dealPrice = AmountToRawValue(deal_price);
        const Value& deal_coin_amount = JsonFindValue(dealItemObj, "deal_coin_amount");
        dealItem.dealCoinAmount = AmountToRawValue(deal_coin_amount);
        const Value& deal_asset_amount = JsonFindValue(dealItemObj, "deal_asset_amount");
        dealItem.dealAssetAmount = AmountToRawValue(deal_asset_amount);
        dealItems.push_back(dealItem);
    }

    uint64_t fee = 0;
    if (params.size() > 2) {
        fee = params[2].get_uint64();
    }

    int validHeight = chainActive.Height();
    CDEXSettleTx tx(*pUserId, validHeight, fee, dealItems);
    return SubmitTx(*pUserId, tx);
}
