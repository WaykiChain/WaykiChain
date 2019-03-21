#ifndef VMLUA_H_
#define VMLUA_H_

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "main.h"

using namespace std;
class CVmRunEnv;

class CVmlua {
public:
    CVmlua(const vector<unsigned char> &vContractScript,
           const vector<unsigned char> &vContractCallParams);
    ~CVmlua();
    tuple<uint64_t, string> Run(uint64_t maxstep, CVmRunEnv *pVmRunEnv);
    static tuple<bool, string> CheckScriptSyntax(const char *filePath);

private:
    unsigned char contractCallArguments[nContractScriptMaxSize];  // to hold contract call arguments
    unsigned char contractScript[nContractArgumentMaxSize];       // to hold contract script content
};

#endif
