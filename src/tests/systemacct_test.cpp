#ifdef TODO

#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "systestbase.h"
using namespace std;
using namespace boost;

class CSysAccountTest:public SysTestBase {
public:

	bool RegisterAccount(const string& strAddr, uint64_t nFee,string& strTxHash,bool bSign = false) {
		Value value = RegisterAccountTx(strAddr,nFee);
		return GetHashFromCreatedTx(value,strTxHash);
	}

	bool SendMoney(const string& strRegAddr, const string& strDestAddr, uint64_t nMoney, uint64_t nFee = 0) {
		Value value = CreateNormalTx(strRegAddr,strDestAddr,nMoney);
		string hash = "";
		return GetHashFromCreatedTx(value,hash);
	}

};

BOOST_FIXTURE_TEST_SUITE(sysacct_test, CSysAccountTest)
BOOST_FIXTURE_TEST_CASE(transfer_test, CSysAccountTest)
{
	ResetEnv();
	BOOST_CHECK(0==chainActive.Height());
	//转账
	string strRegAddr = "t7yXQfSAzsypLJvwnhuehAp3nbKYhv27qW";
	string strSrcRegID = "000000000100";
	uint64_t nMoney = 10000000;
	BOOST_CHECK(SendMoney(strSrcRegID,strRegAddr,nMoney));
	BOOST_CHECK(GenerateOneBlock());

	//确认转账成功
	uint64_t nFreeMoney = GetBalance(strRegAddr);
	BOOST_CHECK(nFreeMoney == nMoney);

	//使用一个没有注册过的keyID地址测试，看是能够注册，用getaccountinfo 检查regid是否存在
	string strTxHash;
	CRegID regId;
	BOOST_CHECK(RegisterAccount(strRegAddr,10000,strTxHash));
	BOOST_CHECK(GenerateOneBlock());
	BOOST_CHECK(GetRegID(strRegAddr,regId));
	BOOST_CHECK(false == regId.IsEmpty());

	//使用一个注册过的keyID地址测试，看是否能够注册成功
	string strResigterd("tQCmxQDFQdHmAZw1j3dteB4CTro2Ph5TYP");
	BOOST_CHECK(!RegisterAccount(strRegAddr,10000,strTxHash));

	//用getnewaddr获取一个没有设置挖坑的公钥地址，注册账户最后一个参数是true，看此地址是否能够注册成功
	string strNewKeyID;
	BOOST_CHECK(GetNewAddr(strNewKeyID,true));

	//用一个注册的地址和一个未注册的地址来发一个交易，看是否能够发送成功
	string strUnRegister("tNGvponTbhkomLUkMVHXQFtZ4Sho8wUonE");
	vector<unsigned char> vRegID = regId.GetRegIdRaw();
	nMoney = nMoney/10;
	BOOST_CHECK(SendMoney(HexStr(vRegID),strUnRegister,nMoney, 10000));
	BOOST_CHECK(GenerateOneBlock());

	//确认转账成功
	nFreeMoney = GetBalance(strUnRegister);
	BOOST_CHECK(nFreeMoney == nMoney);
}

