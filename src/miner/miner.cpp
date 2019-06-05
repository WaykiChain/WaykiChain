// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "miner.h"

#include "init.h"
#include "main.h"
#include "net.h"
#include "wallet/wallet.h"
#include "tx/tx.h"
#include "tx/blockrewardtx.h"
#include "tx/blockpricemediantx.h"
#include "persistence/txdb.h"
#include "persistence/contractdb.h"
#include "persistence/cachewrapper.h"

#include <algorithm>
#include <boost/circular_buffer.hpp>

extern CWallet *pWalletMain;
extern void SetMinerStatus(bool bStatus);
//////////////////////////////////////////////////////////////////////////////
//
// CoinMiner
//

const int MINED_BLOCK_COUNT_MAX = 100; // the max count of mined blocks will be cached
uint64_t nLastBlockTx   = 0;  // 块中交易的总笔数,不含coinbase
uint64_t nLastBlockSize = 0;  // 被创建的块的尺寸

MinedBlockInfo g_miningBlockInfo = MinedBlockInfo();
boost::circular_buffer<MinedBlockInfo> g_minedBlocks(MINED_BLOCK_COUNT_MAX);
CCriticalSection g_csMinedBlocks;

//base on the last 50 blocks
int GetElementForBurn(CBlockIndex *pIndex) {
    if (NULL == pIndex) {
        return INIT_FUEL_RATES;
    }
    int nBlock = SysCfg().GetArg("-blocksizeforburn", DEFAULT_BURN_BLOCK_SIZE);
    if (nBlock * 2 >= pIndex->nHeight - 1) {
        return INIT_FUEL_RATES;
    }

    int64_t nTotalStep(0);
    int64_t nAverateStep(0);
    CBlockIndex *pTemp = pIndex;
    for (int ii = 0; ii < nBlock; ii++) {
        nTotalStep += pTemp->nFuel / pTemp->nFuelRate * 100;
        pTemp = pTemp->pprev;
    }
    nAverateStep = nTotalStep / nBlock;
    int newFuelRate(0);
    if (nAverateStep < MAX_BLOCK_RUN_STEP * 0.75) {
        newFuelRate = pIndex->nFuelRate * 0.9;
    } else if (nAverateStep > MAX_BLOCK_RUN_STEP * 0.85) {
        newFuelRate = pIndex->nFuelRate * 1.1;
    } else {
        newFuelRate = pIndex->nFuelRate;
    }
    if (newFuelRate < MIN_FUEL_RATES)
        newFuelRate = MIN_FUEL_RATES;

    LogPrint("fuel", "preFuelRate=%d fuelRate=%d, nHeight=%d\n", pIndex->nFuelRate, newFuelRate, pIndex->nHeight);
    return newFuelRate;
}

// Sort transactions by priority and fee to decide priority orders to process transactions.
void GetPriorityTx(vector<TxPriority> &vecPriority, int nFuelRate) {
    vecPriority.reserve(mempool.memPoolTxs.size());
    static double dPriority     = 0;
    static double dFeePerKb     = 0;
    static unsigned int nTxSize = 0;
    for (map<uint256, CTxMemPoolEntry>::iterator mi = mempool.memPoolTxs.begin(); mi != mempool.memPoolTxs.end(); ++mi) {
        CBaseTx *pBaseTx = mi->second.GetTx().get();
        if (!pBaseTx->IsCoinBase() && !pCdMan->pTxCache->HaveTx(pBaseTx->GetHash())) {
            nTxSize   = ::GetSerializeSize(*pBaseTx, SER_NETWORK, PROTOCOL_VERSION);
            dFeePerKb = double(pBaseTx->GetFee() - pBaseTx->GetFuel(nFuelRate)) / (double(nTxSize) / 1000.0);
            dPriority = 1000.0 / double(nTxSize);
            vecPriority.push_back(TxPriority(dPriority, dFeePerKb, mi->second.GetTx()));
        }
    }
}

void IncrementExtraNonce(CBlock *pBlock, CBlockIndex *pIndexPrev, unsigned int &nExtraNonce) {
    // Update nExtraNonce
    static uint256 hashPrevBlock;
    if (hashPrevBlock != pBlock->GetPrevBlockHash()) {
        nExtraNonce   = 0;
        hashPrevBlock = pBlock->GetPrevBlockHash();
    }
    ++nExtraNonce;

    pBlock->SetMerkleRootHash(pBlock->BuildMerkleTree());
}

