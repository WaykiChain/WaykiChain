// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "account.h"
#include "configuration.h"

#include "main.h"

string CAccountLog::ToString() const {
    string str;
    str += strprintf(
        "Account log: keyId=%d regId=%s nickId=%s pubKey=%s minerPubKey=%s hasOpenCdp=%d "
        "bcoins=%lld receivedVotes=%lld \n",
        keyID.GetHex(), regID.ToString(), nickID.ToString(), pubKey.ToString(),
        minerPubKey.ToString(), hasOpenCdp, bcoins, receivedVotes);

    return str;
}

bool CAccount::UndoOperateAccount(const CAccountLog &accountLog) {
    LogPrint("undo_account", "after operate:%s\n", ToString());

    bcoins         = accountLog.bcoins;
    scoins         = accountLog.scoins;
    fcoins         = accountLog.fcoins;

    frozenDEXBcoins = accountLog.frozenDEXBcoins;
    frozenDEXScoins = accountLog.frozenDEXScoins;
    frozenDEXFcoins = accountLog.frozenDEXFcoins;
    stakedBcoins   = accountLog.stakedBcoins;
    stakedFcoins   = accountLog.stakedFcoins;
    receivedVotes  = accountLog.receivedVotes;
    lastVoteHeight = accountLog.lastVoteHeight;
    hasOpenCdp     = accountLog.hasOpenCdp;

    LogPrint("undo_account", "before operate:%s\n", ToString().c_str());
    return true;
}

bool CAccount::FreezeDexCoin(CoinType coinType, uint64_t amount) {

    switch (coinType) {
        case WICC:
            if (amount > bcoins) return ERRORMSG("CAccount::FreezeDexCoin, amount larger than bcoins");
            bcoins -= amount;
            frozenDEXBcoins += amount;
            assert(!IsMoneyOverflow(bcoins) && !IsMoneyOverflow(frozenDEXBcoins));
            break;
        case MICC:
            if (amount > scoins) return ERRORMSG("CAccount::FreezeDexCoin, amount larger than scoins");
            scoins -= amount;
            frozenDEXScoins += amount;
            assert(!IsMoneyOverflow(scoins) && !IsMoneyOverflow(frozenDEXScoins));
            break;
        case WUSD:
            if (amount > fcoins) return ERRORMSG("CAccount::FreezeDexCoin, amount larger than fcoins");
            fcoins -= amount;
            frozenDEXFcoins += amount;
            assert(!IsMoneyOverflow(fcoins) && !IsMoneyOverflow(frozenDEXFcoins));
            break;
        default: return ERRORMSG("CAccount::FreezeDexCoin, coin type error");
    }
    return true;
}

bool CAccount::UnFreezeDexCoin(CoinType coinType, uint64_t amount) {
    switch (coinType) {
        case WICC:
            if (amount > frozenDEXBcoins)
                return ERRORMSG("CAccount::UnFreezeDexCoin, amount larger than frozenDEXBcoins");
            bcoins += amount;
            frozenDEXBcoins -= amount;
            assert(!IsMoneyOverflow(bcoins) && !IsMoneyOverflow(frozenDEXBcoins));
            break;
        case MICC:
            if (amount > frozenDEXScoins)
                return ERRORMSG("CAccount::UnFreezeDexCoin, amount larger than frozenDEXScoins");
            scoins += amount;
            frozenDEXScoins -= amount;
            assert(!IsMoneyOverflow(scoins) && !IsMoneyOverflow(frozenDEXScoins));
            break;
        case WUSD:
            if (amount > frozenDEXFcoins)
                return ERRORMSG("CAccount::UnFreezeDexCoin, amount larger than frozenDEXFcoins");
            fcoins += amount;
            frozenDEXFcoins -= amount;
            assert(!IsMoneyOverflow(fcoins) && !IsMoneyOverflow(frozenDEXFcoins));
            break;
        default: return ERRORMSG("CAccount::UnFreezeDexCoin, coin type error");
    }
    return true;
}

