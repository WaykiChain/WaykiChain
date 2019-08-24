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


                                                                                                   \


extern CWallet *pWalletMain;
extern void SetMinerStatus(bool bStatus);
//////////////////////////////////////////////////////////////////////////////
//
// CoinMiner
//

uint64_t nLastBlockTx   = 0;
uint64_t nLastBlockSize = 0;

MinedBlockInfo miningBlockInfo;
boost::circular_buffer<MinedBlockInfo> minedBlocks(MAX_MINED_BLOCK_COUNT);
CCriticalSection csMinedBlocks;





// base on the last 50 blocks
uint32_t GetElementForBurn(CBlockIndex *pIndex) {
    if (!pIndex) {
        return INIT_FUEL_RATES;
    }
    int32_t nBlock = SysCfg().GetArg("-blocksizeforburn", DEFAULT_BURN_BLOCK_SIZE);
    if (nBlock * 2 >= pIndex->height - 1) {
        return INIT_FUEL_RATES;
    }

    uint64_t nTotalStep   = 0;
    uint64_t nAverateStep = 0;
    uint32_t newFuelRate  = 0;
    CBlockIndex *pTemp   = pIndex;
    for (int32_t i = 0; i < nBlock; ++i) {
        nTotalStep += pTemp->nFuel / pTemp->nFuelRate * 100;
        pTemp = pTemp->pprev;
    }

    nAverateStep = nTotalStep / nBlock;
    if (nAverateStep < MAX_BLOCK_RUN_STEP * 0.75) {
        newFuelRate = pIndex->nFuelRate * 0.9;
    } else if (nAverateStep > MAX_BLOCK_RUN_STEP * 0.85) {
        newFuelRate = pIndex->nFuelRate * 1.1;
    } else {
        newFuelRate = pIndex->nFuelRate;
    }

    if (newFuelRate < MIN_FUEL_RATES) {
        newFuelRate = MIN_FUEL_RATES;
    }

    LogPrint("fuel", "preFuelRate=%d fuelRate=%d, height=%d\n", pIndex->nFuelRate, newFuelRate, pIndex->height);
    return newFuelRate;
}

// Sort transactions by priority and fee to decide priority orders to process transactions.
void GetPriorityTx(vector<TxPriority> &vecPriority, const int32_t nFuelRate) {
    vecPriority.reserve(mempool.memPoolTxs.size());
    static double dPriority     = 0;
    static double dFeePerKb     = 0;
    static uint32_t nTxSize     = 0;
    static TokenSymbol coinType = SYMB::WUSD;
    static uint64_t nFees       = 0;

    uint64_t slideWindowBlockCount;
    pCdMan->pSysParamCache->GetParam(SysParamType::MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT, slideWindowBlockCount);
    int32_t height            = chainActive.Height();
    uint64_t bcoinMedianPrice = pCdMan->pPpCache->GetBcoinMedianPrice(height, slideWindowBlockCount);
    uint64_t fcoinMedianPrice = pCdMan->pPpCache->GetFcoinMedianPrice(height, slideWindowBlockCount);
    auto GetCoinMedianPrice   = [&](const TokenSymbol &coinType) -> uint64_t {
        if (coinType == SYMB::WICC)
            return bcoinMedianPrice;
        else if (coinType == SYMB::WGRT)
            return fcoinMedianPrice;
        else if (coinType == SYMB::WUSD)
            return 10000; // boosted by 10^4
        else
            return 0;
    };

    for (map<uint256, CTxMemPoolEntry>::iterator mi = mempool.memPoolTxs.begin(); mi != mempool.memPoolTxs.end(); ++mi) {
        CBaseTx *pBaseTx = mi->second.GetTransaction().get();
        if (!pBaseTx->IsBlockRewardTx() && !pCdMan->pTxCache->HaveTx(pBaseTx->GetHash())) {
            nTxSize   = mi->second.GetTxSize();
            coinType  = std::get<0>(mi->second.GetFees());
            nFees     = std::get<1>(mi->second.GetFees());
            dFeePerKb = double(GetCoinMedianPrice(coinType)) / kPercentBoost *
                        (nFees - pBaseTx->GetFuel(nFuelRate)) / (nTxSize / 1000.0);
            LogPrint("MINER", "GetPriority, medianPrice: %llu, nFees: %llu, fuel: %llu, nTxSize: %u\n",
                GetCoinMedianPrice(coinType), nFees, pBaseTx->GetFuel(nFuelRate), nTxSize);
            dPriority = mi->second.GetPriority();
            vecPriority.push_back(TxPriority(dPriority, dFeePerKb, mi->second.GetTransaction()));
        }
    }
}

