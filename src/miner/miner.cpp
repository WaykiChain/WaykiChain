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
#include "tx/multicoinblockrewardtx.h"
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

uint64_t nLastBlockTx   = 0;
uint64_t nLastBlockSize = 0;

MinedBlockInfo miningBlockInfo = MinedBlockInfo();
boost::circular_buffer<MinedBlockInfo> minedBlocks(kMaxMinedBlocks);
CCriticalSection csMinedBlocks;

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

bool GetCurrentDelegate(const int64_t currentTime, const vector<CRegID> &delegatesList, CRegID &delegate) {
    int64_t slot = currentTime / SysCfg().GetBlockInterval();
    int miner    = slot % IniCfg().GetTotalDelegateNum();
    delegate     = delegatesList[miner];
    LogPrint("DEBUG", "currentTime=%lld, slot=%d, miner=%d, regId=%s\n", currentTime, slot, miner,
             delegate.ToString());
    return true;
}

bool CreateBlockRewardTx(const int64_t currentTime, const CAccount &delegate, CAccountDBCache &accountCache,
                         CBlock *pBlock) {
    CBlock previousBlock;
    CBlockIndex *pBlockIndex = mapBlockIndex[pBlock->GetPrevBlockHash()];
    if (pBlock->GetHeight() != 1 || pBlock->GetPrevBlockHash() != SysCfg().GetGenesisBlockHash()) {
        if (!ReadBlockFromDisk(pBlockIndex, previousBlock))
            return ERRORMSG("read block info fail from disk");

        CAccount previousDelegate;
        if (!accountCache.GetAccount(previousBlock.vptx[0]->txUid, previousDelegate)) {
            return ERRORMSG("get preblock delegate account info error");
        }

        if (currentTime - previousBlock.GetBlockTime() < SysCfg().GetBlockInterval()) {
            if (previousDelegate.regId == delegate.regId)
                return ERRORMSG("one delegate can't produce more than one block at the same slot");
        }
    }

    if (pBlock->vptx[0]->nTxType == BLOCK_REWARD_TX) {
        auto pRewardTx     = (CBlockRewardTx *)pBlock->vptx[0].get();
        pRewardTx->txUid   = delegate.regId;
        pRewardTx->nHeight = pBlock->GetHeight();
    } else if (pBlock->vptx[0]->nTxType == MULTI_COIN_BLOCK_REWARD_TX) {
        auto pRewardTx     = (CMultiCoinBlockRewardTx *)pBlock->vptx[0].get();
        pRewardTx->txUid   = delegate.regId;
        pRewardTx->nHeight = pBlock->GetHeight();
        pRewardTx->profits = delegate.CalculateAccountProfit(pBlock->GetHeight());
    }

    pBlock->SetNonce(GetRand(SysCfg().GetBlockMaxNonce()));
    pBlock->SetMerkleRootHash(pBlock->BuildMerkleTree());
    pBlock->SetTime(currentTime);

    vector<unsigned char> signature;
    if (pWalletMain->Sign(delegate.keyId, pBlock->ComputeSignatureHash(), signature, delegate.minerPubKey.IsValid())) {
        pBlock->SetSignature(signature);
        return true;
    } else {
        return ERRORMSG("Sign failed");
    }
}

void ShuffleDelegates(const int nCurHeight, vector<CRegID> &delegatesList) {
    uint32_t TotalDelegateNum = IniCfg().GetTotalDelegateNum();
    string seedSource = strprintf("%u", nCurHeight / TotalDelegateNum + (nCurHeight % TotalDelegateNum > 0 ? 1 : 0));
    CHashWriter ss(SER_GETHASH, 0);
    ss << seedSource;
    uint256 currendSeed  = ss.GetHash();
    uint64_t currendTemp = 0;
    for (uint32_t i = 0, delCount = TotalDelegateNum; i < delCount; i++) {
        for (uint32_t x = 0; x < 4 && i < delCount; i++, x++) {
            memcpy(&currendTemp, currendSeed.begin() + (x * 8), 8);
            uint32_t newIndex       = currendTemp % delCount;
            CRegID regId            = delegatesList[newIndex];
            delegatesList[newIndex] = delegatesList[i];
            delegatesList[i]        = regId;
        }
        ss << currendSeed;
        currendSeed = ss.GetHash();
    }
}