bool CAccount::MinusDEXFrozenCoin(CoinType coinType,  uint64_t coins) {
    switch (coinType) {
        case WICC:
            if (coins > frozenDEXBcoins) return ERRORMSG("CAccount::SettleDEXBuyOrder, minus bcoins exceed frozen bcoins");
            frozenDEXBcoins -= coins;
            assert(!IsMoneyOverflow(frozenDEXBcoins));
            break;
        case MICC:
            if (coins > frozenDEXScoins) return ERRORMSG("CAccount::SettleDEXBuyOrder, minus scoins exceed frozen scoins");
            frozenDEXScoins -= coins;
            assert(!IsMoneyOverflow(frozenDEXScoins));
            break;
        case WUSD:
            if (coins > frozenDEXFcoins) return ERRORMSG("CAccount::SettleDEXBuyOrder, minus fcoins exceed frozen fcoins");
            frozenDEXFcoins -= coins;
            assert(!IsMoneyOverflow(frozenDEXFcoins));
            break;
        default: return ERRORMSG("CAccount::SettleDEXBuyOrder, coin type error");
    }
    return true;
}

uint64_t CAccount::GetAccountProfit(const vector<CCandidateVote> &candidateVotes, const uint64_t curHeight) {
    if (GetFeatureForkVersion(curHeight) == MAJOR_VER_R2) {
        // The rule is one bcoin one vote, hence no profits at all and return 0.
        return 0;
    }

    if (candidateVotes.empty()) {
        LogPrint("DEBUG", "1st-time vote by the account, hence no minting of interest.");
        return 0;  // 0 for the very 1st vote
    }

    uint64_t nBeginHeight  = lastVoteHeight;
    uint64_t nEndHeight    = curHeight;
    uint64_t nBeginSubsidy = IniCfg().GetBlockSubsidyCfg(lastVoteHeight);
    uint64_t nEndSubsidy   = IniCfg().GetBlockSubsidyCfg(curHeight);
    uint64_t nValue        = candidateVotes.begin()->GetVotedBcoins();
    LogPrint("profits", "nBeginSubsidy:%lld nEndSubsidy:%lld nBeginHeight:%d nEndHeight:%d\n", nBeginSubsidy,
             nEndSubsidy, nBeginHeight, nEndHeight);

    auto calculateProfit = [](uint64_t nValue, uint64_t nSubsidy, int nBeginHeight, int nEndHeight) -> uint64_t {
        int64_t nHoldHeight        = nEndHeight - nBeginHeight;
        static int64_t nYearHeight = SysCfg().GetSubsidyHalvingInterval();
        uint64_t llProfits         = (uint64_t)(nValue * ((long double)nHoldHeight * nSubsidy / nYearHeight / 100));
        LogPrint("profits", "nValue:%lld nSubsidy:%lld nBeginHeight:%d nEndHeight:%d llProfits:%lld\n", nValue,
                 nSubsidy, nBeginHeight, nEndHeight, llProfits);
        return llProfits;
    };

    uint64_t llProfits = 0;
    uint64_t nSubsidy  = nBeginSubsidy;
    while (nSubsidy != nEndSubsidy) {
        int nJumpHeight = IniCfg().GetBlockSubsidyJumpHeight(nSubsidy - 1);
        llProfits += calculateProfit(nValue, nSubsidy, nBeginHeight, nJumpHeight);
        nBeginHeight = nJumpHeight;
        nSubsidy -= 1;
    }

    llProfits += calculateProfit(nValue, nSubsidy, nBeginHeight, nEndHeight);
    LogPrint("profits", "updateHeight:%d curHeight:%d freeze value:%lld\n", lastVoteHeight, curHeight,
             candidateVotes.begin()->GetVotedBcoins());

    return llProfits;
}

uint64_t CAccount::CalculateAccountProfit(const uint64_t curHeight) const {
    if (GetFeatureForkVersion(curHeight) == MAJOR_VER_R1) {
        return 0;
    }

    uint64_t subsidy          = IniCfg().GetBlockSubsidyCfg(curHeight);
    static int64_t holdHeight = 1;
    static int64_t yearHeight = SysCfg().GetSubsidyHalvingInterval();
    uint64_t profits =
        (long double)(receivedVotes * IniCfg().GetTotalDelegateNum() * holdHeight * subsidy) / yearHeight / 100;
    LogPrint("profits", "receivedVotes:%llu subsidy:%llu holdHeight:%lld yearHeight:%lld llProfits:%llu\n",
             receivedVotes, subsidy, holdHeight, yearHeight, profits);

    return profits;
}

