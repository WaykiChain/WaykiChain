// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2014-2016 The Coin developers
// Copyright (c) 2018 The WaykiChain Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "miner.h"

#include "core.h"
#include "init.h"
#include "main.h"
#include "net.h"
#include "tx.h"
#include <algorithm>
#include <boost/circular_buffer.hpp>

#include "./wallet/wallet.h"
extern CWallet *pwalletMain;
extern void SetMinerStatus(bool bStatus);
//////////////////////////////////////////////////////////////////////////////
//
// CoinMiner
//

//int static FormatHashBlocks(void* pbuffer, unsigned int len) {
//  unsigned char* pdata = (unsigned char*) pbuffer;
//  unsigned int blocks = 1 + ((len + 8) / 64);
//  unsigned char* pend = pdata + 64 * blocks;
//  memset(pdata + len, 0, 64 * blocks - len);
//  pdata[len] = 0x80;
//  unsigned int bits = len * 8;
//  pend[-1] = (bits >> 0) & 0xff;
//  pend[-2] = (bits >> 8) & 0xff;
//  pend[-3] = (bits >> 16) & 0xff;
//  pend[-4] = (bits >> 24) & 0xff;
//  return blocks;
//}

const int MINED_BLOCK_COUNT_MAX = 100; // the max count of mined blocks will be cached

