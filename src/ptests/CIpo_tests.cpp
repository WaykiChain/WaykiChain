/*
 * CAnony_tests.cpp
 *
 *  Created on: 2015-04-24
 *      Author: frank
 */

#include "CIpo_tests.h"
#include "CycleTestManger.h"
#include <boost/assign/list_of.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_reader.h"
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

typedef struct user{
	unsigned char address[35];
	int64_t money;
	int64_t freemoney;
	int64_t freeMothmoney;
	user()
	{
		memset(address,0,35);
		money = 0;
		freemoney = 0;
		freeMothmoney = 0;
	}
	IMPLEMENT_SERIALIZE
	(
			for(int i = 0;i < 35;i++)
			READWRITE(address[i]);
			READWRITE(money);
			READWRITE(freemoney);
			READWRITE(freeMothmoney);
	)
}IPO_USER;

typedef struct tagdddd{
	const char * pAddress;
	int64_t nMoney;
}IPO_DATA;

const int64_t totalSendMoney = 1340000000;   //
//const string g_strAppId = "2794-1";              //解冻周期每期10分钟应用
const string g_strAppId = "2816-1";                  //解冻周期每期一周应用
IPO_DATA arrayData[]=
{
	/*测试应用*/
	//  {"WYyZABrjmyCcGXbpWDSf7pzrfHWJtLDTQK",    30000000},
	//  {"WSYh2s6YM5kyeTtBZrZNdNo6QoYYwidFTj",    30000000}
};



#define max_user   300 //100
#define freeze_month_num    100

static IPO_USER userarray[max_user];
CIpoTest::CIpoTest():nNum(0), nStep(0), strTxHash(""), strAppRegId("") {

}

TEST_STATE CIpoTest::Run(){

	int64_t nMoneySend(0);
	size_t t_num = sizeof(arrayData) / sizeof(arrayData[0]);
	BOOST_CHECK(t_num <= max_user);         //防止越界
	//初始化地址表
	for (size_t i = 0; i < t_num; i++) {
		memcpy((char*)userarray[i].address,(char*)arrayData[i].pAddress,sizeof(userarray[i].address));
		userarray[i].money = arrayData[i].nMoney;
		userarray[i].freeMothmoney = arrayData[i].nMoney / freeze_month_num;
		userarray[i].freemoney = userarray[i].money - userarray[i].freeMothmoney * (freeze_month_num - 1);
		nMoneySend += userarray[i].money;  //统计总金额

		cout<<"newaddr"<<i<<"address="<<userarray[i].address<<endl;
		cout<<"newaddr"<<i<<"money="<<userarray[i].money<<endl;
		cout<<"newaddr"<<i<<"freemoney="<<userarray[i].freemoney<<endl;
		cout<<"newaddr"<<i<<"freeMothmoney="<<userarray[i].freeMothmoney<<endl;
	}
	BOOST_CHECK(nMoneySend == totalSendMoney);

#if 0
    // 注册ipo脚本
	RegistScript();

	/// 等待ipo脚本被确认到block中
	while(true)
	{
		if(WaitComfirmed(strTxHash, strAppRegId)) {
					break;
				}
	}

#endif

	cout<<"SendIpoTx start"<<endl;
	SendIpoTx(0);
	cout<<"SendIpoTx end"<<endl;
	return end_state;
}

