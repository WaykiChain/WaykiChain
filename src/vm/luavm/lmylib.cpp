// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <openssl/des.h>
#include <vector>

#include "lmylib.h"
#include "lua/lua.hpp"
#include "luavmrunenv.h"
#include "commons/SafeInt3.hpp"

#define LUA_C_BUFFER_SIZE  500  //传递值，最大字节防止栈溢出

///////////////////////////////////////////////////////////////////////////////
// local static functions

/*
 *  //3.往函数私有栈里存运算后的结果*/
static inline int32_t RetRstToLua(lua_State *L, const vector<uint8_t> &resultData, bool needToTruncate = true) {
    int32_t len = resultData.size();
    // truncate data by default
    if (needToTruncate) {
        len = len > LUA_C_BUFFER_SIZE ? LUA_C_BUFFER_SIZE : len;
    }

    if (len > 0) {
        // check stack to avoid stack overflow
        if (lua_checkstack(L, len)) {
            // LogPrint("vm", "RetRstToLua value:%s\n", HexStr(resultData).c_str());
            for (int32_t i = 0; i < len; i++) {
                lua_pushinteger(L, (lua_Integer)resultData[i]);
            }
            return len;
        } else {
            LogPrint("vm", "%s\n", "RetRstToLua stack overflow");
        }
    } else {
        LogPrint("vm", "RetRstToLua err len = %d\n", len);
    }
    return 0;
}

static inline int32_t RetRstToLua(lua_State *L, const string &resultData, bool needToTruncate = true) {
    int32_t len = resultData.size();
    // truncate data by default
    if (needToTruncate) {
        len = len > LUA_C_BUFFER_SIZE ? LUA_C_BUFFER_SIZE : len;
    }

    if (len > 0) {
        // check stack to avoid stack overflow
        if (lua_checkstack(L, len)) {
            // LogPrint("vm", "RetRstToLua value:%s\n", HexStr(resultData).c_str());
            for (int32_t i = 0; i < len; i++) {
                lua_pushinteger(L, (lua_Integer)uint8_t(resultData[i]));
            }
            return len;
        } else {
            LogPrint("vm", "%s\n", "RetRstToLua stack overflow");
        }
    } else {
        LogPrint("vm", "RetRstToLua err len = %d\n", len);
    }
    return 0;
}

/*
 *  //3.往函数私有栈里存布尔类型返回值*/
static inline int32_t RetRstBooleanToLua(lua_State *L, bool flag) {
    //检测栈空间是否够
    if (lua_checkstack(L, sizeof(int32_t))) {
        // LogPrint("vm", "RetRstBooleanToLua value:%d\n", flag);
        lua_pushboolean(L, (int32_t)flag);
        return 1;
    } else {
        LogPrint("vm", "%s\n", "RetRstBooleanToLua stack overflow");
        return 0;
    }
}

static inline int32_t RetFalse(const string reason) {
    LogPrint("vm", "%s\n", reason.c_str());
    return 0;
}

static CLuaVMRunEnv *GetVmRunEnv(lua_State *L) {
    CLuaVMRunEnv *pVmRunEnv = nullptr;
    int32_t res             = lua_getglobal(L, "VmScriptRun");
    // LogPrint("vm", "GetVmRunEnv lua_getglobal:%d\n", res);

    if (LUA_TLIGHTUSERDATA == res) {
        if (lua_islightuserdata(L, -1)) {
            pVmRunEnv = (CLuaVMRunEnv *)lua_topointer(L, -1);
            // LogPrint("vm", "GetVmRunEnv lua_topointer:%p\n", pVmRunEnv);
        }
    }
    lua_pop(L, 1);
    return pVmRunEnv;
}

static CLuaVMRunEnv* GetVmRunEnvByContext(lua_State *L) {
    lua_burner_state *pState = lua_GetBurnerState(L);
    assert(pState != nullptr && pState->pContext != nullptr);
    return (CLuaVMRunEnv*)pState->pContext;
}

static bool GetKeyId(const CAccountDBCache &accountView, vector<uint8_t> &ret, CKeyID &keyId) {
    if (ret.size() == 6) {
        CRegID reg(ret);
        keyId = reg.GetKeyId(accountView);
    } else if (ret.size() == 34) {
        string addr(ret.begin(), ret.end());
        keyId = CKeyID(addr);
    } else {
        return false;
    }

    if (keyId.IsEmpty())
        return false;

    return true;
}

static bool GetArray(lua_State *L, vector<std::shared_ptr<std::vector<uint8_t>>> &ret) {
    //从栈里取变长的数组
    int32_t totallen = lua_gettop(L);
    if ((totallen <= 0) || (totallen > LUA_C_BUFFER_SIZE)) {
        LogPrint("vm", "totallen error\n");
        return false;
    }

    vector<uint8_t> vBuf;
    vBuf.clear();
    for (int32_t i = 0; i < totallen; i++) {
        if (!lua_isnumber(L, i + 1))  // if(!lua_isnumber(L,-1 - i))
        {
            LogPrint("vm", "%s\n", "data is not number");
            return false;
        }
        vBuf.insert(vBuf.end(), lua_tonumber(L, i + 1));
    }
    ret.insert(ret.end(), std::make_shared<vector<uint8_t>>(vBuf.begin(), vBuf.end()));
    // LogPrint("vm", "GetData:%s, len:%d\n", HexStr(vBuf).c_str(), vBuf.size());
    return true;
}

static bool GetDataInt(lua_State *L, int32_t &intValue) {
    //从栈里取int 高度
    if (!lua_isinteger(L, -1 - 0)) {
        LogPrint("vm", "%s\n", "data is not integer");
        return false;
    } else {
        int32_t value = (int32_t)lua_tointeger(L, -1 - 0);
        // LogPrint("vm", "GetDataInt:%d\n", value);
        intValue = value;
        return true;
    }
}

static bool GetDataString(lua_State *L, vector<std::shared_ptr<std::vector<uint8_t>>> &ret) {
    //从栈里取一串字符串
    if (!lua_isstring(L, -1 - 0)) {
        LogPrint("vm", "%s\n", "data is not string");
        return false;
    }
    vector<uint8_t> vBuf;
    vBuf.clear();
    const char *pStr = nullptr;
    pStr             = lua_tostring(L, -1 - 0);
    if (pStr && (strlen(pStr) <= LUA_C_BUFFER_SIZE)) {
        for (size_t i = 0; i < strlen(pStr); i++) {
            vBuf.insert(vBuf.end(), pStr[i]);
        }
        ret.insert(ret.end(), std::make_shared<vector<uint8_t>>(vBuf.begin(), vBuf.end()));
        // LogPrint("vm", "GetDataString:%s\n", pStr);
        return true;
    } else {
        LogPrint("vm", "%s\n", "lua_tostring get fail");
        return false;
    }
}

// get bool field value of table
static bool GetBoolInTable(lua_State *L, const char *pKey, bool &value) {
    // the top of stack must be a table
    lua_pushstring(L, pKey);
    lua_gettable(L, -2);  // get the table field by key in top of stack
    if (!lua_isboolean(L, -1)) {
        LogPrint("vm", "get boolean field of table error! value=%s\n", lua_tostring(L, -1));
        lua_pop(L, 1);  // pop the result of lua_gettable
        return false;
    }
    value = lua_toboolean(L, -1);
    lua_pop(L, 1);  // pop the result of lua_gettable
    return true;
}

// get Integer field value of table
static bool getIntegerInTable(lua_State *L, const char *pKey, lua_Integer &value) {
    // the top of stack must be a table
    lua_pushstring(L, pKey);
    lua_gettable(L, -2);  // get the table field by key in top of stack
    if (!lua_isinteger(L, -1)) {
        LogPrint("vm", "get integer field of table error! value=%s\n", lua_tostring(L, -1));
        lua_pop(L, 1);  // pop the result of lua_gettable
        return false;
    }
    value = lua_tointeger(L, -1);
    lua_pop(L, 1);  // pop the result of lua_gettable
    return true;
}

static bool getNumberInTable(lua_State *L, const char* pKey, double &ret) {
    // 在table里，取指定pKey对应的一个number值

    //默认栈顶是table，将pKey入栈
    lua_pushstring(L, pKey);
    lua_gettable(L, -2);  //查找键值为key的元素，置于栈顶
    if (!lua_isnumber(L, -1)) {
        LogPrint("vm", "num get error! %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);  //删掉产生的查找结果
        return false;
    } else {
        ret = lua_tonumber(L, -1);
        // LogPrint("vm", "getNumberInTable:%d\n", ret);
        lua_pop(L, 1);  //删掉产生的查找结果
        return true;
    }
}

static bool getStringInTable(lua_State *L, const char * pKey, string &strValue) {
    // 在table里，取指定pKey对应的string值

    const char *pStr = nullptr;
    //默认栈顶是table，将pKey入栈
    lua_pushstring(L, pKey);
    lua_gettable(L, -2);  //查找键值为key的元素，置于栈顶
    if (!lua_isstring(L, -1)) {
        LogPrint("vm", "string get error! %s\n", lua_tostring(L, -1));
    } else {
        pStr = lua_tostring(L, -1);
        if (pStr && (strlen(pStr) <= LUA_C_BUFFER_SIZE)) {
            string res(pStr);
            strValue = res;
            //          LogPrint("vm", "getStringInTable:%s\n", pStr);
            lua_pop(L, 1);  //删掉产生的查找结果
            return true;
        } else {
            LogPrint("vm", "%s\n", "lua_tostring get fail");
        }
    }
    lua_pop(L, 1);  //删掉产生的查找结果
    return false;
}

template <typename ArrayType>
static bool getArrayInTable(lua_State *L, const char *pKey, uint16_t usLen, ArrayType &arrayOut) {
    // 在table里，取指定pKey对应的数组
    if ((usLen <= 0) || (usLen > LUA_C_BUFFER_SIZE)) {
        LogPrint("vm", "usLen error\n");
        return false;
    }
    uint8_t value = 0;
    arrayOut.clear();
    //默认栈顶是table，将key入栈
    lua_pushstring(L, pKey);
    lua_gettable(L, -2);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        LogPrint("vm", "getTableInTable is not table\n");
        return false;
    }
    for (int32_t i = 0; i < usLen; ++i) {
        lua_pushnumber(L, i + 1);  //将索引入栈
        lua_gettable(L, -2);
        if (!lua_isnumber(L, -1)) {
            LogPrint("vm", "getTableInTable is not number\n");
            return false;
        }
        value = 0;
        value = lua_tonumber(L, -1);
        arrayOut.insert(arrayOut.end(), value);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);  //删掉产生的查找结果
    return true;
}

static bool getStringLogPrint(lua_State *L, char *pKey, uint16_t usLen, vector<uint8_t> &vOut) {
    //从栈里取 table的值是一串字符串
    //该函数专用于写日志函数GetDataTableLogPrint，
    if ((usLen <= 0) || (usLen > LUA_C_BUFFER_SIZE)) {
        LogPrint("vm", "usLen error\n");
        return false;
    }

    //默认栈顶是table，将key入栈
    lua_pushstring(L, pKey);
    lua_gettable(L, 1);

    const char *pStr = nullptr;
    vOut.clear();
    lua_getfield(L, -2, pKey);
    // stackDump(L);
    if (!lua_isstring(L, -1) /*LUA_TSTRING != lua_type(L, -1)*/) {
        LogPrint("vm", "getStringLogPrint is not string\n");
        return false;
    }
    pStr = lua_tostring(L, -1 - 0);
    if (pStr && (strlen(pStr) == usLen)) {
        for (size_t i = 0; i < usLen; i++) {
            vOut.insert(vOut.end(), pStr[i]);
        }
        //      LogPrint("vm", "getfieldTableString:%s\n", pStr);
        lua_pop(L, 1);  //删掉产生的查找结果
        return true;
    } else {
        LogPrint("vm", "%s\n", "getStringLogPrint get fail\n");
        lua_pop(L, 1);  //删掉产生的查找结果
        return false;
    }
}

