/*
** Copyright (c) 2015-2019 The WaykiChain Core Developers
** Distributed under the MIT/X11 software license, see the accompanying
** file COPYING or http://www.opensource.org/licenses/mit-license.php
*/

#include <openssl/des.h>
#include <vector>

#include "lua/lua.hpp"
#include "vmrunenv.h"
#include "commons/SafeInt3.hpp"

#define LUA_C_BUFFER_SIZE  500  //传递值，最大字节防止栈溢出

#if 0
static void setfield(lua_State *L,char * key,double value){
     //默认栈顶是table
    lua_pushstring(L,key);
    lua_pushnumber(L,value);
    lua_settable(L,-3); //将这一对键值设成元素
}
static void stackDump(lua_State *L){
    int i;
    int top = lua_gettop(L);
//  int top = 20;//debug
    for(i = 0;i < top;i++){
        int t = lua_type(L,-1 - i);
        switch(t){
        case LUA_TSTRING:
            LogPrint("vm","%d str =%s\n", i, lua_tostring(L,-1 - i));
            break;
        case LUA_TBOOLEAN:
            LogPrint("vm","boolean =%d\n",lua_toboolean(L,-1 - i));
            break;
        case LUA_TNUMBER:
            LogPrint("vm","number =%d\n",lua_tonumber(L,-1 - i));
            break;
        default:
            LogPrint("vm","default =%s\n",lua_typename(L,-1 - i));
            break;
        }
       LogPrint("vm"," ");
    }
    LogPrint("vm","\n");
}
#endif

/*
 *  //3.往函数私有栈里存运算后的结果*/
