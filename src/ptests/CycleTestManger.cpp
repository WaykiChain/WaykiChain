/*
 * CycleTestManger.cpp
 *
 *  Created on: 2014��12��30��
 *      Author: ranger.shi
 */


#include <stdlib.h>
#include <time.h>
#include "main.h"
#include "miner/miner.h"
#include "commons/uint256.h"
#include "commons/util.h"
#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>

#include "json/json_spirit_writer_template.h"
#include "CBlackHalo_tests.h"
#include "./rpc/core/rpcclient.h"

#include "CycleP2PBet_tests.h"

using namespace std;
using namespace boost;
using namespace json_spirit;

#include "CycleTestManger.h"
#include "CycleSesureTrade_tests.h"
#include "CreateTx_tests.h"

void CycleTestManger::Initialize() {
//	vTest.push_back(std::make_shared<CTestSesureTrade>());
//	vTest.push_back(std::make_shared<CTestSesureTrade>());
//	vTest.push_back(std::make_shared<CTestSesureTrade>());
	for (int i = 0; i < 100; i++)
		vTest.push_back(std::make_shared<CBlackHalo>());
	for (int i = 0; i < 100; i++)
		vTest.push_back(std::make_shared<CTestBetTx>());
//	for (int i = 0; i < 300; i++)
//		vTest.push_back(std::make_shared<CCreateTxTest>());
}

void CycleTestManger::Initialize(vector<std::shared_ptr<CycleTestBase> > &vTestIn) {
	vTest = vTestIn;
}

void CycleTestManger::Run() {
	while (vTest.size() > 0) {
		for (auto it = vTest.begin(); it != vTest.end();) {
			bool flag = false;
			try {
				if (it->get()->Run() == end_state) {
					flag = true;
				};
			} catch (...) {
				flag = true;
			}

			if (flag)
				it = vTest.erase(it);
			else
				++it;

		}
		MilliSleep(1000);
	}
}


BOOST_FIXTURE_TEST_SUITE(CycleTest,CycleTestManger)

BOOST_FIXTURE_TEST_CASE(Cycle,CycleTestManger)
{
	Initialize();
	Run();
}

BOOST_AUTO_TEST_SUITE_END()