bool GetDelegatesAcctList(vector<CAccount> &vDelegatesAcctList) {
    LOCK(cs_main);

    // TODO:
    // int TotalDelegateNum = IniCfg().GetTotalDelegateNum();
    // int nIndex       = 0;
    // vector<unsigned char> vScriptData;
    // vector<unsigned char> vScriptKey      = {'d', 'e', 'l', 'e', 'g', 'a', 't', 'e', '_'};
    // vector<unsigned char> vDelegatePrefix = vScriptKey;
    // const int SCRIPT_KEY_PREFIX_LENGTH    = 9;
    // const int VOTES_STRING_SIZE           = 16;
    // while (--TotalDelegateNum >= 0) {
    //     CRegID regId(0, 0);
    //     if (pCdMan->pContractCache->GetContractData(0, regId, nIndex, vScriptKey, vScriptData)) {
    //         nIndex                                    = 1;
    //         vector<unsigned char>::iterator iterVotes = find_first_of(vScriptKey.begin(), vScriptKey.end(), vDelegatePrefix.begin(), vDelegatePrefix.end());
    //         string strVoltes(iterVotes + SCRIPT_KEY_PREFIX_LENGTH, iterVotes + SCRIPT_KEY_PREFIX_LENGTH + VOTES_STRING_SIZE);
    //         uint64_t receivedVotes = 0;
    //         char *stopstring;
    //         receivedVotes = strtoull(strVoltes.c_str(), &stopstring, VOTES_STRING_SIZE);
    //         vector<unsigned char> vAcctRegId(iterVotes + SCRIPT_KEY_PREFIX_LENGTH + VOTES_STRING_SIZE + 1, vScriptKey.end());
    //         CRegID acctRegId(vAcctRegId);
    //         CAccount account;
    //         if (!pCdMan->pAccountCache->GetAccount(acctRegId, account)) {
    //             LogPrint("ERROR", "GetAccount Error, acctRegId:%s\n", acctRegId.ToString());
    //             // StartShutdown();
    //             return false;
    //         }
    //         uint64_t maxNum = 0xFFFFFFFFFFFFFFFF;
    //         if ((maxNum - receivedVotes) != account.receivedVotes) {
    //             LogPrint("ERROR", "acctRegId:%s, scriptkey:%s, scriptvalue:%s => receivedVotes:%lld, account:%s\n",
    //                      acctRegId.ToString(), HexStr(vScriptKey.begin(), vScriptKey.end()),
    //                      HexStr(vScriptData.begin(), vScriptData.end()), maxNum - receivedVotes, account.ToString());
    //             // StartShutdown();
    //             return false;
    //         }
    //         vDelegatesAcctList.push_back(account);
    //     } else {
    //         StartShutdown();
    //         return false;
    //     }
    // }
    return true;
}

bool GetCurrentDelegate(const int64_t currentTime, const vector<CAccount> &vDelegatesAcctList, CAccount &delegateAcct) {
    int64_t slot = currentTime / SysCfg().GetBlockInterval();
    int miner    = slot % IniCfg().GetTotalDelegateNum();
    delegateAcct = vDelegatesAcctList[miner];
    LogPrint("DEBUG", "currentTime=%lld, slot=%d, miner=%d, minderAddr=%s\n",
        currentTime, slot, miner, delegateAcct.keyID.ToAddress());
    return true;
}

