// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <stdint.h>
#include <boost/assign/list_of.hpp>

#include "commons/messagequeue.h"
#include "commons/uint256.h"
#include "config/configuration.h"
#include "init.h"
#include "json/json_spirit_value.h"
#include "main.h"
#include "rpc/core/rpcserver.h"
#include "sync.h"
#include "tx/merkletx.h"
#include "tx/tx.h"
#include "wallet/wallet.h"

using namespace json_spirit;
using namespace std;

class CBaseCoinTransferTx;

double GetDifficulty(const CBlockIndex* pBlockIndex) {
    // Floating point number that is a multiple of the minimum difficulty,
    // minimum difficulty = 1.0.
    if (pBlockIndex == NULL) {
        if (chainActive.Tip() == NULL)
            return 1.0;
        else
            pBlockIndex = chainActive.Tip();
    }

    int nShift = (pBlockIndex->nBits >> 24) & 0xff;

    double dDiff = (double)0x0000ffff / (double)(pBlockIndex->nBits & 0x00ffffff);

    while (nShift < 29) {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29) {
        dDiff /= 256.0;
        nShift--;
    }

    return dDiff;
}

Object BlockToJSON(const CBlock& block, const CBlockIndex* pBlockIndex) {
    Object result;
    result.push_back(Pair("block_hash",     block.GetHash().GetHex()));
    CMerkleTx txGen(block.vptx[0]);
    txGen.SetMerkleBranch(&block);
    result.push_back(Pair("confirmations",  (int32_t)txGen.GetDepthInMainChain()));
    result.push_back(Pair("size",           (int32_t)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION)));
    result.push_back(Pair("height",         (int32_t)block.GetHeight()));
    result.push_back(Pair("version",        block.GetVersion()));
    result.push_back(Pair("merkle_root",    block.GetMerkleRootHash().GetHex()));
    result.push_back(Pair("tx_count",       (int32_t)block.vptx.size()));
    Array txs;
    for (const auto& ptx : block.vptx)
        txs.push_back(ptx->GetHash().GetHex());
    result.push_back(Pair("tx",             txs));
    result.push_back(Pair("time",           block.GetBlockTime()));
    result.push_back(Pair("nonce",          (uint64_t)block.GetNonce()));
    // TODO: Fees
    // CBlockRewardTx* pBlockRewardTx = (CBlockRewardTx*)block.vptx[0].get();
    // uint64_t reward           = pBlockRewardTx->reward;
    // int64_t fees                   = block.GetFees();
    // int64_t fuel                   = block.GetFuel();
    // uint64_t profits               = reward - (fees - fuel);
    // result.push_back(Pair("fuel", (int)block.GetFuel()));
    // result.push_back(Pair("fuel_rate", block.GetFuelRate()));
    // result.push_back(Pair("profits", profits));
    // result.push_back(Pair("fees", fees));
    if (pBlockIndex->pprev)
        result.push_back(Pair("previous_block_hash", pBlockIndex->pprev->GetBlockHash().GetHex()));
    CBlockIndex* pNext = chainActive.Next(pBlockIndex);
    if (pNext)
        result.push_back(Pair("next_block_hash", pNext->GetBlockHash().GetHex()));

    Array prices;
    for (auto &item : block.GetBlockMedianPrice()) {
        Object price;
        price.push_back(Pair("coin_symbol",   item.first.first));
        price.push_back(Pair("price_symbol",  item.first.second));
        price.push_back(Pair("price",         (double)item.second/kPercentBoost));
        prices.push_back(price);
    }
    result.push_back(Pair("median_price", prices));

    return result;
}

Value getblockcount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getblockcount\n"
            "\nReturns the number of blocks in the longest chain.\n"
            "\nResult:\n"
            "\n    (numeric) The current block count\n"
            "\nExamples:\n"
            + HelpExampleCli("getblockcount", "")
            + HelpExampleRpc("getblockcount", "")
        );

    return chainActive.Height();
}

