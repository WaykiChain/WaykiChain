/*****************************************************************************
*                                                                            *
*  @file   /wikichain/src/test/Delegatetx_tests.cpp                          *
*  @brief                                                                    *
*  Details                                                                   *
*                                                                            *
*  @email                                                                    *
*  @version                                                                  *
*  @time    下午2:34:24                                                       *
*  @license                                                                  *
*                                                                            *
*----------------------------------------------------------------------------*
*  Remark         : Description                                              *
*----------------------------------------------------------------------------*
*  Change History :                                                          *
*  <Date>     | <Version> | <Author>       | <Description>                   *
*----------------------------------------------------------------------------*
*  2017年11月6日    |           |                 | Create file                  *
*----------------------------------------------------------------------------*
*                                                                            *
*****************************************************************************/

#include <boost/test/unit_test.hpp>
#include "../test/systestbase.h"


Value GetAccountItem(const Value &account, const string & itemName) {
    return find_value(account.get_obj(), itemName);
}

bool GetAccountDelegateVote(const Value &account, const string &delegate_addr, uint64_t &delegate_vote_value) {
     Array oper_vote_list = find_value(account.get_obj(), "voteFundList").get_array();
     for(auto item : oper_vote_list) {
         Value test_delegate_addr = find_value(item.get_obj(), "address");
         if(test_delegate_addr.type() != null_type && delegate_addr == test_delegate_addr.get_str()) {
             Value test_delegate_value = find_value(item.get_obj(), "votes");
             if(test_delegate_value.type() != null_type) {
                 delegate_vote_value = test_delegate_value.get_uint64();
             }
         }
     }
    return true;
}

bool GetMaxDelegateVote(const Value &account, string &max_delegate_addr, uint64_t &max_delegate_vote_value) {
    max_delegate_vote_value = 0;
    Array oper_vote_list = find_value(account.get_obj(), "voteFundList").get_array();
      for(auto item : oper_vote_list) {
          Value test_delegate_addr = find_value(item.get_obj(), "address");
          if(test_delegate_addr.type() != null_type) {
              Value test_delegate_votes = find_value(item.get_obj(), "votes");
              if(test_delegate_votes.type() != null_type) {
                  if(test_delegate_votes.get_uint64() > max_delegate_vote_value) {
                      max_delegate_vote_value = test_delegate_votes.get_uint64();
                      max_delegate_addr = test_delegate_addr.get_str();
                      cout << "max_delegate_vote_value:" << max_delegate_vote_value
                           << "     max_delegate_addr:" << max_delegate_addr
                           << "\n$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$"
                           << endl;
                  }
              }
          }
    }
    return true;
}

bool GetAccountReceiveVote(const Value & account, uint64_t &receive_vote_value) {
    Value oper_vote_list = find_value(account.get_obj(), "Votes").get_uint64();
    if(oper_vote_list.type() != null_type) {
        receive_vote_value =  oper_vote_list.get_uint64();
        return true;
    }
    return false;
}

bool GetAccountBalance(const Value & account, uint64_t &balance) {
    Value balance_value = find_value(account.get_obj(), "Balance");
    if(balance_value.type() != null_type) {
        balance = balance_value.get_uint64();
        return true;
    }
    return false;
}

