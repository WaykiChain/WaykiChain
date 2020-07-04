// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <stdint.h>
#include <boost/assign/list_of.hpp>

#include "commons/messagequeue.h"
#include "commons/uint256.h"
#include "commons/util/util.h"
#include "config/configuration.h"
#include "init.h"
#include "main.h"
#include "rpc/core/rpcserver.h"
#include "rpc/core/rpccommons.h"
#include "sync.h"
#include "tx/merkletx.h"
#include "tx/tx.h"
#include "tx/coinminttx.h"
#include "tx/universaltx.h"
#include "wallet/wallet.h"
#include "wasm/abi_serializer.hpp"
#include "wasm/modules/wasm_native_dispatch.hpp"

using namespace json_spirit;
using namespace std;

class CBaseCoinTransferTx;

Object BlockToJSON(const CBlock& block, const CBlockIndex* pBlockIndex) {
    Object result;
    result.push_back(Pair("curr_block_hash",     block.GetHash().GetHex()));
    if (pBlockIndex->pprev)
        result.push_back(Pair("prev_block_hash", pBlockIndex->pprev->GetBlockHash().GetHex()));
    CBlockIndex* pNext = chainActive.Next(pBlockIndex);
    if (pNext)
        result.push_back(Pair("next_block_hash", pNext->GetBlockHash().GetHex()));

    result.push_back(Pair("bp_uid",         block.vptx[0]->txUid.ToString()));
    result.push_back(Pair("version",        block.GetVersion()));
    result.push_back(Pair("merkle_root",    block.GetMerkleRootHash().GetHex()));
    result.push_back(Pair("total_fuel_fee", block.GetFuelFee()));
    CMerkleTx txGen(block.vptx[0]);
    txGen.SetMerkleBranch(&block);
    result.push_back(Pair("confirmations",  (int32_t)txGen.GetDepthInMainChain()));
    result.push_back(Pair("size",           (int32_t)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION)));
    result.push_back(Pair("height",         (int32_t)block.GetHeight()));
    result.push_back(Pair("tx_count",       (int32_t)block.vptx.size()));

    Array txs;
    for (const auto& ptx : block.vptx)
        txs.push_back(ptx->GetHash().GetHex());
    result.push_back(Pair("tx",             txs));

    result.push_back(Pair("time",           block.GetBlockTime()));
    result.push_back(Pair("nonce",          (uint64_t)block.GetNonce()));

    Array prices;
    const auto& priceMap = block.GetBlockMedianPrice();
    for (const auto &item : priceMap) {
        if (item.second == 0) {
            continue;
        }

        Object price;
        price.push_back(Pair("coin_symbol",   item.first.first));
        price.push_back(Pair("price_symbol",  item.first.second));
        price.push_back(Pair("price",         (double) item.second / PRICE_BOOST));
        prices.push_back(price);
    }
    result.push_back(Pair("median_price", prices));

    return result;
}

Value getblockcount(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getblockcount\n"
            "\nReturns the number of blocks in the longest chain.\n"
            "\nResult:\n"
            "\n    (numeric) The current block count\n"
            "\nExamples:\n" +
            HelpExampleCli("getblockcount", "") + "\nAs json rpc\n" + HelpExampleRpc("getblockcount", ""));

    return chainActive.Height();
}

Value getfcoingenesistxinfo(const Array& params, bool fHelp) {
    Object output;

    NET_TYPE netType = SysCfg().NetworkID();
    uint32_t nVer2GenesisHeight = IniCfg().GetVer2GenesisHeight(netType);

    // Stablecoin Global Reserve Account with its initial reseve creation
    auto pTx = std::make_shared<CCoinMintTx>(CNullID(), nVer2GenesisHeight, SYMB::WUSD,
                                               FUND_COIN_GENESIS_INITIAL_RESERVE_AMOUNT * COIN);
    pTx->nVersion = INIT_TX_VERSION;
    output.push_back(Pair("fcoin_global_account_txid", pTx->GetHash().GetHex()));

    // FundCoin Genesis Account with the total FundCoin release creation
    pTx = std::make_shared<CCoinMintTx>(CPubKey(ParseHex(IniCfg().GetInitFcoinOwnerPubKey(netType))),
                                          nVer2GenesisHeight, SYMB::WGRT,
                                          FUND_COIN_GENESIS_TOTAL_RELEASE_AMOUNT * COIN);
    output.push_back(Pair("fcoin_genesis_account_txid", pTx->GetHash().GetHex()));

    // DEX Order Matching Service Account
    pTx = std::make_shared<CCoinMintTx>(CPubKey(ParseHex(IniCfg().GetDexMatchServicePubKey(netType))),
                                          nVer2GenesisHeight, SYMB::WGRT, 0);
    output.push_back(Pair("dex_match_account_txid", pTx->GetHash().GetHex()));

    return output;
}

