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
#include "tx/dextx.h"

Value submitpricefeedtx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 2 || params.size() > 4) {
        throw runtime_error(
            "submitpricefeedtx \"{addr}\" \"script_path\"\n"
            "\nsubmit a price feed tx.\n"
            "\nthe execution include registercontracttx and callcontracttx.\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) contract owner address from this wallet\n"
            "2.\"script_path\": (string required), the file path of the app script\n"
            "3.\"arguments\": (string, optional) contract method invoke content (Hex encode required)\n"
            "4.\"fee\": (numeric, optional) fee pay for miner, default is 110010000\n"
            "\nResult vm execute detail\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("vmexecutescript","\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"/tmp/lua/script.lua\"\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("vmexecutescript", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"/tmp/lua/script.lua\"\n"));
    }

    // TODO:
    return Object();
}

Value submitstakefcointx(const Array& params, bool fHelp);


Value submitdexbuylimitordertx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 5 || params.size() > 7) {
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

    EnsureWalletIsUnlocked();

    // 1. addr
    auto pUserId = CUserID::ParseUserId(params[0].get_str());
    if (!pUserId) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid addr");
    }
    
    CoinType coinType;
    if (ParseCoinType(params[0].get_str(), coinType)) {
        throw JSONRPCError(RPC_DEX_COIN_TYPE_INVALID, "Invalid coin_type");
    }

    CoinType assetType;
    if (ParseCoinType(params[1].get_str(), assetType)) {
        throw JSONRPCError(RPC_DEX_ASSET_TYPE_INVALID, "Invalid asset_type");
    }

    uint64_t assetAmount = AmountToRawValue(params[2]);
    uint64_t price = AmountToRawValue(params[3]);

    int64_t defaultFee = SysCfg().GetTxFee(); // default fee
    int64_t fee;
    if (params.size() > 4) {
        fee = AmountToRawValue(params[4]);
        if (fee < defaultFee) {
            throw JSONRPCError(RPC_INSUFFICIENT_FEE,
                               strprintf("Given fee(%ld) < Default fee (%ld)", fee, defaultFee));
        }   
    } else {
        fee = defaultFee;
    }

    CAccount txAccount;
    if (!pCdMan->pAccountCache->GetAccount(*pUserId, txAccount)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                            strprintf("The account not exists! userId=%s", pUserId->ToString()));        
    }
    assert(!txAccount.keyId.IsEmpty());

    // TODO: need to support fee coin type
    uint64_t amount = assetAmount;
    if (txAccount.GetFreeBcoins() < amount + fee) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Account does not have enough coins");
    }

    CUserID txUid;
    if (txAccount.RegIDIsMature()) {
        txUid = txAccount.regId;
    } else if(txAccount.pubKey.IsValid()) {
        txUid = txAccount.pubKey;
    } else{
        CPubKey txPubKey;
        if(!pWalletMain->GetPubKey(txAccount.keyId, txPubKey)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, 
                strprintf("Get pubKey from wallet failed! keyId=%s", txAccount.keyId.ToString()));
        }
        txUid = txPubKey;
    }

    int validHeight = chainActive.Height();
    CDEXBuyLimitOrderTx tx(txUid, validHeight, fee, coinType, assetType, assetAmount, price);

    if (!pWalletMain->Sign(txAccount.keyId, tx.ComputeSignatureHash(), tx.signature))
            throw JSONRPCError(RPC_WALLET_ERROR, "sign tx failed");

    std::tuple<bool, string> ret = pWalletMain->CommitTx(&tx);

    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));
    }

    Object obj;
    obj.push_back(Pair("hash", std::get<1>(ret)));
    return obj;
}

Value submitdexselllimitordertx(const Array& params, bool fHelp) {
    return Object(); // TODO:...
}

Value submitdexbuymarketordertx(const Array& params, bool fHelp) {
    return Object(); // TODO:...
}

Value submitdexsellmarketordertx(const Array& params, bool fHelp) {
    return Object(); // TODO:...
}

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