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

#include "chainparams.h"
#include "core.h"
#include "init.h"
#include "main.h"
#include "miner.h"
//#include "net.h"
#include "rpcprotocol.h"
#include "rpcserver.h"
#include "serialize.h"
#include "sync.h"
#include "txmempool.h"
#include "uint256.h"
#include "util.h"
#include "version.h"

#include "../wallet/wallet.h"

#include <stdint.h>

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace json_spirit;
using namespace std;


// Key used by getwork miners.
// Allocated in InitRPCMining, free'd in ShutdownRPCMining
//static CReserveKey* pMiningKey = NULL;

void InitRPCMining()
{
    if (!pwalletMain)
        return;

    // getwork/getblocktemplate mining rewards paid here:
//    pMiningKey = new CReserveKey(pwalletMain);
     //assert(0);
     LogPrint("TODO","InitRPCMining");
}

void ShutdownRPCMining()
{
//    if (!pMiningKey)
//        return;
//
//    delete pMiningKey; pMiningKey = NULL;
}


// Return average network hashes per second based on the last 'lookup' blocks,
// or from the last difficulty change if 'lookup' is nonpositive.
// If 'height' is nonnegative, compute the estimate at the time when a given block was found.
Value GetNetworkHashPS(int lookup, int height) {
    CBlockIndex *pb = chainActive.Tip();

    if (height >= 0 && height < chainActive.Height())
        pb = chainActive[height];

    if (pb == NULL || !pb->nHeight)
        return 0;

    // If lookup is -1, then use blocks since last difficulty change.
    if (lookup <= 0)
        lookup = pb->nHeight % 2016 + 1;

    // If lookup is larger than chain, then set it to chain length.
    if (lookup > pb->nHeight)
        lookup = pb->nHeight;

    CBlockIndex *pb0 = pb;
    int64_t minTime = pb0->GetBlockTime();
    int64_t maxTime = minTime;
    for (int i = 0; i < lookup; i++) {
        pb0 = pb0->pprev;
        int64_t time = pb0->GetBlockTime();
        minTime = min(time, minTime);
        maxTime = max(time, maxTime);
    }

    // In case there's a situation where minTime == maxTime, we don't want a divide by zero exception.
    if (minTime == maxTime)
        return 0;

    arith_uint256 workDiff = pb->nChainWork - pb0->nChainWork;
    int64_t timeDiff = maxTime - minTime;

    return (int64_t)(workDiff.getdouble() / timeDiff);
}

Value getnetworkhashps(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "getnetworkhashps ( blocks height )\n"
            "\nReturns the estimated network hashes per second based on the last n blocks.\n"
            "Pass in [blocks] to override # of blocks, -1 specifies since last difficulty change.\n"
            "Pass in [height] to estimate the network speed at the time when a certain block was found.\n"
            "\nArguments:\n"
            "1. blocks     (numeric, optional, default=120) The number of blocks, or -1 for blocks since last difficulty change.\n"
            "2. height     (numeric, optional, default=-1) To estimate at the time of the given height.\n"
            "\nResult:\n"
            "x             (numeric) Hashes per second estimated\n"
            "\nExamples:\n"
            + HelpExampleCli("getnetworkhashps", "")
            + HelpExampleRpc("getnetworkhashps", "")
       );

    return GetNetworkHashPS(params.size() > 0 ? params[0].get_int() : 120, params.size() > 1 ? params[1].get_int() : -1);
}

static bool IsMining = false;

void SetMinerStatus(bool bStatus) {
    IsMining = bStatus;
}

static bool GetMiningInfo() {
    return IsMining;
}