bool CreateBlockRewardTx(const int64_t currentTime, const CAccount &delegate, CAccountCache &view, CBlock *pBlock) {
    CBlock preBlock;
    CBlockIndex *pBlockIndex = mapBlockIndex[pBlock->GetPrevBlockHash()];
    if (pBlock->GetHeight() != 1 || pBlock->GetPrevBlockHash() != SysCfg().GetGenesisBlockHash()) {
        if (!ReadBlockFromDisk(pBlockIndex, preBlock))
            return ERRORMSG("read block info fail from disk");

        CAccount preDelegate;
        CBlockRewardTx *preBlockRewardTx = (CBlockRewardTx *)preBlock.vptx[0].get();
        if (!view.GetAccount(preBlockRewardTx->txUid, preDelegate)) {
            return ERRORMSG("get preblock delegate account info error");
        }

        if (currentTime - preBlock.GetBlockTime() < SysCfg().GetBlockInterval()) {
            if (preDelegate.regID == delegate.regID)
                return ERRORMSG("one delegate can't produce more than one block at the same slot");
        }
    }

    CBlockRewardTx *pBlockRewardTx  = (CBlockRewardTx *)pBlock->vptx[0].get();
    pBlockRewardTx->txUid           = delegate.regID;
    pBlockRewardTx->nHeight         = pBlock->GetHeight();
    // Assign profits to the delegate account.
    pBlockRewardTx->rewardValue     += delegate.CalculateAccountProfit(pBlock->GetHeight());

    pBlock->SetNonce(GetRand(SysCfg().GetBlockMaxNonce()));
    pBlock->SetMerkleRootHash(pBlock->BuildMerkleTree());
    pBlock->SetTime(currentTime);

    vector<unsigned char> signature;
    if (pWalletMain->Sign(delegate.keyID, pBlock->ComputeSignatureHash(), signature, delegate.minerPubKey.IsValid())) {
        pBlock->SetSignature(signature);
        return true;
    } else {
        return ERRORMSG("Sign failed");
    }
}