void CIpoTest::RunIpo(unsigned char type){
	int64_t nMoneySend(0);
	size_t t_num = sizeof(arrayData) / sizeof(arrayData[0]);
	BOOST_CHECK(t_num <= max_user);         //防止越界
	//初始化地址表
	for (size_t i = 0; i < t_num; i++) {
		memcpy((char*)userarray[i].address,(char*)arrayData[i].pAddress,sizeof(userarray[i].address));
		if(!strcmp((char *)userarray[i].address,"DnKUZMvwXfprFCKhnsWRsbJTNnRZg88T2F") ||
			!strcmp((char *)userarray[i].address, "DftLSeJrMjJJ3UPeehNgArhcoAuDN5422E") ||
			!strcmp((char *)userarray[i].address, "Dg2dq98hcm84po3RX354SzVyE6DLpxq3QR") ||
			!strcmp((char *)userarray[i].address, "Dpjs5pvXmZbVt3uDEfBrMNbCsWjJzjm8XA") ||
			!strcmp((char *)userarray[i].address, "DZYDEn8CZuwgJ6YS6Zm7VvKaFc6E6tGstz") ||
			!strcmp((char *)userarray[i].address, "DcyumTafQsSh4hJo4V6DaS23Dd2QnpMXKH")) {
			userarray[i].freemoney = 0;  //自由金额已领
		}
		else {
			userarray[i].freemoney = arrayData[i].nMoney;
		}
		 //16696666666666610
		userarray[i].freeMothmoney = arrayData[i].nMoney;
		if(type == 1)
		{  //冻结1次
			userarray[i].money = arrayData[i].nMoney;
		}else{ // 冻结11次 改为冻结10次
			userarray[i].money = userarray[i].freeMothmoney * 10 + userarray[i].freemoney;
		}
		nMoneySend += userarray[i].money;  //统计总金额

	}
	BOOST_CHECK(nMoneySend == totalSendMoney);

//	//// main网络不用
//	/// 给每个地址转一定的金额
//	int64_t money = COIN;
//	t_num = sizeof(arrayData) / sizeof(arrayData[0]);
//	BOOST_CHECK(t_num <= max_user);         //防止越界
//	for(int i=0;i <t_num;i++)
//	{
//		string des =strprintf("%s", userarray[i].address);
//		basetest.CreateNormalTx(des,money);
//	}
//
//	 cout<<"end mempool"<<endl;
//	while(true)
//	{
//		if(basetest.IsMemoryPoolEmpty())
//			break;
//		MilliSleep(100);
//	}


//	strAppRegId = "97792-1";  //"2-1"

   cout<<"SendIpoTx start"<<endl;
	SendIpoTx(type);
   cout<<"SendIpoTx end"<<endl;
}

bool CIpoTest::RegistScript(){

	const char* pKey[] = { "cNcJkU44oG3etbWoEvY46i5qWPeE8jVb7K44keXxEQxsXUZ85MKU",
			"cNcJkU44oG3etbWoEvY46i5qWPeE8jVb7K44keXxEQxsXUZ85MKU"};
	int nCount = sizeof(pKey) / sizeof(char*);
	basetest.ImportWalletKey(pKey, nCount);

//	string strFileName("IpoApp.bin");
	string strFileName("coin.bin");
	int nFee = basetest.GetRandomFee();
	int nCurHight;
	basetest.GetBlockHeight(nCurHight);
	string regAddr="dk2NNjraSvquD9b4SQbysVRQeFikA55HLi";

	//reg anony app
	Value regscript = basetest.DeployContractTx(regAddr, strFileName, nCurHight, nFee+1*COIN);//20
	if(basetest.GetHashFromCreatedTx(regscript, strTxHash)){
		return true;
	}
	return false;
}

