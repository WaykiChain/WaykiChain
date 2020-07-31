// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "miner.h"

#include "pbftcontext.h"
#include "init.h"
#include "main.h"
#include "net.h"
#include "wallet/wallet.h"
#include "tx/tx.h"
#include "tx/blockrewardtx.h"
#include "tx/blockpricemediantx.h"
#include "tx/cdptx.h"
#include "persistence/txdb.h"
#include "persistence/contractdb.h"
#include "persistence/cachewrapper.h"
#include "p2p/protocol.h"

#include <algorithm>
#include <boost/circular_buffer.hpp>

extern CWallet *pWalletMain;
extern CPBFTContext pbftContext;
extern void SetMinerStatus(bool bStatus);


uint64_t nLastBlockTx   = 0;
uint64_t nLastBlockSize = 0;

MinedBlockInfo miningBlockInfo;
boost::circular_buffer<MinedBlockInfo> minedBlocks(MAX_MINED_BLOCK_COUNT);
CCriticalSection csMinedBlocks;
bool g_isMiningTimeLimit = true;

// check the time is not exceed the limit time (2s) for packing new block
static bool CheckPackBlockTime(int64_t startMiningMs, int32_t blockHeight) {
    if (g_isMiningTimeLimit) {
        int64_t nowMs  = GetTimeMillis();
        int64_t limitedTimeMs = std::max(1000LL, (int64_t)GetBlockInterval(blockHeight) * 1000LL - 1000LL);
        if (nowMs - startMiningMs > limitedTimeMs) {
            LogPrint(BCLog::MINER, "[%d] pack block time use up! start_ms=%lld, now_ms=%lld, limited_time_ms=%lld\n",
                blockHeight, startMiningMs, nowMs, limitedTimeMs);
            return false;
        }
    }
    return true;
}

// base on the lastest 50 blocks
uint32_t GetElementForBurn(CBlockIndex *pIndex) {
    if (!pIndex) {
        return INIT_FUEL_RATE;
    }
    int32_t nBlock = SysCfg().GetArg("-blocksizeforburn", DEFAULT_BURN_BLOCK_SIZE);
    if (nBlock * 2 >= pIndex->height - 1) {
        return INIT_FUEL_RATE;
    }

    uint64_t totalFuelFee   = 0;
    uint64_t averageFuelFee = 0;
    uint32_t newFuelRate  = 0;
    CBlockIndex *pTemp    = pIndex;
    for (int32_t i = 0; i < nBlock; ++i) {
        totalFuelFee += pTemp->nFuelFee / pTemp->nFuelRate * 100;
        pTemp = pTemp->pprev;
    }

    averageFuelFee = totalFuelFee / nBlock;
    if (averageFuelFee < MAX_BLOCK_FUEL * 0.75) {
        newFuelRate = pIndex->nFuelRate * 0.9;
    } else if (averageFuelFee > MAX_BLOCK_FUEL * 0.85) {
        newFuelRate = pIndex->nFuelRate * 1.1;
    } else {
        newFuelRate = pIndex->nFuelRate;
    }

    if (newFuelRate < MIN_FUEL_RATE) {
        newFuelRate = MIN_FUEL_RATE;
    }

    LogPrint(BCLog::DEBUG, "[%d] preFuelRate=%d, fuelRate=%d\n", pIndex->height, pIndex->nFuelRate, newFuelRate);

    return newFuelRate;
}

// Sort transactions by priority and fee to decide priority orders to process transactions.
void GetPriorityTx(CCacheWrapper &cw, int32_t height, set<TxPriority> &txPriorities, const int32_t nFuelRate) {
    static TokenSymbol feeSymbol;
    static uint64_t fee    = 0;
    static uint32_t txSize = 0;
    static double feePerKb = 0;
    static double priority = 0;

    for (map<uint256, CTxMemPoolEntry>::iterator mi = mempool.memPoolTxs.begin(); mi != mempool.memPoolTxs.end(); ++mi) {
        CBaseTx *pBaseTx = mi->second.GetTransaction().get();
        if (!pBaseTx->IsBlockRewardTx() && !pCdMan->pTxCache->HasTx(pBaseTx->GetHash())) {
            feeSymbol = std::get<0>(mi->second.GetFees());
            fee       = std::get<1>(mi->second.GetFees());
            txSize    = mi->second.GetTxSize();
            feePerKb  = double(fee - pBaseTx->GetFuelFee(cw, height, nFuelRate)) / txSize * 1000.0;
            priority  = mi->second.GetPriority();

            txPriorities.emplace(TxPriority(priority, feePerKb, mi->second.GetTransaction()));
        }
    }
}


bool GetCurrentDelegate(const int64_t currentTime, const int32_t currHeight, const VoteDelegateVector &delegates,
                               VoteDelegate &delegate) {

    uint32_t slot  = currentTime / GetBlockInterval(currHeight) / GetContinuousBlockProduceCount(currHeight);
    uint32_t index = slot % delegates.size();
    delegate       = delegates[index];
    LogPrint(BCLog::DEBUG, "[%d] currTime=%lld, slot=%d, index=%d, regId=%s\n", currHeight, currentTime, slot, index, delegate.regid.ToString());

    return true;
}


