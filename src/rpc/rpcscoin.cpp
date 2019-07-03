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
            "1. \"pricefeeds\"    (string, required) A json array of pricefeeds\n"
            " [\n"
            "   {\n"
            "      \"coin\": \"WICC|WGRT\", (string, required) The coin type\n"
            "      \"currency\": \"USD|RMB\" (string, required) The currency type\n"
            "      \"price\": (number, required) The price (boosted by 10^) \n"
            "   }\n"
            "       ,...\n"
            " ]\n"
            "2.\"fee\": (numeric, optional) fee pay for miner, default is 10000\n"
            "\nResult pricefeed tx result\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("submitpricefeedtx", 'WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH' \"[{\"coin\", WICC, \"currency\": USD, \"price\": 0.28}]\"\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitpricefeedtx","\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"[{\"coin\", WICC, \"currency\": USD, \"price\": 0.28}]\"\n"));
    }

    // TODO:
    return Object();
}

Value submitstakefcointx(const Array& params, bool fHelp) {

    return Object();
}
Value submitdexbuyordertx(const Array& params, bool fHelp) {
    return Object();
}
Value submitdexsellordertx(const Array& params, bool fHelp) {
    return Object();
}
Value submitdexcancelordertx(const Array& params, bool fHelp) {
    return Object();
}
Value submitdexsettletx(const Array& params, bool fHelp) {
    return Object();
}
Value submitstakecdptx(const Array& params, bool fHelp) {
    return Object();
}
Value submitredeemcdptx(const Array& params, bool fHelp) {
    return Object();
}
Value submitliquidatecdptx(const Array& params, bool fHelp) {
    return Object();
}
Value getmedianprice(const Array& params, bool fHelp) {
    return Object();
}
Value listcdps(const Array& params, bool fHelp) {
    return Object();
}
Value listcdpstoliquidate(const Array& params, bool fHelp) {
    return Object();
}