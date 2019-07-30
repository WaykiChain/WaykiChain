// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The WaykiChain developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/foreach.hpp>
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>

#include "config/chainparams.h"
#include "init.h"
#include "main.h"
#include "miner/miner.h"
//#include "net.h"
#include "rpc/core/rpcprotocol.h"
#include "rpc/core/rpcserver.h"
#include "commons/serialize.h"
#include "sync.h"
#include "tx/txmempool.h"
#include "commons/uint256.h"
#include "commons/util.h"
#include "config/version.h"

#include "../wallet/wallet.h"

#include <stdint.h>

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace json_spirit;
using namespace std;

static bool fMining = false;

void SetMinerStatus(bool bStatus) { fMining = bStatus; }
static bool GetMiningInfo() { return fMining; }

Value setgenerate(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 1 && params.size() != 2))
        throw runtime_error(
            "setgenerate generate ( genblocklimit )\n"
            "\nSet 'generate' true or false to turn generation on or off.\n"
            "Generation is limited to 'genblocklimit' processors, -1 is unlimited.\n"
            "See the getgenerate call for the current setting.\n"
            "\nArguments:\n"
            "1. generate         (boolean, required) Set to true to turn on generation, off to turn off.\n"
            "2. genblocklimit     (numeric, optional) Set the processor limit for when generation is on. Can be -1 for unlimited.\n"
            "                    Note: in -regtest mode, genblocklimit controls how many blocks are generated immediately.\n"
            "\nExamples:\n"
            "\nSet the generation on with a limit of one processor\n"
            + HelpExampleCli("setgenerate", "true 1")
            + HelpExampleRpc("setgenerate", "true, 1")
            + "\nTurn off generation\n"
            + HelpExampleCli("setgenerate", "false")
            + HelpExampleRpc("setgenerate", "false"));

    static bool fGenerate = false;

    set<CKeyID> setKeyId;
    setKeyId.clear();
    pWalletMain->GetKeys(setKeyId, true);

    bool bSetEmpty(true);
    for (auto & keyId : setKeyId) {
        CUserID userId(keyId);
        CAccount acctInfo;
        if (pCdMan->pAccountCache->GetAccount(userId, acctInfo)) {
            bSetEmpty = false;
            break;
        }
    }

    if (bSetEmpty)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "No key for mining");

    if (params.size() > 0)
        fGenerate = params[0].get_bool();

    int genBlockLimit = 1;
    if (params.size() == 2) {
        genBlockLimit = params[1].get_int();
        if(genBlockLimit <= 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid genblocklimit");
        }
    }
    Object obj;
    if (fGenerate == false){
        GenerateCoinBlock(false, pWalletMain, 1);

        obj.push_back(Pair("msg", "stoping  mining"));
        return obj;
    }

    GenerateCoinBlock(true, pWalletMain, genBlockLimit);
    obj.push_back(Pair("msg", "in  mining"));
    return obj;
}

Value getmininginfo(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error("getmininginfo\n"
            "\nReturns a json object containing mining-related information."
            "\nResult:\n"
            "{\n"
            "  \"blocks\": nnn,             (numeric) The current block\n"
            "  \"currentblocksize\": nnn,   (numeric) The last block size\n"
            "  \"currentblocktx\": nnn,     (numeric) The last block transaction\n"
            "  \"errors\": \"...\"          (string) Current errors\n"
            "  \"generate\": true|false     (boolean) If the generation is on or off (see getgenerate or setgenerate calls)\n"
            "  \"pooledtx\": n              (numeric) The size of the mem pool\n"
            "  \"testnet\": true|false      (boolean) If using testnet or not\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getmininginfo", "")
            + HelpExampleRpc("getmininginfo", "")
        );
    }

    static const string NetTypes[] = { "MAIN_NET", "TEST_NET", "REGTEST_NET" };

    Object obj;
    obj.push_back(Pair("blocks",           chainActive.Height()));
    obj.push_back(Pair("currentblocksize", (uint64_t)nLastBlockSize));
    obj.push_back(Pair("currentblocktx",   (uint64_t)nLastBlockTx));
    obj.push_back(Pair("errors",           GetWarnings("statusbar")));
    obj.push_back(Pair("genblocklimit",    1));
    obj.push_back(Pair("pooledtx",         (uint64_t)mempool.Size()));
    obj.push_back(Pair("nettype",          NetTypes[SysCfg().NetworkID()]));
    obj.push_back(Pair("posmaxnonce",      (int32_t)SysCfg().GetBlockMaxNonce()));
    obj.push_back(Pair("generate",         GetMiningInfo()));
    return obj;
}

