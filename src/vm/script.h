/*
 * VmScript.h
 *
 *  Created on: Sep 2, 2014
 *      Author: ranger.shi
 */

#ifndef VMSCRIPT_H_
#define VMSCRIPT_H_

#include "serialize.h"
using namespace std;

/**
 * @brief Load script binary code class
 */
class CVmScript {
public:
    vector<unsigned char> rom;         //!< Binary code
    vector<unsigned char> scriptMemo;  //!< Describe the binary code action

public:
    CVmScript();
    virtual ~CVmScript();

    /**
     * @brief
     * @return
     */
    bool IsValid() {
        /// Binary code'size less 64k
        if ((rom.size() > 64 * 1024) || (rom.size() <= 0)) return false;

        if (!memcmp(&rom[0], "mylib = require", strlen("mylib = require"))) {
            return true;  // lua脚本，直接返回
        }
        return false;
    }

    bool IsCheckAccount(void) { return false; }

    IMPLEMENT_SERIALIZE(
        READWRITE(rom);
        READWRITE(scriptMemo);)
};

#endif /* VMSCRIPT_H_ */
