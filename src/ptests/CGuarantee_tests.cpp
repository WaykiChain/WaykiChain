/*
 * CAnony_tests.cpp
 *
 *  Created on: 2015-04-24
 *      Author: frank
 */

#include "CGuarantee_tests.h"
#include "CycleTestManger.h"
#include <boost/assign/list_of.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_reader.h"
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;


/*
 * 测试地址及余额
 * */
#if 0
{
    "Address" : "dcmCbKbAfKrofNz35MSFupxrx4Uwn3vgjL",
    "KeyID" : "000f047f75144705b0c7f4eb30d205cd66f4599a",
    "RegID" : "1826-1437",
    "PublicKey" : "02bbb24c80a808cb6eb13de90c1dca99196bfce02bcf32812b7c4357a368877c68",
    "minerPubKey" : "02d546cc51b22621b093f08c679102b9b8ca3f1a07ea1d751de3f67c10670e635b",
    "Balance" : 129999825142,
    "CoinDays" : 0,
    "UpdateHeight" : 45433,
    "CurCoinDays" : 1457,
    "position" : "inblock"
}
{
    "Address" : "dcmWdcfxEjRXUHk8LpygtgDHTpixoo3kbd",
    "KeyID" : "001e1384420cad01b0cd364ef064852b1bf3fd96",
    "RegID" : "1826-1285",
    "PublicKey" : "03d765b0f2bae7f6f61350b17bce5e57445cc286cada56d9c61987db5cbd533c43",
    "MinerPKey" : "025cae56b5faf1042f2d6610cde892f0cb1178282fb7b345b78611ccee4feab128",
    "Balance" : 128999717658,
    "CoinDays" : 0,
    "UpdateHeight" : 46859,
    "CurCoinDays" : 169,
    "position" : "inblock"
}
{
    "Address" : "dcnGLkGud6c5bZJSUghzxvCqV45SJEwRcH",
    "KeyID" : "00429013e06bbcdc0529dd5b1117ddf4630544ad",
    "RegID" : "1826-1081",
    "PublicKey" : "02b509a4240ae08118ff2336981301cb2baf6207faf86aa1731a9ce8443e72f7f0",
    "MinerPKey" : "0394b395e1ef08f9c6e71eb2ecd70fe511f7ec0c0fe5a96c139fd4589b8f8a671c",
    "Balance" : 126999676405,
    "CoinDays" : 0,
    "UpdateHeight" : 45130,
    "CurCoinDays" : 1691,
    "position" : "inblock"
}
#endif



#define ADDR_SEND_A        "dcmCbKbAfKrofNz35MSFupxrx4Uwn3vgjL"  //挂单者A  220700009d05 买家
// 1826-1437

#define ADDR_ACCEPT_B      "dcmWdcfxEjRXUHk8LpygtgDHTpixoo3kbd"  //接单者B  220700000505 卖家
// 1826-1285

#define ADDR_ARBITRATION_C "dcnGLkGud6c5bZJSUghzxvCqV45SJEwRcH"  //仲裁者C   220700003904
// 1826-1081

//const static char DeveloperAccount[6]="\x00\x00\x00\x00\x14\x00";//!<开发者地址ID
//#define ADDR_DeveloperAccount   "dk2NNjraSvquD9b4SQbysVRQeFikA55HLi"   //RegID = "0-20"
#define ADDR_DeveloperAccount   "DsSyKYzYBSgyEggq8o6SVD4DnPzETVbaUe"   //RegID = "86720-1"


