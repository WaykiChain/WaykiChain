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
        "Account log: keyid=%d regid=%s nickid=%s owner_pubKey=%s miner_pubKey=%s "
        "free_bcoins=%lld free_scoins=%lld free_fcoins=%lld"
        "staked_bcoins=%lld staked_fcoins=%lld received_votes=%lld last_vote_height=%lld\n",
        keyid.GetHex(), regid.ToString(), nickid.ToString(), owner_pubkey.ToString(), miner_pubkey.ToString(),
        free_bcoins, free_scoins, free_fcoins, staked_bcoins, staked_fcoins, received_votes, last_vote_height);

    return str;
}

bool CAccount::UndoOperateAccount(const CAccountLog &accountLog) {
    LogPrint("undo_account", "after operate:%s\n", ToString());

    free_bcoins     = accountLog.free_bcoins;
    free_scoins     = accountLog.free_scoins;
    free_fcoins     = accountLog.free_fcoins;
    frozen_bcoins   = accountLog.frozen_bcoins;
    frozen_scoins   = accountLog.frozen_scoins;
    frozen_fcoins   = accountLog.frozen_fcoins;
    staked_bcoins   = accountLog.staked_bcoins;
    staked_fcoins   = accountLog.staked_fcoins;
    received_votes  = accountLog.received_votes;
    last_vote_height= accountLog.last_vote_height;

    LogPrint("undo_account", "before operate:%s\n", ToString());
    return true;
}

bool CAccount::FreezeDexCoin(CoinType coinType, uint64_t amount) {
    switch (coinType) {
        case WICC:
            if (amount > free_bcoins) return ERRORMSG("CAccount::FreezeDexCoin, amount larger than bcoins");
            free_bcoins -= amount;
            frozen_bcoins += amount;
            assert(IsBcoinWithinRange(free_bcoins) && IsBcoinWithinRange(frozen_bcoins));
            break;

        case WUSD:
            if (amount > free_scoins) return ERRORMSG("CAccount::FreezeDexCoin, amount larger than scoins");
            free_scoins -= amount;
            frozen_scoins += amount;
            break;

        case WGRT:
            if (amount > free_fcoins) return ERRORMSG("CAccount::FreezeDexCoin, amount larger than fcoins");
            free_fcoins -= amount;
            frozen_fcoins += amount;
            assert(IsFcoinWithinRange(free_fcoins) && IsFcoinWithinRange(frozen_fcoins));
            break;

        default: return ERRORMSG("CAccount::FreezeDexCoin, coin type error");
    }
    return true;
}

bool CAccount::UnFreezeDexCoin(CoinType coinType, uint64_t amount) {
    switch (coinType) {
        case WICC:
            if (amount > frozen_bcoins)
                return ERRORMSG("CAccount::UnFreezeDexCoin, amount larger than frozen_bcoins");

            free_bcoins += amount;
            frozen_bcoins -= amount;
            assert(IsBcoinWithinRange(free_bcoins) && IsBcoinWithinRange(frozen_bcoins));
            break;

        case WUSD:
            if (amount > frozen_scoins)
                return ERRORMSG("CAccount::UnFreezeDexCoin, amount larger than frozen_scoins");

            free_scoins += amount;
            frozen_scoins -= amount;
            break;

        case WGRT:
            if (amount > frozen_fcoins)
                return ERRORMSG("CAccount::UnFreezeDexCoin, amount larger than frozen_fcoins");

            free_fcoins += amount;
            frozen_fcoins -= amount;
            assert(IsFcoinWithinRange(free_fcoins) && IsFcoinWithinRange(frozen_fcoins));
            break;

        default: return ERRORMSG("CAccount::UnFreezeDexCoin, coin type error");
    }
    return true;
}

bool CAccount::MinusDEXFrozenCoin(CoinType coinType,  uint64_t coins) {
    switch (coinType) {
        case WICC:
            if (coins > frozen_bcoins)
                return ERRORMSG("CAccount::SettleDEXBuyOrder, minus bcoins exceed frozen bcoins");

            frozen_bcoins -= coins;
            assert(IsBcoinWithinRange(frozen_bcoins));
            break;
        case WGRT:
            if (coins > frozen_scoins)
                return ERRORMSG("CAccount::SettleDEXBuyOrder, minus scoins exceed frozen scoins");

            frozen_scoins -= coins;
            assert(IsFcoinWithinRange(frozen_scoins));
            break;
        case WUSD:
            if (coins > frozen_fcoins)
                return ERRORMSG("CAccount::SettleDEXBuyOrder, minus fcoins exceed frozen fcoins");

            frozen_fcoins -= coins;
            break;
        default: return ERRORMSG("CAccount::SettleDEXBuyOrder, coin type error");
    }
    return true;
}

