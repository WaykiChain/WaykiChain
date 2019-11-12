// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockdelegates.h"
#include <memory>

#include "main.h"

using namespace std;

static std::string ToString(const DelegateVector &activeDelegates) {
    string s = "";
    for (const auto &item : activeDelegates) {
        if (s != "") s += ",";
        s += item.ToString();
    }
    return strprintf("{count=%d, [%s]}", activeDelegates.size(), s);
}

static bool GenPendingDelegates(CBlock &block, CCacheWrapper &cw, PendingDelegates &pendingDelegates) {

    pendingDelegates.counted_vote_height = block.GetHeight();
    VoteDelegateVector topVoteDelegates;
    if (!cw.delegateCache.GetTopVoteDelegates(topVoteDelegates) ||
        topVoteDelegates.size() != IniCfg().GetTotalDelegateNum()) {

        LogPrint("ERROR", "[WARNING] %s, the got top vote delegates is invalid! block=%d:%s, got_num=%d, definitive_num=%d\n",
                __FUNCTION__, block.GetHeight(), block.GetHash().ToString());
        // update counted_vote_height to skip invalid delegates to next count vote slot height
        return true;
    };

    DelegateVector activeDelegates;
    if (!cw.delegateCache.GetActiveDelegates(activeDelegates)) {
        LogPrint("INFO", "%s() : active delegates do not exist, will be initialized soon! block=%d:%s\n",
            __FUNCTION__, block.GetHeight(), block.GetHash().ToString());
    }

    pendingDelegates.top_vote_delegates = topVoteDelegates;

    if (!activeDelegates.empty() && pendingDelegates.IsSameDelegates(activeDelegates)) {
        LogPrint("INFO", "%s, the top vote delegates are unchanged! block=%d:%s, num=%d, dest_num=%d\n",
                __FUNCTION__, block.GetHeight(), block.GetHash().ToString(),
                pendingDelegates.top_vote_delegates.size(), IniCfg().GetTotalDelegateNum());
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

    // TODO: move to sysconf
    FeatureForkVersionEnum version = GetFeatureForkVersion(block.GetHeight());
    if (version >= MAJOR_VER_R3) {
        countVoteInterval = 8;
        activateDelegateInterval = 24;
    } else {
        countVoteInterval = 0;
        activateDelegateInterval = 0;
    }

    PendingDelegates pendingDelegates;
    cw.delegateCache.GetPendingDelegates(pendingDelegates);

    // get last update height of vote
    if (pendingDelegates.state != VoteDelegateState::PENDING &&
        (countVoteInterval == 0 || (block.GetHeight() % countVoteInterval == 0))) {

        int32_t lastVoteHeight = cw.delegateCache.GetLastVoteHeight();
        if (pendingDelegates.counted_vote_height == 0 ||
            lastVoteHeight > (int32_t)pendingDelegates.counted_vote_height) {

            if (!GenPendingDelegates(block, cw, pendingDelegates)) {
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
            DelegateVector activeDelegates = pendingDelegates.GetDelegates();
            if (!cw.delegateCache.SetActiveDelegates(activeDelegates)) {
                return state.DoS(100, ERRORMSG("%s() : SetActiveDelegates failed! block=%d:%s",
                    __FUNCTION__, block.GetHeight(), block.GetHash().ToString()));
            }
            pendingDelegates.state = VoteDelegateState::ACTIVATED;

            if (!cw.delegateCache.SetPendingDelegates(pendingDelegates)) {
                return state.DoS(100, ERRORMSG("%s() : save pending delegates failed! block=%d:%s",
                    __FUNCTION__, block.GetHeight(), block.GetHash().ToString()));
            }
            LogPrint("INFO", "%s, activate new delegates! block=%d:%s, delegates=[%s]\n",
                    __FUNCTION__, block.GetHeight(), block.GetHash().ToString(),
                    ToString(activeDelegates));
        }
    }
    return true;
}