//#define ID_strAppRegId  "47301-1"    //脚本应用ID 待填
//#define ID_strAppRegId  "47322-1"    //脚本应用ID 待填  47323
//#define ID_strAppRegId  "47323-1"    //脚本应用ID 待填  47323
#define ID_strAppRegId  "47018-1"    //
//#define HASH_sendhash     "7de1faafc2c9f14be5294f5f2b1082eaf92c7d66da5d42be1016e0988143318d"  //挂单交易hash 待填
static const unsigned char HASH_sendhash[] ={
		0x14,0x96,0xb5,0xc0,0x3e,0xa9,0xa2,0x09,
		0xf3,0x97,0x05,0x3a,0x4d,0x32,0xdc,0x4a,
		0xe6,0x31,0x98,0x5e,0x14,0x8f,0x81,0x01,
		0xbb,0xf0,0x53,0xf7,0x4b,0x00,0x06,0x41
};



//!<仲裁者C的配置信息
#define ARBITER_arbiterMoneyX      (2 * 1000000)      //!<仲裁费用X
#define ARBITER_overtimeMoneyYmax  (1 * 100000000) //!<超时未判决的最大赔偿费用Y
#define ARBITER_configMoneyZ       (1 * 1000000)       //!<无争议裁决费用Z
#define ARBITER_overtimeheightT    (1 * 1440)  //!<判决期限时间T

//!<挂单者的配置信息
#define SEND_moneyM     (2 * 100000000)    //!<交易金额
#define SEND_height     (2 * 1440)       //!<每个交易环节的超时高度

//#define  ARBITER_winnerMoney  (1 * 100000000) //!<裁决后，赢家分配金额
#define  ARBITER_winnerMoney  (198000000) //!<裁决后，赢家分配金额

CGuaranteeTest::CGuaranteeTest():nNum(0), nStep(0), strTxHash(""), strAppRegId(ID_strAppRegId) {

}

TEST_STATE CGuaranteeTest::Run(){


	cout<<"CGuaranteeTest run start"<<endl;
#if 0
    // 注册ipo脚本
	if(!RegistScript()){
		cout<<"CGuaranteeTest RegistScript err"<<endl;
		return end_state;
	}

	/// 等待ipo脚本被确认到block中
	while(true)
	{
		if(WaitComfirmed(strTxHash, strAppRegId)) {
					break;
				}
	}
#endif


//	Recharge();
	Withdraw();
//	WithdrawSomemoney();

//	Register(TX_REGISTER);
//	Register(TX_MODIFYREGISTER);
//	ArbitONOrOFF(TX_ARBIT_ON);
//	ArbitONOrOFF(TX_ARBIT_OFF);
//	UnRegister();
//	SendStartTrade();
//	SendCancelTrade();
//	AcceptTrade();
//    DeliveryTrade();
//	BuyerConfirm();
//	Arbitration();
//	RunFinalResult();
	cout<<"CGuaranteeTest run end"<<endl;
	return end_state;
}


bool CGuaranteeTest::RegistScript(){

	const char* pKey[] = { "cNcJkU44oG3etbWoEvY46i5qWPeE8jVb7K44keXxEQxsXUZ85MKU",
			"cNcJkU44oG3etbWoEvY46i5qWPeE8jVb7K44keXxEQxsXUZ85MKU"};
	int nCount = sizeof(pKey) / sizeof(char*);
	basetest.ImportWalletKey(pKey, nCount);

	string strFileName("guarantee.bin");
//	string strFileName("guarantee.lua");
	int nFee = basetest.GetRandomFee();
	int nCurHight;
	basetest.GetBlockHeight(nCurHight);
	string regAddr="dk2NNjraSvquD9b4SQbysVRQeFikA55HLi";

	//reg anony app
	Value regscript = basetest.RegisterAppTx(regAddr, strFileName, nCurHight, nFee + 1 *COIN);// + 20 *COIN
	if(basetest.GetHashFromCreatedTx(regscript, strTxHash)){

		return true;
	}
	return false;
}