static bool CreateBlockRewardTx(Miner &miner, CBlock *pBlock,const uint32_t totalDelegateNum) {

    if (pBlock->vptx[0]->nTxType == BLOCK_REWARD_TX) {
        auto pRewardTx          = (CBlockRewardTx *)pBlock->vptx[0].get();
        pRewardTx->txUid        = miner.delegate.regid;
        pRewardTx->valid_height = pBlock->GetHeight();

    } else if (pBlock->vptx[0]->nTxType == UCOIN_BLOCK_REWARD_TX) {
        auto pRewardTx             = (CUCoinBlockRewardTx *)pBlock->vptx[0].get();
        pRewardTx->txUid           = miner.delegate.regid;
        pRewardTx->valid_height    = pBlock->GetHeight();
        pRewardTx->inflated_bcoins = miner.account.ComputeBlockInflateInterest(pBlock->GetHeight(), miner.delegate, totalDelegateNum);
    }

    pBlock->SetNonce(GetRand(SysCfg().GetBlockMaxNonce()));
    pBlock->SetMerkleRootHash(pBlock->BuildMerkleTree());

    vector<uint8_t> signature;
    if (miner.key.Sign(pBlock->GetHash(), signature)) {
        pBlock->SetSignature(signature);
        return true;
    } else {
        return ERRORMSG("Sign failed");
    }
}

inline int64_t GetShuffleOriginSeed(const int32_t curHeight, const int64_t blockTime ){
    if (curHeight < (int32_t)SysCfg().GetVer3ForkHeight()){
        return curHeight;
    }else{
        int64_t slot = blockTime/GetBlockInterval(curHeight);
        return slot/ GetContinuousBlockProduceCount(curHeight);
    }
}

void ShuffleDelegates(const int32_t curHeight, const int64_t blockTime, VoteDelegateVector &delegates) {

    int64_t oriSeed = GetShuffleOriginSeed( curHeight,blockTime );
    auto totalDelegateNum = delegates.size();

    string seedSource = strprintf("%u", oriSeed / totalDelegateNum + (oriSeed % totalDelegateNum > 0 ? 1 : 0));
    CHashWriter ss(SER_GETHASH, 0);
    ss << seedSource;
    uint256 currentSeed  = ss.GetHash();
    uint64_t newIndexSource = 0;
    for (uint32_t i = 0; i < totalDelegateNum; i++) {
        for (uint32_t x = 0; x < 4 && i < totalDelegateNum; i++, x++) {
            memcpy(&newIndexSource, currentSeed.begin() + (x * 8), 8);
            uint32_t newIndex      = newIndexSource % totalDelegateNum;
            VoteDelegate delegate = delegates[newIndex];
            delegates[newIndex] = delegates[i];
            delegates[i]        = delegate;
        }
        ss << currentSeed;
        currentSeed = ss.GetHash();
    }
}


