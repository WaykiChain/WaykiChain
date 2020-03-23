#include "CRedPacket_tests.h"
#include "CycleTestManger.h"

typedef struct {
	unsigned char dnType;
	uint64_t money;
	int number;
	unsigned char message[200];
	IMPLEMENT_SERIALIZE
	(
			READWRITE(dnType);
			READWRITE(money);
			READWRITE(number);
			for(int i = 0;i < 200;i++)
			READWRITE(message[i]);
	)
}RED_PACKET;

typedef struct {
	unsigned char dnType;
	unsigned char redhash[32];
	IMPLEMENT_SERIALIZE
	(
			READWRITE(dnType);
			for(int i = 0;i < 32;i++)
			READWRITE(redhash[i]);
	)
}ACCEPT_RED_PACKET;

typedef struct {
	unsigned char systype;               //0xff
	unsigned char type;            // 0x01 提现
	unsigned char typeaddr;            // 0x01 regid 0x02 base58
	IMPLEMENT_SERIALIZE
	(
		READWRITE(systype);
		READWRITE(type);
		READWRITE(typeaddr);
	)
} APPACC;
enum TXTYPE{
	TX_COMM_SENDREDPACKET = 0x01,
	TX_COMM_ACCEPTREDPACKET = 0x02,
	TX_SPECIAL_SENDREDPACKET = 0x03,
	TX_SPECIAL_ACCEPTREDPACKET = 0x04
};

//TEST_STATE CRedPacketTest::Run(){
//	 switch(nStep)
//	     {
//	     case 0:
//	    	 RegistScript();
//	    	 break;
//	     case 1:
//	    	 WaitRegistScript();
//	    	 break;
//	     case 2:
//	    	 WithDraw();
//	    	 break;
//	     case 3:
//	    	 WaitTxConfirmedPackage(txid);
//	    	 break;
//	     case 4:
//	    	 SendRedPacketTx();
//	    	 break;
//	     case 5:
//	    	 WaitTxConfirmedPackage(redHash);
//	    	 break;
//	     case 6:
//	    	 AcceptRedPacketTx();
//	    	 break;
//	     case 7:
//	    	 WaitTxConfirmedPackage(txid);
//	    	 break;
//	     default:
//	    	 nStep = 6;
//	    	 break;
//	     }
//	return next_state;
//
//}

TEST_STATE CRedPacketTest::Run(){
	 switch(nStep)
	     {
	     case 0:
	    	 RegistScript();
	    	 break;
	     case 1:
	    	 WaitRegistScript();
	    	 break;
	     case 2:
	    	 WithDraw();
	    	 break;
	     case 3:
	    	 WaitTxConfirmedPackage(txid);
	    	 break;
	     case 4:
	    	 SendSpecailRedPacketTx();
	    	 break;
	     case 5:
	    	 WaitTxConfirmedPackage(redHash);
	    	 break;
	     case 6:
	    	 AcceptSpecailRedPacketTx();
	    	 break;
	     case 7:
	    	 WaitTxConfirmedPackage(txid);
	    	 break;
	     default:
	    	 nStep = 6;
	    	 break;
	     }
	return next_state;

}


CRedPacketTest::CRedPacketTest(){
	nStep = 0;
	txid = "";
	strAppRegId = "";
	nNum = 0;
	appaddr = "";
	specailmM = 0;
	rchangeaddr = "";
}