bool CGuaranteeTest::Recharge()
{
   cout<<"Recharge start"<<endl;

   APPACC senddata;
	memset(&senddata,0,sizeof(senddata));
	senddata.systype = 0xff;
	senddata.type = 0x02;

	CDataStream scriptData(SER_DISK, CLIENT_VERSION);
	scriptData << senddata;
	string sendcontract = HexStr(scriptData);
	uint64_t nTempSend = COIN;//20 * COIN;
//	uint64_t nTempSend = (10000 + 5) * COIN;

    cout<<"Recharge data:"<<sendcontract<<endl;
    cout<<"Recharge strAppRegId:"<<strAppRegId<<endl;
	Value  retValue= basetest.CreateContractTx(strAppRegId,ADDR_SEND_A,sendcontract,0,0,nTempSend);
//   ADDR_DeveloperAccount  //RegID = "0-20"
//   ADDR_SEND_A   //	Id = "1826-1437";
//   ADDR_ACCEPT_B   //	Id = "1826-1285";
//	ADDR_ARBITRATION_C  Id = "1826-1081"
	if (basetest.GetHashFromCreatedTx(retValue, strTxHash)) {
		nStep++;
	}else{
		cout<<"Recharge err end"<<endl;
		return false;
	}

#if 1
	/// 等待交易被确认到block中
	while(true)
	{
		if(WaitComfirmed(strTxHash, strAppRegId)) {
					break;
				}
	}
#endif
    cout<<"Recharge success end"<<endl;
	return false;
}

bool CGuaranteeTest::Withdraw()
{
   cout<<"Withdraw start"<<endl;

   APPACC senddata;
	memset(&senddata,0,sizeof(senddata));
	senddata.systype = 0xff;
	senddata.type = 0x01;
	senddata.typeaddr = TX_REGID;

	CDataStream scriptData(SER_DISK, CLIENT_VERSION);
	scriptData << senddata;
	string sendcontract = HexStr(scriptData);
	uint64_t nTempSend = 0;
    cout<<"Withdraw data:"<<sendcontract<<endl;
	Value  retValue= basetest.CreateContractTx(strAppRegId,ADDR_SEND_A,sendcontract,0,0,nTempSend);
           //ADDR_DeveloperAccount  ADDR_ARBITRATION_C
	if (basetest.GetHashFromCreatedTx(retValue, strTxHash)) {
		nStep++;

	}else{
	    cout<<"Withdraw err end"<<endl;
	    return false;
    }

#if 1
/// 等待交易被确认到block中
while(true)
{
	if(WaitComfirmed(strTxHash, strAppRegId)) {
				break;
			}
}
#endif

	 cout<<"Withdraw success end"<<endl;
	return false;
}

bool CGuaranteeTest::WithdrawSomemoney()
{
   cout<<"WithdrawwSomemoney start"<<endl;

   APPACC_money senddata;
	memset(&senddata,0,sizeof(senddata));
	senddata.systype = 0xff;
	senddata.type = 0x03;
	senddata.typeaddr = TX_REGID;
	senddata.money = 10000 * COIN;
//	senddata.money = COIN;

	CDataStream scriptData(SER_DISK, CLIENT_VERSION);
	scriptData << senddata;
	string sendcontract = HexStr(scriptData);
	uint64_t nTempSend = 0;
    cout<<"WithdrawwSomemoney data:"<<sendcontract<<endl;
	Value  retValue= basetest.CreateContractTx(strAppRegId,ADDR_DeveloperAccount,sendcontract,0,0,nTempSend);
           //ADDR_DeveloperAccount  ADDR_ARBITRATION_C
	if (basetest.GetHashFromCreatedTx(retValue, strTxHash)) {
		nStep++;

	}else{
	    cout<<"WithdrawwSomemoney err end"<<endl;
	    return false;
    }

#if 1
	/// 等待交易被确认到block中
	while(true)
	{
		if(WaitComfirmed(strTxHash, strAppRegId)) {
				break;
			}
	}
#endif
	 cout<<"WithdrawwSomemoney success end"<<endl;
	return false;
}

