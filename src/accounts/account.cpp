// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2019- The WaykiChain Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "account.h"
#include "configuration.h"

#include "main.h"

// /** account db cache*/
// extern CAccountViewCache *pAccountViewTip;

string CAccountLog::ToString() const {
    string str;
    str += strprintf(
        "Account log: keyId=%d regId=%s nickId=%s pubKey=%s minerPubKey=%s hasOpenCdp=%d "
        "bcoins=%lld receivedVotes=%lld \n",
        keyID.GetHex(), regID.ToString(), nickID.ToString(), pubKey.ToString(),
        minerPubKey.ToString(), hasOpenCdp, bcoins, receivedVotes);
    str += "vote fund: ";

    for (auto it = voteFunds.begin(); it != voteFunds.end(); ++it) {
        str += it->ToString();
    }
    return str;
}

bool CAccount::UndoOperateAccount(const CAccountLog &accountLog) {
    LogPrint("undo_account", "after operate:%s\n", ToString());
    bcoins         = accountLog.bcoins;
    hasOpenCdp     = accountLog.hasOpenCdp;
    voteFunds      = accountLog.voteFunds;
    receivedVotes  = accountLog.receivedVotes;
    LogPrint("undo_account", "before operate:%s\n", ToString().c_str());
    return true;
}

// uint64_t CAccount::GetAccountProfit(uint64_t nCurHeight) {
//      if (voteFunds.empty()) {
//         LogPrint("DEBUG", "1st-time vote by the account, hence no minting of interest.");
//         lastVoteHeight = nCurHeight; //record the 1st-time vote block height into account
//         return 0; // 0 for the very 1st vote
//     }

//     // 先判断计算分红的上下限区块高度是否落在同一个分红率区间
//     uint64_t nBeginHeight = lastVoteHeight;
//     uint64_t nEndHeight = nCurHeight;
//     uint64_t nBeginSubsidy = IniCfg().GetBlockSubsidyCfg(lastVoteHeight);
//     uint64_t nEndSubsidy = IniCfg().GetBlockSubsidyCfg(nCurHeight);
//     uint64_t nValue = voteFunds.begin()->GetVotedBcoins();
//     LogPrint("profits", "nBeginSubsidy:%lld nEndSubsidy:%lld nBeginHeight:%d nEndHeight:%d\n",
//         nBeginSubsidy, nEndSubsidy, nBeginHeight, nEndHeight);

//     // 计算分红
//     auto calculateProfit = [](uint64_t nValue, uint64_t nSubsidy, int nBeginHeight, int nEndHeight) -> uint64_t {
//         int64_t nHoldHeight = nEndHeight - nBeginHeight;
//         int64_t nYearHeight = SysCfg().GetSubsidyHalvingInterval();
//         uint64_t llProfits =  (uint64_t)(nValue * ((long double)nHoldHeight * nSubsidy / nYearHeight / 100));
//         LogPrint("profits", "nValue:%lld nSubsidy:%lld nBeginHeight:%d nEndHeight:%d llProfits:%lld\n",
//             nValue, nSubsidy, nBeginHeight, nEndHeight, llProfits);
//         return llProfits;
//     };

//     // 如果属于同一个分红率区间，分红=区块高度差（持有高度）* 分红率；如果不属于同一个分红率区间，则需要根据分段函数累加每一段的分红
//     uint64_t llProfits = 0;
//     uint64_t nSubsidy = nBeginSubsidy;
//     while (nSubsidy != nEndSubsidy) {
//         int nJumpHeight = IniCfg().GetBlockSubsidyJumpHeight(nSubsidy - 1);
//         llProfits += calculateProfit(nValue, nSubsidy, nBeginHeight, nJumpHeight);
//         nBeginHeight = nJumpHeight;
//         nSubsidy -= 1;
//     }

//     llProfits += calculateProfit(nValue, nSubsidy, nBeginHeight, nEndHeight);
//     LogPrint("profits", "updateHeight:%d curHeight:%d freeze value:%lld\n",
//         lastVoteHeight, nCurHeight, voteFunds.begin()->GetVotedBcoins());

//     return llProfits;
// }

uint64_t CAccount::GetVotedBCoins() {
    uint64_t votes = 0;
    if (!voteFunds.empty()) {
        for (auto it = voteFunds.begin(); it != voteFunds.end(); it++) {
            votes += it->GetVotedBcoins(); //one bcoin one vote!
        }
    }
    return votes;
}

uint64_t CAccount::GetTotalBcoins() {
    uint64_t votedBcoins = GetVotedBCoins();
    return ( votedBcoins + bcoins );
}

bool CAccount::RegIDIsMature() const {
    return (!regID.IsEmpty()) &&
           ((regID.GetHeight() == 0) || (chainActive.Height() - (int)regID.GetHeight() > kRegIdMaturePeriodByBlock));
}

Object CAccount::ToJsonObj(bool isAddress) const {
    Array voteFundArray;
    for (auto &fund : voteFunds) {
        voteFundArray.push_back(fund.ToJson());
    }

    Object obj;
    obj.push_back(Pair("address", keyID.ToAddress()));
    obj.push_back(Pair("keyid", keyID.ToString()));
    obj.push_back(Pair("nickid", nickID.ToString()));
    obj.push_back(Pair("regid", regID.ToString()));
    obj.push_back(Pair("regid_mature", RegIDIsMature()));
    obj.push_back(Pair("pubkey", pubKey.ToString()));
    obj.push_back(Pair("miner_pubkey", minerPubKey.ToString()));
    obj.push_back(Pair("bcoins", bcoins));
    obj.push_back(Pair("scoins", scoins));
    obj.push_back(Pair("fcoins", fcoins));
    obj.push_back(Pair("received_votes", receivedVotes));
    obj.push_back(Pair("vote_list", voteFundArray));
    return obj;
}