static bool GetDataTableLogPrint(lua_State *L, vector<std::shared_ptr<std::vector<uint8_t>>> &ret) {
    //取日志的key value
    if (!lua_istable(L, -1)) {
        LogPrint("vm", "GetDataTableLogPrint is not table\n");
        return false;
    }
    uint16_t len = 0;
    vector<uint8_t> vBuf;
    //取key
    int32_t key            = 0;
    double doubleValue = 0;
    if (!(getNumberInTable(L, "key", doubleValue))) {
        LogPrint("vm", "key get fail\n");
        return false;
    } else {
        key = (int32_t)doubleValue;
    }
    vBuf.clear();
    vBuf.insert(vBuf.end(), key);
    ret.insert(ret.end(), std::make_shared<vector<uint8_t>>(vBuf.begin(), vBuf.end()));

    //取value的长度
    if (!(getNumberInTable(L, "length", doubleValue))) {
        LogPrint("vm", "length get fail\n");
        return false;
    } else {
        len = (uint16_t)doubleValue;
    }

    if (len > 0) {
        len = len > LUA_C_BUFFER_SIZE ? LUA_C_BUFFER_SIZE : len;
        if (key) {  // hex
            if (!getArrayInTable(L, (char *)"value", len, vBuf)) {
                LogPrint("vm", "valueTable is not table\n");
                return false;
            } else {
                ret.insert(ret.end(), std::make_shared<vector<uint8_t>>(vBuf.begin(), vBuf.end()));
            }
        } else {  // string
            if (!getStringLogPrint(L, (char *)"value", len, vBuf)) {
                LogPrint("vm", "valueString is not string\n");
                return false;
            } else {
                ret.insert(ret.end(), std::make_shared<vector<uint8_t>>(vBuf.begin(), vBuf.end()));
            }
        }
        return true;
    } else {
        LogPrint("vm", "length error\n");
        return false;
    }
}

static bool GetDataTableDes(lua_State *L, vector<std::shared_ptr<std::vector<uint8_t>>> &ret) {
    if (!lua_istable(L, -1)) {
        LogPrint("vm", "is not table\n");
        return false;
    }
    double doubleValue = 0;
    vector<uint8_t> vBuf;

    int32_t dataLen = 0;
    if (!(getNumberInTable(L, (char *)"dataLen", doubleValue))) {
        LogPrint("vm", "dataLen get fail\n");
        return false;
    } else {
        dataLen = (uint32_t)doubleValue;
    }

    if (dataLen <= 0) {
        LogPrint("vm", "dataLen <= 0\n");
        return false;
    }

    if (!getArrayInTable(L, (char *)"data", dataLen, vBuf)) {
        LogPrint("vm", "data not table\n");
        return false;
    } else {
        ret.push_back(std::make_shared<vector<uint8_t>>(vBuf.begin(), vBuf.end()));
    }

    int32_t keyLen = 0;
    if (!(getNumberInTable(L, (char *)"keyLen", doubleValue))) {
        LogPrint("vm", "keyLen get fail\n");
        return false;
    } else {
        keyLen = (uint32_t)doubleValue;
    }

    if (keyLen <= 0) {
        LogPrint("vm", "keyLen <= 0\n");
        return false;
    }

    if (!getArrayInTable(L, (char *)"key", keyLen, vBuf)) {
        LogPrint("vm", "key not table\n");
        return false;
    } else {
        ret.push_back(std::make_shared<vector<uint8_t>>(vBuf.begin(), vBuf.end()));
    }

    int32_t nFlag = 0;
    if (!(getNumberInTable(L, (char *)"flag", doubleValue))) {
        LogPrint("vm", "flag get fail\n");
        return false;
    } else {
        nFlag = (uint32_t)doubleValue;
        CDataStream tep(SER_DISK, CLIENT_VERSION);
        tep << (nFlag == 0 ? 0 : 1);
        ret.push_back(std::make_shared<vector<uint8_t>>(tep.begin(), tep.end()));
    }

    return true;
}

static bool GetDataTableVerifySignature(lua_State *L, vector<std::shared_ptr<std::vector<uint8_t>>> &ret) {
    if (!lua_istable(L, -1)) {
        LogPrint("vm", "is not table\n");
        return false;
    }
    double doubleValue = 0;
    vector<uint8_t> vBuf;

    int32_t dataLen = 0;
    if (!(getNumberInTable(L, (char *)"dataLen", doubleValue))) {
        LogPrint("vm", "get dataLen failed\n");
        return false;
    } else {
        dataLen = (uint32_t)doubleValue;
    }

    if (dataLen <= 0) {
        LogPrint("vm", "dataLen <= 0\n");
        return false;
    }

    if (!getArrayInTable(L, (char *)"data", dataLen, vBuf)) {
        LogPrint("vm", "get data failed\n");
        return false;
    } else {
        ret.push_back(std::make_shared<vector<uint8_t>>(vBuf.begin(), vBuf.end()));
    }

    int32_t pubKeyLen = 0;
    if (!(getNumberInTable(L, (char *)"pubKeyLen", doubleValue))) {
        LogPrint("vm", "get pubKeyLen failed\n");
        return false;
    } else {
        pubKeyLen = (uint32_t)doubleValue;
    }

    if (pubKeyLen <= 0) {
        LogPrint("vm", "error: pubKeyLen <= 0\n");
        return false;
    }

    if (!getArrayInTable(L, (char *)"pubKey", pubKeyLen, vBuf)) {
        LogPrint("vm", "get pubKey failed\n");
        return false;
    } else {
        ret.push_back(std::make_shared<vector<uint8_t>>(vBuf.begin(), vBuf.end()));
    }

    int32_t signatureLen = 0;
    if (!(getNumberInTable(L, (char *)"signatureLen", doubleValue))) {
        LogPrint("vm", "get signatureLen failed\n");
        return false;
    }
    signatureLen = (uint32_t)doubleValue;

    if (signatureLen <= 0) {
        LogPrint("vm", "hashLen <= 0\n");
        return false;
    }

    if (!getArrayInTable(L, (char *)"signature", signatureLen, vBuf)) {
        LogPrint("vm", "get signature failed\n");
        return false;
    } else {
        ret.push_back(std::make_shared<vector<uint8_t>>(vBuf.begin(), vBuf.end()));
    }

    return true;
}

/**
 * Parse uidType in table
 */
static bool ParseUidTypeInTable(lua_State *L, const char *pKey, AccountType &uidType) {
    lua_Integer uidTypeInt;
    if (!(getIntegerInTable(L, pKey, uidTypeInt))) {
        LogPrint("vm", "ParseUidTypeInTable(), get %s failed\n", pKey);
        return false;
    }

    if (uidTypeInt != AccountType::REGID && uidTypeInt != AccountType::BASE58ADDR) {
        LogPrint("vm", "ParseUidTypeInTable(), invalid accountType: %d\n", uidTypeInt);
        return false;
    }
    uidType = (AccountType)uidTypeInt;
    return true;
}

/**
 * Parse uid in table
 */