Value getbestblockhash(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getbestblockhash\n"
            "\nReturns the hash of the best (tip) block in the longest block chain.\n"
            "\nResult\n"
            "\"hex\"      (string) the block hash hex encoded\n"
            "\nExamples\n"
            + HelpExampleCli("getbestblockhash", "")
            + HelpExampleRpc("getbestblockhash", "")
        );

    return chainActive.Tip()->GetBlockHash().GetHex();
}

Value getdifficulty(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getdifficulty\n"
            "\nReturns the proof-of-work difficulty as a multiple of the minimum difficulty.\n"
            "\nResult:\n"
            "n.nnn       (numeric) the proof-of-work difficulty as a multiple of the minimum difficulty.\n"
            "\nExamples:\n"
            + HelpExampleCli("getdifficulty", "")
            + HelpExampleRpc("getdifficulty", "")
        );

    return 0;//GetDifficulty();
}

Value getrawmempool(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getrawmempool ( verbose )\n"
            "\nReturns all transaction ids in memory pool as a json or an array of string transaction ids.\n"
            "\nArguments:\n"
            "1. verbose           (boolean, optional, default=false) true for a json object, false for array of transaction ids\n"
            "\nResult: (for verbose = false):\n"
            "[                     (json array of string)\n"
            "  \"txid\"     (string) The transaction id\n"
            "  ,...\n"
            "]\n"
            "\nResult: (for verbose = true):\n"
            "{                           (json object)\n"
            "  \"txid\" : {       (json object)\n"
            "    \"fee\" : n,              (numeric) transaction fee in WICC coins\n"
            "    \"size\" : n,             (numeric) transaction size in bytes\n"
            "    \"priority\" : n,         (numeric) priority\n"
            "    \"time\" : n,             (numeric) local time transaction entered pool in seconds since 1 Jan 1970 GMT\n"
            "    \"height\" : n,           (numeric) block height when transaction entered pool\n"
            "  }, ...\n"
            "]\n"
            "\nExamples\n"
            + HelpExampleCli("getrawmempool", "")
            + HelpExampleCli("getrawmempool", "false")
            + HelpExampleRpc("getrawmempool", "true")
        );

    bool fVerbose = false;
    if (params.size() > 0)
        fVerbose = params[0].get_bool();

    if (fVerbose) {
        LOCK(mempool.cs);
        Object obj;
        for (const auto& entry : mempool.memPoolTxs) {
            const uint256& hash      = entry.first;
            const CTxMemPoolEntry& e = entry.second;
            Object info;
            info.push_back(Pair("size",         (int)e.GetTxSize()));
            info.push_back(Pair("fees_type",    std::get<0>(e.GetFees())));
            info.push_back(Pair("fees",         ValueFromAmount(std::get<1>(e.GetFees()))));
            info.push_back(Pair("time",         e.GetTime()));
            info.push_back(Pair("height",       (int)e.GetHeight()));
            info.push_back(Pair("priority",     e.GetPriority()));

            obj.push_back(Pair(hash.ToString(), info));
        }
        return obj;
    } else {
        vector<uint256> txids;
        mempool.QueryHash(txids);

        Array arr;
        for (const auto& hash : txids) {
            arr.push_back(hash.ToString());
        }

        return arr;
    }
}

Value getblockhash(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1) {
        throw runtime_error("getblockhash index\n"
            "\nReturns hash of block in best-block-chain at index provided.\n"
            "\nArguments:\n"
            "1. height         (numeric, required) The block height\n"
            "\nResult:\n"
            "\"hash\"         (string) The block hash\n"
            "\nExamples:\n"
            + HelpExampleRpc("getblockhash", "1000"));
    }

    RPCTypeCheck(params, boost::assign::list_of(int_type));

    int height = params[0].get_int();
    if (height < 0 || height > chainActive.Height())
        throw runtime_error("Block number out of range");

    CBlockIndex* pBlockIndex = chainActive[height];
    Object result;
    result.push_back(Pair("txid", pBlockIndex->GetBlockHash().GetHex()));
    return result;
}

