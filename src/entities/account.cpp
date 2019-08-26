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
        default: return false;
    }
}

uint64_t CAccount::ComputeVoteStakingInterest(const vector<CCandidateReceivedVote> &candidateVotes,
                                              const uint32_t currHeight,
                                              const FeatureForkVersionEnum &featureForkVersion) {
    if (candidateVotes.empty()) {
        LogPrint("DEBUG", "1st-time vote by the account, hence interest inflation.");
        return 0;  // 0 for the very 1st vote
    }

    uint64_t beginHeight = last_vote_height;
    uint64_t endHeight   = currHeight;
    uint8_t beginSubsidy = ::GetSubsidyRate(last_vote_height);
    uint8_t endSubsidy   = ::GetSubsidyRate(currHeight);
    uint64_t amount      = 0;
    switch (featureForkVersion) {
        case MAJOR_VER_R1:
            amount = candidateVotes.begin()->GetVotedBcoins();  // one bcoin eleven votes
            break;
        case MAJOR_VER_R2:
            for (const auto &item : candidateVotes) {
                amount += item.GetVotedBcoins();  // one bcoin one vote
            }
            break;
        default:
            LogPrint("ERROR", "CAccount::ComputeVoteStakingInterest, unexpected feature fork version: %d\n",
                     featureForkVersion);
            break;
    }
    LogPrint("DEBUG", "beginSubsidy: %u, endSubsidy: %u, beginHeight: %llu, endHeight: %llu\n", beginSubsidy,
             endSubsidy, beginHeight, endHeight);

    auto ComputeInterest = [](const uint64_t amount, const uint8_t subsidy, const uint64_t beginHeight,
                              const uint64_t endHeight, const uint32_t yearHeight) -> uint64_t {
        uint64_t holdHeight = endHeight - beginHeight;
        uint64_t interest   = (uint64_t)(amount * ((long double)holdHeight * subsidy / yearHeight / 100));
        LogPrint("DEBUG", "amount: %llu, subsidy: %u, beginHeight: %llu, endHeight: %llu, yearHeight: %u, interest: %llu\n",
                 amount, subsidy, beginHeight, endHeight, yearHeight, interest);
        return interest;
    };

    uint32_t yearHeight = ::GetYearBlockCount(currHeight);
    uint64_t interest   = 0;
    uint8_t subsidy     = beginSubsidy;
    while (subsidy != endSubsidy) {
        uint32_t jumpHeight = ::GetJumpHeightBySubsidy(currHeight, subsidy - 1);
        interest += ComputeInterest(amount, subsidy, beginHeight, jumpHeight, yearHeight);
        beginHeight = jumpHeight;
        subsidy -= 1;
    }

    interest += ComputeInterest(amount, subsidy, beginHeight, endHeight, yearHeight);
    LogPrint("DEBUG", "updateHeight: %llu, currHeight: %llu, freeze value: %llu\n", last_vote_height, currHeight,
             candidateVotes.begin()->GetVotedBcoins());

    return interest;
}

uint64_t CAccount::ComputeBlockInflateInterest(const uint32_t currHeight) const {
    if (GetFeatureForkVersion(currHeight) == MAJOR_VER_R1) return 0;

    uint8_t subsidy     = ::GetSubsidyRate(currHeight);
    uint64_t holdHeight = 1;
    uint32_t yearHeight = ::GetYearBlockCount(currHeight);
    uint64_t profits =
        (long double)(received_votes * IniCfg().GetTotalDelegateNum() * holdHeight * subsidy) / yearHeight / 100;
    LogPrint("profits",
             "account: %s, currHeight: %llu, received_votes: %llu, subsidy: %u, holdHeight: %llu, yearHeight: "
             "%u, llProfits: %llu\n",
             regid.ToString(), currHeight, received_votes, subsidy, holdHeight, yearHeight, profits);

    return profits;
}

uint64_t CAccount::GetVotedBcoins(const vector<CCandidateReceivedVote> &candidateVotes, const uint64_t currHeight) {
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

bool CAccount::ProcessDelegateVotes(const vector<CCandidateVote> &candidateVotesIn,
                                    vector<CCandidateReceivedVote> &candidateVotesInOut, const uint32_t currHeight,
                                    const CAccountDBCache &accountCache, vector<CReceipt> &receipts) {
    if (currHeight < last_vote_height) {
        LogPrint("ERROR", "currHeight (%d) < last_vote_height (%d)", currHeight, last_vote_height);
        return false;
    }

    uint64_t lastTotalVotes = GetVotedBcoins(candidateVotesInOut, currHeight);

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
                    return ERRORMSG("ProcessDelegateVotes() : oper fund value exceeds maximum ");

                itVote->SetVotedBcoins(currVotes + vote.GetVotedBcoins());

                if (!IsBcoinWithinRange(itVote->GetVotedBcoins()))
                    return ERRORMSG("ProcessDelegateVotes() : fund value exceeds maximum");

            } else {  // new vote
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
    std::sort(candidateVotesInOut.begin(), candidateVotesInOut.end(),
            [](CCandidateReceivedVote vote1, CCandidateReceivedVote vote2) {
        return vote1.GetVotedBcoins() > vote2.GetVotedBcoins();
    });

    uint64_t newTotalVotes = GetVotedBcoins(candidateVotesInOut, currHeight);
    uint64_t totalBcoins   = GetToken(SYMB::WICC).free_amount + lastTotalVotes;
    if (totalBcoins < newTotalVotes) {
        return  ERRORMSG("ProcessDelegateVotes() : delegate votes exceeds account bcoins");
    }

    uint64_t free_bcoins  = totalBcoins - newTotalVotes;
    uint64_t currBcoinAmt = GetToken(SYMB::WICC).free_amount;
    if (currBcoinAmt < free_bcoins) {
        OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, free_bcoins - currBcoinAmt);
        CReceipt receipt(nullId, regid, SYMB::WICC, free_bcoins - currBcoinAmt, "add free bcoins due to revoking votes");
        receipts.push_back(receipt);
    } else {
        OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, currBcoinAmt - free_bcoins);
        CReceipt receipt(regid, nullId, SYMB::WICC, currBcoinAmt - free_bcoins, "sub free bcoins due to increasing votes");
        receipts.push_back(receipt);
    }

    auto featureForkVersion          = GetFeatureForkVersion(currHeight);
    uint64_t interestAmountToInflate = ComputeVoteStakingInterest(candidateVotesInOut, currHeight, featureForkVersion);
    if (!IsBcoinWithinRange(interestAmountToInflate))
        return false;

    switch (featureForkVersion) {
        case MAJOR_VER_R1: {  // for backward compatibility
            OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, interestAmountToInflate);
            CReceipt receipt(nullId, regid, SYMB::WICC, interestAmountToInflate, "inflate interest due to voting");
            receipts.push_back(receipt);
            break;
        }
        case MAJOR_VER_R2: {  // only fcoins will be inflated for voters
            OperateBalance(SYMB::WGRT, BalanceOpType::ADD_FREE, interestAmountToInflate);
            CReceipt receipt(nullId, regid, SYMB::WGRT, interestAmountToInflate, "inflate interest due to voting");
            receipts.push_back(receipt);
            break;
        }
        default:
            return false;
    }

    LogPrint("INFLATE", "Account(%s) received vote staking interest amount (bcoin/fcoins): %lld\n",
            regid.ToString(), interestAmountToInflate);

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
            return ERRORMSG("StakeVoteBcoins() : CDelegateVoteTx ExecuteTx AccountVoteOper revocation votes are not exist");
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