uint64_t CAccount::ComputeVoteStakingInterest(  const vector<CCandidateVote> &candidateVotes,
                                                const uint64_t currHeight) {
    if (candidateVotes.empty()) {
        LogPrint("DEBUG", "1st-time vote by the account, hence interest inflation.");
        return 0;  // 0 for the very 1st vote
    }

    uint64_t nBeginHeight  = last_vote_height;
    uint64_t nEndHeight    = currHeight;
    uint64_t nBeginSubsidy = IniCfg().GetBlockSubsidyCfg(last_vote_height);
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
    LogPrint("DEBUG", "updateHeight:%d currHeight:%d freeze value:%lld\n", last_vote_height, currHeight,
             candidateVotes.begin()->GetVotedBcoins());

    return interest;
}

uint64_t CAccount::ComputeBlockInflateInterest(const uint64_t currHeight) const {
    if (GetFeatureForkVersion(currHeight) == MAJOR_VER_R1)
        return 0;

    uint64_t subsidy          = IniCfg().GetBlockSubsidyCfg(currHeight);
    static int64_t holdHeight = 1;
    static int64_t yearHeight = SysCfg().GetSubsidyHalvingInterval();
    uint64_t profits = (long double)(received_votes * IniCfg().GetTotalDelegateNum() * holdHeight * subsidy) / yearHeight / 100;
    LogPrint("profits", "received_votes:%llu subsidy:%llu holdHeight:%lld yearHeight:%lld llProfits:%llu\n",
             received_votes, subsidy, holdHeight, yearHeight, profits);

    return profits;
}

uint64_t CAccount::GetVotedBCoins(const vector<CCandidateVote> &candidateVotes, const uint64_t currHeight) {
    uint64_t votes = 0;
    if (!candidateVotes.empty()) {
        if (GetFeatureForkVersion(currHeight) == MAJOR_VER_R1) {
            votes = candidateVotes[0].GetVotedBcoins(); // one bcoin eleven votes
        } else if (GetFeatureForkVersion(currHeight) == MAJOR_VER_R2) {
            for (const auto &vote : candidateVotes) {
                votes += vote.GetVotedBcoins();         // one bcoin one vote
            }
        }
    }
    return votes;
}

uint64_t CAccount::GetTotalBcoins(const vector<CCandidateVote> &candidateVotes, const uint64_t currHeight) {
    uint64_t votedBcoins = GetVotedBCoins(candidateVotes, currHeight);
    return (votedBcoins + free_bcoins);
}

bool CAccount::RegIDIsMature() const {
    return (!regid.IsEmpty()) &&
           ((regid.GetHeight() == 0) ||
            (chainActive.Height() - (int)regid.GetHeight() > kRegIdMaturePeriodByBlock));
}

Object CAccount::ToJsonObj(bool isAddress) const {
    vector<CCandidateVote> candidateVotes;
    pCdMan->pDelegateCache->GetCandidateVotes(regid, candidateVotes);

    Array candidateVoteArray;
    for (auto &vote : candidateVotes) {
        candidateVoteArray.push_back(vote.ToJson());
    }

    Object obj;
    obj.push_back(Pair("address",           keyid.ToAddress()));
    obj.push_back(Pair("keyid",             keyid.ToString()));
    obj.push_back(Pair("nickid",            nickid.ToString()));
    obj.push_back(Pair("regid",             regid.ToString()));
    obj.push_back(Pair("regid_mature",      RegIDIsMature()));
    obj.push_back(Pair("owner_pubkey",      owner_pubkey.ToString()));
    obj.push_back(Pair("miner_pubkey",      miner_pubkey.ToString()));
    obj.push_back(Pair("free_bcoins",       free_bcoins));
    obj.push_back(Pair("free_scoins",       free_scoins));
    obj.push_back(Pair("free_fcoins",       free_fcoins));
    obj.push_back(Pair("staked_bcoins",     staked_bcoins));
    obj.push_back(Pair("staked_fcoins",     staked_fcoins));
    obj.push_back(Pair("received_votes",    received_votes));
    obj.push_back(Pair("vote_list",         candidateVoteArray));

    return obj;
}

