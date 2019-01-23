// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The WaykiChain developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcserver.h"
#include "main.h"
#include "sync.h"
//#include "checkpoints.h"
#include "configuration.h"

#include <stdint.h>

#include "json/json_spirit_value.h"

using namespace json_spirit;
using namespace std;

//void ScriptPubKeyToJSON(const CScript& scriptPubKey, Object& out, bool fIncludeHex);

double GetDifficulty(const CBlockIndex* blockindex)
{
    // Floating point number that is a multiple of the minimum difficulty,
    // minimum difficulty = 1.0.
    if (blockindex == NULL)
    {
        if (chainActive.Tip() == NULL)
            return 1.0;
        else
            blockindex = chainActive.Tip();
    }

    int nShift = (blockindex->nBits >> 24) & 0xff;

    double dDiff =
        (double)0x0000ffff / (double)(blockindex->nBits & 0x00ffffff);

    while (nShift < 29)
    {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29)
    {
        dDiff /= 256.0;
        nShift--;
    }

    return dDiff;
}

Object blockToJSON(const CBlock& block, const CBlockIndex* blockindex)
{
    Object result;
    result.push_back(Pair("hash", block.GetHash().GetHex()));
    CMerkleTx txGen(block.vptx[0]);
    txGen.SetMerkleBranch(&block);
    result.push_back(Pair("confirmations", (int)txGen.GetDepthInMainChain()));
    result.push_back(Pair("size", (int)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION)));
    result.push_back(Pair("height", blockindex->nHeight));
    result.push_back(Pair("version", block.GetVersion()));
    result.push_back(Pair("merkleroot", block.GetHashMerkleRoot().GetHex()));
    result.push_back(Pair("txnumber", (int)block.vptx.size()));
    Array txs;
    for (const auto& ptx : block.vptx)
//    	txs.push_back(TxToJSON(ptx.get()));
        txs.push_back(ptx->GetHash().GetHex());
    result.push_back(Pair("tx", txs));
    result.push_back(Pair("time", block.GetBlockTime()));
    result.push_back(Pair("nonce", (uint64_t)block.GetNonce()));
    result.push_back(Pair("chainwork", blockindex->nChainWork.GetHex()));
    result.push_back(Pair("fuel", blockindex->nFuel));
    result.push_back(Pair("fuelrate", blockindex->nFuelRate));
    if (blockindex->pprev)
        result.push_back(Pair("previousblockhash", blockindex->pprev->GetBlockHash().GetHex()));
    CBlockIndex *pnext = chainActive.Next(blockindex);
    if (pnext)
        result.push_back(Pair("nextblockhash", pnext->GetBlockHash().GetHex()));
    return result;
}

Value getblockcount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getblockcount\n"
            "\nReturns the number of blocks in the longest block chain.\n"
            "\nResult:\n"
            "n    (numeric) The current block count\n"
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
            "\nReturns all transaction ids in memory pool as a json array of string transaction ids.\n"
            "\nArguments:\n"
            "1. verbose           (boolean, optional, default=false) true for a json object, false for array of transaction ids\n"
            "\nResult: (for verbose = false):\n"
            "[                     (json array of string)\n"
            "  \"transactionid\"     (string) The transaction id\n"
            "  ,...\n"
            "]\n"
            "\nResult: (for verbose = true):\n"
            "{                           (json object)\n"
            "  \"transactionid\" : {       (json object)\n"
            "    \"size\" : n,             (numeric) transaction size in bytes\n"
            "    \"fee\" : n,              (numeric) transaction fee in WICC coins\n"
            "    \"time\" : n,             (numeric) local time transaction entered pool in seconds since 1 Jan 1970 GMT\n"
            "    \"height\" : n,           (numeric) block height when transaction entered pool\n"
            "    \"startingpriority\" : n, (numeric) priority when transaction entered pool\n"
            "    \"currentpriority\" : n,  (numeric) transaction priority now\n"
            "    \"depends\" : [           (array) unconfirmed transactions used as inputs for this transaction\n"
            "        \"transactionid\",    (string) parent transaction id\n"
            "       ... ]\n"
            "  }, ...\n"
            "]\n"
            "\nExamples\n"
            + HelpExampleCli("getrawmempool", "true")
            + HelpExampleRpc("getrawmempool", "true")
        );

    bool fVerbose = false;
    if (params.size() > 0)
        fVerbose = params[0].get_bool();

    if (fVerbose)
    {
        LOCK(mempool.cs);
        Object o;
        for (const auto& entry : mempool.mapTx)
        {
            const uint256& hash = entry.first;
            const CTxMemPoolEntry& e = entry.second;
            Object info;
            info.push_back(Pair("size", (int)e.GetTxSize()));
            info.push_back(Pair("fee", ValueFromAmount(e.GetFee())));
            info.push_back(Pair("time", e.GetTime()));
            info.push_back(Pair("height", (int)e.GetHeight()));
            info.push_back(Pair("startingpriority", e.GetPriority(e.GetHeight())));
            info.push_back(Pair("currentpriority", e.GetPriority(chainActive.Height())));
//            const CTransaction& tx = e.GetTx();
            set<string> setDepends;
//            BOOST_FOREACH(const CTxIn& txin, tx.vin)
//            {
//                if (mempool.exists(txin.prevout.hash))
//                    setDepends.insert(txin.prevout.hash.ToString());
//            }
            Array depends(setDepends.begin(), setDepends.end());
            info.push_back(Pair("depends", depends));
            o.push_back(Pair(hash.ToString(), info));
        }
        return o;
    }
    else
    {
        vector<uint256> vtxid;
        mempool.queryHashes(vtxid);

        Array a;
        for (const auto& hash : vtxid)
            a.push_back(hash.ToString());

        return a;
    }
}