static bool GetCurrentDelegate(const int64_t currentTime, const int32_t currHeight, const vector<CRegID> &delegateList,
                        CRegID &delegate) {
    uint32_t slot  = currentTime / GetBlockInterval(currHeight);
    uint32_t miner = slot % IniCfg().GetTotalDelegateNum();
    delegate       = delegateList[miner];
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

        CAccount prevDelegateAcct;
        if (!accountCache.GetAccount(previousBlock.vptx[0]->txUid, prevDelegateAcct)) {
            return ERRORMSG("get preblock delegate account info error");
        }

        if (currentTime - previousBlock.GetBlockTime() < GetBlockInterval(pBlock->GetHeight())) {
            if (prevDelegateAcct.regid == delegate.regid)
                return ERRORMSG("one delegate can't produce more than one block at the same slot");
        }
    }

    if (pBlock->vptx[0]->nTxType == BLOCK_REWARD_TX) {
        auto pRewardTx          = (CBlockRewardTx *)pBlock->vptx[0].get();
        pRewardTx->txUid        = delegate.regid;
        pRewardTx->valid_height = pBlock->GetHeight();

    } else if (pBlock->vptx[0]->nTxType == UCOIN_BLOCK_REWARD_TX) {
        auto pRewardTx          = (CUCoinBlockRewardTx *)pBlock->vptx[0].get();
        pRewardTx->txUid        = delegate.regid;
        pRewardTx->valid_height = pBlock->GetHeight();
        pRewardTx->profits      = delegate.ComputeBlockInflateInterest(pBlock->GetHeight());

        auto pPriceMedianTx          = (CBlockPriceMedianTx *)pBlock->vptx[1].get();
        pPriceMedianTx->txUid        = delegate.regid;
        pPriceMedianTx->valid_height = pBlock->GetHeight();
    }

    pBlock->SetNonce(GetRand(SysCfg().GetBlockMaxNonce()));
    pBlock->SetMerkleRootHash(pBlock->BuildMerkleTree());
    pBlock->SetTime(currentTime);

    vector<uint8_t> signature;
    if (pWalletMain->Sign(delegate.keyid, pBlock->ComputeSignatureHash(), signature, delegate.miner_pubkey.IsValid())) {
        pBlock->SetSignature(signature);
        return true;
    } else {
        return ERRORMSG("Sign failed");
    }
}

static void ShuffleDelegates(const int32_t nCurHeight, vector<CRegID> &delegateList) {
    uint32_t totalDelegateNum = IniCfg().GetTotalDelegateNum();
    string seedSource = strprintf("%u", nCurHeight / totalDelegateNum + (nCurHeight % totalDelegateNum > 0 ? 1 : 0));
    CHashWriter ss(SER_GETHASH, 0);
    ss << seedSource;
    uint256 currentSeed  = ss.GetHash();
    uint64_t newIndexSource = 0;
    for (uint32_t i = 0; i < totalDelegateNum; i++) {
        for (uint32_t x = 0; x < 4 && i < totalDelegateNum; i++, x++) {
            memcpy(&newIndexSource, currentSeed.begin() + (x * 8), 8);
            uint32_t newIndex      = newIndexSource % totalDelegateNum;
            CRegID regId           = delegateList[newIndex];
            delegateList[newIndex] = delegateList[i];
            delegateList[i]        = regId;
        }
        ss << currentSeed;
        currentSeed = ss.GetHash();
    }
}