bool VerifyPosTx(const CBlock *pBlock, CCacheWrapper &cwIn, bool bNeedRunTx) {
    uint64_t maxNonce = SysCfg().GetBlockMaxNonce();

    vector<CRegID> delegatesList;
    if (!cwIn.delegateCache.GetTopDelegates(delegatesList))
        return false;

    ShuffleDelegates(pBlock->GetHeight(), delegatesList);

    CRegID regId;
    if (!GetCurrentDelegate(pBlock->GetTime(), delegatesList, regId))
        return ERRORMSG("VerifyPosTx() : failed to get current delegate");
    CAccount curDelegate;
    if (!cwIn.accountCache.GetAccount(regId, curDelegate))
        return ERRORMSG("VerifyPosTx() : failed to get current delegate's account, regId=%s", regId.ToString());
    if (pBlock->GetNonce() > maxNonce)
        return ERRORMSG("VerifyPosTx() : invalid nonce: %u", pBlock->GetNonce());

    if (pBlock->GetMerkleRootHash() != pBlock->BuildMerkleTree())
        return ERRORMSG("VerifyPosTx() : wrong merkle root hash");

    auto spCW = std::make_shared<CCacheWrapper>();
    spCW->accountCache.SetBaseView(&cwIn.accountCache);
    spCW->txCache = cwIn.txCache;
    spCW->contractCache.SetBaseView(&cwIn.contractCache);

    CBlockIndex *pBlockIndex = mapBlockIndex[pBlock->GetPrevBlockHash()];
    if (pBlock->GetHeight() != 1 || pBlock->GetPrevBlockHash() != SysCfg().GetGenesisBlockHash()) {
        CBlock previousBlock;
        if (!ReadBlockFromDisk(pBlockIndex, previousBlock))
            return ERRORMSG("VerifyPosTx() : read block info failed from disk");

        CAccount previousDelegate;
        if (!spCW->accountCache.GetAccount(previousBlock.vptx[0]->txUid, previousDelegate))
            return ERRORMSG("VerifyPosTx() : failed to get previous delegate's account, regId=%s",
                previousBlock.vptx[0]->txUid.ToString());

        if (pBlock->GetBlockTime() - previousBlock.GetBlockTime() < SysCfg().GetBlockInterval()) {
            if (previousDelegate.regId == curDelegate.regId)
                return ERRORMSG("VerifyPosTx() : one delegate can't produce more than one block at the same slot");
        }
    }

    CAccount account;
    if (spCW->accountCache.GetAccount(pBlock->vptx[0]->txUid, account)) {
        if (curDelegate.regId != account.regId) {
            return ERRORMSG("VerifyPosTx() : delegate should be(%s) vs what we got(%s)", curDelegate.regId.ToString(),
                            account.regId.ToString());
        }

        const uint256 &blockHash                    = pBlock->ComputeSignatureHash();
        const vector<unsigned char> &blockSignature = pBlock->GetSignature();

        if (blockSignature.size() == 0 || blockSignature.size() > MAX_BLOCK_SIGNATURE_SIZE) {
            return ERRORMSG("VerifyPosTx() : invalid block signature size, hash=%s", blockHash.ToString());
        }

        if (!VerifySignature(blockHash, blockSignature, account.pubKey))
            if (!VerifySignature(blockHash, blockSignature, account.minerPubKey))
                return ERRORMSG("VerifyPosTx() : verify signature error");
    } else {
        return ERRORMSG("VerifyPosTx() : failed to get account info, regId=%s", pBlock->vptx[0]->txUid.ToString());
    }

    if (pBlock->vptx[0]->nVersion != nTxVersion1)
        return ERRORMSG("VerifyPosTx() : transaction version %d vs current %d", pBlock->vptx[0]->nVersion, nTxVersion1);

    if (bNeedRunTx) {
        uint64_t nTotalFuel(0);
        uint64_t nTotalRunStep(0);
        for (unsigned int i = 1; i < pBlock->vptx.size(); i++) {
            shared_ptr<CBaseTx> pBaseTx = pBlock->vptx[i];
            if (spCW->txCache.HaveTx(pBaseTx->GetHash()))
                return ERRORMSG("VerifyPosTx() : duplicate transaction, txid=%s", pBaseTx->GetHash().GetHex());

            spCW->txUndo.Clear();  // Clear first.
            CValidationState state;
            if (!pBaseTx->ExecuteTx(pBlock->GetHeight(), i, *spCW, state))
                return ERRORMSG("VerifyPosTx() : failed to execute transaction, txid=%s", pBaseTx->GetHash().GetHex());

            nTotalRunStep += pBaseTx->nRunStep;
            if (nTotalRunStep > MAX_BLOCK_RUN_STEP)
                return ERRORMSG("VerifyPosTx() : block total run steps(%lu) exceed max run step(%lu)", nTotalRunStep,
                                MAX_BLOCK_RUN_STEP);

            nTotalFuel += pBaseTx->GetFuel(pBlock->GetFuelRate());
            LogPrint("fuel", "VerifyPosTx() : total fuel:%d, tx fuel:%d runStep:%d fuelRate:%d txid:%s \n", nTotalFuel,
                     pBaseTx->GetFuel(pBlock->GetFuelRate()), pBaseTx->nRunStep, pBlock->GetFuelRate(),
                     pBaseTx->GetHash().GetHex());
        }

        if (nTotalFuel != pBlock->GetFuel())
            return ERRORMSG("VerifyPosTx() : total fuel(%lu) mismatch what(%u) in block header", nTotalFuel,
                            pBlock->GetFuel());
    }

    return true;
}