Value getblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2) {
        throw runtime_error("getblock \"hash or height\" ( verbose )\n"
            "\nIf verbose is false, returns a string that is serialized, hex-encoded data for block 'hash'.\n"
            "If verbose is true, returns an Object with information about block <hash>.\n"
            "\nArguments:\n"
            "1. \"hash or height\"(string or numeric,required) string for the block hash, or numeric for the block height\n"
            "2. verbose           (boolean, optional, default=true) true for a json object, false for the hex encoded data\n"
            "\nResult (for verbose = true):\n"
            "{\n"
            "  \"hash\" : \"hash\",     (string) the block hash (same as provided)\n"
            "  \"confirmations\" : n,   (numeric) The number of confirmations\n"
            "  \"size\" : n,            (numeric) The block size\n"
            "  \"height\" : n,          (numeric) The block height or index\n"
            "  \"version\" : n,         (numeric) The block version\n"
            "  \"merkleroot\" : \"xxxx\", (string) The merkle root\n"
            "  \"tx\" : [               (array of string) The transaction ids\n"
            "     \"txid\"     (string) The transaction id\n"
            "     ,...\n"
            "  ],\n"
            "  \"time\" : ttt,          (numeric) The block time in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"nonce\" : n,           (numeric) The nonce\n"
            "  \"bits\" : \"1d00ffff\", (string) The bits\n"
            "  \"difficulty\" : x.xxx,  (numeric) The difficulty\n"
            "  \"previousblockhash\" : \"hash\",  (string) The hash of the previous block\n"
            "  \"nextblockhash\" : \"hash\"       (string) The hash of the next block\n"
            "  \"median_price\" :  \"array\"      (array)  The median price info\n"
            "}\n"
            "\nResult (for verbose=false):\n"
            "\"data\"             (string) A string that is serialized, hex-encoded data for block 'hash'.\n"
            "\nExamples:\n"
            + HelpExampleCli("getblock", "\"1000\"")
            + HelpExampleRpc("getblock", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\""));
    }

    // RPCTypeCheck(params, boost::assign::list_of(str_type)(bool_type)); disable this to allow either string or int argument

    std::string strHash;
    if (int_type == params[0].type()) {
        int height = params[0].get_int();
        if (height < 0 || height > chainActive.Height())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range.");

        CBlockIndex* pBlockIndex = chainActive[height];
        strHash                  = pBlockIndex->GetBlockHash().GetHex();
    } else {
        strHash = params[0].get_str();
    }
    uint256 hash(uint256S(strHash));

    bool fVerbose = true;
    if (params.size() > 1)
        fVerbose = params[1].get_bool();

    if (mapBlockIndex.count(hash) == 0)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

    CBlock block;
    CBlockIndex* pBlockIndex = mapBlockIndex[hash];
    if (!ReadBlockFromDisk(pBlockIndex, block)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Can't read block from disk");
    }

    if (!fVerbose) {
        CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
        ssBlock << block;
        std::string strHex = HexStr(ssBlock.begin(), ssBlock.end());
        return strHex;
    }

    return BlockToJSON(block, pBlockIndex);
}

Value verifychain(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2) {
        throw runtime_error(
            "verifychain ( checklevel numofblocks )\n"
            "\nVerifies blockchain database.\n"
            "\nArguments:\n"
            "1. checklevel   (numeric, optional, 0-4, default=3) How thorough the block verification is.\n"
            "2. numofblocks    (numeric, optional, default=1288, 0=all) The number of blocks to check.\n"
            "\nResult:\n"
            "true|false       (boolean) Verified Okay or not\n"
            "\nExamples:\n"
            + HelpExampleCli("verifychain", "")
            + HelpExampleRpc("verifychain", "( 4 10000 )"));
    }

    int nCheckLevel = SysCfg().GetArg("-checklevel", 3);
    int nCheckDepth = SysCfg().GetArg("-checkblocks", 1288);
    if (params.size() > 0)
        nCheckLevel = params[0].get_int();
    if (params.size() > 1)
        nCheckDepth = params[1].get_int();

    return VerifyDB(nCheckLevel, nCheckDepth);
}