bool VerifyPosTx(const CBlock *pBlock, CCacheWrapper &cwIn, bool bNeedRunTx) {
    uint32_t maxNonce = SysCfg().GetBlockMaxNonce();

    vector<CRegID> delegateList;
    if (!cwIn.delegateCache.GetTopDelegateList(delegateList))
        return false;

    ShuffleDelegates(pBlock->GetHeight(), delegateList);

    CRegID regId;
    if (!GetCurrentDelegate(pBlock->GetTime(), pBlock->GetHeight(), delegateList, regId))
        return ERRORMSG("VerifyPosTx() : failed to get current delegate");
    CAccount curDelegate;
    if (!cwIn.accountCache.GetAccount(regId, curDelegate))
        return ERRORMSG("VerifyPosTx() : failed to get current delegate's account, regId=%s", regId.ToString());
    if (pBlock->GetNonce() > maxNonce)
        return ERRORMSG("VerifyPosTx() : invalid nonce: %u", pBlock->GetNonce());

    if (pBlock->GetMerkleRootHash() != pBlock->BuildMerkleTree())
        return ERRORMSG("VerifyPosTx() : wrong merkle root hash");

    auto spCW = std::make_shared<CCacheWrapper>(cwIn);

    CBlockIndex *pBlockIndex = mapBlockIndex[pBlock->GetPrevBlockHash()];
    if (pBlock->GetHeight() != 1 || pBlock->GetPrevBlockHash() != SysCfg().GetGenesisBlockHash()) {
        CBlock previousBlock;
        if (!ReadBlockFromDisk(pBlockIndex, previousBlock))
            return ERRORMSG("VerifyPosTx() : read block info failed from disk");

        CAccount prevDelegateAcct;
        if (!spCW->accountCache.GetAccount(previousBlock.vptx[0]->txUid, prevDelegateAcct))
            return ERRORMSG("VerifyPosTx() : failed to get previous delegate's account, regId=%s",
                previousBlock.vptx[0]->txUid.ToString());

        if (pBlock->GetBlockTime() - previousBlock.GetBlockTime() < GetBlockInterval(pBlock->GetHeight())) {
            if (prevDelegateAcct.regid == curDelegate.regid)
                return ERRORMSG("VerifyPosTx() : one delegate can't produce more than one block at the same slot");
        }
    }

    CAccount account;
    if (spCW->accountCache.GetAccount(pBlock->vptx[0]->txUid, account)) {
        if (curDelegate.regid != account.regid) {
            return ERRORMSG("VerifyPosTx() : delegate should be(%s) vs what we got(%s)", curDelegate.regid.ToString(),
                            account.regid.ToString());
        }

        const auto &blockHash      = pBlock->ComputeSignatureHash();
        const auto &blockSignature = pBlock->GetSignature();

        if (blockSignature.size() == 0 || blockSignature.size() > MAX_SIGNATURE_SIZE) {
            return ERRORMSG("VerifyPosTx() : invalid block signature size, hash=%s", blockHash.ToString());
        }

        if (!VerifySignature(blockHash, blockSignature, account.owner_pubkey))
            if (!VerifySignature(blockHash, blockSignature, account.miner_pubkey))
                return ERRORMSG("VerifyPosTx() : verify signature error");
    } else {
        return ERRORMSG("VerifyPosTx() : failed to get account info, regId=%s", pBlock->vptx[0]->txUid.ToString());
    }

    if (pBlock->vptx[0]->nVersion != INIT_TX_VERSION)
        return ERRORMSG("VerifyPosTx() : transaction version %d vs current %d", pBlock->vptx[0]->nVersion, INIT_TX_VERSION);

    if (bNeedRunTx) {
        uint64_t totalFuel    = 0;
        uint64_t totalRunStep = 0;
        for (uint32_t i = 1; i < pBlock->vptx.size(); i++) {
            shared_ptr<CBaseTx> pBaseTx = pBlock->vptx[i];
            if (spCW->txCache.HaveTx(pBaseTx->GetHash()))
                return ERRORMSG("VerifyPosTx() : duplicate transaction, txid=%s", pBaseTx->GetHash().GetHex());

            CValidationState state;
            if (!pBaseTx->ExecuteTx(pBlock->GetHeight(), i, *spCW, state)) {
                if (SysCfg().IsLogFailures()) {
                    pCdMan->pLogCache->SetExecuteFail(pBlock->GetHeight(), pBaseTx->GetHash(), state.GetRejectCode(),
                                                      state.GetRejectReason());
                }
                return ERRORMSG("VerifyPosTx() : failed to execute transaction, txid=%s", pBaseTx->GetHash().GetHex());
            }

            totalRunStep += pBaseTx->nRunStep;
            if (totalRunStep > MAX_BLOCK_RUN_STEP)
                return ERRORMSG("VerifyPosTx() : block total run steps(%lu) exceed max run step(%lu)", totalRunStep,
                                MAX_BLOCK_RUN_STEP);

            totalFuel += pBaseTx->GetFuel(pBlock->GetFuelRate());
            LogPrint("fuel", "VerifyPosTx() : total fuel:%d, tx fuel:%d runStep:%d fuelRate:%d txid:%s \n", totalFuel,
                     pBaseTx->GetFuel(pBlock->GetFuelRate()), pBaseTx->nRunStep, pBlock->GetFuelRate(),
                     pBaseTx->GetHash().GetHex());
        }

        if (totalFuel != pBlock->GetFuel())
            return ERRORMSG("VerifyPosTx() : total fuel(%lu) mismatch what(%u) in block header", totalFuel,
                            pBlock->GetFuel());
    }

    return true;
}

