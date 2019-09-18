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

    CAccountToken &accountToken = tokens[tokenSymbol];
    switch (opType) {
        case ADD_FREE: {
            accountToken.free_amount += value;
            return true;
        }
        case SUB_FREE: {
            if (accountToken.free_amount < value)
                return ERRORMSG("CAccount::OperateBalance, free_amount insufficient(%llu vs %llu) of %s",
                                accountToken.free_amount, value, tokenSymbol);

            accountToken.free_amount -= value;
            return true;
        }
        case STAKE: {
            if (accountToken.free_amount < value)
                return ERRORMSG("CAccount::OperateBalance, free_amount insufficient(%llu vs %llu) of %s",
                                accountToken.free_amount, value, tokenSymbol);

            accountToken.free_amount -= value;
            accountToken.staked_amount += value;
            return true;
        }
        case UNSTAKE: {
            if (accountToken.staked_amount < value)
                return ERRORMSG("CAccount::OperateBalance, staked_amount insufficient(%llu vs %llu) of %s",
                                accountToken.staked_amount, value, tokenSymbol);

            accountToken.free_amount += value;
            accountToken.staked_amount -= value;
            return true;
        }
        case FREEZE: {
            if (accountToken.free_amount < value)
                return ERRORMSG("CAccount::OperateBalance, free_amount insufficient(%llu vs %llu) of %s",
                                accountToken.free_amount, value, tokenSymbol);

            accountToken.free_amount -= value;
            accountToken.frozen_amount += value;
            return true;
        }
        case UNFREEZE: {
            if (accountToken.frozen_amount < value)
                return ERRORMSG("CAccount::OperateBalance, frozen_amount insufficient(%llu vs %llu) of %s",
                                accountToken.frozen_amount, value, tokenSymbol);

            accountToken.free_amount += value;
            accountToken.frozen_amount -= value;
            return true;
        }
        case VOTE: {
            if (accountToken.free_amount < value)
                return ERRORMSG("CAccount::OperateBalance, free_amount insufficient(%llu vs %llu) of %s",
                                accountToken.free_amount, value, tokenSymbol);

            accountToken.free_amount -= value;
            accountToken.voted_amount += value;
            return true;
        }
        case UNVOTE: {
            if (accountToken.voted_amount < value)
                return ERRORMSG("CAccount::OperateBalance, voted_amount insufficient(%llu vs %llu) of %s",
                                accountToken.voted_amount, value, tokenSymbol);

            accountToken.free_amount += value;
            accountToken.voted_amount -= value;
            return true;
        }
        default: return false;
    }
}

uint64_t CAccount::ComputeVoteBcoinInterest(const uint64_t lastVotedBcoins, const uint32_t currHeight) {
    if (lastVotedBcoins == 0) {
        LogPrint("profits", "1st-time vote by the account, hence no interest inflation\n");
        return 0;  // 0 for the very 1st vote
    }

    uint64_t beginHeight = last_vote_height;
    uint64_t endHeight   = currHeight;
    uint8_t beginSubsidy = ::GetSubsidyRate(last_vote_height);
    uint8_t endSubsidy   = ::GetSubsidyRate(currHeight);

    auto ComputeInterest = [currHeight](const CRegID &regid, const uint64_t votedBcoins, const uint8_t subsidy,
                              const uint64_t beginHeight, const uint64_t endHeight,
                              const uint32_t yearHeight) -> uint64_t {
        uint64_t holdHeight = endHeight - beginHeight;
        uint64_t interest   = (long double)votedBcoins * holdHeight * subsidy / yearHeight / 100;

        LogPrint("profits",
                 "compute vote staking interest to voter: %s, current height: %u\n"
                 "interest = votedBcoins * (endHeight - beginHeight) * subsidy / yearHeight / 100\n"
                 "formula: %llu = 1.0 * %llu * (%llu - %llu) * %u / %u / 100\n",
                 regid.ToString(), currHeight, interest, votedBcoins, endHeight, beginHeight, subsidy, yearHeight);
        return interest;
    };

    uint32_t yearHeight = ::GetYearBlockCount(currHeight);
    uint64_t interest   = 0;
    uint8_t subsidy     = beginSubsidy;
    while (subsidy != endSubsidy) {
        uint32_t jumpHeight = ::GetJumpHeightBySubsidy(subsidy - 1);
        interest += ComputeInterest(regid, lastVotedBcoins, subsidy, beginHeight, jumpHeight, yearHeight);
        beginHeight = jumpHeight;
        subsidy -= 1;
    }

    interest += ComputeInterest(regid, lastVotedBcoins, subsidy, beginHeight, endHeight, yearHeight);

    return interest;
}