static inline int32_t RetRstToLua(lua_State *L, const vector<unsigned char> &resultData,
                              bool needToTruncate = true) {
    int32_t len = resultData.size();
    // truncate data by default
    if (needToTruncate) {
        len = len > LUA_C_BUFFER_SIZE ? LUA_C_BUFFER_SIZE : len;
    }

    if (len > 0) {
        // check stack to avoid stack overflow
        if (lua_checkstack(L, len)) {
            // LogPrint("vm", "RetRstToLua value:%s\n", HexStr(resultData).c_str());
            for (int i = 0; i < len; i++) {
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

static inline int32_t RetRstToLua(lua_State *L, const string &resultData,
                              bool needToTruncate = true) {
    int32_t len = resultData.size();
    // truncate data by default
    if (needToTruncate) {
        len = len > LUA_C_BUFFER_SIZE ? LUA_C_BUFFER_SIZE : len;
    }

    if (len > 0) {
        // check stack to avoid stack overflow
        if (lua_checkstack(L, len)) {
            // LogPrint("vm", "RetRstToLua value:%s\n", HexStr(resultData).c_str());
            for (int i = 0; i < len; i++) {
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

/*
 *  //3.往函数私有栈里存布尔类型返回值*/
static inline int32_t RetRstBooleanToLua(lua_State *L,bool flag)
{
    //检测栈空间是否够
   if (lua_checkstack(L,sizeof(int))) {
//      LogPrint("vm", "RetRstBooleanToLua value:%d\n",flag);
        lua_pushboolean(L,(int)flag);
        return 1 ;
   } else {
        LogPrint("vm","%s\n", "RetRstBooleanToLua stack overflow");
            return 0;
   }
}

static inline int32_t RetFalse(const string reason)
{
     LogPrint("vm","%s\n", reason.c_str());
     return 0;
}
static CVmRunEnv* GetVmRunEnv(lua_State *L)
{
    CVmRunEnv* pVmRunEnv = NULL;
    int32_t res = lua_getglobal(L, "VmScriptRun");
    //LogPrint("vm", "GetVmRunEnv lua_getglobal:%d\n", res);

    if (LUA_TLIGHTUSERDATA == res) {
        if (lua_islightuserdata(L,-1)) {
            pVmRunEnv = (CVmRunEnv*)lua_topointer(L,-1);
            //LogPrint("vm", "GetVmRunEnv lua_topointer:%p\n", pVmRunEnv);
        }
    }
    lua_pop(L, 1);
    return pVmRunEnv;
}

static bool GetKeyId(const CAccountDBCache &accountView, vector<unsigned char> &ret, CKeyID &keyId) {
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

static bool GetArray(lua_State *L, vector<std::shared_ptr < std::vector<unsigned char> > > &ret) {
    //从栈里取变长的数组
    int totallen = lua_gettop(L);
    if((totallen <= 0) || (totallen > LUA_C_BUFFER_SIZE))
    {
        LogPrint("vm","totallen error\n");
        return false;
    }

    vector<unsigned char> vBuf;
    vBuf.clear();
    for(int i = 0;i < totallen;i++)
    {
        if(!lua_isnumber(L, i + 1))//if(!lua_isnumber(L,-1 - i))
        {
            LogPrint("vm","%s\n","data is not number");
            return false;
        }
        vBuf.insert(vBuf.end(),lua_tonumber(L,i+1));
    }
    ret.insert(ret.end(),std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));
    //LogPrint("vm", "GetData:%s, len:%d\n", HexStr(vBuf).c_str(), vBuf.size());
    return true;
}
static bool GetDataInt(lua_State *L,int &intValue) {
    //从栈里取int 高度
    if(!lua_isinteger(L,-1 - 0))
    {
        LogPrint("vm","%s\n","data is not integer");
        return false;
    }else{
        int value = (int)lua_tointeger(L,-1 - 0);
//      LogPrint("vm", "GetDataInt:%d\n", value);
        intValue = value;
        return true;
    }
}
static bool GetDataString(lua_State *L, vector<std::shared_ptr < std::vector<unsigned char> > > &ret) {
    //从栈里取一串字符串
    if(!lua_isstring(L,-1 - 0))
    {
        LogPrint("vm","%s\n","data is not string");
        return false;
    }
    vector<unsigned char> vBuf;
    vBuf.clear();
    const char *pStr = NULL;
    pStr = lua_tostring(L,-1 - 0);
    if(pStr && (strlen(pStr) <= LUA_C_BUFFER_SIZE)){
        for(size_t i = 0;i < strlen(pStr);i++){
            vBuf.insert(vBuf.end(),pStr[i]);
        }
        ret.insert(ret.end(),std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));
        //LogPrint("vm", "GetDataString:%s\n", pStr);
        return true;
    }else{
        LogPrint("vm","%s\n","lua_tostring get fail");
        return false;
    }
}
static bool getNumberInTable(lua_State *L, const char* pKey, double &ret){
    // 在table里，取指定pKey对应的一个number值

    //默认栈顶是table，将pKey入栈
    lua_pushstring(L,pKey);
    lua_gettable(L,-2);  //查找键值为key的元素，置于栈顶
    if(!lua_isnumber(L,-1))
    {
        LogPrint("vm","num get error! %s\n",lua_tostring(L,-1));
        lua_pop(L,1); //删掉产生的查找结果
        return false;
    }else{
        ret = lua_tonumber(L,-1);
//      LogPrint("vm", "getNumberInTable:%d\n", ret);
        lua_pop(L,1); //删掉产生的查找结果
        return true;
    }
}

static bool getStringInTable(lua_State *L,char * pKey, string &strValue){
    // 在table里，取指定pKey对应的string值

    const char *pStr = NULL;
    //默认栈顶是table，将pKey入栈
    lua_pushstring(L,pKey);
    lua_gettable(L,-2);  //查找键值为key的元素，置于栈顶
    if(!lua_isstring(L,-1))
    {
        LogPrint("vm","string get error! %s\n",lua_tostring(L,-1));
    }else{
        pStr = lua_tostring(L,-1);
        if(pStr && (strlen(pStr) <= LUA_C_BUFFER_SIZE)){
            string res(pStr);
            strValue = res;
//          LogPrint("vm", "getStringInTable:%s\n", pStr);
            lua_pop(L,1); //删掉产生的查找结果
            return true;
        }else{
            LogPrint("vm","%s\n","lua_tostring get fail");
        }
    }
    lua_pop(L,1); //删掉产生的查找结果
    return false;
}

template<typename ArrayType>
static bool getArrayInTable(lua_State *L, const char *pKey, unsigned short usLen, ArrayType &arrayOut){
    // 在table里，取指定pKey对应的数组

    if((usLen <= 0) || (usLen > LUA_C_BUFFER_SIZE)){
        LogPrint("vm","usLen error\n");
        return false;
    }
    unsigned char value = 0;
    arrayOut.clear();
    //默认栈顶是table，将key入栈
    lua_pushstring(L,pKey);
    lua_gettable(L,1);
    if(!lua_istable(L,-1))
    {
        LogPrint("vm","getTableInTable is not table\n");
        return false;
    }
    for (int i = 0; i < usLen; ++i)
    {
        lua_pushnumber(L, i+1); //将索引入栈
        lua_gettable(L, -2);
        if(!lua_isnumber(L,-1))
        {
            LogPrint("vm","getTableInTable is not number\n");
            return false;
        }
        value = 0;
        value = lua_tonumber(L, -1);
        arrayOut.insert(arrayOut.end(),value);
        lua_pop(L, 1);
    }
    lua_pop(L,1); //删掉产生的查找结果
    return true;
}
static bool getStringLogPrint(lua_State *L,char * pKey,unsigned short usLen,vector<unsigned char> &vOut){
    //从栈里取 table的值是一串字符串
    //该函数专用于写日志函数GetDataTableLogPrint，
    if((usLen <= 0) || (usLen > LUA_C_BUFFER_SIZE)){
        LogPrint("vm","usLen error\n");
        return false;
    }


    //默认栈顶是table，将key入栈
    lua_pushstring(L,pKey);
    lua_gettable(L,1);

    const char *pStr = NULL;
    vOut.clear();
    lua_getfield(L,-2,pKey);
    //stackDump(L);
    if(!lua_isstring(L,-1)/*LUA_TSTRING != lua_type(L, -1)*/)
    {
        LogPrint("vm","getStringLogPrint is not string\n");
        return false;
    }
    pStr = lua_tostring(L,-1 - 0);
    if(pStr && (strlen(pStr) == usLen)){
        for(size_t i = 0;i < usLen;i++){
            vOut.insert(vOut.end(),pStr[i]);
        }
//      LogPrint("vm", "getfieldTableString:%s\n", pStr);
        lua_pop(L,1); //删掉产生的查找结果
        return true;
    }else{
        LogPrint("vm","%s\n","getStringLogPrint get fail\n");
        lua_pop(L, 1); //删掉产生的查找结果
        return false;
    }
}
static bool GetDataTableLogPrint(lua_State *L, vector<std::shared_ptr < std::vector<unsigned char> > > &ret) {
    //取日志的key value
    if(!lua_istable(L,-1))
    {
        LogPrint("vm","GetDataTableLogPrint is not table\n");
        return false;
    }
    unsigned short len = 0;
    vector<unsigned char> vBuf ;
    //取key
    int key = 0;
    double doubleValue = 0;
    if (!(getNumberInTable(L, "key", doubleValue))){
        LogPrint("vm", "key get fail\n");
        return false;
    } else {
        key = (int)doubleValue;
    }
    vBuf.clear();
    vBuf.insert(vBuf.end(),key);
    ret.insert(ret.end(),std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));

    //取value的长度
    if (!(getNumberInTable(L, "length", doubleValue))){
        LogPrint("vm", "length get fail\n");
        return false;
    } else {
        len = (unsigned short)doubleValue;
    }

    if (len > 0) {
        len = len > LUA_C_BUFFER_SIZE ? LUA_C_BUFFER_SIZE : len;
        if (key) {   //hex
            if(!getArrayInTable(L,(char *)"value",len,vBuf)) {
                LogPrint("vm","valueTable is not table\n");
                return false;
            } else {
                ret.insert(ret.end(),std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));
            }
        } else { //string
            if(!getStringLogPrint(L,(char *)"value",len,vBuf)) {
                LogPrint("vm","valueString is not string\n");
                return false;
            } else {
                ret.insert(ret.end(),std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));
            }
        }
        return true;
    }else{
        LogPrint("vm", "length error\n");
        return false;
    }
}
static bool GetDataTableDes(lua_State *L, vector<std::shared_ptr < std::vector<unsigned char> > > &ret)
{
    if(!lua_istable(L,-1)) {
        LogPrint("vm","is not table\n");
        return false;
    }
    double doubleValue = 0;
    vector<unsigned char> vBuf ;

    int dataLen = 0;
    if (!(getNumberInTable(L,(char *)"dataLen",doubleValue))){
            LogPrint("vm","dataLen get fail\n");
            return false;
        } else {
            dataLen = (unsigned int)doubleValue;
    }

    if (dataLen <= 0) {
        LogPrint("vm","dataLen <= 0\n");
        return false;
    }

    if (!getArrayInTable(L,(char *)"data",dataLen,vBuf)) {
        LogPrint("vm","data not table\n");
        return false;
    }else{

        ret.push_back(std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));
    }

    int keyLen = 0;
    if(!(getNumberInTable(L,(char *)"keyLen",doubleValue))){
            LogPrint("vm","keyLen get fail\n");
            return false;
        }else{
            keyLen = (unsigned int)doubleValue;
    }

    if(keyLen <= 0) {
        LogPrint("vm","keyLen <= 0\n");
        return false;
    }

    if(!getArrayInTable(L,(char *)"key",keyLen,vBuf))
    {
        LogPrint("vm","key not table\n");
        return false;
    }else{

        ret.push_back(std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));
    }

    int nFlag = 0;
    if(!(getNumberInTable(L,(char *)"flag",doubleValue))){
            LogPrint("vm","flag get fail\n");
            return false;
        }else{
            nFlag = (unsigned int)doubleValue;
            CDataStream tep(SER_DISK, CLIENT_VERSION);
            tep << (nFlag == 0 ? 0 : 1);
            ret.push_back(std::make_shared<vector<unsigned char>>(tep.begin(), tep.end()));
    }

    return true;
}

static bool GetDataTableVerifySignature(lua_State *L, vector<std::shared_ptr < std::vector<unsigned char> > > &ret)
{
    if(!lua_istable(L,-1))
    {
        LogPrint("vm","is not table\n");
        return false;
    }
    double doubleValue = 0;
    vector<unsigned char> vBuf ;

    int dataLen = 0;
    if(!(getNumberInTable(L,(char *)"dataLen",doubleValue))){
            LogPrint("vm","get dataLen failed\n");
            return false;
        }else{
            dataLen = (unsigned int)doubleValue;
    }

    if(dataLen <= 0) {
        LogPrint("vm","dataLen <= 0\n");
        return false;
    }

    if(!getArrayInTable(L,(char *)"data",dataLen,vBuf))
    {
        LogPrint("vm","get data failed\n");
        return false;
    }else{

        ret.push_back(std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));
    }

    int pubKeyLen = 0;
    if(!(getNumberInTable(L,(char *)"pubKeyLen",doubleValue))){
            LogPrint("vm","get pubKeyLen failed\n");
            return false;
        }else{
            pubKeyLen = (unsigned int)doubleValue;
    }

    if(pubKeyLen <= 0) {
        LogPrint("vm","error: pubKeyLen <= 0\n");
        return false;
    }

    if(!getArrayInTable(L,(char *)"pubKey",pubKeyLen,vBuf))
    {
        LogPrint("vm","get pubKey failed\n");
        return false;
    }else{

        ret.push_back(std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));
    }

    int signatureLen = 0;
    if(!(getNumberInTable(L,(char *)"signatureLen",doubleValue))){
        LogPrint("vm","get signatureLen failed\n");
        return false;
    }
    signatureLen = (unsigned int)doubleValue;

    if(signatureLen <= 0) {
        LogPrint("vm","hashLen <= 0\n");
        return false;
    }

    if(!getArrayInTable(L,(char *)"signature",signatureLen,vBuf))
    {
        LogPrint("vm","get signature failed\n");
        return false;
    }else{

        ret.push_back(std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));
    }

    return true;
}

static int ExInt64MulFunc(lua_State *L) {

    int argc = lua_gettop(L);    /* number of arguments */
    if(argc != 2) {
        return RetFalse("argc error\n");
    }

    if(!lua_isinteger(L,1)) {
        return RetFalse("Int64Mul para1 error\n");
    }
    int64_t a = lua_tointeger(L,1);

    if(!lua_isinteger(L,2)) {
        return RetFalse("Int64Mul para2 error\n");
    }
    int64_t b = lua_tointeger(L,2);
    int64_t c = 0;
    LUA_BurnFuncCall(L, FUEL_CALL_Int64Mul, BURN_VER_R2);
    if(!SafeMultiply(a, b, c)) {
        return RetFalse("Int64Mul Operate overflow !\n");
    }

    lua_pushinteger(L, c);
    return 1;
}