std::unique_ptr<CBlock> CreateNewBlockPreStableCoinRelease(CCacheWrapper &cwIn) {
    // Create new block
    std::unique_ptr<CBlock> pBlock(new CBlock());
    if (!pBlock.get())
        return nullptr;

    pBlock->vptx.push_back(std::make_shared<CBlockRewardTx>());

    // Largest block you're willing to create:
    uint32_t nBlockMaxSize = SysCfg().GetArg("-blockmaxsize", DEFAULT_BLOCK_MAX_SIZE);
    // Limit to between 1K and MAX_BLOCK_SIZE-1K for sanity:
    nBlockMaxSize = std::max((uint32_t)1000, std::min((uint32_t)(MAX_BLOCK_SIZE - 1000), nBlockMaxSize));

    // How much of the block should be dedicated to high-priority transactions,
    // included regardless of the fees they pay
    uint32_t nBlockPrioritySize = SysCfg().GetArg("-blockprioritysize", DEFAULT_BLOCK_PRIORITY_SIZE);
    nBlockPrioritySize          = std::min(nBlockMaxSize, nBlockPrioritySize);

    // Minimum block size you want to create; block will be filled with free transactions
    // until there are no more or the block reaches this size:
    uint32_t nBlockMinSize = SysCfg().GetArg("-blockminsize", DEFAULT_BLOCK_MIN_SIZE);
    nBlockMinSize          = std::min(nBlockMaxSize, nBlockMinSize);

    // Collect memory pool transactions into the block
    {
        LOCK2(cs_main, mempool.cs);

        CBlockIndex *pIndexPrev = chainActive.Tip();
        int32_t height          = pIndexPrev->height + 1;
        int32_t index           = 0; // block reward tx
        uint32_t fuelRate       = GetElementForBurn(pIndexPrev);
        uint64_t totalBlockSize = ::GetSerializeSize(*pBlock, SER_NETWORK, PROTOCOL_VERSION);
        uint64_t totalRunStep   = 0;
        uint64_t totalFees      = 0;
        uint64_t totalFuel      = 0;
        uint64_t reward         = 0;

        // Calculate && sort transactions from memory pool.
        vector<TxPriority> txPriorities;
        GetPriorityTx(txPriorities, fuelRate);
        TxPriorityCompare comparer(false); // Priority by size first.
        make_heap(txPriorities.begin(), txPriorities.end(), comparer);
        LogPrint("MINER", "CreateNewBlockPreStableCoinRelease() : got %lu transaction(s) sorted by priority rules\n",
                 txPriorities.size());

        // Collect transactions into the block.
        for (auto item : txPriorities) {

            CBaseTx *pBaseTx = std::get<2>(item).get();

            uint32_t txSize = pBaseTx->GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION);
            if (totalBlockSize + txSize >= nBlockMaxSize) {
                LogPrint("MINER", "CreateNewBlockPreStableCoinRelease() : exceed max block size, txid: %s\n",
                         pBaseTx->GetHash().GetHex());
                continue;
            }

            // Skip trx with MinRelayTxFee fee for this block once the accumulated tx size surpasses the minimum block size.
            const double dFeePerKb = std::get<1>(item);
            if ((dFeePerKb != 0) && (dFeePerKb < CBaseTx::nMinRelayTxFee) && (totalBlockSize + txSize >= nBlockMinSize)) {
                LogPrint("MINER", "CreateNewBlockPreStableCoinRelease() : skip free transaction, txid: %s\n",
                         pBaseTx->GetHash().GetHex());

                continue;
            }

            auto spCW = std::make_shared<CCacheWrapper>(cwIn);

            try {
                CValidationState state;
                pBaseTx->nFuelRate = fuelRate;
                if (!pBaseTx->CheckTx(height, *spCW, state) || !pBaseTx->ExecuteTx(height, index + 1, *spCW, state)) {
                    LogPrint("MINER", "CreateNewBlockPreStableCoinRelease() : failed to pack transaction, txid: %s\n",
                            pBaseTx->GetHash().GetHex());

                    if (SysCfg().IsLogFailures())
                        pCdMan->pLogCache->SetExecuteFail(height, pBaseTx->GetHash(), state.GetRejectCode(),
                                                        state.GetRejectReason());
                    continue;
                }

                // Run step limits
                if (totalRunStep + pBaseTx->nRunStep >= MAX_BLOCK_RUN_STEP) {
                    LogPrint("MINER", "CreateNewBlockPreStableCoinRelease() : exceed max block run steps, txid: %s\n",
                            pBaseTx->GetHash().GetHex());
                    continue;
                }
            } catch (std::exception &e) {
                LogPrint("ERROR", "CreateNewBlockStableCoinRelease() : unexpected exception: %s\n", e.what());
                continue;
            }

            // Need to re-sync all to cache layer except for transaction cache, as it depends on
            // the global transaction cache to verify whether a transaction(txid) has been confirmed
            // already in block.
            spCW->Flush();

            auto fuel        = pBaseTx->GetFuel(fuelRate);
            auto fees_symbol = std::get<0>(pBaseTx->GetFees());
            auto fees        = std::get<1>(pBaseTx->GetFees());
            assert(fees_symbol == SYMB::WICC);

            totalBlockSize += txSize;
            totalRunStep += pBaseTx->nRunStep;
            totalFuel += fuel;
            totalFees += fees;
            assert(fees >= fuel);
            reward += (fees - fuel);

            ++index;

            pBlock->vptx.push_back(std::get<2>(item));

            LogPrint("fuel", "miner total fuel:%d, tx fuel:%d, runStep:%d, fuelRate:%d, txid:%s\n", totalFuel,
                     pBaseTx->GetFuel(fuelRate), pBaseTx->nRunStep, fuelRate, pBaseTx->GetHash().GetHex());
        }

        nLastBlockTx                   = index + 1;
        nLastBlockSize                 = totalBlockSize;
        miningBlockInfo.txCount        = index + 1;
        miningBlockInfo.totalBlockSize = totalBlockSize;
        miningBlockInfo.totalFees      = totalFees;

        ((CBlockRewardTx *)pBlock->vptx[0].get())->reward = reward;

        // Fill in header
        pBlock->SetPrevBlockHash(pIndexPrev->GetBlockHash());
        pBlock->SetNonce(0);
        pBlock->SetHeight(height);
        pBlock->SetFuel(totalFuel);
        pBlock->SetFuelRate(fuelRate);
        UpdateTime(*pBlock, pIndexPrev);

        LogPrint("INFO", "CreateNewBlockPreStableCoinRelease() : height=%d, tx=%d, totalBlockSize=%llu\n", height, index + 1,
                 totalBlockSize);
    }

    return pBlock;
}