string CAccount::ToString(bool isAddress) const {
    string str;
    str += strprintf(
        "regid=%s, keyid=%s, nickId=%s, owner_pubkey=%s, miner_pubkey=%s, free_bcoins=%ld, free_scoins=%ld, free_fcoins=%ld, "
        "received_votes=%lld last_vote_height=%\n",
        regid.ToString(), keyid.GetHex(), nickid.ToString(), owner_pubkey.ToString(), miner_pubkey.ToString(),
        free_bcoins, free_scoins, free_fcoins, received_votes);
    str += "candidate vote list: \n";

    vector<CCandidateVote> candidateVotes;
    pCdMan->pDelegateCache->GetCandidateVotes(regid, candidateVotes);
    for (auto & vote : candidateVotes) {
        str += vote.ToString();
    }

    return str;
}

bool CAccount::IsBcoinWithinRange(uint64_t nAddMoney) {
    if (!CheckBaseCoinRange(nAddMoney))
        return ERRORMSG("money:%lld larger than MaxMoney", nAddMoney);

    return true;
}

bool CAccount::IsFcoinWithinRange(uint64_t nAddMoney) {
    if (!CheckFundCoinRange(nAddMoney))
        return ERRORMSG("money:%lld larger than MaxMoney", nAddMoney);

    return true;
}

bool CAccount::OperateBalance(const CoinType coinType, const BalanceOpType opType, const uint64_t value) {
    assert(opType == BalanceOpType::ADD_VALUE || opType == BalanceOpType::MINUS_VALUE);

    if (!IsBcoinWithinRange(value))
        return false;

    if (keyid.IsEmpty()) {
        return ERRORMSG("operate account's keyId is empty error");
    }

    if (!value)  // value is 0
        return true;

    LogPrint("balance_op", "before op: %s\n", ToString());

    if (opType == BalanceOpType::MINUS_VALUE) {
        switch (coinType) {
            case WICC:  if (free_bcoins < value) return false; break;
            case WGRT:  if (free_fcoins < value) return false; break;
            case WUSD:  if (free_scoins < value) return false; break;
            default: return ERRORMSG("coin type error");
        }
    }

    int64_t opValue = (opType == BalanceOpType::MINUS_VALUE) ? (-value) : (value);
    switch (coinType) {
        case WICC:  free_bcoins += opValue; if (!IsBcoinWithinRange(free_bcoins)) return false; break;
        case WGRT:  free_fcoins += opValue; if (!IsFcoinWithinRange(free_fcoins)) return false; break;
        case WUSD:  free_scoins += opValue; break;
        default: return ERRORMSG("coin type error");
    }

    LogPrint("balance_op", "after op: %s\n", ToString());

    return true;
}

bool CAccount::PayInterest(uint64_t scoinInterest, uint64_t fcoinsInterest) {
    if (free_scoins < scoinInterest || free_fcoins < fcoinsInterest)
        return false;

    free_scoins -= scoinInterest;
    free_fcoins -= fcoinsInterest;
    return true;
}

bool CAccount::StakeFcoins(const int64_t fcoinsToStake) {
    if (fcoinsToStake < 0) {
        if (this->staked_fcoins < (uint64_t)(-1 * fcoinsToStake)) {
            return ERRORMSG("No sufficient staked fcoins(%d) to revoke", staked_fcoins);
        }
    } else {  // > 0
        if (this->free_fcoins < (uint64_t)fcoinsToStake) {
            return ERRORMSG("No sufficient free_fcoins(%d) in account to stake", free_fcoins);
        }
    }

    free_fcoins     -= fcoinsToStake;
    staked_fcoins   += fcoinsToStake;

    return true;
}

bool CAccount::StakeBcoinsToCdp(CoinType coinType, const int64_t bcoinsToStake, const int64_t mintedScoins) {
     if (bcoinsToStake < 0) {
        return ERRORMSG("bcoinsToStake(%d) cannot be negative", bcoinsToStake);
    } else { // > 0
        if (this->free_bcoins < (uint64_t) bcoinsToStake) {
            return ERRORMSG("No sufficient free_bcoins(%d) in account to stake", free_bcoins);
        }
    }

    free_bcoins     -= bcoinsToStake;
    staked_bcoins   += bcoinsToStake;
    free_scoins     += mintedScoins;

    return true;
}