bool CIpoTest::CreateIpoTx(string contact,int64_t llSendTotal){
	int pre =0xff;
	int type = 2;
	string buffer =strprintf("%02x%02x", pre,type);

	buffer += contact;

	Value  retValue = basetest.CallContractTx(strAppRegId, SEND_A, buffer, 0, COIN, llSendTotal);
	if(basetest.GetHashFromCreatedTx(retValue, strTxHash)){
			return true;
	}
	return false;
}
bool CIpoTest::SendIpoTx(unsigned char type)
{
	strAppRegId = g_strAppId;

	// 创建转账交易并且保存转账交易的hash
	Object objRet;
	Array SucceedArray;
	Array UnSucceedArray;
	ofstream file("ipo1_ret", ios::out | ios::ate);
	if (!file.is_open())
		throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open wallet dump file");

	map<string, string> mapTxHash;
	size_t t_num = sizeof(arrayData) / sizeof(arrayData[0]);
	BOOST_CHECK(t_num <= max_user);         //防止越界
	for(size_t i =0;i <t_num;i++)
	{
		string des = strprintf("%s", userarray[i].address);
		int64_t nMoney = userarray[i].money;   //领币的总金额
		Object obj;

		CDataStream scriptData(SER_DISK, CLIENT_VERSION);
		scriptData << userarray[i];
		string sendcontract = HexStr(scriptData);
		if(CreateIpoTx(sendcontract,userarray[i].money)){
			mapTxHash[des]= strTxHash;
			obj.push_back(Pair("addr", des));
			obj.push_back(Pair("amount", nMoney));
			obj.push_back(Pair("txid", strTxHash));
			SucceedArray.push_back(obj);
			cout<<"after SendIpoTx strTxHash="<<strTxHash<<endl;
		} else {
			obj.push_back(Pair("addr", des));
			obj.push_back(Pair("amount", nMoney));
			UnSucceedArray.push_back(obj);
			cout<<"after SendIpoTx strTxHash err"<<endl;
		}
	}
	objRet.push_back(Pair("succeed", SucceedArray));
	objRet.push_back(Pair("unsucceed", UnSucceedArray));
	file << json_spirit::write_string(Value(objRet), true).c_str();
	file.close();

	 cout<<"SendIpoTx wait tx is confirmed"<<endl;
	//确保每个转账交易被确认在block中才退出
	while(mapTxHash.size() != 0)
	{
		map<string, string>::iterator it = mapTxHash.begin();
		for(;it != mapTxHash.end();){
			string addr = it->first;
			string hash = it->second;
			string regindex = "";
			if(basetest.GetTxConfirmedRegID(hash,regindex)){
				it = mapTxHash.erase(it);
			}else{
				it++;
			}
		}
		MilliSleep(100);
	}

	cout<<"after SendIpoTx,check the balance of every address "<<endl;
	//校验发币后，各个地址的账户金额和冻结金额
	for (size_t i = 0; i < t_num; ++i) {

		uint64_t acctValue = basetest.GetBalance(arrayData[i].pAddress);
		cout<<"SendIpoTx addr:"<< arrayData[i].pAddress<<" balance="<<acctValue<<" freemoney="<<userarray[i].freemoney<<endl;
		BOOST_CHECK(acctValue >= (uint64_t)userarray[i].freemoney);

		// 校验每个月的冻结金额
		Value  retValue = basetest.GetContractAccountInfo(strAppRegId,arrayData[i].pAddress);
		Value  result = find_value(retValue.get_obj(), "vFrozenFunds");
		Array array = result.get_array();
//		int64_t nMoneySend(0);
		size_t j = 0;
		cout<<"SendIpoTx freeMonthNum="<<array.size()<<endl;
        for(j = 0;j < array.size();j++)
        {
        	int64_t freedmoney = find_value(array[j].get_obj(), "value").get_int64();
        	cout<<"after SendIpoTx src="<<userarray[i].freeMothmoney <<" dest="<<freedmoney<<endl;
        	BOOST_CHECK(freedmoney == userarray[i].freeMothmoney);
//        	nMoneySend += freedmoney;
        }
		if(type == 1)
		{  //冻结1次

		}else{
//            BOOST_CHECK(j == (12 - 1)); //11个冻结金额
			  BOOST_CHECK(j == (freeze_month_num - 1));
		}
	}

	return true;
}

void CIpoTest::SendErrorIopTx()
{   /*利用一个地址给自己账户充值，从脚本账户 50725-1 把钱取出来*/
	strAppRegId = "50725-1";
	IPO_USER useripo;
	char *dess = "DhxrQ9hsvo3fVVSy6By8bePt8cmPtts88R";
	memcpy((char*)useripo.address,dess,sizeof(useripo.address));
	useripo.money =1;
	useripo.freemoney = totalSendMoney+1;
	useripo.freeMothmoney = 0;
	Object obj;

	CDataStream scriptData(SER_DISK, CLIENT_VERSION);
	scriptData << useripo;
	string sendcontract = HexStr(scriptData);
	if(CreateIpoTx(sendcontract,useripo.money))
	{	cout<<"after SendIpoTx strTxHash="<<strTxHash<<endl;
	} else {
		cout<<"after SendIpoTx strTxHash err"<<endl;
	}
}
BOOST_FIXTURE_TEST_SUITE(CreateIpoTxTest,CIpoTest)

BOOST_FIXTURE_TEST_CASE(Test,CIpoTest)
{
	Run();
//	RunIpo(0); //冻结11次
//	RunIpo(1); //冻结1次
	//SendErrorIopTx();
}

typedef struct _IPOCON{
	unsigned char address[35];
	int64_t money;
}IPO_COIN;
#define max_2ipouser 100