static int ExInt64AddFunc(lua_State *L) {
    int argc = lua_gettop(L);    /* number of arguments */
    if(argc != 2) {
        return RetFalse("argc error\n");
    }

    if(!lua_isinteger(L,1)) {
        return RetFalse("Int64Add para1 error\n");
    }
    int64_t a = lua_tointeger(L,1);

    if(!lua_isinteger(L,2)) {
        return RetFalse("Int64Add para2 error\n");
    }
    int64_t b = lua_tointeger(L,2);

    int64_t c = 0;
    LUA_BurnFuncCall(L, FUEL_CALL_Int64Add, BURN_VER_R2);
    if(!SafeAdd(a, b, c)) {
        return RetFalse("Int64Add Operate overflow !\n");
    }

    lua_pushinteger(L, c);
    return 1;
}

static int ExInt64SubFunc(lua_State *L) {
    int argc = lua_gettop(L);    /* number of arguments */
    if(argc != 2) {
        return RetFalse("argc error\n");
    }

    if(!lua_isinteger(L,1)) {
        return RetFalse("Int64Sub para1 error\n");
    }
    int64_t a = lua_tointeger(L,1);

    if(!lua_isinteger(L,2)) {
        return RetFalse("Int64Sub para2 error\n");
    }
    int64_t b = lua_tointeger(L,2);

    int64_t c = 0;
    LUA_BurnFuncCall(L, FUEL_CALL_Int64Sub, BURN_VER_R2);
    if(!SafeSubtract(a, b, c)) {
        return RetFalse("Int64Sub Operate overflow !\n");
    }

    lua_pushinteger(L, c);
    return 1;
}

static int ExInt64DivFunc(lua_State *L) {
    int argc = lua_gettop(L);    /* number of arguments */
    if(argc != 2) {
        return RetFalse("argc error\n");
    }

    if(!lua_isinteger(L,1)) {
        return RetFalse("Int64Div para1 error\n");
    }
    int64_t a = lua_tointeger(L,1);

    if(!lua_isinteger(L,2)) {
        return RetFalse("Int64Div para2 error\n");
    }
    int64_t b = lua_tointeger(L,2);

    int64_t c = 0;
    LUA_BurnFuncCall(L, FUEL_CALL_Int64Div, BURN_VER_R2);
    if(!SafeDivide(a, b, c)) {
        return RetFalse("Int64Div Operate overflow !\n");
    }

    lua_pushinteger(L, c);
    return 1;
}

/**
 *bool SHA256(void const* pfrist, const unsigned short len, void * const pout)
 * This function receives an input param from a middle layer:
 *   1. The first param is the target string to be hashed twice in a BitCoin way
 */
static int ExSha256Func(lua_State *L) {
    vector<std::shared_ptr < vector<unsigned char> > > retdata;
    if (!GetDataString(L, retdata) || retdata.size() != 1 || retdata.at(0).get()->size() <= 0) {
        return RetFalse("ExSha256Func param err");
    }
    vector<unsigned char> &dataIn = *retdata.at(0).get();
    LUA_BurnFuncData(L, FUEL_CALL_Sha256, dataIn.size(), 32, FUEL_DATA32_Sha256, BURN_VER_R2);
    uint256 hash = Hash(dataIn.begin(), dataIn.end());
    CDataStream tep(SER_DISK, CLIENT_VERSION);
    tep << hash;
    vector<unsigned char> tep1(tep.begin(), tep.end());
    return RetRstToLua(L, tep1);
}

/**
 *bool SHA256Once(void const* pfrist, const unsigned short len, void * const pout)
 * This function receives an input param from a middle layer:
 *   1. The first param is the target string to be hashed once
 */
static int ExSha256OnceFunc(lua_State *L) {

    vector<std::shared_ptr < vector<unsigned char> > > retdata;
    if (!GetDataString(L,retdata) ||retdata.size() != 1 || retdata.at(0).get()->size() <= 0) {
        return RetFalse("ExSha256OnceFunc param err");
    }
    vector<unsigned char> &dataIn = *retdata.at(0).get();

    LUA_BurnFuncData(L, FUEL_CALL_Sha256Once, dataIn.size(), 32, FUEL_DATA32_Sha256Once, BURN_VER_R2);
    uint256 hash;
    SHA256(&dataIn.at(0), dataIn.size(), hash.begin());
    vector<unsigned char> tep1(hash.begin(), hash.end());
    return RetRstToLua(L, tep1);
}

/**
 *unsigned short Des(void const* pdata, unsigned short len, void const* pkey, unsigned short keylen, bool IsEn, void * const pOut,unsigned short poutlen)
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
static int ExDesFunc(lua_State *L) {
    vector<std::shared_ptr<vector<unsigned char> > > retdata;

    if (!GetDataTableDes(L, retdata) || retdata.size() != 3) {
        return RetFalse(string(__FUNCTION__) + "para  err !");
    }
    vector<unsigned char> &dataIn = *retdata.at(0).get();
    vector<unsigned char> &keyIn = *retdata.at(1).get();
    unsigned char flag = retdata.at(2).get()->at(0);
    static_assert(sizeof(DES_cblock) == 8, "DES block must be 8");
    if (keyIn.size() == 8) {
        LUA_BurnFuncData(L, FUEL_CALL_DesBasic, dataIn.size(), 8, FUEL_DATA8_DesBasic, BURN_VER_R2);
    } else if (keyIn.size() == 8) {
        LUA_BurnFuncData(L, FUEL_CALL_DesTriple, dataIn.size(), 8, FUEL_DATA8_DesTriple, BURN_VER_R2);
    } else {
        return RetFalse(string(__FUNCTION__) + "para  err !");
    }

    DES_key_schedule deskey1, deskey2, deskey3;

    vector<unsigned char> desdata;
    vector<unsigned char> desout;
    unsigned char datalen_rest = dataIn.size() % sizeof(DES_cblock);
    desdata.assign(dataIn.begin(), dataIn.end());
    if (datalen_rest) {
        desdata.insert(desdata.end(), sizeof(DES_cblock) - datalen_rest, 0);
    }

    const_DES_cblock in;
    DES_cblock out, key;

    desout.resize(desdata.size());

    if (flag == 1) {
        if (keyIn.size() == 8) {
            //          printf("the des encrypt\n");
            memcpy(key, &keyIn.at(0), sizeof(DES_cblock));
            DES_set_key_unchecked(&key, &deskey1);
            for (unsigned int ii = 0; ii < desdata.size() / sizeof(DES_cblock); ii++) {
                memcpy(&in, &desdata[ii * sizeof(DES_cblock)], sizeof(in));
                //              printf("in :%s\n", HexStr(in, in + 8, true).c_str());
                DES_ecb_encrypt(&in, &out, &deskey1, DES_ENCRYPT);
                //              printf("out :%s\n", HexStr(out, out + 8, true).c_str());
                memcpy(&desout[ii * sizeof(DES_cblock)], &out, sizeof(out));
            }
        } else if (keyIn.size() == 16) {
            //          printf("the 3 des encrypt\n");
            memcpy(key, &keyIn.at(0), sizeof(DES_cblock));
            DES_set_key_unchecked(&key, &deskey1);
            DES_set_key_unchecked(&key, &deskey3);
            memcpy(key, &keyIn.at(0) + sizeof(DES_cblock), sizeof(DES_cblock));
            DES_set_key_unchecked(&key, &deskey2);
            for (unsigned int ii = 0; ii < desdata.size() / sizeof(DES_cblock); ii++) {
                memcpy(&in, &desdata[ii * sizeof(DES_cblock)], sizeof(in));
                DES_ecb3_encrypt(&in, &out, &deskey1, &deskey2, &deskey3, DES_ENCRYPT);
                memcpy(&desout[ii * sizeof(DES_cblock)], &out, sizeof(out));
            }
        } else {
            //error
            return RetFalse(string(__FUNCTION__) + "para  err !");
        }
    } else {
        if (keyIn.size() == 8) {
            //          printf("the des decrypt\n");
            memcpy(key, &keyIn.at(0), sizeof(DES_cblock));
            DES_set_key_unchecked(&key, &deskey1);
            for (unsigned int ii = 0; ii < desdata.size() / sizeof(DES_cblock); ii++) {
                memcpy(&in, &desdata[ii * sizeof(DES_cblock)], sizeof(in));
                //              printf("in :%s\n", HexStr(in, in + 8, true).c_str());
                DES_ecb_encrypt(&in, &out, &deskey1, DES_DECRYPT);
                //              printf("out :%s\n", HexStr(out, out + 8, true).c_str());
                memcpy(&desout[ii * sizeof(DES_cblock)], &out, sizeof(out));
            }
        } else if (keyIn.size() == 16) {
            //          printf("the 3 des decrypt\n");
            memcpy(key, &keyIn.at(0), sizeof(DES_cblock));
            DES_set_key_unchecked(&key, &deskey1);
            DES_set_key_unchecked(&key, &deskey3);
            memcpy(key, &keyIn.at(0) + sizeof(DES_cblock), sizeof(DES_cblock));
            DES_set_key_unchecked(&key, &deskey2);
            for (unsigned int ii = 0; ii < desdata.size() / sizeof(DES_cblock); ii++) {
                memcpy(&in, &desdata[ii * sizeof(DES_cblock)], sizeof(in));
                DES_ecb3_encrypt(&in, &out, &deskey1, &deskey2, &deskey3, DES_DECRYPT);
                memcpy(&desout[ii * sizeof(DES_cblock)], &out, sizeof(out));
            }
        } else {
            //error
            return RetFalse(string(__FUNCTION__) + "para  err !");
        }
    }

    return RetRstToLua(L,desout);
}

/**
 *bool SignatureVerify(void const* data, unsigned short datalen, void const* key, unsigned short keylen,
        void const* phash, unsigned short hashlen)
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
static int ExVerifySignatureFunc(lua_State *L) {
    vector<std::shared_ptr<vector<unsigned char> > > retdata;

    if (!GetDataTableVerifySignature(L, retdata) || retdata.size() != 3 || retdata.at(1).get()->size() != 33) {
        return RetFalse(string(__FUNCTION__) + "para  err !");
    }
    vector<unsigned char> &data = *retdata.at(0);
    vector<unsigned char> &pubKey = *retdata.at(1);
    vector<unsigned char> &signature = *retdata.at(2);

    LUA_BurnFuncData(L, FUEL_CALL_VerifySignature, data.size(), 32, FUEL_DATA32_VerifySignature, BURN_VER_R2);

    CPubKey pk(pubKey.begin(), pubKey.end());

    uint256 dataHash = Hash(data.begin(), data.end());

    bool rlt = VerifySignature(dataHash, signature, pk);
    if (!rlt) {
        LogPrint("INFO", "ExVerifySignatureFunc call VerifySignature verify signature failed!\n");
    }

    return RetRstBooleanToLua(L, rlt);
}

static int ExGetTxContractFunc(lua_State *L) {
    vector<std::shared_ptr<vector<unsigned char>>> retdata;
    if (!GetArray(L, retdata) || retdata.size() != 1 || retdata.at(0).get()->size() != 32) {
        return RetFalse("ExGetTxContractFunc, para error");
    }

    CVmRunEnv *pVmRunEnv = GetVmRunEnv(L);
    if (NULL == pVmRunEnv) {
        return RetFalse("ExGetTxContractFunc, pVmRunEnv is NULL");
    }

    vector<unsigned char> vHash(retdata.at(0).get()->rbegin(), retdata.at(0).get()->rend());
    CDataStream ds(vHash, SER_DISK, CLIENT_VERSION);
    uint256 hash;
    ds >> hash;

    LogPrint("vm", "ExGetTxContractFunc, hash: %s\n", hash.GetHex().c_str());

    std::shared_ptr<CBaseTx> pBaseTx;
    int len = 0;
    if (hash == pVmRunEnv->GetCurTxHash()) {
        const string &curTxArguments = pVmRunEnv->GetTxContract();
        LUA_BurnFuncData(L, FUEL_CALL_GetCurTxContract, curTxArguments.size(), 32, FUEL_DATA32_GetTxContract, BURN_VER_R2);
        len = RetRstToLua(L, curTxArguments, false);
    } else if (GetTransaction(pBaseTx, hash, *pVmRunEnv->GetScriptDB(), false)) {
        if (pBaseTx->nTxType == LCONTRACT_INVOKE_TX) {
            CLuaContractInvokeTx *tx = static_cast<CLuaContractInvokeTx *>(pBaseTx.get());
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
 *void LogPrint(const void *pdata, const unsigned short datalen,PRINT_FORMAT flag )
 * 这个函数式从中间层传了两个个参数过来:
 * 1.第一个是打印数据的表示符号，true是一十六进制打印,否则以字符串的格式打印
 * 2.第二个是打印的字符串
 */