bool CGuaranteeTest::Register(unsigned char type)
{
	TX_REGISTER_CONTRACT senddata;
	memset(&senddata,0,sizeof(senddata));
    if(type == TX_REGISTER)
    {
    	senddata.type = TX_REGISTER;
    	 cout<<"TX_REGISTER start"<<endl;
    }else if(type == TX_MODIFYREGISTER)
    {
    	senddata.type = TX_MODIFYREGISTER;
    	 cout<<"TX_MODIFYREGISTER start"<<endl;
    }
    else{
    	cout<<"Register input err"<<endl;
    	return false;
    }
	senddata.arbiterMoneyX = ARBITER_arbiterMoneyX;
	senddata.overtimeMoneyYmax = ARBITER_overtimeMoneyYmax;
	senddata.configMoneyZ = ARBITER_configMoneyZ; //0x8234567812345678
	senddata.overtimeheightT = ARBITER_overtimeheightT;
    strcpy(senddata.comment,"联系电话: 188888888888");

	CDataStream scriptData(SER_DISK, CLIENT_VERSION);
	scriptData << senddata;
	string sendcontract = HexStr(scriptData);
	uint64_t nTempSend = 0;
    cout<<"Register data:"<<sendcontract<<endl;
    cout<<"Register strAppRegId:"<<strAppRegId.c_str()<<endl;
	Value  retValue= basetest.CreateContractTx(strAppRegId,ADDR_DeveloperAccount,sendcontract,0,0,nTempSend);
	//ADDR_DeveloperAccount  ADDR_ARBITRATION_C

	if (basetest.GetHashFromCreatedTx(retValue, strTxHash)) {
		nStep++;

	}else
	{
	    cout<<"Register fail"<<endl;
	    return false;
	}
#if 1
	/// 等待交易被确认到block中
	while(true)
	{
		if(WaitComfirmed(strTxHash, strAppRegId)) {
					break;
				}
	}
#endif
	 cout<<"Register success"<<endl;
	return true;
}

bool CGuaranteeTest::UnRegister()
{
   cout<<"UnRegister start"<<endl;

   unsigned char senddata = TX_UNREGISTER;

	CDataStream scriptData(SER_DISK, CLIENT_VERSION);
	scriptData << senddata;
	string sendcontract = HexStr(scriptData);
	uint64_t nTempSend = 0;
	cout<<"UnRegister data:"<<sendcontract.c_str()<<endl;
	Value  retValue= basetest.CreateContractTx(strAppRegId,ADDR_ARBITRATION_C,sendcontract,0,0,nTempSend);

	if (basetest.GetHashFromCreatedTx(retValue, strTxHash)) {
		nStep++;

	}
	else{
	    cout<<"UnRegister fail"<<endl;
	    return false;
	}
#if 1
	/// 等待交易被确认到block中
	while(true)
	{
		if(WaitComfirmed(strTxHash, strAppRegId)) {
					break;
				}
	}
#endif

	cout<<"UnRegister success end"<<endl;
	return false;
}
bool CGuaranteeTest::ArbitONOrOFF(unsigned char type)
{
   cout<<"ArbitONOrOFF start"<<endl;

   unsigned char senddata;
   if(type == TX_ARBIT_ON)
   {
	   senddata = TX_ARBIT_ON;
   }else{
	   senddata = TX_ARBIT_OFF;
   }
	CDataStream scriptData(SER_DISK, CLIENT_VERSION);
	scriptData << senddata;
	string sendcontract = HexStr(scriptData);
	uint64_t nTempSend = 0;
	cout<<"ArbitONOrOFF data:"<<sendcontract.c_str()<<endl;
	Value  retValue= basetest.CreateContractTx(strAppRegId,ADDR_ARBITRATION_C,sendcontract,0,0,nTempSend);

	if (basetest.GetHashFromCreatedTx(retValue, strTxHash)) {
		nStep++;
	}
	else{
	    cout<<"ArbitONOrOFF fail"<<endl;
	    return false;
	}
#if 1
	/// 等待交易被确认到block中
	while(true)
	{
		if(WaitComfirmed(strTxHash, strAppRegId)) {
					break;
				}
	}
#endif

	cout<<"ArbitONOrOFF success end"<<endl;
	return false;
}

