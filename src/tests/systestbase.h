/*
 * CRPCRequest2.h
 *
 *  Created on: Dec 9, 2014
 *      Author: ranger.shi
 */

#ifndef CRPCREQUEST_H_
#define CRPCREQUEST_H_

#include <stdlib.h>
#include <time.h>
#include "main.h"
#include "init.h"
#include "miner/miner.h"
#include "commons/uint256.h"
#include "commons/util.h"
#include <boost/test/unit_test.hpp>
#include "rpc/rpcclient.h"
#include "tx/tx.h"
#include "wallet/wallet.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include "vm/luavm/script.h"
#include "rpc/core/rpcserver.h"
#include <boost/algorithm/string/predicate.hpp>

using namespace std;
using namespace boost;
struct AccState {
	int64_t dUnmatureMoney;
	int64_t dFreeMoney;
	int64_t dFrozenMoney;
	AccState() {
		dUnmatureMoney = 0;
		dFreeMoney = 0;
		dFrozenMoney = 0;
	}
	AccState(int64_t a, int64_t b, int64_t c) {
		dUnmatureMoney = a;
		dFreeMoney = b;
		dFrozenMoney = c;
	}

	bool operator==(AccState &a) {
		if (dUnmatureMoney == a.dUnmatureMoney && dFreeMoney == a.dFreeMoney && dFrozenMoney == a.dFrozenMoney) {
			return true;
		}
		return false;
	}

	bool SumEqual(AccState &a) {
		int64_t l = dUnmatureMoney + dFreeMoney + dFrozenMoney;
		int64_t r = a.dUnmatureMoney + a.dFreeMoney + a.dFrozenMoney;

		return l == r;
	}

	AccState& operator+=(AccState &a) {
		dUnmatureMoney += a.dUnmatureMoney;
		dFreeMoney += a.dFreeMoney;
		dFrozenMoney += a.dFrozenMoney;
		return *this;
	}
};

struct AccOperLog {
	const static int nFrozenHeight = 100;
	const static int nMatureHeight = 100;
	std::map<int, AccState> mapAccState;
	AccOperLog() {
		mapAccState.clear();
	}
	bool Add(int &height, AccState &accstate) {
		mapAccState[height] = accstate;
		return true;
	}

	void MergeAcc(int height) {
		for (auto &item : mapAccState) {
			if (height > item.first + nFrozenHeight) {
				item.second.dFreeMoney += item.second.dFrozenMoney;
				item.second.dFrozenMoney = 0.0;
			}

			if (height > item.first + nMatureHeight) {
				item.second.dFreeMoney += item.second.dUnmatureMoney;
				item.second.dUnmatureMoney = 0.0;
			}
		}
	}
};

class SysTestBase {
protected:

	int GetRandomMoney();

public:
	SysTestBase();

	~SysTestBase();

	Value GetAccountInfo(const string& strID);

	static void StartServer(int argc,const char* argv[]);

	static void StopServer();

	int GetRandomFee();

	bool GetMemPoolSize(int &size);

	bool ImportAllPrivateKey();

	bool GetHashFromCreatedTx(const Value& valueRes,string& strHash)
	{
		if (valueRes.type() == null_type) {
			return false;
		}

		const Value& result = find_value(valueRes.get_obj(), "txid");
		const Value& result1 = find_value(valueRes.get_obj(), "rawtx");
		const Value& result3 = find_value(valueRes.get_obj(), "script");
		if (result.type() == null_type && result1.type() == null_type && result3.type() == null_type){
			return false;
		}
		if (result.type() != null_type){
			strHash = result.get_str();
			}
		else if(result1.type() != null_type)
		{
			strHash = result1.get_str();
		}else if(result3.type() != null_type)
		{
			strHash = result3.get_str();
		}

		return true;
	}

	bool GetTxConfirmedRegID(const string& haseh,string& strRegID)
	{
		const char *argv[] = { "rpctest", "getcontractregid", (char*) haseh.c_str() };

		Value value;
		if (!CommandLineRPC_GetValue(sizeof(argv) / sizeof(argv[0]), argv, value)) {
			return false;
		}

		if (value.type() == null_type) {
			//cout<<write_string(value, true)<<endl;
			return false;
		}

		const Value& result = find_value(value.get_obj(), "script");
		if (result.type() == null_type){
			//cout<<write_string(value, true)<<endl;
			return false;
		}
		strRegID = result.get_str();
		return true;
	}

	Value CreateRegAppTx(const string& strAddress, const string& strScript, bool bRigsterScript, int nFee,
			int height);

	bool ResetEnv();

	static bool CommandLineRPC_GetValue(int argc, const char *argv[], Value &value);

	bool IsScriptAccCreated(const string& strScript);

	uint64_t GetBalance(const string& strID);

	bool GetNewAddr(std::string &addr,bool flag);

	bool GetBlockHeight(int &height);

	Value CreateDelegateTx(const string& strAddress, const string &operVoteFund, const uint64_t fee);

	Value CreateNormalTx(const std::string &srcAddr, const std::string &desAddr,uint64_t nMoney);

	Value CreateNormalTx(const std::string &desAddr,uint64_t nMoney);

	Value RegisterAccountTx(const std::string &addr, const int nfee =0);

	Value CallContractTx(const std::string &scriptid, const std::string &addrs, const std::string &contract,
			int height,int nFee = 0,uint64_t nMoney = 0);

	Value DeployContractTx(const string& strAddress, const string& strScript, int height, int nFee = 100000000);

	Value SignSecureTx(const string &securetx);

	bool IsAllTxInBlock();

	bool GetBlockHash(const int height, std::string &blockhash);

	bool GetBlockMinerAddr(const std::string &blockhash, std::string &addr);

	bool GenerateOneBlock();

	bool SetAddrGenerteBlock(const char *addr);

	bool DisConnectBlock(int nNum);

	bool GetStrFromObj(const Value& valueRes,string& str);

	bool ImportWalletKey(const char**address,int nCount);

	bool ShowProgress(string const &msg,float rate)
    {
    	for(int j=0; j<100 ;++j)
    	cout<<'\b';
    	cout << msg <<  rate << "%";
    	return true;
    }


	bool ShowProgressTotal(string const &msg,int rate)
    {
    	for(int j=0; j<100 ;++j)
    	cout<<'\b';
    	cout << msg << " Total: " << rate;
    	return true;
    }

	uint64_t GetRandomBetfee();

	bool GetKeyId(string const &addr,CKeyID &KeyId);

	bool IsTxInMemorypool(const uint256& txid);

	bool IsTxUnConfirmdInWallet(const uint256& txid) ;

	bool IsTxInTipBlock(const uint256& txid);

	bool GetRegID(string& strAddr,CRegID& regId);

	bool GetRegID(string& strAddr,string& regId);

	bool GetTxOperateLog(const uint256& txid, vector<CAccount>& vLog) ;

	bool PrintLog();

	bool IsMemoryPoolEmpty();

	Value GetContractAccountInfo(const string& scriptId,const string& strAddr);
protected:
	static boost::thread* pThreadShutdown ;
	std::map<string, AccState> mapAccState;
	std::map<string, AccOperLog> mapAccOperLog;
	int nCurHeight;
	int64_t nCurMoney;
	int64_t nCurFee;
};

#endif /* CRPCREQUEST2_H_ */
