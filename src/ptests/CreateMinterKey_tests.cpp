// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <stdlib.h>
#include <time.h>
#include "main.h"
#include "init.h"
#include "miner/miner.h"
#include "commons/uint256.h"
#include "commons/util.h"
#include <boost/test/unit_test.hpp>
#include "../rpc/core/rpcclient.h"
#include "tx/tx.h"
#include "wallet/wallet.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include "../vm/script.h"
#include "rpc/core/rpcserver.h"
#include "../test/systestbase.h"
#include <boost/algorithm/string/predicate.hpp>
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_reader.h"
#include "json/json_spirit_writer.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_stream_reader.h"
#include "../tx.h"
#include <boost/test/included/unit_test.hpp>

#include <boost/test/parameterized_test.hpp>

using namespace boost::unit_test;
using namespace std;
using namespace boost;


#include "CreateMinterKey_tests.h"


const uint64_t sendMoney = 20000 * COIN;
bool CCreateMinerkey::SelectAccounts() {
	const char *argv[] = { "rpctest", "listaddr"};
	int argc = sizeof(argv) / sizeof(char*);

	Value value;
	if (CommandLineRPC_GetValue(argc, argv, value)) {
		if (value.type() == null_type) {
			return false;
		}
		for(auto & item : value.get_array()) {
			const Value& balance = find_value(item.get_obj(), "balance");
			if(balance.get_real() > 10000000.0) {
				const Value& regId = find_value(item.get_obj(), "regid");
				if("" != regId.get_str() && " "!=regId.get_str())
					vAccount.push_back(regId.get_str());
			}
		}
	}
	return true;
}

string CCreateMinerkey::GetOneAccount() {
	for(auto &item : vAccount) {
		if(GetBalance(item) > 1000000 * COIN) {
			mapSendValue[item] += sendMoney;
			if(mapSendValue[item] > 200000000 * COIN)
				continue;
			return item;
		}
	}
	return "";
}

void CCreateMinerkey::CreateAccount() {
//	if(2 == argc){
//		const char* newArgv[] = {argv[0], argv[2] };
//		CBaseParams::InitializeParams(2, newArgv);
//	}
	if(!SelectAccounts())
		return;
	std::string txid("");
	const int nNewAddrs = 2000;
	string hash = "";
	vector<string> vNewAddress;
//	string strAddress[] = {"0-1","0-2","0-3","0-4","0-5"};

	for (int i = 0; i < nNewAddrs; i++) {
		string newaddr;
		BOOST_CHECK(GetNewAddr(newaddr, true));
		vNewAddress.push_back(newaddr);
		string srcAcct = GetOneAccount();
		if("" == srcAcct) {
			cout << "Get source acct failed" << endl;
			return;
		}
		Value value = CreateNormalTx( srcAcct, newaddr, sendMoney);
		BOOST_CHECK(GetHashFromCreatedTx(value, hash));
	}
	int size = 0 ;
	GenerateOneBlock();

	while (1) {
		if (!GetMemPoolSize(size)) {
			cout << "GetMemPoolSize error" << endl;
			return;
		}
		if (size > 0) {
			cout << "GetMemPoolSize size :" << size << endl;
			MilliSleep(100);
		} else {
			break;
		}
	}

	for (size_t i = 0; i < vNewAddress.size(); i++) {
		int nfee = GetRandomFee();
		Value value1 = RegisterAccountTx(vNewAddress[i], nfee);
		BOOST_CHECK(GetHashFromCreatedTx(value1,hash));
	}

	GenerateOneBlock();
	while (1) {
		if (!GetMemPoolSize(size)) {
			cout << "GetMemPoolSize error" << endl;
			return;
		}
		if (size > 0) {
			cout << "GetMemPoolSize size :" << size << endl;
			MilliSleep(100);
		} else {
			break;
		}
	}

	cout << "all ok  "  <<  endl;
}

CCreateMinerkey::~CCreateMinerkey() {
	// LEARN Auto-generated destructor stub
}





BOOST_FIXTURE_TEST_SUITE(CreateAccount, CCreateMinerkey)

BOOST_FIXTURE_TEST_CASE(create, CCreateMinerkey)
{
	CreateAccount();
}

BOOST_AUTO_TEST_SUITE_END()