Value getblockchaininfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0) {
        throw runtime_error("getblockchaininfo\n"
            "Returns an object containing various state info regarding block chain processing.\n"
            "\nResult:\n"
            "{\n"
            "  \"chain\": \"xxxx\",        (string) current chain (main, testnet3, regtest)\n"
            "  \"blocks\": xxxxxx,         (numeric) the current number of blocks processed in the server\n"
            "  \"bestblockhash\": \"...\", (string) the hash of the currently best block\n"
            "  \"difficulty\": xxxxxx,     (numeric) the current difficulty\n"
            "  \"verificationprogress\": xxxx, (numeric) estimate of verification progress [0..1]\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleRpc("getblockchaininfo", ""));
    }

    ProxyType proxy;
    GetProxy(NET_IPV4, proxy);

    Object obj;
    std::string chain = SysCfg().DataDir();
    if(chain.empty()) chain = "main";
    obj.push_back(Pair("chain",         chain));
    obj.push_back(Pair("blocks",        (int)chainActive.Height()));
    obj.push_back(Pair("bestblockhash", chainActive.Tip()->GetBlockHash().GetHex()));
    return obj;
}

Value getcontractregid(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1) {
        throw runtime_error("getcontractregid \n"
            "\nreturn an object with regid\n"
            "\nArguments:\n"
            "1. txid   (string, required) the contract registration txid.\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleRpc("getcontractregid", "5zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG"));
    }

    uint256 txid(uint256S(params[0].get_str()));

    int index = 0;
    int nBlockHeight = GetTxConfirmHeight(txid, *pCdMan->pContractCache);
    if (nBlockHeight > chainActive.Height()) {
        throw runtime_error("height bigger than tip block");
    } else if (-1 == nBlockHeight) {
        throw runtime_error("tx unconfirmed");
    }
    CBlockIndex* pIndex = chainActive[nBlockHeight];
    CBlock block;
    if (!ReadBlockFromDisk(pIndex, block))
        return false;

    block.BuildMerkleTree();
    std::tuple<bool,int> ret = block.GetTxIndex(txid);
    if (!std::get<0>(ret)) {
        throw runtime_error("tx not exit in block");
    }

    index = std::get<1>(ret);
    CRegID regId(nBlockHeight, index);
    Object result;
    result.push_back(Pair("regid", regId.ToString()));
    result.push_back(Pair("regid_hex", HexStr(regId.GetRegIdRaw())));
    return result;
}

Value invalidateblock(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "invalidateblock \"hash\"\n"
            "\nPermanently marks a block as invalid, as if it violated a consensus rule.\n"
            "\nArguments:\n"
            "1. \"hash\"         (string, required) the hash of the block to mark as invalid\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleRpc("invalidateblock", "\"hash\""));
    }

    std::string strHash = params[0].get_str();
    uint256 hash(uint256S(strHash));
    CValidationState state;

    {
        LOCK(cs_main);
        if (mapBlockIndex.count(hash) == 0)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

        CBlockIndex* pBlockIndex = mapBlockIndex[hash];
        InvalidateBlock(state, pBlockIndex);
    }

    if (state.IsValid()) {
        ActivateBestChain(state);
    }

    if (!state.IsValid()) {
        throw JSONRPCError(RPC_DATABASE_ERROR, state.GetRejectReason());
    }

    Object obj;
    obj.push_back(Pair("msg", "success"));
    return obj;
}

Value reconsiderblock(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "reconsiderblock \"hash\"\n"
            "\nRemoves invalidity status of a block and its descendants, reconsider them for activation.\n"
            "This can be used to undo the effects of invalidateblock.\n"
            "\nArguments:\n"
            "1. hash   (string, required) the hash of the block to reconsider\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleRpc("reconsiderblock", "\"hash\""));
    }

    std::string strHash = params[0].get_str();
    uint256 hash(uint256S(strHash));
    CValidationState state;

    {
        LOCK(cs_main);
        if (mapBlockIndex.count(hash) == 0)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

        CBlockIndex* pBlockIndex = mapBlockIndex[hash];
        ReconsiderBlock(state, pBlockIndex);
    }

    if (state.IsValid()) {
        ActivateBestChain(state);
    }

    if (!state.IsValid()) {
        throw JSONRPCError(RPC_DATABASE_ERROR, state.GetRejectReason());
    }

    Object obj;
    obj.push_back(Pair("msg", "success"));
    return obj;
}

