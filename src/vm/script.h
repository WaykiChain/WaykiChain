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
	constexpr static char* kLuaScriptHeadLine = (char*)"mylib = require";

    vector<unsigned char> rom;   //!< Binary code
    vector<unsigned char> memo;  //!< Describe the binary code action

public:
    CVmScript();
    virtual ~CVmScript();

	vector<unsigned char>& GetRom() { return rom; }

	vector<unsigned char>& GetMemo() { return memo; }

    /**
     * @brief
     * @return
     */
    bool IsValid() {
        if ((rom.size() > nContractScriptMaxSize) || (rom.size() <= 0))
            return false;

        if (!memcmp(&rom[0], kLuaScriptHeadLine, strlen(kLuaScriptHeadLine)))
            return true;  // lua script shebang existing verified

        return false;
    }

    bool IsCheckAccount(void) { return false; }

    IMPLEMENT_SERIALIZE(
        READWRITE(rom);
        READWRITE(memo);)
};

#endif /* VMSCRIPT_H_ */