std::unique_ptr<CBlock> CreateStableCoinGenesisBlock() {
    // Create new block
    std::unique_ptr<CBlock> pBlock(new CBlock());
    if (!pBlock.get())
        return nullptr;

    {
        LOCK(cs_main);

        // Create block reward transaction.
        pBlock->vptx.push_back(std::make_shared<CBlockRewardTx>());

        // Create stale coin genesis transactions.
        SysCfg().CreateFundCoinRewardTx(pBlock->vptx, SysCfg().NetworkID());

        // Fill in header
        CBlockIndex *pIndexPrev = chainActive.Tip();
        int32_t height          = pIndexPrev->height + 1;
        uint32_t fuelRate       = GetElementForBurn(pIndexPrev);

        pBlock->SetPrevBlockHash(pIndexPrev->GetBlockHash());
        UpdateTime(*pBlock, pIndexPrev);
        pBlock->SetNonce(0);
        pBlock->SetHeight(height);
        pBlock->SetFuel(0);
        pBlock->SetFuelRate(fuelRate);
    }

    return pBlock;
}


std::unique_ptr<CBlock> CreateNewBlockStableCoinRelease(CCacheWrapper &cwIn) {
    // Create new block
    std::unique_ptr<CBlock> pBlock(new CBlock());
    if (!pBlock.get())
        return nullptr;

    pBlock->vptx.push_back(std::make_shared<CUCoinBlockRewardTx>());
    pBlock->vptx.push_back(std::make_shared<CBlockPriceMedianTx>());

    // Largest block you're willing to create:
    uint32_t nBlockMaxSize = SysCfg().GetArg("-blockmaxsize", DEFAULT_BLOCK_MAX_SIZE);
    // Limit to between 1K and MAX_BLOCK_SIZE-1K for sanity:
    nBlockMaxSize = std::max((uint32_t)1000, std::min((uint32_t)(MAX_BLOCK_SIZE - 1000), nBlockMaxSize));

    // How much of the block should be dedicated to high-priority transactions,
    // included regardless of the fees they pay
    uint32_t nBlockPrioritySize = SysCfg().GetArg("-blockprioritysize", DEFAULT_BLOCK_PRIORITY_SIZE);
    nBlockPrioritySize          = std::min(nBlockMaxSize, nBlockPrioritySize);

    // Minimum block size you want to create; block will be filled with free transactions
    // until there are no more or the block reaches this size:
    uint32_t nBlockMinSize = SysCfg().GetArg("-blockminsize", DEFAULT_BLOCK_MIN_SIZE);
    nBlockMinSize          = std::min(nBlockMaxSize, nBlockMinSize);

    // Collect memory pool transactions into the block
    {
        LOCK2(cs_main, mempool.cs);

        CBlockIndex *pIndexPrev            = chainActive.Tip();
        int32_t height                     = pIndexPrev->height + 1;
        int32_t index                      = 1; // 0: block reward tx; 1: median price tx
        uint32_t fuelRate                  = GetElementForBurn(pIndexPrev);
        uint64_t totalBlockSize            = ::GetSerializeSize(*pBlock, SER_NETWORK, PROTOCOL_VERSION);
        uint64_t totalRunStep              = 0;
        uint64_t totalFees                 = 0;
        uint64_t totalFuel                 = 0;
        map<TokenSymbol, uint64_t> rewards = {{SYMB::WICC, 0}, {SYMB::WUSD, 0}};

        // Calculate && sort transactions from memory pool.
        vector<TxPriority> txPriorities;
        GetPriorityTx(txPriorities, fuelRate);
        TxPriorityCompare comparer(false); // Priority by size first.
        make_heap(txPriorities.begin(), txPriorities.end(), comparer);
        LogPrint("MINER", "CreateNewBlockStableCoinRelease() : got %lu transaction(s) sorted by priority rules\n",
                 txPriorities.size());

        auto startTime = std::chrono::steady_clock::now() ;
        // Collect transactions into the block.
        for (auto item : txPriorities) {
            auto endTime    = std::chrono::steady_clock::now();
            double costTime = std::chrono::duration<double>(endTime - startTime).count();
            if (costTime >= GetBlockInterval(height) - 1) {
                break;
            }

            CBaseTx *pBaseTx = std::get<2>(item).get();

            uint32_t txSize = pBaseTx->GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION);
            if (totalBlockSize + txSize >= nBlockMaxSize) {
                LogPrint("MINER", "CreateNewBlockStableCoinRelease() : exceed max block size, txid: %s\n",
                         pBaseTx->GetHash().GetHex());
                continue;
            }

            // Skip trx with MinRelayTxFee fee for this block once the accumulated tx size surpasses the minimum block size.
            const double dFeePerKb = std::get<1>(item);
            if ((dFeePerKb != 0) && (dFeePerKb < CBaseTx::nMinRelayTxFee) && (totalBlockSize + txSize >= nBlockMinSize)) {
                LogPrint("MINER", "CreateNewBlockStableCoinRelease() : skip fee too litter in fee/kb: %.4f < %lld, txid: %s\n",
                         dFeePerKb, CBaseTx::nMinRelayTxFee, pBaseTx->GetHash().GetHex());
                continue;
            }

            auto spCW = std::make_shared<CCacheWrapper>(cwIn);

            try {
                CValidationState state;
                pBaseTx->nFuelRate = fuelRate;
                if (!pBaseTx->CheckTx(height, *spCW, state) || !pBaseTx->ExecuteTx(height, index + 1, *spCW, state)) {
                    LogPrint("MINER", "CreateNewBlockStableCoinRelease() : failed to pack transaction, txid: %s\n",
                             pBaseTx->GetHash().GetHex());

                    if (SysCfg().IsLogFailures())
                        pCdMan->pLogCache->SetExecuteFail(height, pBaseTx->GetHash(), state.GetRejectCode(),
                                                        state.GetRejectReason());
                    continue;
                }

                // Run step limits
                if (totalRunStep + pBaseTx->nRunStep >= MAX_BLOCK_RUN_STEP) {
                    LogPrint("MINER", "CreateNewBlockStableCoinRelease() : exceed max block run steps, txid: %s\n",
                            pBaseTx->GetHash().GetHex());
                    continue;
                }
            } catch (std::exception &e) {
                LogPrint("ERROR", "CreateNewBlockStableCoinRelease() : unexpected exception: %s\n", e.what());


                continue;
            }

            // Need to re-sync all to cache layer except for transaction cache, as it depends on
            // the global transaction cache to verify whether a transaction(txid) has been confirmed
            // already in block.
            spCW->Flush();

            auto fuel        = pBaseTx->GetFuel(fuelRate);
            auto fees_symbol = std::get<0>(pBaseTx->GetFees());
            auto fees        = std::get<1>(pBaseTx->GetFees());
            assert(fees_symbol == SYMB::WICC || fees_symbol == SYMB::WUSD);

            totalBlockSize += txSize;
            totalRunStep += pBaseTx->nRunStep;
            totalFuel += fuel;
            totalFees += fees;
            assert(fees >= fuel);
            // FIXME: reward = (fees * X - fuel)/X, X = WUSD/WICC
            rewards[fees_symbol] += (fees - fuel);

            ++index;

            pBlock->vptx.push_back(std::get<2>(item));

            LogPrint("fuel", "miner total fuel:%d, tx fuel:%d, runStep:%d, fuelRate:%d, txid:%s\n", totalFuel,
                     pBaseTx->GetFuel(fuelRate), pBaseTx->nRunStep, fuelRate, pBaseTx->GetHash().GetHex());

        }

        nLastBlockTx                   = index + 1;
        nLastBlockSize                 = totalBlockSize;
        miningBlockInfo.txCount        = index + 1;
        miningBlockInfo.totalBlockSize = totalBlockSize;
        miningBlockInfo.totalFees      = totalFees;

        ((CUCoinBlockRewardTx *)pBlock->vptx[0].get())->rewards = rewards;

        CBlockPriceMedianTx *pPriceMedianTx = (CBlockPriceMedianTx *)pBlock->vptx[1].get();
        map<CoinPricePair, uint64_t> mapMedianPricePoints;
        uint64_t slideWindowBlockCount;
        cwIn.sysParamCache.GetParam(SysParamType::MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT, slideWindowBlockCount);
        cwIn.ppCache.GetBlockMedianPricePoints(height, slideWindowBlockCount, mapMedianPricePoints);
        pPriceMedianTx->SetMedianPricePoints(mapMedianPricePoints);

        // Fill in header
        pBlock->SetPrevBlockHash(pIndexPrev->GetBlockHash());
        pBlock->SetNonce(0);
        pBlock->SetHeight(height);
        pBlock->SetFuel(totalFuel);
        pBlock->SetFuelRate(fuelRate);
        UpdateTime(*pBlock, pIndexPrev);

        LogPrint("INFO", "CreateNewBlockStableCoinRelease() : height=%d, tx=%d, totalBlockSize=%llu\n", height, index + 1,
                 totalBlockSize);
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

        // Process this block the same as if we received it from another node
        CValidationState state;
        if (!ProcessBlock(state, nullptr, pBlock))
            return ERRORMSG("CheckWork() : failed to process block");
    }

    return true;
}

