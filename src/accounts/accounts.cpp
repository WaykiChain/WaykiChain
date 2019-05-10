// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2019- The WaykiChain Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "accounts.h"
#include "database.h"
#include "main.h"

// /** account db cache*/
// extern CAccountViewCache *pAccountViewTip;

string CAccountLog::ToString() const {
    string str;
    str += strprintf(
        "Account log: keyId=%d regId=%s nickId=%s pubKey=%s minerPubKey=%s hasOpenCdp=%d "
        "bcoins=%lld lastVoteHeight=%lld receivedVotes=%lld \n",
        keyID.GetHex(), regID.ToString(), nickID.ToString(), pubKey.ToString(),
        minerPubKey.ToString(), hasOpenCdp, bcoins, lastVoteHeight, receivedVotes);
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
    lastVoteHeight = accountLog.lastVoteHeight;
    voteFunds      = accountLog.voteFunds;
    receivedVotes  = accountLog.receivedVotes;
    LogPrint("undo_account", "before operate:%s\n", ToString().c_str());
    return true;
}

uint64_t CAccount::GetAccountProfit(uint64_t nCurHeight) {
     if (voteFunds.empty()) {
        LogPrint("DEBUG", "1st-time vote by the account, hence no minting of interest.");
        lastVoteHeight = nCurHeight; //record the 1st-time vote block height into account
        return 0; // 0 for the very 1st vote
    }

    // 先判断计算分红的上下限区块高度是否落在同一个分红率区间
    uint64_t nBeginHeight = lastVoteHeight;
    uint64_t nEndHeight = nCurHeight;
    uint64_t nBeginSubsidy = IniCfg().GetBlockSubsidyCfg(lastVoteHeight);
    uint64_t nEndSubsidy = IniCfg().GetBlockSubsidyCfg(nCurHeight);
    uint64_t nValue = voteFunds.begin()->GetVoteCount();
    LogPrint("profits", "nBeginSubsidy:%lld nEndSubsidy:%lld nBeginHeight:%d nEndHeight:%d\n",
        nBeginSubsidy, nEndSubsidy, nBeginHeight, nEndHeight);

    // 计算分红
    auto calculateProfit = [](uint64_t nValue, uint64_t nSubsidy, int nBeginHeight, int nEndHeight) -> uint64_t {
        int64_t nHoldHeight = nEndHeight - nBeginHeight;
        int64_t nYearHeight = SysCfg().GetSubsidyHalvingInterval();
        uint64_t llProfits =  (uint64_t)(nValue * ((long double)nHoldHeight * nSubsidy / nYearHeight / 100));
        LogPrint("profits", "nValue:%lld nSubsidy:%lld nBeginHeight:%d nEndHeight:%d llProfits:%lld\n",
            nValue, nSubsidy, nBeginHeight, nEndHeight, llProfits);
        return llProfits;
    };

    // 如果属于同一个分红率区间，分红=区块高度差（持有高度）* 分红率；如果不属于同一个分红率区间，则需要根据分段函数累加每一段的分红
    uint64_t llProfits = 0;
    uint64_t nSubsidy = nBeginSubsidy;
    while (nSubsidy != nEndSubsidy) {
        int nJumpHeight = IniCfg().GetBlockSubsidyJumpHeight(nSubsidy - 1);
        llProfits += calculateProfit(nValue, nSubsidy, nBeginHeight, nJumpHeight);
        nBeginHeight = nJumpHeight;
        nSubsidy -= 1;
    }

    llProfits += calculateProfit(nValue, nSubsidy, nBeginHeight, nEndHeight);
    LogPrint("profits", "updateHeight:%d curHeight:%d freeze value:%lld\n",
        lastVoteHeight, nCurHeight, voteFunds.begin()->GetVoteCount());

    return llProfits;
}

uint64_t CAccount::GetVotedBCoins() {
    uint64_t votes = 0;
    if (!voteFunds.empty()) {
        for (auto it = voteFunds.begin(); it != voteFunds.end(); it++) {
            votes += it->GetVoteCount(); //one bcoin one vote!
        }
    }
    return votes;
}

uint64_t CAccount::GetTotalBcoins() {
    uint64_t votedBcoins = GetVotedBCoins();
    return ( votedBcoins + bcoins );
}

Object CAccount::ToJsonObj(bool isAddress) const {
    Array voteFundArray;
    for (auto &fund : voteFunds) {
        voteFundArray.push_back(fund.ToJson());
    }

    Object obj;
    bool isMature = (!regID.IsEmpty()) && ((regID.GetHeight() == 0) ||
                                           (chainActive.Height() - (int)regID.GetHeight() > kRegIdMaturePeriodByBlock))
                        ? true
                        : false;
    obj.push_back(Pair("address", keyID.ToAddress()));
    obj.push_back(Pair("keyid", keyID.ToString()));
    obj.push_back(Pair("nickid", nickID.ToString()));
    obj.push_back(Pair("regid", regID.ToString()));
    obj.push_back(Pair("regid_mature", isMature));
    obj.push_back(Pair("pubkey", pubKey.ToString()));
    obj.push_back(Pair("miner_pubkey", minerPubKey.ToString()));
    obj.push_back(Pair("bcoins", bcoins));
    obj.push_back(Pair("scoins", scoins));
    obj.push_back(Pair("fcoins", fcoins));
    obj.push_back(Pair("received_votes", receivedVotes));
    obj.push_back(Pair("last_vote_height", lastVoteHeight));
    obj.push_back(Pair("vote_list", voteFundArray));
    return obj;
}

