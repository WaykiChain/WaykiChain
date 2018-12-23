// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2014-2016 The Coin developers
// Copyright (c) 2018 The WaykiChain Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "miner.h"

#include "core.h"
#include "main.h"
#include "tx.h"
#include "net.h"
#include "init.h"

#include "./wallet/wallet.h"
extern CWallet* pwalletMain;
extern void SetMinerStatus(bool bstatue );
//////////////////////////////////////////////////////////////////////////////
//
// CoinMiner
//

//int static FormatHashBlocks(void* pbuffer, unsigned int len) {
//	unsigned char* pdata = (unsigned char*) pbuffer;
//	unsigned int blocks = 1 + ((len + 8) / 64);
//	unsigned char* pend = pdata + 64 * blocks;
//	memset(pdata + len, 0, 64 * blocks - len);
//	pdata[len] = 0x80;
//	unsigned int bits = len * 8;
//	pend[-1] = (bits >> 0) & 0xff;
//	pend[-2] = (bits >> 8) & 0xff;
//	pend[-3] = (bits >> 16) & 0xff;
//	pend[-4] = (bits >> 24) & 0xff;
//	return blocks;
//}

static const unsigned int pSHA256InitState[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f,
		0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };

//void SHA256Transform(void* pstate, void* pinput, const void* pinit) {
//	SHA256_CTX ctx;
//	unsigned char data[64];
//
//	SHA256_Init(&ctx);
//
//	for (int i = 0; i < 16; i++)
//		((uint32_t*) data)[i] = ByteReverse(((uint32_t*) pinput)[i]);
//
//	for (int i = 0; i < 8; i++)
//		ctx.h[i] = ((uint32_t*) pinit)[i];
//
//	SHA256_Update(&ctx, data, sizeof(data));
//	for (int i = 0; i < 8; i++)
//		((uint32_t*) pstate)[i] = ctx.h[i];
//}

// Some explaining would be appreciated
class COrphan {
public:
	const CTransaction* ptx;
	set<uint256> setDependsOn;
	double dPriority;
	double dFeePerKb;

	COrphan(const CTransaction* ptxIn) {
		ptx = ptxIn;
		dPriority = dFeePerKb = 0;
	}

	void print() const {
		LogPrint("INFO", "COrphan(hash=%s, dPriority=%.1f, dFeePerKb=%.1f)\n", 
				ptx->GetHash().ToString(), dPriority, dFeePerKb);
		for (const auto& hash : setDependsOn)
			LogPrint("INFO", "   setDependsOn %s\n", hash.ToString());
	}
};

uint64_t nLastBlockTx = 0;    // 块中交易的总笔数,不含coinbase
uint64_t nLastBlockSize = 0;  //被创建的块 尺寸

//base on the last 50 blocks
int GetElementForBurn(CBlockIndex* pindex)
{
	if (NULL == pindex) {
		return INIT_FUEL_RATES;
	}
	int nBlock = SysCfg().GetArg("-blocksizeforburn", DEFAULT_BURN_BLOCK_SIZE);
	if (nBlock * 2 >= pindex->nHeight - 1) {
		return INIT_FUEL_RATES;
	} else {
		int64_t nTotalStep(0);
		int64_t nAverateStep(0);
		CBlockIndex * pTemp = pindex;
		for (int ii = 0; ii < nBlock; ii++) {
			nTotalStep += pTemp->nFuel / pTemp->nFuelRate * 100;
			pTemp = pTemp->pprev;
		}
		nAverateStep = nTotalStep / nBlock;
		int newFuelRate(0);
		if (nAverateStep < MAX_BLOCK_RUN_STEP * 0.75) {
			newFuelRate = pindex->nFuelRate * 0.9;
		} else if (nAverateStep > MAX_BLOCK_RUN_STEP * 0.85) {
			newFuelRate = pindex->nFuelRate * 1.1;
		} else {
			newFuelRate = pindex->nFuelRate;
		}
		if (newFuelRate < MIN_FUEL_RATES)
			newFuelRate = MIN_FUEL_RATES;
		LogPrint("fuel", "preFuelRate=%d fuelRate=%d, nHeight=%d\n", pindex->nFuelRate, newFuelRate, pindex->nHeight);
		return newFuelRate;
	}


}