bool CGuaranteeTest::SendStartTrade()
{
	cout<<"SendStartTrade start"<<endl;

	TX_SNED_CONTRACT senddata;
	memset(&senddata,0,sizeof(senddata));
	senddata.type = TX_SEND;
//	senddata.sendType = SEND_TYPE_BUY;   //待修改 挂单类型
	senddata.sendType = SEND_TYPE_SELL;   //待修改 挂单类型

//	string arbitationID = "47046-1";
	uint32_t height = 1826;   //待填   仲裁者ID arbiterAddr_C
	uint16_t index = 1081;

	memcpy(&senddata.arbitationID[0],&height,4);
    memcpy(&senddata.arbitationID[4],&index,2);
	senddata.moneyM = SEND_moneyM;
	senddata.height = SEND_height;
	strcpy(senddata.goods,"小米3手机");
	strcpy(senddata.comment,"联系电话: 188888888888");

	CDataStream scriptData(SER_DISK, CLIENT_VERSION);
	scriptData << senddata;
	string sendcontract = HexStr(scriptData);
	uint64_t nTempSend = 0;
	cout<<"SendStartTrade data:"<<sendcontract.c_str()<<endl;
	Value sendret;
    if(senddata.sendType == SEND_TYPE_BUY)
    {
    	sendret= basetest.CreateContractTx(strAppRegId,ADDR_SEND_A,sendcontract,0,0,nTempSend); // 待填写
    }else{
    	sendret= basetest.CreateContractTx(strAppRegId,ADDR_ACCEPT_B,sendcontract,0,0,nTempSend); // 待填写
    }

	if (basetest.GetHashFromCreatedTx(sendret, strTxHash)) {
		nStep++;

	}
	else{
		cout<<"SendStartTrade err end"<<endl;
		return false;
	}
#if 1
		/// 等待交易被确认到block中
		while(true)
		{
			if(WaitComfirmed(strTxHash, strAppRegId)) {
						break;
					}
		}
#endif
		cout<<"SendStartTrade success end"<<endl;
	return true;
}

bool CGuaranteeTest::SendCancelTrade()
{
   cout<<"SendCancelTrade start"<<endl;

	TX_CONTRACT senddata;
	memset(&senddata,0,sizeof(senddata));
	senddata.type = TX_CANCEL;
//	memcpy(senddata.txid, uint256S(HASH_sendhash).begin(), sizeof(senddata.txid)); //待填交易HASH
	memcpy(senddata.txid,HASH_sendhash, sizeof(senddata.txid)); //待填交易HASH
	senddata.height = SEND_height;

	CDataStream scriptData(SER_DISK, CLIENT_VERSION);
	scriptData << senddata;
	string sendcontract = HexStr(scriptData);
	uint64_t nTempSend = 0;
	cout<<"SendCancelTrade data:"<<sendcontract.c_str()<<endl;
	Value  sendret= basetest.CreateContractTx(strAppRegId,ADDR_SEND_A,sendcontract,0,0,nTempSend); // 取消挂买单
//    Value  sendret= basetest.CreateContractTx(strAppRegId,ADDR_ACCEPT_B,sendcontract,0,0,nTempSend); //取消挂卖单
	if (basetest.GetHashFromCreatedTx(sendret, strTxHash)) {
		nStep++;
	}
	else{
		cout<<"SendCancelTrade err end"<<endl;
		return false;
	}
#if 1
		/// 等待交易被确认到block中
		while(true)
		{
			if(WaitComfirmed(strTxHash, strAppRegId)) {
						break;
					}
		}
#endif
	cout<<"SendCancelTrade success end"<<endl;
	return true;
}

