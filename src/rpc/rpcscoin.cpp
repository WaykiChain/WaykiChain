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
Value submitdexbuyordertx(const Array& params, bool fHelp);
Value submitdexsellordertx(const Array& params, bool fHelp);
Value submitdexcancelordertx(const Array& params, bool fHelp);
Value submitdexsettletx(const Array& params, bool fHelp);
Value submitstakecdptx(const Array& params, bool fHelp);
Value submitredeemcdptx(const Array& params, bool fHelp);
Value submitliquidatecdptx(const Array& params, bool fHelp);
Value getmedianprice(const Array& params, bool fHelp);
Value listcdps(const Array& params, bool fHelp);
Value listcdpstoliquidate(const Array& params, bool fHelp);