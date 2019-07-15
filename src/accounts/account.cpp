// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "account.h"
#include "config/configuration.h"

#include "main.h"

string CAccountLog::ToString() const {
    string str;
    str += strprintf(
        "Account log: keyId=%d regId=%s nickId=%s pubKey=%s minerPubKey=%s "
        "bcoins=%lld receivedVotes=%lld \n",
        keyId.GetHex(), regId.ToString(), nickId.ToString(), pubKey.ToString(),
        minerPubKey.ToString(), bcoins, receivedVotes);

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

    LogPrint("undo_account", "before operate:%s\n", ToString());
    return true;
}

bool CAccount::FreezeDexCoin(CoinType coinType, uint64_t amount) {

    switch (coinType) {
        case WICC:
            if (amount > bcoins) return ERRORMSG("CAccount::FreezeDexCoin, amount larger than bcoins");
            bcoins -= amount;
            frozenDEXBcoins += amount;
            assert(IsMoneyValid(bcoins) && IsMoneyValid(frozenDEXBcoins));
            break;

        case WUSD:
            if (amount > scoins) return ERRORMSG("CAccount::FreezeDexCoin, amount larger than scoins");
            scoins -= amount;
            frozenDEXScoins += amount;
            assert(IsMoneyValid(scoins) && IsMoneyValid(frozenDEXScoins));
            break;

        case WGRT:
            if (amount > fcoins) return ERRORMSG("CAccount::FreezeDexCoin, amount larger than fcoins");
            fcoins -= amount;
            frozenDEXFcoins += amount;
            assert(IsMoneyValid(fcoins) && IsMoneyValid(frozenDEXFcoins));
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
            assert(IsMoneyValid(bcoins) && IsMoneyValid(frozenDEXBcoins));
            break;

        case WUSD:
            if (amount > frozenDEXScoins)
                return ERRORMSG("CAccount::UnFreezeDexCoin, amount larger than frozenDEXScoins");

            scoins += amount;
            frozenDEXScoins -= amount;
            assert(IsMoneyValid(scoins) && IsMoneyValid(frozenDEXScoins));
            break;

        case WGRT:
            if (amount > frozenDEXFcoins)
                return ERRORMSG("CAccount::UnFreezeDexCoin, amount larger than frozenDEXFcoins");

            fcoins += amount;
            frozenDEXFcoins -= amount;
            assert(IsMoneyValid(fcoins) && IsMoneyValid(frozenDEXFcoins));
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
            assert(IsMoneyValid(frozenDEXBcoins));
            break;
        case WGRT:
            if (coins > frozenDEXScoins) return ERRORMSG("CAccount::SettleDEXBuyOrder, minus scoins exceed frozen scoins");
            frozenDEXScoins -= coins;
            assert(IsMoneyValid(frozenDEXScoins));
            break;
        case WUSD:
            if (coins > frozenDEXFcoins) return ERRORMSG("CAccount::SettleDEXBuyOrder, minus fcoins exceed frozen fcoins");
            frozenDEXFcoins -= coins;
            assert(IsMoneyValid(frozenDEXFcoins));
            break;
        default: return ERRORMSG("CAccount::SettleDEXBuyOrder, coin type error");
    }
    return true;
}

uint64_t CAccount::ComputeVoteStakingInterest(const vector<CCandidateVote> &candidateVotes, const uint64_t currHeight) {
    if (candidateVotes.empty()) {
        LogPrint("DEBUG", "1st-time vote by the account, hence interest inflation.");
        return 0;  // 0 for the very 1st vote
    }

    uint64_t nBeginHeight  = lastVoteHeight;
    uint64_t nEndHeight    = currHeight;
    uint64_t nBeginSubsidy = IniCfg().GetBlockSubsidyCfg(lastVoteHeight);
    uint64_t nEndSubsidy   = IniCfg().GetBlockSubsidyCfg(currHeight);
    uint64_t nValue        = candidateVotes.begin()->GetVotedBcoins();
    LogPrint("DEBUG", "nBeginSubsidy:%lld nEndSubsidy:%lld nBeginHeight:%d nEndHeight:%d\n", nBeginSubsidy,
             nEndSubsidy, nBeginHeight, nEndHeight);

    auto computeInterest = [](uint64_t nValue, uint64_t nSubsidy, int nBeginHeight, int nEndHeight) -> uint64_t {
        int64_t nHoldHeight        = nEndHeight - nBeginHeight;
        static int64_t nYearHeight = SysCfg().GetSubsidyHalvingInterval();
        uint64_t interest         = (uint64_t)(nValue * ((long double)nHoldHeight * nSubsidy / nYearHeight / 100));
        LogPrint("DEBUG", "nValue:%lld nSubsidy:%lld nBeginHeight:%d nEndHeight:%d interest:%lld\n", nValue,
                 nSubsidy, nBeginHeight, nEndHeight, interest);
        return interest;
    };

    uint64_t interest = 0;
    uint64_t nSubsidy  = nBeginSubsidy;
    while (nSubsidy != nEndSubsidy) {
        int nJumpHeight = IniCfg().GetBlockSubsidyJumpHeight(nSubsidy - 1);
        interest += computeInterest(nValue, nSubsidy, nBeginHeight, nJumpHeight);
        nBeginHeight = nJumpHeight;
        nSubsidy -= 1;
    }

    interest += computeInterest(nValue, nSubsidy, nBeginHeight, nEndHeight);
    LogPrint("DEBUG", "updateHeight:%d currHeight:%d freeze value:%lld\n", lastVoteHeight, currHeight,
             candidateVotes.begin()->GetVotedBcoins());

    return interest;
}