bool CGuaranteeTest::AcceptTrade()
{
   cout<<"AcceptTrade start"<<endl;

	TX_CONTRACT senddata;
	memset(&senddata,0,sizeof(senddata));
	senddata.type = TX_ACCEPT;
	memcpy(senddata.txid,HASH_sendhash, sizeof(senddata.txid)); //待填交易HASH
	senddata.height = SEND_height;

	CDataStream scriptData(SER_DISK, CLIENT_VERSION);
	scriptData << senddata;
	string sendcontract = HexStr(scriptData);
	uint64_t nTempSend = 0;
	cout<<"AcceptTrade data:"<<sendcontract.c_str()<<endl;
//	Value  sendret= basetest.CreateContractTx(strAppRegId,ADDR_ACCEPT_B,sendcontract,0,0,nTempSend);//卖家接单
	Value  sendret= basetest.CreateContractTx(strAppRegId,ADDR_SEND_A,sendcontract,0,0,nTempSend);//买家接单

	if (basetest.GetHashFromCreatedTx(sendret, strTxHash)) {
		nStep++;
	}
	else{
		cout<<"AcceptTrade err end"<<endl;
		return false;
	}
#if 1
		/// 等待交易被确认到block中
		while(true)
		{
			if(WaitComfirmed(strTxHash, strAppRegId)) {
						break;
					}
		}
#endif
	cout<<"AcceptTrade success end"<<endl;
	return true;
}
bool  CGuaranteeTest::DeliveryTrade(){

	   cout<<"DeliveryTrade start"<<endl;

		TX_CONTRACT senddata;
		memset(&senddata,0,sizeof(senddata));
		senddata.type = TX_DELIVERY;
		memcpy(senddata.txid,HASH_sendhash, sizeof(senddata.txid)); //待填交易HASH
		senddata.height = SEND_height;

		CDataStream scriptData(SER_DISK, CLIENT_VERSION);
		scriptData << senddata;
		string sendcontract = HexStr(scriptData);
		uint64_t nTempSend = 0;
		cout<<"DeliveryTrade data:"<<sendcontract.c_str()<<endl;
		Value  sendret= basetest.CreateContractTx(strAppRegId,ADDR_ACCEPT_B,sendcontract,0,0,nTempSend);//卖家发货

		if (basetest.GetHashFromCreatedTx(sendret, strTxHash)) {
			nStep++;
		}
		else{
			cout<<"DeliveryTrade err end"<<endl;
			return false;
		}
	#if 1
			/// 等待交易被确认到block中
			while(true)
			{
				if(WaitComfirmed(strTxHash, strAppRegId)) {
							break;
						}
			}
	#endif
		cout<<"DeliveryTrade success end"<<endl;
		return true;

}
bool CGuaranteeTest::BuyerConfirm()
{
   cout<<"BuyerConfirm start"<<endl;

	TX_CONTRACT senddata;
	memset(&senddata,0,sizeof(senddata));
	senddata.type = TX_BUYERCONFIRM;
	memcpy(senddata.txid,HASH_sendhash, sizeof(senddata.txid)); //待填交易HASH
	senddata.height = SEND_height;

	CDataStream scriptData(SER_DISK, CLIENT_VERSION);
	scriptData << senddata;
	string sendcontract = HexStr(scriptData);
	uint64_t nTempSend = 0;
	cout<<"BuyerConfirm data:"<<sendcontract.c_str()<<endl;
	Value  sendret= basetest.CreateContractTx(strAppRegId,ADDR_SEND_A,sendcontract,0,0,nTempSend); //待填写

	if (basetest.GetHashFromCreatedTx(sendret, strTxHash)) {
		nStep++;
	}
	else{
		cout<<"BuyerConfirm err end"<<endl;
		return false;
	}
#if 1
		/// 等待交易被确认到block中
		while(true)
		{
			if(WaitComfirmed(strTxHash, strAppRegId)) {
						break;
					}
		}
#endif
	cout<<"BuyerConfirm success end"<<endl;
	return true;
}