static bool ParseUidInTable(lua_State *L, const char *pKey, AccountType uidType, CUserID &uid) {
    int32_t len;
    if (uidType == AccountType::REGID) {
       len = 6;
    } else {
        assert(uidType == AccountType::BASE58ADDR);
       len = 34;
    }

    vector<uint8_t> accountBuf;
    if (!getArrayInTable(L, pKey, len, accountBuf)) {
        LogPrint("vm","ParseUidInTable(), get %s failed\n", pKey);
        return false;
    }

    if (uidType == AccountType::REGID) {
        CRegID regid(accountBuf);
        if (regid.IsEmpty()) {
            LogPrint("vm","ParseUidInTable(), %s is invalid regid! value(hex)=%s\n", pKey, HexStr(accountBuf));
            return false;
        }
        uid = regid;
    } else {
        assert(uidType == AccountType::BASE58ADDR);
        CKeyID keyid;
        CCoinAddress coinAddress;
        string addrStr(accountBuf.begin(), accountBuf.end());
        if (!coinAddress.SetString(addrStr) || !coinAddress.GetKeyId(keyid) || keyid.IsEmpty()) {
            LogPrint("vm","ParseUidInTable(), %s is invalid keyid! keyid=%s, hex=%s\n", pKey,
                addrStr, HexStr(accountBuf));
        }
        uid = keyid;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// lua contract api functions added in MAJOR_VER_R1

int32_t ExInt64MulFunc(lua_State *L) {

    int32_t argc = lua_gettop(L);    /* number of arguments */
    if(argc != 2) {
        return RetFalse("argc error\n");
    }

    if (!lua_isinteger(L, 1)) {
        return RetFalse("Int64Mul para1 error\n");
    }
    int64_t a = lua_tointeger(L, 1);

    if (!lua_isinteger(L, 2)) {
        return RetFalse("Int64Mul para2 error\n");
    }
    int64_t b = lua_tointeger(L, 2);
    int64_t c = 0;
    LUA_BurnFuncCall(L, FUEL_CALL_Int64Mul, BURN_VER_R2);
    if (!SafeMultiply(a, b, c)) {
        return RetFalse("Int64Mul Operate overflow !\n");
    }

    lua_pushinteger(L, c);
    return 1;
}

int32_t ExInt64AddFunc(lua_State *L) {
    int32_t argc = lua_gettop(L); /* number of arguments */
    if (argc != 2) {
        return RetFalse("argc error\n");
    }

    if (!lua_isinteger(L, 1)) {
        return RetFalse("Int64Add para1 error\n");
    }
    int64_t a = lua_tointeger(L, 1);

    if (!lua_isinteger(L, 2)) {
        return RetFalse("Int64Add para2 error\n");
    }
    int64_t b = lua_tointeger(L, 2);

    int64_t c = 0;
    LUA_BurnFuncCall(L, FUEL_CALL_Int64Add, BURN_VER_R2);
    if (!SafeAdd(a, b, c)) {
        return RetFalse("Int64Add Operate overflow !\n");
    }

    lua_pushinteger(L, c);
    return 1;
}

int32_t ExInt64SubFunc(lua_State *L) {
    int32_t argc = lua_gettop(L); /* number of arguments */
    if (argc != 2) {
        return RetFalse("argc error\n");
    }

    if (!lua_isinteger(L, 1)) {
        return RetFalse("Int64Sub para1 error\n");
    }
    int64_t a = lua_tointeger(L, 1);

    if (!lua_isinteger(L, 2)) {
        return RetFalse("Int64Sub para2 error\n");
    }
    int64_t b = lua_tointeger(L, 2);

    int64_t c = 0;
    LUA_BurnFuncCall(L, FUEL_CALL_Int64Sub, BURN_VER_R2);
    if (!SafeSubtract(a, b, c)) {
        return RetFalse("Int64Sub Operate overflow !\n");
    }

    lua_pushinteger(L, c);
    return 1;
}

int32_t ExInt64DivFunc(lua_State *L) {
    int32_t argc = lua_gettop(L); /* number of arguments */
    if (argc != 2) {
        return RetFalse("argc error\n");
    }

    if (!lua_isinteger(L, 1)) {
        return RetFalse("Int64Div para1 error\n");
    }
    int64_t a = lua_tointeger(L, 1);

    if (!lua_isinteger(L, 2)) {
        return RetFalse("Int64Div para2 error\n");
    }
    int64_t b = lua_tointeger(L, 2);

    int64_t c = 0;
    LUA_BurnFuncCall(L, FUEL_CALL_Int64Div, BURN_VER_R2);
    if (!SafeDivide(a, b, c)) {
        return RetFalse("Int64Div Operate overflow !\n");
    }

    lua_pushinteger(L, c);
    return 1;
}

/**
 *bool SHA256(void const* pfrist, const uint16_t len, void * const pout)
 * This function receives an input param from a middle layer:
 *   1. The first param is the target string to be hashed twice in a BitCoin way
 */
int32_t ExSha256Func(lua_State *L) {
    vector<std::shared_ptr < vector<uint8_t> > > retdata;
    if (!GetDataString(L, retdata) || retdata.size() != 1 || retdata.at(0).get()->size() <= 0) {
        return RetFalse("ExSha256Func param err");
    }
    vector<uint8_t> &dataIn = *retdata.at(0).get();
    LUA_BurnFuncData(L, FUEL_CALL_Sha256, dataIn.size(), 32, FUEL_DATA32_Sha256, BURN_VER_R2);
    uint256 hash = Hash(dataIn.begin(), dataIn.end());
    CDataStream tep(SER_DISK, CLIENT_VERSION);
    tep << hash;
    vector<uint8_t> tep1(tep.begin(), tep.end());
    return RetRstToLua(L, tep1);
}

/**
 *bool SHA256Once(void const* pfrist, const uint16_t len, void * const pout)
 * This function receives an input param from a middle layer:
 *   1. The first param is the target string to be hashed once
 */
int32_t ExSha256OnceFunc(lua_State *L) {

    vector<std::shared_ptr < vector<uint8_t> > > retdata;
    if (!GetDataString(L,retdata) ||retdata.size() != 1 || retdata.at(0).get()->size() <= 0) {
        return RetFalse("ExSha256OnceFunc param err");
    }
    vector<uint8_t> &dataIn = *retdata.at(0).get();

    LUA_BurnFuncData(L, FUEL_CALL_Sha256Once, dataIn.size(), 32, FUEL_DATA32_Sha256Once, BURN_VER_R2);
    uint256 hash;
    SHA256(&dataIn.at(0), dataIn.size(), hash.begin());
    vector<uint8_t> tep1(hash.begin(), hash.end());
    return RetRstToLua(L, tep1);
}

/**
 *uint16_t Des(void const* pData, uint16_t len, void const* pkey, uint16_t keylen, bool IsEn, void * const pOut,uint16_t poutlen)
 * 这个函数式从中间层传了三个个参数过来:
 * 1.第一个是要被加密数据或者解密数据
 * 2.第二格式加密或者解密的key值
 * 3.第三是标识符，是加密还是解密
 *
 * {
 *  dataLen = 0,
 *  data = {},
 *  keyLen = 0,
 *  key = {},
 *  flag = 0
 * }
 */
int32_t ExDesFunc(lua_State *L) {
    vector<std::shared_ptr<vector<uint8_t> > > retdata;

    if (!GetDataTableDes(L, retdata) || retdata.size() != 3) {
        return RetFalse(string(__FUNCTION__) + "para  err !");
    }
    vector<uint8_t> &dataIn = *retdata.at(0).get();
    vector<uint8_t> &keyIn = *retdata.at(1).get();
    uint8_t flag = retdata.at(2).get()->at(0);
    static_assert(sizeof(DES_cblock) == 8, "DES block must be 8");
    if (keyIn.size() == 8) {
        LUA_BurnFuncData(L, FUEL_CALL_DesBasic, dataIn.size(), 8, FUEL_DATA8_DesBasic, BURN_VER_R2);
    } else if (keyIn.size() == 8) {
        LUA_BurnFuncData(L, FUEL_CALL_DesTriple, dataIn.size(), 8, FUEL_DATA8_DesTriple, BURN_VER_R2);
    } else {
        return RetFalse(string(__FUNCTION__) + "para  err !");
    }

    DES_key_schedule deskey1, deskey2, deskey3;

    vector<uint8_t> desdata;
    vector<uint8_t> desout;
    uint8_t datalen_rest = dataIn.size() % sizeof(DES_cblock);
    desdata.assign(dataIn.begin(), dataIn.end());
    if (datalen_rest) {
        desdata.insert(desdata.end(), sizeof(DES_cblock) - datalen_rest, 0);
    }

    const_DES_cblock in;
    DES_cblock out, key;

    desout.resize(desdata.size());

    if (flag == 1) {
        if (keyIn.size() == 8) {
            memcpy(key, &keyIn.at(0), sizeof(DES_cblock));
            DES_set_key_unchecked(&key, &deskey1);
            for (uint32_t ii = 0; ii < desdata.size() / sizeof(DES_cblock); ii++) {
                memcpy(&in, &desdata[ii * sizeof(DES_cblock)], sizeof(in));
                DES_ecb_encrypt(&in, &out, &deskey1, DES_ENCRYPT);
                memcpy(&desout[ii * sizeof(DES_cblock)], &out, sizeof(out));
            }
        } else if (keyIn.size() == 16) {
            memcpy(key, &keyIn.at(0), sizeof(DES_cblock));
            DES_set_key_unchecked(&key, &deskey1);
            DES_set_key_unchecked(&key, &deskey3);
            memcpy(key, &keyIn.at(0) + sizeof(DES_cblock), sizeof(DES_cblock));
            DES_set_key_unchecked(&key, &deskey2);
            for (uint32_t ii = 0; ii < desdata.size() / sizeof(DES_cblock); ii++) {
                memcpy(&in, &desdata[ii * sizeof(DES_cblock)], sizeof(in));
                DES_ecb3_encrypt(&in, &out, &deskey1, &deskey2, &deskey3, DES_ENCRYPT);
                memcpy(&desout[ii * sizeof(DES_cblock)], &out, sizeof(out));
            }
        } else {
            return RetFalse(string(__FUNCTION__) + "para  err !");
        }
    } else {
        if (keyIn.size() == 8) {
            memcpy(key, &keyIn.at(0), sizeof(DES_cblock));
            DES_set_key_unchecked(&key, &deskey1);
            for (uint32_t ii = 0; ii < desdata.size() / sizeof(DES_cblock); ii++) {
                memcpy(&in, &desdata[ii * sizeof(DES_cblock)], sizeof(in));
                DES_ecb_encrypt(&in, &out, &deskey1, DES_DECRYPT);
                memcpy(&desout[ii * sizeof(DES_cblock)], &out, sizeof(out));
            }
        } else if (keyIn.size() == 16) {
            memcpy(key, &keyIn.at(0), sizeof(DES_cblock));
            DES_set_key_unchecked(&key, &deskey1);
            DES_set_key_unchecked(&key, &deskey3);
            memcpy(key, &keyIn.at(0) + sizeof(DES_cblock), sizeof(DES_cblock));
            DES_set_key_unchecked(&key, &deskey2);
            for (uint32_t ii = 0; ii < desdata.size() / sizeof(DES_cblock); ii++) {
                memcpy(&in, &desdata[ii * sizeof(DES_cblock)], sizeof(in));
                DES_ecb3_encrypt(&in, &out, &deskey1, &deskey2, &deskey3, DES_DECRYPT);
                memcpy(&desout[ii * sizeof(DES_cblock)], &out, sizeof(out));
            }
        } else {
            return RetFalse(string(__FUNCTION__) + "para  err !");
        }
    }

    return RetRstToLua(L,desout);
}

/**
 *bool SignatureVerify(void const* data, uint16_t datalen, void const* key, uint16_t keylen,
        void const* phash, uint16_t hashlen)
 * 这个函数式从中间层传了三个个参数过来:
 * 1.第一个是签名前的原始数据
 * 2.第二个是签名的公钥(public key)
 * 3.第三是已签名的数据
 *
 *{
 *  dataLen = 0,
 *  data = {},
 *  pubKeyLen = 0,
 *  pubKey = {},
 *  signatureLen = 0,
 *  signature = {}
 * }
 */
int32_t ExVerifySignatureFunc(lua_State *L) {
    vector<std::shared_ptr<vector<uint8_t> > > retdata;

    if (!GetDataTableVerifySignature(L, retdata) || retdata.size() != 3 || retdata.at(1).get()->size() != 33) {
        return RetFalse(string(__FUNCTION__) + "para  err !");
    }
    vector<uint8_t> &data = *retdata.at(0);
    vector<uint8_t> &pubKey = *retdata.at(1);
    vector<uint8_t> &signature = *retdata.at(2);

    LUA_BurnFuncData(L, FUEL_CALL_VerifySignature, data.size(), 32, FUEL_DATA32_VerifySignature, BURN_VER_R2);

    CPubKey pk(pubKey.begin(), pubKey.end());

    uint256 dataHash = Hash(data.begin(), data.end());

    bool rlt = VerifySignature(dataHash, signature, pk);
    if (!rlt) {
        LogPrint("INFO", "ExVerifySignatureFunc call VerifySignature verify signature failed!\n");
    }

    return RetRstBooleanToLua(L, rlt);
}

int32_t ExGetTxContractFunc(lua_State *L) {
    vector<std::shared_ptr<vector<uint8_t>>> retdata;
    if (!GetArray(L, retdata) || retdata.size() != 1 || retdata.at(0).get()->size() != 32) {
        return RetFalse("ExGetTxContractFunc, para error");
    }

    CLuaVMRunEnv *pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv) {
        return RetFalse("ExGetTxContractFunc, pVmRunEnv is nullptr");
    }

    vector<uint8_t> vHash(retdata.at(0).get()->rbegin(), retdata.at(0).get()->rend());
    CDataStream ds(vHash, SER_DISK, CLIENT_VERSION);
    uint256 hash;
    ds >> hash;

    LogPrint("vm", "ExGetTxContractFunc, hash: %s\n", hash.GetHex().c_str());

    std::shared_ptr<CBaseTx> pBaseTx;
    int32_t len = 0;
    if (hash == pVmRunEnv->GetCurTxHash()) {
        const string &curTxArguments = pVmRunEnv->GetTxContract();
        LUA_BurnFuncData(L, FUEL_CALL_GetCurTxContract, curTxArguments.size(), 32, FUEL_DATA32_GetTxContract, BURN_VER_R2);
        len = RetRstToLua(L, curTxArguments, false);
    } else if (GetTransaction(pBaseTx, hash, pVmRunEnv->GetCw()->blockCache, false)) {
        if (pBaseTx->nTxType == LCONTRACT_INVOKE_TX) {
            CLuaContractInvokeTx *tx = static_cast<CLuaContractInvokeTx *>(pBaseTx.get());
            LUA_BurnFuncData(L, FUEL_CALL_GetTxContract, tx->arguments.size(), 32, FUEL_DATA32_GetTxContract, BURN_VER_R2);
            len = RetRstToLua(L, tx->arguments, false);
        } else if (pBaseTx->nTxType == UCONTRACT_INVOKE_TX) {
            CUniversalContractInvokeTx *tx = static_cast<CUniversalContractInvokeTx *>(pBaseTx.get());
            LUA_BurnFuncData(L, FUEL_CALL_GetTxContract, tx->arguments.size(), 32, FUEL_DATA32_GetTxContract, BURN_VER_R2);
            len = RetRstToLua(L, tx->arguments, false);
        } else {
            LUA_BurnFuncCall(L, FUEL_CALL_GetTxContract, BURN_VER_R2);
            return RetFalse("ExGetTxContractFunc, tx type error");
        }
    }

    return len;
}

/**
 *void LogPrint(const void *pData, const uint16_t datalen,PRINT_FORMAT flag )
 * 这个函数式从中间层传了两个个参数过来:
 * 1.第一个是打印数据的表示符号，true是一十六进制打印,否则以字符串的格式打印
 * 2.第二个是打印的字符串
 */
int32_t ExLogPrintFunc(lua_State *L) {
    vector<std::shared_ptr<vector<uint8_t>>> retdata;
    if (!GetDataTableLogPrint(L, retdata) || retdata.size() != 2) {
        return RetFalse("ExLogPrintFunc para, err1");
    }

    CDataStream tep1(*retdata.at(0), SER_DISK, CLIENT_VERSION);
    bool flag;
    tep1 >> flag;
    string pData((*retdata[1]).begin(), (*retdata[1]).end());
    LUA_BurnFuncData(L, FUEL_CALL_LogPrint, pData.size(), 1, FUEL_DATA1_LogPrint, BURN_VER_R2);

    if (flag) {
        LogPrint("vm", "%s\n", HexStr(pData).c_str());
    } else {
        LogPrint("vm", "%s\n", pData.c_str());
    }

    return 0;
}

/**
 *uint16_t GetAccounts(const uint8_t *txid, void* const paccount, uint16_t maxlen)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是 hash
 */
int32_t ExGetTxRegIDFunc(lua_State *L) {
    vector<std::shared_ptr<vector<uint8_t>>> retdata;
    if (!GetArray(L, retdata) || retdata.size() != 1 || retdata.at(0).get()->size() != 32) {
        return RetFalse("ExGetTxRegIDFunc, para error");
    }

    CLuaVMRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv) {
        return RetFalse("ExGetTxRegIDFunc, pVmRunEnv is nullptr");
    }

    vector<uint8_t> vHash(retdata.at(0).get()->rbegin(), retdata.at(0).get()->rend());

    CDataStream ds(vHash, SER_DISK, CLIENT_VERSION);
    uint256 hash;
    ds >> hash;

    LogPrint("vm","ExGetTxRegIDFunc, hash: %s\n", hash.GetHex().c_str());

    LUA_BurnFuncCall(L, FUEL_CALL_GetTxRegID, BURN_VER_R2);
    std::shared_ptr<CBaseTx> pBaseTx;
    int32_t len = 0;
    if (GetTransaction(pBaseTx, hash, pVmRunEnv->GetCw()->blockCache, false)) {
        if (pBaseTx->nTxType == BCOIN_TRANSFER_TX) {
            CBaseCoinTransferTx *tx = static_cast<CBaseCoinTransferTx*>(pBaseTx.get());
            if (tx->txUid.type() != typeid(CRegID))
                return RetFalse("ExGetTxRegIDFunc, txUid is not CRegID type");

            vector<uint8_t> item = tx->txUid.get<CRegID>().GetRegIdRaw();
            len = RetRstToLua(L, item);
        } else if (pBaseTx->nTxType == LCONTRACT_INVOKE_TX) {
            CLuaContractInvokeTx *tx = static_cast<CLuaContractInvokeTx*>(pBaseTx.get());
            if (tx->txUid.type() != typeid(CRegID))
                return RetFalse("ExGetTxRegIDFunc, txUid is not CRegID type");

            vector<uint8_t> item = tx->txUid.get<CRegID>().GetRegIdRaw();
            len = RetRstToLua(L, item);
        } else {
            return RetFalse("ExGetTxRegIDFunc, tx type error");
        }
    }

    return len;
}