static int ExLogPrintFunc(lua_State *L) {
    vector<std::shared_ptr < vector<unsigned char> > > retdata;
    if(!GetDataTableLogPrint(L,retdata) || retdata.size() != 2) {
        return RetFalse("ExLogPrintFunc para err1");
    }
    CDataStream tep1(*retdata.at(0), SER_DISK, CLIENT_VERSION);
    bool flag ;
    tep1 >> flag;
    string pdata((*retdata[1]).begin(), (*retdata[1]).end());
    LUA_BurnFuncData(L, FUEL_CALL_LogPrint, pdata.size(), 1, FUEL_DATA1_LogPrint, BURN_VER_R2);

    if(flag) {
        LogPrint("vm","%s\n", HexStr(pdata).c_str());
    } else {
        LogPrint("vm","%s\n", pdata.c_str());
    }
    return  0;
}


/**
 *unsigned short GetAccounts(const unsigned char *txid, void* const paccount, unsigned short maxlen)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是 hash
 */
static int ExGetTxRegIDFunc(lua_State *L) {
    vector<std::shared_ptr<vector<unsigned char> > > retdata;
    if (!GetArray(L, retdata) || retdata.size() != 1 || retdata.at(0).get()->size() != 32) {
        return RetFalse("ExGetTxRegIDFunc, para error");
    }

    CVmRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (NULL == pVmRunEnv) {
        return RetFalse("ExGetTxRegIDFunc, pVmRunEnv is NULL");
    }

    vector<unsigned char> vHash(retdata.at(0).get()->rbegin(), retdata.at(0).get()->rend());

    CDataStream ds(vHash, SER_DISK, CLIENT_VERSION);
    uint256 hash;
    ds >> hash;

    LogPrint("vm","ExGetTxRegIDFunc, hash: %s\n", hash.GetHex().c_str());

    LUA_BurnFuncCall(L, FUEL_CALL_GetTxRegID, BURN_VER_R2);
    std::shared_ptr<CBaseTx> pBaseTx;
    int len = 0;
    if (GetTransaction(pBaseTx, hash, *pVmRunEnv->GetScriptDB(), false)) {
        if (pBaseTx->nTxType == BCOIN_TRANSFER_TX) {
            CBaseCoinTransferTx *tx = static_cast<CBaseCoinTransferTx*>(pBaseTx.get());
            if (tx->txUid.type() != typeid(CRegID))
                return RetFalse("ExGetTxRegIDFunc, txUid is not CRegID type");

            vector<unsigned char> item = tx->txUid.get<CRegID>().GetRegIdRaw();
            len = RetRstToLua(L, item);
        } else if (pBaseTx->nTxType == LCONTRACT_INVOKE_TX) {
            CLuaContractInvokeTx *tx = static_cast<CLuaContractInvokeTx*>(pBaseTx.get());
            if (tx->txUid.type() != typeid(CRegID))
                return RetFalse("ExGetTxRegIDFunc, txUid is not CRegID type");

            vector<unsigned char> item = tx->txUid.get<CRegID>().GetRegIdRaw();
            len = RetRstToLua(L, item);
        } else {
            return RetFalse("ExGetTxRegIDFunc, tx type error");
        }
    }

    return len;
}

static int ExByteToIntegerFunc(lua_State *L) {
    //把字节流组合成integer
    vector< std::shared_ptr<vector<unsigned char>> > retdata;
    if( !GetArray(L, retdata) ||retdata.size() != 1 ||
        ((retdata.at(0).get()->size() != 4) && (retdata.at(0).get()->size() != 8)) ) {
        return RetFalse("ExByteToIntegerFunc para err1");
    }

    //将数据反向
    vector<unsigned char> vValue(retdata.at(0).get()->begin(), retdata.at(0).get()->end());
    CDataStream tep1(vValue, SER_DISK, CLIENT_VERSION);

    LUA_BurnFuncCall(L, FUEL_CALL_ByteToInteger, BURN_VER_R2);
    if(retdata.at(0).get()->size() == 4) {
        unsigned int height;
        tep1 >>height;

//      LogPrint("vm","%d\n", height);
       if(lua_checkstack(L,sizeof(lua_Integer))){
            lua_pushinteger(L,(lua_Integer)height);
            return 1 ;
       }else{
            return RetFalse("ExByteToIntegerFunc stack overflow");
       }
    } else {
        int64_t llValue = 0;
        tep1 >>llValue;
//      LogPrint("vm","%lld\n", llValue);
       if (lua_checkstack(L, sizeof(lua_Integer))) {
            lua_pushinteger(L, (lua_Integer)llValue);
            return 1 ;
       } else {
            return RetFalse("ExByteToIntegerFunc stack overflow");
       }
    }
}