static unique_ptr<MsgQueue<CBaseCoinTransferTx>> generationQueue;

void static CommonTxGenerator(const int64_t period, const int64_t batchSize) {
    RenameThread("CommonTxGenerator");
    SetThreadPriority(THREAD_PRIORITY_NORMAL);

    CCoinSecret vchSecret;
    vchSecret.SetString("Y6J4aK6Wcs4A3Ex4HXdfjJ6ZsHpNZfjaS4B9w7xqEnmFEYMqQd13");
    CKey key = vchSecret.GetKey();

    // remove key from wallet first.
    {
        LOCK2(cs_main, pWalletMain->cs_wallet);
        if (!pWalletMain->RemoveKey(key)) throw boost::thread_interrupted();
    }

    CRegID srcRegId("0-1");
    CRegID desRegId("0-1");
    static uint64_t llValue = 10000;  // use static variable to keep autoincrement
    uint64_t llFees         = SysCfg().GetTxFee();

    while (true) {
        // add interruption point
        boost::this_thread::interruption_point();

        int64_t nStart      = GetTimeMillis();
        int32_t validHeight = chainActive.Height();

        for (int64_t i = 0; i < batchSize; ++i) {
            CBaseCoinTransferTx tx;
            tx.txUid        = srcRegId;
            tx.toUid        = desRegId;
            tx.coin_amount  = llValue++;
            tx.llFees       = llFees;
            tx.valid_height = validHeight;

            // sign transaction
            key.Sign(tx.ComputeSignatureHash(), tx.signature);

            generationQueue.get()->Push(std::move(tx));
        }

        int64_t elapseTime = GetTimeMillis() - nStart;
        LogPrint("DEBUG",
                 "CommonTxGenerator, batch generate transaction(s): %ld, elapse time: %ld ms.\n",
                 batchSize, elapseTime);
        if (elapseTime < period) {
            MilliSleep(period - elapseTime);
        } else {
            LogPrint("DEBUG", "CommonTxGenerator, need to slow down for overloading.\n");
        }
    }
}

void static CommonTxSender() {
    RenameThread("CommonTxSender");
    SetThreadPriority(THREAD_PRIORITY_NORMAL);

    CValidationState state;
    CBaseCoinTransferTx tx;

    while (true) {
        // add interruption point
        boost::this_thread::interruption_point();

        if (generationQueue.get()->Pop(&tx)) {
            LOCK(cs_main);
            if (!::AcceptToMemoryPool(mempool, state, (CBaseTx*)&tx, true)) {
                LogPrint("ERROR", "CommonTxSender, accept to mempool failed: %s\n",
                         state.GetRejectReason());
                throw boost::thread_interrupted();
            }
        }
    }
}

void StartCommonGeneration(const int64_t period, const int64_t batchSize) {
    static boost::thread_group* generateThreads = NULL;

    if (generateThreads != NULL) {
        generateThreads->interrupt_all();
        delete generateThreads;
        generateThreads = NULL;
    }

    if (period == 0 || batchSize == 0) return;

    // reset message queue according to <period, batchSize>
    // For example, generate 50(batchSize) transactions in 20(period), then
    // we need to prepare 1000 * 10 / 20 * 50 = 25,000 transactions in 10 second.
    // Actually, set the message queue's size to 50,000(double or up to 60,000).
    MsgQueue<CBaseCoinTransferTx>::SizeType size       = 1000 * 10 * batchSize * 2 / period;
    MsgQueue<CBaseCoinTransferTx>::SizeType actualSize = size > MSG_QUEUE_MAX_LEN ? MSG_QUEUE_MAX_LEN : size;
    generationQueue.reset(new MsgQueue<CBaseCoinTransferTx>(actualSize));

    generateThreads = new boost::thread_group();
    generateThreads->create_thread(boost::bind(&CommonTxGenerator, period, batchSize));
    generateThreads->create_thread(boost::bind(&CommonTxSender));
}