bool VerifyRewardTx(const CBlock *pBlock, CCacheWrapper &cwIn, VoteDelegate &curDelegateOut, uint32_t& totalDelegateNumOut) {
    uint32_t maxNonce = SysCfg().GetBlockMaxNonce();

    VoteDelegateVector delegates;
    if (!cwIn.delegateCache.GetActiveDelegates(delegates))
        return ERRORMSG("get active delegates failed");

    totalDelegateNumOut = delegates.size();
    ShuffleDelegates(pBlock->GetHeight(),pBlock->GetTime(), delegates);

    if (!GetCurrentDelegate(pBlock->GetTime(), pBlock->GetHeight(), delegates, curDelegateOut))
        return ERRORMSG("failed to get current delegate");

    CAccount delegateAccount;
    if (!cwIn.accountCache.GetAccount(curDelegateOut.regid, delegateAccount)) {
        string delegatesStr;
        for (const auto & item : delegates) {
            delegatesStr += strprintf("%s, ", item.regid.ToString());
        }

        LogPrint(BCLog::ERROR, "delegate list: %s\n", delegatesStr);

        return ERRORMSG("failed to get current delegate's account, regId=%s", curDelegateOut.regid.ToString());
    }

    if (pBlock->GetNonce() > maxNonce)
        return ERRORMSG("invalid nonce: %u", pBlock->GetNonce());

    if (pBlock->GetMerkleRootHash() != pBlock->BuildMerkleTree())
        return ERRORMSG("wrong merkle root hash");

    auto spCW = std::make_shared<CCacheWrapper>(&cwIn);

    CBlockIndex *pBlockIndex = mapBlockIndex[pBlock->GetPrevBlockHash()];
    if (pBlock->GetHeight() != 1 || pBlock->GetPrevBlockHash() != SysCfg().GetGenesisBlockHash()) {
        CBlock previousBlock;
        if (!ReadBlockFromDisk(pBlockIndex, previousBlock))
            return ERRORMSG("read block info failed from disk");

        CAccount prevDelegateAcct;
        if (!spCW->accountCache.GetAccount(previousBlock.vptx[0]->txUid, prevDelegateAcct))
            return ERRORMSG("failed to get previous delegate's account, regId=%s",
                previousBlock.vptx[0]->txUid.ToString());

        if (pBlock->GetBlockTime() - previousBlock.GetBlockTime() < GetBlockInterval(pBlock->GetHeight())) {
            if (prevDelegateAcct.regid == delegateAccount.regid)
                return ERRORMSG("one delegate can't produce more than one block at the same slot");
        }
    }

    CAccount account;
    if (!spCW->accountCache.GetAccount(pBlock->vptx[0]->txUid, account))
        return ERRORMSG("[%d] failed to get account info, regId=%s", pBlock->GetHeight(), pBlock->vptx[0]->txUid.ToString());

    if (delegateAccount.regid != account.regid)
        return ERRORMSG("[%d] delegate should be (%s) vs what we got (%s)",
                        pBlock->GetHeight(), delegateAccount.regid.ToString(), account.regid.ToString());

    const auto &blockHash      = pBlock->GetHash();
    const auto &blockSignature = pBlock->GetSignature();

    if (blockSignature.size() == 0 || blockSignature.size() > MAX_SIGNATURE_SIZE)
        return ERRORMSG("invalid block signature size, hash=%s", blockHash.ToString());

    if (!VerifySignature(blockHash, blockSignature, account.owner_pubkey) &&
        !VerifySignature(blockHash, blockSignature, account.miner_pubkey))
            return ERRORMSG("verify signature error");

    if (pBlock->vptx[0]->nVersion != INIT_TX_VERSION)
        return ERRORMSG("transaction version %d vs current %d", pBlock->vptx[0]->nVersion, INIT_TX_VERSION);

    // if (bNeedRunTx) {
    //     uint64_t totalFuelFee    = 0;
    //     uint64_t totalFuel = 0;
    //     for (uint32_t i = 1; i < pBlock->vptx.size(); i++) {
    //         shared_ptr<CBaseTx> pBaseTx = pBlock->vptx[i];
    //         if (spCW->txCache.HasTx(pBaseTx->GetHash()))
    //             return ERRORMSG("duplicate transaction, txid=%s", pBaseTx->GetHash().GetHex());

    //         CValidationState state;
    //         uint32_t prevBlockTime =
    //             pBlockIndex->pprev != nullptr ? pBlockIndex->pprev->GetBlockTime() : pBlockIndex->GetBlockTime();
    //         CTxExecuteContext context(pBlock->GetHeight(), i, pBlock->GetFuelRate(), pBlock->GetTime(), prevBlockTime,
    //                                   spCW.get(), &state);
    //         if (!pBaseTx->ExecuteFullTx(context)) {
    //             pCdMan->pLogCache->SetExecuteFail(pBlock->GetHeight(), pBaseTx->GetHash(), state.GetRejectCode(),
    //                                               state.GetRejectReason());
    //             return ERRORMSG("failed to execute transaction, txid=%s",
    //                             pBaseTx->GetHash().GetHex());
    //         }

    //         totalFuel += pBaseTx->fuel;
    //         if (totalFuel > MAX_BLOCK_FUEL)
    //             return ERRORMSG("block total run steps(%lu) exceed max run step(%lu)", totalFuel,
    //                             MAX_BLOCK_FUEL);

    //         uint32_t fuelFee = pBaseTx->GetFuelFee(pBlock->GetHeight(), pBlock->GetFuelRate());
    //         totalFuelFee += fuelFee;
    //         LogPrint(BCLog::DEBUG, "VerifyRewardTx() : total fuel fee:%d, tx fuel fee:%d fuel:%d fuelRate:%d txid:%s\n", totalFuelFee,
    //                  fuelFee, pBaseTx->fuel, pBlock->GetFuelRate(), pBaseTx->GetHash().GetHex());
    //     }

    //     if (totalFuelFee != pBlock->GetFuelFee())
    //         return ERRORMSG("total fuel fee(%lu) mismatch what(%u) in block header", totalFuelFee,
    //                         pBlock->GetFuelFee());
    // }

    return true;
}