Value getblockhash(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getblockhash index\n"
            "\nReturns hash of block in best-block-chain at index provided.\n"
            "\nArguments:\n"
            "1. height         (numeric, required) The block height\n"
            "\nResult:\n"
            "\"hash\"         (string) The block hash\n"
            "\nExamples:\n"
            + HelpExampleCli("getblockhash", "1000")
            + HelpExampleRpc("getblockhash", "1000")
        );

    int nHeight = params[0].get_int();
    if (nHeight < 0 || nHeight > chainActive.Height())
        throw runtime_error("Block number out of range.");

    CBlockIndex* pblockindex = chainActive[nHeight];
    Object result;
    result.push_back(Pair("hash", pblockindex->GetBlockHash().GetHex()));
    return result;
}

Value getblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getblock \"hash or height\" ( verbose )\n"
            "\nIf verbose is false, returns a string that is serialized, hex-encoded data for block 'hash'.\n"
            "If verbose is true, returns an Object with information about block <hash>.\n"
            "\nArguments:\n"
            "1. \"hash or height\"(string or numeric,required) string for The block hash,numeric for the block height\n"
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
            "     \"transactionid\"     (string) The transaction id\n"
            "     ,...\n"
            "  ],\n"
            "  \"time\" : ttt,          (numeric) The block time in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"nonce\" : n,           (numeric) The nonce\n"
            "  \"bits\" : \"1d00ffff\", (string) The bits\n"
            "  \"difficulty\" : x.xxx,  (numeric) The difficulty\n"
            "  \"previousblockhash\" : \"hash\",  (string) The hash of the previous block\n"
            "  \"nextblockhash\" : \"hash\"       (string) The hash of the next block\n"
            "}\n"
            "\nResult (for verbose=false):\n"
            "\"data\"             (string) A string that is serialized, hex-encoded data for block 'hash'.\n"
            "\nExamples:\n" +
            HelpExampleCli("getblock", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"") + HelpExampleRpc("getblock", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\""));

    std::string strHash;
    if(int_type == params[0].type()) {
        int nHeight = params[0].get_int();
        if (nHeight < 0 || nHeight > chainActive.Height())
            throw runtime_error("Block number out of range.");

        CBlockIndex* pblockindex = chainActive[nHeight];
        strHash= pblockindex->GetBlockHash().GetHex();
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
    CBlockIndex* pblockindex = mapBlockIndex[hash];
    if (!ReadBlockFromDisk(block, pblockindex)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Can't read block from disk");
    }

    if (!fVerbose)
    {
        CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
        ssBlock << block;
        std::string strHex = HexStr(ssBlock.begin(), ssBlock.end());
        return strHex;
    }

    return blockToJSON(block, pblockindex);
}

Value verifychain(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "verifychain ( checklevel numblocks )\n"
            "\nVerifies blockchain database.\n"
            "\nArguments:\n"
            "1. checklevel   (numeric, optional, 0-4, default=3) How thorough the block verification is.\n"
            "2. numblocks    (numeric, optional, default=288, 0=all) The number of blocks to check.\n"
            "\nResult:\n"
            "true|false       (boolean) Verified or not\n"
            "\nExamples:\n"
            + HelpExampleCli("verifychain", "( checklevel numblocks )")
            + HelpExampleRpc("verifychain", "( checklevel numblocks )")
        );

    int nCheckLevel = SysCfg().GetArg("-checklevel", 3);
    int nCheckDepth = SysCfg().GetArg("-checkblocks", 288);
    if (params.size() > 0)
        nCheckLevel = params[0].get_int();
    if (params.size() > 1)
        nCheckDepth = params[1].get_int();

    return VerifyDB(nCheckLevel, nCheckDepth);
}

