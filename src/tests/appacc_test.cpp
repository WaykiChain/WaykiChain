// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2014-2015 The WaykiChain Core developers
// Copyright (c) 2016 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php
#ifdef TODO

#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include  "boost/filesystem/operations.hpp"
#include  "boost/filesystem/path.hpp"
#include  "../vm/appaccount.h"
#include "../vm/vmrunenv.h"
#include "tx/tx.h"
#include "commons/util.h"

using namespace std;

int64_t opValue[8][8] = {
    {100 * COIN, 10 * COIN, 10 * COIN, 30 * COIN, 40 * COIN, 30 * COIN, 30 * COIN, 20 * COIN},        //false
    {1000 * COIN, 200 * COIN, 20 * COIN, 30 * COIN, 40 * COIN, 20 * COIN, 10 * COIN, 20 * COIN},      //false
    {500 * COIN, 200 * COIN, 20 * COIN, 100 * COIN, 200 * COIN, 100 * COIN, 300 * COIN, 100 * COIN},  //false
    {100 * COIN, 10 * COIN, 20 * COIN, 50 * COIN, 50 * COIN, 10 * COIN, 20 * COIN, 30 * COIN},        //false
    {200 * COIN, 20 * COIN, 30 * COIN, 40 * COIN, 30 * COIN, 30 * COIN, 40 * COIN, 40 * COIN},        //false
    {1000 * COIN, 200 * COIN, 20 * COIN, 500 * COIN, 800 * COIN, 400 * COIN, 200 * COIN, 100 * COIN}, //true
    {500 * COIN, 200 * COIN, 200 * COIN, 300 * COIN, 200 * COIN, 50 * COIN, 100 * COIN, 50 * COIN},   //true
    {600 * COIN, 200 * COIN, 20 * COIN, 30 * COIN, 50 * COIN, 60 * COIN, 70 * COIN, 20 * COIN}        //false
};

// appacc_tests/key_test1


bool CheckAppAcct(int64_t opValue[]) {
    CRegID srcRegId(100,1);
    CRegID desRegId(100,2);
    CRegID desUser1RegId(100,3);
    CRegID desUser2RegId(100,4);
    CAccount contractAcct;
    contractacct.free_bcoins = 100 * COIN;  //这里将TX中100 COIN先充值到合约账户中，扮演系统操作账户角色
    contractAcct.regid = desRegId;

    CUserID srcUserId = srcRegId;
    CUserID desUserId = desRegId;
    UnsignedCharArray arguments;
    CLuaContractInvokeTx tx(srcUserId, desRegId, 10000, opValue[0], 1, arguments); //100 * COIN

    CVmRunEnv vmRunEnv;
    vector<CVmOperate> vAcctOper;

    UnsignedCharArray vDesUser1RegId = desUser1RegId.GetRegIdRaw();
    int64_t temp                        = opValue[1];  // 10 * COIN
    CVmOperate acctAddOper;
    acctAddOper.accountType = regid;
    acctAddOper.opTye    = ADD_BCOIN;
    memcpy(acctAddOper.accountId, &vDesUser1RegId[0], 6);
    memcpy(acctAddOper.money, &temp, sizeof(temp));
    vAcctOper.push_back(acctAddOper);

    UnsignedCharArray vDesUser2RegId = desUser2RegId.GetRegIdRaw();
    temp = opValue[2];   //20 * COIN
    acctAddOper.accountType = regid;
    acctAddOper.opType = ADD_BCOIN;
    memcpy(acctAddOper.accountId, &vDesUser2RegId[0], 6);
    memcpy(acctAddOper.money, &temp, sizeof(temp));
    vAcctOper.push_back(acctAddOper);

    UnsignedCharArray vDesRegId = desRegId.GetRegIdRaw();
    temp = opValue[3];  //30 * COIN
    acctAddOper.accountType = regid;
    acctAddOper.opType = MINUS_BCOIN;
    memcpy(acctAddOper.accountId, &vDesRegId[0], 6);
    memcpy(acctAddOper.money, &temp, sizeof(temp));
    vAcctOper.push_back(acctAddOper);
    vmRunEnv.InsertOutputData(vAcctOper);

    CAppFundOperate appFundOper;
    appFundOper.opType = ADD_FREE_OP;
    appFundOper.mMoney = opValue[4];       //20 * COIN
    appFundOper.appuserIDlen = 6;
    memcpy(appFundOper.vAppuser,  &vDesUser1RegId[0], 6);
    appFundOper.fundTagLen = 6;
    memcpy(appFundOper.vFundTag, &vDesUser1RegId[0], 6);
    vmRunEnv.InsertOutAPPOperte(vDesUser1RegId, appFundOper);

    appFundOper.opType = SUB_FREE_OP;
    appFundOper.mMoney = opValue[5];      //90 * COIN
    appFundOper.appuserIDlen = 6;
    memcpy(appFundOper.vAppuser,  &vDesUser2RegId[0], 6);
    appFundOper.fundTagLen = 6;
    memcpy(appFundOper.vFundTag, &vDesUser2RegId[0], 6);
    vmRunEnv.InsertOutAPPOperte(vDesUser2RegId, appFundOper);

    appFundOper.opType = ADD_TAG_OP;
    appFundOper.mMoney = opValue[6];     // 90 * COIN
    appFundOper.appuserIDlen = 6;
    memcpy(appFundOper.vAppuser,  &vDesUser2RegId[0], 6);
    appFundOper.fundTagLen = 6;
    memcpy(appFundOper.vFundTag, &vDesUser2RegId[0], 6);
    vmRunEnv.InsertOutAPPOperte(vDesUser2RegId, appFundOper);

    appFundOper.opType = SUB_TAG_OP;
    appFundOper.mMoney = opValue[7];  // 80 * COIN
    appFundOper.appuserIDlen = 6;
    memcpy(appFundOper.vAppuser,  &vDesUser1RegId[0], 6);
    appFundOper.fundTagLen = 6;
    memcpy(appFundOper.vFundTag, &vDesUser1RegId[0], 6);
    vmRunEnv.InsertOutAPPOperte(vDesUser1RegId, appFundOper);

    return vmRunEnv.CheckAppAcctOperate(&tx);
}