//当value小于零的时候，代表撤回对某个候选人的投票
bool OperAccountDelegateVote(const string &send_addr, const string &receive_vote_addr, const int64_t &value) {
    SysTestBase basetest;
    string sendTxAddr = send_addr;
    Array operVoteFund;
    Object voteFund;
    voteFund.clear();

    string delegate_addr = receive_vote_addr;
    uint64_t receive_delegate_vote = 0;
    uint64_t send_delegate_vote = 0;
    uint64_t send_addr_balance = 0;
    uint64_t max_send_delegate_vote = 0;
    string max_send_delegate_addr = "";
    uint64_t send_tx_fee = 10000;
    uint64_t send_profits = 0;
    int current_block_height = 0;
    Value send_account_info = basetest.GetAccountInfo(sendTxAddr);
    Value item_value = GetAccountItem(send_account_info, sendTxAddr);
    if(item_value.type() != null_type) {
        send_addr_balance = item_value.get_int64();
    }

    //获取投票前，候选人列表中最大投票数
    BOOST_CHECK(GetMaxDelegateVote(send_account_info, max_send_delegate_addr, max_send_delegate_vote));
    //投票前，投票人的投票列表中对应候选人已投票数量
    BOOST_CHECK(GetAccountDelegateVote(send_account_info, delegate_addr, send_delegate_vote));
    //投票前，获取投票人可用余额
    BOOST_CHECK(GetAccountBalance(send_account_info, send_addr_balance));
    Value receive_account_info = basetest.GetAccountInfo(delegate_addr);
    //投票前，候选人获得票数
    BOOST_CHECK(GetAccountReceiveVote(receive_account_info, receive_delegate_vote));

    voteFund.clear();
    voteFund.push_back(Pair("delegate",delegate_addr));
    voteFund.push_back(Pair("votes", value));
    operVoteFund.push_back(voteFund);
    Value retValue = basetest.CreateDelegateTx(sendTxAddr, write_string((Value)operVoteFund, false).c_str(), send_tx_fee);
    string strHash;
    BOOST_CHECK(basetest.GetHashFromCreatedTx(retValue, strHash));

    //获取投票前,当前区块高度
    BOOST_CHECK(basetest.GetBlockHeight(current_block_height));
    ComputeVoteStakingInterests(send_account_info, current_block_height+1, send_profits);

    int mempool_tx_size = 0;
    while (1) {
        if (!basetest.GetMemPoolSize(mempool_tx_size)) {
            return false;
        }
        if (mempool_tx_size > 0) {
           MilliSleep(1000);
        } else {
           break;
        }
    }
    uint64_t receive_delegate_vote_new = 0;
    uint64_t send_delegate_vote_new = 0;
    uint64_t send_addr_balance_new = 0;
    uint64_t max_send_delegate_vote_new = 0;
    string max_send_delegate_addr_new = "";

    send_account_info = basetest.GetAccountInfo(sendTxAddr);
    //获取投票后，候选人列表中最大投票数
     BOOST_CHECK(GetMaxDelegateVote(send_account_info, max_send_delegate_addr_new, max_send_delegate_vote_new));
    //投票后，投票人的投票列表中候选人的最新投票数
    BOOST_CHECK(GetAccountDelegateVote(send_account_info, delegate_addr, send_delegate_vote_new));
    BOOST_CHECK(GetAccountBalance(send_account_info, send_addr_balance_new));
    receive_account_info = basetest.GetAccountInfo(delegate_addr);
    BOOST_CHECK(GetAccountReceiveVote(receive_account_info, receive_delegate_vote_new));
    //投票后，投票人投票列表投票项减去投票前投票项的值为投票数
    BOOST_CHECK((int64_t)send_delegate_vote_new - (int64_t)send_delegate_vote == value);
//    cout <<  "send_delegate_vote_new:" << send_delegate_vote_new
//           << "       send_delegate_vote:" << send_delegate_vote
//           << "       value:" << value
//           <<"\n#################################################" << endl;
    //投票后，候选人获得投票数投票前后差等于投票数
    BOOST_CHECK((int64_t)receive_delegate_vote_new - (int64_t)receive_delegate_vote == value);
    //投票后，检查投票人最大投票数的变化和可用余额是否正确
    int64_t max_vote_differ = max_send_delegate_vote - max_send_delegate_vote_new;
    //投票后，新的账户余额+利息-冻结变化余额==投票前账户余额
    cout <<  "send_addr_balance_new:" << send_addr_balance_new
        << "       send_profits:" << send_profits
        << "\nmax_vote_differ:" << max_vote_differ
        << "      send_addr_balance:" << send_addr_balance
        << "\nfee:" << send_tx_fee << endl;
    BOOST_CHECK((int64_t)send_addr_balance_new + (int64_t)send_tx_fee - (int64_t)send_profits  - (int64_t)max_vote_differ == (int64_t)send_addr_balance);

    return true;
}