static bool CreateNewBlockForPreStableCoinRelease(Miner &miner, CCacheWrapper &cwIn, std::unique_ptr<CBlock> &pBlock) {
    pBlock->vptx.push_back(std::make_shared<CBlockRewardTx>());

    // Largest block you're willing to create:
    uint32_t nBlockMaxSize = SysCfg().GetArg("-blockmaxsize", DEFAULT_BLOCK_MAX_SIZE);
    // Limit to between 1K and MAX_BLOCK_SIZE-1K for sanity:
    nBlockMaxSize = std::max<uint32_t>(1000, std::min<uint32_t>((MAX_BLOCK_SIZE - 1000), nBlockMaxSize));

    // Collect memory pool transactions into the block
    {
        LOCK2(cs_main, mempool.cs);

        CBlockIndex *pIndexPrev = chainActive.Tip();
        uint32_t blockTime      = pBlock->GetTime();
        int32_t height          = pIndexPrev->height + 1;
        int32_t index           = 0; // block reward tx
        uint32_t fuelRate       = GetElementForBurn(pIndexPrev);
        uint64_t totalBlockSize = ::GetSerializeSize(*pBlock, SER_NETWORK, PROTOCOL_VERSION);
        uint64_t totalFuel   = 0;
        uint64_t totalFees      = 0;
        uint64_t totalFuelFee   = 0;
        uint64_t reward         = 0;

        // Calculate && sort transactions from memory pool.
        set<TxPriority> txPriorities;
        GetPriorityTx(cwIn, height, txPriorities, fuelRate);

        LogPrint(BCLog::MINER, "got %lu transaction(s) sorted by priority rules\n",
                 txPriorities.size());

        // Collect transactions into the block.
        for (auto itor = txPriorities.rbegin(); itor != txPriorities.rend(); ++itor) {
            CBaseTx *pBaseTx = itor->baseTx.get();

            uint32_t txSize = pBaseTx->GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION);
            if (totalBlockSize + txSize >= nBlockMaxSize) {
                LogPrint(BCLog::MINER, "exceed max block size, txid: %s\n", pBaseTx->GetHash().GetHex());

                continue;
            }

            auto spCW = std::make_shared<CCacheWrapper>(&cwIn);

            try {
                auto bm = MAKE_BENCHMARK("execute tx in mining block");
                CValidationState state;
                pBaseTx->nFuelRate = fuelRate;
                uint32_t prevBlockTime = pIndexPrev->GetBlockTime();
                CTxExecuteContext context(height, index + 1, fuelRate, blockTime, prevBlockTime,
                                          miner.account.regid, spCW.get(), &state,
                                          TxExecuteContextType::PRODUCE_BLOCK);

                if (!pBaseTx->CheckAndExecuteTx(context)) {
                    LogPrint(BCLog::MINER, "Check/ExecuteTx failed, txid: %s\n", pBaseTx->GetHash().GetHex());

                    pCdMan->pLogCache->SetExecuteFail(height, pBaseTx->GetHash(), state.GetRejectCode(), state.GetRejectReason());
                    continue;
                }

                // Run step limits
                if (totalFuel + pBaseTx->fuel >= MAX_BLOCK_FUEL) {
                    LogPrint(BCLog::MINER, "exceed max block run steps, txid: %s\n",
                            pBaseTx->GetHash().GetHex());
                    continue;
                }
            } catch (std::exception &e) {
                LogPrint(BCLog::ERROR, "unexpected exception: %s\n", e.what());
                continue;
            }

            spCW->Flush();

            auto fuelFee     = pBaseTx->GetFuelFee(cwIn, height, fuelRate);
            auto fees_symbol = std::get<0>(pBaseTx->GetFees());
            auto fees        = std::get<1>(pBaseTx->GetFees());
            assert(fees_symbol == SYMB::WICC);

            totalBlockSize += txSize;
            totalFuel += pBaseTx->fuel;
            totalFuelFee += fuelFee;
            totalFees += fees;
            assert(fees >= fuelFee);
            reward += (fees - fuelFee);

            ++index;

            pBlock->vptx.push_back(itor->baseTx);

            LogPrint(BCLog::DEBUG, "miner's total fuel fee:%d, tx fuel fee:%d, fuel:%d, fuelRate:%d, txid:%s\n",
                    totalFuelFee, fuelFee, pBaseTx->fuel, fuelRate, pBaseTx->GetHash().GetHex());
        }

        nLastBlockTx                   = index + 1;
        nLastBlockSize                 = totalBlockSize;

        ((CBlockRewardTx *)pBlock->vptx[0].get())->reward_fees = reward;

        // Fill in header
        pBlock->SetPrevBlockHash(pIndexPrev->GetBlockHash());
        pBlock->SetNonce(0);
        pBlock->SetHeight(height);
        pBlock->SetFuel(totalFuelFee);
        pBlock->SetFuelRate(fuelRate);

        LogPrint(BCLog::INFO, "[%d] tx=%d, totalBlockSize=%llu\n", height, index + 1, totalBlockSize);
    }

    return true;
}

static bool CreateStableCoinGenesisBlock(std::unique_ptr<CBlock> &pBlock) {
    LOCK(cs_main);

    // Create block reward transaction.
    pBlock->vptx.push_back(std::make_shared<CBlockRewardTx>());

    // Create stale coin genesis transactions.
    SysCfg().CreateFundCoinMintTx(pBlock->vptx, SysCfg().NetworkID());

    // Fill in header
    CBlockIndex *pIndexPrev = chainActive.Tip();
    int32_t height          = pIndexPrev->height + 1;
    uint32_t fuelRate       = GetElementForBurn(pIndexPrev);

    pBlock->SetPrevBlockHash(pIndexPrev->GetBlockHash());
    pBlock->SetNonce(0);
    pBlock->SetHeight(height);
    pBlock->SetFuel(0);
    pBlock->SetFuelRate(fuelRate);

    return true;
}