// We want to sort transactions by priority and fee, so:

void GetPriorityTx(vector<TxPriority> &vecPriority, int nFuelRate) {
	vecPriority.reserve(mempool.mapTx.size());
	// Priority order to process transactions
	list<COrphan> vOrphan; // list memory doesn't move
	double dPriority = 0;
	for (map<uint256, CTxMemPoolEntry>::iterator mi = mempool.mapTx.begin(); mi != mempool.mapTx.end(); ++mi) {
		CBaseTransaction *pBaseTx = mi->second.GetTx().get();

		if (uint256() == std::move(pTxCacheTip->IsContainTx(std::move(pBaseTx->GetHash())))) {
			unsigned int nTxSize = ::GetSerializeSize(*pBaseTx, SER_NETWORK, PROTOCOL_VERSION);
			double dFeePerKb = double(pBaseTx->GetFee() - pBaseTx->GetFuel(nFuelRate))/ (double(nTxSize) / 1000.0);
			dPriority = 1000.0 / double(nTxSize);
			vecPriority.push_back(TxPriority(dPriority, dFeePerKb, mi->second.GetTx()));
		}

	}
}

void IncrementExtraNonce(CBlock* pblock, CBlockIndex* pindexPrev, unsigned int& nExtraNonce) {
	// Update nExtraNonce
	static uint256 hashPrevBlock;
	if (hashPrevBlock != pblock->GetHashPrevBlock()) {
		nExtraNonce = 0;
		hashPrevBlock = pblock->GetHashPrevBlock();
	}
	++nExtraNonce;
//	unsigned int nHeight = pindexPrev->nHeight + 1; // Height first in coinbase required for block.version=2
//    pblock->vtx[0].vin[0].scriptSig = (CScript() << nHeight << CBigNum(nExtraNonce)) + COINBASE_FLAGS;
//    assert(pblock->vtx[0].vin[0].scriptSig.size() <= 100);

	pblock->GetHashMerkleRoot() = pblock->BuildMerkleTree();
}

struct CAccountComparator {
	bool operator()(const CAccount &a, const CAccount&b) {
		CAccount &a1 = const_cast<CAccount &>(a);
		CAccount &b1 = const_cast<CAccount &>(b);
		if (a1.llVotes < b1.llVotes) {
			return false;
		}
		if(a1.llVotes > b1.llVotes) {
		    return true;
		}
		if(a1.llValues < b1.llValues) {
		    return false;
		}
		if(a1.llValues > b1.llValues) {
		    return false;
		}
		return false;
	}
};

uint256 GetAdjustHash(const uint256 TargetHash, const uint64_t nPos, const int nCurHeight) {

	uint64_t posacc = nPos/COIN;
	posacc /= SysCfg().GetIntervalPos();
	posacc = max(posacc, (uint64_t) 1);
	arith_uint256 adjusthash = UintToArith256(TargetHash); //adjust nbits
	arith_uint256 minhash = SysCfg().ProofOfWorkLimit();

	while (posacc) {
		adjusthash = adjusthash << 1;
		posacc = posacc >> 1;
		if (adjusthash > minhash) {
			adjusthash = minhash;
			break;
		}
	}

	return std::move(ArithToUint256(adjusthash));
}