string CAccount::ToString(bool isAddress) const {
    string str;
    str += strprintf("regID=%s, keyID=%s, nickID=%s, pubKey=%s, minerPubKey=%s, bcoins=%ld, scoins=%ld, fcoins=%ld, receivedVotes=%lld\n",
        regID.ToString(), keyID.GetHex().c_str(), nickID.ToString(), pubKey.ToString().c_str(),
        minerPubKey.ToString().c_str(), bcoins, scoins, fcoins, receivedVotes);
    str += "voteFunds list: \n";
    for (auto & fund : voteFunds) {
        str += fund.ToString();
    }
    return str;
}

bool CAccount::IsMoneyOverflow(uint64_t nAddMoney) {
    if (!CheckMoneyRange(nAddMoney))
        return ERRORMSG("money:%lld larger than MaxMoney");

    return true;
}

bool CAccount::OperateBalance(CoinType coinType, BalanceOpType opType, const uint64_t &value) {
    if (!IsMoneyOverflow(value))
        return false;

    if (keyID == uint160()) {
        return ERRORMSG("operate account's keyId is 0 error");
    }

    if (!value) //value is 0
        return true;

    LogPrint("balance_op", "before op: %s\n", ToString());
    int64 opValue = value;
    if (opType == MINUS_VALUE)
        opValue *= -1;

    switch (coinType) {
        case WICC:  bcoins += opValue; if (!IsMoneyOverflow(bcoins)) return false; break;
        case MICC:  fcoins += opValue; if (!IsMoneyOverflow(fcoins)) return false; break;
        case WUSD:  scoins += opValue; if (!IsMoneyOverflow(scoins)) return false; break;
        default: return ERRORMSG("coin type error!");
    }
    LogPrint("balance_op", "after op: %s\n", ToString());

    return true;
}

bool CAccount::ProcessDelegateVote(vector<CCandidateVote> & candidateVotes, const uint64_t nCurHeight) {
    uint64_t lastTotalVotes = GetVotedBCoins();

    for (auto operVote = candidateVotes.begin(); operVote != candidateVotes.end(); ++operVote) {
        const CUserID &voteId = operVote->fund.GetCandidateUid();
        vector<CCandidateVote>::iterator itfund =
            find_if(candidateVotes.begin(), candidateVotes.end(),
                    [voteId](CCandidateVote vote) { return vote.GetCandidateUid() == voteId; });

        int voteType = VoteType(operVote->operType);
        if (ADD_VALUE == voteType) {
            if (itfund != candidateVotes.end()) { //existing vote
                uint64_t currVotes = itfund->GetVotedBcoins();

                if (!IsMoneyOverflow(operVote->fund.GetVotedBcoins()))
                     return ERRORMSG("ProcessDelegateVote() : oper fund value exceeds maximum ");

                itfund->SetVotedBcoins( currVotes + operVote->fund.GetVotedBcoins() );

                if (!IsMoneyOverflow(itfund->GetVotedBcoins()))
                     return ERRORMSG("ProcessDelegateVote() : fund value exceeds maximum");

            } else { //new vote
               if (candidateVotes.size() == IniCfg().GetMaxVoteCandidateNum()) {
                   return ERRORMSG("ProcessDelegateVote() : MaxVoteCandidateNum reached. Must revoke old votes 1st.");
               }

               candidateVotes.push_back(operVote->fund);
            }
        } else if (MINUS_VALUE == voteType) {
            if  (itfund != candidateVotes.end()) { //existing vote
                uint64_t currVotes = itfund->GetVotedBcoins();

                if (!IsMoneyOverflow(operVote->fund.GetVotedBcoins()))
                    return ERRORMSG("ProcessDelegateVote() : oper fund value exceeds maximum ");

                if (itfund->GetVotedBcoins() < operVote->fund.GetVotedBcoins())
                    return ERRORMSG("ProcessDelegateVote() : oper fund value exceeds delegate fund value");

                itfund->SetVotedBcoins( currVotes - operVote->fund.GetVotedBcoins() );

                if (0 == itfund->GetVotedBcoins())
                    candidateVotes.erase(itfund);

            } else {
                return ERRORMSG("ProcessDelegateVote() : revocation votes not exist");
            }
        } else {
            return ERRORMSG("ProcessDelegateVote() : operType: %d invalid", voteType);
        }
    }

    // sort account votes after the operations against the new votes
    std::sort(voteFunds.begin(), voteFunds.end(), [](CCandidateVote fund1, CCandidateVote fund2) {
        return fund1.GetVotedBcoins() > fund2.GetVotedBcoins();
    });

    uint64_t newTotalVotes = GetVotedBCoins();
    uint64_t totalBcoins = bcoins + lastTotalVotes;
    if (totalBcoins < newTotalVotes) {
        return  ERRORMSG("ProcessDelegateVote() : delegate votes exceeds account bcoins");
    }
    bcoins = totalBcoins - newTotalVotes;
    return true;
}

bool CAccount::OperateVote(VoteType type, const uint64_t & values) {
    if(ADD_VALUE == type) {
        receivedVotes += values;
        if(!IsMoneyOverflow(receivedVotes)) {
            return ERRORMSG("OperateVote() : delegates total votes exceed maximum ");
        }
    } else if (MINUS_VALUE == type) {
        if(receivedVotes < values) {
            return ERRORMSG("OperateVote() : delegates total votes less than revocation vote value");
        }
        receivedVotes -= values;
    } else {
        return ERRORMSG("OperateVote() : CDelegateVoteTx ExecuteTx AccountVoteOper revocation votes are not exist");
    }
    return true;
}