static bool CreateNewBlockForStableCoinRelease(int64_t startMiningMs, Miner &miner, CCacheWrapper &cwIn, std::unique_ptr<CBlock> &pBlock) {
    pBlock->vptx.push_back(std::make_shared<CUCoinBlockRewardTx>());

    // Largest block you're willing to create:
    uint32_t nBlockMaxSize = SysCfg().GetArg("-blockmaxsize", DEFAULT_BLOCK_MAX_SIZE);
    // Limit to between 1K and MAX_BLOCK_SIZE-1K for sanity:
    nBlockMaxSize = std::max<uint32_t>(1000, std::min<uint32_t>((MAX_BLOCK_SIZE - 1000), nBlockMaxSize));

    // Collect memory pool transactions into the block
    {
        LOCK2(cs_main, mempool.cs);

        CBlockIndex *pIndexPrev            = chainActive.Tip();
        uint32_t blockTime                 = pBlock->GetTime();
        int32_t height                     = pIndexPrev->height + 1;
        int32_t index                      = 0; // 0: block reward tx
        uint32_t fuelRate                  = GetElementForBurn(pIndexPrev);
        uint64_t totalBlockSize            = ::GetSerializeSize(*pBlock, SER_NETWORK, PROTOCOL_VERSION);
        uint64_t totalFuel              = 0;
        uint64_t totalFees                 = 0;
        uint64_t totalFuelFee              = 0;
        map<TokenSymbol, uint64_t> rewards = { {SYMB::WICC, 0}, {SYMB::WUSD, 0} };

        // Calculate && sort transactions from memory pool.
        set<TxPriority> txPriorities;
        GetPriorityTx(cwIn, height, txPriorities, fuelRate);

        // Push block price median transaction into queue.
        txPriorities.emplace(TxPriority(PRICE_MEDIAN_TRANSACTION_PRIORITY, 0, std::make_shared<CBlockPriceMedianTx>(height)));

        if (GetFeatureForkVersion(height) >= MAJOR_VER_R3) {
            auto spCdpForceSettleInterestTx = std::make_shared<CCDPInterestForceSettleTx>(height);
            if (!GetSettledInterestCdps(cwIn, height, spCdpForceSettleInterestTx->cdp_list)) {
                return ERRORMSG("GetSettledInterestCdps error");
            }
            if (!spCdpForceSettleInterestTx->cdp_list.empty()) {
                txPriorities.emplace(TxPriority(TRANSACTION_PRIORITY_CEILING, 0, spCdpForceSettleInterestTx));

                LogPrint(BCLog::MINER, "create CCDPInterestForceSettleTx to block! tx=%s\n",
                        spCdpForceSettleInterestTx->ToString(cwIn.accountCache));
            }
        }

        LogPrint(BCLog::MINER, "Got %lu trx(s), sorted by priority\n", txPriorities.size());

        // Collect transactions into the block.
        for (auto itor = txPriorities.rbegin(); itor != txPriorities.rend(); ++itor) {

            if (!CheckPackBlockTime(startMiningMs, height)) {
                LogPrint(BCLog::MINER, "[%d] no time left to pack more tx, ignore! start_ms=%lld, tx_count=%u\n",
                        height, startMiningMs, pBlock->vptx.size());
                break;
            }

            CBaseTx *pBaseTx = itor->baseTx.get();

            uint32_t txSize = pBaseTx->GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION);
            if (totalBlockSize + txSize >= nBlockMaxSize) {
                LogPrint(BCLog::MINER, "Exceed max block size, txid: %s\n", pBaseTx->GetHash().GetHex());

                continue;
            }

            auto spCW = std::make_shared<CCacheWrapper>(&cwIn);

            try {
                auto bm = MAKE_BENCHMARK("execute tx in mining block");
                CValidationState state;

                pBaseTx->nFuelRate = fuelRate;

                // Special case for price median tx,
                if (pBaseTx->IsPriceMedianTx()) {
                    CBlockPriceMedianTx *pPriceMedianTx = (CBlockPriceMedianTx *)itor->baseTx.get();
                    if (!spCW->ppCache.CalcMedianPrices(*spCW, height, pPriceMedianTx->median_prices))
                        return ERRORMSG("calculate block median prices error");
                }

                LogPrint(BCLog::MINER, "begin to pack trx: %s\n", pBaseTx->ToString(spCW->accountCache));

                uint32_t prevBlockTime = pIndexPrev->GetBlockTime();
                CTxExecuteContext context(height, index + 1, fuelRate, blockTime, prevBlockTime,
                                          miner.account.regid, spCW.get(), &state,
                                          TxExecuteContextType::PRODUCE_BLOCK);

                if (!pBaseTx->CheckAndExecuteTx(context)) {
                    LogPrint(BCLog::MINER, "failed to check/exec tx: %s\n", pBaseTx->ToString(spCW->accountCache));

                    pCdMan->pLogCache->SetExecuteFail(height, pBaseTx->GetHash(), state.GetRejectCode(), state.GetRejectReason());
                    continue;
                }

                // Run step limits
                if (totalFuel + pBaseTx->fuel >= MAX_BLOCK_FUEL) {
                    LogPrint(BCLog::MINER, "Exceed max block run steps, txid: %s\n", pBaseTx->GetHash().GetHex());
                    continue;
                }
            } catch (std::exception &e) {
                LogPrint(BCLog::ERROR, "unexpected exception: %s\n", e.what());

                continue;
            }

            spCW->Flush();

            auto fuelFee        = pBaseTx->GetFuelFee(cwIn, height, fuelRate);
            auto fees_symbol = std::get<0>(pBaseTx->GetFees());
            auto fees        = std::get<1>(pBaseTx->GetFees());
            assert(fees_symbol == SYMB::WICC || fees_symbol == SYMB::WUSD);

            totalBlockSize += txSize;
            totalFuel += pBaseTx->fuel;
            totalFuelFee += fuelFee;
            totalFees += fees;
            assert(fees >= fuelFee);
            rewards[fees_symbol] += (fees - fuelFee);

            ++index;

            pBlock->vptx.push_back(itor->baseTx);

            LogPrint(BCLog::DEBUG, "miner total_fuel_fee=%d, tx_fuel_fee=%d, fuel=%d, fuelRate:%d, txid:%s\n",
                    totalFuelFee, fuelFee, pBaseTx->fuel, fuelRate, pBaseTx->GetHash().GetHex());

        }

        nLastBlockTx                   = index + 1;
        nLastBlockSize                 = totalBlockSize;

        ((CUCoinBlockRewardTx *)pBlock->vptx[0].get())->reward_fees = rewards;

        // Fill in header
        pBlock->SetPrevBlockHash(pIndexPrev->GetBlockHash());
        pBlock->SetNonce(0);
        pBlock->SetHeight(height);
        pBlock->SetFuel(totalFuelFee);
        pBlock->SetFuelRate(fuelRate);

        LogPrint(BCLog::INFO, "[%d] tx=%d, totalBlockSize=%llu\n", height, index + 1, totalBlockSize);
    }

    return true;
}