std::unique_ptr<CBlock> CreateNewBlock(CCacheWrapper &cwIn) {
    // Create new block
    std::unique_ptr<CBlock> pBlock(new CBlock());
    if (!pBlock.get())
        return nullptr;

    pBlock->vptx.push_back(std::make_shared<CBlockRewardTx>());
    // TODO: add softfork to enable price median transaction.
    // pBlock->vptx.push_back(std::make_shared<CBlockPriceMedianTx>());

    // Largest block you're willing to create:
    unsigned int nBlockMaxSize = SysCfg().GetArg("-blockmaxsize", DEFAULT_BLOCK_MAX_SIZE);
    // Limit to between 1K and MAX_BLOCK_SIZE-1K for sanity:
    nBlockMaxSize = std::max((unsigned int)1000, std::min((unsigned int)(MAX_BLOCK_SIZE - 1000), nBlockMaxSize));

    // How much of the block should be dedicated to high-priority transactions,
    // included regardless of the fees they pay
    unsigned int nBlockPrioritySize = SysCfg().GetArg("-blockprioritysize", DEFAULT_BLOCK_PRIORITY_SIZE);
    nBlockPrioritySize = std::min(nBlockMaxSize, nBlockPrioritySize);

    // Minimum block size you want to create; block will be filled with free transactions
    // until there are no more or the block reaches this size:
    unsigned int nBlockMinSize = SysCfg().GetArg("-blockminsize", DEFAULT_BLOCK_MIN_SIZE);
    nBlockMinSize              = std::min(nBlockMaxSize, nBlockMinSize);

    // Collect memory pool transactions into the block
    {
        LOCK2(cs_main, mempool.cs);

        CBlockIndex *pIndexPrev = chainActive.Tip();
        uint32_t nHeight        = pIndexPrev->nHeight + 1;
        int32_t nFuelRate       = GetElementForBurn(pIndexPrev);
        uint64_t nBlockSize     = ::GetSerializeSize(*pBlock, SER_NETWORK, PROTOCOL_VERSION);
        uint64_t nBlockTx       = 0;
        bool fSortedByFee       = true;
        uint64_t nTotalRunStep  = 0;
        int64_t nTotalFees      = 0;
        int64_t nTotalFuel      = 0;

        // Calculate && sort transactions from memory pool.
        vector<TxPriority> vTxPriority;
        GetPriorityTx(vTxPriority, nFuelRate);
        TxPriorityCompare comparer(fSortedByFee);
        make_heap(vTxPriority.begin(), vTxPriority.end(), comparer);

        // Collect transactions into the block.
        while (!vTxPriority.empty()) {
            // Take highest priority transaction off the priority queue.
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

            auto spCW = std::make_shared<CCacheWrapper>();
            spCW->accountCache.SetBaseView(&cwIn.accountCache);
            spCW->contractCache.SetBaseView(&cwIn.contractCache);

            CValidationState state;
            pBaseTx->nFuelRate = nFuelRate;
            if (!pBaseTx->ExecuteTx(nHeight, nBlockTx + 1, *spCW, state))
                continue;

            // Run step limits
            if (nTotalRunStep + pBaseTx->nRunStep >= MAX_BLOCK_RUN_STEP)
                continue;

            // Need to re-sync all to cache layer except for transaction cache, as it's depend on
            // the global transaction cache to verify whether a transaction(txid) has been confirmed
            // already in block.
            spCW->accountCache.Flush();
            spCW->contractCache.Flush();

            nTotalFees += pBaseTx->GetFee();
            nBlockSize += stx->GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION);
            nTotalRunStep += pBaseTx->nRunStep;
            nTotalFuel += pBaseTx->GetFuel(nFuelRate);
            ++nBlockTx;
            pBlock->vptx.push_back(stx);

            LogPrint("fuel", "miner total fuel:%d, tx fuel:%d runStep:%d fuelRate:%d txid:%s\n", nTotalFuel,
                     pBaseTx->GetFuel(nFuelRate), pBaseTx->nRunStep, nFuelRate, pBaseTx->GetHash().GetHex());
        }

        nLastBlockTx               = nBlockTx;
        nLastBlockSize             = nBlockSize;
        miningBlockInfo.nTxCount   = nBlockTx;
        miningBlockInfo.nBlockSize = nBlockSize;
        miningBlockInfo.nTotalFees = nTotalFees;

        assert(nTotalFees >= nTotalFuel);
        ((CBlockRewardTx *)pBlock->vptx[0].get())->rewardValue = nTotalFees - nTotalFuel;

        // Fill in header
        pBlock->SetPrevBlockHash(pIndexPrev->GetBlockHash());
        UpdateTime(*pBlock, pIndexPrev);
        pBlock->SetNonce(0);
        pBlock->SetHeight(nHeight);
        pBlock->SetFuel(nTotalFuel);
        pBlock->SetFuelRate(nFuelRate);


        LogPrint("INFO", "CreateNewBlock(): total size %u\n", nBlockSize);
    }

    return pBlock;
}