Value getblockchaininfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getblockchaininfo\n"
            "Returns an object containing various state info regarding block chain processing.\n"
            "\nResult:\n"
            "{\n"
            "  \"chain\": \"xxxx\",        (string) current chain (main, testnet3, regtest)\n"
            "  \"blocks\": xxxxxx,         (numeric) the current number of blocks processed in the server\n"
            "  \"bestblockhash\": \"...\", (string) the hash of the currently best block\n"
            "  \"difficulty\": xxxxxx,     (numeric) the current difficulty\n"
            "  \"verificationprogress\": xxxx, (numeric) estimate of verification progress [0..1]\n"
            "  \"chainwork\": \"xxxx\"     (string) total amount of work in active chain, in hexadecimal\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getblockchaininfo", "")
            + HelpExampleRpc("getblockchaininfo", "")
        );

    proxyType proxy;
    GetProxy(NET_IPV4, proxy);

    Object obj;
    std::string chain = SysCfg().DataDir();
    if(chain.empty())
        chain = "main";
    obj.push_back(Pair("chain",         chain));
    obj.push_back(Pair("blocks",        (int)chainActive.Height()));
    obj.push_back(Pair("bestblockhash", chainActive.Tip()->GetBlockHash().GetHex()));
    obj.push_back(Pair("verificationprogress", Checkpoints::GuessVerificationProgress(chainActive.Tip())));
    obj.push_back(Pair("chainwork",     chainActive.Tip()->nChainWork.GetHex()));
    return obj;
}

Value listsetblockindexvalid(const Array& params, bool fHelp)
{
	if (fHelp || params.size() != 0) {
		throw runtime_error("listsetblockindexvalid \n"
							"\ncall ListSetBlockIndexValid function\n"
							"\nArguments:\n"
							"\nResult:\n"
							"\nExamples:\n"
							+ HelpExampleCli("listsetblockindexvalid", "")
							+ HelpExampleRpc("listsetblockindexvalid",""));
	}
	return ListSetBlockIndexValid();
}

Value getappregid(const Array& params, bool fHelp)
{
	if (fHelp || params.size() != 1) {
		throw runtime_error("getappregid \n"
							"\nreturn an object with regid\n"
							"\nArguments:\n"
							"1. txhash   (string, required) the App Script txid.\n"
							"\nResult:\n"
							"\nExamples:\n"
							+ HelpExampleCli("getappregid", "5zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG")
							+ HelpExampleRpc("getappregid","5zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG"));
	}

	uint256 txhash(uint256S(params[0].get_str()));

	int nIndex = 0;
	int BlockHeight = GetTxConfirmHeight(txhash, *pScriptDBTip) ;
	if(BlockHeight > chainActive.Height())
	{
		throw runtime_error("height larger than tip block \n");
	}
	else if(BlockHeight == -1){
		throw runtime_error("tx hash unconfirmed \n");
	}
	CBlockIndex* pindex = chainActive[BlockHeight];
	CBlock block;
	if (!ReadBlockFromDisk(block, pindex))
		return false;

	block.BuildMerkleTree();
	std::tuple<bool,int> ret = block.GetTxIndex(txhash);
	if (!std::get<0>(ret)) {
		 throw runtime_error("tx not exit in block");
	}

	nIndex = std::get<1>(ret);

	CRegID regID(BlockHeight, nIndex);

	Object result;
	result.push_back(Pair("regid", regID.ToString()));
	// result.push_back(Pair("script", HexStr(regID.GetVec6())));
	return result;
}

Value listcheckpoint(const Array& params, bool fHelp)
{
	if (fHelp || params.size() != 0) {
			throw runtime_error(
				"listcheckpoint index\n"
				"\nget the list of checkpoint.\n"
			    "\nResult a object  contain checkpoint\n"
//				"\nResult:\n"
//				"\"hash\"         (string) The block hash\n"
				"\nExamples:\n"
				+ HelpExampleCli("listcheckpoint", "")
				+ HelpExampleRpc("listcheckpoint", "")
			);
		}

	Object result;
	std::map<int, uint256> checkpointMap;
	Checkpoints::GetCheckpointMap(checkpointMap);
	for(std::map<int, uint256>::iterator iterCheck = checkpointMap.begin(); iterCheck != checkpointMap.end(); ++iterCheck){
		result.push_back(Pair(tfm::format("%d", iterCheck->first).c_str(), iterCheck->second.GetHex()));
	}
	return result;
}

Value invalidateblock(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "invalidateblock \"hash or height\"\n"
            "\nPermanently marks a block as invalid, as if it violated a consensus rule.\n"
            "\nArguments:\n"
            "1. \"hash or height\"(string or numeric, required) string for The block hash, numeric for the block height\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("invalidateblock", "\"hash or height\"")
            + HelpExampleRpc("invalidateblock", "\"hash or height\""));

    std::string strHash;
    if (int_type == params[0].type()) {
        int nHeight = params[0].get_int();
        if (nHeight < 0 || nHeight > chainActive.Height())
            throw runtime_error("Block number out of range.");

        CBlockIndex *pblockindex = chainActive[nHeight];
        strHash = pblockindex->GetBlockHash().GetHex();
    } else {
        strHash = params[0].get_str();
    }

    uint256 hash(uint256S(strHash));
    CValidationState state;

    {
        LOCK(cs_main);
        if (mapBlockIndex.count(hash) == 0)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

        CBlockIndex* pblockindex = mapBlockIndex[hash];
        InvalidateBlock(state, pblockindex);
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