bool CheckWork(CBlock *pBlock) {



    if (GetFeatureForkVersion(pBlock->GetHeight()) > MAJOR_VER_R1) {
        bool hasMedianPriceTx = false;
        for (size_t index = 1; index < pBlock->vptx.size(); ++ index) {

            if (pBlock->vptx[index]->IsPriceMedianTx()){
                hasMedianPriceTx = true;
                break;
            }

        }
        if (!hasMedianPriceTx)
            return ERRORMSG("medianpricetx not found in block");
    }


    if (pBlock->GetPrevBlockHash() != chainActive.Tip()->GetBlockHash())
        return ERRORMSG("generated block is stale");

    // Process this block the same as if we received it from another node
    CValidationState state;
    if (!ProcessBlock(state, nullptr, pBlock))
        return ERRORMSG("failed to process block");

    return true;
}

static bool GetMiner(int64_t startMiningMs, const int32_t blockHeight, Miner &miner, uint32_t& totalDelegateNumOut) {
    VoteDelegateVector delegates;
    {
        LOCK(cs_main);

        if (!pCdMan->pDelegateCache->GetActiveDelegates(delegates)) {
            LogPrint(BCLog::MINER, "[%d] fail to get top delegates! time_ms=%lld\n",
                blockHeight, startMiningMs);

            return false;
        }
    }
    totalDelegateNumOut = delegates.size();

    ShuffleDelegates(blockHeight,MillisToSecond(startMiningMs), delegates);

    GetCurrentDelegate(MillisToSecond(startMiningMs), blockHeight, delegates, miner.delegate);

    {
        LOCK(cs_main);

        if (!pCdMan->pAccountCache->GetAccount(miner.delegate.regid, miner.account)) {
            LogPrint(BCLog::MINER, "[%d] fail to get miner account! time_ms=%lld, regid=%s\n",
                blockHeight, startMiningMs, miner.delegate.regid.ToString());
            return false;
        }
    }
    bool isMinerKey = false;
    {
        LOCK(pWalletMain->cs_wallet);

        if (miner.account.miner_pubkey.IsValid() && pWalletMain->GetKey(miner.account.keyid, miner.key, true)) {
            isMinerKey = true;
        } else if (!pWalletMain->GetKey(miner.account.keyid, miner.key)) {
            LogPrint(BCLog::DEBUG, "[%d] [ignore] Not on-duty miner, time_ms=%lld, regid=%s, addr=%s\n",
                blockHeight, startMiningMs, miner.delegate.regid.ToString(), miner.account.keyid.ToAddress());
            return false;
        }
    }
    LogPrint(BCLog::DEBUG, "[%d] on-duty miner: time_ms=%lld, regid=%s, addr=%s, is_miner_key=%d\n",
        blockHeight, startMiningMs, miner.delegate.regid.ToString(), miner.account.keyid.ToAddress(), isMinerKey);

    return true;
}


