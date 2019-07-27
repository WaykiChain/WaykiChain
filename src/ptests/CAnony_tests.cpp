/*
 * CAnony_tests.cpp
 *
 *  Created on: 2015-04-24
 *      Author: frank
 */

#include "CAnony_tests.h"
#include "CycleTestManger.h"

CAnonyTest::CAnonyTest():nNum(0), nStep(0), strTxHash(""), strAppRegId(""),regId("") {
	for (int i = 1; i < boost::unit_test::framework::master_test_suite().argc; ++i) {
		string strArgv = boost::unit_test::framework::master_test_suite().argv[i];
		string::size_type pos = strArgv.find("-number=");
		if (string::npos != pos) {
			string strNum = strArgv.substr(pos + strlen("-number="), string::npos);
			nNum = Str2Int(strNum);
			break;
		}
	}
}

TEST_STATE CAnonyTest::Run(){
	switch(nStep){
	case 0:
	{
		if(RegistScript())
			nStep = 2;
		break;
	}
	case 1:
		CreateAnonyTx();
		break;
	case 2:
		if(WaitComfirmed(strTxHash, strAppRegId)) {
			nStep = 1;
		}
		break;
	default:
		nStep = 1;
		break;
	}
	return next_state;
}

bool CAnonyTest::RegistContract(){
	string strFileName("anony.bin");
	int nFee = basetest.GetRandomFee();
	int nCurHight;
	basetest.GetBlockHeight(nCurHight);
	string regAddr="";
	if(!SelectOneAccount(regAddr))
		return false;
	//reg anony app
	Value regscript = basetest.CLuaContractDeployTx(regAddr, strFileName, nCurHight, nFee+20*COIN);
	if(basetest.GetHashFromCreatedTx(regscript, strTxHash)){
		return true;
	}
	return false;
}

bool CAnonyTest::CreateAnonyTx(){
	srand(time(NULL));
	int nNum = (rand() % 3) + 3;
	vector<string> vRegId;
	string regId;
	std::pair<std::set<string>::iterator, bool> ret;
	for(int i=0; i< nNum;) {
		if(SelectOneAccount(regId)) {
			if(vRegId.end() == find(vRegId.begin(), vRegId.end(), regId)) {
				CRegID nRegid(regId);
				string strTemp = HexStr(nRegid.GetRegIdRaw());
				vRegId.push_back(strTemp);
				++i;
			}
		}
		else
			return false;
	}
	ACCOUNT_INFO accountInfo;
	int accountInfoSize = ::GetSerializeSize(accountInfo, SER_NETWORK, PROTOCOL_VERSION);
	unsigned short length = (nNum-1) * accountInfoSize;

	ACCOUNT_INFO pAccountInfo[nNum-1];
	memset(pAccountInfo, 0, length);

	vector<unsigned char> sendRegId = ParseHex(vRegId[0]);
	int64_t llSendTotal(0);
	for(int i=1; i<nNum; ++i) {
		vector<unsigned char> vTemp =ParseHex(vRegId[i]);
		memcpy(&accountInfo.account, &vTemp.at(0),vTemp.size());
		accountInfo.nReciMoney = i*100*COIN;
		llSendTotal += accountInfo.nReciMoney;
		pAccountInfo[i-1] = accountInfo;
		//memcpy(pAccountInfo+(i-1)*accountInfoSize, &accountInfo, accountInfoSize);
	}
	CONTRACT head;
	memcpy(head.Sender, &sendRegId.at(0), sendRegId.size());
	head.nPayMoney = llSendTotal;
	head.len = length;
	CDataStream ds(SER_DISK, CLIENT_VERSION);
	ds<<head;
	for(int i=1; i<nNum; ++i)
		ds<<pAccountInfo[i-1];

	string contract = HexStr(ds);
	Value  retValue = basetest.CallContractTx(strAppRegId, vRegId[0], contract, 0, 0, llSendTotal);
	if(basetest.GetHashFromCreatedTx(retValue, strTxHash)){
			return true;
	}
	return false;
}

void CAnonyTest::Initialize(){
	CycleTestManger aCycleManager  = CycleTestManger::GetNewInstance();
	vector<std::shared_ptr<CycleTestBase> > vTest;
	if(0 == nNum) { //if don't have input param -number, default create 100 CCreateTxText instance defalue;
		nNum = 100;
	}
	for(int i=0; i< nNum; ++i)
	{
		vTest.push_back(std::make_shared<CAnonyTest>());
	}
	aCycleManager.Initialize(vTest);
	aCycleManager.Run();
}

BOOST_FIXTURE_TEST_SUITE(CreateAnonyTxTest,CAnonyTest)

BOOST_FIXTURE_TEST_CASE(Test,CAnonyTest)
{
	Initialize();
}

BOOST_AUTO_TEST_SUITE_END()