int32_t ExByteToIntegerFunc(lua_State *L) {
    //把字节流组合成integer
    vector<std::shared_ptr<vector<uint8_t>>> retdata;
    if (!GetArray(L, retdata) || retdata.size() != 1 ||
        ((retdata.at(0).get()->size() != 4) && (retdata.at(0).get()->size() != 8))) {
        return RetFalse("ExByteToIntegerFunc para err1");
    }

    //将数据反向
    vector<uint8_t> vValue(retdata.at(0).get()->begin(), retdata.at(0).get()->end());
    CDataStream tep1(vValue, SER_DISK, CLIENT_VERSION);

    LUA_BurnFuncCall(L, FUEL_CALL_ByteToInteger, BURN_VER_R2);
    if (retdata.at(0).get()->size() == 4) {
        uint32_t height;
        tep1 >> height;

        // LogPrint("vm", "%d\n", height);
        if (lua_checkstack(L, sizeof(lua_Integer))) {
            lua_pushinteger(L, (lua_Integer)height);
            return 1;
        } else {
            return RetFalse("ExByteToIntegerFunc stack overflow");
        }
    } else {
        int64_t llValue = 0;
        tep1 >> llValue;
        // LogPrint("vm", "%lld\n", llValue);
        if (lua_checkstack(L, sizeof(lua_Integer))) {
            lua_pushinteger(L, (lua_Integer)llValue);
            return 1;
        } else {
            return RetFalse("ExByteToIntegerFunc stack overflow");
        }
    }
}

int32_t ExIntegerToByte4Func(lua_State *L) {
    //把integer转换成4字节数组
    int32_t height = 0;
    if (!GetDataInt(L, height)) {
        return RetFalse("ExIntegerToByte4Func para err1");
    }
    LUA_BurnFuncCall(L, FUEL_CALL_IntegerToByte4, BURN_VER_R2);
    CDataStream tep(SER_DISK, CLIENT_VERSION);
    tep << height;
    vector<uint8_t> TMP(tep.begin(), tep.end());
    return RetRstToLua(L, TMP);
}

int32_t ExIntegerToByte8Func(lua_State *L) {
    //把integer转换成8字节数组
    int64_t llValue = 0;
    if (!lua_isinteger(L, -1 - 0)) {
        LogPrint("vm", "%s\n", "data is not integer");
        return 0;
    } else {
        llValue = (int64_t)lua_tointeger(L, -1 - 0);
        //      LogPrint("vm", "ExIntegerToByte8Func:%lld\n", llValue);
    }

    LUA_BurnFuncCall(L, FUEL_CALL_IntegerToByte8, BURN_VER_R2);
    CDataStream tep(SER_DISK, CLIENT_VERSION);
    tep << llValue;
    vector<uint8_t> TMP(tep.begin(), tep.end());
    return RetRstToLua(L, TMP);
}
/**
 *uint16_t GetAccountPublickey(const void* const accountId,void * const pubkey,const uint16_t maxlength)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是 账户id,六个字节
 */
int32_t ExGetAccountPublickeyFunc(lua_State *L) {
    vector<std::shared_ptr<vector<uint8_t>>> retdata;
    if (!GetArray(L, retdata) || retdata.size() != 1 ||
        !(retdata.at(0).get()->size() == 6 || retdata.at(0).get()->size() == 34)) {
        return RetFalse("ExGetAccountPublickeyFunc para err1");
    }

    CLuaVMRunEnv *pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv) {
        return RetFalse("pVmRunEnv is nullptr");
    }

    LUA_BurnFuncCall(L, FUEL_CALL_GetAccountPublickey, BURN_VER_R2);
    CKeyID addrKeyId;
    if (!GetKeyId(*(pVmRunEnv->GetCatchView()), *retdata.at(0).get(), addrKeyId)) {
        return RetFalse("ExGetAccountPublickeyFunc para err2");
    }
    CUserID userid(addrKeyId);
    CAccount account;
    if (!pVmRunEnv->GetCatchView()->GetAccount(userid, account)) {
        return RetFalse("ExGetAccountPublickeyFunc para err3");
    }
    CDataStream tep(SER_DISK, CLIENT_VERSION);
    vector<char> te;
    tep << account.owner_pubkey;
    // assert(account.owner_pubkey.IsFullyValid());
    if (false == account.owner_pubkey.IsFullyValid()) {
        return RetFalse("ExGetAccountPublickeyFunc pubKey invalid");
    }
    tep >> te;
    vector<uint8_t> tep1(te.begin(), te.end());
    return RetRstToLua(L, tep1);
}

/**
 *bool QueryAccountBalance(const uint8_t* const account,Int64* const pBalance)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是 账户id,六个字节
 */
int32_t ExQueryAccountBalanceFunc(lua_State *L) {
    vector<std::shared_ptr<vector<uint8_t>>> retdata;
    if (!GetArray(L, retdata) || retdata.size() != 1 ||
        !(retdata.at(0).get()->size() == 6 || retdata.at(0).get()->size() == 34)) {
        return RetFalse("ExQueryAccountBalanceFunc para err1");
    }

    CLuaVMRunEnv *pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv) {
        return RetFalse("pVmRunEnv is nullptr");
    }

    LUA_BurnFuncCall(L, FUEL_ACCOUNT_GET_VALUE, BURN_VER_R2);
    CKeyID addrKeyId;
    if (!GetKeyId(*(pVmRunEnv->GetCatchView()), *retdata.at(0).get(), addrKeyId)) {
        return RetFalse("ExQueryAccountBalanceFunc para err2");
    }

    CUserID userid(addrKeyId);
    CAccount account;
    int32_t len = 0;
    if (!pVmRunEnv->GetCatchView()->GetAccount(userid, account)) {
        len = 0;
    } else {
        uint64_t nbalance = account.GetToken(SYMB::WICC).free_amount;
        CDataStream tep(SER_DISK, CLIENT_VERSION);
        tep << nbalance;
        vector<uint8_t> TMP(tep.begin(), tep.end());
        len = RetRstToLua(L, TMP);
    }
    return len;
}

/**
 *uint32_t GetTxConfirmHeight(const void * const txid)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个入参: hash,32个字节
 */
int32_t ExGetTxConfirmHeightFunc(lua_State *L) {
    vector<std::shared_ptr<vector<uint8_t>>> retdata;

    if (!GetArray(L, retdata) || retdata.size() != 1 || retdata.at(0).get()->size() != 32) {
        return RetFalse("ExGetTxConfirmHeightFunc para err1");
    }

    // uint256 hash1(*retdata.at(0));
    // LogPrint("vm","ExGetTxContractsFunc1:%s",hash1.GetHex().c_str());

    // reverse hash value
    vector<uint8_t> vec_hash(retdata.at(0).get()->rbegin(), retdata.at(0).get()->rend());
    CDataStream tep1(vec_hash, SER_DISK, CLIENT_VERSION);
    uint256 hash1;
    tep1 >> hash1;

    CLuaVMRunEnv *pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv) {
        return RetFalse("pVmRunEnv is nullptr");
    }

    LUA_BurnFuncCall(L, FUEL_CALL_GetTxConfirmHeight, BURN_VER_R2);
    int32_t height = GetTxConfirmHeight(hash1, pVmRunEnv->GetCw()->blockCache);
    if (-1 == height) {
        return RetFalse("ExGetTxConfirmHeightFunc para err2");
    } else {
        if (lua_checkstack(L, sizeof(lua_Number))) {
            lua_pushnumber(L, (lua_Number)height);
            return 1;
        } else {
            LogPrint("vm", "%s\n", "ExGetCurRunEnvHeightFunc stack overflow");
            return 0;
        }
    }
}


/**
 *bool GetBlockHash(const uint32_t height,void * const pBlochHash)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是 int类型的参数
 */
int32_t ExGetBlockHashFunc(lua_State *L) {
    int32_t height = 0;
    if (!GetDataInt(L, height)) {
        return RetFalse("ExGetBlockHashFunc para err1");
    }

    CLuaVMRunEnv *pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv) {
        return RetFalse("pVmRunEnv is nullptr");
    }

    LUA_BurnFuncCall(L, FUEL_CALL_GetBlockHash, BURN_VER_R2);
    //当前block 是不可以获取hash的
    if (height <= 0 || height >= pVmRunEnv->GetConfirmHeight()) {
        return RetFalse("ExGetBlockHashFunc para err2");
    }

    if (chainActive.Height() < height) {  //获取比当前高度高的数据是不可以的
        return RetFalse("ExGetBlockHashFunc para err3");
    }

    CBlockIndex *pIndex = chainActive[height];
    uint256 blockHash   = pIndex->GetBlockHash();

    //  LogPrint("vm","ExGetBlockHashFunc:%s",HexStr(blockHash).c_str());
    CDataStream tep(SER_DISK, CLIENT_VERSION);
    tep << blockHash;
    vector<uint8_t> TMP(tep.begin(), tep.end());
    // reverse hash value
    vector<uint8_t> TMP2(TMP.rbegin(), TMP.rend());
    return RetRstToLua(L, TMP2);
}

int32_t ExGetCurRunEnvHeightFunc(lua_State *L) {
    CLuaVMRunEnv *pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv) {
        return RetFalse("pVmRunEnv is nullptr");
    }

    LUA_BurnFuncCall(L, FUEL_CALL_GetCurRunEnvHeight, BURN_VER_R2);
    int32_t height = pVmRunEnv->GetConfirmHeight();

    //检测栈空间是否够
    if (height > 0) {
        // lua_Integer
        /*
        if(lua_checkstack(L,sizeof(lua_Number))){
             lua_pushnumber(L,(lua_Number)height);
             return 1 ;
        }else{
            LogPrint("vm","%s\n", "ExGetCurRunEnvHeightFunc stack overflow");
        }
        */
        if (lua_checkstack(L, sizeof(lua_Integer))) {
            lua_pushinteger(L, (lua_Integer)height);
            return 1;
        } else {
            LogPrint("vm", "%s\n", "ExGetCurRunEnvHeightFunc stack overflow");
        }
    } else {
        LogPrint("vm", "ExGetCurRunEnvHeightFunc err height =%d\n", height);
    }

    return 0;
}

static bool GetDataTableWriteDataDB(lua_State *L, vector<std::shared_ptr<std::vector<uint8_t>>> &ret) {
    //取写数据库的key value
    if (!lua_istable(L, -1)) {
        LogPrint("vm", "GetDataTableWriteDataDB is not table\n");
        return false;
    }
    uint16_t len = 0;
    vector<uint8_t> vBuf;
    //取key
    string key = "";
    if (!(getStringInTable(L, (char *)"key", key))) {
        LogPrint("vm", "key get fail\n");
        return false;
    } else {
        // LogPrint("vm", "key:%s\n", key);
    }
    vBuf.clear();
    for (size_t i = 0; i < key.size(); i++) {
        vBuf.insert(vBuf.end(), key.at(i));
    }
    ret.insert(ret.end(), std::make_shared<vector<uint8_t>>(vBuf.begin(), vBuf.end()));

    //取value的长度
    double doubleValue = 0;
    if (!(getNumberInTable(L, (char *)"length", doubleValue))) {
        LogPrint("vm", "length get fail\n");
        return false;
    } else {
        len = (uint16_t)doubleValue;
        // LogPrint("vm", "len =%d\n", len);
    }
    if ((len > 0) && (len <= LUA_C_BUFFER_SIZE)) {
        if (!getArrayInTable(L, (char *)"value", len, vBuf)) {
            LogPrint("vm", "value is not table\n");
            return false;
        } else {
            // LogPrint("vm", "value:%s\n", HexStr(vBuf).c_str());
            ret.insert(ret.end(), std::make_shared<vector<uint8_t>>(vBuf.begin(), vBuf.end()));
        }
        return true;
    } else {
        LogPrint("vm", "len overflow\n");
        return false;
    }
}