void ShuffleDelegates(const int nCurHeight, vector<CAccount> &vDelegatesList) {
    int TotalDelegateNum = IniCfg().GetTotalDelegateNum();
    string seedSource = strprintf("%lld", nCurHeight / TotalDelegateNum + (nCurHeight % TotalDelegateNum > 0 ? 1 : 0));
    CHashWriter ss(SER_GETHASH, 0);
    ss << seedSource;
    uint256 currendSeed = ss.GetHash();
    uint64_t currendTemp(0);
    for (int i = 0, delCount = TotalDelegateNum; i < delCount; i++) {
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

bool VerifyPosTx(const CBlock *pBlock, CCacheWrapper &cwIn, bool bNeedRunTx) {
    uint64_t maxNonce = SysCfg().GetBlockMaxNonce();
    vector<CAccount> vDelegatesAcctList;

    if (!GetDelegatesAcctList(vDelegatesAcctList))
        return false;

    ShuffleDelegates(pBlock->GetHeight(), vDelegatesAcctList);

    CAccount curDelegate;
    if (!GetCurrentDelegate(pBlock->GetTime(), vDelegatesAcctList, curDelegate))
        return false;

    if (pBlock->GetNonce() > maxNonce)
        return ERRORMSG("Nonce is larger than maxNonce");

    if (pBlock->GetMerkleRootHash() != pBlock->BuildMerkleTree())
        return ERRORMSG("wrong merkleRootHash");

    CBlock preBlock;

    auto spCW = std::make_shared<CCacheWrapper>();
    spCW->accountCache.SetBaseView(&cwIn.accountCache);
    spCW->txCache = cwIn.txCache;
    spCW->contractCache.SetBaseView(&cwIn.contractCache);

    CBlockIndex *pBlockIndex = mapBlockIndex[pBlock->GetPrevBlockHash()];
    if (pBlock->GetHeight() != 1 || pBlock->GetPrevBlockHash() != SysCfg().GetGenesisBlockHash()) {
        if (!ReadBlockFromDisk(pBlockIndex, preBlock))
            return ERRORMSG("read block info failed from disk");

        CAccount preDelegate;
        CBlockRewardTx *preBlockRewardTx = (CBlockRewardTx *)preBlock.vptx[0].get();
        if (!spCW->accountCache.GetAccount(preBlockRewardTx->txUid, preDelegate))
            return ERRORMSG("get preblock delegate account info error");

        if (pBlock->GetBlockTime() - preBlock.GetBlockTime() < SysCfg().GetBlockInterval()) {
            if (preDelegate.regID == curDelegate.regID)
                return ERRORMSG("one delegate can't produce more than one block at the same slot");
        }
    }

    CAccount account;
    CBlockRewardTx *pRewardTx = (CBlockRewardTx *)pBlock->vptx[0].get();
    if (spCW->accountCache.GetAccount(pRewardTx->txUid, account)) {
        if (curDelegate.regID != account.regID) {
            return ERRORMSG("Verify delegate account error, delegate regid=%s vs reward regid=%s!",
                curDelegate.regID.ToString(), account.regID.ToString());
        }

        const uint256 &blockHash = pBlock->ComputeSignatureHash();
        const vector<unsigned char> &blockSignature = pBlock->GetSignature();

        if (blockSignature.size() == 0 || blockSignature.size() > MAX_BLOCK_SIGNATURE_SIZE) {
            return ERRORMSG("Signature size of block invalid, hash=%s", blockHash.ToString());
        }

        if (!VerifySignature(blockHash, blockSignature, account.pubKey))
            if (!VerifySignature(blockHash, blockSignature, account.minerPubKey))
                return ERRORMSG("Verify miner publickey signature error");
    } else {
        return ERRORMSG("AccountView has no accountId");
    }

    if (pRewardTx->nVersion != nTxVersion1)
        return ERRORMSG("Verify tx version error, tx version %d: vs current %d",
            pRewardTx->nVersion, nTxVersion1);

    if (bNeedRunTx) {
        int64_t nTotalFuel(0);
        uint64_t nTotalRunStep(0);
        for (unsigned int i = 1; i < pBlock->vptx.size(); i++) {
            shared_ptr<CBaseTx> pBaseTx = pBlock->vptx[i];
            if (spCW->txCache.HaveTx(pBaseTx->GetHash()))
                return ERRORMSG("VerifyPosTx duplicate tx hash:%s", pBaseTx->GetHash().GetHex());

            CValidationState state;
            if (CONTRACT_INVOKE_TX == pBaseTx->nTxType)
                LogPrint("vm", "tx hash=%s VerifyPosTx run contract\n", pBaseTx->GetHash().GetHex());

            pBaseTx->nFuelRate = pBlock->GetFuelRate();

            spCW->txUndo.Clear(); // Clear first.
            if (!pBaseTx->ExecuteTx(pBlock->GetHeight(), i, *spCW, state))
                return ERRORMSG("transaction UpdateAccount account error");

            nTotalRunStep += pBaseTx->nRunStep;
            if (nTotalRunStep > MAX_BLOCK_RUN_STEP)
                return ERRORMSG("block total run steps exceed max run step");

            nTotalFuel += pBaseTx->GetFuel(pBlock->GetFuelRate());
            LogPrint("fuel", "VerifyPosTx total fuel:%d, tx fuel:%d runStep:%d fuelRate:%d txid:%s \n",
                    nTotalFuel, pBaseTx->GetFuel(pBlock->GetFuelRate()),
                    pBaseTx->nRunStep, pBlock->GetFuelRate(), pBaseTx->GetHash().GetHex());
        }

        if (nTotalFuel != pBlock->GetFuel())
            return ERRORMSG("fuel value at block header calculate error");
    }

    return true;
}

unique_ptr<CBlockTemplate> CreateNewBlock(CCacheWrapper &cwIn) {
    // Create new block
    unique_ptr<CBlockTemplate> pBlockTemplate(new CBlockTemplate());
    if (!pBlockTemplate.get())
        return NULL;

    CBlock *pBlock = &pBlockTemplate->block;  // pointer for convenience

    // Create BlockReward tx
    CBlockRewardTx rewardTx;
    CBlockPriceMedianTx priceMedianTx;

    // Add our Block Reward tx as the first one
    pBlock->vptx.push_back(std::make_shared<CBlockRewardTx>(rewardTx));
    // TODO: add softfork to enable price median transaction.
    // pBlock->vptx.push_back(std::make_shared<CBlockPriceMedianTx>(priceMedianTx));
    pBlockTemplate->vTxFees.push_back(-1);    // updated at end
    pBlockTemplate->vTxSigOps.push_back(-1);  // updated at end

    // Largest block you're willing to create:
    unsigned int nBlockMaxSize = SysCfg().GetArg("-blockmaxsize", DEFAULT_BLOCK_MAX_SIZE);
    // Limit to between 1K and MAX_BLOCK_SIZE-1K for sanity:
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
        pBlock->SetFuelRate(GetElementForBurn(pIndexPrev));

        // This vector will be sorted into a priority queue:
        vector<TxPriority> vTxPriority;
        GetPriorityTx(vTxPriority, pBlock->GetFuelRate());

        // Collect transactions into the block
        uint64_t nBlockSize = ::GetSerializeSize(*pBlock, SER_NETWORK, PROTOCOL_VERSION);
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

            auto spCW           = std::make_shared<CCacheWrapper>();
            spCW->accountCache.SetBaseView(&cwIn.accountCache);
            spCW->contractCache.SetBaseView(&cwIn.contractCache);

            CValidationState state;
            pBaseTx->nFuelRate = pBlock->GetFuelRate();
            if (!pBaseTx->ExecuteTx(pIndexPrev->nHeight + 1, nBlockTx + 1, *spCW, state))
                continue;

            // Run step limits
            if (nTotalRunStep + pBaseTx->nRunStep >= MAX_BLOCK_RUN_STEP)
                continue;

            // Need to re-sync all to cache layer except for transaction cache, as it's depend on
            // the global transaction cache to verify whether a transaction(txid) has been confirmed
            // already in block.
            spCW->accountCache.Flush(&cwIn.accountCache);
            spCW->contractCache.Flush(&cwIn.contractCache);

            nFees += pBaseTx->GetFee();
            nBlockSize += stx->GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION);
            nTotalRunStep += pBaseTx->nRunStep;
            nTotalFuel += pBaseTx->GetFuel(pBlock->GetFuelRate());
            nBlockTx++;
            pBlock->vptx.push_back(stx);
            LogPrint("fuel", "miner total fuel:%d, tx fuel:%d runStep:%d fuelRate:%d txid:%s\n",
                     nTotalFuel, pBaseTx->GetFuel(pBlock->GetFuelRate()), pBaseTx->nRunStep,
                     pBlock->GetFuelRate(), pBaseTx->GetHash().GetHex());
        }

        nLastBlockTx                 = nBlockTx;
        nLastBlockSize               = nBlockSize;
        g_miningBlockInfo.nTxCount   = nBlockTx;
        g_miningBlockInfo.nBlockSize = nBlockSize;
        g_miningBlockInfo.nTotalFees = nFees;

        assert(nFees >= nTotalFuel);
        ((CBlockRewardTx *)pBlock->vptx[0].get())->rewardValue = nFees - nTotalFuel;

        // Fill in header
        pBlock->SetPrevBlockHash(pIndexPrev->GetBlockHash());
        UpdateTime(*pBlock, pIndexPrev);
        pBlock->SetNonce(0);
        pBlock->SetHeight(pIndexPrev->nHeight + 1);
        pBlock->SetFuel(nTotalFuel);

        LogPrint("INFO", "CreateNewBlock(): total size %u\n", nBlockSize);
    }

    return std::move(pBlockTemplate);
}