uint64_t CAccount::ComputeBlockInflateInterest(const uint64_t currHeight) const {
    if (GetFeatureForkVersion(currHeight) == MAJOR_VER_R1) {
        return 0;
    }

    uint64_t subsidy          = IniCfg().GetBlockSubsidyCfg(currHeight);
    static int64_t holdHeight = 1;
    static int64_t yearHeight = SysCfg().GetSubsidyHalvingInterval();
    uint64_t profits =
        (long double)(receivedVotes * IniCfg().GetTotalDelegateNum() * holdHeight * subsidy) / yearHeight / 100;
    LogPrint("profits", "receivedVotes:%llu subsidy:%llu holdHeight:%lld yearHeight:%lld llProfits:%llu\n",
             receivedVotes, subsidy, holdHeight, yearHeight, profits);

    return profits;
}

uint64_t CAccount::GetVotedBCoins(const vector<CCandidateVote> &candidateVotes, const uint64_t currHeight) {
    uint64_t votes = 0;
    if (!candidateVotes.empty()) {
        if (GetFeatureForkVersion(currHeight) == MAJOR_VER_R1) {
            votes = candidateVotes[0].GetVotedBcoins(); // one bcoin eleven votes
        } else if (GetFeatureForkVersion(currHeight) == MAJOR_VER_R2) {
            for (const auto &vote : candidateVotes) {
                votes += vote.GetVotedBcoins();  // one bcoin one vote
            }
        }
    }
    return votes;
}

uint64_t CAccount::GetTotalBcoins(const vector<CCandidateVote> &candidateVotes, const uint64_t currHeight) {
    uint64_t votedBcoins = GetVotedBCoins(candidateVotes, currHeight);
    return (votedBcoins + bcoins);
}

bool CAccount::RegIDIsMature() const {
    return (!regId.IsEmpty()) &&
           ((regId.GetHeight() == 0) || (chainActive.Height() - (int)regId.GetHeight() > kRegIdMaturePeriodByBlock));
}

Object CAccount::ToJsonObj(bool isAddress) const {
    vector<CCandidateVote> candidateVotes;
    pCdMan->pDelegateCache->GetCandidateVotes(regId, candidateVotes);

    Array candidateVoteArray;
    for (auto &vote : candidateVotes) {
        candidateVoteArray.push_back(vote.ToJson());
    }

    Object obj;
    obj.push_back(Pair("address",           keyId.ToAddress()));
    obj.push_back(Pair("keyid",             keyId.ToString()));
    obj.push_back(Pair("nickid",            nickId.ToString()));
    obj.push_back(Pair("regid",             regId.ToString()));
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
        "regId=%s, keyId=%s, nickId=%s, pubKey=%s, minerPubKey=%s, bcoins=%ld, scoins=%ld, fcoins=%ld, "
        "receivedVotes=%lld\n",
        regId.ToString(), keyId.GetHex(), nickId.ToString(), pubKey.ToString(), minerPubKey.ToString(),
        bcoins, scoins, fcoins, receivedVotes);
    str += "candidate vote list: \n";

    vector<CCandidateVote> candidateVotes;
    pCdMan->pDelegateCache->GetCandidateVotes(regId, candidateVotes);
    for (auto & vote : candidateVotes) {
        str += vote.ToString();
    }

    return str;
}

bool CAccount::IsMoneyValid(uint64_t nAddMoney) {
    if (!CheckBaseCoinRange(nAddMoney))
        return ERRORMSG("money:%lld larger than MaxMoney", nAddMoney);

    return true;
}

