/*
 * CycleP2PBet_test.cpp
 *
 *  Created on: 2015��1��15��
 *      Author: spark.huang
 */

#include "CycleP2PBet_tests.h"
enum OPERATE{
	OP_SYSTEMACC = 0x00,
	OP_APPACC = 0x01,
	OP_NULL = 0x02,
};
#define OUT_HEIGHT 20
CTestBetTx::CTestBetTx() {
	memset(nSdata, 0, sizeof(nSdata));
	mCurStep = 0;
	strRegScriptHash = "";
	scriptid = "";
	strRegScriptHash = "";
	strAsendHash = "";
	strBacceptHash = "";
	strAopenHash = "";
	betamount = 0;
}

CTestBetTx::~CTestBetTx() {
}

TEST_STATE CTestBetTx::Run() {
	switch (mCurStep) {
	case 0:
		RegScript();
		break;
	case 1:
		WaiteRegScript();
		break;
	case 2:
		ASendP2PBet();
		break;
	case 3:
		WaitASendP2PBet();
		break;
	case 4:
		BAcceptP2PBet();
		break;
	case 5:
		WaitBAcceptP2PBet();
		break;
	case 6:
		AOpenP2PBet();
		break;
	case 7:
		WaitAOpenP2PBet();

		break;
	default:
		assert(0);
		break;
	}
	return next_state;
}


bool CTestBetTx::RegScript(void) {

	const char* pKey[] = { "cPqVgscsWpPgkLHZP3pKJVSU5ZTCCvVhkd5cmXVWVydXdMTtBGj7",
			"cU1dxQgvyKt8yEqqkKiNLK9jfyW498RKi8y2evqzjtLXrLD4fBMs", };
	int nCount = sizeof(pKey) / sizeof(char*);
	basetest.ImportWalletKey(pKey, nCount);

	string strFileName("p2pbet.bin");
	int nCurHight;
	GetBlockHeight(nCurHight);
	//ע��ԶĽű�
	Value valueRes = DeployContractTx(ADDR_A, strFileName, nCurHight, 200000000);
	//BOOST_CHECK(GetHashFromCreatedTx(valueRes, strRegScriptHash));
	if(GetHashFromCreatedTx(valueRes, strRegScriptHash)){
		mCurStep++;
		return true;
	}
//	BOOST_CHECK(GenerateOneBlock());
	return true;
}
bool CTestBetTx::WaiteRegScript(void){
	if (basetest.GetTxConfirmedRegID(strRegScriptHash, scriptid)) {
			mCurStep++;
			return true;
	}

	return true;
}
bool CTestBetTx::ASendP2PBet() {

	if(scriptid == "")
		return false;

		unsigned char randdata[32];
		GetRandomData(randdata, sizeof(randdata));
		int num = GetBetData();
//		cout<<"win:"<<num<<endl;
		memcpy(nSdata,randdata,sizeof(randdata));
		memcpy(&nSdata[32],&num,sizeof(char));
		SEND_DATA senddata;

		senddata.noperateType = GetRanOpType();
		senddata.type = 1;
		senddata.hight = OUT_HEIGHT;
		senddata.money = GetRandomBetAmount();
		betamount = senddata.money;
		memcpy(senddata.dhash, Hash(nSdata, nSdata + sizeof(nSdata)).begin(), sizeof(senddata.dhash));
//		vector<unsigned char>temp;
//		temp.assign(nSdata,nSdata + sizeof(nSdata));
//		vector<unsigned char>temp2;
//		temp2.assign(senddata.dhash,senddata.dhash + sizeof(senddata.dhash));

		CDataStream scriptData(SER_DISK, CLIENT_VERSION);
		scriptData << senddata;
		string sendcontract = HexStr(scriptData);
		//uint64_t sendfee = GetRandomBetfee();

		uint64_t nTempSend = 0;
		if((int)senddata.noperateType == 0x01){
			nTempSend = 0;
		}else{
			nTempSend = betamount;
		}
		Value  sendret= CallContractTx(scriptid,ADDR_A,sendcontract,0,0,nTempSend);

	if (GetHashFromCreatedTx(sendret, strAsendHash)) {
		mCurStep++;
		return true;
	}
		return true;
}
bool CTestBetTx::WaitASendP2PBet(void){
	string index = "";
	if (basetest.GetTxConfirmedRegID(strAsendHash, index)) {
			mCurStep++;
			return true;
	}
	return true;
}
bool CTestBetTx::BAcceptP2PBet(void) {
		unsigned char cType;
		RAND_bytes(&cType, sizeof(cType));
		unsigned char  gussnum = cType % 2;
		ACCEPT_DATA acceptdata;
		acceptdata.type = 2;
		acceptdata.noperateType = GetRanOpType();
		acceptdata.money = betamount;
		acceptdata.data=gussnum;
		memcpy(acceptdata.txid, uint256S(strAsendHash).begin(), sizeof(acceptdata.txid));

		CDataStream scriptData(SER_DISK, CLIENT_VERSION);
		scriptData << acceptdata;

		string acceptcontract = HexStr(scriptData);
		int nCurHight;
		GetBlockHeight(nCurHight);

		uint64_t nTempSend = 0;
		if((int)acceptdata.noperateType  == 0x01){
			nTempSend = 0;
		}else{
			nTempSend = betamount;
		}
		Value vaccept = CallContractTx(scriptid, ADDR_B, acceptcontract, nCurHight, 0,nTempSend);
		string hash = "";
//		cout<<"type"<<(int)acceptdata.noperateType<<endl;
/*		if((int)acceptdata.noperateType == 0x01){

			GetHashFromCreatedTx(vaccept, hash);
			if(hash == "") {
				return true;
			}
			else{
				BOOST_CHECK(GenerateOneBlock());
			}
		}else{
			BOOST_CHECK(GetHashFromCreatedTx(vaccept, hash));
			BOOST_CHECK(GenerateOneBlock());
		}*/

		if(GetHashFromCreatedTx(vaccept, strBacceptHash)){
			mCurStep++;
			return true;
		}
		return true;
}
bool CTestBetTx::WaitBAcceptP2PBet(void){
	string index = "";
	if (basetest.GetTxConfirmedRegID(strBacceptHash, index)) {
			mCurStep++;
			return true;
	}
	return true;
}
bool CTestBetTx::AOpenP2PBet(void) {

		OPEN_DATA openA;
		openA.noperateType = GetRanOpType();
		openA.type = 3;
		memcpy(openA.txid, uint256S(strAsendHash).begin(), sizeof(openA.txid));
		memcpy(openA.dhash, nSdata, sizeof(nSdata));
		CDataStream scriptData(SER_DISK, CLIENT_VERSION);
		scriptData << openA;
		string openAcontract = HexStr(scriptData);

		int nCurHight;
		GetBlockHeight(nCurHight);
		Value vopenA = CallContractTx(scriptid, ADDR_A, openAcontract, nCurHight, 0);
		if(GetHashFromCreatedTx(vopenA, strAopenHash)){
			mCurStep++;
				return true;
		}
		return true;
}
bool CTestBetTx::WaitAOpenP2PBet(void){
	string index = "";
	if (basetest.GetTxConfirmedRegID(strAopenHash, index)) {
			mCurStep = 1;
			 IncSentTotal();
			return true;
	}
		return true;
}