/**
 *bool WriteDataDB(const void* const key,const uint8_t keylen,const void * const value,const uint16_t valuelen,const uint32_t time)
 * 这个函数式从中间层传了三个个参数过来:
 * 1.第一个是 key值
 * 2.第二个是value值
 */
int32_t ExWriteDataDBFunc(lua_State *L) {
    vector<std::shared_ptr < vector<uint8_t> > > retdata;
    if (!GetDataTableWriteDataDB(L, retdata) || retdata.size() != 2) {
        return RetFalse("ExWriteDataDBFunc key err1");
    }

    string key((*retdata.at(0)).begin(), (*retdata.at(0)).end());
    string value((*retdata.at(1)).begin(), (*retdata.at(1)).end());

    CLuaVMRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv) {

        return RetFalse("pVmRunEnv is nullptr");
    }

    const CRegID contractRegId = pVmRunEnv->GetContractRegID();
    bool flag = true;
    CContractDBCache* scriptDB = pVmRunEnv->GetScriptDB();
    string oldValue;
    // TODO: get old data when set data ??
    scriptDB->GetContractData(contractRegId, key, oldValue);
    if (!scriptDB->SetContractData(contractRegId, key, value)) {
        LogPrint("vm", "ExWriteDataDBFunc SetContractData failed, key:%s!\n",HexStr(key));
        lua_BurnStoreUnchanged(L, key.size(), value.size(), BURN_VER_R2);
        flag = false;
    } else {
        lua_BurnStoreSet(L, key.size(), oldValue.size(), value.size(), BURN_VER_R2);
    }
    return RetRstBooleanToLua(L,flag);
}

/**
 *bool DeleteDataDB(const void* const key,const uint8_t keylen)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是 key值
 */
int32_t ExDeleteDataDBFunc(lua_State *L) {
    vector<std::shared_ptr < vector<uint8_t> > > retdata;

    if (!GetDataString(L, retdata) || retdata.size() != 1) {
        LogPrint("vm", "ExDeleteDataDBFunc key err1");
        return RetFalse(string(__FUNCTION__) + "para  err !");
    }
    string key = string((*retdata.at(0)).begin(), (*retdata.at(0)).end());

    CLuaVMRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv) {
        return RetFalse("pVmRunEnv is nullptr");
    }

    CRegID contractRegId       = pVmRunEnv->GetContractRegID();
    CContractDBCache *scriptDB = pVmRunEnv->GetScriptDB();

    bool flag = true;
    string oldValue;
    // TODO: get old data when set data ??
    scriptDB->GetContractData(contractRegId, key, oldValue);

    if (!scriptDB->EraseContractData(contractRegId, key)) {
        LogPrint("vm", "ExDeleteDataDBFunc EraseContractData railed, key:%s!\n", HexStr(*retdata.at(0)));
        lua_BurnStoreUnchanged(L, key.size(), oldValue.size(), BURN_VER_R2);
        flag = false;
    } else {
        lua_BurnStoreSet(L, key.size(), oldValue.size(), 0, BURN_VER_R2);
    }

    return RetRstBooleanToLua(L, flag);
}

/**
 *uint16_t ReadDataValueDB(const void* const key,const uint8_t keylen, void* const value,uint16_t const maxbuffer)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是 key值
 */
int32_t ExReadDataDBFunc(lua_State *L) {
    vector<std::shared_ptr < vector<uint8_t> > > retdata;

    if (!GetDataString(L,retdata) ||retdata.size() != 1) {
        return RetFalse("ExReadDataDBFunc key err1");
    }

    string key((*retdata.at(0)).begin(), (*retdata.at(0)).end());

    CLuaVMRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv) {
        return RetFalse("pVmRunEnv is nullptr");
    }

    CRegID scriptRegId = pVmRunEnv->GetContractRegID();

    string value;
    CContractDBCache* scriptDB = pVmRunEnv->GetScriptDB();
    int32_t len = 0;
    if (!scriptDB->GetContractData(scriptRegId, key, value)) {
        len = 0;
        lua_BurnStoreUnchanged(L, key.size(), 0, BURN_VER_R2);
    } else {
        lua_BurnStoreGet(L, key.size(), value.size(), BURN_VER_R2);
        len = RetRstToLua(L, vector<uint8_t>(value.begin(), value.end()));
    }
    return len;
}

int32_t ExGetCurTxHash(lua_State *L) {

    CLuaVMRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv)
        return RetFalse("pVmRunEnv is nullptr");

    LUA_BurnFuncCall(L, FUEL_CALL_GetCurTxHash, BURN_VER_R2);
    uint256 hash = pVmRunEnv->GetCurTxHash();
    CDataStream tep(SER_DISK, CLIENT_VERSION);
    tep << hash;
    vector<uint8_t> tep1(tep.begin(),tep.end());
    vector<uint8_t> tep2(tep1.rbegin(),tep1.rend());
    return RetRstToLua(L,tep2);
}

/**
 *bool ExModifyDataDBFunc(const void* const key,const uint8_t keylen, const void* const pvalue,const uint16_t valuelen)
 * 中间层传了两个参数
 * 1.第一个是 key
 * 2.第二个是 value
 */
int32_t ExModifyDataDBFunc(lua_State *L) {
    vector<std::shared_ptr < vector<uint8_t> > > retdata;
    if (!GetDataTableWriteDataDB(L,retdata) ||retdata.size() != 2) {
        return RetFalse("ExModifyDataDBFunc key err");
    }

    string key((*retdata.at(0)).begin(), (*retdata.at(0)).end());
    string newValue((*retdata.at(1)).begin(), (*retdata.at(1)).end());

    CLuaVMRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv) {
        return RetFalse("pVmRunEnv is nullptr");
    }

    CRegID contractRegId = pVmRunEnv->GetContractRegID();
    CContractDBCache* scriptDB = pVmRunEnv->GetScriptDB();
    string oldValue;
    bool flag = false;
    if (scriptDB->GetContractData(contractRegId, key, oldValue)) {
        if (scriptDB->SetContractData(contractRegId, key, newValue)) {
            lua_BurnStoreSet(L, key.size(),  oldValue.size(), newValue.size(), BURN_VER_R2);
            flag = true;
        } else {
            lua_BurnStoreUnchanged(L, key.size(), newValue.size(), BURN_VER_R2);
        }
    } else {
        lua_BurnStoreUnchanged(L, key.size(), newValue.size(), BURN_VER_R2);
    }

    return RetRstBooleanToLua(L,flag);
}


static bool GetDataTableWriteOutput(lua_State *L, CVmOperate &operate) {
    if (!lua_istable(L,-1)) {
        LogPrint("vm","WriteOutput(), param 1 must be table\n");
        return false;
    }

    double doubleValue = 0;
    uint16_t len = 0;
    vector<uint8_t> vBuf ;
    if (!(getNumberInTable(L,(char *)"addrType",doubleValue))) {
        LogPrint("vm", "WriteOutput(), get addrType failed\n");
        return false;
    } else {
        operate.accountType = (AccountType)doubleValue;
    }

    if (operate.accountType == AccountType::REGID) {
       len = 6;
    } else if (operate.accountType == AccountType::BASE58ADDR){
       len = 34;
    } else {
        LogPrint("vm", "WriteOutput(), invalid accountType: %d\n", operate.accountType);
        return false;
    }

    if (!getArrayInTable(L, "accountIdTbl", len, vBuf)) {
        LogPrint("vm","WriteOutput(), get accountIdTbl failed\n");
        return false;
    } else {
       memcpy(operate.accountId,&vBuf[0],len);
    }

    if (!(getNumberInTable(L, "operatorType", doubleValue))) {
        LogPrint("vm", "WriteOutput(),  get opType failed\n");
        return false;

    } else {
        operate.opType = (BalanceOpType) doubleValue;
    }

    if (!(getNumberInTable(L, "outHeight", doubleValue))) {
        LogPrint("vm", "WriteOutput(),  get outheight failed\n");
        return false;
    } else {
        operate.timeoutHeight = (uint32_t)doubleValue;
    }

    if (!getArrayInTable(L, "moneyTbl", sizeof(operate.money), vBuf)) {
        LogPrint("vm","WriteOutput(), moneyTbl not table\n");
        return false;
    } else {
        memcpy(operate.money, &vBuf[0], sizeof(operate.money));
    }
    return true;
}

/**
 * contract api - lua function
 * bool WriteOutput( vmOperTable )
 * @param vmOperTable:          operation param table
 * {
 *      addrType: (number, required),     address type of accountIdTbl, enum(REGID=1,ADDR=2)
 *      accountIdTbl: (array, required)   account id, array format
 *      operatorType: (number, required)  operator type, enum(ADD_FREE=1, SUB_FREE=2)
 *      outHeight: (number, required)     timeout height, use by contract script
 *      moneyTbl: (array, required)       money amount, serialized format of int64 (little endian)
 *      moneySymbol: (string, optional)   money symbol, must be valid symbol, such as WICC|WUSD, default is WICC
 * }
 * @return write succeed or not
 */
int32_t ExWriteOutputFunc(lua_State *L) {
    CVmOperate operateIn;
    if (!GetDataTableWriteOutput(L, operateIn))
        return RetFalse("WriteOutput(), parse params failed");

    CLuaVMRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv)
        return RetFalse("WriteOutput(), pVmRunEnv is nullptr");

    LUA_BurnAccountOperate(L, 1, BURN_VER_R2);

    if (!pVmRunEnv->InsertOutputData({operateIn})) {
         return RetFalse("WriteOutput(), InsertOutputData failed");
    }

    return RetRstBooleanToLua(L,true);
}

static bool GetDataTableGetContractData(lua_State *L, vector<std::shared_ptr<std::vector<uint8_t>>> &ret) {
    if (!lua_istable(L,-1)) {
        LogPrint("vm", "GetDataTableGetContractData is not table\n");
        return false;
    }

    vector<uint8_t> vBuf ;
    //取脚本id
    if (!getArrayInTable(L,(char *)"id",6,vBuf)) {
        LogPrint("vm","idTbl not table\n");
        return false;
    } else {
       ret.insert(ret.end(),std::make_shared<vector<uint8_t>>(vBuf.begin(), vBuf.end()));
    }

    //取key
    string key = "";
    if (!(getStringInTable(L,(char *)"key",key))) {
        LogPrint("vm","key get fail\n");
        return false;
    } else {
        // LogPrint("vm", "key:%s\n", key);
    }

    vBuf.clear();
    for (size_t i = 0;i < key.size();i++) {
        vBuf.insert(vBuf.end(),key.at(i));
    }

    ret.insert(ret.end(),std::make_shared<vector<uint8_t>>(vBuf.begin(), vBuf.end()));
    return true;
}

/**
 *bool GetContractData(const void* const scriptID,void* const pkey,short len,void* const pValue,short maxlen)
 * 中间层传了两个个参数
 * 1.脚本的id号
 * 2.数据库的key值
 */
int32_t ExGetContractDataFunc(lua_State *L) {
    vector<std::shared_ptr<vector<uint8_t>>> retdata;

    if (!GetDataTableGetContractData(L, retdata) || retdata.size() != 2 || retdata.at(0).get()->size() != 6)
        return RetFalse("ExGetContractDataFunc tep1 err1");

    CLuaVMRunEnv *pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv)
        return RetFalse("pVmRunEnv is nullptr");

    CContractDBCache *scriptDB = pVmRunEnv->GetScriptDB();
    CRegID contractRegId(*retdata.at(0));
    string key((*retdata.at(1)).begin(), (*retdata.at(1)).end());
    string value;

    int32_t len = 0;
    if (!scriptDB->GetContractData(contractRegId, key, value)) {
        len = 0;
        lua_BurnStoreUnchanged(L, key.size(), 0, BURN_VER_R2);
    } else {
        lua_BurnStoreGet(L, key.size(), value.size(), BURN_VER_R2);
        len = RetRstToLua(L, vector<uint8_t>(value.begin(), value.end()));
    }
    /*
     * 每个函数里的Lua栈是私有的,当把返回值压入Lua栈以后，该栈会自动被清空*/
    return len;
}

