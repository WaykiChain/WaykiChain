#include <stdlib.h>
#include <time.h>
#include "main.h"
#include "init.h"
#include "miner.h"
#include "uint256.h"
#include "util.h"
#include <boost/test/unit_test.hpp>
#include "rpc/rpcclient.h"
#include "tx/tx.h"
#include "wallet/wallet.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include "vm/script.h"
#include "rpc/rpcserver.h"
#include "noui.h"
#include "ui_interface.h"
#include "systestbase.h"
#include <boost/algorithm/string/predicate.hpp>

using namespace std;
using namespace boost;

#define ACCOUNT_ID_SIZE 6
#define MAX_ACCOUNT_LEN 20
#pragma pack(1)
typedef struct tag_INT64 {
	unsigned char data[8];
} Int64;
typedef struct tagACCOUNT_ID
{
	char accounid[MAX_ACCOUNT_LEN];
}ACCOUNT_ID;
typedef struct {
	unsigned char nType;
	ACCOUNT_ID vregID[3];
	long nHeight;
	Int64 nPay;
} CONTRACT_DATA;
#pragma pack()

class CSystemTest:public SysTestBase
{
public:
	enum
	{
		ID1_FREE_TO_ID2_FREE = 1,
		ID2_FREE_TO_ID3_FREE,
		ID3_FREE_TO_ID3_SELF,
		ID3_SELF_TO_ID2_FREE,
		ID3_FREE_TO_ID2_FREE,
		UNDEFINED_OPER
	};
	CSystemTest() {
		nOldBlockHeight = 0;
		nNewBlockHeight = 0;
		nTimeOutHeight = 100;
		nOldMoney = 0;
		nNewMoney = 0;
		strFileName = "unit_test.bin";
		strAddr1 = "dsjkLDFfhenmx2JkFMdtJ22TYDvSGgmJem";
	}

	~CSystemTest(){

	}

public:

	bool IsTxConfirmdInWallet(int nBlockHeight,const uint256& txHash)
	{
		string hash ="";
		if (!SysTestBase::GetBlockHash(nBlockHeight, hash)) {
			return false;
		}

		uint256 blockHash(uint256S(hash));
		auto itAccountTx = pwalletMain->mapInBlockTx.find(blockHash);
		if (pwalletMain->mapInBlockTx.end() == itAccountTx)
			return false;

		for (const auto &item :itAccountTx->second.mapAccountTx) {
			if (txHash == item.first) {
				return true;
			}
		}
		return false;
	}

	bool GetTxIndexInBlock(const uint256& txHash, int& nIndex) {
		CBlockIndex* pindex = chainActive.Tip();
		CBlock block;
		if (!ReadBlockFromDisk(block, pindex))
			return false;

		block.BuildMerkleTree();
		std::tuple<bool,int> ret = block.GetTxIndex(txHash);
		if (!std::get<0>(ret)) {
			return false;
		}

		nIndex = std::get<1>(ret);
		return true;
	}

	bool GetRegScript(map<string, string>& mapRegScript) {
		CRegID regId;
		vector<unsigned char> vScript;

		if (pScriptDBTip == nullptr)
			return false;

		assert(pScriptDBTip->Flush());

		int nCount(0);
		if (!pScriptDBTip->GetScriptCount(nCount))
			return false;

		if (!pScriptDBTip->GetScript(0, regId, vScript))
			return false;

		string strRegID = HexStr(regId.GetVec6());
		string strScript = HexStr(vScript.begin(), vScript.end());
		mapRegScript.insert(make_pair(strRegID, strScript));

		while (pScriptDBTip->GetScript(1, regId, vScript)) {
			strRegID = HexStr(regId.GetVec6());
			strScript = HexStr(vScript.begin(), vScript.end());
			mapRegScript.insert(make_pair(strRegID, strScript));
		}

		return true;
	}

	bool CheckRegScript(const string& strRegID,const string& strPath) {
		map<string, string> mapRegScript;
		if (!GetRegScript(mapRegScript)) {
			return false;
		}

		string strFileData;
		if (!GetFileData(strPath,strFileData)) {
			return false;
		}

		for (const auto& item:mapRegScript) {
			if (strRegID == item.first) {
				if (strFileData == item.second) {
					return true;
				}
			}
		}

		return false;
	}

	bool GetFileData(const string& strFilePath, string& strFileData) {
		FILE* file = fopen(strFilePath.c_str(), "rb+");
		if (!file) {
			return false;
		}

		unsigned long lSize;
		fseek(file, 0, SEEK_END);
		lSize = ftell(file);
		rewind(file);

		// allocate memory to contain the whole file:
		char *buffer = (char*) malloc(sizeof(char) * lSize);
		if (buffer == NULL) {
			return false;
		}

		if (fread(buffer, 1, lSize, file) != lSize) {
			if (buffer)
				free(buffer);
			throw runtime_error("read script file error");
		}

		CVmScript vmScript;
		vmScript.GetRom().insert(vmScript.GetRom().end(), buffer, buffer + lSize);
		string desp("this is description");
		vmScript.memo.assign(desp.begin(), desp.end());
		CDataStream ds(SER_DISK, CLIENT_VERSION);
		ds << vmScript;

		vector<unsigned char> vscript;
		vscript.assign(ds.begin(), ds.end());

		if (file)
			fclose(file);
		if (buffer)
			free(buffer);

		strFileData = HexStr(vscript);
		return true;
	}