uint64_t CAccount::ComputeVoteFcoinInterest(const uint64_t lastVotedBcoins, const uint32_t currHeight) {
    int64_t epoch_last_vote = chainActive[last_vote_height-1]->GetBlockTime();
    int64_t epoch_curr_vote = chainActive[currHeight-1]->GetBlockTime();
    if (epoch_curr_vote <= FCOIN_VOTEMINE_EPOCH_FROM ||
        epoch_last_vote >= FCOIN_VOTEMINE_EPOCH_TO)
        return 0;

    if (epoch_last_vote < FCOIN_VOTEMINE_EPOCH_FROM)
        epoch_last_vote = FCOIN_VOTEMINE_EPOCH_FROM;

    if (epoch_curr_vote > FCOIN_VOTEMINE_EPOCH_TO)
        epoch_curr_vote = FCOIN_VOTEMINE_EPOCH_TO;

    uint64_t duration = epoch_curr_vote - epoch_last_vote;
    if (duration == 0) return 0;

    // interest = (1% * 21 billion) * (lastVotedBcoins/0.21 billion) * (duration / 365*24*3600)
    return (lastVotedBcoins * (duration / 31536000.0));
}

uint64_t CAccount::ComputeBlockInflateInterest(const uint32_t currHeight) const {
    if (GetFeatureForkVersion(currHeight) == MAJOR_VER_R1)
        return 0;

    uint8_t subsidy      = ::GetSubsidyRate(currHeight);
    uint64_t holdHeight  = 1;
    uint32_t yearHeight  = ::GetYearBlockCount(currHeight);
    uint32_t delegateNum = IniCfg().GetTotalDelegateNum();
    uint64_t interest    = (long double)received_votes * delegateNum * holdHeight * subsidy / yearHeight / 100;

    LogPrint("profits",
             "compute block inflate interest to miner: %s, current height: %u\n"
             "interest = received_votes * delegateNum * holdHeight * subsidy / yearHeight / 100\n"
             "formula: %llu = 1.0 * %llu * %u * %llu * %u / %u / 100\n",
             regid.ToString(), currHeight, interest, received_votes, delegateNum, holdHeight, subsidy, yearHeight);

    return interest;
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
    vector<CCandidateReceivedVote> candidateVotes;
    pCdMan->pDelegateCache->GetCandidateVotes(regid, candidateVotes);

    Array candidateVoteArray;
    for (auto &vote : candidateVotes) {
        candidateVoteArray.push_back(vote.ToJson());
    }

    Object tokenMapObj;
    for (auto tokenPair : tokens) {
        Object tokenObj;
        const CAccountToken &token = tokenPair.second;
        tokenObj.push_back(Pair("free_amount",      token.free_amount));
        tokenObj.push_back(Pair("staked_amount",    token.staked_amount));
        tokenObj.push_back(Pair("frozen_amount",    token.frozen_amount));
        tokenObj.push_back(Pair("voted_amount",     token.voted_amount));

        tokenMapObj.push_back(Pair(tokenPair.first, tokenObj));
    }

    Object obj;
    obj.push_back(Pair("address",           keyid.ToAddress()));
    obj.push_back(Pair("keyid",             keyid.ToString()));
    obj.push_back(Pair("nickid",            nickid.ToString()));
    obj.push_back(Pair("regid",             regid.ToString()));
    obj.push_back(Pair("regid_mature",      regid.IsMature(chainActive.Height())));
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

    vector<CCandidateReceivedVote> candidateVotes;
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

bool CAccount::ProcessCandidateVotes(const vector<CCandidateVote> &candidateVotesIn,
                                    vector<CCandidateReceivedVote> &candidateVotesInOut, const uint32_t currHeight,
                                    const CAccountDBCache &accountCache, vector<CReceipt> &receipts) {
    if (currHeight < last_vote_height) {
        LogPrint("ERROR", "currHeight (%d) < last_vote_height (%d)", currHeight, last_vote_height);
        return false;
    }

    auto featureForkVersion = GetFeatureForkVersion(currHeight);
    uint64_t lastTotalVotes = GetToken(SYMB::WICC).voted_amount;
    // assert(lastTotalVotes == GetVotedBcoins(candidateVotesInOut, currHeight));

    for (const auto &vote : candidateVotesIn) {
        const CUserID &voteId = vote.GetCandidateUid();
        vector<CCandidateReceivedVote>::iterator itVote =
            find_if(candidateVotesInOut.begin(), candidateVotesInOut.end(),
                    [&voteId, &accountCache](const CCandidateReceivedVote &vote) {
                        if (voteId.type() != vote.GetCandidateUid().type()) {
                            CAccount account;
                            if (voteId.type() == typeid(CRegID)) {
                                accountCache.GetAccount(voteId, account);
                                return vote.GetCandidateUid() == account.owner_pubkey;

                            } else {  // vote.GetCandidateUid().type() == typeid(CPubKey)
                                accountCache.GetAccount(vote.GetCandidateUid(), account);
                                return account.owner_pubkey == voteId;
                            }
                        } else {
                            return vote.GetCandidateUid() == voteId;
                        }
                    });

        int32_t voteType = VoteType(vote.GetCandidateVoteType());
        if (ADD_BCOIN == voteType) {
            if (itVote != candidateVotesInOut.end()) { //existing vote
                uint64_t currVotes = itVote->GetVotedBcoins();

                if (!IsBcoinWithinRange(vote.GetVotedBcoins()))
                    return ERRORMSG("ProcessCandidateVotes() : oper fund value exceeds maximum ");

                itVote->SetVotedBcoins(currVotes + vote.GetVotedBcoins());

                if (!IsBcoinWithinRange(itVote->GetVotedBcoins()))
                    return ERRORMSG("ProcessCandidateVotes() : fund value exceeds maximum");

            } else {  // new vote
                if (candidateVotesInOut.size() == IniCfg().GetMaxVoteCandidateNum()) {
                    return ERRORMSG("ProcessCandidateVotes() : MaxVoteCandidateNum reached. Must revoke old votes 1st.");
                }

                candidateVotesInOut.push_back(vote);
            }
        } else if (MINUS_BCOIN == voteType) {
            // if (currHeight - last_vote_height < 100) {
            //     return ERRORMSG("ProcessCandidateVotes() : last vote not cooled down yet: lastVoteHeigh=%d",
            //                     last_vote_height);
            // }
            if  (itVote != candidateVotesInOut.end()) { //existing vote
                uint64_t currVotes = itVote->GetVotedBcoins();

                if (!IsBcoinWithinRange(vote.GetVotedBcoins()))
                    return ERRORMSG("ProcessCandidateVotes() : oper fund value exceeds maximum ");

                if (itVote->GetVotedBcoins() < vote.GetVotedBcoins())
                    return ERRORMSG("ProcessCandidateVotes() : oper fund value exceeds delegate fund value");

                itVote->SetVotedBcoins(currVotes - vote.GetVotedBcoins());

                if (0 == itVote->GetVotedBcoins())
                    candidateVotesInOut.erase(itVote);

            } else {
                return ERRORMSG("ProcessCandidateVotes() : revocation votes not exist");
            }
        } else {
            return ERRORMSG("ProcessCandidateVotes() : operType: %d invalid", voteType);
        }
    }

    // sort account votes after the operations against the new votes
    std::sort(candidateVotesInOut.begin(), candidateVotesInOut.end(),
            [](CCandidateReceivedVote vote1, CCandidateReceivedVote vote2) {
        return vote1.GetVotedBcoins() > vote2.GetVotedBcoins();
    });

    uint64_t newTotalVotes = 0;
    if (!candidateVotesInOut.empty()) {
        if (featureForkVersion == MAJOR_VER_R1) {
            newTotalVotes = candidateVotesInOut[0].GetVotedBcoins(); // one bcoin eleven votes

        } else if (featureForkVersion == MAJOR_VER_R2) {
            for (const auto &vote : candidateVotesInOut) {
                newTotalVotes += vote.GetVotedBcoins();         // one bcoin one vote
            }
        }
    }

    if (newTotalVotes > lastTotalVotes) {
        uint64_t addedVotes = newTotalVotes - lastTotalVotes;
        if (!OperateBalance(SYMB::WICC, BalanceOpType::VOTE, addedVotes)) {
            return ERRORMSG(
                "ProcessCandidateVotes() : delegate votes exceeds account bcoins when voting! "
                "newTotalVotes=%llu, lastTotalVotes=%llu, freeAmount=%llu",
                newTotalVotes, lastTotalVotes, GetToken(SYMB::WICC).free_amount);
        }
        CReceipt receipt(regid, nullId, SYMB::WICC, addedVotes, "sub free bcoins for increasing votes");
        receipts.push_back(receipt);
    } else if (newTotalVotes < lastTotalVotes) {
        uint64_t subVotes = lastTotalVotes - newTotalVotes;
        if (!OperateBalance(SYMB::WICC, BalanceOpType::UNVOTE, subVotes)) {
            return ERRORMSG(
                "ProcessCandidateVotes() : delegate votes insufficient to unvote! "
                "newTotalVotes=%llu, lastTotalVotes=%llu, freeAmount=%llu",
                newTotalVotes, lastTotalVotes, GetToken(SYMB::WICC).free_amount);
        }
        CReceipt receipt(nullId, regid, SYMB::WICC, subVotes, "add free bcoins due to revoking votes");
        receipts.push_back(receipt);
    } // else newTotalVotes == lastTotalVotes // do nothing

    // collect inflated bcoins or fcoins
    switch (featureForkVersion) {
        case MAJOR_VER_R1: {  // for backward compatibility
            uint64_t bcoinAmountToInflate = ComputeVoteBcoinInterest(lastTotalVotes, currHeight);
            if (!IsBcoinWithinRange(bcoinAmountToInflate))
                return false;

            OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, bcoinAmountToInflate);
            CReceipt receipt(nullId, regid, SYMB::WICC, bcoinAmountToInflate, "inflate bcoin due to voting interest");
            receipts.push_back(receipt);

            LogPrint("profits", "Account(%s) received vote staking interest amount (bcoins): %lld\n",
                    regid.ToString(), bcoinAmountToInflate);

            break;
        }
        case MAJOR_VER_R2: {  // only fcoins will be inflated for voters
            uint64_t fcoinAmountToInflate = ComputeVoteFcoinInterest(lastTotalVotes, currHeight);

            if (fcoinAmountToInflate > 0) {
                OperateBalance(SYMB::WGRT, BalanceOpType::ADD_FREE, fcoinAmountToInflate);
                CReceipt receipt(nullId, regid, SYMB::WGRT, fcoinAmountToInflate, "inflate fcoin due to voting interest");
                receipts.push_back(receipt);

                LogPrint("profits", "Account(%s) received vote staking interest amount (fcoins): %lld\n",
                    regid.ToString(), fcoinAmountToInflate);
            }
            break;
        }
        default:
            return false;
    }

    // Attention: update last vote height after computing vote staking interest.
    last_vote_height = currHeight;

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
            return ERRORMSG("StakeVoteBcoins() : CDelegateVoteTx ExecuteTx AccountVoteOper revocation votes do not exist");
    }

    return true;
}

