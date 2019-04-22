// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2019- The WaykiChain Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "accounts.h"
#include "database.h"
#include "main.h"

/** account db cache*/
extern CAccountViewCache *pAccountViewTip;

bool CID::Set(const CRegID &id) {
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << id;
    vchData.clear();
    vchData.insert(vchData.end(), ds.begin(), ds.end());
    return true;
}

bool CID::Set(const CKeyID &id) {
    vchData.resize(20);
    memcpy(&vchData[0], &id, 20);
    return true;
}

bool CID::Set(const CPubKey &id) {
    vchData.resize(id.size());
    memcpy(&vchData[0], &id, id.size());
    return true;
}

bool CID::Set(const CNullID &id) {
    return true;
}

bool CID::Set(const CUserID &userid) {
    return boost::apply_visitor(CIDVisitor(this), userid);
}

CUserID CID::GetUserId() const {
    unsigned long len = vchData.size();
    if (1 < len && len <= 10) {
        CRegID regId;
        regId.SetRegIDByCompact(vchData);
        return CUserID(regId);
    } else if (len == 33) {
        CPubKey pubKey(vchData);
        return CUserID(pubKey);
    } else if (len == 20) {
        uint160 data = uint160(vchData);
        CKeyID keyId(data);
        return CUserID(keyId);
    } else if(vchData.empty()) {
        return CNullID();
    } else {
        LogPrint("ERROR", "vchData:%s, len:%d\n", HexStr(vchData).c_str(), len);
        throw ios_base::failure("GetUserId error from CID");
    }
    return CNullID();
}

bool CRegID::Clean()
{
    nHeight = 0 ;
    nIndex = 0 ;
    vRegID.clear();
    return true;
}

CRegID::CRegID(const vector<unsigned char>& vIn) {
    assert(vIn.size() == 6);
    vRegID = vIn;
    nHeight = 0;
    nIndex = 0;
    CDataStream ds(vIn, SER_DISK, CLIENT_VERSION);
    ds >> nHeight;
    ds >> nIndex;
}

bool CRegID::IsSimpleRegIdStr(const string & str)
{
    int len = str.length();
    if (len >= 3) {
        int pos = str.find('-');

        if (pos > len - 1) {
            return false;
        }
        string firtstr = str.substr(0, pos);

        if (firtstr.length() > 10 || firtstr.length() == 0) //int max is 4294967295 can not over 10
            return false;

        for (auto te : firtstr) {
            if (!isdigit(te))
                return false;
        }
        string endstr = str.substr(pos + 1);
        if (endstr.length() > 10 || endstr.length() == 0) //int max is 4294967295 can not over 10
            return false;
        for (auto te : endstr) {
            if (!isdigit(te))
                return false;
        }
        return true;
    }
    return false;
}

bool CRegID::GetKeyId(const string & str,CKeyID &keyId)
{
    CRegID regId(str);
    if (regId.IsEmpty())
        return false;

    keyId = regId.GetKeyId(*pAccountViewTip);
    return !keyId.IsEmpty();
}

bool CRegID::IsRegIdStr(const string & str)
{
    bool ret = IsSimpleRegIdStr(str) || (str.length() == 12);
    return ret;
}

void CRegID::SetRegID(string strRegID)
{
    nHeight = 0;
    nIndex = 0;
    vRegID.clear();

    if (IsSimpleRegIdStr(strRegID)) {
        int pos = strRegID.find('-');
        nHeight = atoi(strRegID.substr(0, pos).c_str());
        nIndex = atoi(strRegID.substr(pos+1).c_str());
        vRegID.insert(vRegID.end(), BEGIN(nHeight), END(nHeight));
        vRegID.insert(vRegID.end(), BEGIN(nIndex), END(nIndex));
//      memcpy(&vRegID.at(0),&nHeight,sizeof(nHeight));
//      memcpy(&vRegID[sizeof(nHeight)],&nIndex,sizeof(nIndex));
    } else if (strRegID.length() == 12) {
        vRegID = ::ParseHex(strRegID);
        memcpy(&nHeight,&vRegID[0],sizeof(nHeight));
        memcpy(&nIndex,&vRegID[sizeof(nHeight)],sizeof(nIndex));
    }
}

void CRegID::SetRegID(const vector<unsigned char>& vIn)
{
    assert(vIn.size() == 6);
    vRegID = vIn;
    CDataStream ds(vIn, SER_DISK, CLIENT_VERSION);
    ds >> nHeight;
    ds >> nIndex;
}

CRegID::CRegID(string strRegID)
{
    SetRegID(strRegID);
}

CRegID::CRegID(uint32_t nHeightIn, uint16_t nIndexIn)
{
    nHeight = nHeightIn;
    nIndex = nIndexIn;
    vRegID.clear();
    vRegID.insert(vRegID.end(), BEGIN(nHeightIn), END(nHeightIn));
    vRegID.insert(vRegID.end(), BEGIN(nIndexIn), END(nIndexIn));
}