Value setgenerate(const Array& params, bool fHelp)
{
    if (fHelp || (params.size() != 1 && params.size() != 2))
        throw runtime_error(
            "setgenerate generate ( genproclimit )\n"
            "\nSet 'generate' true or false to turn generation on or off.\n"
            "Generation is limited to 'genproclimit' processors, -1 is unlimited.\n"
            "See the getgenerate call for the current setting.\n"
            "\nArguments:\n"
            "1. generate         (boolean, required) Set to true to turn on generation, off to turn off.\n"
            "2. genproclimit     (numeric, optional) Set the processor limit for when generation is on. Can be -1 for unlimited.\n"
            "                    Note: in -regtest mode, genproclimit controls how many blocks are generated immediately.\n"
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
    pwalletMain->GetKeys(setKeyId, true);

    bool bSetEmpty(true);
    for (auto & keyId : setKeyId) {
        CUserID userId(keyId);
        CAccount acctInfo;
        if (pAccountViewTip->GetAccount(userId, acctInfo)) {
            bSetEmpty = false;
            break;
        }
    }

    if (bSetEmpty)
        throw JSONRPCError(RPC_INVALID_PARAMS, "no key for mining");

    if (params.size() > 0)
        fGenerate = params[0].get_bool();

    int nGenProcLimit = 1;
    if (params.size() == 2) {
        nGenProcLimit = params[1].get_int();
        if(nGenProcLimit <= 0) {
            throw JSONRPCError(RPC_INVALID_PARAMS, "limit conter err for mining");
        }
    }
    Object obj;
    if (fGenerate == false){
        GenerateCoinBlock(false, pwalletMain, 1);

        obj.push_back(Pair("msg", "stoping  mining"));
        return obj;
    }

    GenerateCoinBlock(true, pwalletMain, nGenProcLimit);
    obj.push_back(Pair("msg", "in  mining"));
    return obj;
}

Value getmininginfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0) {
        throw runtime_error("getmininginfo\n"
            "\nReturns a json object containing mining-related information."
            "\nResult:\n"
            "{\n"
            "  \"blocks\": nnn,             (numeric) The current block\n"
            "  \"currentblocksize\": nnn,   (numeric) The last block size\n"
            "  \"currentblocktx\": nnn,     (numeric) The last block transaction\n"
            "  \"difficulty\": xxx.xxxxx    (numeric) The current difficulty\n"
            "  \"errors\": \"...\"          (string) Current errors\n"
            "  \"generate\": true|false     (boolean) If the generation is on or off (see getgenerate or setgenerate calls)\n"
            "  \"genproclimit\": n          (numeric) The processor limit for generation. -1 if no generation. (see getgenerate or setgenerate calls)\n"
            "  \"hashespersec\": n          (numeric) The hashes per second of the generation, or 0 if no generation.\n"
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
    obj.push_back(Pair("blocks",           (int)chainActive.Height()));
    obj.push_back(Pair("currentblocksize", (uint64_t)nLastBlockSize));
    obj.push_back(Pair("currentblocktx",   (uint64_t)nLastBlockTx));
    obj.push_back(Pair("errors",           GetWarnings("statusbar")));
    obj.push_back(Pair("genproclimit",     1));
    obj.push_back(Pair("networkhashps",    getnetworkhashps(params, false)));
    obj.push_back(Pair("pooledtx",         (uint64_t)mempool.size()));
    obj.push_back(Pair("nettype",          NetTypes[SysCfg().NetworkID()]));
    obj.push_back(Pair("posmaxnonce",      SysCfg().GetBlockMaxNonce()));
    obj.push_back(Pair("generate",         GetMiningInfo()));
    return obj;
}

Value submitblock(const Array& params, bool fHelp)
{
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
    CBlock pblock;
    try {
        ssBlock >> pblock;
    }
    catch (std::exception &e) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Block decode failed");
    }

    CValidationState state;
    bool fAccepted = ProcessBlock(state, NULL, &pblock);
    Object obj;
    if (!fAccepted) {
        obj.push_back(Pair("status", "rejected"));
        obj.push_back(Pair("reject code", state.GetRejectCode()));
        obj.push_back(Pair("info", state.GetRejectReason()));
    } else {

        obj.push_back(Pair("status", "OK"));
        obj.push_back(Pair("hash", pblock.GetHash().ToString()));
    }
    return obj;
}

    int64_t         nTime;              // block time
    int64_t         nNonce;             // nonce
    int             nHeight;            // block height
    int64_t         nTotalFuels;        // the total fuels of all transactions in the block
    int             nFuelRate;          // the fuel rate
    int64_t         nTotalFees;         // the total fees of all transactions in the block
    uint64_t        nTxCount;           // transaction count in block, exclude coinbase
    uint64_t        nBlockSize;         // block size(bytes)
    uint256         hash;               // block hash
    uint256         hashPrevBlock;      // prev block hash


Value getminedblocks(const Array& params, bool fHelp)
{
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
        obj.push_back(Pair("time",          blockInfo.nTime));
        obj.push_back(Pair("nonce",         blockInfo.nNonce));
        obj.push_back(Pair("height",        blockInfo.nHeight));
        obj.push_back(Pair("totalfuels",    blockInfo.nTotalFuels));
        obj.push_back(Pair("fuelrate",      blockInfo.nFuelRate));
        obj.push_back(Pair("totalfees",     blockInfo.nTotalFees));
        obj.push_back(Pair("reward",        blockInfo.GetReward()));
        obj.push_back(Pair("txcount",       blockInfo.nTxCount));
        obj.push_back(Pair("blocksize",     blockInfo.nBlockSize));
        obj.push_back(Pair("hash",          blockInfo.hash.ToString()));
        obj.push_back(Pair("preblockhash",  blockInfo.hashPrevBlock.ToString()));
        ret.push_back(obj);
    }

    return ret;
}