bool static MineBlock(CBlock *pBlock, CWallet *pWallet, CBlockIndex *pIndexPrev, uint32_t txUpdated,
                      CCacheWrapper &cw) {
    int64_t nStart = GetTime();

    while (true) {
        boost::this_thread::interruption_point();

        // Should not mine new blocks if the miner does not connect to other nodes except running
        // in regtest network.
        if (vNodes.empty() && SysCfg().NetworkID() != REGTEST_NET)
            return false;

        if (pIndexPrev != chainActive.Tip())
            return false;

        // Take a sleep and check.
        [&]() {
            int64_t whenCanIStart = pIndexPrev->GetBlockTime() + GetBlockInterval(chainActive.Height() + 1);
            while (GetTime() < whenCanIStart) {
                ::MilliSleep(100);
            }
        }();

        vector<CRegID> delegateList;
        if (!cw.delegateCache.GetTopDelegateList(delegateList)) {
            LogPrint("MINER", "MineBlock() : failed to get top delegates\n");
            return false;
        }

        uint16_t index = 0;
        for (auto &delegate : delegateList)
            LogPrint("shuffle", "before shuffle: index=%d, regId=%s\n", index++, delegate.ToString());

        ShuffleDelegates(pBlock->GetHeight(), delegateList);

        index = 0;
        for (auto &delegate : delegateList)
            LogPrint("shuffle", "after shuffle: index=%d, regId=%s\n", index++, delegate.ToString());

        int64_t currentTime = GetTime();
        CRegID regId;
        GetCurrentDelegate(currentTime, pBlock->GetHeight(), delegateList, regId);
        CAccount minerAcct;
        if (!cw.accountCache.GetAccount(regId, minerAcct)) {
            LogPrint("MINER", "MineBlock() : failed to get miner's account: %s\n", regId.ToString());
            return false;
        }

        bool success = false;
        int64_t lastTime;
        {
            LOCK2(cs_main, pWalletMain->cs_wallet);
            if (uint32_t(chainActive.Height() + 1) != pBlock->GetHeight())
                return false;

            CKey acctKey;
            if (pWalletMain->GetKey(minerAcct.keyid.ToAddress(), acctKey, true) ||
                pWalletMain->GetKey(minerAcct.keyid.ToAddress(), acctKey)) {
                lastTime = GetTimeMillis();
                success   = CreateBlockRewardTx(currentTime, minerAcct, cw.accountCache, pBlock);
                LogPrint("MINER", "MineBlock() : %s to create block reward transaction, used %d ms, miner address %s\n",
                         success ? "succeed" : "failed", GetTimeMillis() - lastTime,
                         minerAcct.keyid.ToAddress());
            }
        }

        if (success) {
            SetThreadPriority(THREAD_PRIORITY_NORMAL);

            lastTime = GetTimeMillis();
            success   = CheckWork(pBlock, *pWallet);
            LogPrint("MINER", "MineBlock() : %s to check work, used %s ms\n", success ? "succeed" : "failed",
                     GetTimeMillis() - lastTime);

            SetThreadPriority(THREAD_PRIORITY_LOWEST);

            miningBlockInfo.time          = pBlock->GetBlockTime();
            miningBlockInfo.nonce         = pBlock->GetNonce();
            miningBlockInfo.height        = pBlock->GetHeight();
            miningBlockInfo.totalFuel     = pBlock->GetFuel();
            miningBlockInfo.fuelRate      = pBlock->GetFuelRate();
            miningBlockInfo.hash          = pBlock->GetHash();
            miningBlockInfo.hashPrevBlock = pBlock->GetHash();

            {
                LOCK(csMinedBlocks);
                minedBlocks.push_front(miningBlockInfo);
            }

            return true;
        }

        if (mempool.GetUpdatedTransactionNum() != txUpdated || GetTime() - nStart > 60)
            return false;
    }

    return false;
}