bool CAccount::IsMyUid(const CUserID &uid) {
    if (uid.type() == typeid(CKeyID)) {
        return keyid == uid.get<CKeyID>();
    } else if (uid.type() == typeid(CRegID)) {
        return !regid.IsEmpty() && regid == uid.get<CRegID>();
    } else if (uid.type() == typeid(CPubKey)) {
        return owner_pubkey.IsValid() && owner_pubkey == uid.get<CPubKey>();
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
// class CVmOperate

Object CVmOperate::ToJson() {
    Object obj;
    if (accountType == AccountType::REGID) {
        vector<uint8_t> vRegId(accountId, accountId + 6);
        CRegID regId(vRegId);
        obj.push_back(Pair("regid", regId.ToString()));
    } else if (accountType == AccountType::BASE58ADDR) {
        string addr(accountId, accountId + sizeof(accountId));
        obj.push_back(Pair("addr", addr));
    }
    obj.push_back(Pair("opertype", GetBalanceOpTypeName((BalanceOpType)opType)));
    if (timeoutHeight > 0)
        obj.push_back(Pair("outHeight", (int32_t)timeoutHeight));

    uint64_t amount;
    memcpy(&amount, money, sizeof(money));
    obj.push_back(Pair("amount", amount));
    return obj;
}