	bool IsScriptAccCreatedEx(const uint256& txHash,int nConfirmHeight) {
		int nIndex = 0;
		if (!GetTxIndexInBlock(uint256(uint256S(strTxHash)), nIndex)) {
			return false;
		}

		CRegID regID(nConfirmHeight, nIndex);
		return IsScriptAccCreated(HexStr(regID.GetVec6()));
	}

protected:
	int nOldBlockHeight;
	int nNewBlockHeight;
	int nTimeOutHeight;
	static const int nFee = 1*COIN + 100000;
	uint64_t nOldMoney;
	uint64_t nNewMoney;
	string strTxHash;
	string strFileName;
	string strAddr1;
};
/*
 * 测试脚本账户一切在系统中的流程
 * 暂时无法测试，需要unit_test.bin
 */
BOOST_FIXTURE_TEST_SUITE(system_test,CSystemTest)
BOOST_FIXTURE_TEST_CASE(acct_process,CSystemTest)
{
	ResetEnv();
	vector<map<int,string> >vDataInfo;
	vector<CAccountLog> vLog;
	for (int i = 0; i < nTimeOutHeight; i++) {
		//0:产生注册脚本交易
		Value valueRes = RegisterContractTx(strAddr1,strFileName , nTimeOutHeight, nFee);
		BOOST_CHECK(GetHashFromCreatedTx(valueRes,strTxHash));

		//1:挖矿
		nOldMoney = GetBalance(strAddr1);
		BOOST_CHECK(GenerateOneBlock());
		SysTestBase::GetBlockHeight(nNewBlockHeight);

		//2:确认钱已经扣除
		nNewMoney = GetBalance(strAddr1);
		BOOST_CHECK(nNewMoney == nOldMoney - nFee);

		//3:确认脚本账号已经生成
		int nIndex = 0;
		BOOST_CHECK(GetTxIndexInBlock(uint256(uint256S(strTxHash)), nIndex));
		CRegID regID(nNewBlockHeight, nIndex);
		BOOST_CHECK(IsScriptAccCreated(HexStr(regID.GetVec6())));

		//4:检查钱包里的已确认交易里是否有此笔交易
		BOOST_CHECK(IsTxConfirmdInWallet(nNewBlockHeight, uint256(uint256S(strTxHash))));

		//5:通过listregscript 获取相关信息，一一核对，看是否和输入的一致
		string strPath = SysCfg().GetDefaultTestDataPath() + strFileName;
		BOOST_CHECK(CheckRegScript(HexStr(regID.GetVec6()), strPath));

		//6:Gettxoperationlog 获取交易log，查看是否正确
		BOOST_CHECK(GetTxOperateLog(uint256(uint256S(strTxHash)), vLog));
//		BOOST_CHECK(1 == vLog.size() && 1 == vLog[0].vOperFund.size() && 1 == vLog[0].vOperFund[0].vFund.size());
		BOOST_CHECK(strAddr1 == vLog[0].keyID.ToAddress());
//		BOOST_CHECK(vLog[0].vOperFund[0].operType == MINUS_FREE && vLog[0].vOperFund[0].vFund[0].value == nFee);

		map<int,string> mapData;
		mapData.insert(make_pair(nIndex,strTxHash));
		vDataInfo.push_back(mapData);
		ShowProgress("acct_process progress: ",(int)(((i+1)/(float)100) * 100));

	}

	for(int i = vDataInfo.size()-1;i>=0;i--) {
		map<int,string> mapData = vDataInfo[i];
		BOOST_CHECK(1 == mapData.size());

//		int nTxIndex = mapData.begin()->first;
		string strTxHash = mapData.begin()->second;
		uint256 txHash(uint256S(strTxHash));

		SysTestBase::GetBlockHeight(nOldBlockHeight);
		nOldMoney = GetBalance(strAddr1);

		//8:回滚
		BOOST_CHECK(DisConnectBlock(1));

		//9.1:检查账户手续费是否回退
		nNewMoney = GetBalance(strAddr1);
		SysTestBase::GetBlockHeight(nNewBlockHeight);
		BOOST_CHECK(nOldBlockHeight - 1 == nNewBlockHeight);
		BOOST_CHECK(nNewMoney-nFee == nOldMoney);

		//9.2:检测脚本账户是否删除
		CRegID regID(nOldBlockHeight, mapData.begin()->first);
		BOOST_CHECK(!IsScriptAccCreated(HexStr(regID.GetVec6())));

		//9.3:交易是否已经已经放到钱包的未确认交易里
		BOOST_CHECK(IsTxUnConfirmdInWallet(txHash));

		//9.4:检查交易是否在mempool里
		BOOST_CHECK(IsTxInMemorypool(txHash));

		//9.5:检查operationlog 是否可以重新获取
		BOOST_CHECK(!GetTxOperateLog(txHash, vLog));
	}

	//清空环境
	ResetEnv();
	SysTestBase::GetBlockHeight(nNewBlockHeight);
	BOOST_CHECK(0 == nNewBlockHeight);
}

BOOST_AUTO_TEST_SUITE_END()