string CRegID::ToString() const
{
    if(!IsEmpty())
      return  strprintf("%d-%d", nHeight, nIndex);
    return string(" ");
}

CKeyID CRegID::GetKeyId(const CAccountViewCache &view)const
{
    CKeyID ret;
    CAccountViewCache(view).GetKeyId(*this, ret);
    return ret;
}

void CRegID::SetRegIDByCompact(const vector<unsigned char> &vIn)
{
    if (vIn.size() > 0) {
        CDataStream ds(vIn, SER_DISK, CLIENT_VERSION);
        ds >> *this;
    } else {
        Clean();
    }
}

string COperVoteFund::ToString() const {
    string str = strprintf("operVoteType=%s %s", voteOperTypeArray[operType], fund.ToString());
    return str;
}

Object COperVoteFund::ToJson() const {
    Object obj;
    string sOperType;
    if (operType >= 3) {
        sOperType = "INVALID_OPER_TYPE";
        LogPrint("ERROR", "Delegate Vote Tx contains invalid operType: %d", operType);
    } else {
        sOperType = voteOperTypeArray[operType];
    }
    obj.push_back(Pair("operType", sOperType));
    obj.push_back(Pair("voteFund", fund.ToJson()));
    return obj;
}

string CAccountLog::ToString() const {
    string str("");
    str += strprintf("    Account log: keyId=%d bcoinBalance=%lld nVoteHeight=%lld receivedVotes=%lld \n",
        keyID.GetHex(), bcoinBalance, nVoteHeight, receivedVotes);
    str += string("    vote fund:");

    for (auto it =  vVoteFunds.begin(); it != vVoteFunds.end(); ++it) {
        str += it->ToString();
    }
    return str;
}

bool CAccount::UndoOperateAccount(const CAccountLog & accountLog) {
    LogPrint("undo_account", "after operate:%s\n", ToString());
    bcoinBalance    = accountLog.bcoinBalance;
    nVoteHeight = accountLog.nVoteHeight;
    vVoteFunds  = accountLog.vVoteFunds;
    receivedVotes     = accountLog.receivedVotes;
    LogPrint("undo_account", "before operate:%s\n", ToString().c_str());
    return true;
}

uint64_t CAccount::GetAccountProfit(uint64_t nCurHeight) {
     if (vVoteFunds.empty()) {
        LogPrint("DEBUG", "1st-time vote for the account, hence no minting of interest.");
        nVoteHeight = nCurHeight; //record the 1st-time vote block height into account
        return 0; // 0 profit for 1st-time vote
    }

    // 先判断计算分红的上下限区块高度是否落在同一个分红率区间
    uint64_t nBeginHeight = nVoteHeight;
    uint64_t nEndHeight = nCurHeight;
    uint64_t nBeginSubsidy = IniCfg().GetBlockSubsidyCfg(nVoteHeight);
    uint64_t nEndSubsidy = IniCfg().GetBlockSubsidyCfg(nCurHeight);
    uint64_t nValue = vVoteFunds.begin()->GetVoteCount();
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
        nVoteHeight, nCurHeight, vVoteFunds.begin()->GetVoteCount());

    nVoteHeight = nCurHeight;
    return llProfits;
}

uint64_t CAccount::GetRawBalance() {
    return bcoinBalance;
}

uint64_t CAccount::GetTotalBalance() {
    if (!vVoteFunds.empty())
        return vVoteFunds.begin()->GetVoteCount() + bcoinBalance;

    return bcoinBalance;
}

uint64_t CAccount::GetFrozenBalance() {
    uint64_t votes = 0;
    for (auto it = vVoteFunds.begin(); it != vVoteFunds.end(); it++) {
      if (it->GetVoteCount() > votes) {
          votes = it->GetVoteCount();
      }
    }
    return votes;
}

Object CAccount::ToJsonObj(bool isAddress) const {
    Array voteFundArray;
    for (auto &fund : vVoteFunds) {
        voteFundArray.push_back(fund.ToJson());
    }

    Object obj;
    bool isMature = regID.GetHeight() > 0 && chainActive.Height() - (int)regID.GetHeight() >
                                                 kRegIdMaturePeriodByBlock
                        ? true
                        : false;
    obj.push_back(Pair("address",       keyID.ToAddress()));
    obj.push_back(Pair("keyID",         keyID.ToString()));
    obj.push_back(Pair("publicKey",     pubKey.ToString()));
    obj.push_back(Pair("minerPubKey",   minerPubKey.ToString()));
    obj.push_back(Pair("regID",         regID.ToString()));
    obj.push_back(Pair("regIDMature",   isMature));
    obj.push_back(Pair("balance",       bcoinBalance));
    obj.push_back(Pair("updateHeight",  nVoteHeight));
    obj.push_back(Pair("votes",         receivedVotes));
    obj.push_back(Pair("voteFundList",  voteFundArray));
    return obj;
}

