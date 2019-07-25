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
    CVmlua(const std::string &code, const std::string &arguments);
    ~CVmlua();
    std::tuple<uint64_t, string> Run(uint64_t fuelLimit, CVmRunEnv *pVmRunEnv);
    static std::tuple<bool, string> CheckScriptSyntax(const char *filePath);

private:
    // to hold contract call arguments
    std::string code;
    std::string arguments;
};

#endif
