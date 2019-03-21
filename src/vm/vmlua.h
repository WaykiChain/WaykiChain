#ifndef VMLUA_H_
#define VMLUA_H_

#include <cstdio>

#include <vector>
#include <string>
#include <memory>

using namespace std;
class CVmRunEnv;

class CVmlua {
public:
	CVmlua(const vector<unsigned char> &vContractScript, const vector<unsigned char> &vContractCallParams);
	~CVmlua();
	tuple<uint64_t, string> Run(uint64_t maxstep,CVmRunEnv *pVmRunEnv);
	static tuple<bool, string> CheckScriptSyntax(const char* filePath);

private:
	unsigned char contractCallArguments[4096];  // to hold contract call arguments (contract)
	unsigned char contractScript[65536];        // to hold contract script content
};


#endif