bool GetDelegatesAcctList(vector<CAccount> &vDelegatesAcctList, CAccountViewCache &accViewIn, CTransactionDBCache &txCacheIn, CScriptDBViewCache &scriptCacheIn) {
    LOCK(cs_main);
    CAccountViewCache accview(accViewIn, true);
    CTransactionDBCache txCache(txCacheIn, true);
    CScriptDBViewCache scriptCache(scriptCacheIn, true);

    int nDelegateNum = IniCfg().GetDelegatesCfg();
    int nIndex = 0;
    vector<unsigned char> vScriptData;
    vector<unsigned char> vScriptKey = {'d', 'e', 'l', 'e', 'g', 'a', 't', 'e', '_'};
    vector<unsigned char> vDelegatePrefix = vScriptKey;
    const int SCRIPT_KEY_PREFIX_LENGTH = 9;
    const int VOTES_STRING_SIZE = 16;
    while (--nDelegateNum >= 0) {
        CRegID regId(0, 0);
        if (scriptCache.GetAppData(0, regId, nIndex, vScriptKey, vScriptData)) {
            nIndex = 1;
            vector<unsigned char>::iterator iterVotes = find_first_of(vScriptKey.begin(), vScriptKey.end(), vDelegatePrefix.begin(), vDelegatePrefix.end());
            string strVoltes(iterVotes + SCRIPT_KEY_PREFIX_LENGTH, iterVotes + SCRIPT_KEY_PREFIX_LENGTH + VOTES_STRING_SIZE);
            uint64_t llVotes = 0;
            char *stopstring;
            llVotes = strtoull(strVoltes.c_str(), &stopstring, VOTES_STRING_SIZE);
            vector<unsigned char> vAcctRegId(iterVotes + SCRIPT_KEY_PREFIX_LENGTH + VOTES_STRING_SIZE + 1, vScriptKey.end());
            CRegID acctRegId(vAcctRegId);
            CAccount account;
            if (!accview.GetAccount(acctRegId, account)) {
                LogPrint("ERROR", "GetAccount Error, acctRegId:%s\n", acctRegId.ToString());
                //assert(0);
                //StartShutdown();
                return false;
            }
            uint64_t maxNum = 0xFFFFFFFFFFFFFFFF;
            if ((maxNum - llVotes) != account.llVotes) {
                LogPrint("ERROR", "acctRegId:%s, llVotes:%lld, account:%s\n", acctRegId.ToString(), maxNum - llVotes, account.ToString());
                LogPrint("ERROR", "scriptkey:%s, scriptvalue:%s\n", HexStr(vScriptKey.begin(), vScriptKey.end()), HexStr(vScriptData.begin(), vScriptData.end()));
                //StartShutdown();
                return false;
            }
            vDelegatesAcctList.push_back(account);
        } else {
            StartShutdown();
            return false;
            //assert(0);
        }
    }
    return true;
}

bool GetDelegatesAcctList(vector<CAccount> & vDelegatesAcctList) {
    return GetDelegatesAcctList(vDelegatesAcctList, *pAccountViewTip, *pTxCacheTip, *pScriptDBTip);
}

bool GetCurrentDelegate(const int64_t currentTime,  const vector<CAccount> &vDelegatesAcctList, CAccount &delegateAcct) {
    int64_t snot =  currentTime / SysCfg().GetTargetSpacing();
    int miner = snot % IniCfg().GetDelegatesCfg();
    delegateAcct = vDelegatesAcctList[miner];
    LogPrint("INFO", "currentTime=%lld, snot=%d, miner=%d\n", currentTime, snot, miner);
    return true;
}

bool CreatePosTx(const int64_t currentTime, const CAccount &delegate, CAccountViewCache &view, CBlock *pBlock) {
    unsigned int nNonce = GetRand(SysCfg().GetBlockMaxNonce());
    CBlock preBlock;
    CBlockIndex* pblockindex = mapBlockIndex[pBlock->GetHashPrevBlock()];
    if(pBlock->GetHashPrevBlock() != SysCfg().HashGenesisBlock()) {
        if(!ReadBlockFromDisk(preBlock, pblockindex))
            return ERRORMSG("read block info fail from disk");

        CAccount preDelegate;
        CRewardTransaction *preBlockRewardTx = (CRewardTransaction *) preBlock.vptx[0].get();
        if(!view.GetAccount(preBlockRewardTx->account, preDelegate)) {
            return ERRORMSG("get preblock delegate account info error");
        }
        if(currentTime - preBlock.GetBlockTime() < SysCfg().GetTargetSpacing()) {
            if(preDelegate.regID == delegate.regID) {
                return ERRORMSG("one delegate can't produce more than one block at the same slot");
            }
        }
    }

    pBlock->SetNonce(nNonce);
    CRewardTransaction *prtx = (CRewardTransaction *) pBlock->vptx[0].get();
    prtx->account = delegate.regID;                                   //记账人账户ID
    prtx->nHeight = pBlock->GetHeight();
    pBlock->SetHashMerkleRoot(pBlock->BuildMerkleTree());
    pBlock->SetTime(currentTime);
    vector<unsigned char> vSign;
    if (pwalletMain->Sign(delegate.keyID, pBlock->SignatureHash(), vSign,
            delegate.MinerPKey.IsValid())) {
        pBlock->SetSignature(vSign);
        return true;
    } else {
        return false;
        LogPrint("ERROR", "sign fail\r\n");
    }
	return true;
}

