// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcscoin.h"

#include "commons/base58.h"
#include "rpc/core/rpcserver.h"
#include "init.h"
#include "net.h"
#include "miner/miner.h"
#include "util.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"

Value submitpricefeedtx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 2 || params.size() > 4) {
        throw runtime_error(
            "submitpricefeedtx \"$pricefeeds\" \n"
            "\nsubmit a price feed tx.\n"
            "\nthe execution include registercontracttx and callcontracttx.\n"
            "\nArguments:\n"
            "1. \"address\" : Price Feeder's address\n"
            "2. \"pricefeeds\"    (string, required) A json array of pricefeeds\n"
            " [\n"
            "   {\n"
            "      \"coin\": \"WICC|WGRT\", (string, required) The coin type\n"
            "      \"currency\": \"USD|RMB\" (string, required) The currency type\n"
            "      \"price\": (number, required) The price (boosted by 10^) \n"
            "   }\n"
            "       ,...\n"
            " ]\n"
            "3.\"fee\": (numeric, optional) fee pay for miner, default is 10000\n"
            "\nResult pricefeed tx result\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("submitpricefeedtx", 'WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH' '[{\"coin\", WICC, \"currency\": \"USD\", \"price\": 0.28}]'\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitpricefeedtx","\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"[{\"coin\", WICC, \"currency\": \"USD\", \"price\": 0.28}]\"\n"));
    }

    RPCTypeCheck(params, list_of(array_type)(int_type);
    EnsureWalletIsUnlocked();

    CPriceFeedTx()


    CKeyID feedKid;
    if (!GetKeyId(params[0].get_str(), feedKeyId))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid PriceFeeder address");
    }
    CPubKey feedPubKey;
    if (!pWalletMain->GetPubKey(feedKeyId, feedPubKey)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Key not found in the local wallet.");
    }
    CUserID feedUid;
    CRegID feedRegId;
    feedUid = ( pCdMan->pAccountCache->GetRegId(CUserID(feedKeyId), feedRegId) &&
                pCdMan->pAccountCache->RegIDIsMature(feedRegId))
                    ? CUserID(feedRegId)
                    : CUserID(feedPubKey);

    Array arrPricePoints   = params[1].get_array();
    uint64_t fee        = params[2].get_uint64();  // real type

    int validHeight = chainActive.Tip()->nHeight;
    Vector<CPricePoint> pricePoints;
    for (auto objPp : arrPricePoints) {
        const Value& coinValue = find_value(objPp.get_obj(), "coin");
        const Value& currencyValue = find_value(objPp.get_obj(), "currency");
        const Value& priceValue = find_value(objPp.get_obj(), "price");
        if (    coinValue.type() == null_type
            ||  currencyValue.type() == null_type
            ||  priceValue.type() == null_type ) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "null tpye not allowed!");
        }
    }

    CPriceFeedTx tx(feedUid, validHeight, fee)

     if (!pWalletMain->Sign(sendKeyId, tx.ComputeSignatureHash(), tx.signature)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
    }

    std::tuple<bool, string> ret = pWalletMain->CommitTx((CBaseTx*)&tx);
    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));
    }

    Object obj;
    obj.push_back(Pair("tx_hash", std::get<1>(ret)));
    return obj;
}

Value submitstakefcointx(const Array& params, bool fHelp);


Value submitdexbuylimitordertx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 2 || params.size() > 4) {
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
            "\nResult detail\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("submitdexbuylimitordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"WUSD\" \"WICC\" 1000000 200000000\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitdexbuylimitordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"WUSD\" \"WICC\" 1000000 200000000\n")
        );
    }
    // TODO: ...
    return Object();

}

Value submitdexselllimitordertx(const Array& params, bool fHelp);
Value submitdexbuymarketordertx(const Array& params, bool fHelp);
Value submitdexsellmarketordertx(const Array& params, bool fHelp);

Value submitdexcancelordertx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 2 || params.size() > 4) {
        throw runtime_error(
            "submitdexcancelordertx \"addr\" \"txid\"\n"
            "\nsubmit a dex cancel order tx.\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) order owner address\n"
            "2.\"txid\": (string required) txid of order tx want to cancel\n"
            "3.\"fee\": (numeric, optional) fee pay for miner, default is 10000\n"
            "\nResult detail\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("submitdexcancelordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" "
                             "\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\" ")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitdexcancelordertx", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" "\
                             "\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\"")
        );
    }
    // TODO: ...
    return Object();
}
Value submitdexsettletx(const Array& params, bool fHelp) {
     if (fHelp || params.size() < 2 || params.size() > 4) {
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
            "\nResult detail\n"
            "\nResult:\n"
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
    // TODO: ...
    return Object();
}

Value submitstakecdptx(const Array& params, bool fHelp);
Value submitredeemcdptx(const Array& params, bool fHelp);
Value submitliquidatecdptx(const Array& params, bool fHelp);
Value getmedianprice(const Array& params, bool fHelp);
Value listcdps(const Array& params, bool fHelp);
Value listcdpstoliquidate(const Array& params, bool fHelp);