Value submitblock(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error("submitblock \"hexdata\" ( \"jsonparametersobject\" )\n"
            "\nAttempts to submit new block to network.\n"
            "The 'jsonparametersobject' parameter is currently ignored.\n"
            "See https://en.bitcoin.it/wiki/BIP_0022 for full specification.\n"

            "\nArguments\n"
            "1. \"hexdata\"    (string, required) the hex-encoded block data to submit\n"
            "2. \"jsonparametersobject\"     (string, optional) object of optional parameters\n"
            "    {\n"
            "      \"workid\" : \"id\"    (string, optional) if the server provided a workid, it MUST be included with submissions\n"
            "    }\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("submitblock", "\"mydata\"")
            + HelpExampleRpc("submitblock", "\"mydata\"")
        );

    vector<unsigned char> blockData(ParseHex(params[0].get_str()));
    CDataStream ssBlock(blockData, SER_NETWORK, PROTOCOL_VERSION);
    CBlock pBlock;
    try {
        ssBlock >> pBlock;
    }
    catch (std::exception &e) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Block decode failed");
    }

    CValidationState state;
    bool fAccepted = ProcessBlock(state, NULL, &pBlock);
    Object obj;
    if (!fAccepted) {
        obj.push_back(Pair("status",        "rejected"));
        obj.push_back(Pair("reject_code",   state.GetRejectCode()));
        obj.push_back(Pair("info",          state.GetRejectReason()));
    } else {

        obj.push_back(Pair("status",        "OK"));
        obj.push_back(Pair("txid",          pBlock.GetHash().ToString()));
    }
    return obj;
}


Value getminedblocks(const Array& params, bool fHelp) {
    if (fHelp || params.size() > 1) {
        throw runtime_error("getminedblocks\n"
            "\nReturns a json array containing the blocks mined by this node."
            "\nArguments:\n"
            "1. count         (numeric, optional) If provided, get the specified count blocks,\n"
            "                                     otherwise get all nodes.\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"time\": n               (numeric) block time\n"
            "    \"nonce\": n              (numeric) block nonce\n"
            "    \"height\": n             (numeric) block height\n"
            "    \"totalfuels\": n         (numeric) the total fuels of all transactions in the block\n"
            "    \"fuelrate\": n           (numeric) block fuel rate\n"
            "    \"totalfees\": n          (numeric) the total fees of all transactions in the block\n"
            "    \"reward\": n             (numeric) block reward for miner\n"
            "    \"txcount\": n            (numeric) transaction count in block, exclude coinbase\n"
            "    \"blocksize\": n          (numeric) block size (bytes)\n"
            "    \"hash\": xxx             (string) block hash\n"
            "    \"preblockhash\": xxx     (string) pre block hash\n"
            "  }\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getminedblocks", "")
            + HelpExampleRpc("getminedblocks", "")
        );
    }

    unsigned int count = (unsigned int)-1;
    if (params.size() == 1) {
        count = params[0].get_uint64();
    }

    Array ret;

    auto minedBlocks = GetMinedBlocks(count);
    for ( auto &blockInfo : minedBlocks) {
        Object obj;
        obj.push_back(Pair("time",          blockInfo.time));
        obj.push_back(Pair("nonce",         blockInfo.nonce));
        obj.push_back(Pair("height",        blockInfo.height));
        obj.push_back(Pair("total_fuels",   blockInfo.totalFuel));
        obj.push_back(Pair("fuel_rate",     (int32_t)blockInfo.fuelRate));
        obj.push_back(Pair("total_fees",    blockInfo.totalFees));
        obj.push_back(Pair("tx_count",      blockInfo.txCount));
        obj.push_back(Pair("block_size",    blockInfo.totalBlockSize));
        obj.push_back(Pair("txid",          blockInfo.hash.ToString()));
        obj.push_back(Pair("preblockhash",  blockInfo.hashPrevBlock.ToString()));
        ret.push_back(obj);
    }

    return ret;
}