void ShuffleDelegates(const int nCurHeight, vector<CAccount> &vDelegatesList) {

  int nDelegateCfg = IniCfg().GetDelegatesCfg();
  string seedSource = strprintf("%lld", nCurHeight / nDelegateCfg + (nCurHeight % nDelegateCfg > 0 ? 1 : 0));
  CHashWriter ss(SER_GETHASH, 0);
  ss << seedSource;
  uint256 currendSeed = ss.GetHash();
  uint64_t currendTemp(0);
  for (int i = 0, delCount = nDelegateCfg; i < delCount; i++) {
    for (int x = 0; x < 4 && i < delCount; i++, x++) {
      memcpy(&currendTemp, currendSeed.begin()+(x*8), 8);
      int newIndex = currendTemp % delCount;
      CAccount accountTemp = vDelegatesList[newIndex];
      vDelegatesList[newIndex] = vDelegatesList[i];
      vDelegatesList[i] = accountTemp;
    }
    ss << currendSeed;
    currendSeed = ss.GetHash();
  }

}

bool VerifyPosTx(const CBlock *pBlock, CAccountViewCache &accView, CTransactionDBCache &txCache, CScriptDBViewCache &scriptCache, bool bNeedRunTx) {

	uint64_t maxNonce = SysCfg().GetBlockMaxNonce(); //cacul times
	vector<CAccount> vDelegatesAcctList;

	if (!GetDelegatesAcctList(vDelegatesAcctList, accView, txCache, scriptCache))
		return false;

	ShuffleDelegates(pBlock->GetHeight(), vDelegatesAcctList);

	CAccount curDelegate;
	if(!GetCurrentDelegate(pBlock->GetTime(), vDelegatesAcctList, curDelegate)) {
	    return false;
	}

	if (pBlock->GetNonce() > maxNonce) {
		return ERRORMSG("Nonce is larger than maxNonce");
	}

	if (pBlock->GetHashMerkleRoot() != pBlock->BuildMerkleTree()) {
		return ERRORMSG("hashMerkleRoot is error");
	}

	CAccountViewCache view(accView, true);
	CScriptDBViewCache scriptDBView(scriptCache, true);
	CBlock preBlock;

	CBlockIndex* pblockindex = mapBlockIndex[pBlock->GetHashPrevBlock()];
	if(pBlock->GetHashPrevBlock() != SysCfg().HashGenesisBlock()) {
        if(!ReadBlockFromDisk(preBlock, pblockindex))
            return ERRORMSG("read block info fail from disk");

        CAccount preDelegate;
        CRewardTransaction *preBlockRewardTx = (CRewardTransaction *) preBlock.vptx[0].get();
        if(!view.GetAccount(preBlockRewardTx->account, preDelegate)) {
            return ERRORMSG("get preblock delegate account info error");
        }
        if(pBlock->GetBlockTime()-preBlock.GetBlockTime() < SysCfg().GetTargetSpacing()) {
             if(preDelegate.regID == curDelegate.regID) {
                return ERRORMSG("one delegate can't produce more than one block at the same slot");
             }
        }
	}
	CAccount account;
	CRewardTransaction *prtx = (CRewardTransaction *) pBlock->vptx[0].get();
	if (view.GetAccount(prtx->account, account)) {
	    if(curDelegate.regID != account.regID) {
	        return ERRORMSG("Verify delegate account error, delegate regid=%s vs reward regid=%s!",
	                curDelegate.regID.ToString(), account.regID.ToString());;
	    }

		if(!CheckSignScript(pBlock->SignatureHash(), pBlock->GetSignature(), account.PublicKey)) {
			if (!CheckSignScript(pBlock->SignatureHash(), pBlock->GetSignature(), account.MinerPKey)) {
//				LogPrint("ERROR", "block verify fail\r\n");
//				LogPrint("ERROR", "block hash:%s\n", pBlock->GetHash().GetHex());
//				LogPrint("ERROR", "signature block:%s\n", HexStr(pBlock->vSignature.begin(), pBlock->vSignature.end()));
				return ERRORMSG("Verify miner publickey signature error");
			}
		}
	} else {
		return ERRORMSG("AccountView have no the accountid");
	}

	if (prtx->nVersion != nTxVersion1) {
		return ERRORMSG("CTransaction CheckTransaction,tx version is not equal current version, (tx version %d: vs current %d)",
				prtx->nVersion, nTxVersion1);
	}

	if (bNeedRunTx) {
		int64_t nTotalFuel(0);
		uint64_t nTotalRunStep(0);
		for (unsigned int i = 1; i < pBlock->vptx.size(); i++) {
			shared_ptr<CBaseTransaction> pBaseTx = pBlock->vptx[i];
			if (uint256() != txCache.IsContainTx(pBaseTx->GetHash())) {
				return ERRORMSG("VerifyPosTx duplicate tx hash:%s", pBaseTx->GetHash().GetHex());
			}
			CTxUndo txundo;
			CValidationState state;
			if (CONTRACT_TX == pBaseTx->nTxType) {
				LogPrint("vm", "tx hash=%s VerifyPosTx run contract\n", pBaseTx->GetHash().GetHex());
			}
			pBaseTx->nFuelRate = pBlock->GetFuelRate();
			if (!pBaseTx->ExecuteTx(i, view, state, txundo, pBlock->GetHeight(), txCache, scriptDBView)) {
				return ERRORMSG("transaction UpdateAccount account error");
			}
			nTotalRunStep += pBaseTx->nRunStep;
			if(nTotalRunStep > MAX_BLOCK_RUN_STEP) {
				return ERRORMSG("block total run steps exceed max run step");
			}

			nTotalFuel += pBaseTx->GetFuel(pBlock->GetFuelRate());
			LogPrint("fuel", "VerifyPosTx total fuel:%d, tx fuel:%d runStep:%d fuelRate:%d txhash:%s \n",nTotalFuel, pBaseTx->GetFuel(pBlock->GetFuelRate()), pBaseTx->nRunStep, pBlock->GetFuelRate(), pBaseTx->GetHash().GetHex());
		}

		if(nTotalFuel != pBlock->GetFuel()) {
			return ERRORMSG("fuel value at block header calculate error");
		}
	}
	return true;
}