BOOST_FIXTURE_TEST_CASE(register_test,CSysAccountTest)
 {
	ResetEnv();
	BOOST_CHECK(0==chainActive.Height());
	string strTxHash;
	CRegID regId;
	string strRegAddr1("tNGvponTbhkomLUkMVHXQFtZ4Sho8wUonE");
	string strRegAddr2 = "";
	BOOST_CHECK(GetNewAddr(strRegAddr2,false));
	uint64_t nFee = 10000;
	vector<string> vFailedTxHash;

	//余额不够
	BOOST_CHECK(!RegisterAccount(strRegAddr1, nFee, strTxHash));
	vFailedTxHash.push_back(strTxHash);

	//无法读取的账号地址
	string strInvalidAddr("fjsofeoifdsfdsfdsafafafafafafafa");
	BOOST_CHECK(!RegisterAccount(strInvalidAddr, nFee, strTxHash));
	vFailedTxHash.push_back(strTxHash);

//	//不签名
//	BOOST_CHECK(!RegisterAccount(strRegAddr1, nFee, strTxHash,nIvalid_height,false));
//	vFailedTxHash.push_back(strTxHash);

	//手续费超过最大值
	BOOST_CHECK(!RegisterAccount(strRegAddr1, nFee+GetBaseCoinMaxMoney(), strTxHash));
	vFailedTxHash.push_back(strTxHash);

	//重复注册的地址
	string strReRegisrerAddr("tQCmxQDFQdHmAZw1j3dteB4CTro2Ph5TYP");
	BOOST_CHECK(!RegisterAccount(strReRegisrerAddr, nFee, strTxHash));
	vFailedTxHash.push_back(strTxHash);

	//账户上没有余额的未注册地址
	BOOST_CHECK(!RegisterAccount(strRegAddr2, nFee, strTxHash));
	vFailedTxHash.push_back(strTxHash);

//	//检查失败的交易是否在memorypool中
//	for (const auto& item : vFailedTxHash) {
//		uint256 txid(item);
//		BOOST_CHECK(!IsTxInMemorypool(txid));
//		BOOST_CHECK(!IsTxUnConfirmdInWallet(txid));
//	}

	//给没有余额的账户转账
	string strSrcRegID = "000000000100";
	uint64_t nMoney = 10000000;
	BOOST_CHECK(SendMoney(strSrcRegID, strRegAddr2, nMoney));
	BOOST_CHECK(GenerateOneBlock());

	//确认转账成功
	uint64_t nFreeMoney = GetBalance(strRegAddr2);
	BOOST_CHECK(nFreeMoney == nMoney);

	//再次检查失败的交易是否在memorypool中
	for (const auto& item : vFailedTxHash) {
		uint256 txid(uint256S(item));
		BOOST_CHECK(!IsTxInMemorypool(txid));
		BOOST_CHECK(!IsTxUnConfirmdInWallet(txid));
	}

	//给地址为：dkJwhBs2P2SjbQWt5Bz6vzjqUhXTymvsGr 的账户转账
	BOOST_CHECK(SendMoney(strSrcRegID, strRegAddr1, nMoney));
	BOOST_CHECK(GenerateOneBlock());

	//确认转账成功
	nFreeMoney = GetBalance(strRegAddr1);
	BOOST_CHECK(nFreeMoney == nMoney);

	string strSpecial;
	BOOST_CHECK(RegisterAccount(strRegAddr1, nFee, strSpecial,false));
	BOOST_CHECK(IsTxInMemorypool(uint256(uint256S(strSpecial))));
	BOOST_CHECK(IsTxUnConfirmdInWallet(uint256(uint256S(strSpecial))));

	//交易已经在memorypool中
	BOOST_CHECK(!RegisterAccount(strRegAddr1, nFee, strTxHash,false));
	BOOST_CHECK(!IsTxInMemorypool(uint256(uint256S(strTxHash))));
	BOOST_CHECK(!IsTxUnConfirmdInWallet(uint256(uint256S(strTxHash))));

	BOOST_CHECK(GenerateOneBlock());

	//确认注册成功的交易在tip中
	BOOST_CHECK(IsTxInTipBlock(uint256(uint256S(strSpecial))));

	vector<CAccount> vLog;
	BOOST_CHECK(GetTxOperateLog(uint256(uint256S(strSpecial)),vLog));

	//检查日志记录是否正确
//	BOOST_CHECK(1 == vLog.size() && 1 == vLog[0].vOperFund.size() && 1 == vLog[0].vOperFund[0].vFund.size());
	BOOST_CHECK(strRegAddr1 == vLog[0].keyid.ToAddress());
//	BOOST_CHECK(vLog[0].vOperFund[0].operType == MINUS_BCOIN && vLog[0].vOperFund[0].vFund[0].value == nFee);
}

BOOST_AUTO_TEST_SUITE_END()

#endif//TODO