bool CGuaranteeTest::Arbitration()
{
   cout<<"Arbitration start"<<endl;

   TX_Arbitration senddata;
	memset(&senddata,0,sizeof(senddata));
	senddata.type = TX_ARBITRATION;
	memcpy(senddata.txid,HASH_sendhash, sizeof(senddata.txid)); //待填交易HASH
	senddata.height = SEND_height;
//	string arbitationID = "47046-1";
	uint32_t height = 1826;   //待填   仲裁者ID arbiterAddr_C
	uint16_t index = 1081;

	memcpy(&senddata.arbitationID[0],&height,4);
	memcpy(&senddata.arbitationID[4],&index,2);


	CDataStream scriptData(SER_DISK, CLIENT_VERSION);
	scriptData << senddata;
	string sendcontract = HexStr(scriptData);
	uint64_t nTempSend = 0;
	cout<<"Arbitration data:"<<sendcontract.c_str()<<endl;
	Value  sendret= basetest.CreateContractTx(strAppRegId,ADDR_SEND_A,sendcontract,0,0,nTempSend);// 待填写 ADDR_ARBITRATION_C

	if (basetest.GetHashFromCreatedTx(sendret, strTxHash)) {
		nStep++;

	}
	else{
		cout<<"Arbitration err end"<<endl;
		return false;
	}
#if 1
		/// 等待交易被确认到block中
		while(true)
		{
			if(WaitComfirmed(strTxHash, strAppRegId)) {
						break;
					}
		}
#endif
	cout<<"Arbitration success end"<<endl;
	return true;
}
bool CGuaranteeTest::RunFinalResult()
{
   cout<<"RunFinalResult start"<<endl;

	TX_FINALRESULT_CONTRACT senddata;
	memset(&senddata,0,sizeof(senddata));
	senddata.type = TX_FINALRESULT;
	memcpy(senddata.arbitHash, HASH_sendhash, sizeof(senddata.arbitHash)); //待填交易HASH
	senddata.overtimeheightT = ARBITER_overtimeheightT;

//	string buyId = "1826-1437";
	uint32_t height = 1826;   //待填   赢家 ID 买家 赢
	uint16_t index = 1285;  //1437

    memcpy(&senddata.winner[0],&height,4);
    memcpy(&senddata.winner[4],&index,2);
	senddata.winnerMoney = ARBITER_winnerMoney;

//	string sellerId = "1826-1285";
	height = 1826;  //待填   输家ID
	index = 1437;  //1285
    memcpy(&senddata.loser[0],&height,4);
    memcpy(&senddata.loser[4],&index,2);
	senddata.loserMoney = SEND_moneyM - ARBITER_winnerMoney;  //  交易金额M - 赢家分配的钱  待填写

	CDataStream scriptData(SER_DISK, CLIENT_VERSION);
	scriptData << senddata;
	string sendcontract = HexStr(scriptData);
	uint64_t nTempSend = 0;
	cout<<"RunFinalResult data:"<<sendcontract.c_str()<<endl;
	Value  sendret= basetest.CreateContractTx(strAppRegId,ADDR_ARBITRATION_C,sendcontract,0,0,nTempSend);//ADDR_ARBITRATION_C

	if (basetest.GetHashFromCreatedTx(sendret, strTxHash)) {
		nStep++;

	}
	else{
		cout<<"RunFinalResult err end"<<endl;
		return false;
	}
#if 1
		/// 等待交易被确认到block中
		while(true)
		{
			if(WaitComfirmed(strTxHash, strAppRegId)) {
						break;
					}
		}
#endif
	cout<<"RunFinalResult success end"<<endl;
	return true;
}

BOOST_FIXTURE_TEST_SUITE(CGuaranteeTxTest,CGuaranteeTest)

BOOST_FIXTURE_TEST_CASE(Test,CGuaranteeTest)
{
	Run();
}
BOOST_AUTO_TEST_SUITE_END()

