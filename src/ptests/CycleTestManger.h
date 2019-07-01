/*
 * CycleTestManger.h
 *
 *  Created on: 2014Äê12ÔÂ30ÈÕ
 *      Author: ranger.shi
 */

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