BOOST_AUTO_TEST_SUITE(appacc_tests)

BOOST_AUTO_TEST_CASE(key_test1)
 {
    auto StrTVector = [&](string tag)
    {
        return vector<unsigned char>(tag.begin(),tag.end());
    };

    srand((int) time(NULL));

    vector<unsigned char> AppuserId = StrTVector("test1");
    vector<unsigned char> fundtag = StrTVector("foundtag");
    vector<unsigned char> fundtag2 = StrTVector("foundtag2");

    CAppFundOperate opTe(AppuserId,fundtag, ADD_TAG_OP, 500, 800000);
    BOOST_CHECK(opTe.GetFundTagV() == fundtag);
    BOOST_CHECK(opTe.GetUint64Value()== 800000);
    BOOST_CHECK(opTe.GetOpType()== ADD_TAG_OP);


    vector<CAppFundOperate> OpArry;
    uint64_t allmony = 0;

    int timeout = (rand() % 15000) + 51;
    int loop = 500;

    int maxtimeout = timeout + loop+1;
    for (int i = 0; i < loop; i++) {
        int64_t temp = ((rand() * rand()) % 15000000) + 20;
        if(temp < 0)
        {
            temp = 20;
            continue;
        }
        allmony += temp;
        CAppFundOperate op(AppuserId,fundtag, ADD_TAG_OP, timeout + i, temp);
        OpArry.insert(OpArry.end(), op);
    }

    CAppUserAccount AccCount(AppuserId);
    BOOST_CHECK(AccCount.GetAccUserId() == AppuserId);      //初始化的ID 必须是
    BOOST_CHECK(AccCount.Operate(OpArry));               //执行所有的操作符合
    BOOST_CHECK(AccCount.GetBcoins() == 0);            //因为操作符全是加冻结的钱所以自由金额必须是0

    {
        CAppCFund tep;
        BOOST_CHECK(AccCount.GetAppCFund(tep, fundtag, timeout)); //获取相应的冻结项
        BOOST_CHECK(tep.GetValue() == OpArry[0].GetUint64Value());                    //冻结的金额需要没有问题
        CAppCFund tep2;
        BOOST_CHECK(AccCount.GetAppCFund(tep2, fundtag, maxtimeout + 5) == false); //获取相应的冻结项 超时时间不同 必须获取不到

        AccCount.AutoMergeFreezeToFree(timeout - 1);                   //自动合并 超时高度没有到  这里的 50 是为了配合签名 time out de 51
        BOOST_CHECK(AccCount.GetAppCFund(tep, fundtag, timeout));            //没有合并必须金额还是没有变动
        BOOST_CHECK(tep.GetValue() == OpArry[0].GetUint64Value());          //没有合并必须金额还是没有变动
    }

    {
        vector<CAppFundOperate> OpArry2;
        CAppFundOperate subfreexeop(AppuserId,fundtag, SUB_TAG_OP, timeout, 8);
        OpArry2.insert(OpArry2.end(), subfreexeop);
        BOOST_CHECK(AccCount.Operate(OpArry2));               //执行所有的操作符合
    }

    {
        CAppCFund subtemptep;
        BOOST_CHECK(AccCount.GetAppCFund(subtemptep, fundtag, timeout));        //获取相应的冻结项
        BOOST_CHECK(subtemptep.GetValue() == (OpArry[0].GetUint64Value() - 8));    //上面减去了8  检查是否对
    }

    {
        vector<CAppFundOperate> OpArry2;
        CAppFundOperate revertfreexeop(AppuserId,fundtag, ADD_TAG_OP, timeout, 8);
        OpArry2.clear();
        OpArry2.insert(OpArry2.end(), revertfreexeop);
        BOOST_CHECK(AccCount.Operate(OpArry2));               //执行所有的操作符合
    }

    {
        CAppCFund reverttemptep;
        BOOST_CHECK(AccCount.GetAppCFund(reverttemptep, fundtag, timeout));          //没有合并必须金额还是没有变动
        BOOST_CHECK(reverttemptep.GetValue() == OpArry[0].GetUint64Value());            //没有合并必须金额还是没有变动
    }

    {           //合并第一个
        CAppCFund tep;
        AccCount.AutoMergeFreezeToFree(timeout);                                //自动合并 第0个
        BOOST_CHECK(AccCount.GetAppCFund(tep, fundtag, timeout) == false);      //必须找不到数据
        BOOST_CHECK(AccCount.GetBcoins() == OpArry[0].GetUint64Value());;                         //合并后自由金额必须没有问题
    }

    {                       //减去全部
        CAppFundOperate subfreeop(AppuserId,fundtag, SUB_FREE_OP, timeout, OpArry[0].GetUint64Value());
        vector<CAppFundOperate> OpArry2;
        OpArry2.insert(OpArry2.end(), subfreeop);
        BOOST_CHECK(AccCount.Operate(OpArry2));                             //执行所有的操作符合
        BOOST_CHECK(AccCount.GetBcoins() == 0);;                           //钱必须可以核对
    }

    {
        vector<CAppFundOperate> OpArry2;
        CAppFundOperate addfreeop(AppuserId,fundtag, ADD_FREE_OP, timeout, OpArry[0].GetUint64Value());    //再次把数据加进去
        OpArry2.clear();
        OpArry2.insert(OpArry2.end(), addfreeop);
        BOOST_CHECK(AccCount.Operate(OpArry2));                             //执行所有的操作符合
        BOOST_CHECK(AccCount.GetBcoins() == OpArry[0].GetUint64Value());                //加上后 就回来了
    }

    AccCount.AutoMergeFreezeToFree(maxtimeout);                     //全部合并
    printf("%lu, %lu\n", AccCount.GetBcoins(), allmony);
    BOOST_CHECK(AccCount.GetBcoins() == allmony);                //余额平账

}

BOOST_AUTO_TEST_CASE(checkappacct_test) {
    for(int j=0; j <8; ++j) {
        for(int i=0; i<8; ++i) {
            cout << opValue[j][i] <<" ";
        }
        cout << endl;
        int64_t txValue = opValue[j][0];
        int64_t acctMinusValue = opValue[j][3];
        int64_t acctSum = txValue - acctMinusValue;
        int64_t appAcctSum = opValue[j][4] - opValue[j][5] + opValue[j][6] - opValue[j][7];
        bool isCheck = (acctSum == appAcctSum);
        cout << "ischeck:" << isCheck << endl;
        BOOST_CHECK(CheckAppAcct(opValue[j]) == isCheck);
    }

}
BOOST_AUTO_TEST_SUITE_END()
#endif //TODO