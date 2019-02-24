#ifndef VMLUA_H_
#define VMLUA_H_

#include <cstdio>

#include <vector>
#include <string>
#include <memory>

using namespace std;
class CVmRunEvn;

class CVmlua {
public:
	CVmlua(const vector<unsigned char> &vContractScript, const vector<unsigned char> &vContractCallParams);
	~CVmlua();
	tuple<uint64_t, string> run(uint64_t maxstep,CVmRunEvn *pVmScriptRun);
	static tuple<bool, string> CheckScriptSyntax(const char* filePath);

private:
	unsigned char m_ContractCallParams[4096];  		// to hold contract call params (contract)
	unsigned char m_ContractScript[65536];			// to hold contract script content
	
};


#endif