uint64_t CAccount::GetVotedBCoins(const vector<CCandidateVote> &candidateVotes, const uint64_t curHeight) {
    uint64_t votes = 0;
    if (!candidateVotes.empty()) {
        if (GetFeatureForkVersion(curHeight) == MAJOR_VER_R1) {
            votes = candidateVotes[0].GetVotedBcoins(); // one bcoin eleven votes
        } else if (GetFeatureForkVersion(curHeight) == MAJOR_VER_R2) {
            for (const auto &vote : candidateVotes) {
                votes += vote.GetVotedBcoins();  // one bcoin one vote
            }
        }
    }
    return votes;
}

uint64_t CAccount::GetTotalBcoins(const vector<CCandidateVote> &candidateVotes, const uint64_t curHeight) {
    uint64_t votedBcoins = GetVotedBCoins(candidateVotes, curHeight);
    return (votedBcoins + bcoins);
}

bool CAccount::RegIDIsMature() const {
    return (!regID.IsEmpty()) &&
           ((regID.GetHeight() == 0) || (chainActive.Height() - (int)regID.GetHeight() > kRegIdMaturePeriodByBlock));
}

Object CAccount::ToJsonObj(bool isAddress) const {
    vector<CCandidateVote> candidateVotes;
    pCdMan->pDelegateCache->GetCandidateVotes(regID, candidateVotes);

    Array candidateVoteArray;
    for (auto &vote : candidateVotes) {
        candidateVoteArray.push_back(vote.ToJson());
    }

    Object obj;
    obj.push_back(Pair("address",           keyID.ToAddress()));
    obj.push_back(Pair("keyid",             keyID.ToString()));
    obj.push_back(Pair("nickid",            nickID.ToString()));
    obj.push_back(Pair("regid",             regID.ToString()));
    obj.push_back(Pair("regid_mature",      RegIDIsMature()));
    obj.push_back(Pair("pubkey",            pubKey.ToString()));
    obj.push_back(Pair("miner_pubkey",      minerPubKey.ToString()));
    obj.push_back(Pair("bcoins",            bcoins));
    obj.push_back(Pair("scoins",            scoins));
    obj.push_back(Pair("fcoins",            fcoins));
    obj.push_back(Pair("staked_bcoins",     stakedBcoins));
    obj.push_back(Pair("staked_fcoins",     stakedFcoins));
    obj.push_back(Pair("received_votes",    receivedVotes));
    obj.push_back(Pair("vote_list",         candidateVoteArray));

    return obj;
}

string CAccount::ToString(bool isAddress) const {
    string str;
    str += strprintf(
        "regID=%s, keyID=%s, nickID=%s, pubKey=%s, minerPubKey=%s, bcoins=%ld, scoins=%ld, fcoins=%ld, "
        "receivedVotes=%lld\n",
        regID.ToString(), keyID.GetHex().c_str(), nickID.ToString(), pubKey.ToString().c_str(),
        minerPubKey.ToString().c_str(), bcoins, scoins, fcoins, receivedVotes);
    str += "candidate vote list: \n";

    vector<CCandidateVote> candidateVotes;
    pCdMan->pDelegateCache->GetCandidateVotes(regID, candidateVotes);
    for (auto & vote : candidateVotes) {
        str += vote.ToString();
    }

    return str;
}

bool CAccount::IsMoneyOverflow(uint64_t nAddMoney) {
    if (!CheckBaseCoinRange(nAddMoney))
        return ERRORMSG("money:%lld larger than MaxMoney", nAddMoney);

    return true;
}