BOOST_FIXTURE_TEST_CASE(get_coin,CIpoTest)
{

	// 创建转账交易并且保存转账交易的hash
	Object objRet;
	Array SucceedArray;
	Array UnSucceedArray;
	ofstream file("ipo_ret", ios::out | ios::ate);
	if (!file.is_open())
		throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open wallet dump file");

	map<string, string> mapTxHash;
	int64_t nMoneySend(0);
	size_t t_num = sizeof(arrayData) / sizeof(arrayData[0]);
	for (size_t i = 0; i <t_num ; ++i) {
		nMoneySend += arrayData[i].nMoney;
	}
	BOOST_CHECK(nMoneySend == totalSendMoney);
	for (size_t i = 0; i < t_num; ++i) {
		string des = strprintf("%s", arrayData[i].pAddress);
		int64_t nMoney = arrayData[i].nMoney;
		Value ret = basetest.CreateNormalTx(des, nMoney);
		string txid;
		Object obj;
		if(basetest.GetHashFromCreatedTx(ret, txid)) {
			mapTxHash[des]= txid;
			obj.push_back(Pair("addr", des));
			obj.push_back(Pair("amount", nMoney));
			obj.push_back(Pair("txid", txid));
			SucceedArray.push_back(obj);
		} else {
			obj.push_back(Pair("addr", des));
			obj.push_back(Pair("amount", nMoney));
			UnSucceedArray.push_back(obj);
		}
	}
	objRet.push_back(Pair("succeed", SucceedArray));
	objRet.push_back(Pair("unsucceed", UnSucceedArray));
	file << json_spirit::write_string(Value(objRet), true).c_str();
	file.close();

	//确保每个转账交易被确认在block中才退出
	while(mapTxHash.size() != 0)
	{
		map<string, string>::iterator it = mapTxHash.begin();
		for(;it != mapTxHash.end();){
			string addr = it->first;
			string hash = it->second;
			string regindex = "";
			if(basetest.GetTxConfirmedRegID(hash,regindex)){
				it = mapTxHash.erase(it);
			}else{
				it++;
			}
		}
		MilliSleep(100);
	}

	for (size_t i = 0; i < t_num; ++i) {
		uint64_t acctValue = basetest.GetBalance(arrayData[i].pAddress);
		BOOST_CHECK(acctValue >= (uint64_t)arrayData[i].nMoney);
	}
}
BOOST_FIXTURE_TEST_CASE(check_coin,CIpoTest)
{
	size_t t_num = sizeof(arrayData) / sizeof(arrayData[0]);
	for (size_t i = 0; i < t_num; ++i) {
		uint64_t acctValue = basetest.GetBalance(arrayData[i].pAddress);
		string errorMsg = strprintf("acctValue = %lld, realValue= %lld, address=%s \n",acctValue,  arrayData[i].nMoney, arrayData[i].pAddress);
		BOOST_CHECK_MESSAGE(acctValue >= (uint64_t )arrayData[i].nMoney, errorMsg);
	}
}

BOOST_FIXTURE_TEST_CASE(check_money,CIpoTest) {
	int64_t data1(0);
	int64_t total(0);
	size_t t_num = sizeof(arrayData) / sizeof(arrayData[0]);
	BOOST_CHECK(t_num <= max_user);         //防止越界
	//初始化地址表
	for (size_t i = 0; i < t_num; i++) {
		if(!strcmp((char *)arrayData[i].pAddress,"DnKUZMvwXfprFCKhnsWRsbJTNnRZg88T2F") ||
					!strcmp((char *)arrayData[i].pAddress, "DftLSeJrMjJJ3UPeehNgArhcoAuDN5422E") ||
					!strcmp((char *)arrayData[i].pAddress, "Dg2dq98hcm84po3RX354SzVyE6DLpxq3QR") ||
					!strcmp((char *)arrayData[i].pAddress, "Dpjs5pvXmZbVt3uDEfBrMNbCsWjJzjm8XA") ||
					!strcmp((char *)arrayData[i].pAddress, "DZYDEn8CZuwgJ6YS6Zm7VvKaFc6E6tGstz") ||
					!strcmp((char *)arrayData[i].pAddress, "DcyumTafQsSh4hJo4V6DaS23Dd2QnpMXKH")) {
					data1 += arrayData[i].nMoney;
				}
		total += arrayData[i].nMoney;
	}
	total = total * 11;
	total -= data1;
	cout <<"total amount:" << total <<endl;
}
BOOST_FIXTURE_TEST_CASE(check_recharge,CIpoTest) {

	int64_t nMoneySend(0);
	size_t t_num = sizeof(arrayData) / sizeof(arrayData[0]);
	BOOST_CHECK(t_num <= max_user);         //防止越界
	//初始化地址表
	for (size_t i = 0; i < 1; i++) {
		memcpy((char*)userarray[i].address,(char*)arrayData[i].pAddress,sizeof(userarray[i].address));
		userarray[i].freemoney = arrayData[i].nMoney;
		userarray[i].freeMothmoney = arrayData[i].nMoney;
		userarray[i].money = userarray[i].freeMothmoney * 10 + userarray[i].freemoney;
	}
	//"app regid"

    cout<<"SendIpoTx start"<<endl;
	SendIpoTx(0);
    cout<<"SendIpoTx end"<<endl;
}
BOOST_AUTO_TEST_SUITE_END()

