#ifdef TODO
#include <iostream>
#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>
#include "commons/serialize.h"
#include "main.h"
#include "systestbase.h"
using namespace std;
#define DAY_BLOCKS	((24 * 60 * 60)/(10*60))
#define MONTH_BLOCKS (30*DAY_BLOCKS)
#define CHAIN_HEIGHT (10*MONTH_BLOCKS+10)
#define TEST_SIZE 10000
#define MAX_FUND_MONEY 15
#define RANDOM_FUND_MONEY (random(MAX_FUND_MONEY)+1)
#define random(x) (rand()%x)

#define txid "022596466a"
#define amount 100*COIN
#define number 20

bool GetRpcHash(const string &hash, string &retHash)
{
	const char *argv[] = { "rpctest", "gethash", hash.c_str()};
	int argc = sizeof(argv) / sizeof(char*);
	Value value;
	if (!SysTestBase::CommandLineRPC_GetValue(argc, argv, value)) {
		return false;
	}
	const Value& result = find_value(value.get_obj(), "txid");
	if(result == null_type) {
		return false;
	}
	retHash = result.get_str();
	return true;
}
//bool IsEqual(const vector<CFund>& vSrc, const vector<CFund>& vDest) {
//	if (vSrc.size() != vDest.size()) {
//		return false;
//	}
//
//	for (vector<CFund>::const_iterator it = vDest.begin(); it != vDest.end(); it++) {
//		if (vSrc.end() == find(vSrc.begin(), vSrc.end(), *it)) {
//			return false;
//		}
//	}
//
//	return true;
//}
struct CTxTest :public SysTestBase{
	int nRunTimeHeight;
	string strRegID;
	string strKeyID;
	string strSignAddr;
	CAccount accOperate;
	CAccount accBeforOperate;
	vector<unsigned char> authorScript;
	UnsignedCharArray v[11]; //0~9 is valid,10 is used to for invalid scriptID

	CTxTest() {
		ResetEnv();


		accOperate.keyid.SetNull();
		accBeforOperate = accOperate;
		Init();
	}
	~CTxTest(){

	}


	void InitFund() {
		srand((unsigned) time(NULL));

//		for (int i = 0; i < TEST_SIZE/100; i++) {
//			accOperate.vRewardFund.push_back(CFund(RANDOM_FUND_MONEY, random(5)));
//		}

//		for (int i = 0; i < TEST_SIZE; i++) {
//			int nFundHeight = CHAIN_HEIGHT - MONTH_BLOCKS;
//			accOperate.OperateBalance(ADD_BCOIN, nFundHeight+random(MONTH_BLOCKS), nFundHeight);
//		}
	}

	void Init() {

		nRunTimeHeight = 0;
		strRegID = "000000000900";
		strKeyID = "a4529134008a4e09e68bec89045ccea6c013bd0b";
		strSignAddr = "dsjkLDFfhenmx2JkFMdtJ22TYDvSGgmJem";

		CKeyID keyId;
		keyId.SetHex(strKeyID);
		accOperate.keyid = keyId;


		accOperate.bcoins = TEST_SIZE*5;

		InitFund();
	}


	void CheckAccountEqual(bool bCheckAuthority = true) {
//		BOOST_CHECK(IsEqual(accBeforOperate.vRewardFund, accOperate.vRewardFund));
//		BOOST_CHECK(accBeforOperate.bcoins == accOperate.bcoins);

		//cout<<"old: "<<GetTotalValue(accBeforOperate.vSelfFreeze)<<" new: "<<GetTotalValue(accOperate.vSelfFreeze)<<endl;
	}

};

BOOST_FIXTURE_TEST_SUITE(tx_tests,CTxTest)

BOOST_FIXTURE_TEST_CASE(tx_add_free,CTxTest) {
	//invalid data
//	CFund fund(1, CHAIN_HEIGHT + 1);
	int height = chainActive.Height();
	BOOST_CHECK(accOperate.OperateBalance(ADD_BCOIN, 1, height));
//	fund.value = BASECOIN_MAX_MONEY;


//	accOperate.CompactAccount(CHAIN_HEIGHT);

	for (int i = 0; i < TEST_SIZE; i++) {
	//	uint64_t nOld = accOperate.GetRewardAmount(CHAIN_HEIGHT)+accOperate.GetFreeBcoins(CHAIN_HEIGHT);
		uint64_t randValue = random(10);
	//	CFund fundReward(randValue, CHAIN_HEIGHT - 1);
		BOOST_CHECK(accOperate.OperateBalance(ADD_BCOIN, randValue, height));
		//BOOST_CHECK(accOperate.GetRewardAmount(CHAIN_HEIGHT)+accOperate.GetFreeBcoins(CHAIN_HEIGHT) == nOld + randValue);

	}

	BOOST_CHECK(!accOperate.OperateBalance(ADD_BCOIN, GetBaseCoinMaxMoney(), height));

	CheckAccountEqual();
}

/**
 * brief	:each height test 10 times
 */
BOOST_FIXTURE_TEST_CASE(tx_minus_free,CTxTest) {

	for (int i = 1; i <= TEST_SIZE / 10; i++) {

//		for (int j = 0; j < 10; j++) {
//			uint64_t nOldVectorSum = GetTotalValue(accOperate.vFreedomFund);
//			uint64_t minusValue = random(40) + 1;
//			CFund fund(minusValue, random(20));
//			if (nOldVectorSum >= minusValue) {
//
//				BOOST_CHECK(accOperate.OperateBalance(MINUS_BCOIN, fund));
//				BOOST_CHECK(GetTotalValue(accOperate.vFreedomFund) == nOldVectorSum - minusValue);
//			}
//
//		}

//		CAccount accountAfterOper = accOperate;
//		accOperate.UndoOperateAccount(accOperate.accountOperLog);
//		CheckAccountEqual();
//
//		accOperate = accountAfterOper;
//		CAccountOperLog log;
//		accOperate.accountOperLog = log;
//
//		accBeforOperate = accOperate;
	}

}

BOOST_FIXTURE_TEST_CASE(red_packet, CTxTest) {
	//gethash
	string retHash;
	vector<int> vRetPacket;
	int64_t nTotal = 0;
	string initHash = txid;
	do{
		BOOST_CHECK(GetRpcHash(initHash, retHash));
		initHash = retHash;
		vector<unsigned char> vRet = ParseHex(retHash);
		for(size_t i=0; i< vRet.size(); )
		{
			int data = vRet[i] << 16 | vRet[i+1];
			vRetPacket.push_back(data % 1000 + 1000);
			nTotal += data % 1000 + 1000;
			if (vRetPacket.size() == number)
				break;
			i += 2;
		}
	}while(vRetPacket.size() != number);
	vector<int> vRedPacket;
	int64_t total_packet = 0;
	for(size_t j=0; j<vRetPacket.size(); ++j)
	{
		int64_t redAmount = vRetPacket[j] * amount / nTotal;
		vRedPacket.push_back(redAmount);
		total_packet += redAmount;
		double dRedAmount = redAmount/COIN;
		cout << "index"<< j << ", redPackets:" <<redAmount << " " <<dRedAmount<< endl;
	}
	double dTotalPacket = total_packet/COIN;
	cout << "total:" <<total_packet<< " "<<dTotalPacket<< endl;

}

BOOST_AUTO_TEST_SUITE_END()
#endif//TODO
