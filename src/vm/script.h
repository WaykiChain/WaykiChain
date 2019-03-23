/*
 * VmScript.h
 *
 *  Created on: Sep 2, 2014
 *      Author: ranger.shi
 */

#ifndef VMSCRIPT_H_
#define VMSCRIPT_H_

#include "main.h"
#include "serialize.h"
using namespace std;

/**
 * @brief Load script binary code class
 */
class CVmScript {
private:
	const static char* kLuaScriptHeadLine = "mylib = require";

public:
    vector<unsigned char> rom;   //!< Binary code
    vector<unsigned char> memo;  //!< Describe the binary code action

public:
    CVmScript();
    virtual ~CVmScript();

    /**
     * @brief
     * @return
     */
    bool IsValid() {
        if ((rom.size() > nContractScriptMaxSize) || (rom.size() <= 0))
            return false;

        if (!memcmp(&rom[0], LuaScriptHeadLine, strlen(LuaScriptHeadLine)))
            return true;  // lua脚本，直接返回

        return false;
    }

    bool IsCheckAccount(void) { return false; }

    IMPLEMENT_SERIALIZE(
        READWRITE(rom);
        READWRITE(memo);)
};

#endif /* VMSCRIPT_H_ */