/**
 * 取目的账户ID
 * @param ipara
 * @param pVmEvn
 * @return
 */
int32_t ExGetContractRegIdFunc(lua_State *L) {
    CLuaVMRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv)
        return RetFalse("pVmRunEnv is nullptr");

    LUA_BurnFuncCall(L, FUEL_CALL_GetContractRegId, BURN_VER_R2);
   //1.从lua取参数
   //2.调用C++库函数 执行运算
    UnsignedCharArray contractRegId = pVmRunEnv->GetContractRegID().GetRegIdRaw();
   //3.往函数私有栈里存运算后的结果
    int32_t len = RetRstToLua(L,contractRegId);
   /*
    * 每个函数里的Lua栈是私有的,当把返回值压入Lua栈以后，该栈会自动被清空*/
    return len; //number of results 告诉Lua返回了几个返回值
}

int32_t ExGetCurTxAccountFunc(lua_State *L) {
    CLuaVMRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv)
        return RetFalse("pVmRunEnv is nullptr");

    LUA_BurnFuncCall(L, FUEL_CALL_GetCurTxAccount, BURN_VER_R2);
   //1.从lua取参数
   //2.调用C++库函数 执行运算
    UnsignedCharArray vUserId =pVmRunEnv->GetTxUserRegid().GetRegIdRaw();

   //3.往函数私有栈里存运算后的结果
    int32_t len = RetRstToLua(L,vUserId);
   /*
    * 每个函数里的Lua栈是私有的,当把返回值压入Lua栈以后，该栈会自动被清空*/
    return len; //number of results 告诉Lua返回了几个返回值
}

int32_t ExGetCurTxPayAmountFunc(lua_State *L) {
    CLuaVMRunEnv *pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv)
        return RetFalse("pVmRunEnv is nullptr");

    LUA_BurnFuncCall(L, FUEL_CALL_GetCurTxPayAmount, BURN_VER_R2);
    uint64_t lvalue = pVmRunEnv->GetValue();

    CDataStream tep(SER_DISK, CLIENT_VERSION);
    tep << lvalue;
    vector<uint8_t> tep1(tep.begin(), tep.end());
    int32_t len = RetRstToLua(L, tep1);
    /*
     * 每个函数里的Lua栈是私有的,当把返回值压入Lua栈以后，该栈会自动被清空*/
    return len;  // number of results 告诉Lua返回了几个返回值
}

int32_t ExGetUserAppAccValueFunc(lua_State *L) {
    vector<std::shared_ptr < vector<uint8_t> > > retdata;
    if (!lua_istable(L, -1)) {
        LogPrint("vm", "is not table\n");
        return 0;
    }
    double doubleValue = 0;
    uint32_t idlen = 0;
    vector<uint8_t> accountId;
    memset(&accountId, 0, sizeof(accountId));
    if (!(getNumberInTable(L, "idLen", doubleValue))) {
        LogPrint("vm", "get idlen failed\n");
        return 0;
    } else {
        idlen = (uint8_t)doubleValue;
    }
    if ((idlen < 1) || (idlen > CAppCFund::MAX_TAG_SIZE)) {
        LogPrint("vm","idlen invalid\n");
        return 0;
    }

    if (!getArrayInTable(L, "idValueTbl", idlen, accountId)) {
        LogPrint("vm", "idValueTbl not table\n");
        return 0;
    }

    CLuaVMRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if(nullptr == pVmRunEnv)
        return RetFalse("pVmRunEnv is nullptr");

    shared_ptr<CAppUserAccount> appAccount;
    uint64_t valueData = 0 ;
    int32_t len = 0;
    LUA_BurnAccount(L, FUEL_ACCOUNT_GET_VALUE, BURN_VER_R2);
    if (pVmRunEnv->GetAppUserAccount(accountId, appAccount)) {
        valueData = appAccount->GetBcoins();

        CDataStream tep(SER_DISK, CLIENT_VERSION);
        tep << valueData;
        vector<uint8_t> TMP(tep.begin(), tep.end());
        len = RetRstToLua(L, TMP);
    }
    return len;
}

static bool GetDataTableOutAppOperate(lua_State *L, vector<std::shared_ptr<std::vector<uint8_t>>> &ret) {
    if (!lua_istable(L, -1)) {
        LogPrint("vm","is not table\n");
        return false;
    }
    double doubleValue = 0;
    vector<uint8_t> vBuf ;
    CAppFundOperate temp;
    memset(&temp,0,sizeof(temp));
    if (!(getNumberInTable(L,(char *)"operatorType",doubleValue))) {
        LogPrint("vm", "opType get fail\n");
        return false;
    } else {
        temp.opType = (uint8_t)doubleValue;
    }

    if (!(getNumberInTable(L, (char *)"outHeight", doubleValue))) {
        LogPrint("vm", "outHeight get fail\n");
        return false;
    } else {
        temp.timeoutHeight = (uint32_t) doubleValue;
    }

    if (!getArrayInTable(L, (char *)"moneyTbl", sizeof(temp.mMoney), vBuf)) {
        LogPrint("vm", "moneyTbl not table\n");
        return false;
    } else {
        memcpy(&temp.mMoney, &vBuf[0], sizeof(temp.mMoney));
    }

    if (!(getNumberInTable(L, (char *) "userIdLen", doubleValue))) {
        LogPrint("vm", "appuserIDlen get fail\n");
        return false;
    } else {
        temp.appuserIDlen = (uint8_t) doubleValue;
    }

    if ((temp.appuserIDlen < 1) || (temp.appuserIDlen > sizeof(temp.vAppuser))) {
        LogPrint("vm", "appuserIDlen is err\n");
        return false;
    }

    if (!getArrayInTable(L,(char *)"userIdTbl",temp.appuserIDlen,vBuf)) {
        LogPrint("vm", "useridTbl not table\n");
        return false;
    } else {
        memcpy(temp.vAppuser,&vBuf[0],temp.appuserIDlen);
    }

    if (!(getNumberInTable(L,(char *)"fundTagLen",doubleValue))) {
        LogPrint("vm", "fundTagLen get fail\n");
        return false;
    } else {
        temp.fundTagLen = (uint8_t)doubleValue;
    }

    if ((temp.fundTagLen > 0) && (temp.fundTagLen <= sizeof(temp.vFundTag))) {
        if (!getArrayInTable(L, (char *)"fundTagTbl", temp.fundTagLen, vBuf)) {
            LogPrint("vm", "FundTagTbl not table\n");
            return false;
        } else {
            memcpy(temp.vFundTag, &vBuf[0], temp.fundTagLen);
        }
    }

    CDataStream tep(SER_DISK, CLIENT_VERSION);
    tep << temp;
    vector<uint8_t> tep1(tep.begin(),tep.end());
    ret.insert(ret.end(),std::make_shared<vector<uint8_t>>(tep1.begin(), tep1.end()));

    return true;
}

int32_t ExGetUserAppAccFundWithTagFunc(lua_State *L) {
    vector<std::shared_ptr<vector<uint8_t>>> retdata;
    CAppFundOperate temp;
    uint32_t size = ::GetSerializeSize(temp, SER_NETWORK, PROTOCOL_VERSION);

    if (!GetDataTableOutAppOperate(L,retdata) ||retdata.size() != 1 || retdata.at(0).get()->size() != size)
        return RetFalse("ExGetUserAppAccFundWithTagFunc para err0");

    CLuaVMRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv)
        return RetFalse("pVmRunEnv is nullptr");

    CDataStream ss(*retdata.at(0), SER_DISK, CLIENT_VERSION);
    CAppFundOperate userfund;
    ss >> userfund;

    shared_ptr<CAppUserAccount> appAccount;
    CAppCFund fund;
    int32_t len = 0;
    LUA_BurnAccount(L, FUEL_ACCOUNT_GET_FUND_TAG, BURN_VER_R2);
    if (pVmRunEnv->GetAppUserAccount(userfund.GetAppUserV(), appAccount)) {
        if (!appAccount->GetAppCFund(fund,userfund.GetFundTagV(), userfund.timeoutHeight))
            return RetFalse("GetUserAppAccFundWithTag GetAppCFund fail");

        CDataStream tep(SER_DISK, CLIENT_VERSION);
        tep << fund.GetValue() ;
        vector<uint8_t> TMP(tep.begin(),tep.end());
        len = RetRstToLua(L,TMP);
    }

    return len;
}

static bool GetDataTableAssetOperate(lua_State *L, int32_t index, vector<std::shared_ptr<std::vector<uint8_t>>> &ret) {
    if (!lua_istable(L, index)) {
        LogPrint("vm", "L is not table\n");
        return false;
    }

    double doubleValue = 0;
    vector<uint8_t> vBuf ;
    CAssetOperate temp;
    memset(&temp,0,sizeof(temp));

    if (!getArrayInTable(L,(char *)"toAddrTbl",34,vBuf)) {
        LogPrint("vm","toAddrTbl not table\n");
        return false;
    } else {
        ret.push_back(std::make_shared<vector<uint8_t>>(vBuf.begin(), vBuf.end()));
    }

    if (!(getNumberInTable(L, (char *) "outHeight", doubleValue))) {
        LogPrint("vm", "get timeoutHeight failed\n");
        return false;
    } else {
        temp.timeoutHeight = (uint32_t) doubleValue;
        LogPrint("vm", "height = %d", temp.timeoutHeight);
    }

    if (!getArrayInTable(L, (char *) "moneyTbl", sizeof(temp.mMoney), vBuf)) {
        LogPrint("vm", "moneyTbl not table\n");
        return false;
    } else {
       memcpy(&temp.mMoney,&vBuf[0],sizeof(temp.mMoney));
    }

    if (!(getNumberInTable(L,(char *)"fundTagLen",doubleValue))) {
        LogPrint("vm", "fundTagLen get fail\n");
        return false;
    } else {
        temp.fundTagLen = (uint8_t)doubleValue;
    }

    if ((temp.fundTagLen > 0) && (temp.fundTagLen <= sizeof(temp.vFundTag))) {
        if (!getArrayInTable(L,(char *)"fundTagTbl",temp.fundTagLen,vBuf)) {
            LogPrint("vm","FundTagTbl not table\n");
            return false;
        } else {
            memcpy(temp.vFundTag,&vBuf[0],temp.fundTagLen);
        }
    }

    CDataStream tep(SER_DISK, CLIENT_VERSION);
    tep << temp;
    vector<uint8_t> tep1(tep.begin(),tep.end());
    ret.insert(ret.end(),std::make_shared<vector<uint8_t>>(tep1.begin(), tep1.end()));
    return true;
}

/**
 * 写应用操作输出到 pVmRunEnv->mapAppFundOperate[0]
 * @param ipara
 * @param pVmEvn
 * @return
 */
int32_t ExWriteOutAppOperateFunc(lua_State *L) {
    vector<std::shared_ptr<vector<uint8_t>>> retdata;

    CAppFundOperate temp;
    uint32_t size = ::GetSerializeSize(temp, SER_NETWORK, PROTOCOL_VERSION);

    if (!GetDataTableOutAppOperate(L, retdata) || retdata.size() != 1 || (retdata.at(0).get()->size() % size) != 0)
        return RetFalse("ExWriteOutAppOperateFunc para err1");

    CLuaVMRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv)
        return RetFalse("pVmRunEnv is nullptr");

    int32_t count = retdata.at(0).get()->size() / size;
    CDataStream ss(*retdata.at(0), SER_DISK, CLIENT_VERSION);
    LUA_BurnAccountOperate(L, count, BURN_VER_R2);

    int64_t step =-1;
    while (count--) {
        ss >> temp;
        // soft fork for contract negative money
        if (GetFeatureForkVersion(pVmRunEnv->GetConfirmHeight()) >= MAJOR_VER_R2 &&
            temp.mMoney < 0)  // in case contract uses negative money input
            return RetFalse("ExWriteOutAppOperateFunc para err2");

        pVmRunEnv->InsertOutAPPOperte(temp.GetAppUserV(),temp);
        step += size;
    }

    /*
    * 每个函数里的Lua栈是私有的,当把返回值压入Lua栈以后，该栈会自动被清空*/
    return RetRstBooleanToLua(L, true);
}

int32_t ExGetBase58AddrFunc(lua_State *L) {
    vector<std::shared_ptr<vector<uint8_t>>> retdata;

    if (!GetArray(L, retdata) || retdata.size() != 1 || retdata.at(0).get()->size() != 6)
        return RetFalse("ExGetBase58AddrFunc para err0");

    CLuaVMRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv)
        return RetFalse("pVmRunEnv is nullptr");
