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
	CVmlua(const vector<unsigned char> & vRom,const vector<unsigned char> &InputData);
	~CVmlua();
	tuple<uint64_t, string> run(uint64_t maxstep,CVmRunEvn *pVmScriptRun);
	static tuple<bool, string> CheckScriptSyntax(const char* filePath);

private:
	unsigned char m_ExRam[65536];  	// to save contract tx function argument (contract)
	unsigned char m_ExeFile[65536];	// executable file IpboApp.lua
};


#endif