bool CAccount::OperateBalance(const CoinType coinType, const BalanceOpType opType, const uint64_t value) {
    assert(opType == BalanceOpType::ADD_VALUE || opType == BalanceOpType::MINUS_VALUE);

    if (!IsMoneyOverflow(value))
        return false;

    if (keyID == uint160()) {
        return ERRORMSG("operate account's keyId is 0 error");
    }

    if (!value)  // value is 0
        return true;

    LogPrint("balance_op", "before op: %s\n", ToString());

    if (opType == BalanceOpType::MINUS_VALUE) {
        switch (coinType) {
            case WICC:  if (bcoins < value) return false; break;
            case MICC:  if (fcoins < value) return false; break;
            case WUSD:  if (scoins < value) return false; break;
            default: return ERRORMSG("coin type error");
        }
    }

    int64_t opValue = (opType == BalanceOpType::MINUS_VALUE) ? (-value) : (value);
    switch (coinType) {
        case WICC:  bcoins += opValue; if (!IsMoneyOverflow(bcoins)) return false; break;
        case MICC:  fcoins += opValue; if (!IsMoneyOverflow(fcoins)) return false; break;
        case WUSD:  scoins += opValue; if (!IsMoneyOverflow(scoins)) return false; break;
        default: return ERRORMSG("coin type error");
    }

    LogPrint("balance_op", "after op: %s\n", ToString());

    return true;
}

bool CAccount::PayInterest(uint64_t scoinInterest, uint64_t fcoinsInterest) {
    if (scoins < scoinInterest || fcoins < fcoinsInterest)
        return false;

    scoins -= scoinInterest;
    fcoins -= fcoinsInterest;
    return true;
}

bool CAccount::StakeFcoins(const int64_t fcoinsToStake) {
     if (fcoinsToStake < 0) {
        if (this->stakedFcoins < (uint64_t) (-1 * fcoinsToStake)) {
            return ERRORMSG("No sufficient staked fcoins(%d) to revoke", fcoins);
        }
    } else { // > 0
        if (this->fcoins < (uint64_t) fcoinsToStake) {
            return ERRORMSG("No sufficient fcoins(%d) in account to stake", fcoins);
        }
    }

    fcoins -= fcoinsToStake;
    stakedFcoins += fcoinsToStake;

    return true;
}

bool CAccount::StakeBcoinsToCdp(CoinType coinType, const int64_t bcoinsToStake, const int64_t mintedScoins) {
     if (bcoinsToStake < 0) {
        return ERRORMSG("bcoinsToStake(%d) cannot be negative", bcoinsToStake);
    } else { // > 0
        if (this->bcoins < (uint64_t) bcoinsToStake) {
            return ERRORMSG("No sufficient bcoins(%d) in account to stake", bcoins);
        }
    }

    bcoins -= bcoinsToStake;
    stakedBcoins += bcoinsToStake;
    scoins += mintedScoins;

    return true;
}

bool CAccount::RedeemScoinsToCdp(const int64_t bcoinsToStake) {

    return true;
}

bool CAccount::LiquidateCdp(const int64_t bcoinsToStake) {

    return true;
}