static bool ProduceBlock(int64_t startMiningMs, CBlockIndex *pPrevIndex, Miner &miner, const uint32_t totalDelegateNum) {
    // has done LOCK(cs_main);
    int64_t lastTime    = 0;
    bool success        = false;
    int32_t blockHeight = 0;
    std::unique_ptr<CBlock> pBlock(new CBlock());
    if (!pBlock.get())
        throw runtime_error("ProduceBlock() : failed to create new block");

    blockHeight = pPrevIndex->height + 1;
    const CRegID &minerId = miner.account.regid;

    if (!CheckPackBlockTime(startMiningMs, blockHeight)) {
        LogPrint(BCLog::MINER, "[%d] no time left to pack block! start_ms=%lld, miner_regid=%s\n",
                blockHeight, startMiningMs, minerId.ToString());
        return false;
    }

    lastTime  = GetTimeMillis();
    auto spCW = std::make_shared<CCacheWrapper>(pCdMan);

    pBlock->SetTime(MillisToSecond(startMiningMs));  // set block time first

    if (blockHeight == (int32_t)SysCfg().GetVer2GenesisHeight()) {
        success = CreateStableCoinGenesisBlock(pBlock);  // stable coin genesis

    } else if (GetFeatureForkVersion(blockHeight) == MAJOR_VER_R1) {
        success = CreateNewBlockForPreStableCoinRelease(miner, *spCW, pBlock); // pre-stable coin release

    } else {
        success = CreateNewBlockForStableCoinRelease(startMiningMs, miner, *spCW, pBlock);    // stable coin release
    }

    if (!success) {
        LogPrint(BCLog::MINER, "[%d] failed to add a new block: regid=%s, "
            "used_time_ms=%lld\n", blockHeight, minerId.ToString(),
            GetTimeMillis() - lastTime);
        return false;
    }
    LogPrint(BCLog::MINER,
                "[%d] succeeded in adding a new block: regid=%s, tx_count=%u, "
                "used_time_ms=%lld\n", blockHeight, minerId.ToString(),
                pBlock->vptx.size(), GetTimeMillis() - lastTime);

    lastTime = GetTimeMillis();
    success  = CreateBlockRewardTx(miner, pBlock.get(), totalDelegateNum);
    if (!success) {
        LogPrint(BCLog::MINER, "[%d] failed in CreateBlockRewardTx: regid=%s, "
            "used_time_ms=%lld\n", blockHeight, minerId.ToString(), GetTimeMillis() - lastTime);
        return false;
    }
    LogPrint(BCLog::MINER, "[%d] succeeded in CreateBlockRewardTx: regid=%s, reward_txid=%s, "
        "used_time_ms=%lld\n", blockHeight, minerId.ToString(), pBlock->vptx[0]->GetHash().ToString(),
        GetTimeMillis() - lastTime);

    lastTime = GetTimeMillis();
    success  = CheckWork(pBlock.get());
    if (!success) {
        LogPrint(BCLog::MINER, "[%d] failed in CheckWork for new block, regid=%s, "
            "used_time_ms=%lld\n", blockHeight, minerId.ToString(), GetTimeMillis() - lastTime);
        return false;
    }

    LogPrint(BCLog::MINER, "success to CheckWork for new block=%s, regid=%s, used_time_ms=%lld\n",
            pBlock->GetIdStr(), minerId.ToString(), GetTimeMillis() - lastTime);

    {
        LOCK(csMinedBlocks);
        miningBlockInfo.Set(pBlock.get());
        minedBlocks.push_front(miningBlockInfo);
        miningBlockInfo.SetNull();
    }

    LogPrint(BCLog::INFO, "[%d] mined a new block: regid=%s, hash=%s, "
        "used_time_ms=%lld\n", blockHeight, minerId.ToString(), pBlock->GetHash().ToString(),
        GetTimeMillis() - startMiningMs);
    return true;
}

