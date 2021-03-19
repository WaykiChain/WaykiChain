// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockdelegates.h"
#include <memory>

#include "main.h"

using namespace std;

static std::string ToString(const VoteDelegateVector &activeDelegates) {
    string s = "";
    for (const auto &item : activeDelegates) {
        if (s != "") s += ",";
        s += strprintf("{regid=%s, votes=%s}", item.regid.ToString(), item.votes);
    }
    return strprintf("{count=%d, [%s]}", activeDelegates.size(), s);
}

static bool GenPendingDelegates(CBlock &block, uint32_t delegateNum, CCacheWrapper &cw,
                                const VoteDelegateVector activeDelegates,
                                PendingDelegates &pendingDelegates, bool isR3Fork) {

    pendingDelegates.counted_vote_height = block.GetHeight();
    uint64_t BpMinVote;
    if (!cw.sysParamCache.GetParam(SysParamType::BP_DELEGATE_VOTE_MIN, BpMinVote))
        return ERRORMSG("get sys param BP_DELEGATE_VOTE_MIN failed! block=%d:%s\n",
                        block.GetHeight(), block.GetHash().ToString());

    VoteDelegateVector topVoteDelegates;
    if (!cw.delegateCache.GetTopVoteDelegates(delegateNum, BpMinVote, topVoteDelegates, isR3Fork)) {
        LogPrint(BCLog::INFO, "[WARN] [%d] GetTopVoteDelegates() failed! no need to update pending delegates! "
                "block=%s, delegate_num=%d\n", block.GetHeight(), block.GetHash().ToString(), delegateNum);

        return true;
    };

    pendingDelegates.top_vote_delegates = topVoteDelegates;

    if (!activeDelegates.empty() && pendingDelegates.top_vote_delegates == activeDelegates) {
        LogPrint(BCLog::INFO, "[%d] The top vote delegates are unchanged! block=%s, num=%d, dest_num=%d\n",
                block.GetHeight(), block.GetHash().ToString(), pendingDelegates.top_vote_delegates.size(), delegateNum);
        // update counted_vote_height and top_vote_delegates to skip unchanged delegates to next count vote slot height
        return true;
    }

    LogPrint(BCLog::DELEGATE, "Gen new pending delegates={%s}\n", pendingDelegates.ToString());
    pendingDelegates.state = VoteDelegateState::PENDING;

    return true;
}

bool chain::IsBlockInflatedRewardStarted(CCacheWrapper &cw, HeightType height) {
    auto version = GetFeatureForkVersion(height);
    if (version >= MAJOR_VER_R3_5) {
        CBlockInflatedReward blockInflatedReward;
        cw.blockCache.GetBlockInflatedReward(blockInflatedReward);
        return (blockInflatedReward.start_height != 0 && height >= blockInflatedReward.start_height);
    }
    return false;
}

// process block delegates, call at the tail of block executing
bool chain::ProcessBlockDelegates(CBlock &block, CCacheWrapper &cw, CValidationState &state) {
    // required preparing the undo for cw

    auto version = GetFeatureForkVersion(block.GetHeight());

    if (version >= MAJOR_VER_R3_5) {
        CBlockInflatedReward blockInflatedReward;
        cw.blockCache.GetBlockInflatedReward(blockInflatedReward);
        if (blockInflatedReward.start_height != 0 && block.GetHeight() >= blockInflatedReward.start_height) {
            // TODO: add proposal of BLOCK_INFLATED_REWARD_AMOUNT
            static const uint64_t BLOCK_INFLATED_REWARD_AMOUNT = 20000000; // 0.2 WICC
            blockInflatedReward.new_rewards += BLOCK_INFLATED_REWARD_AMOUNT;
            if (!cw.blockCache.SetBlockInflatedReward(blockInflatedReward)) {
                return state.DoS(100, ERRORMSG("[%d] Save BlockInflatedReward failed! block=%s",
                        block.GetHeight(), block.GetHash().ToString()));
            }
        }
    }

    bool isR3Fork = version >= MAJOR_VER_R3;
    int32_t countVoteInterval = (isR3Fork) ? COUNT_VOTE_INTERVAL_AFTER_V3 : COUNT_VOTE_INTERVAL_BEFORE_V3; // the interval to count the vote
    int32_t activateDelegateInterval = (isR3Fork) ? ACTIVATE_DELEGATE_DELAY_AFTER_V3 : ACTIVATE_DELEGATE_DELAY_BEFORE_V3;

    PendingDelegates pendingDelegates;
    cw.delegateCache.GetPendingDelegates(pendingDelegates);

    // get last update height of vote
    if (pendingDelegates.state != VoteDelegateState::PENDING &&
        (countVoteInterval == 0 || (block.GetHeight() % countVoteInterval == 0))) {
        VoteDelegateVector activeDelegates;
        if (!cw.delegateCache.GetActiveDelegates(activeDelegates)) {
            LogPrint(BCLog::INFO, "[%d] Active delegates do not exist, will be initialized later! block=%s\n",
                    block.GetHeight(), block.GetHash().ToString());
        }
        int32_t lastVoteHeight = cw.delegateCache.GetLastVoteHeight();
        uint32_t delegateNum = cw.sysParamCache.GetTotalBpsSize(block.GetHeight()) ;

        if (pendingDelegates.counted_vote_height == 0 ||
            lastVoteHeight > (int32_t)pendingDelegates.counted_vote_height ||
            activeDelegates.size() != delegateNum) {

            if (!GenPendingDelegates(block, delegateNum, cw, activeDelegates, pendingDelegates, isR3Fork)) {
                return state.DoS(100, ERRORMSG("[%d] GenPendingDelegates failed! block=%s",
                        block.GetHeight(), block.GetHash().ToString()));
            }

            if (!cw.delegateCache.SetPendingDelegates(pendingDelegates)) {
                return state.DoS(100, ERRORMSG("[%d] Save pending delegates failed! block=%s",
                        block.GetHeight(), block.GetHash().ToString()));
            }
        }
    }

    // must execute below separately because activateDelegateInterval may be 0
    if (pendingDelegates.state != VoteDelegateState::ACTIVATED) {
        if (block.GetHeight() - pendingDelegates.counted_vote_height >= (uint32_t)activateDelegateInterval) {
            VoteDelegateVector activeDelegates = pendingDelegates.top_vote_delegates;
            if (!cw.delegateCache.SetActiveDelegates(activeDelegates)) {
                return state.DoS(100, ERRORMSG("[%d] SetActiveDelegates failed! block=%s",
                        block.GetHeight(), block.GetHash().ToString()));
            }
            pendingDelegates.state = VoteDelegateState::ACTIVATED;

            if (!cw.delegateCache.SetPendingDelegates(pendingDelegates)) {
                return state.DoS(100, ERRORMSG("[%d] : save pending delegates failed! block=%s",
                    block.GetHeight(), block.GetHash().ToString()));
            }
            LogPrint(BCLog::INFO, "[%d] activate new delegates! block=%s, delegates=[%s]\n",
                    block.GetHeight(), block.GetHash().ToString(),
                    ToString(activeDelegates));
        }
    }
    return true;
}