bool CAccount::ProcessDelegateVote(const vector<CCandidateVote> &candidateVotesIn,
                                   vector<CCandidateVote> &candidateVotesInOut, const uint64_t curHeight) {
    if (curHeight < lastVoteHeight) {
        LogPrint("ERROR", "curHeight (%d) < lastVoteHeight (%d)", curHeight, lastVoteHeight);
        return false;
    }

    uint64_t llProfit = GetAccountProfit(candidateVotesInOut, curHeight);
    if (!IsMoneyOverflow(llProfit))
        return false;

    lastVoteHeight = curHeight;

    uint64_t lastTotalVotes = GetVotedBCoins(candidateVotesInOut, curHeight);

    for (const auto &vote : candidateVotesIn) {
        const CUserID &voteId = vote.GetCandidateUid();
        vector<CCandidateVote>::iterator itVote =
            find_if(candidateVotesInOut.begin(), candidateVotesInOut.end(),
                    [voteId](CCandidateVote vote) { return vote.GetCandidateUid() == voteId; });

        int voteType = VoteType(vote.GetCandidateVoteType());
        if (ADD_BCOIN == voteType) {
            if (itVote != candidateVotesInOut.end()) { //existing vote
                uint64_t currVotes = itVote->GetVotedBcoins();

                if (!IsMoneyOverflow(vote.GetVotedBcoins()))
                     return ERRORMSG("ProcessDelegateVote() : oper fund value exceeds maximum ");

                itVote->SetVotedBcoins( currVotes + vote.GetVotedBcoins() );

                if (!IsMoneyOverflow(itVote->GetVotedBcoins()))
                     return ERRORMSG("ProcessDelegateVote() : fund value exceeds maximum");

            } else { //new vote
               if (candidateVotesInOut.size() == IniCfg().GetMaxVoteCandidateNum()) {
                   return ERRORMSG("ProcessDelegateVote() : MaxVoteCandidateNum reached. Must revoke old votes 1st.");
               }

               candidateVotesInOut.push_back(vote);
            }
        } else if (MINUS_BCOIN == voteType) {
            if  (itVote != candidateVotesInOut.end()) { //existing vote
                uint64_t currVotes = itVote->GetVotedBcoins();

                if (!IsMoneyOverflow(vote.GetVotedBcoins()))
                    return ERRORMSG("ProcessDelegateVote() : oper fund value exceeds maximum ");

                if (itVote->GetVotedBcoins() < vote.GetVotedBcoins())
                    return ERRORMSG("ProcessDelegateVote() : oper fund value exceeds delegate fund value");

                itVote->SetVotedBcoins(currVotes - vote.GetVotedBcoins());

                if (0 == itVote->GetVotedBcoins())
                    candidateVotesInOut.erase(itVote);

            } else {
                return ERRORMSG("ProcessDelegateVote() : revocation votes not exist");
            }
        } else {
            return ERRORMSG("ProcessDelegateVote() : operType: %d invalid", voteType);
        }
    }

    // sort account votes after the operations against the new votes
    std::sort(candidateVotesInOut.begin(), candidateVotesInOut.end(), [](CCandidateVote vote1, CCandidateVote vote2) {
        return vote1.GetVotedBcoins() > vote2.GetVotedBcoins();
    });

    uint64_t newTotalVotes = GetVotedBCoins(candidateVotesInOut, curHeight);
    uint64_t totalBcoins = bcoins + lastTotalVotes;
    if (totalBcoins < newTotalVotes) {
        return  ERRORMSG("ProcessDelegateVote() : delegate votes exceeds account bcoins");
    }
    bcoins = totalBcoins - newTotalVotes;
    bcoins += llProfit; // In one bcoin one vote, the profit will always be 0.
    LogPrint("profits", "received profits: %lld\n", llProfit);

    return true;
}

bool CAccount::StakeVoteBcoins(VoteType type, const uint64_t votes) {
    switch (type) {
        case ADD_BCOIN: {
            receivedVotes += votes;
            if (!IsMoneyOverflow(receivedVotes)) {
                return ERRORMSG("StakeVoteBcoins() : delegates total votes exceed maximum ");
            }

            break;
        }

        case MINUS_BCOIN: {
            if (receivedVotes < votes) {
                return ERRORMSG("StakeVoteBcoins() : delegates total votes less than revocation vote number");
            }
            receivedVotes -= votes;

            break;
        }

        default:
            return ERRORMSG("StakeVoteBcoins() : CDelegateVoteTx ExecuteTx AccountVoteOper revocation votes are not exist");
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// class CVmOperate

Object CVmOperate::ToJson() {
    Object obj;
    if (accountType == regid) {
        vector<unsigned char> vRegId(accountId, accountId + 6);
        CRegID regId(vRegId);
        obj.push_back(Pair("regid", regId.ToString()));
    } else if (accountType == base58addr) {
        string addr(accountId, accountId + sizeof(accountId));
        obj.push_back(Pair("addr", addr));
    }

    if (opType == ADD_BCOIN) {
        obj.push_back(Pair("opertype", "add"));
    } else if (opType == MINUS_BCOIN) {
        obj.push_back(Pair("opertype", "minus"));
    }

    if (timeoutHeight > 0) obj.push_back(Pair("outHeight", (int)timeoutHeight));

    uint64_t amount;
    memcpy(&amount, money, sizeof(money));
    obj.push_back(Pair("amount", amount));
    return obj;
}
