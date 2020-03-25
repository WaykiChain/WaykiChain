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
        s += strprintf("{regid=%s, votes=%llu}", item.regid.ToString(), item.votes);
    }
    return strprintf("{count=%d, [%s]}", activeDelegates.size(), s);
}

static const int BP_DELEGATE_VOTE_MIN = 21000;

static bool GenPendingDelegates(CBlock &block, uint32_t delegateNum, CCacheWrapper &cw, PendingDelegates &pendingDelegates) {

    pendingDelegates.counted_vote_height = block.GetHeight();
    VoteDelegateVector topVoteDelegates;
    if (!cw.delegateCache.GetTopVoteDelegates(delegateNum, BP_DELEGATE_VOTE_MIN, topVoteDelegates)) {
        LogPrint(BCLog::INFO, "[WARNING] %s, GetTopVoteDelegates() failed! no need to update pending delegates! "
                "block=%d:%s, delegate_num=%d\n",
                __FUNCTION__, block.GetHeight(), block.GetHash().ToString(), delegateNum);
        return true;
    };

    VoteDelegateVector activeDelegates;
    if (!cw.delegateCache.GetActiveDelegates(activeDelegates)) {
        LogPrint(BCLog::INFO, "%s() : active delegates do not exist, will be initialized later! block=%d:%s\n",
            __FUNCTION__, block.GetHeight(), block.GetHash().ToString());
    }

    pendingDelegates.top_vote_delegates = topVoteDelegates;

    if (!activeDelegates.empty() && pendingDelegates.top_vote_delegates == activeDelegates) {
        LogPrint(BCLog::INFO, "%s, the top vote delegates are unchanged! block=%d:%s, num=%d, dest_num=%d\n",
                __FUNCTION__, block.GetHeight(), block.GetHash().ToString(),
                pendingDelegates.top_vote_delegates.size(), delegateNum);
        // update counted_vote_height and top_vote_delegates to skip unchanged delegates to next count vote slot height
        return true;
    }

    pendingDelegates.state = VoteDelegateState::PENDING;
    return true;
}

// process block delegates, call in the tail of block executing
bool chain::ProcessBlockDelegates(CBlock &block, CCacheWrapper &cw, CValidationState &state) {
    // required preparing the undo for cw

    int32_t countVoteInterval; // the interval to count the vote
    int32_t activateDelegateInterval; // the interval to count the vote

    FeatureForkVersionEnum version = GetFeatureForkVersion(block.GetHeight());
    if (version >= MAJOR_VER_R3) {
        countVoteInterval = COUNT_VOTE_INTERVAL_AFTER_V3;
        activateDelegateInterval = ACTIVATE_DELEGATE_DELAY_AFTER_V3;
    } else {
        countVoteInterval = COUNT_VOTE_INTERVAL_BEFORE_V3;
        activateDelegateInterval = COUNT_VOTE_INTERVAL_AFTER_V3;
    }

    PendingDelegates pendingDelegates;
    cw.delegateCache.GetPendingDelegates(pendingDelegates);

    // get last update height of vote
    if (pendingDelegates.state != VoteDelegateState::PENDING &&
        (countVoteInterval == 0 || (block.GetHeight() % countVoteInterval == 0))) {

        int32_t lastVoteHeight = cw.delegateCache.GetLastVoteHeight();
        uint32_t activedDelegateNum = cw.delegateCache.GetActivedDelegateNum() ;
        uint32_t delegateNum = cw.sysParamCache.GetTotalBpsSize(block.GetHeight()) ;
        assert(delegateNum < uint32_t(BP_MAX_COUNT));

        if (pendingDelegates.counted_vote_height == 0 ||
            lastVoteHeight > (int32_t)pendingDelegates.counted_vote_height
            || activedDelegateNum != delegateNum) {

            if (!GenPendingDelegates(block, delegateNum, cw, pendingDelegates)) {
                return state.DoS(100, ERRORMSG("%s() : GenPendingDelegates failed! block=%d:%s",
                    __FUNCTION__, block.GetHeight(), block.GetHash().ToString()));
            }

            if (!cw.delegateCache.SetPendingDelegates(pendingDelegates)) {
                return state.DoS(100, ERRORMSG("%s() : save pending delegates failed! block=%d:%s",
                    __FUNCTION__, block.GetHeight(), block.GetHash().ToString()));
            }
        }
    }

    // must execute below separately because activateDelegateInterval may be 0
    if (pendingDelegates.state != VoteDelegateState::ACTIVATED) {
        // TODO: use the aBFT irreversible height to check
        if (block.GetHeight() - pendingDelegates.counted_vote_height >= (uint32_t)activateDelegateInterval) {
            VoteDelegateVector activeDelegates = pendingDelegates.top_vote_delegates;
            if (!cw.delegateCache.SetActiveDelegates(activeDelegates)) {
                return state.DoS(100, ERRORMSG("%s() : SetActiveDelegates failed! block=%d:%s",
                    __FUNCTION__, block.GetHeight(), block.GetHash().ToString()));
            }
            pendingDelegates.state = VoteDelegateState::ACTIVATED;

            if (!cw.delegateCache.SetPendingDelegates(pendingDelegates)) {
                return state.DoS(100, ERRORMSG("%s() : save pending delegates failed! block=%d:%s",
                    __FUNCTION__, block.GetHeight(), block.GetHash().ToString()));
            }
            LogPrint(BCLog::INFO, "%s, activate new delegates! block=%d:%s, delegates=[%s]\n",
                    __FUNCTION__, block.GetHeight(), block.GetHash().ToString(),
                    ToString(activeDelegates));
        }
    }
    return true;
}