Value startcommontpstest(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2) {
        throw runtime_error(
            "startcommontpstest \"period\" \"batch_size\"\n"
            "\nStart generation blocks with batch_size transactions in period ms.\n"
            "\nArguments:\n"
            "1.\"period\" (numeric, required) 0~1000\n"
            "2.\"batch_size\" (numeric, required)\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("startcommontpstest", "20 20") + "\nAs json rpc call\n" +
            HelpExampleRpc("startcommontpstest", "20, 20"));
    }

    Object obj;
    if (SysCfg().NetworkID() != REGTEST_NET) {
        obj.push_back(Pair("msg", "regtest only."));
        return obj;
    }

    int64_t period = params[0].get_int64();
    if (period < 0 || period > 1000) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "period should range between 0 to 1000");
    }

    int64_t batchSize = params[1].get_int64();
    if (batchSize < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "batch size should be bigger than 0");
    }

    StartCommonGeneration(period, batchSize);

    obj.push_back(Pair("msg", "success"));
    return obj;
}

static unique_ptr<MsgQueue<CLuaContractInvokeTx>> generationContractQueue;

void static ContractTxGenerator(const string& regid, const int64_t period,
                                const int64_t batchSize) {
    RenameThread("Tx-generator-v2");
    SetThreadPriority(THREAD_PRIORITY_NORMAL);

    CCoinSecret vchSecret;
    vchSecret.SetString("Y6J4aK6Wcs4A3Ex4HXdfjJ6ZsHpNZfjaS4B9w7xqEnmFEYMqQd13");
    CKey key = vchSecret.GetKey();

    // remove key from wallet first.
    {
        LOCK2(cs_main, pWalletMain->cs_wallet);
        if (!pWalletMain->RemoveKey(key)) throw boost::thread_interrupted();
    }

    CRegID txUid("0-1");
    CRegID appUid(regid);
    static uint64_t llValue = 10000;  // use static variable to keep autoincrement
    uint64_t llFees         = 10 * SysCfg().GetTxFee();
    // hex(whmD4M8Q8qbEx6R5gULbcb5ZkedbcRDGY1) =
    // 77686d44344d3851387162457836523567554c626362355a6b656462635244475931
    string arguments =
        ParseHexStr("77686d44344d3851387162457836523567554c626362355a6b656462635244475931");

    while (true) {
        // add interruption point
        boost::this_thread::interruption_point();

        int64_t nStart      = GetTimeMillis();
        int32_t validHeight = chainActive.Height();

        for (int64_t i = 0; i < batchSize; ++i) {
            CLuaContractInvokeTx tx;
            tx.txUid        = txUid;
            tx.app_uid      = appUid;
            tx.coin_amount  = llValue++;
            tx.llFees       = llFees;
            tx.arguments    = arguments;
            tx.valid_height = validHeight;

            // sign transaction
            key.Sign(tx.ComputeSignatureHash(), tx.signature);

            generationContractQueue.get()->Push(std::move(tx));
        }

        int64_t elapseTime = GetTimeMillis() - nStart;
        LogPrint("DEBUG",
                 "ContractTxGenerator, batch generate transaction(s): %ld, elapse time: %ld ms.\n",
                 batchSize, elapseTime);
        if (elapseTime < period) {
            MilliSleep(period - elapseTime);
        } else {
            LogPrint("DEBUG", "ContractTxGenerator, need to slow down for overloading.\n");
        }
    }
}