/*
    vector<uint8_t> recvKey;
    recvKey.assign(retdata.at(0).get()->begin(), retdata.at(0).get()->end());

    std::string recvaddr( recvKey.begin(), recvKey.end() );

    for(int32_t i = 0; i < recvKey.size(); i++)
        LogPrint("vm", "==============%02X\n", recvKey[i]);

    vector<uint8_t> tmp;
    for(int32_t i = recvKey.size() - 1; i >=0 ; i--)
        tmp.push_back(recvKey[i]);
*/

    LUA_BurnFuncCall(L, FUEL_CALL_GetBase58Addr, BURN_VER_R2);
    CKeyID addrKeyId;
    if (!GetKeyId(*pVmRunEnv->GetCatchView(), *retdata.at(0).get(), addrKeyId))
        return RetFalse("ExGetBase58AddrFunc para err1");

    string addr = addrKeyId.ToAddress();

    vector<uint8_t> vTemp;
    vTemp.assign(addr.c_str(), addr.c_str() + addr.length());
    return RetRstToLua(L, vTemp);
}

int32_t ExTransferContractAsset(lua_State *L) {
    vector<std::shared_ptr<vector<uint8_t>>> retdata;

    if (!GetArray(L,retdata) ||retdata.size() != 1 || retdata.at(0).get()->size() != 34)
        return RetFalse(string(__FUNCTION__)+"para  err !");

    CLuaVMRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv)
        return RetFalse("pVmRunEnv is nullptr");

    vector<uint8_t> sendKey;
    vector<uint8_t> recvKey;
    CRegID script = pVmRunEnv->GetContractRegID();

    CRegID sendRegID =pVmRunEnv->GetTxUserRegid();
    CKeyID SendKeyID = sendRegID.GetKeyId(*pVmRunEnv->GetCatchView());
    string addr = SendKeyID.ToAddress();
    sendKey.assign(addr.c_str(),addr.c_str()+addr.length());

    recvKey.assign(retdata.at(0).get()->begin(), retdata.at(0).get()->end());

    std::string recvaddr( recvKey.begin(), recvKey.end() );
    if (addr == recvaddr) {
        LogPrint("vm", "%s\n", "send addr and recv addr is same !");
        return RetFalse(string(__FUNCTION__)+"send addr and recv addr is same !");
    }

    CKeyID RecvKeyID;
    bool bValid = GetKeyId(*pVmRunEnv->GetCatchView(), recvKey, RecvKeyID);
    if (!bValid) {
        LUA_BurnAccount(L, FUEL_ACCOUNT_UNCHANGED, BURN_VER_R2);
        LogPrint("vm", "%s\n", "recv addr is not valid !");
        return RetFalse(string(__FUNCTION__)+"recv addr is not valid !");
    }

    std::shared_ptr<CAppUserAccount> temp = std::make_shared<CAppUserAccount>();
    CContractDBCache* pContractScript = pVmRunEnv->GetScriptDB();

    if (!pContractScript->GetContractAccount(script, string(sendKey.begin(), sendKey.end()), *temp.get())) {
        LUA_BurnAccount(L, FUEL_ACCOUNT_UNCHANGED, BURN_VER_R2);
        return RetFalse(string(__FUNCTION__) + "para  err3 !");
    }

    temp.get()->AutoMergeFreezeToFree(chainActive.Height());

    uint64_t nMoney = temp.get()->GetBcoins();

    int32_t count = 0;
    int32_t i = 0;
    CAppFundOperate op;
    memset(&op, 0, sizeof(op));

    if (nMoney > 0) {
        op.mMoney = nMoney;
        op.timeoutHeight = 0;
        op.opType = SUB_FREE_OP;
        op.appuserIDlen = sendKey.size();
        for (i = 0; i < op.appuserIDlen; i++)
            op.vAppuser[i] = sendKey[i];

        pVmRunEnv->InsertOutAPPOperte(sendKey, op);

        op.opType = ADD_FREE_OP;
        op.appuserIDlen = recvKey.size();
        for (i = 0; i < op.appuserIDlen; i++)
            op.vAppuser[i] = recvKey[i];

        pVmRunEnv->InsertOutAPPOperte(recvKey, op);
        count += 2;
    }

    vector<CAppCFund> vTemp = temp.get()->GetFrozenFunds();
    for (auto fund : vTemp) {
        op.mMoney = fund.GetValue();
        op.timeoutHeight = fund.GetHeight();
        op.opType = SUB_TAG_OP;
        op.appuserIDlen = sendKey.size();
        for (i = 0; i < op.appuserIDlen; i++) {
            op.vAppuser[i] = sendKey[i];
        }

        op.fundTagLen = fund.GetTag().size();
        for (i = 0; i < op.fundTagLen; i++) {
            op.vFundTag[i] = fund.GetTag()[i];
        }

        pVmRunEnv->InsertOutAPPOperte(sendKey, op);

        op.opType = ADD_TAG_OP;
        op.appuserIDlen = recvKey.size();
        for (i = 0; i < op.appuserIDlen; i++) {
            op.vAppuser[i] = recvKey[i];
        }

        pVmRunEnv->InsertOutAPPOperte(recvKey, op);
        count += 2;
    }
    LUA_BurnAccountOperate(L, count, BURN_VER_R2);

    return RetRstBooleanToLua(L, true);
}

int32_t ExTransferSomeAsset(lua_State *L) {
    vector<std::shared_ptr < vector<uint8_t> > > retdata;

    CAssetOperate tempAsset;
    uint32_t size = ::GetSerializeSize(tempAsset, SER_NETWORK, PROTOCOL_VERSION);

    if (!GetDataTableAssetOperate(L, -1, retdata) || retdata.size() != 2 || (retdata.at(1).get()->size() % size) != 0 ||
        retdata.at(0).get()->size() != 34)
        return RetFalse(string(__FUNCTION__) + "para err !");

    CLuaVMRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv)
        return RetFalse("pVmRunEnv is nullptr");

    CDataStream ss(*retdata.at(1), SER_DISK, CLIENT_VERSION);
    CAssetOperate assetOp;
    ss >> assetOp;

    vector<uint8_t> sendKey;
    vector<uint8_t> recvKey;
    CRegID script = pVmRunEnv->GetContractRegID();

    CRegID sendRegID = pVmRunEnv->GetTxUserRegid();
    CKeyID SendKeyID = sendRegID.GetKeyId(*pVmRunEnv->GetCatchView());
    string addr      = SendKeyID.ToAddress();
    sendKey.assign(addr.c_str(), addr.c_str() + addr.length());

    recvKey.assign(retdata.at(0).get()->begin(), retdata.at(0).get()->end());

    std::string recvaddr( recvKey.begin(), recvKey.end() );
    if (addr == recvaddr) {
        LogPrint("vm", "%s\n", "send addr and recv addr is same !");
        return RetFalse(string(__FUNCTION__)+"send addr and recv addr is same !");
    }

    CKeyID RecvKeyID;
    bool bValid = GetKeyId(*pVmRunEnv->GetCatchView(), recvKey, RecvKeyID);
    if (!bValid) {
        LogPrint("vm", "%s\n", "recv addr is not valid !");
        return RetFalse(string(__FUNCTION__)+"recv addr is not valid !");
    }

    uint64_t uTransferMoney = assetOp.GetUint64Value();
    if (0 == uTransferMoney)
        return RetFalse(string(__FUNCTION__) + " Transfer Money is not valid !");

    int32_t height = assetOp.GetHeight();
    if (height < 0)
        return RetFalse(string(__FUNCTION__) + " outHeight is not valid !");

    int32_t i = 0;
    CAppFundOperate op;
    memset(&op, 0, sizeof(op));
    vector<uint8_t> vtag = assetOp.GetFundTagV();
    op.fundTagLen = vtag.size();

    for (i = 0; i < op.fundTagLen; i++) {
        op.vFundTag[i] = vtag[i];
    }

    op.mMoney = uTransferMoney;
    op.timeoutHeight = height;
    op.appuserIDlen = sendKey.size();

    for (i = 0; i < op.appuserIDlen; i++) {
        op.vAppuser[i] = sendKey[i];
    }

    op.opType = (height > 0) ? SUB_TAG_OP : SUB_FREE_OP;
    pVmRunEnv->InsertOutAPPOperte(sendKey, op);

    op.opType = (height > 0) ? ADD_TAG_OP : ADD_FREE_OP ;
    op.appuserIDlen = recvKey.size();

    for (i = 0; i < op.appuserIDlen; i++) {
        op.vAppuser[i] = recvKey[i];
    }

    pVmRunEnv->InsertOutAPPOperte(recvKey, op);
    LUA_BurnAccountOperate(L, 2, BURN_VER_R2);
    return RetRstBooleanToLua(L, true);

}

int32_t ExGetBlockTimestamp(lua_State *L) {
    CLuaVMRunEnv* pLuaVMRunEnv = GetVmRunEnvByContext(L);
    int32_t height = 0;
    if (!GetDataInt(L,height))
        return RetFalse("ExGetBlockTimestamp para err1");

    // only support to get current block time
    if (height != 0) {
        LogPrint("vm", "[ERROR]ExGetBlockTimestamp(), the input height=%d must be 0\n", height);
        return 0;
    }

    LUA_BurnFuncCall(L, FUEL_CALL_GetBlockTimestamp, BURN_VER_R2);

    CLuaVMContext &vmContext = pLuaVMRunEnv->GetContext();
    auto featureForkVersion = GetFeatureForkVersion(vmContext.height);

    lua_Integer blockTime = 0;
    if (featureForkVersion == MAJOR_VER_R1) {
        // compact with old data
        blockTime = vmContext.prev_block_time;
    } else {
        blockTime = vmContext.block_time;
    }

    if (!lua_checkstack(L, sizeof(lua_Integer))) {
        LogPrint("vm", "[ERROR]ExGetBlockTimestamp(), lua stack overflow! input_height=%d, curBlockHeight=%d\n",
            height, vmContext.height);
        return 0;
    }

    lua_pushinteger(L, blockTime);
    return 1;
}


int32_t ExLimitedRequire(lua_State *L) {
    const char* name = luaL_checkstring(L, 1);
    if (strcmp(name, "mylib") != 0) {
        return luaL_error(L, "Only supports to require \"mylib\"");
    }
    /* mylib is already loaded, just load it from "_LOADED" */
    lua_settop(L, 1);  /* _LOADED table will be at index 2 */
    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
    lua_getfield(L, 2, name);  /* _LOADED[name] */
    if (!lua_toboolean(L, -1)) { /* is it there? */
        return luaL_error(L, "require \"mylib\" failed!");
    }

    return 1;
}