static int ExIntegerToByte4Func(lua_State *L) {
    //把integer转换成4字节数组
    int height = 0;
    if(!GetDataInt(L,height)){
        return RetFalse("ExGetBlockHashFunc para err1");
    }
    LUA_BurnFuncCall(L, FUEL_CALL_IntegerToByte4, BURN_VER_R2);
    CDataStream tep(SER_DISK, CLIENT_VERSION);
    tep << height;
    vector<unsigned char> TMP(tep.begin(),tep.end());
    return RetRstToLua(L,TMP);
}
static int ExIntegerToByte8Func(lua_State *L) {
    //把integer转换成8字节数组
    int64_t llValue = 0;
    if(!lua_isinteger(L,-1 - 0))
    {
        LogPrint("vm","%s\n","data is not integer");
        return 0;
    }else{
        llValue = (int64_t)lua_tointeger(L,-1 - 0);
//      LogPrint("vm", "ExIntegerToByte8Func:%lld\n", llValue);
    }

    LUA_BurnFuncCall(L, FUEL_CALL_IntegerToByte8, BURN_VER_R2);
    CDataStream tep(SER_DISK, CLIENT_VERSION);
    tep << llValue;
    vector<unsigned char> TMP(tep.begin(),tep.end());
    return RetRstToLua(L,TMP);
}
/**
 *unsigned short GetAccountPublickey(const void* const accounid,void * const pubkey,const unsigned short maxlength)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是 账户id,六个字节
 */