Value getrawmempool(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getrawmempool ( verbose )\n"
            "\nReturns all transaction ids in memory pool as a json or an array of string transaction ids.\n"
            "\nArguments:\n"
            "1. verbose           (boolean, optional, default=false) true for a json object, false for array of "
            "transaction ids\n"
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
            "    \"time\" : n,             (numeric) local time transaction entered pool in seconds since 1 Jan 1970 "
            "GMT\n"
            "    \"height\" : n,           (numeric) block height when transaction entered pool\n"
            "  }, ...\n"
            "]\n"
            "\nExamples\n" +
            HelpExampleCli("getrawmempool", "true") + "\nAs json rpc\n" + HelpExampleRpc("getrawmempool", "true"));

    bool fVerbose = false;
    if (params.size() > 0)
        fVerbose = params[0].get_bool();

    if (fVerbose) {
        LOCK(mempool.cs);
        Object obj;
        for (const auto& entry : mempool.memPoolTxs) {
            const uint256& hash      = entry.first;
            const CTxMemPoolEntry& mpe = entry.second;
            Object info;
            info.push_back(Pair("size",         (int) mpe.GetTxSize()));
            info.push_back(Pair("fees_type",    std::get<0>(mpe.GetFees())));
            info.push_back(Pair("fees",         JsonValueFromAmount(std::get<1>(mpe.GetFees()))));
            info.push_back(Pair("time",         mpe.GetTime()));
            info.push_back(Pair("height",       (int) mpe.GetHeight()));
            info.push_back(Pair("priority",     mpe.GetPriority()));

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

Value getblock(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 1 || params.size() > 3) {
        throw runtime_error(
            "getblock \"hash or height\"  [\"list_txs\"] [\"verbose\"]\n"
            "\nIf verbose is false, returns a string that is serialized, hex-encoded data for block 'hash'.\n"
            "If verbose is true, returns an Object with information about block <hash>.\n"
            "\nArguments:\n"
            "1.\"hash or height\"   (string or numeric, required) string for the block hash, or numeric for the block "
            "height\n"
            "2.\"list_txs\"         (boolean, optional, default=false) true: return a detailed list of txes from the block\n"
            "3.\"verbose\"          (boolean, optional, default=true) true: return a json object; false: return hex only\n"
            "encoded data\n"
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
            "  \"time\" : n,            (numeric) The block time in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"nonce\" : n,           (numeric) The nonce\n"
            "  \"previousblockhash\" : \"hash\",  (string) The hash of the previous block\n"
            "  \"nextblockhash\" : \"hash\"       (string) The hash of the next block\n"
            "  \"median_price\" :  \"array\"      (array)  The median price info\n"
            "}\n"
            "\nResult (for verbose=false):\n"
            "\"data\"             (string) A string that is serialized, hex-encoded data for block 'hash'.\n"
            "\nExamples:\n" +
            HelpExampleCli("getblock", "\"d640d051704155b1fd3ec8d0331497448c259b0ab0499e109da7ae2bc7423bc2\"") +
            "\nAs json rpc\n" +
            HelpExampleRpc("getblock", "\"d640d051704155b1fd3ec8d0331497448c259b0ab0499e109da7ae2bc7423bc2\""));
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

    bool fListTxs = false;
    if (params.size() > 1)
        fListTxs = params[1].get_bool();

    bool fVerbose = true;
    if (params.size() > 2)
        fVerbose = params[2].get_bool();


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

    vector<CReceipt> blockReceipts;
    Object o = BlockToJSON(block, pBlockIndex);

    auto pCw = make_shared<CCacheWrapper>(pCdMan);
    if(fListTxs) {
        Array arr;
        for (size_t i = 0; i < block.vptx.size(); i++) {
            arr.push_back(GetTxDetailJSON(*pCw, block, block.vptx[i], CTxCord(block.GetHeight(), i)));
        }
        o.push_back(Pair("tx_details", arr));
    }

    pCdMan->pReceiptCache->GetBlockReceipts(block.GetHash(), blockReceipts);
    o.push_back(Pair("receipts",  JSON::ToJson(*pCdMan->pAccountCache, blockReceipts)));

    return o;
}

Value verifychain(const Array& params, bool fHelp) {
    if (fHelp || params.size() > 2) {
        throw runtime_error(
            "verifychain ( checklevel numofblocks )\n"
            "\nVerifies blockchain database.\n"
            "\nArguments:\n"
            "1.\"checklevel\"   (numeric, optional, 0-4, default=3) How thorough the block verification is.\n"
            "2.\"numofblocks\"  (numeric, optional, default=1288, 0=all) The number of blocks to check.\n"
            "\nResult:\n"
            "true|false       (boolean) Verified Okay or not\n"
            "\nExamples:\n" +
            HelpExampleCli("verifychain", "") + "\nAs json rpc\n" + HelpExampleRpc("verifychain", "4, 10000"));
    }

    int nCheckLevel = SysCfg().GetArg("-checklevel", 3);
    int nCheckDepth = SysCfg().GetArg("-checkblocks", 1288);
    if (params.size() > 0)
        nCheckLevel = params[0].get_int();
    if (params.size() > 1)
        nCheckDepth = params[1].get_int();

    return VerifyDB(nCheckLevel, nCheckDepth);
}

Value getcontractregid(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "getcontractregid\n"
            "\nreturn an object with regid\n"
            "\nArguments:\n"
            "1. txid   (string, required) the contract registration txid.\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("getcontractregid", "\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\"") + "\nAs json rpc\n" +
            HelpExampleRpc("getcontractregid", "\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\""));
    }

    uint256 txid(uint256S(params[0].get_str()));

    int index = 0;
    int nBlockHeight = GetTxConfirmHeight(txid, *pCdMan->pBlockCache);
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
    if (fHelp || params.size() < 1 || params.size() > 2) {
        throw runtime_error(
            "reconsiderblock \"hash\" [children]\n"
            "\nRemoves invalidity status of a block and its descendants, reconsider them for activation.\n"
            "This can be used to undo the effects of invalidateblock.\n"
            "\nArguments:\n"
            "1. hash   (string, required) the hash of the block to reconsider\n"
            "2. children (bool, optional) reconsider all children of the block, default is false"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleRpc("reconsiderblock", "\"hash\" false"));
    }

    std::string strHash = params[0].get_str();
    bool children = params.size() > 1 ? params[1].get_bool() : false;
    uint256 hash(uint256S(strHash));
    CValidationState state;

    {
        LOCK(cs_main);
        if (mapBlockIndex.count(hash) == 0)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

        CBlockIndex* pBlockIndex = mapBlockIndex[hash];
        ReconsiderBlock(state, pBlockIndex, children);
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


class TpsTester {
public:
    typedef MsgQueue<std::shared_ptr<CBaseTx>> GenTxQueue;
public:
    int64_t period;
    int64_t batchSize;
    unique_ptr<GenTxQueue> generationQueue;
    boost::thread_group* generateThreads;
public:
    TpsTester() {

    }

    ~TpsTester() {
        Stop();
    }

    void Stop() {
        if (generateThreads != nullptr) {
            generateThreads->interrupt_all();
            generateThreads->join_all();
            delete generateThreads;
            generateThreads = nullptr;
        }
    }

    void SendTx() {
        RenameThread("TpsTester::SendTx");
        SetThreadPriority(THREAD_PRIORITY_NORMAL);

        CValidationState state;
        std::shared_ptr<CBaseTx> tx;

        while (true) {
            // add interruption point
            boost::this_thread::interruption_point();

            if (generationQueue->Pop(&tx)) {
                LOCK(cs_main);
                if (!::AcceptToMemoryPool(mempool, state, tx.get(), true)) {
                    LogPrint(BCLog::ERROR, "TpsTester::SendTx, accept to mempool failed: %s\n", state.GetRejectReason());
                    throw boost::thread_interrupted();
                }
            }
        }
    }

    template<typename F>
    void StartCommonGeneration(const int64_t periodIn, const int64_t batchSizeIn, F generateFunc) {

        Stop();

        period = periodIn;
        batchSize = batchSizeIn;
        if (period == 0 || batchSize == 0)
            return;

        // reset message queue according to <period, batchSize>
        // For example, generate 50(batchSize) transactions in 20(period), then
        // we need to prepare 1000 * 10 / 20 * 50 = 25,000 transactions in 10 second.
        // Actually, set the message queue's size to 50,000(double or up to 60,000).
        GenTxQueue::SizeType size       = 1000 * 10 * batchSize * 2 / period;
        GenTxQueue::SizeType actualSize = size > MSG_QUEUE_MAX_LEN ? MSG_QUEUE_MAX_LEN : size;

        generationQueue = std::make_unique<GenTxQueue>(actualSize);

        generateThreads = new boost::thread_group();
        generateThreads->create_thread(generateFunc);
        generateThreads->create_thread(boost::bind(&TpsTester::SendTx, this));
    }


};

TpsTester g_tpsTester = TpsTester();


static void CommonTxGenerator(TpsTester *tpsTester) {
    RenameThread("CommonTxGenerator");
    SetThreadPriority(THREAD_PRIORITY_NORMAL);

    CCoinSecret vchSecret;
    vchSecret.SetString("Y6J4aK6Wcs4A3Ex4HXdfjJ6ZsHpNZfjaS4B9w7xqEnmFEYMqQd13");
    CKey key = vchSecret.GetKey();

    // remove key from wallet first.
    {
        LOCK2(cs_main, pWalletMain->cs_wallet);
        if (!pWalletMain->RemoveKey(key))
            throw boost::thread_interrupted();
    }
    shared_ptr<CCacheWrapper> spCw;
    uint64_t llFees         = 0;
    {
        LOCK(cs_main);
        spCw = make_shared<CCacheWrapper>(pCdMan);
        GetTxMinFee(*spCw, BCOIN_TRANSFER_TX, chainActive.Height(), SYMB::WICC, llFees);
    }

    CRegID srcRegId("0-1");
    CRegID desRegId("0-1");
    static uint64_t llValue = 10000;  // use static variable to keep autoincrement

    while (true) {
        // add interruption point
        boost::this_thread::interruption_point();

        int64_t nStart      = GetTimeMillis();
        int32_t validHeight = chainActive.Height();

        for (int64_t i = 0; i < tpsTester->batchSize; ++i) {
            std::shared_ptr<CBaseCoinTransferTx> tx;
            tx->txUid        = srcRegId;
            tx->toUid        = desRegId;
            tx->coin_amount  = llValue++;
            tx->llFees       = llFees;
            tx->valid_height = validHeight;

            // sign transaction
            key.Sign(tx->GetHash(), tx->signature);

            tpsTester->generationQueue->Push(tx);
        }

        int64_t elapseTime = GetTimeMillis() - nStart;
        LogPrint(BCLog::DEBUG, "batch generate transaction(s): %ld, elapse time: %ld ms.\n",
                tpsTester->batchSize, elapseTime);

        if (elapseTime < tpsTester->period) {
            MilliSleep(tpsTester->period - elapseTime);
        } else {
            LogPrint(BCLog::DEBUG, "need to slow down for overloading.\n");
        }
    }
}

void StartCommonGeneration(const int64_t period, const int64_t batchSize) {
    g_tpsTester.StartCommonGeneration(period, batchSize, boost::bind(&CommonTxGenerator, &g_tpsTester));
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

void static ContractTxGenerator(TpsTester *tpsTester, const string& regid) {
    RenameThread("contract-tx-generator");
    SetThreadPriority(THREAD_PRIORITY_NORMAL);

    CCoinSecret vchSecret;
    vchSecret.SetString("Y6J4aK6Wcs4A3Ex4HXdfjJ6ZsHpNZfjaS4B9w7xqEnmFEYMqQd13");
    CKey key = vchSecret.GetKey();

    // remove key from wallet first.
    {
        LOCK2(cs_main, pWalletMain->cs_wallet);
        if (!pWalletMain->RemoveKey(key))
            throw boost::thread_interrupted();
    }
    shared_ptr<CCacheWrapper> spCw;
    uint64_t llFees         = 0;
    {
        LOCK(cs_main);
        spCw = make_shared<CCacheWrapper>(pCdMan);
        GetTxMinFee(*spCw, BCOIN_TRANSFER_TX, chainActive.Height(), SYMB::WICC, llFees);
    }
    CRegID txUid("0-1");
    CRegID appUid(regid);
    static uint64_t llValue = 10000;  // use static variable to keep autoincrement

    // hex(whmD4M8Q8qbEx6R5gULbcb5ZkedbcRDGY1) =
    // 77686d44344d3851387162457836523567554c626362355a6b656462635244475931
    string arguments = ParseHexStr("77686d44344d3851387162457836523567554c626362355a6b656462635244475931");

    while (true) {
        // add interruption point
        boost::this_thread::interruption_point();

        int64_t nStart      = GetTimeMillis();
        int32_t validHeight = chainActive.Height();

        for (int64_t i = 0; i < tpsTester->batchSize; ++i) {
            shared_ptr<CLuaContractInvokeTx> tx;
            tx->txUid        = txUid;
            tx->app_uid      = appUid;
            tx->coin_amount  = llValue++;
            tx->llFees       = llFees;
            tx->arguments    = arguments;
            tx->valid_height = validHeight;

            // sign transaction
            key.Sign(tx->GetHash(), tx->signature);

            tpsTester->generationQueue->Push(tx);
        }

        int64_t elapseTime = GetTimeMillis() - nStart;
        LogPrint(BCLog::DEBUG, "batch generate transaction(s): %ld, elapse time: %ld ms.\n", tpsTester->batchSize, elapseTime);

        if (elapseTime < tpsTester->period) {
            MilliSleep(tpsTester->period - elapseTime);
        } else {
            LogPrint(BCLog::DEBUG, "need to slow down for overloading.\n");
        }
    }
}

void StartContractGeneration(const string& regid, const int64_t period, const int64_t batchSize) {
    g_tpsTester.StartCommonGeneration(period, batchSize, boost::bind(&ContractTxGenerator, &g_tpsTester, regid));
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



void static WasmTxGenerator(TpsTester *tpsTester, wasm::inline_transaction &inlineTx) {
    RenameThread("contract-tx-generator");
    SetThreadPriority(THREAD_PRIORITY_NORMAL);

    CCoinSecret vchSecret;
    vchSecret.SetString("Y6J4aK6Wcs4A3Ex4HXdfjJ6ZsHpNZfjaS4B9w7xqEnmFEYMqQd13");
    CKey key = vchSecret.GetKey();

    // remove key from wallet first.
    {
        LOCK2(cs_main, pWalletMain->cs_wallet);
        if (!pWalletMain->RemoveKey(key))
            throw boost::thread_interrupted();
    }
    shared_ptr<CCacheWrapper> spCw;
    uint64_t minFees         = 0;
    {
        LOCK(cs_main);
        spCw = make_shared<CCacheWrapper>(pCdMan);
        GetTxMinFee(*spCw, BCOIN_TRANSFER_TX, chainActive.Height(), SYMB::WICC, minFees);
    }
    CRegID txUid("0-1");
    auto payer      = wasm::regid(txUid.GetIntValue());

    inlineTx.authorization = std::vector<permission>{{payer.value, wasmio_owner}};

    int32_t lastHeight = 0;
    uint64_t fees = minFees;
    while (true) {
        // add interruption point
        boost::this_thread::interruption_point();

        int64_t nStart      = GetTimeMillis();
        int32_t validHeight = chainActive.Height();
        if (lastHeight != validHeight) {
            fees = minFees;
            lastHeight = validHeight;
        }

        for (int64_t i = 0; i < tpsTester->batchSize; ++i) {
            shared_ptr<CUniversalTx> tx;
            tx->txUid        = txUid;
            tx->fee_symbol   = SYMB::WICC;
            tx->llFees       = fees;
            tx->valid_height = validHeight;

            tx->inline_transactions.push_back(inlineTx);
            // sign transaction
            key.Sign(tx->GetHash(), tx->signature);

            tpsTester->generationQueue->Push(tx);
            fees++;
        }

        int64_t elapseTime = GetTimeMillis() - nStart;
        LogPrint(BCLog::DEBUG, "batch generate transaction(s): %ld, elapse time: %ld ms.\n", tpsTester->batchSize, elapseTime);

        if (elapseTime < tpsTester->period) {
            MilliSleep(tpsTester->period - elapseTime);
        } else {
            LogPrint(BCLog::DEBUG, "need to slow down for overloading.\n");
        }
    }
}

namespace RPC_PARAM {
    int64_t ParseInt64(const Value &jsonValue, const string &name) {
        ComboMoney money;
        Value_type valueType = jsonValue.type();
        if (valueType == json_spirit::Value_type::int_type ) {
            return jsonValue.get_int64();
        } else if (valueType == json_spirit::Value_type::str_type) {
            return atoi64(jsonValue.get_str());
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s should be numeric or string, but got %s type",
                name, JSON::GetValueTypeName(valueType)));
        }
    }
}

Value startwasmtpstest(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 5) {
        throw runtime_error(
            "startwasmtpstest \"regid\" \"period\" \"batch_size\"\n"
            "\nStart generate wasm contract tx for tps with batch_size tx in period ms.\n"
            "\nArguments:\n"
            "1.\"regid\" (string, required) contract regid\n"
            "2.\"action\" (string, required) contract action\n"
            "3.\"data\" (object, required) contract data, can be object or array\n"
            "4.\"period\" (numeric, required) 0~1000\n"
            "5.\"batch_size\" (numeric, required)\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("startwasmtpstest", "\"0-1\" \"0-800\" \"transfer\" '["
                "\"0-1\", \"8-2\", \"1.00000000 WICC\", \"\"]' 20 20") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("startwasmtpstest", "\"0-1\", \"0-800\", \"transfer\", ["
                "\"0-1\", \"8-2\", \"1.00000000 WICC\", \"\"], 20, 20"));
    }
    auto regid  = RPC_PARAM::ParseRegId(params[0], "contract");
    auto action = wasm::name(params[1].get_str());
    auto argsIn = RPC_PARAM::GetWasmContractArgs(params[2]);
    int64_t period = RPC_PARAM::ParseInt64(params[3], "period");
    int64_t batchSize = RPC_PARAM::ParseInt64(params[4], "batch_size");

    if (SysCfg().NetworkID() != REGTEST_NET) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "work in regtest only");
    }

    //get abi
    std::vector<char> abi;
    {
        LOCK(cs_main);
        if (!get_native_contract_abi(regid.GetIntValue(), abi)) {
            CUniversalContractStore contract_store = RPC_PARAM::GetWasmContract(*pCdMan->pContractCache, regid);
            abi = std::vector<char>(contract_store.abi.begin(), contract_store.abi.end());
        }
    }


    std::vector<char> action_data = wasm::abi_serializer::pack(abi, action.to_string(), argsIn, max_serialization_time);
    CHAIN_ASSERT( action_data.size() < MAX_CONTRACT_ARGUMENT_SIZE,
                    wasm_chain::inline_transaction_data_size_exceeds_exception,
                    "inline transaction args is out of size(%u vs %u)",
                    action_data.size(), MAX_CONTRACT_ARGUMENT_SIZE)


    if (period < 0 || period > 1000) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "period should range between 0 to 1000");
    }

    if (batchSize < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "batch size should be bigger than 0");
    }

    wasm::inline_transaction inlineTx = { regid.GetIntValue(), action.value, {}, action_data };

    g_tpsTester.StartCommonGeneration(period, batchSize, boost::bind(&WasmTxGenerator, &g_tpsTester, inlineTx));

    Object obj;
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

    auto dbIt = MakeDbIterator(pCdMan->pLogCache->executeFailCache);

    Object obj;
    Array failures;
    Object failure;

    for (dbIt->First(); dbIt->IsValid(); dbIt->Next()) {

    //CCompositeKVCache<dbk::TX_EXECUTE_FAIL,   string,            std::pair<uint8_t, string> > executeFailCache;
        failure.push_back(Pair("txid",          dbIt->GetKey().second.GetHex()));
        failure.push_back(Pair("error_code",    dbIt->GetValue().first));
        failure.push_back(Pair("error_message", dbIt->GetValue().second));

        failures.push_back(failure);
    }

    obj.push_back(Pair("block_height",  height));
    obj.push_back(Pair("failures",      failures));

    return obj;
}