bool CheckWork(CBlock *pBlock, CWallet &wallet) {
    // Print block information
    pBlock->Print(*pCdMan->pAccountCache);

    // Found a solution
    {
        LOCK(cs_main);
        if (pBlock->GetPrevBlockHash() != chainActive.Tip()->GetBlockHash())
            return ERRORMSG("CoinMiner : generated block is stale");

        // Process this block the same as if we had received it from another node
        CValidationState state;
        if (!ProcessBlock(state, NULL, pBlock))
            return ERRORMSG("CoinMiner : ProcessBlock, block not accepted");
    }

    return true;
}

bool static MineBlock(CBlock *pBlock, CWallet *pWallet, CBlockIndex *pIndexPrev,
                        unsigned int nTransactionsUpdated, CCacheWrapper &cw) {
    int64_t nStart = GetTime();

    unsigned int nLastTime = 0xFFFFFFFF;
    while (true) {
        // Check for stop or if block needs to be rebuilt
        boost::this_thread::interruption_point();
        if (vNodes.empty() && SysCfg().NetworkID() != REGTEST_NET)
            return false;

        if (pIndexPrev != chainActive.Tip())
            return false;

        auto GetNextTimeAndSleep = [&]() {
            while (GetTime() == nLastTime || (GetTime() - pIndexPrev->GetBlockTime()) < SysCfg().GetBlockInterval()) {
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

        ShuffleDelegates(pBlock->GetHeight(), vDelegatesAcctList);

        nIndex = 0;
        for (auto &delegate : vDelegatesAcctList)
            LogPrint("shuffle", "after shuffle: index=%d, address=%s\n", nIndex++, delegate.keyID.ToAddress());

        int64_t currentTime = GetTime();
        CAccount minerAcct;
        if (!GetCurrentDelegate(currentTime, vDelegatesAcctList, minerAcct))
            return false; // not on duty hence returns

        bool success = false;
        int64_t nLastTime;
        {
            LOCK2(cs_main, pWalletMain->cs_wallet);
            if ((unsigned int)(chainActive.Tip()->nHeight + 1) != pBlock->GetHeight())
                return false;

            CKey acctKey;
            if (pWalletMain->GetKey(minerAcct.keyID.ToAddress(), acctKey, true) ||
                pWalletMain->GetKey(minerAcct.keyID.ToAddress(), acctKey)) {
                nLastTime = GetTimeMillis();
                success   = CreateBlockRewardTx(currentTime, minerAcct, cw.accountCache, pBlock);
                LogPrint("MINER", "CreateBlockRewardTx %s, used time:%d ms, miner address=%s\n",
                    success ? "success" : "failure", GetTimeMillis() - nLastTime, minerAcct.keyID.ToAddress());
            }
        }

        if (success) {
            SetThreadPriority(THREAD_PRIORITY_NORMAL);

            nLastTime = GetTimeMillis();
            CheckWork(pBlock, *pWallet);
            LogPrint("MINER", "CheckWork used time:%d ms\n", GetTimeMillis() - nLastTime);

            SetThreadPriority(THREAD_PRIORITY_LOWEST);

            g_miningBlockInfo.nTime         = pBlock->GetBlockTime();
            g_miningBlockInfo.nNonce        = pBlock->GetNonce();
            g_miningBlockInfo.nHeight       = pBlock->GetHeight();
            g_miningBlockInfo.nTotalFuels   = pBlock->GetFuel();
            g_miningBlockInfo.nFuelRate     = pBlock->GetFuelRate();
            g_miningBlockInfo.hash          = pBlock->GetHash();
            g_miningBlockInfo.hashPrevBlock = pBlock->GetHash();

            {
                LOCK(g_csMinedBlocks);
                g_minedBlocks.push_front(g_miningBlockInfo);
            }

            return true;
        }

        if (mempool.GetUpdatedTransactionNum() != nTransactionsUpdated || GetTime() - nStart > 60)
            return false;
    }

    return false;
}

void static CoinMiner(CWallet *pWallet, int targetHeight) {
    LogPrint("INFO", "CoinMiner started.\n");

    SetThreadPriority(THREAD_PRIORITY_LOWEST);
    RenameThread("Coin-miner");

    auto HaveMinerKey = [&]() {
        LOCK2(cs_main, pWalletMain->cs_wallet);

        set<CKeyID> setMineKey;
        setMineKey.clear();
        pWalletMain->GetKeys(setMineKey, true);
        return !setMineKey.empty();
    };

    if (!HaveMinerKey()) {
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
                while (vNodes.empty() || (chainActive.Tip() && chainActive.Tip()->nHeight > 1 &&
                                          GetAdjustedTime() - chainActive.Tip()->nTime > 60 * 60 &&
                                          !SysCfg().GetBoolArg("-genblockforce", false))) {
                    MilliSleep(1000);
                }
            }

            //
            // Create new block
            //
            unsigned int nTransactionsUpdated = mempool.GetUpdatedTransactionNum();
            CBlockIndex *pIndexPrev           = chainActive.Tip();

            auto spCW = std::make_shared<CCacheWrapper>();
            spCW->accountCache.SetBaseView(pCdMan->pAccountCache);
            spCW->txCache = *pCdMan->pTxCache;
            spCW->contractCache.SetBaseView(pCdMan->pContractCache);

            g_miningBlockInfo.SetNull();
            int64_t nLastTime = GetTimeMillis();
            unique_ptr<CBlockTemplate> pBlockTemplate(CreateNewBlock(*spCW));
            if (!pBlockTemplate.get())
                throw runtime_error("Create new block failed");

            LogPrint("MINER", "CreateNewBlock tx count: %d spent time: %d ms\n",
                pBlockTemplate.get()->block.vptx.size(), GetTimeMillis() - nLastTime);

            CBlock *pBlock = &pBlockTemplate.get()->block;
            MineBlock(pBlock, pWallet, pIndexPrev, nTransactionsUpdated, *spCW);

            if (SysCfg().NetworkID() != MAIN_NET && targetHeight <= GetCurrHeight())
                throw boost::thread_interrupted();
        }
    } catch (...) {
        LogPrint("INFO", "CoinMiner terminated\n");
        SetMinerStatus(false);
        throw;
    }
}

void GenerateCoinBlock(bool fGenerate, CWallet *pWallet, int targetHeight) {
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
        ERRORMSG("targetHeight <=0 (%d)", targetHeight);
        return;
    }

    minerThreads = new boost::thread_group();
    minerThreads->create_thread(boost::bind(&CoinMiner, pWallet, targetHeight));
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