void static CoinMiner(CWallet *pWallet, int32_t targetHeight) {
    LogPrint("INFO", "CoinMiner() : started\n");

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
        LogPrint("ERROR", "CoinMiner() : terminated for lack of miner key\n");
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
            boost::this_thread::interruption_point();

            if (SysCfg().NetworkID() != REGTEST_NET) {
                // Busy-wait for the network to come online so we don't waste time mining
                // on an obsolete chain. In regtest mode we expect to fly solo.
                while (vNodes.empty() || (chainActive.Tip() && chainActive.Height() > 1 &&
                                          GetAdjustedTime() - chainActive.Tip()->nTime > 60 * 60 &&
                                          !SysCfg().GetBoolArg("-genblockforce", false))) {
                    MilliSleep(1000);
                }
            }

            miningBlockInfo.SetNull();  // TODO: remove

            //
            // Create new block
            //
            int64_t lastTime        = GetTimeMillis();
            uint32_t txUpdated      = mempool.GetUpdatedTransactionNum();
            int32_t blockHeight     = chainActive.Height() + 1;
            CBlockIndex *pIndexPrev = chainActive.Tip();

            auto spCW   = std::make_shared<CCacheWrapper>(pCdMan);
            auto pBlock = (blockHeight == (int32_t)SysCfg().GetStableCoinGenesisHeight())
                              ? CreateStableCoinGenesisBlock()  // stable coin genesis
                              : (GetFeatureForkVersion(blockHeight) == MAJOR_VER_R1)
                                    ? CreateNewBlockPreStableCoinRelease(*spCW) // pre-stable coin release
                                    : CreateNewBlockStableCoinRelease(*spCW);   // stable coin release

            if (!pBlock.get()) {
                throw runtime_error("CoinMiner() : failed to create new block");
            } else {
                LogPrint("MINER", "CoinMiner() : succeed to create new block, contain %s transactions, used %s ms\n",
                         pBlock->vptx.size(), GetTimeMillis() - lastTime);
            }

            // Attention: need to reset delegate cache to compute the miner account according to received votes ranking
            // list.
            spCW->delegateCache.Clear();
            MineBlock(pBlock.get(), pWallet, pIndexPrev, txUpdated, *spCW);

            if (SysCfg().NetworkID() != MAIN_NET && targetHeight <= GetCurrHeight())
                throw boost::thread_interrupted();
        }
    } catch (...) {
        LogPrint("INFO", "CoinMiner() : terminated\n");
        SetMinerStatus(false);
        throw;
    }
}