bool CAccount::ProcessDelegateVotes(const vector<CCandidateVote> &candidateVotesIn,
                                    vector<CCandidateVote> &candidateVotesInOut, const uint64_t currHeight,
                                    const CAccountDBCache *pAccountCache) {
    if (currHeight < last_vote_height) {
        LogPrint("ERROR", "currHeight (%d) < last_vote_height (%d)", currHeight, last_vote_height);
        return false;
    }

    last_vote_height = currHeight;
    uint64_t lastTotalVotes = GetVotedBCoins(candidateVotesInOut, currHeight);

    for (const auto &vote : candidateVotesIn) {
        const CUserID &voteId = vote.GetCandidateUid();
        vector<CCandidateVote>::iterator itVote =
            find_if(candidateVotesInOut.begin(), candidateVotesInOut.end(),
                    [&voteId, pAccountCache](const CCandidateVote &vote) {
                        if (voteId.type() != vote.GetCandidateUid().type()) {
                            CAccount account;
                            if (voteId.type() == typeid(CRegID)) {
                                pAccountCache->GetAccount(voteId, account);
                                return vote.GetCandidateUid() == account.owner_pubkey;
                            } else {  // vote.GetCandidateUid().type() == typeid(CRegID)
                                pAccountCache->GetAccount(vote.GetCandidateUid(), account);
                                return account.owner_pubkey == voteId;
                            }
                        } else {
                            return vote.GetCandidateUid() == voteId;
                        }
                    });

        int voteType = VoteType(vote.GetCandidateVoteType());
        if (ADD_BCOIN == voteType) {
            if (itVote != candidateVotesInOut.end()) { //existing vote
                uint64_t currVotes = itVote->GetVotedBcoins();

                if (!IsBcoinWithinRange(vote.GetVotedBcoins()))
                     return ERRORMSG("ProcessDelegateVotes() : oper fund value exceeds maximum ");

                itVote->SetVotedBcoins(currVotes + vote.GetVotedBcoins());

                if (!IsBcoinWithinRange(itVote->GetVotedBcoins()))
                     return ERRORMSG("ProcessDelegateVotes() : fund value exceeds maximum");

            } else { //new vote
               if (candidateVotesInOut.size() == IniCfg().GetMaxVoteCandidateNum()) {
                   return ERRORMSG("ProcessDelegateVotes() : MaxVoteCandidateNum reached. Must revoke old votes 1st.");
               }

               candidateVotesInOut.push_back(vote);
            }
        } else if (MINUS_BCOIN == voteType) {
            // if (currHeight - last_vote_height < 100) {
            //     return ERRORMSG("ProcessDelegateVotes() : last vote not cooled down yet: lastVoteHeigh=%d",
            //                     last_vote_height);
            // }
            if  (itVote != candidateVotesInOut.end()) { //existing vote
                uint64_t currVotes = itVote->GetVotedBcoins();

                if (!IsBcoinWithinRange(vote.GetVotedBcoins()))
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
    uint64_t totalBcoins = free_bcoins + lastTotalVotes;
    if (totalBcoins < newTotalVotes) {
        return  ERRORMSG("ProcessDelegateVotes() : delegate votes exceeds account bcoins");
    }
    free_bcoins = totalBcoins - newTotalVotes;

    uint64_t interestAmountToInflate = ComputeVoteStakingInterest(candidateVotesInOut, currHeight);
    if (!IsBcoinWithinRange(interestAmountToInflate))
        return false;

    switch (GetFeatureForkVersion(currHeight)) {
        case MAJOR_VER_R1:
            free_bcoins += interestAmountToInflate; // for backward compatibility
            break;

        case MAJOR_VER_R2:
            free_fcoins += interestAmountToInflate; // only fcoins will be inflated for voters
            break;

        default:
            return false;
    }

    LogPrint("INFLATE", "Account(%s) received vote staking interest amount (fcoins): %lld\n",
            regid.ToString(), interestAmountToInflate);

    return true;
}

bool CAccount::StakeVoteBcoins(VoteType type, const uint64_t votes) {
    switch (type) {
        case ADD_BCOIN: {
            received_votes += votes;
            if (!IsBcoinWithinRange(received_votes)) {
                return ERRORMSG("StakeVoteBcoins() : delegates total votes exceed maximum ");
            }

            break;
        }

        case MINUS_BCOIN: {
            if (received_votes < votes) {
                return ERRORMSG("StakeVoteBcoins() : delegates total votes less than revocation vote number");
            }
            received_votes -= votes;

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
    if (accountType == ACCOUNT_TYPE::REGID) {
        vector<unsigned char> vRegId(accountId, accountId + 6);
        CRegID regId(vRegId);
        obj.push_back(Pair("regid", regId.ToString()));
    } else if (accountType == ACCOUNT_TYPE::BASE58ADDR) {
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