BOOST_AUTO_TEST_SUITE(Delegatetx_tests)
//测试一创建12个投票交易
BOOST_AUTO_TEST_CASE(excced_fund) {
    SysTestBase basetest;
    string sendTxAddr = "wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4";
    Array operVoteFund;
    Object voteFund;
    voteFund.push_back(Pair("delegate", "wVTUdfEaeAAVSuXKrmMyqQXH5j5Z9oGmTt"));
    voteFund.push_back(Pair("votes", 10*COIN));
    operVoteFund.push_back(voteFund);
    voteFund.clear();
    voteFund.push_back(Pair("delegate", "wUt89R4bjD3Ca6Vb7mk18oGsVtSTCxJu2q"));
    voteFund.push_back(Pair("votes", 10*COIN));
    operVoteFund.push_back(voteFund);
    voteFund.clear();
    voteFund.push_back(Pair("delegate", "wT75mYY9C8xgqVgXquBmEfRmAXPDpJHU62"));
    voteFund.push_back(Pair("votes", 10*COIN));
    operVoteFund.push_back(voteFund);
    voteFund.clear();
    voteFund.push_back(Pair("delegate","wSms4pZnNe7bxjouLxUXQLowc7JqtNps94"));
    voteFund.push_back(Pair("votes", 10*COIN));
    operVoteFund.push_back(voteFund);
    voteFund.clear();
    voteFund.push_back(Pair("delegate", "wSjMDgKWHC2MzrUamhJtyyR2FTtw8oMUfx"));
    voteFund.push_back(Pair("votes", 10*COIN));
    operVoteFund.push_back(voteFund);
    voteFund.clear();
    voteFund.push_back(Pair("delegate", "wRQwgYkPNe1oX9Ts3cfuQ4KerqiV2e8gqM"));
    voteFund.push_back(Pair("votes", 10*COIN));
    operVoteFund.push_back(voteFund);
    voteFund.clear();
    voteFund.push_back(Pair("delegate","wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t"));
    voteFund.push_back(Pair("votes", 10*COIN));
    operVoteFund.push_back(voteFund);
    voteFund.clear();
    voteFund.push_back(Pair("delegate","wQewSbKL5kAfpwnrivSiCcaiFffgNva4uB"));
    voteFund.push_back(Pair("votes", 10*COIN));
    operVoteFund.push_back(voteFund);
    voteFund.clear();
    voteFund.push_back(Pair("delegate", "wP64X59EoRmeq2M5GrJ23UVttE9uxnuoFa"));
    voteFund.push_back(Pair("votes", 10*COIN));
    operVoteFund.push_back(voteFund);
    voteFund.clear();
    voteFund.push_back(Pair("delegate", "wNuJM44FPC5NxearNLP98pg295VqP7hsqu"));
    voteFund.push_back(Pair("votes", 10*COIN));
    operVoteFund.push_back(voteFund);
    voteFund.clear();
    voteFund.push_back(Pair("delegate", "wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6"));
    voteFund.push_back(Pair("votes", 10*COIN));
    operVoteFund.push_back(voteFund);
    voteFund.clear();
    voteFund.push_back(Pair("delegate", "wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4"));
    voteFund.push_back(Pair("votes", 10*COIN));
    operVoteFund.push_back(voteFund);
    string operliststr = write_string((Value)operVoteFund, false);
    Value retValue = basetest.CreateDelegateTx(sendTxAddr, operliststr, 0);
    string strHash;
    BOOST_CHECK(!basetest.GetHashFromCreatedTx(retValue, strHash));
}

//创建重复的投票选项
BOOST_AUTO_TEST_CASE(duplication_fund) {
    SysTestBase basetest;
    string sendTxAddr = "wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4";
    vector<CCandidateVote>  votes;
    Array operVoteFund;
    Object voteFund;
    votes.clear();
    voteFund.clear();
    voteFund.push_back(Pair("delegate","wVTUdfEaeAAVSuXKrmMyqQXH5j5Z9oGmTt"));
    voteFund.push_back(Pair("votes", 10*COIN));
    operVoteFund.push_back(voteFund);
    voteFund.clear();
    voteFund.push_back(Pair("delegate","wVTUdfEaeAAVSuXKrmMyqQXH5j5Z9oGmTt"));
    voteFund.push_back(Pair("votes", 10*COIN));
    Value retValue = basetest.CreateDelegateTx(sendTxAddr, write_string((Value)operVoteFund, false).c_str(), 0);
    string strHash;
    BOOST_CHECK(!basetest.GetHashFromCreatedTx(retValue, strHash));
}

//测试投票,投票人增量投票某候选人
BOOST_AUTO_TEST_CASE(oper_fund) {
    //增加对候选人票数
    string sendTxAddr = "wQewSbKL5kAfpwnrivSiCcaiFffgNva4uB";
//    string delegate_addr = "wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t";
    string delegate_addr = "wRQwgYkPNe1oX9Ts3cfuQ4KerqiV2e8gqM";
    int64_t value = -10 *COIN;
    OperAccountDelegateVote(sendTxAddr, delegate_addr, value);


//    string sendTxAddr = "wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4";
//    string delegate_addr = "wVTUdfEaeAAVSuXKrmMyqQXH5j5Z9oGmTt";
//    int64_t value = 10 *COIN;
//    OperAccountDelegateVote(sendTxAddr, delegate_addr, value);


}





BOOST_AUTO_TEST_SUITE_END()