std::unique_ptr<CBlock> CreateFundCoinGenesisBlock() {
    // Create new block
    std::unique_ptr<CBlock> pBlock(new CBlock());
    if (!pBlock.get())
        return nullptr;

    {
        LOCK(cs_main);

        pBlock->vptx.push_back(std::make_shared<CBlockRewardTx>());
        SysCfg().CreateFundCoinAccountRegisterTx(pBlock->vptx, SysCfg().NetworkID());
        SysCfg().CreateFundCoinGenesisBlockRewardTx(pBlock->vptx, SysCfg().NetworkID());

        // Fill in header.
        CBlockIndex *pIndexPrev = chainActive.Tip();
        uint32_t nHeight        = pIndexPrev->nHeight + 1;
        int32_t nFuelRate       = GetElementForBurn(pIndexPrev);

        pBlock->SetPrevBlockHash(pIndexPrev->GetBlockHash());
        UpdateTime(*pBlock, pIndexPrev);
        pBlock->SetNonce(0);
        pBlock->SetHeight(nHeight);
        pBlock->SetFuel(0);
        pBlock->SetFuelRate(nFuelRate);
    }

    return pBlock;
}

bool CheckWork(CBlock *pBlock, CWallet &wallet) {
    // Print block information
    pBlock->Print(*pCdMan->pAccountCache);

    // Found a solution
    {
        LOCK(cs_main);
        if (pBlock->GetPrevBlockHash() != chainActive.Tip()->GetBlockHash())
            return ERRORMSG("CheckWork() : generated block is stale");

        // Process this block the same as if we had received it from another node
        CValidationState state;
        if (!ProcessBlock(state, NULL, pBlock))
            return ERRORMSG("CheckWork() : failed to process block");
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

        vector<CRegID> delegatesList;
        if (!cw.delegateCache.GetTopDelegates(delegatesList))
            return false;

        uint16_t nIndex = 0;
        for (auto &delegate : delegatesList)
            LogPrint("shuffle", "before shuffle: index=%d, regId=%s\n", nIndex++, delegate.ToString());

        ShuffleDelegates(pBlock->GetHeight(), delegatesList);

        nIndex = 0;
        for (auto &delegate : delegatesList)
            LogPrint("shuffle", "after shuffle: index=%d, regId=%s\n", nIndex++, delegate.ToString());

        int64_t currentTime = GetTime();
        CRegID regId;
        if (!GetCurrentDelegate(currentTime, delegatesList, regId))
            return false; // not on duty hence returns
        CAccount minerAcct;
        if (!cw.accountCache.GetAccount(regId, minerAcct))
            return false;

        bool success = false;
        int64_t nLastTime;
        {
            LOCK2(cs_main, pWalletMain->cs_wallet);
            if ((unsigned int)(chainActive.Tip()->nHeight + 1) != pBlock->GetHeight())
                return false;

            CKey acctKey;
            if (pWalletMain->GetKey(minerAcct.keyId.ToAddress(), acctKey, true) ||
                pWalletMain->GetKey(minerAcct.keyId.ToAddress(), acctKey)) {
                nLastTime = GetTimeMillis();
                success   = CreateBlockRewardTx(currentTime, minerAcct, cw.accountCache, pBlock);
                LogPrint("MINER", "CreateBlockRewardTx %s, used time:%d ms, miner address=%s\n",
                    success ? "success" : "failure", GetTimeMillis() - nLastTime, minerAcct.keyId.ToAddress());
            }
        }

        if (success) {
            SetThreadPriority(THREAD_PRIORITY_NORMAL);

            nLastTime = GetTimeMillis();
            CheckWork(pBlock, *pWallet);
            LogPrint("MINER", "CheckWork used time:%d ms\n", GetTimeMillis() - nLastTime);

            SetThreadPriority(THREAD_PRIORITY_LOWEST);

            miningBlockInfo.nTime         = pBlock->GetBlockTime();
            miningBlockInfo.nNonce        = pBlock->GetNonce();
            miningBlockInfo.nHeight       = pBlock->GetHeight();
            miningBlockInfo.nTotalFuels   = pBlock->GetFuel();
            miningBlockInfo.nFuelRate     = pBlock->GetFuelRate();
            miningBlockInfo.hash          = pBlock->GetHash();
            miningBlockInfo.hashPrevBlock = pBlock->GetHash();

            {
                LOCK(csMinedBlocks);
                minedBlocks.push_front(miningBlockInfo);
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
            spCW->txCache.SetBaseView(pCdMan->pTxCache);
            spCW->contractCache.SetBaseView(pCdMan->pContractCache);

            miningBlockInfo.SetNull();
            int64_t nLastTime = GetTimeMillis();

            auto pBlock = (pIndexPrev->nHeight + 1 == kFcoinGenesisTxHeight) ? CreateFundCoinGenesisBlock()
                                                                             : CreateNewBlock(*spCW);
            if (!pBlock.get())
                throw runtime_error("Create new block failed");

            LogPrint("MINER", "CreateNewBlock tx count: %d spent time: %d ms\n", pBlock->vptx.size(),
                     GetTimeMillis() - nLastTime);

            MineBlock(pBlock.get(), pWallet, pIndexPrev, nTransactionsUpdated, *spCW);

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

void MinedBlockInfo::SetNull() {
    nTime       = 0;
    nNonce      = 0;
    nHeight     = 0;
    nTotalFuels = 0;
    nFuelRate   = 0;
    nTotalFees  = 0;
    nTxCount    = 0;
    nBlockSize  = 0;
    hash.SetNull();
    hashPrevBlock.SetNull();
}

int64_t MinedBlockInfo::GetReward() { return nTotalFees - nTotalFuels; }

vector<MinedBlockInfo> GetMinedBlocks(unsigned int count) {
    std::vector<MinedBlockInfo> ret;
    LOCK(csMinedBlocks);
    count = std::min((unsigned int)minedBlocks.size(), count);
    for (unsigned int i = 0; i < count; i++) {
        ret.push_back(minedBlocks[i]);
    }

    return ret;
}
