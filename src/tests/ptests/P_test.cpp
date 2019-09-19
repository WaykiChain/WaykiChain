// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_TEST_MODULE Process Test Suite

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <string>
#include "config/chainparams.h"
#include "main.h"
#include "commons/util.h"
#include "wallet/wallet.h"

using namespace std;

struct TestingSetup {
	TestingSetup() {
			bool bSetDataDir(false);
			for (int i = 1; i < boost::unit_test::framework::master_test_suite().argc; ++i) {
				string strArgv = boost::unit_test::framework::master_test_suite().argv[i];
				if (string::npos != strArgv.find("-datadir=")) {
					const char* newArgv[] = { boost::unit_test::framework::master_test_suite().argv[0], strArgv.c_str() };
					CBaseParams::InitializeParams(2, newArgv);
					bSetDataDir = true;
					break;
				}
			}
			if (!bSetDataDir) {
				int argc = 2;
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
							string param = "-datadir=";
							param += strCurDir;
							const char* argv[] = { "D:\\cppwork\\coin\\src\\coind.exe", param.c_str() };
				#else
							strCurDir +="/coin_test";
							string param = "-datadir=";
							param +=strCurDir;
							const char* argv[] = {"D:\\cppwork\\coin\\src\\coind.exe", param.c_str()};
				#endif
				CBaseParams::InitialParams(argc, argv);
			}
	}
    ~TestingSetup()
    {

    }
};

BOOST_GLOBAL_FIXTURE(TestingSetup);