bool CAccount::OperateBalance(const CoinType coinType, const BalanceOpType opType, const uint64_t value) {
    assert(opType == BalanceOpType::ADD_VALUE || opType == BalanceOpType::MINUS_VALUE);

    if (!IsMoneyValid(value))
        return false;

    if (keyId.IsEmpty()) {
        return ERRORMSG("operate account's keyId is empty error");
    }

    if (!value)  // value is 0
        return true;

    LogPrint("balance_op", "before op: %s\n", ToString());

    if (opType == BalanceOpType::MINUS_VALUE) {
        switch (coinType) {
            case WICC:  if (bcoins < value) return false; break;
            case WGRT:  if (fcoins < value) return false; break;
            case WUSD:  if (scoins < value) return false; break;
            default: return ERRORMSG("coin type error");
        }
    }

    int64_t opValue = (opType == BalanceOpType::MINUS_VALUE) ? (-value) : (value);
    switch (coinType) {
        case WICC:  bcoins += opValue; if (!IsMoneyValid(bcoins)) return false; break;
        case WGRT:  fcoins += opValue; if (!IsMoneyValid(fcoins)) return false; break;
        case WUSD:  scoins += opValue; if (!IsMoneyValid(scoins)) return false; break;
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

// bool CAccount::RedeemScoinsToCdp(const int64_t bcoinsToStake) {

//     return true;
// }

bool CAccount::ProcessDelegateVotes(const vector<CCandidateVote> &candidateVotesIn,
                                   vector<CCandidateVote> &candidateVotesInOut, const uint64_t currHeight) {
    if (currHeight < lastVoteHeight) {
        LogPrint("ERROR", "currHeight (%d) < lastVoteHeight (%d)", currHeight, lastVoteHeight);
        return false;
    }

    lastVoteHeight = currHeight;
    uint64_t lastTotalVotes = GetVotedBCoins(candidateVotesInOut, currHeight);

    for (const auto &vote : candidateVotesIn) {
        const CUserID &voteId = vote.GetCandidateUid();
        vector<CCandidateVote>::iterator itVote =
            find_if(candidateVotesInOut.begin(), candidateVotesInOut.end(),
                    [voteId](CCandidateVote vote) { return vote.GetCandidateUid() == voteId; });

        int voteType = VoteType(vote.GetCandidateVoteType());
        if (ADD_BCOIN == voteType) {
            if (itVote != candidateVotesInOut.end()) { //existing vote
                uint64_t currVotes = itVote->GetVotedBcoins();

                if (!IsMoneyValid(vote.GetVotedBcoins()))
                     return ERRORMSG("ProcessDelegateVotes() : oper fund value exceeds maximum ");

                itVote->SetVotedBcoins( currVotes + vote.GetVotedBcoins() );

                if (!IsMoneyValid(itVote->GetVotedBcoins()))
                     return ERRORMSG("ProcessDelegateVotes() : fund value exceeds maximum");

            } else { //new vote
               if (candidateVotesInOut.size() == IniCfg().GetMaxVoteCandidateNum()) {
                   return ERRORMSG("ProcessDelegateVotes() : MaxVoteCandidateNum reached. Must revoke old votes 1st.");
               }

               candidateVotesInOut.push_back(vote);
            }
        } else if (MINUS_BCOIN == voteType) {
            // if (currHeight - lastVoteHeight < 100) {
            //     return ERRORMSG("ProcessDelegateVotes() : last vote not cooled down yet: lastVoteHeigh=%d",
            //                     lastVoteHeight);
            // }
            if  (itVote != candidateVotesInOut.end()) { //existing vote
                uint64_t currVotes = itVote->GetVotedBcoins();

                if (!IsMoneyValid(vote.GetVotedBcoins()))
                    return ERRORMSG("ProcessDelegateVotes() : oper fund value exceeds maximum ");

                if (itVote->GetVotedBcoins() < vote.GetVotedBcoins())
                    return ERRORMSG("ProcessDelegateVotes() : oper fund value exceeds delegate fund value");

                itVote->SetVotedBcoins(currVotes - vote.GetVotedBcoins());

                if (0 == itVote->GetVotedBcoins())
                    candidateVotesInOut.erase(itVote);

            } else {
                return ERRORMSG("ProcessDelegateVotes() : revocation votes not exist");
            }
        } else {
            return ERRORMSG("ProcessDelegateVotes() : operType: %d invalid", voteType);
        }
    }

    // sort account votes after the operations against the new votes
    std::sort(candidateVotesInOut.begin(), candidateVotesInOut.end(), [](CCandidateVote vote1, CCandidateVote vote2) {
        return vote1.GetVotedBcoins() > vote2.GetVotedBcoins();
    });

    uint64_t newTotalVotes = GetVotedBCoins(candidateVotesInOut, currHeight);
    uint64_t totalBcoins = bcoins + lastTotalVotes;
    if (totalBcoins < newTotalVotes) {
        return  ERRORMSG("ProcessDelegateVotes() : delegate votes exceeds account bcoins");
    }
    bcoins = totalBcoins - newTotalVotes;

    uint64_t interestAmountToInflate = ComputeVoteStakingInterest(candidateVotesInOut, currHeight);
    if (!IsMoneyValid(interestAmountToInflate))
        return false;

    switch (GetFeatureForkVersion(currHeight)) {
        case MAJOR_VER_R1: 
            bcoins += interestAmountToInflate; // for backward compatibility
            break;

        case MAJOR_VER_R2:
            fcoins += interestAmountToInflate; // only fcoins will be inflated for voters
            break;

        default:
            return false;
    }

    LogPrint("INFLATE", "Account(%s) received vote staking interest amount (fcoins): %lld\n", 
            regId.ToString(), interestAmountToInflate);

    return true;
}

bool CAccount::StakeVoteBcoins(VoteType type, const uint64_t votes) {
    switch (type) {
        case ADD_BCOIN: {
            receivedVotes += votes;
            if (!IsMoneyValid(receivedVotes)) {
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
