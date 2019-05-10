// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_TEST_MODULE Coin Test Suite


#include "main.h"
#include "ui_interface.h"
#include "util.h"


#include "wallet/wallet.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/test/unit_test.hpp>
#include "systestbase.h"
//CWallet* pwalletMain;


extern void noui_connect();

struct TestingSetup {
	TestingSetup() {
		int argc = 2;
		string param1("coind.exe");
		string param2("");

		bool bSetDataDir(false);
		for (int i = 1; i < boost::unit_test::framework::master_test_suite().argc; ++i) {
			string strArgv = boost::unit_test::framework::master_test_suite().argv[i];
			if (string::npos != strArgv.find("-datadir=")) {
				param1 = boost::unit_test::framework::master_test_suite().argv[0];
				param2 = strArgv.c_str();
				bSetDataDir = true;
				break;
			}
		}
		if (!bSetDataDir) {
			char findchar;
#ifdef WIN32
			findchar = '\\';
#else
			findchar = '/';
#endif

			string strCurDir = boost::filesystem::initial_path<boost::filesystem::path>().string();
			int index = strCurDir.find_last_of(findchar);
			int count = 3;
			while (count--) {
				index = strCurDir.find_last_of(findchar);
				strCurDir = strCurDir.substr(0, index);

			}
#ifdef WIN32
			strCurDir += "\\coin_test";
			param2 = "-datadir=";
			param2 += strCurDir;

#else
			strCurDir +="/coin_test";
			param2 = "-datadir=";
			param2 +=strCurDir;
#endif
		}
		const char* argv[] = { param1.c_str(), param2.c_str()};
		SysTestBase::StartServer(argc, argv);

		}
		~TestingSetup()
		{
			SysTestBase::StopServer();
		}

//        boost::filesystem::remove_all(pathTemp);

};

BOOST_GLOBAL_FIXTURE(TestingSetup);

//void Shutdown(void* parg)
//{
//  exit(0);
//}
//
//void StartShutdown()
//{
//  exit(0);
//}

//bool ShutdownRequested()
//{
//  return false;
//}

