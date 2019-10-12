// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CYCLETESTMANGER_H_
#define CYCLETESTMANGER_H_

#include "CycleTestBase.h"

class CycleTestManger {
	vector<std::shared_ptr<CycleTestBase> > vTest;

public:
	CycleTestManger(){};
	void Initialize();
	void Initialize(vector<std::shared_ptr<CycleTestBase> > &vTestIn);
	void Run();
	static CycleTestManger &GetNewInstance() {
		static CycleTestManger aInstance;
		return aInstance;
	}
	virtual ~CycleTestManger(){};
};
#endif /* CYCLETESTMANGER_H_ */