string CAccount::ToString(bool isAddress) const {
    string str;
    str += strprintf("regID=%s, keyID=%s, publicKey=%s, minerpubkey=%s, values=%ld updateHeight=%d receivedVotes=%lld\n",
        regID.ToString(), keyID.GetHex().c_str(), pubKey.ToString().c_str(),
        minerPubKey.ToString().c_str(), bcoinBalance, nVoteHeight, receivedVotes);
    str += "vVoteFunds list: \n";
    for (auto & fund : vVoteFunds) {
        str += fund.ToString();
    }
    return str;
}

bool CAccount::IsMoneyOverflow(uint64_t nAddMoney) {
    if (!CheckMoneyRange(nAddMoney))
        return ERRORMSG("money:%lld too larger than MaxMoney");

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
        bcoinBalance += value;
        if (!IsMoneyOverflow(bcoinBalance))
            return false;
        break;
    }
    case MINUS_FREE: {
        if (value > bcoinBalance)
            return false;
        bcoinBalance -= value;
        break;
    }
    default:
        return ERRORMSG("operate account type error!");
    }
    LogPrint("op_account", "after operate:%s\n", ToString());
    return true;
}

bool CAccount::ProcessDelegateVote(vector<COperVoteFund> & operVoteFunds, const uint64_t nCurHeight) {
    if (nCurHeight < nVoteHeight) {
        LogPrint("ERROR", "current vote tx height (%d) can't be smaller than the last nVoteHeight (%d)",
            nCurHeight, nVoteHeight);
        return false;
    }

    uint64_t llProfit = GetAccountProfit(nCurHeight);
    if (!IsMoneyOverflow(llProfit)) return false;

    uint64_t totalVotes = vVoteFunds.empty() ? 0 : vVoteFunds.begin()->GetVoteCount();

    for (auto operVote = operVoteFunds.begin(); operVote != operVoteFunds.end(); ++operVote) {
        CID voteId = operVote->fund.GetVoteId();
        vector<CVoteFund>::iterator itfund =
            find_if(vVoteFunds.begin(), vVoteFunds.end(),
                    [voteId](CVoteFund fund) { return fund.GetVoteId() == voteId; });

        int voteType = VoteOperType(operVote->operType);
        if (ADD_FUND == voteType) {
            if (itfund != vVoteFunds.end()) {
                uint64_t currVotes = itfund->GetVoteCount();

                if (!IsMoneyOverflow(operVote->fund.GetVoteCount()))
                     return ERRORMSG("ProcessDelegateVote() : oper fund value exceed maximum ");

                itfund->SetVoteCount( currVotes + operVote->fund.GetVoteCount() );

                if (!IsMoneyOverflow(itfund->GetVoteCount()))
                     return ERRORMSG("ProcessDelegateVote() : fund value exceeds maximum");

            } else {
               vVoteFunds.push_back(operVote->fund);
               if (vVoteFunds.size() > IniCfg().GetDelegatesNum()) {
                   return ERRORMSG("ProcessDelegateVote() : fund number exceeds maximum");
               }
            }
        } else if (MINUS_FUND == voteType) {
            if  (itfund != vVoteFunds.end()) {
                uint64_t currVotes = itfund->GetVoteCount();

                if (!IsMoneyOverflow(operVote->fund.GetVoteCount()))
                    return ERRORMSG("ProcessDelegateVote() : oper fund value exceed maximum ");

                if (itfund->GetVoteCount() < operVote->fund.GetVoteCount())
                    return ERRORMSG("ProcessDelegateVote() : oper fund value exceed delegate fund value");

                itfund->SetVoteCount( currVotes - operVote->fund.GetVoteCount() );

                if (0 == itfund->GetVoteCount())
                    vVoteFunds.erase(itfund);

            } else {
                return ERRORMSG("ProcessDelegateVote() : revocation votes not exist");
            }
        } else {
            return ERRORMSG("ProcessDelegateVote() : operType: %d invalid", voteType);
        }
    }

    // sort account votes after the operations against the new votes
    std::sort(vVoteFunds.begin(), vVoteFunds.end(), [](CVoteFund fund1, CVoteFund fund2) {
        return fund1.GetVoteCount() > fund2.GetVoteCount();
    });

    // get the maximum one as the vote amount
    uint64_t newTotalVotes = vVoteFunds.empty() ? 0 : vVoteFunds.begin()->GetVoteCount();

    if (bcoinBalance + totalVotes < newTotalVotes) {
        return  ERRORMSG("ProcessDelegateVote() : delegate value exceed account value");
    }
    bcoinBalance = (bcoinBalance + totalVotes) - newTotalVotes;
    bcoinBalance += llProfit;
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
        return ERRORMSG("OperateVote() : CDelegateTx ExecuteTx AccountVoteOper revocation votes are not exist");
    }
    return true;
}