void GenerateCoinBlock(bool fGenerate, CWallet *pWallet, int32_t targetHeight) {
    static boost::thread_group *minerThreads = nullptr;

    if (minerThreads != nullptr) {
        minerThreads->interrupt_all();
        delete minerThreads;
        minerThreads = nullptr;
    }

    if (!fGenerate)
        return;

    // In mainnet, coin miner should generate blocks continuously regardless of target height.
    if (SysCfg().NetworkID() != MAIN_NET && targetHeight <= 0) {
        LogPrint("ERROR", "GenerateCoinBlock() : target height <=0 (%d)", targetHeight);
        return;
    }

    minerThreads = new boost::thread_group();
    minerThreads->create_thread(boost::bind(&CoinMiner, pWallet, targetHeight));
}

void MinedBlockInfo::SetNull() {
    time           = 0;
    nonce          = 0;
    height         = 0;
    totalFuel      = 0;
    fuelRate       = 0;
    totalFees      = 0;
    txCount        = 0;
    totalBlockSize = 0;
    hash.SetNull();
    hashPrevBlock.SetNull();
}

vector<MinedBlockInfo> GetMinedBlocks(uint32_t count) {
    std::vector<MinedBlockInfo> ret;
    LOCK(csMinedBlocks);
    count = std::min((uint32_t)minedBlocks.size(), count);
    for (uint32_t i = 0; i < count; i++) {
        ret.push_back(minedBlocks[i]);
    }

    return ret;
}
