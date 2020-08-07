// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <stdint.h>
#include <boost/assign/list_of.hpp>

#include "commons/uint256.h"
#include "commons/util/util.h"
#include "config/configuration.h"
#include "init.h"
#include "main.h"
#include "rpc/core/rpcserver.h"
#include "rpc/core/rpccommons.h"
#include "sync.h"
#include "tx/tx.h"
#include "tx/coinminttx.h"

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
    result.push_back(Pair("fuel_rate", block.GetFuelRate()));
    result.push_back(Pair("total_fuel_fee", block.GetFuelFee()));
    result.push_back(Pair("confirmations",  (int32_t)chainActive.Height() - pBlockIndex->height + 1));
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
