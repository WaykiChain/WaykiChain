// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "account.h"
#include "config/configuration.h"
#include "config/const.h"
#include "main.h"

bool CAccount::GetBalance(const TokenSymbol &tokenSymbol, const BalanceType balanceType, uint64_t &value) {
    auto iter = tokens.find(tokenSymbol);
    if (iter != tokens.end()) {
        auto accountToken = iter->second;
        switch (balanceType) {
            case FREE_VALUE:    value = accountToken.free_amount;   return true;
            case STAKED_VALUE:  value = accountToken.staked_amount; return true;
            case FROZEN_VALUE:  value = accountToken.frozen_amount; return true;
            default: return false;
        }
    }

    return false;
}

bool CAccount::OperateBalance(const TokenSymbol &tokenSymbol, const BalanceOpType opType, const uint64_t &value) {
    if (!kCoinTypeSet.count(tokenSymbol)) {
        assert(false && "Unsupport token symbol");
        return false;
    }
    CAccountToken &accountToken = tokens[tokenSymbol];
    switch (opType) {
        case ADD_FREE: {
            accountToken.free_amount += value;
            return true;
        }
        case SUB_FREE: {
            if (accountToken.free_amount < value)
                return ERRORMSG("CAccount::OperateBalance, free_amount insufficient");

            accountToken.free_amount -= value;
            return true;
        }
        case STAKE: {
            if (accountToken.free_amount < value)
                return ERRORMSG("CAccount::OperateBalance, free_amount insufficient");

            accountToken.free_amount -= value;
            accountToken.staked_amount += value;
            return true;
        }
        case UNSTAKE: {
            if (accountToken.staked_amount < value)
                return ERRORMSG("CAccount::OperateBalance, staked_amount insufficient");

            accountToken.free_amount += value;
            accountToken.staked_amount -= value;
            return true;
        }
        case FREEZE: {
            if (accountToken.free_amount < value)
                return ERRORMSG("CAccount::OperateBalance, free_amount insufficient");

            accountToken.free_amount -= value;
            accountToken.frozen_amount += value;
            return true;
        }
        case UNFREEZE: {
            if (accountToken.frozen_amount < value)
                return ERRORMSG("CAccount::OperateBalance, frozen_amount insufficient");

            accountToken.free_amount += value;
            accountToken.frozen_amount -= value;
            return true;
        }
        default: return false;
    }
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
    auto wicc_token = GetToken(SYMB::WICC);
    return (votedBcoins + wicc_token.free_amount);
}

bool CAccount::RegIDIsMature() const {
    return (!regid.IsEmpty()) &&
           ((regid.GetHeight() == 0) ||
            (chainActive.Height() - (int)regid.GetHeight() > kRegIdMaturePeriodByBlock));
}

CAccountToken CAccount::GetToken(const TokenSymbol &tokenSymbol) const {
    auto iter = tokens.find(tokenSymbol);
    if (iter != tokens.end())
        return iter->second;

    return CAccountToken();
}

bool CAccount::SetToken(const TokenSymbol &tokenSymbol, const CAccountToken &accountToken) {
    tokens[tokenSymbol] = accountToken;
    return true;
}

Object CAccount::ToJsonObj() const {
    vector<CCandidateVote> candidateVotes;
    pCdMan->pDelegateCache->GetCandidateVotes(regid, candidateVotes);

    Array candidateVoteArray;
    for (auto &vote : candidateVotes) {
        candidateVoteArray.push_back(vote.ToJson());
    }

    Object tokenMapObj;
    for (auto tokenPair : tokens) {
        Object tokenObj;
        const CAccountToken &token = tokenPair.second;        
        tokenObj.push_back(Pair("free_amount",           token.free_amount));
        tokenObj.push_back(Pair("staked_amount",           token.staked_amount));
        tokenObj.push_back(Pair("frozen_amount",           token.frozen_amount));

        tokenMapObj.push_back(Pair(tokenPair.first,        tokenObj));        
    }  

    Object obj;
    obj.push_back(Pair("address",           keyid.ToAddress()));
    obj.push_back(Pair("keyid",             keyid.ToString()));
    obj.push_back(Pair("nickid",            nickid.ToString()));
    obj.push_back(Pair("regid",             regid.ToString()));
    obj.push_back(Pair("regid_mature",      RegIDIsMature()));
    obj.push_back(Pair("owner_pubkey",      owner_pubkey.ToString()));
    obj.push_back(Pair("miner_pubkey",      miner_pubkey.ToString()));
    obj.push_back(Pair("tokens",            tokenMapObj));
    obj.push_back(Pair("received_votes",    received_votes));
    obj.push_back(Pair("vote_list",         candidateVoteArray));

    return obj;
}

string CAccount::ToString() const {
    string str;
    string  strTokens = "";
    for (auto pair : tokens) {
        CAccountToken &token = pair.second;
        strTokens += strprintf ("\n %s: {free=%llu, staked=%llu, frozen=%llu}\n",
                    pair.first, token.free_amount, token.staked_amount, token.frozen_amount);
    }
    str += strprintf(
        "regid=%s, keyid=%s, nickId=%s, owner_pubkey=%s, miner_pubkey=%s, "
        "tokens=%s, received_votes=%llu, last_vote_height=%llu\n",
        regid.ToString(), keyid.GetHex(), nickid.ToString(), owner_pubkey.ToString(), miner_pubkey.ToString(),
        strTokens, received_votes, last_vote_height);
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
    uint64_t totalBcoins = GetToken(SYMB::WICC).free_amount + lastTotalVotes;
    if (totalBcoins < newTotalVotes) {
        return  ERRORMSG("ProcessDelegateVotes() : delegate votes exceeds account bcoins");
    }
    uint64_t free_bcoins = totalBcoins - newTotalVotes;

    uint64_t currBcoinAmt = GetToken(SYMB::WICC).free_amount;
    if (currBcoinAmt < free_bcoins) {
        OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, free_bcoins - currBcoinAmt);
    } else {
        OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, currBcoinAmt - free_bcoins);
    }

    uint64_t interestAmountToInflate = ComputeVoteStakingInterest(candidateVotesInOut, currHeight);
    if (!IsBcoinWithinRange(interestAmountToInflate))
        return false;

    switch (GetFeatureForkVersion(currHeight)) {
        case MAJOR_VER_R1: // for backward compatibility
            OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, interestAmountToInflate);
            break;

        case MAJOR_VER_R2: // only fcoins will be inflated for voters
            OperateBalance(SYMB::WGRT, BalanceOpType::ADD_FREE, interestAmountToInflate);
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