static int ExGetAccountPublickeyFunc(lua_State *L) {

    vector<std::shared_ptr<vector<unsigned char> > > retdata;
    if(!GetArray(L,retdata) ||retdata.size() != 1
        || !(retdata.at(0).get()->size() == 6 || retdata.at(0).get()->size() == 34))
    {
        return RetFalse("ExGetAccountPublickeyFunc para err1");
    }

    CVmRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if(NULL == pVmRunEnv)
    {
        return RetFalse("pVmRunEnv is NULL");
    }

    LUA_BurnFuncCall(L, FUEL_CALL_GetAccountPublickey, BURN_VER_R2);
     CKeyID addrKeyId;
     if (!GetKeyId(*(pVmRunEnv->GetCatchView()),*retdata.at(0).get(), addrKeyId)) {
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
//    assert(aaccount.owner_pubkey.IsFullyValid());
    if(false == account.owner_pubkey.IsFullyValid()){
        return RetFalse("ExGetAccountPublickeyFunc pubKey invalid");
    }
    tep >>te;
    vector<unsigned char> tep1(te.begin(),te.end());
    return RetRstToLua(L,tep1);
}

/**
 *bool QueryAccountBalance(const unsigned char* const account,Int64* const pBalance)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是 账户id,六个字节
 */
static int ExQueryAccountBalanceFunc(lua_State *L) {
    vector<std::shared_ptr < vector<unsigned char> > > retdata;
    if(!GetArray(L,retdata) ||retdata.size() != 1
        || !(retdata.at(0).get()->size() == 6 || retdata.at(0).get()->size() == 34))
    {
        return RetFalse("ExQueryAccountBalanceFunc para err1");
    }

    CVmRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if(NULL == pVmRunEnv)
    {
        return RetFalse("pVmRunEnv is NULL");
    }

    LUA_BurnFuncCall(L, FUEL_ACCOUNT_GET_VALUE, BURN_VER_R2);
     CKeyID addrKeyId;
     if (!GetKeyId(*(pVmRunEnv->GetCatchView()),*retdata.at(0).get(), addrKeyId)) {
            return RetFalse("ExQueryAccountBalanceFunc para err2");
     }

     CUserID userid(addrKeyId);
     CAccount account;
     int len = 0;
    if (!pVmRunEnv->GetCatchView()->GetAccount(userid, account)) {
        len = 0;
    }
    else
    {
        uint64_t nbalance = account.GetToken(SYMB::WICC).free_amount;
        CDataStream tep(SER_DISK, CLIENT_VERSION);
        tep << nbalance;
        vector<unsigned char> TMP(tep.begin(),tep.end());
        len = RetRstToLua(L,TMP);
    }
    return len;
}

/**
 *unsigned long GetTxConfirmHeight(const void * const txid)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个入参: hash,32个字节
 */
static int ExGetTxConfirmHeightFunc(lua_State *L) {
    vector<std::shared_ptr < vector<unsigned char> > > retdata;

    if(!GetArray(L,retdata) ||retdata.size() != 1|| retdata.at(0).get()->size() != 32)
    {
        return RetFalse("ExGetTxConfirmHeightFunc para err1");
    }

    // uint256 hash1(*retdata.at(0));
    // LogPrint("vm","ExGetTxContractsFunc1:%s",hash1.GetHex().c_str());

    // reverse hash value
    vector<unsigned char> vec_hash(retdata.at(0).get()->rbegin(), retdata.at(0).get()->rend());
    CDataStream tep1(vec_hash, SER_DISK, CLIENT_VERSION);
    uint256 hash1;
    tep1 >>hash1;

    CVmRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if(NULL == pVmRunEnv)
    {
        return RetFalse("pVmRunEnv is NULL");
    }

    LUA_BurnFuncCall(L, FUEL_CALL_GetTxConfirmHeight, BURN_VER_R2);
    int height = GetTxConfirmHeight(hash1, *pVmRunEnv->GetScriptDB());
    if(-1 == height)
    {
        return RetFalse("ExGetTxConfirmHeightFunc para err2");
    }
    else{
       if(lua_checkstack(L,sizeof(lua_Number))){
            lua_pushnumber(L,(lua_Number)height);
            return 1 ;
       }else{
           LogPrint("vm","%s\n", "ExGetCurRunEnvHeightFunc stack overflow");
           return 0;
       }
    }
}


/**
 *bool GetBlockHash(const unsigned long height,void * const pblochHash)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是 int类型的参数
 */
static int ExGetBlockHashFunc(lua_State *L) {
    int height = 0;
    if (!GetDataInt(L, height)) {
        return RetFalse("ExGetBlockHashFunc para err1");
    }

    CVmRunEnv *pVmRunEnv = GetVmRunEnv(L);
    if (NULL == pVmRunEnv) {
        return RetFalse("pVmRunEnv is NULL");
    }

    LUA_BurnFuncCall(L, FUEL_CALL_GetBlockHash, BURN_VER_R2);
    if (height <= 0 || height >= pVmRunEnv->GetConfirmHeight())  //当前block 是不可以获取hash的
    {
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
    vector<unsigned char> TMP(tep.begin(), tep.end());
    // reverse hash value
    vector<unsigned char> TMP2(TMP.rbegin(), TMP.rend());
    return RetRstToLua(L, TMP2);
}

static int ExGetCurRunEnvHeightFunc(lua_State *L) {
    CVmRunEnv *pVmRunEnv = GetVmRunEnv(L);
    if (NULL == pVmRunEnv) {
        return RetFalse("pVmRunEnv is NULL");
    }

    LUA_BurnFuncCall(L, FUEL_CALL_GetCurRunEnvHeight, BURN_VER_R2);
    int height = pVmRunEnv->GetConfirmHeight();

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

static bool GetDataTableWriteDataDB(lua_State *L, vector<std::shared_ptr < std::vector<unsigned char> > > &ret) {
    //取写数据库的key value
    if (!lua_istable(L, -1)) {
        LogPrint("vm", "GetDataTableWriteOutput is not table\n");
        return false;
    }
    unsigned short len = 0;
    vector<unsigned char> vBuf;
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
    ret.insert(ret.end(), std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));

    //取value的长度
    double doubleValue = 0;
    if (!(getNumberInTable(L, (char *)"length", doubleValue))) {
        LogPrint("vm", "length get fail\n");
        return false;
    } else {
        len = (unsigned short)doubleValue;
        // LogPrint("vm", "len =%d\n", len);
    }
    if ((len > 0) && (len <= LUA_C_BUFFER_SIZE)) {
        if (!getArrayInTable(L, (char *)"value", len, vBuf)) {
            LogPrint("vm", "value is not table\n");
            return false;
        } else {
            // LogPrint("vm", "value:%s\n", HexStr(vBuf).c_str());
            ret.insert(ret.end(), std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));
        }
        return true;
    } else {
        LogPrint("vm", "len overflow\n");
        return false;
    }
}

/**
 *bool WriteDataDB(const void* const key,const unsigned char keylen,const void * const value,const unsigned short valuelen,const unsigned long time)
 * 这个函数式从中间层传了三个个参数过来:
 * 1.第一个是 key值
 * 2.第二个是value值
 */
static int ExWriteDataDBFunc(lua_State *L)
{
    vector<std::shared_ptr < vector<unsigned char> > > retdata;
    if (!GetDataTableWriteDataDB(L, retdata) || retdata.size() != 2) {
        return RetFalse("ExWriteDataDBFunc key err1");
    }

    // TODO:
    string key((*retdata.at(0)).begin(), (*retdata.at(0)).end());
    string value((*retdata.at(1)).begin(), (*retdata.at(1)).end());

    CVmRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (NULL == pVmRunEnv) {

        return RetFalse("pVmRunEnv is NULL");
    }

    const CRegID contractRegId = pVmRunEnv->GetScriptRegID();
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
 *bool DeleteDataDB(const void* const key,const unsigned char keylen)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是 key值
 */
static int ExDeleteDataDBFunc(lua_State *L) {
    vector<std::shared_ptr < vector<unsigned char> > > retdata;

    if (!GetDataString(L, retdata) || retdata.size() != 1) {
        LogPrint("vm", "ExDeleteDataDBFunc key err1");
        return RetFalse(string(__FUNCTION__) + "para  err !");
    }
    string key = string((*retdata.at(0)).begin(), (*retdata.at(0)).end());

    CVmRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (NULL == pVmRunEnv) {
        return RetFalse("pVmRunEnv is NULL");
    }

    CRegID contractRegId       = pVmRunEnv->GetScriptRegID();
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
 *unsigned short ReadDataValueDB(const void* const key,const unsigned char keylen, void* const value,unsigned short const maxbuffer)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是 key值
 */
static int ExReadDataDBFunc(lua_State *L) {
    vector<std::shared_ptr < vector<unsigned char> > > retdata;

    if (!GetDataString(L,retdata) ||retdata.size() != 1) {
        return RetFalse("ExReadDataDBFunc key err1");
    }

    string key((*retdata.at(0)).begin(), (*retdata.at(0)).end());

    CVmRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (NULL == pVmRunEnv) {
        return RetFalse("pVmRunEnv is NULL");
    }

    CRegID scriptRegId = pVmRunEnv->GetScriptRegID();

    string value;
    CContractDBCache* scriptDB = pVmRunEnv->GetScriptDB();
    int len = 0;
    if (!scriptDB->GetContractData(scriptRegId, key, value)) {
        len = 0;
        lua_BurnStoreUnchanged(L, key.size(), 0, BURN_VER_R2);
    } else {
        lua_BurnStoreGet(L, key.size(), value.size(), BURN_VER_R2);
        len = RetRstToLua(L, vector<unsigned char>(value.begin(), value.end()));
    }
    return len;
}

static int ExGetCurTxHash(lua_State *L) {

    CVmRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (NULL == pVmRunEnv)
        return RetFalse("pVmRunEnv is NULL");

    LUA_BurnFuncCall(L, FUEL_CALL_GetCurTxHash, BURN_VER_R2);
    uint256 hash = pVmRunEnv->GetCurTxHash();
    CDataStream tep(SER_DISK, CLIENT_VERSION);
    tep << hash;
    vector<unsigned char> tep1(tep.begin(),tep.end());
    vector<unsigned char> tep2(tep1.rbegin(),tep1.rend());
    return RetRstToLua(L,tep2);
}

/**
 *bool ExModifyDataDBFunc(const void* const key,const unsigned char keylen, const void* const pvalue,const unsigned short valuelen)
 * 中间层传了两个参数
 * 1.第一个是 key
 * 2.第二个是 value
 */
static int ExModifyDataDBFunc(lua_State *L)
{
    vector<std::shared_ptr < vector<unsigned char> > > retdata;
    if (!GetDataTableWriteDataDB(L,retdata) ||retdata.size() != 2) {
        return RetFalse("ExModifyDataDBFunc key err");
    }

    string key((*retdata.at(0)).begin(), (*retdata.at(0)).end());
    string newValue((*retdata.at(1)).begin(), (*retdata.at(1)).end());

    CVmRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (NULL == pVmRunEnv) {
        return RetFalse("pVmRunEnv is NULL");
    }

    CRegID contractRegId = pVmRunEnv->GetScriptRegID();
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
    unsigned short len = 0;
    vector<unsigned char> vBuf ;
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
        LogPrint("vm","WriteOutput(), get accountidTbl failed\n");
        return false;
    } else {
       memcpy(operate.accountId,&vBuf[0],len);
    }

    if (!(getNumberInTable(L, "operatorType", doubleValue))) {
        LogPrint("vm", "WriteOutput(),  get opType fail\n");
        return false;

    } else {
        operate.opType = (BalanceOpType) doubleValue;
    }

    if (!(getNumberInTable(L, "outHeight", doubleValue))) {
        LogPrint("vm", "WriteOutput(),  get outheight fail\n");
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
 *      moneySymbol: (string, optional)   money symbol, must be valid symbol, such as WICC|WUSD
 * }
 * @return write succeed or not
 */
static int ExWriteOutputFunc(lua_State *L)
{
    CVmOperate operateIn;
    if (!GetDataTableWriteOutput(L, operateIn))
        return RetFalse("WriteOutput(), parse params failed");

    CVmRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (NULL == pVmRunEnv)
        return RetFalse("WriteOutput(), pVmRunEnv is NULL");

    LUA_BurnAccountOperate(L, 1, BURN_VER_R2);

    if (!pVmRunEnv->InsertOutputData({operateIn})) {
         return RetFalse("WriteOutput(), InsertOutputData failed");
    }

    return RetRstBooleanToLua(L,true);
}

static bool GetDataTableGetContractData(lua_State *L, vector<std::shared_ptr < std::vector<unsigned char> > > &ret)
{
    if (!lua_istable(L,-1)) {
        LogPrint("vm", "GetDataTableGetContractData is not table\n");
        return false;
    }

    vector<unsigned char> vBuf ;
    //取脚本id
    if (!getArrayInTable(L,(char *)"id",6,vBuf)) {
        LogPrint("vm","idTbl not table\n");
        return false;
    } else {
       ret.insert(ret.end(),std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));
    }

    //取key
    string key = "";
    if (!(getStringInTable(L,(char *)"key",key))) {
        LogPrint("vm","key get fail\n");
        return false;
    } else {
//      LogPrint("vm", "key:%s\n", key);
    }

    vBuf.clear();
    for (size_t i = 0;i < key.size();i++) {
        vBuf.insert(vBuf.end(),key.at(i));
    }

    ret.insert(ret.end(),std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));
    return true;
}

/**
 *bool GetContractData(const void* const scriptID,void* const pkey,short len,void* const pvalve,short maxlen)
 * 中间层传了两个个参数
 * 1.脚本的id号
 * 2.数据库的key值
 */
static int ExGetContractDataFunc(lua_State *L) {
    vector<std::shared_ptr<vector<unsigned char>>> retdata;

    if (!GetDataTableGetContractData(L, retdata) || retdata.size() != 2 || retdata.at(0).get()->size() != 6)
        return RetFalse("ExGetContractDataFunc tep1 err1");

    CVmRunEnv *pVmRunEnv = GetVmRunEnv(L);
    if (NULL == pVmRunEnv) return RetFalse("pVmRunEnv is NULL");

    CContractDBCache *scriptDB = pVmRunEnv->GetScriptDB();
    CRegID contractRegId(*retdata.at(0));
    string key((*retdata.at(1)).begin(), (*retdata.at(1)).end());
    string value;

    int len = 0;
    if (!scriptDB->GetContractData(contractRegId, key, value)) {
        len = 0;
        lua_BurnStoreUnchanged(L, key.size(), 0, BURN_VER_R2);
    } else {
        lua_BurnStoreGet(L, key.size(), value.size(), BURN_VER_R2);
        len = RetRstToLua(L, vector<unsigned char>(value.begin(), value.end()));
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
static int ExGetContractRegIdFunc(lua_State *L)
{
    CVmRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (NULL == pVmRunEnv)
        return RetFalse("pVmRunEnv is NULL");

    LUA_BurnFuncCall(L, FUEL_CALL_GetContractRegId, BURN_VER_R2);
   //1.从lua取参数
   //2.调用C++库函数 执行运算
    UnsignedCharArray contractRegId = pVmRunEnv->GetScriptRegID().GetRegIdRaw();
   //3.往函数私有栈里存运算后的结果
    int len = RetRstToLua(L,contractRegId);
   /*
    * 每个函数里的Lua栈是私有的,当把返回值压入Lua栈以后，该栈会自动被清空*/
    return len; //number of results 告诉Lua返回了几个返回值
}
static int ExGetCurTxAccountFunc(lua_State *L)
{
    CVmRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (NULL == pVmRunEnv)
        return RetFalse("pVmRunEnv is NULL");

    LUA_BurnFuncCall(L, FUEL_CALL_GetCurTxAccount, BURN_VER_R2);
   //1.从lua取参数
   //2.调用C++库函数 执行运算
    UnsignedCharArray vUserId =pVmRunEnv->GetTxAccount().GetRegIdRaw();

   //3.往函数私有栈里存运算后的结果
    int len = RetRstToLua(L,vUserId);
   /*
    * 每个函数里的Lua栈是私有的,当把返回值压入Lua栈以后，该栈会自动被清空*/
    return len; //number of results 告诉Lua返回了几个返回值
}

static int ExGetCurTxPayAmountFunc(lua_State *L)
{
    CVmRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if(NULL == pVmRunEnv)
        return RetFalse("pVmRunEnv is NULL");

    LUA_BurnFuncCall(L, FUEL_CALL_GetCurTxPayAmount, BURN_VER_R2);
    uint64_t lvalue =pVmRunEnv->GetValue();

    CDataStream tep(SER_DISK, CLIENT_VERSION);
    tep << lvalue;
    vector<unsigned char> tep1(tep.begin(),tep.end());
    int len = RetRstToLua(L,tep1);
    /*
    * 每个函数里的Lua栈是私有的,当把返回值压入Lua栈以后，该栈会自动被清空*/
    return len; //number of results 告诉Lua返回了几个返回值
}

struct S_APP_ID
{
    unsigned char idlen;                        //! the len of the tag
    unsigned char ID[CAppCFund::MAX_TAG_SIZE];  //! the ID for the

    const vector<unsigned char> GetIdV() const {
        // assert(sizeof(ID) >= idlen);
        vector<unsigned char> Id(&ID[0], &ID[idlen]);
        return (Id);
    }
}__attribute((aligned (1)));

static int ExGetUserAppAccValueFunc(lua_State *L)
{
    vector<std::shared_ptr < vector<unsigned char> > > retdata;
    if (!lua_istable(L, -1)) {
        LogPrint("vm", "is not table\n");
        return 0;
    }
    double doubleValue = 0;
    vector<unsigned char> vBuf ;
    S_APP_ID accid;
    memset(&accid,0,sizeof(accid));
    if (!(getNumberInTable(L, (char *)"idLen", doubleValue))){
        LogPrint("vm", "get idlen failed\n");
        return 0;
    } else {
        accid.idlen = (unsigned char)doubleValue;
    }
    if ((accid.idlen < 1) || (accid.idlen > sizeof(accid.ID))) {
        LogPrint("vm","idlen invalid\n");
        return 0;
    }
    if (!getArrayInTable(L, (char *) "idValueTbl", accid.idlen,vBuf)) {
        LogPrint("vm", "idValueTbl not table\n");
        return 0;
    } else {
       memcpy(&accid.ID[0],&vBuf[0],accid.idlen);
    }

    CVmRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if(NULL == pVmRunEnv)
        return RetFalse("pVmRunEnv is NULL");

    shared_ptr<CAppUserAccount> sptrAcc;
    uint64_t valueData = 0 ;
    int len = 0;
    LUA_BurnAccountGet(L, FUEL_ACCOUNT_GET_VALUE, BURN_VER_R2);
    if (pVmRunEnv->GetAppUserAccount(accid.GetIdV(), sptrAcc)) {
        valueData = sptrAcc->GetBcoins();

        CDataStream tep(SER_DISK, CLIENT_VERSION);
        tep << valueData;
        vector<unsigned char> TMP(tep.begin(), tep.end());
        len = RetRstToLua(L, TMP);
    }
    return len;
}

static bool GetDataTableOutAppOperate(lua_State *L, vector<std::shared_ptr < std::vector<unsigned char> > > &ret) {
    if (!lua_istable(L,-1)) {
        LogPrint("vm","is not table\n");
        return false;
    }
    double doubleValue = 0;
    vector<unsigned char> vBuf ;
    CAppFundOperate temp;
    memset(&temp,0,sizeof(temp));
    if (!(getNumberInTable(L,(char *)"operatorType",doubleValue))) {
        LogPrint("vm", "opType get fail\n");
        return false;
    } else {
        temp.opType = (unsigned char)doubleValue;
    }

    if (!(getNumberInTable(L, (char *)"outHeight", doubleValue))) {
        LogPrint("vm", "outHeight get fail\n");
        return false;
    } else {
        temp.timeoutHeight = (unsigned int) doubleValue;
    }

    if(!getArrayInTable(L,(char *)"moneyTbl",sizeof(temp.mMoney),vBuf))
    {
        LogPrint("vm", "moneyTbl not table\n");
        return false;
    }else{
       memcpy(&temp.mMoney,&vBuf[0],sizeof(temp.mMoney));
    }

    if (!(getNumberInTable(L, (char *) "userIdLen", doubleValue))) {
        LogPrint("vm", "appuserIDlen get fail\n");
        return false;
    } else {
        temp.appuserIDlen = (unsigned char) doubleValue;
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
        temp.fundTagLen = (unsigned char)doubleValue;
    }

    if((temp.fundTagLen > 0) && (temp.fundTagLen <= sizeof(temp.vFundTag))) {
        if (!getArrayInTable(L,(char *)"fundTagTbl", temp.fundTagLen, vBuf)) {
            LogPrint("vm","FundTagTbl not table\n");
            return false;
        } else {
            memcpy(temp.vFundTag,&vBuf[0],temp.fundTagLen);
        }
    }
    CDataStream tep(SER_DISK, CLIENT_VERSION);
    tep << temp;
    vector<unsigned char> tep1(tep.begin(),tep.end());
    ret.insert(ret.end(),std::make_shared<vector<unsigned char>>(tep1.begin(), tep1.end()));
    return true;
}

static int ExGetUserAppAccFundWithTagFunc(lua_State *L)
{
    vector<std::shared_ptr < vector<unsigned char> > > retdata;
    unsigned int Size(0);
    CAppFundOperate temp;
    Size = ::GetSerializeSize(temp, SER_NETWORK, PROTOCOL_VERSION);

    if (!GetDataTableOutAppOperate(L,retdata) ||retdata.size() != 1 || retdata.at(0).get()->size() != Size)
        return RetFalse("ExGetUserAppAccFundWithTagFunc para err0");

    CVmRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if(NULL == pVmRunEnv)
        return RetFalse("pVmRunEnv is NULL");

    CDataStream ss(*retdata.at(0),SER_DISK, CLIENT_VERSION);
    CAppFundOperate userfund;
    ss>>userfund;

    shared_ptr<CAppUserAccount> sptrAcc;
    CAppCFund fund;
    int len = 0;
    LUA_BurnAccountGet(L, FUEL_ACCOUNT_GET_FUND_TAG, BURN_VER_R2);
    if (pVmRunEnv->GetAppUserAccount(userfund.GetAppUserV(), sptrAcc)) {
        if (!sptrAcc->GetAppCFund(fund,userfund.GetFundTagV(), userfund.timeoutHeight))
            return RetFalse("GetUserAppAccFundWithTag GetAppCFund fail");

        CDataStream tep(SER_DISK, CLIENT_VERSION);
        tep << fund.GetValue() ;
        vector<unsigned char> TMP(tep.begin(),tep.end());
        len = RetRstToLua(L,TMP);
    }
    return len;
}

static bool GetDataTableAssetOperate(lua_State *L, int index, vector<std::shared_ptr < std::vector<unsigned char> > > &ret)
{
    if (!lua_istable(L, index)) {
        LogPrint("vm", "L is not table\n");
        return false;
    }

    double doubleValue = 0;
    vector<unsigned char> vBuf ;
    CAssetOperate temp;
    memset(&temp,0,sizeof(temp));

    if (!getArrayInTable(L,(char *)"toAddrTbl",34,vBuf)) {
        LogPrint("vm","toAddrTbl not table\n");
        return false;
    } else {
        ret.push_back(std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));
    }

    if (!(getNumberInTable(L, (char *) "outHeight", doubleValue))) {
        LogPrint("vm", "get timeoutHeight failed\n");
        return false;
    } else {
        temp.timeoutHeight = (unsigned int) doubleValue;
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
        temp.fundTagLen = (unsigned char)doubleValue;
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
    vector<unsigned char> tep1(tep.begin(),tep.end());
    ret.insert(ret.end(),std::make_shared<vector<unsigned char>>(tep1.begin(), tep1.end()));
    return true;
}

/**
 * 写应用操作输出到 pVmRunEnv->mapAppFundOperate[0]
 * @param ipara
 * @param pVmEvn
 * @return
 */
static int ExWriteOutAppOperateFunc(lua_State *L)
{
    vector<std::shared_ptr < vector<unsigned char> > > retdata;

    CAppFundOperate temp;
    unsigned int Size = ::GetSerializeSize(temp, SER_NETWORK, PROTOCOL_VERSION);

    if(!GetDataTableOutAppOperate(L,retdata) ||retdata.size() != 1 || (retdata.at(0).get()->size()%Size) != 0 )
        return RetFalse("ExWriteOutAppOperateFunc para err1");

    CVmRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if(NULL == pVmRunEnv)
        return RetFalse("pVmRunEnv is NULL");

    int count = retdata.at(0).get()->size()/Size;
    CDataStream ss(*retdata.at(0),SER_DISK, CLIENT_VERSION);
    LUA_BurnAccountOperate(L, count, BURN_VER_R2);

    int64_t step =-1;
    while (count--) {
        ss >> temp;
        // soft fork for contract negative money
        if (GetFeatureForkVersion(pVmRunEnv->GetConfirmHeight()) >= MAJOR_VER_R2 && temp.mMoney < 0) // in case contract uses negative money input
            return RetFalse("ExWriteOutAppOperateFunc para err2");

        pVmRunEnv->InsertOutAPPOperte(temp.GetAppUserV(),temp);
        step +=Size;
    }

    /*
    * 每个函数里的Lua栈是私有的,当把返回值压入Lua栈以后，该栈会自动被清空*/
    return RetRstBooleanToLua(L,true);
}

static int ExGetBase58AddrFunc(lua_State *L)
{
    vector<std::shared_ptr < vector<unsigned char> > > retdata;

    if (!GetArray(L,retdata) ||retdata.size() != 1 || retdata.at(0).get()->size() != 6)
        return RetFalse("ExGetBase58AddrFunc para err0");

    CVmRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if(NULL == pVmRunEnv)
        return RetFalse("pVmRunEnv is NULL");
/*
    vector<unsigned char> recvKey;
    recvKey.assign(retdata.at(0).get()->begin(), retdata.at(0).get()->end());

    std::string recvaddr( recvKey.begin(), recvKey.end() );

    for(int i = 0; i < recvKey.size(); i++)
        LogPrint("vm", "==============%02X\n", recvKey[i]);

    vector<unsigned char> tmp;
    for(int i = recvKey.size() - 1; i >=0 ; i--)
        tmp.push_back(recvKey[i]);
*/

    LUA_BurnFuncCall(L, FUEL_CALL_GetBase58Addr, BURN_VER_R2);
     CKeyID addrKeyId;
     if (!GetKeyId(*pVmRunEnv->GetCatchView(),*retdata.at(0).get(), addrKeyId))
        return RetFalse("ExGetBase58AddrFunc para err1");

     string wiccaddr = addrKeyId.ToAddress();

     vector<unsigned char> vTemp;
     vTemp.assign(wiccaddr.c_str(), wiccaddr.c_str()+wiccaddr.length());
     return RetRstToLua(L,vTemp);
}

static int ExTransferContractAsset(lua_State *L)
{
    vector<std::shared_ptr < vector<unsigned char> > > retdata;

    if (!GetArray(L,retdata) ||retdata.size() != 1 || retdata.at(0).get()->size() != 34)
        return RetFalse(string(__FUNCTION__)+"para  err !");

    CVmRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if(NULL == pVmRunEnv)
        return RetFalse("pVmRunEnv is NULL");

    vector<unsigned char> sendKey;
    vector<unsigned char> recvKey;
    CRegID script = pVmRunEnv->GetScriptRegID();

    CRegID sendRegID =pVmRunEnv->GetTxAccount();
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
        LUA_BurnAccountGet(L, FUEL_ACCTOUNT_UNCHANGED, BURN_VER_R2);
        LogPrint("vm", "%s\n", "recv addr is not valid !");
        return RetFalse(string(__FUNCTION__)+"recv addr is not valid !");
    }

    std::shared_ptr<CAppUserAccount> temp = std::make_shared<CAppUserAccount>();
    CContractDBCache* pContractScript = pVmRunEnv->GetScriptDB();

    if (!pContractScript->GetContractAccount(script, string(sendKey.begin(), sendKey.end()), *temp.get())) {
        LUA_BurnAccountGet(L, FUEL_ACCTOUNT_UNCHANGED, BURN_VER_R2);
        return RetFalse(string(__FUNCTION__) + "para  err3 !");
    }

    temp.get()->AutoMergeFreezeToFree(chainActive.Height());

    uint64_t nMoney = temp.get()->GetBcoins();

    int count = 0;
    int i = 0;
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

static int ExTransferSomeAsset(lua_State *L) {
    vector<std::shared_ptr < vector<unsigned char> > > retdata;

    unsigned int Size(0);
    CAssetOperate tempAsset;
    Size = ::GetSerializeSize(tempAsset, SER_NETWORK, PROTOCOL_VERSION);

    if (!GetDataTableAssetOperate(L, -1, retdata) || retdata.size() != 2 ||
        (retdata.at(1).get()->size()%Size) != 0 || retdata.at(0).get()->size() != 34)
        return RetFalse(string(__FUNCTION__)+"para err !");

    CVmRunEnv* pVmRunEnv = GetVmRunEnv(L);
    if (NULL == pVmRunEnv)
        return RetFalse("pVmRunEnv is NULL");

    CDataStream ss(*retdata.at(1),SER_DISK, CLIENT_VERSION);
    CAssetOperate assetOp;
    ss>>assetOp;

    vector<unsigned char> sendKey;
    vector<unsigned char> recvKey;
    CRegID script = pVmRunEnv->GetScriptRegID();

    CRegID sendRegID = pVmRunEnv->GetTxAccount();
    CKeyID SendKeyID = sendRegID.GetKeyId(*pVmRunEnv->GetCatchView());
    string addr = SendKeyID.ToAddress();
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

    int height = assetOp.GetHeight();
    if (height < 0)
        return RetFalse(string(__FUNCTION__) + " outHeight is not valid !");

    int i = 0;
    CAppFundOperate op;
    memset(&op, 0, sizeof(op));
    vector<unsigned char> vtag = assetOp.GetFundTagV();
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

static int ExGetBlockTimestamp(lua_State *L)
{
    int height = 0;
    if (!GetDataInt(L,height))
        return RetFalse("ExGetBlcokTimestamp para err1");

    LUA_BurnFuncCall(L, FUEL_CALL_GetBlockTimestamp, BURN_VER_R2);
    if (height <= 0) {
        height = chainActive.Height() + height;
        if(height < 0)
            return RetFalse("ExGetBlcokTimestamp para err2");
    }

    CBlockIndex *pIndex = chainActive[height];
    if (!pIndex)
        return RetFalse("ExGetBlcokTimestamp get time stamp error");

    if (lua_checkstack(L, sizeof(lua_Integer))) {
        lua_pushinteger(L, (lua_Integer) pIndex->nTime);
        return 1;
    }

    LogPrint("vm", "%s\n", "ExGetBlcokTimestamp stack overflow");
    return 0;
}


static int ExLimitedRequire(lua_State *L) {
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

static int ExLuaPrint(lua_State *L) {
    int n = lua_gettop(L); /* number of arguments */
    int i;
    std::string str = "";
    lua_getglobal(L, "tostring");
    for (i = 1; i <= n; i++) {
        const char *s;
        size_t l;
        lua_pushvalue(L, -1); /* function to be called */
        lua_pushvalue(L, i);  /* value to print */
        lua_call(L, 1, 1);
        s = lua_tolstring(L, -1, &l); /* get result */
        if (s == NULL) return luaL_error(L, "'tostring' must return a string to 'print'");
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

    {NULL, NULL}

};

// replace all global(in the _G) functions
static const luaL_Reg baseLibsEx[] = {
    {"print",                       ExLuaPrint},        // replace default print function
    {"require",                     ExLimitedRequire},  // repalace default require function

    {NULL, NULL}
};

/*
 * 注册一个新Lua模块*/
#ifdef WIN_DLL
    extern "C" __declspec(dllexport)int luaopen_mylib(lua_State *L)
#else
    LUAMOD_API int luaopen_mylib(lua_State *L)
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