static const unsigned int pSHA256InitState[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f,
                                                 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

uint64_t nLastBlockTx   = 0;  // 块中交易的总笔数,不含coinbase
uint64_t nLastBlockSize = 0;  // 被创建的块的尺寸

MinedBlockInfo g_miningBlockInfo = MinedBlockInfo();
boost::circular_buffer<MinedBlockInfo> g_minedBlocks(MINED_BLOCK_COUNT_MAX);
CCriticalSection g_csMinedBlocks;

//base on the last 50 blocks
int GetElementForBurn(CBlockIndex *pindex) {
    if (NULL == pindex) {
        return INIT_FUEL_RATES;
    }
    int nBlock = SysCfg().GetArg("-blocksizeforburn", DEFAULT_BURN_BLOCK_SIZE);
    if (nBlock * 2 >= pindex->nHeight - 1) {
        return INIT_FUEL_RATES;
    } else {
        int64_t nTotalStep(0);
        int64_t nAverateStep(0);
        CBlockIndex *pTemp = pindex;
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
    static double dPriority     = 0;
    static double dFeePerKb     = 0;
    static unsigned int nTxSize = 0;
    for (map<uint256, CTxMemPoolEntry>::iterator mi = mempool.mapTx.begin(); mi != mempool.mapTx.end(); ++mi) {
        CBaseTx *pBaseTx = mi->second.GetTx().get();
        if (!pBaseTx->IsCoinBase() && uint256() == pTxCacheTip->HasTx(pBaseTx->GetHash())) {
            nTxSize   = ::GetSerializeSize(*pBaseTx, SER_NETWORK, PROTOCOL_VERSION);
            dFeePerKb = double(pBaseTx->GetFee() - pBaseTx->GetFuel(nFuelRate)) / (double(nTxSize) / 1000.0);
            dPriority = 1000.0 / double(nTxSize);
            vecPriority.push_back(TxPriority(dPriority, dFeePerKb, mi->second.GetTx()));
        }
    }
}

void IncrementExtraNonce(CBlock *pblock, CBlockIndex *pindexPrev, unsigned int &nExtraNonce) {
    // Update nExtraNonce
    static uint256 hashPrevBlock;
    if (hashPrevBlock != pblock->GetHashPrevBlock()) {
        nExtraNonce   = 0;
        hashPrevBlock = pblock->GetHashPrevBlock();
    }
    ++nExtraNonce;
    //  unsigned int nHeight = pindexPrev->nHeight + 1; // Height first in coinbase required for block.version=2
    //    pblock->vtx[0].vin[0].scriptSig = (CScript() << nHeight << CBigNum(nExtraNonce)) + COINBASE_FLAGS;
    //    assert(pblock->vtx[0].vin[0].scriptSig.size() <= 100);

    pblock->GetHashMerkleRoot() = pblock->BuildMerkleTree();
}

bool GetDelegatesAcctList(vector<CAccount> &vDelegatesAcctList, CAccountViewCache &accViewIn, CScriptDBViewCache &scriptCacheIn) {
    LOCK(cs_main);
    CAccountViewCache accView(accViewIn, true);
    CScriptDBViewCache scriptCache(scriptCacheIn, true);

    int nDelegateNum = IniCfg().GetDelegatesNum();
    int nIndex       = 0;
    vector<unsigned char> vScriptData;
    vector<unsigned char> vScriptKey      = {'d', 'e', 'l', 'e', 'g', 'a', 't', 'e', '_'};
    vector<unsigned char> vDelegatePrefix = vScriptKey;
    const int SCRIPT_KEY_PREFIX_LENGTH    = 9;
    const int VOTES_STRING_SIZE           = 16;
    while (--nDelegateNum >= 0) {
        CRegID regId(0, 0);
        if (scriptCache.GetContractData(0, regId, nIndex, vScriptKey, vScriptData)) {
            nIndex                                    = 1;
            vector<unsigned char>::iterator iterVotes = find_first_of(vScriptKey.begin(), vScriptKey.end(), vDelegatePrefix.begin(), vDelegatePrefix.end());
            string strVoltes(iterVotes + SCRIPT_KEY_PREFIX_LENGTH, iterVotes + SCRIPT_KEY_PREFIX_LENGTH + VOTES_STRING_SIZE);
            uint64_t llVotes = 0;
            char *stopstring;
            llVotes = strtoull(strVoltes.c_str(), &stopstring, VOTES_STRING_SIZE);
            vector<unsigned char> vAcctRegId(iterVotes + SCRIPT_KEY_PREFIX_LENGTH + VOTES_STRING_SIZE + 1, vScriptKey.end());
            CRegID acctRegId(vAcctRegId);
            CAccount account;
            if (!accView.GetAccount(acctRegId, account)) {
                LogPrint("ERROR", "GetAccount Error, acctRegId:%s\n", acctRegId.ToString());
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
        }
    }
    return true;
}

bool GetDelegatesAcctList(vector<CAccount> &vDelegatesAcctList) {
    return GetDelegatesAcctList(vDelegatesAcctList, *pAccountViewTip, *pScriptDBTip);
}

bool GetCurrentDelegate(const int64_t currentTime, const vector<CAccount> &vDelegatesAcctList, CAccount &delegateAcct) {
    int64_t slot = currentTime / SysCfg().GetBlockInterval();
    int miner    = slot % IniCfg().GetDelegatesNum();
    delegateAcct = vDelegatesAcctList[miner];
    LogPrint("DEBUG", "currentTime=%lld, slot=%d, miner=%d, minderAddr=%s\n",
        currentTime, slot, miner, delegateAcct.keyID.ToAddress());
    return true;
}

bool CreatePosTx(const int64_t currentTime, const CAccount &delegate, CAccountViewCache &view, CBlock *pBlock) {
    unsigned int nNonce = GetRand(SysCfg().GetBlockMaxNonce());
    CBlock preBlock;
    CBlockIndex *pblockindex = mapBlockIndex[pBlock->GetHashPrevBlock()];
    if (pBlock->GetHashPrevBlock() != SysCfg().HashGenesisBlock()) {
        if (!ReadBlockFromDisk(preBlock, pblockindex))
            return ERRORMSG("read block info fail from disk");

        CAccount preDelegate;
        CRewardTx *preBlockRewardTx = (CRewardTx *)preBlock.vptx[0].get();
        if (!view.GetAccount(preBlockRewardTx->account, preDelegate)) {
            return ERRORMSG("get preblock delegate account info error");
        }
        if (currentTime - preBlock.GetBlockTime() < SysCfg().GetBlockInterval()) {
            if (preDelegate.regID == delegate.regID)
                return ERRORMSG("one delegate can't produce more than one block at the same slot");
        }
    }

    pBlock->SetNonce(nNonce);
    CRewardTx *prtx = (CRewardTx *)pBlock->vptx[0].get();
    prtx->account            = delegate.regID;  //记账人账户ID
    prtx->nHeight            = pBlock->GetHeight();
    pBlock->SetHashMerkleRoot(pBlock->BuildMerkleTree());
    pBlock->SetTime(currentTime);

    vector<unsigned char> vSign;
    if (pwalletMain->Sign(delegate.keyID, pBlock->SignatureHash(), vSign, delegate.MinerPKey.IsValid())) {
        pBlock->SetSignature(vSign);
        return true;
    } else {
        return false;
    }
    return true;
}

void ShuffleDelegates(const int nCurHeight, vector<CAccount> &vDelegatesList) {
    int nDelegatesNum = IniCfg().GetDelegatesNum();
    string seedSource = strprintf("%lld", nCurHeight / nDelegatesNum + (nCurHeight % nDelegatesNum > 0 ? 1 : 0));
    CHashWriter ss(SER_GETHASH, 0);
    ss << seedSource;
    uint256 currendSeed = ss.GetHash();
    uint64_t currendTemp(0);
    for (int i = 0, delCount = nDelegatesNum; i < delCount; i++) {
        for (int x = 0; x < 4 && i < delCount; i++, x++) {
            memcpy(&currendTemp, currendSeed.begin() + (x * 8), 8);
            int newIndex             = currendTemp % delCount;
            CAccount accountTemp     = vDelegatesList[newIndex];
            vDelegatesList[newIndex] = vDelegatesList[i];
            vDelegatesList[i]        = accountTemp;
        }
        ss << currendSeed;
        currendSeed = ss.GetHash();
    }
}

bool VerifyPosTx(const CBlock *pBlock, CAccountViewCache &accView, CTransactionDBCache &txCache,
                 CScriptDBViewCache &scriptCache, bool bNeedRunTx) {
    uint64_t maxNonce = SysCfg().GetBlockMaxNonce();
    vector<CAccount> vDelegatesAcctList;

    if (!GetDelegatesAcctList(vDelegatesAcctList, accView, scriptCache))
        return false;

    ShuffleDelegates(pBlock->GetHeight(), vDelegatesAcctList);

    CAccount curDelegate;
    if (!GetCurrentDelegate(pBlock->GetTime(), vDelegatesAcctList, curDelegate))
        return false;

    if (pBlock->GetNonce() > maxNonce)
        return ERRORMSG("Nonce is larger than maxNonce");

    if (pBlock->GetHashMerkleRoot() != pBlock->BuildMerkleTree())
        return ERRORMSG("hashMerkleRoot is error");

    CAccountViewCache view(accView, true);
    CScriptDBViewCache scriptDBView(scriptCache, true);
    CBlock preBlock;

    CBlockIndex *pblockindex = mapBlockIndex[pBlock->GetHashPrevBlock()];
    if (pBlock->GetHashPrevBlock() != SysCfg().HashGenesisBlock()) {
        if (!ReadBlockFromDisk(preBlock, pblockindex))
            return ERRORMSG("read block info fail from disk");

        CAccount preDelegate;
        CRewardTx *preBlockRewardTx = (CRewardTx *)preBlock.vptx[0].get();
        if (!view.GetAccount(preBlockRewardTx->account, preDelegate))
            return ERRORMSG("get preblock delegate account info error");

        if (pBlock->GetBlockTime() - preBlock.GetBlockTime() < SysCfg().GetBlockInterval()) {
            if (preDelegate.regID == curDelegate.regID)
                return ERRORMSG("one delegate can't produce more than one block at the same slot");
        }
    }

    CAccount account;
    CRewardTx *prtx = (CRewardTx *)pBlock->vptx[0].get();
    if (view.GetAccount(prtx->account, account)) {
        if (curDelegate.regID != account.regID) {
            return ERRORMSG("Verify delegate account error, delegate regid=%s vs reward regid=%s!",
                curDelegate.regID.ToString(), account.regID.ToString());
        }

        const uint256 &blockHash = pBlock->SignatureHash();
        const vector<unsigned char> &blockSignature = pBlock->GetSignature();

        if (blockSignature.size() == 0 || blockSignature.size() > MAX_BLOCK_SIGNATURE_SIZE) {
            return ERRORMSG("Signature size of block invalid, hash=%s", blockHash.ToString());
        }

        if (!CheckSignScript(blockHash, blockSignature, account.PublicKey))
            if (!CheckSignScript(blockHash, blockSignature, account.MinerPKey))
                return ERRORMSG("Verify miner publickey signature error");
    } else {
        return ERRORMSG("AccountView has no accountid");
    }

    if (prtx->nVersion != nTxVersion1)
        return ERRORMSG("Verify tx version error, tx version %d: vs current %d",
            prtx->nVersion, nTxVersion1);

    if (bNeedRunTx) {
        int64_t nTotalFuel(0);
        uint64_t nTotalRunStep(0);
        for (unsigned int i = 1; i < pBlock->vptx.size(); i++) {
            shared_ptr<CBaseTx> pBaseTx = pBlock->vptx[i];
            if (uint256() != txCache.HasTx(pBaseTx->GetHash()))
                return ERRORMSG("VerifyPosTx duplicate tx hash:%s", pBaseTx->GetHash().GetHex());

            CTxUndo txundo;
            CValidationState state;
            if (CONTRACT_TX == pBaseTx->nTxType)
                LogPrint("vm", "tx hash=%s VerifyPosTx run contract\n", pBaseTx->GetHash().GetHex());

            pBaseTx->nFuelRate = pBlock->GetFuelRate();
            if (!pBaseTx->ExecuteTx(i, view, state, txundo, pBlock->GetHeight(), txCache, scriptDBView))
                return ERRORMSG("transaction UpdateAccount account error");

            nTotalRunStep += pBaseTx->nRunStep;
            if (nTotalRunStep > MAX_BLOCK_RUN_STEP)
                return ERRORMSG("block total run steps exceed max run step");

            nTotalFuel += pBaseTx->GetFuel(pBlock->GetFuelRate());
            LogPrint("fuel", "VerifyPosTx total fuel:%d, tx fuel:%d runStep:%d fuelRate:%d txhash:%s \n", nTotalFuel, pBaseTx->GetFuel(pBlock->GetFuelRate()), pBaseTx->nRunStep, pBlock->GetFuelRate(), pBaseTx->GetHash().GetHex());
        }

        if (nTotalFuel != pBlock->GetFuel())
            return ERRORMSG("fuel value at block header calculate error");
    }

    return true;
}

unique_ptr<CBlockTemplate> CreateNewBlock(CAccountViewCache &view, CTransactionDBCache &txCache,
                                          CScriptDBViewCache &scriptCache) {
    // Create new block
    unique_ptr<CBlockTemplate> pblocktemplate(new CBlockTemplate());
    if (!pblocktemplate.get())
        return NULL;

    CBlock *pblock = &pblocktemplate->block;  // pointer for convenience

    // Create coinbase tx
    CRewardTx rewardTx;

    // Add our coinbase tx as the first tx
    pblock->vptx.push_back(std::make_shared<CRewardTx>(rewardTx));
    pblocktemplate->vTxFees.push_back(-1);    // updated at end
    pblocktemplate->vTxSigOps.push_back(-1);  // updated at end

    // Largest block you're willing to create:
    unsigned int nBlockMaxSize = SysCfg().GetArg("-blockmaxsize", DEFAULT_BLOCK_MAX_SIZE);
    // Limit to betweeen 1K and MAX_BLOCK_SIZE-1K for sanity:
    nBlockMaxSize = max((unsigned int)1000, min((unsigned int)(MAX_BLOCK_SIZE - 1000), nBlockMaxSize));

    // How much of the block should be dedicated to high-priority transactions,
    // included regardless of the fees they pay
    unsigned int nBlockPrioritySize = SysCfg().GetArg("-blockprioritysize", DEFAULT_BLOCK_PRIORITY_SIZE);
    nBlockPrioritySize = min(nBlockMaxSize, nBlockPrioritySize);

    // Minimum block size you want to create; block will be filled with free transactions
    // until there are no more or the block reaches this size:
    unsigned int nBlockMinSize = SysCfg().GetArg("-blockminsize", DEFAULT_BLOCK_MIN_SIZE);
    nBlockMinSize              = min(nBlockMaxSize, nBlockMinSize);

    // Collect memory pool transactions into the block
    int64_t nFees = 0;
    {
        LOCK2(cs_main, mempool.cs);
        CBlockIndex *pIndexPrev = chainActive.Tip();
        pblock->SetFuelRate(GetElementForBurn(pIndexPrev));

        // This vector will be sorted into a priority queue:
        vector<TxPriority> vTxPriority;
        GetPriorityTx(vTxPriority, pblock->GetFuelRate());

        // Collect transactions into the block
        uint64_t nBlockSize = ::GetSerializeSize(*pblock, SER_NETWORK, PROTOCOL_VERSION);
        uint64_t nBlockTx(0);
        bool fSortedByFee(true);
        uint64_t nTotalRunStep(0);
        int64_t nTotalFuel(0);
        TxPriorityCompare comparer(fSortedByFee);
        make_heap(vTxPriority.begin(), vTxPriority.end(), comparer);

        while (!vTxPriority.empty()) {
            // Take highest priority transaction off the priority queue:
            double dFeePerKb        = vTxPriority.front().get<1>();
            shared_ptr<CBaseTx> stx = vTxPriority.front().get<2>();
            CBaseTx *pBaseTx        = stx.get();

            pop_heap(vTxPriority.begin(), vTxPriority.end(), comparer);
            vTxPriority.pop_back();

            // Size limits
            unsigned int nTxSize = ::GetSerializeSize(*pBaseTx, SER_NETWORK, PROTOCOL_VERSION);
            if (nBlockSize + nTxSize >= nBlockMaxSize)
                continue;

            // Skip free transactions if we're past the minimum block size:
            if ((dFeePerKb < CBaseTx::nMinRelayTxFee) && (nBlockSize + nTxSize >= nBlockMinSize))
                continue;

            CTxUndo txundo;
            CValidationState state;
            if (CONTRACT_TX == pBaseTx->nTxType)
                LogPrint("vm", "CreateNewBlock: contract tx hash=%s\n",
                         pBaseTx->GetHash().GetHex());

            CAccountViewCache viewTemp(view, true);
            CScriptDBViewCache scriptCacheTemp(scriptCache, true);
            pBaseTx->nFuelRate = pblock->GetFuelRate();
            if (!pBaseTx->ExecuteTx(nBlockTx + 1, viewTemp, state, txundo, pIndexPrev->nHeight + 1,
                                    txCache, scriptCacheTemp))
                continue;

            // Run step limits
            if (nTotalRunStep + pBaseTx->nRunStep >= MAX_BLOCK_RUN_STEP)
                continue;

            assert(viewTemp.Flush());
            assert(scriptCacheTemp.Flush());
            nFees += pBaseTx->GetFee();
            nBlockSize += stx->GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION);
            nTotalRunStep += pBaseTx->nRunStep;
            nTotalFuel += pBaseTx->GetFuel(pblock->GetFuelRate());
            nBlockTx++;
            pblock->vptx.push_back(stx);
            LogPrint("fuel", "miner total fuel:%d, tx fuel:%d runStep:%d fuelRate:%d txhash:%s\n",
                     nTotalFuel, pBaseTx->GetFuel(pblock->GetFuelRate()), pBaseTx->nRunStep,
                     pblock->GetFuelRate(), pBaseTx->GetHash().GetHex());
        }

        nLastBlockTx                 = nBlockTx;
        nLastBlockSize               = nBlockSize;
        g_miningBlockInfo.nTxCount   = nBlockTx;
        g_miningBlockInfo.nBlockSize = nBlockSize;
        g_miningBlockInfo.nTotalFees = nFees;

        assert(nFees >= nTotalFuel);
        ((CRewardTx *)pblock->vptx[0].get())->rewardValue = nFees - nTotalFuel;

        // Fill in header
        pblock->SetHashPrevBlock(pIndexPrev->GetBlockHash());
        UpdateTime(*pblock, pIndexPrev);
        pblock->SetNonce(0);
        pblock->SetHeight(pIndexPrev->nHeight + 1);
        pblock->SetFuel(nTotalFuel);

        LogPrint("INFO", "CreateNewBlock(): total size %u\n", nBlockSize);
    }

    return std::move(pblocktemplate);
}

bool CheckWork(CBlock *pblock, CWallet &wallet) {
    // Print block information
    pblock->Print(*pAccountViewTip);

    // Found a solution
    {
        LOCK(cs_main);
        if (pblock->GetHashPrevBlock() != chainActive.Tip()->GetBlockHash())
            return ERRORMSG("CoinMiner : generated block is stale");

        // Process this block the same as if we had received it from another node
        CValidationState state;
        if (!ProcessBlock(state, NULL, pblock))
            return ERRORMSG("CoinMiner : ProcessBlock, block not accepted");
    }

    return true;
}

bool static MineBlock(CBlock *pblock, CWallet *pwallet, CBlockIndex *pindexPrev, unsigned int nTransactionsUpdated,
                      CAccountViewCache &view, CTransactionDBCache &txCache, CScriptDBViewCache &scriptCache) {
    int64_t nStart = GetTime();

    unsigned int nLastTime = 0xFFFFFFFF;
    while (true) {
        // Check for stop or if block needs to be rebuilt
        boost::this_thread::interruption_point();
        if (vNodes.empty() && SysCfg().NetworkID() != REGTEST_NET)
            return false;

        if (pindexPrev != chainActive.Tip())
            return false;

        auto GetNextTimeAndSleep = [&]() {
            while (GetTime() == nLastTime || (GetTime() - pindexPrev->GetBlockTime()) < SysCfg().GetBlockInterval()) {
                ::MilliSleep(100);
            }
            return (nLastTime = GetTime());
        };

        GetNextTimeAndSleep();

        vector<CAccount> vDelegatesAcctList;
        if (!GetDelegatesAcctList(vDelegatesAcctList))
            return false;

        int nIndex = 0;
        for (auto &delegate : vDelegatesAcctList)
            LogPrint("shuffle", "before shuffle: index=%d, address=%s\n", nIndex++, delegate.keyID.ToAddress());

        ShuffleDelegates(pblock->GetHeight(), vDelegatesAcctList);

        nIndex = 0;
        for (auto &delegate : vDelegatesAcctList)
            LogPrint("shuffle", "after shuffle: index=%d, address=%s\n", nIndex++, delegate.keyID.ToAddress());

        int64_t currentTime = GetTime();
        CAccount minerAcct;
        if (!GetCurrentDelegate(currentTime, vDelegatesAcctList, minerAcct))
            return false;

        bool createdFlag = false;
        int64_t nLastTime;
        {
            LOCK2(cs_main, pwalletMain->cs_wallet);
            if ((unsigned int)(chainActive.Tip()->nHeight + 1) != pblock->GetHeight())
                return false;
            CKey acctKey;
            if (pwalletMain->GetKey(minerAcct.keyID.ToAddress(), acctKey, true) ||
                pwalletMain->GetKey(minerAcct.keyID.ToAddress(), acctKey)) {
                nLastTime   = GetTimeMillis();
                createdFlag = CreatePosTx(currentTime, minerAcct, view, pblock);
                LogPrint("MINER", "CreatePosTx %s, used time:%d ms, miner address=%s\n",
                    createdFlag ? "success" : "failure", GetTimeMillis() - nLastTime, minerAcct.keyID.ToAddress());
            }
        }

        if (createdFlag == true) {
            SetThreadPriority(THREAD_PRIORITY_NORMAL);
            {
                nLastTime = GetTimeMillis();
                CheckWork(pblock, *pwallet);
                LogPrint("MINER", "CheckWork used time:%d ms\n", GetTimeMillis() - nLastTime);
            }
            SetThreadPriority(THREAD_PRIORITY_LOWEST);

            g_miningBlockInfo.nTime = pblock->GetBlockTime();
            g_miningBlockInfo.nNonce = pblock->GetNonce();
            g_miningBlockInfo.nHeight = pblock->GetHeight();
            g_miningBlockInfo.nTotalFuels = pblock->GetFuel();
            g_miningBlockInfo.nFuelRate = pblock->GetFuelRate();
            g_miningBlockInfo.hash = pblock->GetHash();
            g_miningBlockInfo.hashPrevBlock = pblock->GetHash();

            {
                LOCK(g_csMinedBlocks);
                g_minedBlocks.push_front(g_miningBlockInfo);
            }

            return true;
        }

        if (mempool.GetTransactionsUpdated() != nTransactionsUpdated || GetTime() - nStart > 60)
            return false;
    }

    return false;
}

void static CoinMiner(CWallet *pwallet, int targetHeight) {
    LogPrint("INFO", "CoinMiner started.\n");

    SetThreadPriority(THREAD_PRIORITY_LOWEST);
    RenameThread("Coin-miner");

    auto HasMinerKey = [&]() {
        LOCK2(cs_main, pwalletMain->cs_wallet);

        set<CKeyID> setMineKey;
        setMineKey.clear();
        pwalletMain->GetKeys(setMineKey, true);
        return !setMineKey.empty();
    };

    if (!HasMinerKey()) {
        LogPrint("INFO", "CoinMiner terminated.\n");
        ERRORMSG("No key for mining");
        return;
    }

    auto GetCurrHeight = [&]() {
        LOCK(cs_main);
        return chainActive.Height();
    };

    targetHeight += GetCurrHeight();

    try {
        SetMinerStatus(true);

        while (true) {
            if (SysCfg().NetworkID() != REGTEST_NET) {
                // Busy-wait for the network to come online so we don't waste time mining
                // on an obsolete chain. In regtest mode we expect to fly solo.
                while ( vNodes.empty() ||
                        (chainActive.Tip() &&
                        chainActive.Tip()->nHeight > 1 &&
                        GetAdjustedTime() - chainActive.Tip()->nTime > 60 * 60 &&
                        !SysCfg().GetBoolArg("-genblockforce", false)) ) {
                    MilliSleep(1000);
                }
            }

            //
            // Create new block
            //
            unsigned int nTransactionsUpdated = mempool.GetTransactionsUpdated();
            CBlockIndex *pindexPrev           = chainActive.Tip();
            CAccountViewCache accountView(*pAccountViewTip, true);
            CTransactionDBCache txCache(*pTxCacheTip, true);
            CScriptDBViewCache scriptDB(*pScriptDBTip, true);
            g_miningBlockInfo.SetNull();

            int64_t nLastTime = GetTimeMillis();
            unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock(accountView, txCache, scriptDB));
            if (!pblocktemplate.get())
                throw runtime_error("Create new block failed");

            LogPrint("MINER", "CreateNewBlock tx count: %d spent time: %d ms\n",
                pblocktemplate.get()->block.vptx.size(), GetTimeMillis() - nLastTime);

            CBlock *pblock = &pblocktemplate.get()->block;
            MineBlock(pblock, pwallet, pindexPrev, nTransactionsUpdated, accountView, txCache, scriptDB);

            if (SysCfg().NetworkID() != MAIN_NET && targetHeight <= GetCurrHeight())
                throw boost::thread_interrupted();
        }
    } catch (...) {
        LogPrint("INFO", "CoinMiner terminated\n");
        SetMinerStatus(false);
        throw;
    }
}

void GenerateCoinBlock(bool fGenerate, CWallet *pwallet, int targetHeight) {
    static boost::thread_group *minerThreads = NULL;

    if (minerThreads != NULL) {
        minerThreads->interrupt_all();
        delete minerThreads;
        minerThreads = NULL;
    }

    if (!fGenerate)
        return;

    // In mainnet, coin miner should generate blocks continuously regardless of target height.
    if (SysCfg().NetworkID() != MAIN_NET && targetHeight <= 0) {
        ERRORMSG("targetHeight, fGenerate value error");
        return;
    }

    minerThreads = new boost::thread_group();
    minerThreads->create_thread(boost::bind(&CoinMiner, pwallet, targetHeight));
}


// class MinedBlock
void MinedBlockInfo::SetNull()
{
    nTime = 0;
    nNonce = 0;
    nHeight = 0;
    nTotalFuels = 0;
    nFuelRate = 0;
    nTotalFees = 0;
    nTxCount = 0;
    nBlockSize = 0;
    hash.SetNull();
    hashPrevBlock.SetNull();
}


int64_t MinedBlockInfo::GetReward()
{
    return nTotalFees - nTotalFuels;
}


std::vector<MinedBlockInfo> GetMinedBlocks(unsigned int count)
{
    std::vector<MinedBlockInfo> ret;
    LOCK(g_csMinedBlocks);
    count = std::min((unsigned int)g_minedBlocks.size(), count);
    for (unsigned int i = 0; i < count; i++) {
        ret.push_back(g_minedBlocks[i]);
    }
    return ret;
}