void static ThreadBlockProducing(CWallet *pWallet, int32_t targetHeight) {
    LogPrint(BCLog::INFO, "started\n");

    RenameThread("coin-miner");

    auto HaveMinerKey = [&]() {
        LOCK2(cs_main, pWalletMain->cs_wallet);

        set<CKeyID> setMineKey;
        setMineKey.clear();
        pWalletMain->GetKeys(setMineKey, true);
        return !setMineKey.empty();
    };

    if (!HaveMinerKey()) {
        LogPrint(BCLog::ERROR, "terminated due to lack of miner key\n");
        return;
    }

    auto GetCurrHeight = [&]() {
        LOCK(cs_main);
        return chainActive.Height();
    };

    g_isMiningTimeLimit = SysCfg().GetBoolArg("-isminingtimelimit", true);
    targetHeight += GetCurrHeight();
    bool needSleep = false;
    int64_t nextSlotTime = 0;
    int64_t sleepSeconds = 100;

    try {
        SetMinerStatus(true);

        while (true) {
            boost::this_thread::interruption_point();

            CBlockIndex *pPrevIndex = nullptr;
            sleepSeconds = 100; // set sleep seconds default value
            if (!needSleep && (SysCfg().NetworkID() != REGTEST_NET)) {

                LOCK(cs_main);
                pPrevIndex = chainActive.Tip();

                if (vNodes.empty() || (pPrevIndex && pPrevIndex->height > 1 &&
                                       GetAdjustedTime() - pPrevIndex->nTime > 60 * 60 &&
                                       !SysCfg().GetBoolArg("-genblockforce", false))) {
                    // Busy-wait for the network to come online so we don't waste time mining
                    // on an obsolete chain. In regtest mode we expect to fly solo.
                    sleepSeconds = 1000;
                    needSleep    = true;
                }
            }

            if (needSleep) {
                MilliSleep(sleepSeconds);
                needSleep = false;
                continue;
            }

            if (pPrevIndex == nullptr) {
                LOCK(cs_main);
                pPrevIndex = chainActive.Tip();
            }

            if(pPrevIndex== nullptr) {
                needSleep = true;
                continue;
            }

            if(SysCfg().IsReindex()){
                needSleep = true;
                continue;
            }

            int32_t blockHeight = pPrevIndex->height + 1;

            int64_t startMiningMs = GetTimeMillis();
            int64_t curMiningTime = MillisToSecond(startMiningMs);
            int64_t curSlotTime = std::max(nextSlotTime, pPrevIndex->GetBlockTime() + GetBlockInterval(blockHeight));
            if (curMiningTime < curSlotTime) {
                needSleep = true;
                continue;
            }

            shared_ptr<Miner> spMiner = make_shared<Miner>();
            uint32_t totalDelegateNum;
            if (!GetMiner(startMiningMs, blockHeight, *spMiner,totalDelegateNum)) {
                needSleep = true;
                mining     = false;
                // miner key not exist in my wallet, skip to next slot time
                int64_t nextInterval = GetBlockInterval(blockHeight + 1);
                nextSlotTime = std::max(curSlotTime + nextInterval, curMiningTime - curMiningTime % nextInterval);
                continue;
            }

            if (!spMiner->account.CheckPerms(AccountPermType::PERM_MINE_BLOCK)) {
                continue;
            }

            mining     = true;
            {
                LOCK(cs_main);
                CBlockIndex *pTipIndex = chainActive.Tip();
                if (pPrevIndex != pTipIndex) {
                    LogPrint(BCLog::MINER, "active chain tip changed when mining! pre_block=%s, tip_block=%s\n",
                        pPrevIndex->GetIdString(), pTipIndex->GetIdString());
                    needSleep = false;
                    continue; // need to check and mine again
                }
                if (!ProduceBlock(startMiningMs, pPrevIndex, *spMiner,totalDelegateNum)) {
                    needSleep = true;
                    continue;
                }
            }

            if (SysCfg().NetworkID() != MAIN_NET && targetHeight <= GetCurrHeight())
                throw boost::thread_interrupted();
        }
    } catch (...) {
        LogPrint(BCLog::INFO, "terminated\n");
        SetMinerStatus(false);
        throw;
    }
}

void GenerateProduceBlockThread(bool fGenerate, CWallet *pWallet, int32_t targetHeight) {
    static boost::thread_group *minerThreads = nullptr;

    if (minerThreads != nullptr) {
        minerThreads->interrupt_all();
        delete minerThreads;
        minerThreads = nullptr;
    }

    if (!fGenerate)
        return;

    // // In mainnet, coin miner should generate blocks continuously regardless of target height.
    // if (SysCfg().NetworkID() != MAIN_NET && targetHeight <= 0) {
    //     LogPrint(BCLog::ERROR, "GenerateProduceBlockThread() : target height <=0 (%d)", targetHeight);
    //     return;
    // }

    minerThreads = new boost::thread_group();
    minerThreads->create_thread(boost::bind(&ThreadBlockProducing, pWallet, targetHeight));
}

void MinedBlockInfo::SetNull() {
    time           = 0;
    nonce          = 0;
    height         = 0;
    totalFuelFee   = 0;
    fuelRate       = 0;
    txCount        = 0;
    totalBlockSize = 0;
    hash.SetNull();
    hashPrevBlock.SetNull();
}

void MinedBlockInfo::Set(const CBlock *pBlock) {
    time           = pBlock->GetBlockTime();
    nonce          = pBlock->GetNonce();
    height         = pBlock->GetHeight();
    totalFuelFee   = pBlock->GetFuelFee();
    fuelRate       = pBlock->GetFuelRate();
    txCount        = pBlock->vptx.size();
    totalBlockSize = ::GetSerializeSize(*pBlock, SER_NETWORK, PROTOCOL_VERSION);
    hash           = pBlock->GetHash();
    hashPrevBlock  = pBlock->GetPrevBlockHash();
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