void static ContractTxGenerator() {
    RenameThread("ContractTxGenerator");
    SetThreadPriority(THREAD_PRIORITY_NORMAL);

    CValidationState state;
    CLuaContractInvokeTx tx;

    while (true) {
        // add interruption point
        boost::this_thread::interruption_point();

        if (generationContractQueue.get()->Pop(&tx)) {
            LOCK(cs_main);
            if (!::AcceptToMemoryPool(mempool, state, (CBaseTx*)&tx, true)) {
                LogPrint("ERROR", "ContractTxGenerator, accept to mempool failed: %s\n",
                         state.GetRejectReason());
                throw boost::thread_interrupted();
            }
        }
    }
}

void StartContractGeneration(const string& regid, const int64_t period, const int64_t batchSize) {
    static boost::thread_group* generateContractThreads = NULL;

    if (generateContractThreads != NULL) {
        generateContractThreads->interrupt_all();
        delete generateContractThreads;
        generateContractThreads = NULL;
    }

    if (regid.empty() || period == 0 || batchSize == 0) return;

    // reset message queue according to <period, batchSize>
    // For example, generate 50(batchSize) transactions in 20(period), then
    // we need to prepare 1000 * 10 / 20 * 50 = 25,000 transactions in 10 second.
    // Actually, set the message queue's size to 50,000(double or up to 60,000).
    MsgQueue<CLuaContractInvokeTx>::SizeType size = 1000 * 10 * batchSize * 2 / period;
    MsgQueue<CLuaContractInvokeTx>::SizeType actualSize =
        size > MSG_QUEUE_MAX_LEN ? MSG_QUEUE_MAX_LEN : size;
    generationContractQueue.reset(new MsgQueue<CLuaContractInvokeTx>(actualSize));

    generateContractThreads = new boost::thread_group();
    generateContractThreads->create_thread(
        boost::bind(&ContractTxGenerator, regid, period, batchSize));
    generateContractThreads->create_thread(boost::bind(&ContractTxGenerator));
}

Value startcontracttpstest(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 3) {
        throw runtime_error(
            "startcontracttpstest \"regid\" \"period\" \"batch_size\"\n"
            "\nStart generation blocks with batch_size contract transactions in period ms.\n"
            "\nArguments:\n"
            "1.\"regid\" (string, required) contract regid\n"
            "2.\"period\" (numeric, required) 0~1000\n"
            "3.\"batch_size\" (numeric, required)\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("startcontracttpstest", "\"3-1\" 20 20") + "\nAs json rpc call\n" +
            HelpExampleRpc("startcontracttpstest", "\"3-1\", 20, 20"));
    }

    Object obj;
    if (SysCfg().NetworkID() != REGTEST_NET) {
        obj.push_back(Pair("msg", "regtest only."));
        return obj;
    }

    string regid = params[0].get_str();
    if (regid.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "regid should not be empty");
    }

    int64_t period = params[1].get_int64();
    if (period < 0 || period > 1000) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "period should range between 0 to 1000");
    }

    int64_t batchSize = params[2].get_int64();
    if (batchSize < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "batch size should be bigger than 0");
    }

    StartContractGeneration(regid, period, batchSize);

    obj.push_back(Pair("msg", "success"));
    return obj;
}

Value getblockfailures(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "getblockfailures \"block height\"\n"
            "\nGet log failures by block height.\n"
            "\nArguments:\n"
            "1.\"block height\" (numberic, required)\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("getblockfailures", "100") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("getblockfailures", "100"));
    }

    int height = params[0].get_int();
    if (height < 0 || height > chainActive.Height())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range.");

    vector<std::tuple<uint256, uint8_t, string>> results;
    if (!pCdMan->pLogCache->GetExecuteFail(height, results)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to get log failures.");
    }

    Object obj;
    Array failures;
    Object failure;

    for (const auto &item : results) {
        failure.push_back(Pair("txid",          std::get<0>(item).GetHex()));
        failure.push_back(Pair("error_code",    std::get<1>(item)));
        failure.push_back(Pair("error_message", std::get<2>(item)));

        failures.push_back(failure);
    }

    obj.push_back(Pair("block_height",  height));
    obj.push_back(Pair("failures",      failures));

    return obj;
}