string CAccount::ToString(bool isAddress) const {
    string str;
    str += strprintf("regID=%s, keyID=%s, nickID=%s, pubKey=%s, minerPubKey=%s, bcoins=%ld, scoins=%ld, fcoins=%ld, updateHeight=%d receivedVotes=%lld\n",
        regID.ToString(), keyID.GetHex().c_str(), nickID.ToString(), pubKey.ToString().c_str(),
        minerPubKey.ToString().c_str(), bcoins, scoins, fcoins, lastVoteHeight, receivedVotes);
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

bool CAccount::OperateAccount(OperType type, const uint64_t &value, const uint64_t nCurHeight) {
    LogPrint("op_account", "before operate:%s\n", ToString());
    if (!IsMoneyOverflow(value))
        return false;

    if (keyID == uint160()) {
        return ERRORMSG("operate account's keyId is 0 error");
    }

    if (!value)
        return true;

    switch (type) {
    case ADD_FREE: {
        bcoins += value;
        if (!IsMoneyOverflow(bcoins))
            return false;

        break;
    }
    case MINUS_FREE: {
        if (value > bcoins)
            return false;

        bcoins -= value;
        break;
    }
    default:
        return ERRORMSG("operate account type error!");
    }

    LogPrint("op_account", "after operate:%s\n", ToString());
    return true;
}

bool CAccount::ProcessDelegateVote(vector<COperVoteFund> & operVoteFunds, const uint64_t nCurHeight) {
    if (nCurHeight < lastVoteHeight) {
        LogPrint("ERROR", "nCurHeight (%d) < lastVoteHeight (%d)", nCurHeight, lastVoteHeight);
        return false;
    }

    uint64_t llProfit = GetAccountProfit(nCurHeight);
    lastVoteHeight = nCurHeight;

    if (!IsMoneyOverflow(llProfit)) return false;

    uint64_t totalVotes = GetVotedBCoins();

    for (auto operVote = operVoteFunds.begin(); operVote != operVoteFunds.end(); ++operVote) {
        const CUserID &voteId = operVote->fund.GetVoteId();
        vector<CVoteFund>::iterator itfund =
            find_if(voteFunds.begin(), voteFunds.end(),
                    [voteId](CVoteFund fund) { return fund.GetVoteId() == voteId; });

        int voteType = VoteOperType(operVote->operType);
        if (ADD_FUND == voteType) {
            if (itfund != voteFunds.end()) {
                uint64_t currVotes = itfund->GetVoteCount();

                if (!IsMoneyOverflow(operVote->fund.GetVoteCount()))
                     return ERRORMSG("ProcessDelegateVote() : oper fund value exceeds maximum ");

                itfund->SetVoteCount( currVotes + operVote->fund.GetVoteCount() );

                if (!IsMoneyOverflow(itfund->GetVoteCount()))
                     return ERRORMSG("ProcessDelegateVote() : fund value exceeds maximum");

            } else {
               voteFunds.push_back(operVote->fund);
               if (voteFunds.size() > IniCfg().GetDelegatesNum()) {
                   return ERRORMSG("ProcessDelegateVote() : fund number exceeds maximum");
               }
            }
        } else if (MINUS_FUND == voteType) {
            if  (itfund != voteFunds.end()) {
                uint64_t currVotes = itfund->GetVoteCount();

                if (!IsMoneyOverflow(operVote->fund.GetVoteCount()))
                    return ERRORMSG("ProcessDelegateVote() : oper fund value exceeds maximum ");

                if (itfund->GetVoteCount() < operVote->fund.GetVoteCount())
                    return ERRORMSG("ProcessDelegateVote() : oper fund value exceed delegate fund value");

                itfund->SetVoteCount( currVotes - operVote->fund.GetVoteCount() );

                if (0 == itfund->GetVoteCount())
                    voteFunds.erase(itfund);

            } else {
                return ERRORMSG("ProcessDelegateVote() : revocation votes not exist");
            }
        } else {
            return ERRORMSG("ProcessDelegateVote() : operType: %d invalid", voteType);
        }
    }

    // sort account votes after the operations against the new votes
    std::sort(voteFunds.begin(), voteFunds.end(), [](CVoteFund fund1, CVoteFund fund2) {
        return fund1.GetVoteCount() > fund2.GetVoteCount();
    });

    uint64_t newTotalVotes = GetVotedBCoins();
    if (bcoins + totalVotes < newTotalVotes) {
        return  ERRORMSG("ProcessDelegateVote() : delegate votes exceeds account bcoins");
    }
    bcoins = (bcoins + totalVotes) - newTotalVotes;
    bcoins += llProfit;
    LogPrint("profits", "received profits: %lld\n", llProfit);
    return true;
}

bool CAccount::OperateVote(VoteOperType type, const uint64_t & values) {
    if(ADD_FUND == type) {
        receivedVotes += values;
        if(!IsMoneyOverflow(receivedVotes)) {
            return ERRORMSG("OperateVote() : delegates total votes exceed maximum ");
        }
    } else if (MINUS_FUND == type) {
        if(receivedVotes < values) {
            return ERRORMSG("OperateVote() : delegates total votes less than revocation vote value");
        }
        receivedVotes -= values;
    } else {
        return ERRORMSG("OperateVote() : CDelegateVoteTx ExecuteTx AccountVoteOper revocation votes are not exist");
    }
    return true;
}