CBlockTemplate* CreateNewBlock(CAccountViewCache &view, CTransactionDBCache &txCache, CScriptDBViewCache &scriptCache){

	//    // Create new block
		auto_ptr<CBlockTemplate> pblocktemplate(new CBlockTemplate());
		if (!pblocktemplate.get())
			return NULL;
		CBlock *pblock = &pblocktemplate->block; // pointer for convenience

		// Create coinbase tx
		CRewardTransaction rtx;

		// Add our coinbase tx as first transaction
		pblock->vptx.push_back(std::make_shared<CRewardTransaction>(rtx));
		pblocktemplate->vTxFees.push_back(-1); // updated at end
		pblocktemplate->vTxSigOps.push_back(-1); // updated at end

		// Largest block you're willing to create:
		unsigned int nBlockMaxSize = SysCfg().GetArg("-blockmaxsize", DEFAULT_BLOCK_MAX_SIZE);
		// Limit to betweeen 1K and MAX_BLOCK_SIZE-1K for sanity:
		nBlockMaxSize = max((unsigned int) 1000, min((unsigned int) (MAX_BLOCK_SIZE - 1000), nBlockMaxSize));

		// How much of the block should be dedicated to high-priority transactions,
		// included regardless of the fees they pay
		unsigned int nBlockPrioritySize = SysCfg().GetArg("-blockprioritysize", DEFAULT_BLOCK_PRIORITY_SIZE);
		nBlockPrioritySize = min(nBlockMaxSize, nBlockPrioritySize);

		// Minimum block size you want to create; block will be filled with free transactions
		// until there are no more or the block reaches this size:
		unsigned int nBlockMinSize = SysCfg().GetArg("-blockminsize", DEFAULT_BLOCK_MIN_SIZE);
		nBlockMinSize = min(nBlockMaxSize, nBlockMinSize);

		// Collect memory pool transactions into the block
		int64_t nFees = 0;
		{
			LOCK2(cs_main, mempool.cs);
			CBlockIndex* pIndexPrev = chainActive.Tip();
			pblock->SetFuelRate(GetElementForBurn(pIndexPrev));

			// This vector will be sorted into a priority queue:
			vector<TxPriority> vecPriority;
			GetPriorityTx(vecPriority, pblock->GetFuelRate());

			// Collect transactions into block
			uint64_t nBlockSize = ::GetSerializeSize(*pblock, SER_NETWORK, PROTOCOL_VERSION);
			uint64_t nBlockTx(0);
			bool fSortedByFee(true);
			uint64_t nTotalRunStep(0);
			int64_t  nTotalFuel(0);
			TxPriorityCompare comparer(fSortedByFee);
			make_heap(vecPriority.begin(), vecPriority.end(), comparer);

			while (!vecPriority.empty()) {
				// Take highest priority transaction off the priority queue:
				double dPriority = vecPriority.front().get<0>();
				double dFeePerKb = vecPriority.front().get<1>();
				shared_ptr<CBaseTransaction> stx = vecPriority.front().get<2>();
				CBaseTransaction *pBaseTx = stx.get();
				//const CTransaction& tx = *(vecPriority.front().get<2>());

				pop_heap(vecPriority.begin(), vecPriority.end(), comparer);
				vecPriority.pop_back();

				// Size limits
				unsigned int nTxSize = ::GetSerializeSize(*pBaseTx, SER_NETWORK, PROTOCOL_VERSION);
				if (nBlockSize + nTxSize >= nBlockMaxSize)
					continue;
				// Skip free transactions if we're past the minimum block size:
				if (fSortedByFee && (dFeePerKb < CTransaction::nMinRelayTxFee) && (nBlockSize + nTxSize >= nBlockMinSize))
					continue;

				// Prioritize by fee once past the priority size or we run out of high-priority
				// transactions:
				if (!fSortedByFee && ((nBlockSize + nTxSize >= nBlockPrioritySize) || !AllowFree(dPriority))) {
					fSortedByFee = true;
					comparer = TxPriorityCompare(fSortedByFee);
					make_heap(vecPriority.begin(), vecPriority.end(), comparer);
				}

				if(uint256() != std::move(txCache.IsContainTx(std::move(pBaseTx->GetHash())))) {
					LogPrint("INFO","CreateNewBlock duplicate tx\n");
					continue;
				}

				CTxUndo txundo;
				CValidationState state;
				if(pBaseTx->IsCoinBase()){
					ERRORMSG("TX type is coin base tx error......");
			//		assert(0); //never come here
				}
				if (CONTRACT_TX == pBaseTx->nTxType) {
					LogPrint("vm", "tx hash=%s CreateNewBlock run contract\n", pBaseTx->GetHash().GetHex());
				}
				CAccountViewCache viewTemp(view, true);
				CScriptDBViewCache scriptCacheTemp(scriptCache, true);
				pBaseTx->nFuelRate = pblock->GetFuelRate();
				if (!pBaseTx->ExecuteTx(nBlockTx + 1, viewTemp, state, txundo, pIndexPrev->nHeight + 1,
						txCache, scriptCacheTemp)) {
					continue;
				}
				// Run step limits
				if(nTotalRunStep + pBaseTx->nRunStep >= MAX_BLOCK_RUN_STEP)
					continue;
				assert(viewTemp.Flush());
				assert(scriptCacheTemp.Flush());
				nFees += pBaseTx->GetFee();
				nBlockSize += stx->GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION);
				nTotalRunStep += pBaseTx->nRunStep;
				nTotalFuel += pBaseTx->GetFuel(pblock->GetFuelRate());
				nBlockTx++;
				pblock->vptx.push_back(stx);
				LogPrint("fuel", "miner total fuel:%d, tx fuel:%d runStep:%d fuelRate:%d txhash:%s\n",nTotalFuel, pBaseTx->GetFuel(pblock->GetFuelRate()), pBaseTx->nRunStep, pblock->GetFuelRate(), pBaseTx->GetHash().GetHex());
			}

			nLastBlockTx = nBlockTx;
			nLastBlockSize = nBlockSize;
			LogPrint("INFO","CreateNewBlock(): total size %u\n", nBlockSize);

			assert(nFees-nTotalFuel >= 0);
			((CRewardTransaction*) pblock->vptx[0].get())->rewardValue = nFees - nTotalFuel;

			// Fill in header
			pblock->SetHashPrevBlock(pIndexPrev->GetBlockHash());
			UpdateTime(*pblock, pIndexPrev);
			pblock->SetNonce(0);
			pblock->SetHeight(pIndexPrev->nHeight + 1);
			pblock->SetFuel(nTotalFuel);
		}

		return pblocktemplate.release();
}