bool CRedPacketTest::RegistScript()
{
	basetest.ImportAllPrivateKey();
	string strFileName("redpacket.bin");
	int nFee = basetest.GetRandomFee();
	int nCurHight;
	basetest.GetBlockHeight(nCurHight);
	string regAddr="";
	if(!SelectOneAccount(regAddr))
		return false;

	//reg anony app
	Value regscript = basetest.DeployContractTx(regAddr, strFileName, nCurHight, nFee+20*COIN);
	if(basetest.GetHashFromCreatedTx(regscript, txid)){
		nStep++;
		return true;
	}
	return false;
}
bool CRedPacketTest::WaitRegistScript()
{
	basetest.GenerateOneBlock();
	if (basetest.GetTxConfirmedRegID(txid, strAppRegId)) {
		nStep++;
		return true;
	}
	return true;
}
bool CRedPacketTest::WaitTxConfirmedPackage(string txid)
{
	basetest.GenerateOneBlock();
	string regid ="";
	if (basetest.GetTxConfirmedRegID(txid, regid)) {
		nStep++;
			return true;
	}
	return true;
}
bool CRedPacketTest::WithDraw()
{
	APPACC accdata;
	memset(&accdata,0,sizeof(APPACC));
	accdata.systype = 0xff;
	accdata.type = 0x02;  /// 0xff 表示提现 或者充值 0x01 提现 0x02 充值
	CDataStream scriptData(SER_DISK, CLIENT_VERSION);
	scriptData << accdata;
	string sendcontract = HexStr(scriptData);

	if(!SelectOneAccount(appaddr))
		return false;

	rchangeaddr = appaddr;
	uint64_t money = 100000000000;
	Value  darwpack= basetest.CallContractTx(strAppRegId,appaddr,sendcontract,0,100000000,money);

	if(basetest.GetHashFromCreatedTx(darwpack, txid)){
		nStep++;
		return true;
	}
	return true;
}
bool CRedPacketTest::SendRedPacketTx(){

	if(strAppRegId == "" || appaddr =="")
		return false;

	RED_PACKET redpacket;
	redpacket.dnType = TX_COMM_SENDREDPACKET;
	redpacket.money = 100000000;
	redpacket.number = 2;

	CDataStream scriptData(SER_DISK, CLIENT_VERSION);
	scriptData << redpacket;
	string sendcontract = HexStr(scriptData);


	Value  buyerpack= basetest.CallContractTx(strAppRegId,appaddr,sendcontract,0,100000000,redpacket.money);

	if(basetest.GetHashFromCreatedTx(buyerpack, redHash)){
		nStep++;
		return true;
	}
	return true;
}
bool CRedPacketTest::AcceptRedPacketTx(){

	if(strAppRegId == "")
		return false;

	ACCEPT_RED_PACKET Acceptredpacket;

	Acceptredpacket.dnType = TX_COMM_ACCEPTREDPACKET;
	memcpy(Acceptredpacket.redhash, uint256S(redHash).begin(), sizeof(Acceptredpacket.redhash));

	CDataStream scriptData(SER_DISK, CLIENT_VERSION);
	scriptData << Acceptredpacket;
	string sendcontract = HexStr(scriptData);
	string regAddr="";
	if(!SelectOneAccount(regAddr))
		return false;

	Value  buyerpack= basetest.CallContractTx(strAppRegId,regAddr,sendcontract,0,100000000,0);

	if(basetest.GetHashFromCreatedTx(buyerpack, txid)){
		nStep++;
		return true;
	}else{
		 nStep = 4;
	}
	return true;
}

void CRedPacketTest::Initialize(){
	CycleTestManger aCycleManager  = CycleTestManger::GetNewInstance();
	vector<std::shared_ptr<CycleTestBase> > vTest;
	if(0 == nNum) { //if don't have input param -number, default create 100 CCreateTxText instance defalue;
		nNum = 1;
	}
	for(int i=0; i< nNum; ++i)
	{
		vTest.push_back(std::make_shared<CRedPacketTest>());
	}
	aCycleManager.Initialize(vTest);
	aCycleManager.Run();
}
bool CRedPacketTest::SendSpecailRedPacketTx()
{
	if(strAppRegId == "" || appaddr =="")
		return false;

	RED_PACKET redpacket;
	redpacket.dnType = TX_SPECIAL_SENDREDPACKET;
	redpacket.money = 2000000000;
	redpacket.number = 2;
	CDataStream scriptData(SER_DISK, CLIENT_VERSION);
	scriptData << redpacket;
	string sendcontract = HexStr(scriptData);


	Value  Specailbuyerpack= basetest.CallContractTx(strAppRegId,appaddr,sendcontract,0,100000000,redpacket.money);

	if(basetest.GetHashFromCreatedTx(Specailbuyerpack, redHash)){
		nStep++;
		return true;
	}
	return true;
}
bool CRedPacketTest::AcceptSpecailRedPacketTx()
{
	if(strAppRegId == "")
		return false;
	if(WithDraw())
	{
		ACCEPT_RED_PACKET Acceptredpacket;

		Acceptredpacket.dnType = TX_SPECIAL_ACCEPTREDPACKET;
		memcpy(Acceptredpacket.redhash, uint256S(redHash).begin(), sizeof(Acceptredpacket.redhash));

		CDataStream scriptData(SER_DISK, CLIENT_VERSION);
		scriptData << Acceptredpacket;
		string sendcontract = HexStr(scriptData);

		Value  buyerpack= basetest.CallContractTx(strAppRegId,rchangeaddr,sendcontract,0,1000000000,0);

		if(basetest.GetHashFromCreatedTx(buyerpack, txid)){
			nStep++;
			return true;
		}else{
			 nStep = 4;
		}
	}
	return true;
}
BOOST_FIXTURE_TEST_SUITE(CredTest,CRedPacketTest)

BOOST_FIXTURE_TEST_CASE(Test,CRedPacketTest)
{
	Initialize();
}
BOOST_AUTO_TEST_SUITE_END()