int32_t ExLuaPrint(lua_State *L) {
    int32_t n = lua_gettop(L); /* number of arguments */
    int32_t i;
    std::string str = "";
    lua_getglobal(L, "tostring");
    for (i = 1; i <= n; i++) {
        const char *s;
        size_t l;
        lua_pushvalue(L, -1); /* function to be called */
        lua_pushvalue(L, i);  /* value to print */
        lua_call(L, 1, 1);
        s = lua_tolstring(L, -1, &l); /* get result */
        if (s == nullptr)
            return luaL_error(L, "'tostring' must return a string to 'print'");

        if (i == 1) {
            str = std::string(s, l);
        } else {
            str += "\t" + std::string(s, l);
        }
        lua_pop(L, 1); /* pop result */
    }

    LUA_BurnFuncData(L, FUEL_CALL_LogPrint, str.size(), 1, FUEL_DATA1_LogPrint, BURN_VER_R2);
    LogPrint("vm", "%s\n", str);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// lua contract api function added in MAJOR_VER_R2

static bool ParseAccountAssetTransfer(lua_State *L, CLuaVMRunEnv &vmRunEnv, AssetTransfer &transfer) {
    if (!lua_istable(L,-1)) {
        LogPrint("vm","ParseAccountAssetTransfer(), transfer param must be table\n");
        return false;
    }

    if (!(GetBoolInTable(L, "isContractAccount", transfer.isContractAccount))) {
        LogPrint("vm", "ParseAccountAssetTransfer(), get isContractAccount failed\n");
        return false;
    }

    AccountType uidType;
    if (!(ParseUidTypeInTable(L, "toAddressType", uidType))) {
        LogPrint("vm", "ParseAccountAssetTransfer(), get toAddressType failed\n");
        return false;
    }

    if (!ParseUidInTable(L, "toAddress", uidType, transfer.toUid)) {
        LogPrint("vm","ParseAccountAssetTransfer(), get toAddress failed\n");
        return false;
    }

    if (!(getStringInTable(L, "tokenType", transfer.tokenType))) {
        LogPrint("vm", "ParseAccountAssetTransfer(), get tokenType failed\n");
        return false;
    }

    auto pErr = vmRunEnv.GetCw()->assetCache.CheckTransferCoinSymbol(transfer.tokenType);
    if (pErr) {
        LogPrint("vm", "ParseAccountAssetTransfer(), Invalid tokenType=%s! %s \n", transfer.tokenType, *pErr);
        return false;
    }

    vector<uint8_t> amountVector;
    if (!getArrayInTable(L, "tokenAmount", sizeof(uint64_t), amountVector)) {
        LogPrint("vm", "ParseAccountAssetTransfer(), get tokenAmount failed\n");
        return false;
    };
    assert(amountVector.size() == sizeof(uint64_t));

    int64_t amount = 0;
    CDataStream ssAmount(amountVector, SER_DISK, CLIENT_VERSION);
    ssAmount >> amount;

    if (amount == 0 || !CheckBaseCoinRange(amount) ) {
        LogPrint("vm", "ParseAccountAssetTransfer(), tokenAmount=%lld is 0 or out of range\n", amount);
        return false;
    }
    transfer.tokenAmount = amount;

    return true;
}

int32_t ExTransferAccountAssetFunc(lua_State *L) {
    CLuaVMRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv) {
        LogPrint("vm","[ERROR]%s(), pVmRunEnv is nullptr", __FUNCTION__);
        return 0;
    }

    AssetTransfer transfer;
    if (!ParseAccountAssetTransfer(L, *pVmRunEnv, transfer)) {
        LogPrint("vm","[ERROR]%s(), parse params of TransferAccountAsset function failed", __FUNCTION__);
        return 0;
    }

    if (!pVmRunEnv->TransferAccountAsset(L, {transfer})) {
        LogPrint("vm","[ERROR]%s(), execute pVmRunEnv->TransferAccountAsset() failed", __FUNCTION__);
        return 0;
    }

    return RetRstBooleanToLua(L, true);
}

static bool ParseAccountAssetTransfers(lua_State *L, CLuaVMRunEnv &vmRunEnv, vector<AssetTransfer> &transfers) {
    if (!lua_istable(L, -1)) {
        LogPrint("vm","[ERROR]%s(), transfers param must be table\n", __FUNCTION__);
        return false;
    }
    //默认栈顶是table，将key入栈
    size_t sz = lua_rawlen(L, -1);
    if (sz == 0) {
        LogPrint("vm","ParseAccountAssetTransfers(), transfers param is empty table\n");
        return false;
    }
    for (size_t i = 1; i <= sz; i++) {
        AssetTransfer transfer;
        lua_geti(L, -1, i);
        if (!ParseAccountAssetTransfer(L, vmRunEnv, transfer)) {
            LogPrint("vm","ParseAccountAssetTransfers(), ParseAccountAssetTransfer[%d] failed\n", i);
            lua_pop(L, 1); // pop the read item
            return false;
        }
        lua_pop(L, 1); // pop the read item
        transfers.push_back(transfer);
    }

    return true;
}

int32_t ExTransferAccountAssetsFunc(lua_State *L) {

    CLuaVMRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv) {
        LogPrint("vm","[ERROR]%s(), pVmRunEnv is nullptr", __FUNCTION__);
        return 0;
    }

    // TODO: parse vector
    vector<AssetTransfer> transfers;
    if (!ParseAccountAssetTransfers(L, *pVmRunEnv, transfers)) {
        LogPrint("vm","[ERROR]%s(), parse params of TransferAccountAsset function failed", __FUNCTION__);
        return 0;
    }

    if (!pVmRunEnv->TransferAccountAsset(L, transfers)) {
        LogPrint("vm","[ERROR]%s(), execute pVmRunEnv->TransferAccountAsset() failed", __FUNCTION__);
        return 0;
    }

    return RetRstBooleanToLua(L,true);
}

/**
 * GetCurTxInputAsset - lua api
 * table GetCurTxTransferAsset()
 * get symbol and amount of current tx asset input by sender of tx
 * @return table of asset info:
 * {
 *     symbol: (string), symbol of current tx input asset
 *     amount: (integer)   amount of current tx input asset
 * },
 */
int32_t ExGetCurTxInputAssetFunc(lua_State *L) {

    CLuaVMRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv) {
        LogPrint("vm","[ERROR]%s(), pVmRunEnv is nullptr", __FUNCTION__);
        return 0;
    }

    // check stack to avoid stack overflow
    if (!lua_checkstack(L, 2)) {
        LogPrint("vm", "[ERROR] ExGetCurTxInputAssetFunc(), lua stack overflow\n");
        return 0;
    }

    LUA_BurnFuncCall(L, FUEL_CALL_GetCurTxInputAsset, BURN_VER_R2);

    lua_createtable (L, 0, 2); // create table object with 2 field
    // set asset.symbol
    lua_pushstring(L, pVmRunEnv->GetContext().transfer_symbol.c_str());
    lua_setfield(L, -2, "symbol");

    // set asset.amount
    lua_pushinteger(L, pVmRunEnv->GetContext().transfer_amount);
    lua_setfield(L, -2, "amount");

    return 1;
}

/**
 * GetAccountAsset - lua api
 * boolean GetAccountAsset( transferTable )
 * get asset of account by address
 * @param transferTable:          transfer param table
 * {
 *   addressType: (number, required)       address type, REGID = 1, BASE58 = 2
 *   address: (array, required)            address, array format
 *   tokenType: (string, required)         Token type of the transfer, such as WICC | WUSD
 * }
 * @return asset info table or none
 * {
 *     symbol: (string), transfer symbol of current tx
 *     amount: (integer)   transfer amount of current tx
 * },
 */
int32_t ExGetAccountAssetFunc(lua_State *L) {
    CLuaVMRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (nullptr == pVmRunEnv) {
        LogPrint("vm","[ERROR]%s(), pVmRunEnv is nullptr", __FUNCTION__);
        return 0;
    }

    if (!lua_istable(L,-1)) {
        LogPrint("vm","[ERROR]%s(), input param must be a table\n", __FUNCTION__);
        return 0;
    }

    AccountType uidType;
    if (!(ParseUidTypeInTable(L, "addressType", uidType))) {
        LogPrint("vm", "[ERROR]%s(), get addressType failed\n", __FUNCTION__);
        return 0;
    }

    CUserID uid;
    if (!ParseUidInTable(L, "address", uidType, uid)) {
        LogPrint("vm","[ERROR]%s(), get address failed\n", __FUNCTION__);
        return 0;
    }

    TokenSymbol tokenType;
    if (!(getStringInTable(L, "tokenType", tokenType))) {
        LogPrint("vm", "[ERROR]%s(), get tokenType failed\n", __FUNCTION__);
        return 0;
    }
    auto pErr = pVmRunEnv->GetCw()->assetCache.CheckTransferCoinSymbol(tokenType);
    if (pErr) {
        LogPrint("vm", "[ERROR]%s(), Invalid tokenType=%s! %s \n", __FUNCTION__, tokenType, *pErr);
        return 0;
    }
    LUA_BurnAccount(L, FUEL_ACCOUNT_GET_VALUE, BURN_VER_R2);

    auto pAccount = make_shared<CAccount>();
    if (!pVmRunEnv->GetCw()->accountCache.GetAccount(uid, *pAccount)) {
        LogPrint("vm", "[ERROR]%s(), The account not exist! address=%s\n", __FUNCTION__, uid.ToDebugString());
        return 0;
    }

    uint64_t value;
    if (!pAccount->GetBalance(tokenType, FREE_VALUE, value)) {
        value = 0;
    }

    // check stack to avoid stack overflow
    if (!lua_checkstack(L, 2)) {
        LogPrint("vm", "[ERROR] lua stack overflow\n");
        return 0;
    }
    lua_createtable (L, 0, 2); // create table object with 2 field
    // set asset.symbol
    lua_pushstring(L, tokenType.c_str());
    lua_setfield(L, -2, "symbol");

    // set asset.amount
    lua_pushinteger(L, value);
    lua_setfield(L, -2, "amount");

    return 1;
}

static const luaL_Reg mylib[] = {
    {"Int64Mul",                    ExInt64MulFunc},
    {"Int64Add",                    ExInt64AddFunc},
    {"Int64Sub",                    ExInt64SubFunc},
    {"Int64Div",                    ExInt64DivFunc},
    {"Sha256",                      ExSha256Func},
    {"Sha256Once",                  ExSha256OnceFunc},
    {"Des",                         ExDesFunc},

    {"VerifySignature",             ExVerifySignatureFunc},
    {"LogPrint",                    ExLogPrintFunc},
    {"GetTxContract",               ExGetTxContractFunc},
    {"GetTxRegID",                  ExGetTxRegIDFunc},
    {"GetAccountPublickey",         ExGetAccountPublickeyFunc},
    {"QueryAccountBalance",         ExQueryAccountBalanceFunc},
    {"GetTxConfirmHeight",          ExGetTxConfirmHeightFunc},
    {"GetTxConFirmHeight",          ExGetTxConfirmHeightFunc}, /** deprecated */
    {"GetBlockHash",                ExGetBlockHashFunc},

    {"GetCurTxHash",                ExGetCurTxHash},
    {"GetCurRunEnvHeight",          ExGetCurRunEnvHeightFunc},

    {"WriteData",                   ExWriteDataDBFunc},
    {"ReadData",                    ExReadDataDBFunc},
    {"ModifyData",                  ExModifyDataDBFunc},
    {"DeleteData",                  ExDeleteDataDBFunc},

    {"WriteOutput",                 ExWriteOutputFunc},
    {"GetScriptData",               ExGetContractDataFunc}, /** deprecated */
    {"GetContractData",             ExGetContractDataFunc},
    {"GetScriptID",                 ExGetContractRegIdFunc}, /** deprecated */
    {"GetContractRegId",            ExGetContractRegIdFunc},
    {"GetCurTxAccount",             ExGetCurTxAccountFunc},
    {"GetCurTxPayAmount",           ExGetCurTxPayAmountFunc},

    {"GetUserAppAccValue",          ExGetUserAppAccValueFunc},
    {"GetUserAppAccFundWithTag",    ExGetUserAppAccFundWithTagFunc},
    {"WriteOutAppOperate",          ExWriteOutAppOperateFunc},

    {"GetBase58Addr",               ExGetBase58AddrFunc},
    {"ByteToInteger",               ExByteToIntegerFunc},
    {"IntegerToByte4",              ExIntegerToByte4Func},
    {"IntegerToByte8",              ExIntegerToByte8Func},
    {"TransferContractAsset",       ExTransferContractAsset},
    {"TransferSomeAsset",           ExTransferSomeAsset},
    {"GetBlockTimestamp",           ExGetBlockTimestamp},

///////////////////////////////////////////////////////////////////////////////
// new function add in MAJOR_VER_R2
    {"TransferAccountAsset",        ExTransferAccountAssetFunc},
    {"TransferAccountAssets",       ExTransferAccountAssetsFunc},
    {"GetCurTxInputAsset",          ExGetCurTxInputAssetFunc},
    {"GetAccountAsset",             ExGetAccountAssetFunc},

    {nullptr, nullptr}

};

// replace all global(in the _G) functions
static const luaL_Reg baseLibsEx[] = {
    {"print",                       ExLuaPrint},        // replace default print function
    {"require",                     ExLimitedRequire},  // replace default require function

    {nullptr, nullptr}
};

/*
 * 注册一个新Lua模块*/
#ifdef WIN_DLL
    extern "C" __declspec(dllexport)int32_t luaopen_mylib(lua_State *L)
#else
    LUAMOD_API int32_t luaopen_mylib(lua_State *L)
#endif

{
    luaL_newlib(L, mylib); //生成一个table,把mylibs所有函数填充进去
    return 1;
}

bool InitLuaLibsEx(lua_State *L) {
    lua_pushglobaltable(L);
    luaL_setfuncs(L, baseLibsEx, 0);
    lua_pop(L, 1);  // pop the global table
    return true;
}