bool CheckWork(CBlock* pblock, CWallet& wallet) {


	//// debug print
//	LogPrint("INFO","proof-of-work found  \n  hash: %s  \ntarget: %s\n", hash.GetHex(), hashTarget.GetHex());
	pblock->print(*pAccountViewTip);
	// LogPrint("INFO","generated %s\n", FormatMoney(pblock->vtx[0].vout[0].nValue));

	// Found a solution
	{
		LOCK(cs_main);
		if (pblock->GetHashPrevBlock() != chainActive.Tip()->GetBlockHash())
			return ERRORMSG("CoinMiner : generated block is stale");

		// Remove key from key pool
	//	reservekey.KeepKey();

		// Track how many getdata requests this block gets
//		{
//			LOCK(wallet.cs_wallet);
//			wallet.mapRequestCount[pblock->GetHash()] = 0;
//		}

		// Process this block the same as if we had received it from another node
		CValidationState state;
		if (!ProcessBlock(state, NULL, pblock))
			return ERRORMSG("CoinMiner : ProcessBlock, block not accepted");
	}

	return true;
}

bool static MineBlock(CBlock *pblock, CWallet *pwallet,CBlockIndex* pindexPrev,unsigned int nTransactionsUpdatedLast,CAccountViewCache &view, CTransactionDBCache &txCache, CScriptDBViewCache &scriptCache){

	int64_t nStart = GetTime();

	unsigned int lasttime = 0xFFFFFFFF;
	while (true) {

		// Check for stop or if block needs to be rebuilt
		boost::this_thread::interruption_point();
		if (vNodes.empty() && SysCfg().NetworkID() != REGTEST_NET)
			return false;

		if (pindexPrev != chainActive.Tip())
			return false;

		//获取时间, 同时等待下次时间到
		auto GetNextTimeAndSleep = [&]() {
			while(GetTime() == lasttime  || (GetTime() - pindexPrev->GetBlockTime()) < SysCfg().GetTargetSpacing())
			{
				::MilliSleep(100);
			}
			return (lasttime = GetTime());
		};

		GetNextTimeAndSleep();	// max(pindexPrev->GetMedianTimePast() + 1, GetAdjustedTime());

        vector<CAccount> vDelegatesAcctList;
        if(!GetDelegatesAcctList(vDelegatesAcctList)) {
            return false;
        }
        int nIndex = 0;
        for(auto & acct : vDelegatesAcctList) {
            LogPrint("shuffle","before shuffle index=%d, address=%s\n", nIndex++, acct.keyID.ToAddress());
        }
        ShuffleDelegates(pblock->GetHeight(), vDelegatesAcctList);
        nIndex = 0;
        for(auto & acct : vDelegatesAcctList) {
            LogPrint("shuffle","after shuffle index=%d, address=%s\n", nIndex++, acct.keyID.ToAddress());
        }
        int64_t currentTime = GetTime();
        CAccount minerAcct;
        if(!GetCurrentDelegate(currentTime, vDelegatesAcctList, minerAcct)) {
            return false;
        }

//        LogPrint("INFO", "Current charger delegate address=%s\n", minerAcct.keyID.ToAddress());
        bool increatedfalg = false;
        int64_t lasttime1;
        {
            LOCK2(cs_main, pwalletMain->cs_wallet);
            if((unsigned int)(chainActive.Tip()->nHeight + 1) !=  pblock->GetHeight())
                return false;
            CKey acctKey;
            if(pwalletMain->GetKey(minerAcct.keyID.ToAddress(), acctKey, true) ||
                    pwalletMain->GetKey(minerAcct.keyID.ToAddress(), acctKey)) {
                lasttime1 = GetTimeMillis();
                increatedfalg = CreatePosTx(currentTime, minerAcct, view, pblock);
            }
        }
		if (increatedfalg == true) {
		    LogPrint("MINER","CreatePosTx used time:%d ms, miner address=%s\n", GetTimeMillis() - lasttime1, minerAcct.keyID.ToAddress());
			SetThreadPriority(THREAD_PRIORITY_NORMAL);
			{
				lasttime1 = GetTimeMillis();
				CheckWork(pblock, *pwallet);
				LogPrint("MINER","CheckWork used time:%d ms\n",   GetTimeMillis() - lasttime1);
			}
			SetThreadPriority(THREAD_PRIORITY_LOWEST);

			return true;
		}

		if (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast || GetTime() - nStart > 60)
				return false;
	}
	return false;
}

void static CoinMiner(CWallet *pwallet,int targetConter) {
	LogPrint("INFO","CoinMiner started.\n");

	SetThreadPriority(THREAD_PRIORITY_LOWEST);
	RenameThread("Coin-miner");

	auto CheckIsHaveMinerKey = [&]() {
		    LOCK2(cs_main, pwalletMain->cs_wallet);
			set<CKeyID> setMineKey;
			setMineKey.clear();
			pwalletMain->GetKeys(setMineKey, true);
			return !setMineKey.empty();
	};

	if (!CheckIsHaveMinerKey()) {
			LogPrint("INFO", "CoinMiner terminated.\n");
			ERRORMSG("ERROR:%s ", "no key for mining\n");
            return ;
	}

	auto getCurrHeight = [&]() {
		LOCK(cs_main);
		return chainActive.Height();
	};

	targetConter += getCurrHeight();

	try {
	       SetMinerStatus(true);
		while (true) {
			if (SysCfg().NetworkID() != REGTEST_NET) {
				// Busy-wait for the network to come online so we don't waste time mining
				// on an obsolete chain. In regtest mode we expect to fly solo.
				while (vNodes.empty() || (chainActive.Tip() && chainActive.Tip()->nHeight>1 && GetAdjustedTime()-chainActive.Tip()->nTime > 60*60))
					MilliSleep(1000);
			}

			//
			// Create new block
			//
			unsigned int LastTrsa = mempool.GetTransactionsUpdated();
			CBlockIndex* pindexPrev = chainActive.Tip();

			CAccountViewCache accview(*pAccountViewTip, true);
			CTransactionDBCache txCache(*pTxCacheTip, true);
			CScriptDBViewCache ScriptDbTemp(*pScriptDBTip, true);
			int64_t lasttime1 = GetTimeMillis();
			shared_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock(accview, txCache, ScriptDbTemp));
			if (!pblocktemplate.get()){
				throw runtime_error("Create new block fail.");
			}
			LogPrint("MINER", "CreateNewBlock tx count:%d used time :%d ms\n", pblocktemplate.get()->block.vptx.size(),
					GetTimeMillis() - lasttime1);
			CBlock *pblock = &pblocktemplate.get()->block;
			MineBlock(pblock, pwallet, pindexPrev, LastTrsa, accview, txCache, ScriptDbTemp);

			if (SysCfg().NetworkID() != MAIN_NET)
				if(targetConter <= getCurrHeight())	{
						throw boost::thread_interrupted();
				}
		}
	} catch (...) {
		LogPrint("INFO","CoinMiner  terminated\n");
    	SetMinerStatus(false);
		throw;
	}
}

void GenerateCoinBlock(bool fGenerate, CWallet* pwallet, int targetHeight) {
	static boost::thread_group* minerThreads = NULL;

	if (minerThreads != NULL) {
		minerThreads->interrupt_all();
//		minerThreads->join_all();
		delete minerThreads;
		minerThreads = NULL;
	}

	if(targetHeight <= 0 && fGenerate == true)
	{
//		assert(0);
		ERRORMSG("targetHeight, fGenerate value error");
		return ;
	}
	if (!fGenerate)
		return;
	//in pos system one thread is enough  marked by ranger.shi
	minerThreads = new boost::thread_group();
	minerThreads->create_thread(boost::bind(&CoinMiner, pwallet, targetHeight));

//	minerThreads->join_all();
}
