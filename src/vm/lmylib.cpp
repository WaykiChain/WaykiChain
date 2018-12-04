/*
** $Id: ldblib.c,v 1.149 2015/02/19 17:06:21 roberto Exp $
** Interface from Lua to its debug API
** See Copyright Notice in lua.h
*/

//#define ldblib_c
//#define LUA_LIB

//#include "lprefix.h"
//
//
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//
//#include "lua.h"
//
//#include "lauxlib.h"
//#include "lualib.h"

#include "lua/lua.hpp"
//#include "hash.h"
//#include "key.h"
//#include "main.h"
#include <openssl/des.h>
#include <vector>
#include "vmrunevn.h"
#include "SafeInt3.hpp"

#define LUA_C_BUFFER_SIZE  500  //传递值，最大字节防止栈溢出

#if 0
static void setfield(lua_State *L,char * key,double value){
	 //默认栈顶是table
	lua_pushstring(L,key);
	lua_pushnumber(L,value);
	lua_settable(L,-3);	//将这一对键值设成元素
}
static void stackDump(lua_State *L){
	int i;
	int top = lua_gettop(L);
//	int top = 20;//debug
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
static inline int RetRstToLua(lua_State *L,const vector<unsigned char> &ResultData )
{
	int len = ResultData.size();

	len = len > LUA_C_BUFFER_SIZE ? LUA_C_BUFFER_SIZE : len;
    if(len > 0)
    {	//检测栈空间是否够
    	if(lua_checkstack(L,len)){
//			LogPrint("vm", "RetRstToLua value:%s\n",HexStr(ResultData).c_str());
			for(int i = 0;i < len;i++){
				lua_pushinteger(L,(lua_Integer)ResultData[i]);
			}
			return len ;
    	}else{
    		LogPrint("vm","%s\r\n", "RetRstToLua stack overflow");
    	}
    }else{
    	LogPrint("vm","RetRstToLua err len = %d\r\n", len);
    }
    return  0;
}
/*
 *  //3.往函数私有栈里存布尔类型返回值*/
static inline int RetRstBooleanToLua(lua_State *L,bool flag)
{
	//检测栈空间是否够
   if(lua_checkstack(L,sizeof(int)))
   {
//      LogPrint("vm", "RetRstBooleanToLua value:%d\n",flag);
		lua_pushboolean(L,(int)flag);
		return 1 ;
   }else{
	    LogPrint("vm","%s\r\n", "RetRstBooleanToLua stack overflow");
		return 0;
   }
}

static inline int RetFalse(const string reason)
{
	 LogPrint("vm","%s\r\n", reason.c_str());
	 return 0;
}
static CVmRunEvn* GetVmRunEvn(lua_State *L)
{
	CVmRunEvn* pVmRunEvn = NULL;
	int res = lua_getglobal(L, "VmScriptRun");
	//LogPrint("vm", "GetVmRunEvn lua_getglobal:%d\n", res);

    if(LUA_TLIGHTUSERDATA == res)
    {
    	if(lua_islightuserdata(L,-1))
    	{
    		pVmRunEvn = (CVmRunEvn*)lua_topointer(L,-1);
    		//LogPrint("vm", "GetVmRunEvn lua_topointer:%p\n", pVmRunEvn);
    	}
    }
    lua_pop(L, 1);

    return pVmRunEvn;
}

static bool GetKeyId(const CAccountViewCache &view, vector<unsigned char> &ret,
		CKeyID &KeyId) {
	if (ret.size() == 6) {
		CRegID reg(ret);
		KeyId = reg.getKeyID(view);
	} else if (ret.size() == 34) {
		string addr(ret.begin(), ret.end());
		KeyId = CKeyID(addr);
	}else{
		return false;
	}
	if (KeyId.IsEmpty())
		return false;

	return true;
}
static bool GetArray(lua_State *L, vector<std::shared_ptr < std::vector<unsigned char> > > &ret) {
	//从栈里取变长的数组
	int totallen = lua_gettop(L);
	if((totallen <= 0) || (totallen > LUA_C_BUFFER_SIZE))
	{
		LogPrint("vm","totallen error\r\n");
		return false;
	}

    vector<unsigned char> vBuf;
	vBuf.clear();
	for(int i = 0;i < totallen;i++)
	{
		if(!lua_isnumber(L, i + 1))//if(!lua_isnumber(L,-1 - i))
		{
			LogPrint("vm","%s\r\n","data is not number");
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
		LogPrint("vm","%s\r\n","data is not integer");
		return false;
	}else{
		int value = (int)lua_tointeger(L,-1 - 0);
//		LogPrint("vm", "GetDataInt:%d\n", value);
		intValue = value;
		return true;
	}
}
static bool GetDataString(lua_State *L, vector<std::shared_ptr < std::vector<unsigned char> > > &ret) {
	//从栈里取一串字符串
	if(!lua_isstring(L,-1 - 0))
	{
		LogPrint("vm","%s\r\n","data is not string");
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
		LogPrint("vm","%s\r\n","lua_tostring get fail");
		return false;
	}
}
static bool getNumberInTable(lua_State *L,char * pKey, double &ret){
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
//    	LogPrint("vm", "getNumberInTable:%d\n", ret);
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
//    		LogPrint("vm", "getStringInTable:%s\n", pStr);
			lua_pop(L,1); //删掉产生的查找结果
			return true;
    	}else{
    		LogPrint("vm","%s\r\n","lua_tostring get fail");
    	}
    }
    lua_pop(L,1); //删掉产生的查找结果
    return false;
}

static bool getArrayInTable(lua_State *L,char * pKey,unsigned short usLen,vector<unsigned char> &vOut){
	// 在table里，取指定pKey对应的数组

    if((usLen <= 0) || (usLen > LUA_C_BUFFER_SIZE)){
    	LogPrint("vm","usLen error\r\n");
		return false;
    }
    unsigned char value = 0;
    vOut.clear();
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
		vOut.insert(vOut.end(),value);
		lua_pop(L, 1);
	}
    lua_pop(L,1); //删掉产生的查找结果
    return true;
}
static bool getStringLogPrint(lua_State *L,char * pKey,unsigned short usLen,vector<unsigned char> &vOut){
	//从栈里取 table的值是一串字符串
	//该函数专用于写日志函数GetDataTableLogPrint，
    if((usLen <= 0) || (usLen > LUA_C_BUFFER_SIZE)){
    	LogPrint("vm","usLen error\r\n");
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
//		LogPrint("vm", "getfieldTableString:%s\n", pStr);
		lua_pop(L,1); //删掉产生的查找结果
		return true;
	}else{
		LogPrint("vm","%s\r\n","getStringLogPrint get fail");
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
    if(!(getNumberInTable(L,(char *)"key",doubleValue))){
    	LogPrint("vm", "key get fail\n");
    	return false;
    }else{
    	key = (int)doubleValue;
    }
	vBuf.clear();
	vBuf.insert(vBuf.end(),key);
    ret.insert(ret.end(),std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));

    //取value的长度
    if(!(getNumberInTable(L,(char *)"length",doubleValue))){
    	LogPrint("vm", "length get fail\n");
    	return false;
    }else{
    	len = (unsigned short)doubleValue;
    }

    if(len > 0)
    {
    	len = len > LUA_C_BUFFER_SIZE ? LUA_C_BUFFER_SIZE : len;
		if(key)
		{   //hex
			if(!getArrayInTable(L,(char *)"value",len,vBuf))
			{
				LogPrint("vm","valueTable is not table\n");
				return false;
			}else{
				ret.insert(ret.end(),std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));
			}
		}else{ //string
			if(!getStringLogPrint(L,(char *)"value",len,vBuf))
			{
				LogPrint("vm","valueString is not string\n");
				return false;
			}else{
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
    if(!lua_istable(L,-1))
    {
    	LogPrint("vm","is not table\n");
    	return false;
    }
    double doubleValue = 0;
    vector<unsigned char> vBuf ;

    int dataLen = 0;
    if(!(getNumberInTable(L,(char *)"dataLen",doubleValue))){
    		LogPrint("vm","dataLen get fail\n");
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
    		LogPrint("vm","dataLen get fail\n");
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

    int hashLen = 0;
    if(!(getNumberInTable(L,(char *)"hashLen",doubleValue))){
    		LogPrint("vm","hashLen get fail\n");
    		return false;
    	}else{
    		hashLen = (unsigned int)doubleValue;
    }

    if(hashLen <= 0) {
		LogPrint("vm","hashLen <= 0\n");
		return false;
    }

    if(!getArrayInTable(L,(char *)"hash",hashLen,vBuf))
    {
    	LogPrint("vm","hash not table\n");
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
	if(!SafeDivide(a, b, c)) {
		return RetFalse("Int64Div Operate overflow !\n");
	}

	lua_pushinteger(L, c);
	return 1;
}

/**
 *bool SHA256(void const* pfrist, const unsigned short len, void * const pout)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是要被计算hash值的字符串
 */
static int ExSha256Func(lua_State *L) {
	vector<std::shared_ptr < vector<unsigned char> > > retdata;

	if(!GetDataString(L,retdata) ||retdata.size() != 1 || retdata.at(0).get()->size() <= 0)
	{
		return RetFalse("ExSha256Func para err0");
	}

	uint256 rslt = Hash(&retdata.at(0).get()->at(0), &retdata.at(0).get()->at(0) + retdata.at(0).get()->size());

	CDataStream tep(SER_DISK, CLIENT_VERSION);
	tep << rslt;
	vector<unsigned char> tep1(tep.begin(), tep.end());
	return RetRstToLua(L,tep1);
}

/**
 *unsigned short Des(void const* pdata, unsigned short len, void const* pkey, unsigned short keylen, bool IsEn, void * const pOut,unsigned short poutlen)
 * 这个函数式从中间层传了三个个参数过来:
 * 1.第一个是要被加密数据或者解密数据
 * 2.第二格式加密或者解密的key值
 * 3.第三是标识符，是加密还是解密
 *
 * {
 * 	dataLen = 0,
 * 	data = {},
 * 	keyLen = 0,
 * 	key = {},
 * 	flag = 0
 * }
 */
static int ExDesFunc(lua_State *L) {
	vector<std::shared_ptr<vector<unsigned char> > > retdata;

	if (!GetDataTableDes(L, retdata) || retdata.size() != 3) {
		return RetFalse(string(__FUNCTION__) + "para  err !");
	}

	DES_key_schedule deskey1, deskey2, deskey3;

	vector<unsigned char> desdata;
	vector<unsigned char> desout;
	unsigned char datalen_rest = retdata.at(0).get()->size() % sizeof(DES_cblock);
	desdata.assign(retdata.at(0).get()->begin(), retdata.at(0).get()->end());
	if (datalen_rest) {
		desdata.insert(desdata.end(), sizeof(DES_cblock) - datalen_rest, 0);
	}

	const_DES_cblock in;
	DES_cblock out, key;

	desout.resize(desdata.size());

	unsigned char flag = retdata.at(2).get()->at(0);
	if (flag == 1) {
		if (retdata.at(1).get()->size() == 8) {
			//			printf("the des encrypt\r\n");
			memcpy(key, &retdata.at(1).get()->at(0), sizeof(DES_cblock));
			DES_set_key_unchecked(&key, &deskey1);
			for (unsigned int ii = 0; ii < desdata.size() / sizeof(DES_cblock); ii++) {
				memcpy(&in, &desdata[ii * sizeof(DES_cblock)], sizeof(in));
				//				printf("in :%s\r\n", HexStr(in, in + 8, true).c_str());
				DES_ecb_encrypt(&in, &out, &deskey1, DES_ENCRYPT);
				//				printf("out :%s\r\n", HexStr(out, out + 8, true).c_str());
				memcpy(&desout[ii * sizeof(DES_cblock)], &out, sizeof(out));
			}
		} else if (retdata.at(1).get()->size() == 16) {
			//			printf("the 3 des encrypt\r\n");
			memcpy(key, &retdata.at(1).get()->at(0), sizeof(DES_cblock));
			DES_set_key_unchecked(&key, &deskey1);
			DES_set_key_unchecked(&key, &deskey3);
			memcpy(key, &retdata.at(1).get()->at(0) + sizeof(DES_cblock), sizeof(DES_cblock));
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
		if (retdata.at(1).get()->size() == 8) {
			//			printf("the des decrypt\r\n");
			memcpy(key, &retdata.at(1).get()->at(0), sizeof(DES_cblock));
			DES_set_key_unchecked(&key, &deskey1);
			for (unsigned int ii = 0; ii < desdata.size() / sizeof(DES_cblock); ii++) {
				memcpy(&in, &desdata[ii * sizeof(DES_cblock)], sizeof(in));
				//				printf("in :%s\r\n", HexStr(in, in + 8, true).c_str());
				DES_ecb_encrypt(&in, &out, &deskey1, DES_DECRYPT);
				//				printf("out :%s\r\n", HexStr(out, out + 8, true).c_str());
				memcpy(&desout[ii * sizeof(DES_cblock)], &out, sizeof(out));
			}
		} else if (retdata.at(1).get()->size() == 16) {
			//			printf("the 3 des decrypt\r\n");
			memcpy(key, &retdata.at(1).get()->at(0), sizeof(DES_cblock));
			DES_set_key_unchecked(&key, &deskey1);
			DES_set_key_unchecked(&key, &deskey3);
			memcpy(key, &retdata.at(1).get()->at(0) + sizeof(DES_cblock), sizeof(DES_cblock));
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
 * 1.第一个是签名的数据
 * 2.第二个是用的签名的publickey
 * 3.第三是签名之前的hash值
 *
 *{
 * 	dataLen = 0,
 * 	data = {},
 * 	keyLen = 0,
 * 	key = {},
 * 	hashLen = 0,
 * 	hash = {}
 * }
 */
static int ExVerifySignatureFunc(lua_State *L) {
	vector<std::shared_ptr<vector<unsigned char> > > retdata;

	if (!GetDataTableVerifySignature(L, retdata) || retdata.size() != 3 || retdata.at(1).get()->size() != 33
			|| retdata.at(2).get()->size() != 32) {
		return RetFalse(string(__FUNCTION__) + "para  err !");
	}

	CPubKey pk(retdata.at(1).get()->begin(), retdata.at(1).get()->end());
	vector<unsigned char> vec_hash(retdata.at(2).get()->rbegin(), retdata.at(2).get()->rend());
	uint256 hash(vec_hash);
	auto tem = std::make_shared<std::vector<vector<unsigned char> > >();

	bool rlt = CheckSignScript(hash, *retdata.at(0), pk);
	if (!rlt) {
		LogPrint("INFO", "ExVerifySignatureFunc call CheckSignScript verify signature failed!\n");
	}

	return RetRstBooleanToLua(L, rlt);
}


static int ExGetTxContractsFunc(lua_State *L) {

	vector<std::shared_ptr < vector<unsigned char> > > retdata;
    if(!GetArray(L,retdata) ||retdata.size() != 1 || retdata.at(0).get()->size() != 32)
    {
    	return RetFalse(string(__FUNCTION__)+"para  err !");
    }

    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }

    vector<unsigned char> vec_hash(retdata.at(0).get()->rbegin(), retdata.at(0).get()->rend());
	CDataStream tep1(vec_hash, SER_DISK, CLIENT_VERSION);
	uint256 hash1;
	tep1 >>hash1;

	std::shared_ptr<CBaseTransaction> pBaseTx;

	if (GetTransaction(pBaseTx, hash1, *pVmRunEvn->GetScriptDB(), false)) {
		CTransaction *tx = static_cast<CTransaction*>(pBaseTx.get());
		 return RetRstToLua(L, tx->vContract);
	}

	return 0;
}

/**
 *void LogPrint(const void *pdata, const unsigned short datalen,PRINT_FORMAT flag )
 * 这个函数式从中间层传了两个个参数过来:
 * 1.第一个是打印数据的表示符号，true是一十六进制打印,否则以字符串的格式打印
 * 2.第二个是打印的字符串
 */
static int ExLogPrintFunc(lua_State *L) {
	vector<std::shared_ptr < vector<unsigned char> > > retdata;
    if(!GetDataTableLogPrint(L,retdata) ||retdata.size() != 2)
    {
    	return RetFalse("ExLogPrintFunc para err1");
    }
	CDataStream tep1(*retdata.at(0), SER_DISK, CLIENT_VERSION);
	bool flag ;
	tep1 >> flag;
	string pdata((*retdata[1]).begin(), (*retdata[1]).end());

	if(flag)
	{
		LogPrint("vm","%s\r\n", HexStr(pdata).c_str());
	}else
	{
		LogPrint("vm","%s\r\n",pdata.c_str());
	}
    return  0;
}


/**
 *unsigned short GetAccounts(const unsigned char *txhash,void* const paccoutn,unsigned short maxlen)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是 hash
 */
static int ExGetTxAccountsFunc(lua_State *L) {
	vector<std::shared_ptr<vector<unsigned char> > > retdata;
    if(!GetArray(L,retdata) ||retdata.size() != 1|| retdata.at(0).get()->size() != 32)
    {
    	return RetFalse("ExGetTxAccountsFunc para err1");
    }

    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }

    vector<unsigned char> vec_hash(retdata.at(0).get()->rbegin(), retdata.at(0).get()->rend());

	CDataStream tep1(vec_hash, SER_DISK, CLIENT_VERSION);
	uint256 hash1;
	tep1 >>hash1;

	LogPrint("vm","ExGetTxAccountsFunc:%s",hash1.GetHex().c_str());
	std::shared_ptr<CBaseTransaction> pBaseTx;

//	auto tem = make_shared<std::vector<vector<unsigned char> > >();
    int len = 0;
	if (GetTransaction(pBaseTx, hash1, *pVmRunEvn->GetScriptDB(), false)) {
		CTransaction *tx = static_cast<CTransaction*>(pBaseTx.get());
		vector<unsigned char> item = boost::get<CRegID>(tx->srcRegId).GetVec6();
		len = RetRstToLua(L,item);
	}
	return len;
}

static int ExByteToIntegerFunc(lua_State *L) {
	//把字节流组合成integer
	vector<std::shared_ptr<vector<unsigned char> > > retdata;
    if(!GetArray(L,retdata) ||retdata.size() != 1|| ((retdata.at(0).get()->size() != 4) && (retdata.at(0).get()->size() != 8)))
    {
    	return RetFalse("ExGetTxAccountsFunc para err1");
    }

    //将数据反向
    vector<unsigned char>  vValue(retdata.at(0).get()->begin(), retdata.at(0).get()->end());
    CDataStream tep1(vValue, SER_DISK, CLIENT_VERSION);

    if(retdata.at(0).get()->size() == 4)
    {
		unsigned int height;
		tep1 >>height;

//		LogPrint("vm","%d\r\n", height);
	   if(lua_checkstack(L,sizeof(lua_Integer))){
			lua_pushinteger(L,(lua_Integer)height);
			return 1 ;
	   }else{
			return RetFalse("ExGetTxAccountsFunc stack overflow");
	   }
    }else{
		int64_t llValue = 0;
		tep1 >>llValue;
//		LogPrint("vm","%lld\r\n", llValue);
	   if(lua_checkstack(L,sizeof(lua_Integer))){
			lua_pushinteger(L,(lua_Integer)llValue);
			return 1 ;
	   }else{
			return RetFalse("ExGetTxAccountsFunc stack overflow");
	   }
    }
}

static int ExIntegerToByte4Func(lua_State *L) {
	//把integer转换成4字节数组
	int height = 0;
    if(!GetDataInt(L,height)){
    	return RetFalse("ExGetBlockHashFunc para err1");
    }
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
		LogPrint("vm","%s\r\n","data is not integer");
		return 0;
	}else{
		llValue = (int64_t)lua_tointeger(L,-1 - 0);
//		LogPrint("vm", "ExIntegerToByte8Func:%lld\n", llValue);
	}

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

    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }

	 CKeyID addrKeyId;
	 if (!GetKeyId(*(pVmRunEvn->GetCatchView()),*retdata.at(0).get(), addrKeyId)) {
	    	return RetFalse("ExGetAccountPublickeyFunc para err2");
	 }
	CUserID userid(addrKeyId);
	CAccount aAccount;
	if (!pVmRunEvn->GetCatchView()->GetAccount(userid, aAccount)) {
		return RetFalse("ExGetAccountPublickeyFunc para err3");
	}
    CDataStream tep(SER_DISK, CLIENT_VERSION);
    vector<char> te;
    tep << aAccount.PublicKey;
//    assert(aAccount.PublicKey.IsFullyValid());
    if(false == aAccount.PublicKey.IsFullyValid()){
    	return RetFalse("ExGetAccountPublickeyFunc PublicKey invalid");
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

    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }

	 CKeyID addrKeyId;
	 if (!GetKeyId(*(pVmRunEvn->GetCatchView()),*retdata.at(0).get(), addrKeyId)) {
	    	return RetFalse("ExQueryAccountBalanceFunc para err2");
	 }

	 CUserID userid(addrKeyId);
	 CAccount aAccount;
	 int len = 0;
	if (!pVmRunEvn->GetCatchView()->GetAccount(userid, aAccount)) {
		len = 0;
	}
	else
	{
		uint64_t nbalance = aAccount.GetRawBalance();
		CDataStream tep(SER_DISK, CLIENT_VERSION);
		tep << nbalance;
		vector<unsigned char> TMP(tep.begin(),tep.end());
		len = RetRstToLua(L,TMP);
	}
	return len;
}

/**
 *unsigned long GetTxConFirmHeight(const void * const txhash)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个入参: hash,32个字节
 */
static int ExGetTxConFirmHeightFunc(lua_State *L) {
	vector<std::shared_ptr < vector<unsigned char> > > retdata;

    if(!GetArray(L,retdata) ||retdata.size() != 1|| retdata.at(0).get()->size() != 32)
    {
    	return RetFalse("ExGetTxConFirmHeightFunc para err1");
    }
	uint256 hash1(*retdata.at(0));
//	LogPrint("vm","ExGetTxContractsFunc1:%s",hash1.GetHex().c_str());
    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }

	int nHeight = GetTxConfirmHeight(hash1, *pVmRunEvn->GetScriptDB());
	if(-1 == nHeight)
	{
		return RetFalse("ExGetTxConFirmHeightFunc para err2");
	}
	else{
	   if(lua_checkstack(L,sizeof(lua_Number))){
			lua_pushnumber(L,(lua_Number)nHeight);
			return 1 ;
	   }else{
		   LogPrint("vm","%s\r\n", "ExGetCurRunEnvHeightFunc stack overflow");
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
    if(!GetDataInt(L,height)){
    	return RetFalse("ExGetBlockHashFunc para err1");
    }

    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }

	if (height <= 0 || height >= pVmRunEvn->GetComfirHeight()) //当前block 是不可以获取hash的
	{
		return RetFalse("ExGetBlockHashFunc para err2");
	}

	if(chainActive.Height() < height){	         //获取比当前高度高的数据是不可以的
		return RetFalse("ExGetBlockHashFunc para err3");
	}
	CBlockIndex *pindex = chainActive[height];
	uint256 blockHash = pindex->GetBlockHash();

//	LogPrint("vm","ExGetBlockHashFunc:%s",HexStr(blockHash).c_str());
    CDataStream tep(SER_DISK, CLIENT_VERSION);
    tep << blockHash;
    vector<unsigned char> TMP(tep.begin(),tep.end());
    return RetRstToLua(L,TMP);
}

static int ExGetCurRunEnvHeightFunc(lua_State *L) {
    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }

	int height = pVmRunEvn->GetComfirHeight();

	//检测栈空间是否够
   if(height > 0)
   {
	   //lua_Integer
	   /*
	   if(lua_checkstack(L,sizeof(lua_Number))){
			lua_pushnumber(L,(lua_Number)height);
			return 1 ;
	   }else{
		   LogPrint("vm","%s\r\n", "ExGetCurRunEnvHeightFunc stack overflow");
	   }
	   */
	   if(lua_checkstack(L,sizeof(lua_Integer))){
		   lua_pushinteger(L,(lua_Integer)height);
			return 1 ;
	   }else{
		   LogPrint("vm","%s\r\n", "ExGetCurRunEnvHeightFunc stack overflow");
	   }
   }else{
	   LogPrint("vm","ExGetCurRunEnvHeightFunc err height =%d\r\n", height);
   }
   return 0;
}

static bool GetDataTableWriteDataDB(lua_State *L, vector<std::shared_ptr < std::vector<unsigned char> > > &ret) {
	//取写数据库的key value
    if(!lua_istable(L,-1))
    {
    	LogPrint("vm","GetDataTableWriteOutput is not table\n");
    	return false;
    }
	unsigned short len = 0;
    vector<unsigned char> vBuf ;
    //取key
    string key = "";
    if(!(getStringInTable(L,(char *)"key",key))){
    	LogPrint("vm","key get fail\n");
    	return false;
    }else{
//		LogPrint("vm", "key:%s\n", key);
    }
	vBuf.clear();
	for(size_t i = 0;i < key.size();i++)
	{
		vBuf.insert(vBuf.end(),key.at(i));
	}
    ret.insert(ret.end(),std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));

    //取value的长度
    double doubleValue = 0;
    if(!(getNumberInTable(L,(char *)"length",doubleValue))){
    	LogPrint("vm", "length get fail\n");
    	return false;
    }else{
    	 len = (unsigned short)doubleValue;
//    	 LogPrint("vm","len =%d\n",len);
    }
    if((len > 0) && (len <= LUA_C_BUFFER_SIZE))
    {
		if(!getArrayInTable(L,(char *)"value",len,vBuf))
		{
			LogPrint("vm","value is not table\n");
			return false;
		}else{
//			LogPrint("vm", "value:%s\n", HexStr(vBuf).c_str());
			ret.insert(ret.end(),std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));
		}
		return true;
    }else{
    	LogPrint("vm","len overflow\n");
		return false;
    }
}

/**
 *bool WriteDataDB(const void* const key,const unsigned char keylen,const void * const value,const unsigned short valuelen,const unsigned long time)
 * 这个函数式从中间层传了三个个参数过来:
 * 1.第一个是 key值
 * 2.第二个是value值
 */
static int ExWriteDataDBFunc(lua_State *L) {
	vector<std::shared_ptr < vector<unsigned char> > > retdata;

    if(!GetDataTableWriteDataDB(L,retdata) ||retdata.size() != 2)
    {
    	return RetFalse("ExWriteDataDBFunc key err1");
    }

    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }

	const CRegID scriptid = pVmRunEvn->GetScriptRegID();
	bool flag = true;
	CScriptDBViewCache* scriptDB = pVmRunEvn->GetScriptDB();

	CScriptDBOperLog operlog;
//	int64_t step = (*retdata.at(1)).size() -1;
	if (!scriptDB->SetAppData(scriptid, *retdata.at(0), *retdata.at(1),operlog)) {
		flag = false;
	} else {
		shared_ptr<vector<CScriptDBOperLog> > m_dblog = pVmRunEvn->GetDbLog();
		(*m_dblog.get()).push_back(operlog);
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

	if(!GetDataString(L,retdata) ||retdata.size() != 1)
    {
    	LogPrint("vm", "ExDeleteDataDBFunc key err1");
    	return RetFalse(string(__FUNCTION__)+"para  err !");
    }
    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }
	CRegID scriptid = pVmRunEvn->GetScriptRegID();

	CScriptDBViewCache* scriptDB = pVmRunEvn->GetScriptDB();

	bool flag = true;
	CScriptDBOperLog operlog;
	int64_t nstep = 0;
	vector<unsigned char> vValue;
	if(scriptDB->GetAppData(pVmRunEvn->GetComfirHeight(),scriptid, *retdata.at(0), vValue)){
		nstep = nstep - (int64_t)(vValue.size()+1);//删除数据奖励step
	}
	if (!scriptDB->EraseAppData(scriptid, *retdata.at(0), operlog)) {
		LogPrint("vm", "ExDeleteDataDBFunc error key:%s!\n",HexStr(*retdata.at(0)));
		flag = false;
	} else {
		shared_ptr<vector<CScriptDBOperLog> > m_dblog = pVmRunEvn->GetDbLog();
		m_dblog.get()->push_back(operlog);
	}
    return RetRstBooleanToLua(L,flag);
}

/**
 *unsigned short ReadDataValueDB(const void* const key,const unsigned char keylen, void* const value,unsigned short const maxbuffer)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是 key值
 */
static int ExReadDataValueDBFunc(lua_State *L) {
	vector<std::shared_ptr < vector<unsigned char> > > retdata;

    if(!GetDataString(L,retdata) ||retdata.size() != 1)
    {
    	return RetFalse("ExReadDataValueDBFunc key err1");
    }

    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }

	CRegID scriptid = pVmRunEvn->GetScriptRegID();

	vector_unsigned_char vValue;
	CScriptDBViewCache* scriptDB = pVmRunEvn->GetScriptDB();
	int len = 0;
	if(!scriptDB->GetAppData(pVmRunEvn->GetComfirHeight(),scriptid, *retdata.at(0), vValue))
	{
		len = 0;
	}
	else
	{
		len = RetRstToLua(L,vValue);
	}
	return len;
}

#if 0
static int ExGetDBSizeFunc(lua_State *L) {
//	CVmRunEvn *pVmRunEvn = (CVmRunEvn *)pVmEvn;
	CRegID scriptid = pVmRunEvn->GetScriptRegID();
	int count = 0;
	CScriptDBViewCache* scriptDB = pVmRunEvn->GetScriptDB();
	if(!scriptDB->GetAppItemCount(scriptid,count))
	{
		return RetFalse("ExGetDBSizeFunc can't use");
	}
	else
	{
//		CDataStream tep(SER_DISK, CLIENT_VERSION);
//		tep << count;
//		vector<unsigned char> tep1(tep.begin(),tep.end());
//        return RetRstToLua(L,tep1);
		LogPrint("vm", "ExGetDBSizeFunc:%d\n", count);
	    lua_pushnumber(L,(lua_Number)count);
		return 1 ;
	}
}

/**
 *bool GetDBValue(const unsigned long index,void* const key,unsigned char * const keylen,unsigned short maxkeylen,void* const value,unsigned short* const maxbuffer, unsigned long* const ptime)
 * 当传的第一个参数index == 0，则传了一个参数过来
 * 1.第一个是 index值
 * 当传的第一个参数index == 1，则传了两个个参数过来
 * 1.第一个是 index值
 * 2.第二是key值
 */
static int ExGetDBValueFunc(lua_State *L) {

	if (SysCfg().GetArg("-isdbtraversal", 0) == 0) {
    	return RetFalse("ExGetDBValueFunc can't use");
	}

	vector<std::shared_ptr < vector<unsigned char> > > retdata;
    if(!GetArray(L,retdata) ||(retdata.size() != 2 && retdata.size() != 1))
    {
    	return RetFalse("ExGetDBValueFunc index err1");
    }
	int index = 0;
	bool flag = true;
	memcpy(&index,&retdata.at(0).get()->at(0),sizeof(int));
	if(!(index == 0 ||(index == 1 && retdata.size() == 2)))
	{
	    return RetFalse("ExGetDBValueFunc para err2");
	}
	CRegID scriptid = pVmRunEvn->GetScriptRegID();

	vector_unsigned_char vValue;
	vector<unsigned char> vScriptKey;
	if(index == 1)
	{
		vScriptKey.assign(retdata.at(1).get()->begin(),retdata.at(1).get()->end());
	}

	CScriptDBViewCache* scriptDB = pVmRunEvn->GetScriptDB();
	flag = scriptDB->GetAppData(pVmRunEvn->GetComfirHeight(),scriptid,index,vScriptKey,vValue);
    int len = 0;
	if(flag){
	    len = RetRstToLua(L,vScriptKey) + RetRstToLua(L,vValue);
	}
	return len;
}
#endif

static int ExGetCurTxHash(lua_State *L) {
    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }
	uint256 hash = pVmRunEvn->GetCurTxHash();
    CDataStream tep(SER_DISK, CLIENT_VERSION);
    tep << hash;
    vector<unsigned char> tep1(tep.begin(),tep.end());

    vector<unsigned char> tep2(tep1.rbegin(),tep1.rend());
    return RetRstToLua(L,tep2);
}

/**
 *bool ModifyDataDBVavle(const void* const key,const unsigned char keylen, const void* const pvalue,const unsigned short valuelen)
 * 中间层传了两个参数
 * 1.第一个是 key
 * 2.第二个是 value
 */
static int ExModifyDataDBValueFunc(lua_State *L)
{
	vector<std::shared_ptr < vector<unsigned char> > > retdata;
	if(!GetDataTableWriteDataDB(L,retdata) ||retdata.size() != 2 )
    {
    	return RetFalse("ExModifyDataDBValueFunc key err1");
    }
    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }

	CRegID scriptid = pVmRunEvn->GetScriptRegID();
	bool flag = false;
	CScriptDBViewCache* scriptDB = pVmRunEvn->GetScriptDB();

//	int64_t step = 0;
	CScriptDBOperLog operlog;
	vector_unsigned_char vTemp;
	if(scriptDB->GetAppData(pVmRunEvn->GetComfirHeight(),scriptid, *retdata.at(0), vTemp)) {
		if(scriptDB->SetAppData(scriptid,*retdata.at(0),*retdata.at(1).get(),operlog))
		{
			shared_ptr<vector<CScriptDBOperLog> > m_dblog = pVmRunEvn->GetDbLog();
			m_dblog.get()->push_back(operlog);
			flag = true;
		}
	}

//	step =(((int64_t)(*retdata.at(1)).size())- (int64_t)(vTemp.size()) -1);
    return RetRstBooleanToLua(L,flag);
}


static bool GetDataTableWriteOutput(lua_State *L, vector<std::shared_ptr < std::vector<unsigned char> > > &ret) {
    if(!lua_istable(L,-1))
    {
    	LogPrint("vm","GetDataTableWriteOutput is not table\n");
    	return false;
    }
    double doubleValue = 0;
	unsigned short len = 0;
    vector<unsigned char> vBuf ;
    CVmOperate temp;
    memset(&temp,0,sizeof(temp));
    if(!(getNumberInTable(L,(char *)"addrType",doubleValue))){
    	LogPrint("vm", "addrType get fail\n");
    	return false;
    }else{
    	temp.nacctype = (unsigned char)doubleValue;
    }
	if(temp.nacctype == 1)
	{
       len = 6;
	}else if(temp.nacctype == 2){
       len = 34;
	}else{
		LogPrint("vm", "error nacctype:%d\n", temp.nacctype);
		return false;
	}
    if(!getArrayInTable(L,(char *)"accountIdTbl",len,vBuf))
    {
    	LogPrint("vm","accountidTbl not table\n");
    	return false;
    }else{
       memcpy(temp.accountid,&vBuf[0],len);
    }
    if(!(getNumberInTable(L,(char *)"operatorType",doubleValue))){
    	LogPrint("vm", "opeatortype get fail\n");
    	return false;
    }else{
    	temp.opeatortype = (unsigned char)doubleValue;
    }
    if(!(getNumberInTable(L,(char *)"outHeight",doubleValue))){
    	LogPrint("vm", "outheight get fail\n");
    	return false;
    }else{
    	temp.outheight = (unsigned int)doubleValue;
    }

	if(!getArrayInTable(L,(char *)"moneyTbl",sizeof(temp.money),vBuf))
	{
		LogPrint("vm","moneyTbl not table\n");
		return false;
	}else{
		memcpy(temp.money,&vBuf[0],sizeof(temp.money));
	}

    CDataStream tep(SER_DISK, CLIENT_VERSION);
    tep << temp;
    vector<unsigned char> tep1(tep.begin(),tep.end());
	ret.insert(ret.end(),std::make_shared<vector<unsigned char>>(tep1.begin(), tep1.end()));
	return true;
}
/**
 *bool WriteOutput( const VM_OPERATE* data, const unsigned short conter)
 * 中间层传了一个参数 ,写 CVmOperate操作结果
 * 1.第一个是输出指令
 */
static int ExWriteOutputFunc(lua_State *L)
{
	vector<std::shared_ptr < vector<unsigned char> > > retdata;

    if(!GetDataTableWriteOutput(L,retdata) ||retdata.size() != 1 )
    {
  		 return RetFalse("para err0");
  	 }
    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }
	vector<CVmOperate> source;
	CVmOperate temp;
	int Size = ::GetSerializeSize(temp, SER_NETWORK, PROTOCOL_VERSION);
	int datadsize = retdata.at(0)->size();
	int count = datadsize/Size;
	if(datadsize%Size != 0)
	{
//	  assert(0);
	 return RetFalse("para err1");
	}
	CDataStream ss(*retdata.at(0),SER_DISK, CLIENT_VERSION);

	while(count--)
	{
		ss >> temp;
      source.push_back(temp);
	}
	if(!pVmRunEvn->InsertOutputData(source)) {
		 return RetFalse("InsertOutput err");
	}else{
		/*
		* 每个函数里的Lua栈是私有的,当把返回值压入Lua栈以后，该栈会自动被清空*/
		return RetRstBooleanToLua(L,true);
	}
}


static bool GetDataTableGetAppData(lua_State *L, vector<std::shared_ptr < std::vector<unsigned char> > > &ret) {
    if(!lua_istable(L,-1))
    {
    	LogPrint("vm", "GetDataTableGetAppData is not table\n");
    	return false;
    }
    vector<unsigned char> vBuf ;
    //取脚本id
    if(!getArrayInTable(L,(char *)"id",6,vBuf))
    {
    	LogPrint("vm","idTbl not table\n");
    	return false;
    }else{
       ret.insert(ret.end(),std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));
    }

    //取key
    string key = "";
    if(!(getStringInTable(L,(char *)"key",key))){
    	LogPrint("vm","key get fail\n");
    	return false;
    }else{
//		LogPrint("vm", "key:%s\n", key);
    }

	vBuf.clear();
	for(size_t i = 0;i < key.size();i++)
	{
		vBuf.insert(vBuf.end(),key.at(i));
	}
	ret.insert(ret.end(),std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));
	return true;
}

/**
 *bool GetAppData(const void* const scriptID,void* const pkey,short len,void* const pvalve,short maxlen)
 * 中间层传了两个个参数
 * 1.脚本的id号
 * 2.数据库的key值
 */
static int ExGetAppDataFunc(lua_State *L)
{
	vector<std::shared_ptr < vector<unsigned char> > > retdata;

    if(!GetDataTableGetAppData(L,retdata) ||retdata.size() != 2 || retdata.at(0).get()->size() != 6)
    {
    	return RetFalse("ExGetAppDataFunc tep1 err1");
    }
    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }

	vector_unsigned_char vValue;
	CScriptDBViewCache* scriptDB = pVmRunEvn->GetScriptDB();
	CRegID scriptid(*retdata.at(0));
    int len = 0;
	if(!scriptDB->GetAppData(pVmRunEvn->GetComfirHeight(), scriptid, *retdata.at(1), vValue))
	{
		len = 0;
	}
	else
	{
	   //3.往函数私有栈里存运算后的结果
		len = RetRstToLua(L,vValue);
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
static int ExGetAppRegIDFunc(lua_State *L)
{
    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }
   //1.从lua取参数
   //2.调用C++库函数 执行运算
	vector_unsigned_char scriptid = pVmRunEvn->GetScriptRegID().GetVec6();
   //3.往函数私有栈里存运算后的结果
	int len = RetRstToLua(L,scriptid);
   /*
	* 每个函数里的Lua栈是私有的,当把返回值压入Lua栈以后，该栈会自动被清空*/
	return len; //number of results 告诉Lua返回了几个返回值
}
static int ExGetCurTxAccountFunc(lua_State *L)
{
    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }
   //1.从lua取参数
   //2.调用C++库函数 执行运算
	vector_unsigned_char vUserId =pVmRunEvn->GetTxAccount().GetVec6();

   //3.往函数私有栈里存运算后的结果
	int len = RetRstToLua(L,vUserId);
   /*
    * 每个函数里的Lua栈是私有的,当把返回值压入Lua栈以后，该栈会自动被清空*/
	return len; //number of results 告诉Lua返回了几个返回值
}

static int GetCurTxPayAmountFunc(lua_State *L){

    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }
	uint64_t lvalue =pVmRunEvn->GetValue();

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
	unsigned char idlen;                    //!the len of the tag
	unsigned char ID[CAppCFund::MAX_TAG_SIZE];     //! the ID for the

	const vector<unsigned char> GetIdV() const {
//		assert(sizeof(ID) >= idlen);
		vector<unsigned char> Id(&ID[0], &ID[idlen]);
		return (Id);
	}
}__attribute((aligned (1)));

static int GetUserAppAccValue(lua_State *L){

	vector<std::shared_ptr < vector<unsigned char> > > retdata;
    if(!lua_istable(L,-1))
    {
    	LogPrint("vm","is not table\n");
    	return 0;
    }
    double doubleValue = 0;
    vector<unsigned char> vBuf ;
    S_APP_ID accid;
    memset(&accid,0,sizeof(accid));
    if(!(getNumberInTable(L,(char *)"idLen",doubleValue))){
    	LogPrint("vm","idlen get fail\n");
    	return 0;
    }else{
    	accid.idlen = (unsigned char)doubleValue;
    }
    if((accid.idlen < 1) || (accid.idlen > sizeof(accid.ID))){
    	LogPrint("vm","idlen is err\n");
    	return 0;
    }
    if(!getArrayInTable(L,(char *)"idValueTbl",accid.idlen,vBuf))
    {
    	LogPrint("vm","idValueTbl not table\n");
    	return 0;
    }else{
       memcpy(&accid.ID[0],&vBuf[0],accid.idlen);
    }

    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }

   	shared_ptr<CAppUserAccout> sptrAcc;
   	uint64_t valueData = 0 ;
   	int len = 0;
   	if(pVmRunEvn->GetAppUserAccout(accid.GetIdV(),sptrAcc))
	{
   		valueData = sptrAcc->getllValues();

		CDataStream tep(SER_DISK, CLIENT_VERSION);
		tep << valueData;
		vector<unsigned char> TMP(tep.begin(),tep.end());
		len = RetRstToLua(L,TMP);
	}
    return len;
}

static bool GetDataTableOutAppOperate(lua_State *L, vector<std::shared_ptr < std::vector<unsigned char> > > &ret) {
    if(!lua_istable(L,-1))
    {
    	LogPrint("vm","is not table\n");
    	return false;
    }
    double doubleValue = 0;
    vector<unsigned char> vBuf ;
	CAppFundOperate temp;
	memset(&temp,0,sizeof(temp));
	if(!(getNumberInTable(L,(char *)"operatorType",doubleValue))){
		LogPrint("vm","opeatortype get fail\n");
		return false;
	}else{
		temp.opeatortype = (unsigned char)doubleValue;
	}
	if(!(getNumberInTable(L,(char *)"outHeight",doubleValue))){
		LogPrint("vm","outheight get fail\n");
		return false;
	}else{
		temp.outheight = (unsigned int)doubleValue;
	}

    if(!getArrayInTable(L,(char *)"moneyTbl",sizeof(temp.mMoney),vBuf))
    {
    	LogPrint("vm","moneyTbl not table\n");
    	return false;
    }else{
       memcpy(&temp.mMoney,&vBuf[0],sizeof(temp.mMoney));
    }
    if(!(getNumberInTable(L,(char *)"userIdLen",doubleValue))){
    	LogPrint("vm","appuserIDlen get fail\n");
    	return false;
    }else{
    	temp.appuserIDlen = (unsigned char)doubleValue;
    }
    if((temp.appuserIDlen < 1) || (temp.appuserIDlen > sizeof(temp.vAppuser))){
    	LogPrint("vm","appuserIDlen is err\n");
    	return false;
    }
	if(!getArrayInTable(L,(char *)"userIdTbl",temp.appuserIDlen,vBuf))
	{
		LogPrint("vm","useridTbl not table\n");
		return false;
	}else{
		memcpy(temp.vAppuser,&vBuf[0],temp.appuserIDlen);
	}
    if(!(getNumberInTable(L,(char *)"fundTagLen",doubleValue))){
		LogPrint("vm","FundTaglen get fail\n");
		return false;
    }else{
    	temp.FundTaglen = (unsigned char)doubleValue;
    }
	if((temp.FundTaglen > 0) && (temp.FundTaglen <= sizeof(temp.vFundTag)))
    {
		if(!getArrayInTable(L,(char *)"fundTagTbl",temp.FundTaglen,vBuf))
		{
			LogPrint("vm","FundTagTbl not table\n");
			return false;
		}else{
			memcpy(temp.vFundTag,&vBuf[0],temp.FundTaglen);
		}
    }
    CDataStream tep(SER_DISK, CLIENT_VERSION);
    tep << temp;
    vector<unsigned char> tep1(tep.begin(),tep.end());
	ret.insert(ret.end(),std::make_shared<vector<unsigned char>>(tep1.begin(), tep1.end()));
	return true;
}
static int GetUserAppAccFoudWithTag(lua_State *L){
	vector<std::shared_ptr < vector<unsigned char> > > retdata;
	unsigned int Size(0);
	CAppFundOperate temp;
	Size = ::GetSerializeSize(temp, SER_NETWORK, PROTOCOL_VERSION);

    if(!GetDataTableOutAppOperate(L,retdata) ||retdata.size() != 1
    	|| retdata.at(0).get()->size() !=Size)
    {
    	return RetFalse("GetUserAppAccFoudWithTag para err0");
    }

    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }

    CDataStream ss(*retdata.at(0),SER_DISK, CLIENT_VERSION);
    CAppFundOperate userfund;
    ss>>userfund;

   	shared_ptr<CAppUserAccout> sptrAcc;
    CAppCFund fund;
	int len = 0;
	if(pVmRunEvn->GetAppUserAccout(userfund.GetAppUserV(),sptrAcc))
	{
		if(!sptrAcc->GetAppCFund(fund,userfund.GetFundTagV(),userfund.outheight))	{
			return RetFalse("GetUserAppAccFoudWithTag get fail");
		}
		CDataStream tep(SER_DISK, CLIENT_VERSION);
		tep << fund.getvalue() ;
		vector<unsigned char> TMP(tep.begin(),tep.end());
		len = RetRstToLua(L,TMP);
	}
    return len;
}

static bool GetDataTableAssetOperate(lua_State *L, int nIndex, vector<std::shared_ptr < std::vector<unsigned char> > > &ret) {
    if(!lua_istable(L,nIndex))
    {
    	LogPrint("vm","is not table\n");
    	return false;
    }
    double doubleValue = 0;
    vector<unsigned char> vBuf ;
    CAssetOperate temp;
	memset(&temp,0,sizeof(temp));

    if(!getArrayInTable(L,(char *)"toAddrTbl",34,vBuf))
    {
    	LogPrint("vm","toAddrTbl not table\n");
    	return false;
    }else{

    	ret.push_back(std::make_shared<vector<unsigned char>>(vBuf.begin(), vBuf.end()));
    }

	if(!(getNumberInTable(L,(char *)"outHeight",doubleValue))){
		LogPrint("vm","outheight get fail\n");
		return false;
	}else{
		temp.outheight = (unsigned int)doubleValue;

		printf("height:%d", temp.outheight);
	}

    if(!getArrayInTable(L,(char *)"moneyTbl",sizeof(temp.mMoney),vBuf))
    {
    	LogPrint("vm","moneyTbl not table\n");
    	return false;
    }else{
       memcpy(&temp.mMoney,&vBuf[0],sizeof(temp.mMoney));
    }
    if(!(getNumberInTable(L,(char *)"fundTagLen",doubleValue))){
		LogPrint("vm","FundTaglen get fail\n");
		return false;
    }else{
    	temp.FundTaglen = (unsigned char)doubleValue;
    }
	if((temp.FundTaglen > 0) && (temp.FundTaglen <= sizeof(temp.vFundTag)))
    {
		if(!getArrayInTable(L,(char *)"fundTagTbl",temp.FundTaglen,vBuf))
		{
			LogPrint("vm","FundTagTbl not table\n");
			return false;
		}else{
			memcpy(temp.vFundTag,&vBuf[0],temp.FundTaglen);
		}
    }
    CDataStream tep(SER_DISK, CLIENT_VERSION);
    tep << temp;
    vector<unsigned char> tep1(tep.begin(),tep.end());
	ret.insert(ret.end(),std::make_shared<vector<unsigned char>>(tep1.begin(), tep1.end()));
	return true;
}

/**
 *   写 应用操作输出到 pVmRunEvn->MapAppOperate[0]
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
    {
  		 return RetFalse("ExWriteOutAppOperateFunc para err1");
  	 }

    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }

	int count = retdata.at(0).get()->size()/Size;
	CDataStream ss(*retdata.at(0),SER_DISK, CLIENT_VERSION);

	int64_t step =-1;
	while(count--)
	{
		ss >> temp;
		if(pVmRunEvn->GetComfirHeight() > nFreezeBlackAcctHeight && temp.mMoney < 0) //不能小于0,防止 上层传错金额小于20150904
		{
			return RetFalse("ExWriteOutAppOperateFunc para err2");
		}
		pVmRunEvn->InsertOutAPPOperte(temp.GetAppUserV(),temp);
		step +=Size;
	}

	/*
	* 每个函数里的Lua栈是私有的,当把返回值压入Lua栈以后，该栈会自动被清空*/
	return RetRstBooleanToLua(L,true);
}
static int ExGetBase58AddrFunc(lua_State *L){
	vector<std::shared_ptr < vector<unsigned char> > > retdata;

	if(!GetArray(L,retdata) ||retdata.size() != 1 || retdata.at(0).get()->size() != 6)
	{
		return RetFalse("ExGetBase58AddrFunc para err0");
	}

    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }
/*
	vector<unsigned char> recvkey;
	recvkey.assign(retdata.at(0).get()->begin(), retdata.at(0).get()->end());

	std::string recvaddr( recvkey.begin(), recvkey.end() );

	for(int i = 0; i < recvkey.size(); i++)
		LogPrint("vm", "==============%02X\n", recvkey[i]);

	vector<unsigned char> tmp;
	for(int i = recvkey.size() - 1; i >=0 ; i--)
		tmp.push_back(recvkey[i]);
*/
	 CKeyID addrKeyId;
	 if (!GetKeyId(*pVmRunEvn->GetCatchView(),*retdata.at(0).get(), addrKeyId)) {
		    return RetFalse("ExGetBase58AddrFunc para err1");
	 }
	 string wiccaddr = addrKeyId.ToAddress();

	 vector<unsigned char> vTemp;
	 vTemp.assign(wiccaddr.c_str(), wiccaddr.c_str()+wiccaddr.length());
	 return RetRstToLua(L,vTemp);
}

static int ExTransferContactAsset(lua_State *L) {
	vector<std::shared_ptr < vector<unsigned char> > > retdata;

    if(!GetArray(L,retdata) ||retdata.size() != 1 || retdata.at(0).get()->size() != 34)
    {
    	return RetFalse(string(__FUNCTION__)+"para  err !");
    }
    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }

	vector<unsigned char> sendkey;
	vector<unsigned char> recvkey;
	CRegID script = pVmRunEvn->GetScriptRegID();

	CRegID sendRegID =pVmRunEvn->GetTxAccount();
	CKeyID SendKeyID = sendRegID.getKeyID(*pVmRunEvn->GetCatchView());
	string addr = SendKeyID.ToAddress();
	sendkey.assign(addr.c_str(),addr.c_str()+addr.length());

	recvkey.assign(retdata.at(0).get()->begin(), retdata.at(0).get()->end());

	std::string recvaddr( recvkey.begin(), recvkey.end() );
	if(addr == recvaddr)
	{
		LogPrint("vm", "%s\n", "send addr and recv addr is same !");
		return RetFalse(string(__FUNCTION__)+"send addr and recv addr is same !");
	}

	CKeyID RecvKeyID;
	bool bValid = GetKeyId(*pVmRunEvn->GetCatchView(), recvkey, RecvKeyID);
	if(!bValid)
	{
		LogPrint("vm", "%s\n", "recv addr is not valid !");
		return RetFalse(string(__FUNCTION__)+"recv addr is not valid !");
	}

	std::shared_ptr<CAppUserAccout> temp = std::make_shared<CAppUserAccout>();
	CScriptDBViewCache* pContractScript = pVmRunEvn->GetScriptDB();

	if (!pContractScript->GetScriptAcc(script, sendkey, *temp.get())) {
		return RetFalse(string(__FUNCTION__)+"para  err3 !");
	}

	temp.get()->AutoMergeFreezeToFree(chainActive.Tip()->nHeight);

	uint64_t nMoney = temp.get()->getllValues();

	int i = 0;
	CAppFundOperate op;
	memset(&op, 0, sizeof(op));

	if(nMoney > 0) {
		op.mMoney = nMoney;
		op.outheight = 0;
		op.opeatortype = SUB_FREE_OP;
		op.appuserIDlen = sendkey.size();
		for (i = 0; i < op.appuserIDlen; i++) {
			op.vAppuser[i] = sendkey[i];
		}

		pVmRunEvn->InsertOutAPPOperte(sendkey, op);

		op.opeatortype = ADD_FREE_OP;
		op.appuserIDlen = recvkey.size();
		for (i = 0; i < op.appuserIDlen; i++) {
			op.vAppuser[i] = recvkey[i];
		}
		pVmRunEvn->InsertOutAPPOperte(recvkey, op);
	}

	vector<CAppCFund> vTemp = temp.get()->getFreezedFund();
	for(auto fund : vTemp)
	{
		op.mMoney = fund.getvalue();
		op.outheight = fund.getheight();
		op.opeatortype = SUB_TAG_OP;
		op.appuserIDlen = sendkey.size();
		for (i = 0; i < op.appuserIDlen; i++) {
			op.vAppuser[i] = sendkey[i];
		}

		op.FundTaglen = fund.GetTag().size();
		for(i = 0; i < op.FundTaglen; i++) {
			op.vFundTag[i] = fund.GetTag()[i];
		}

		pVmRunEvn->InsertOutAPPOperte(sendkey, op);

		op.opeatortype = ADD_TAG_OP;
		op.appuserIDlen = recvkey.size();
		for (i = 0; i < op.appuserIDlen; i++) {
			op.vAppuser[i] = recvkey[i];
		}

		pVmRunEvn->InsertOutAPPOperte(recvkey, op);
	}

	return RetRstBooleanToLua(L, true);

}

static int ExTransferSomeAsset(lua_State *L) {
	vector<std::shared_ptr < vector<unsigned char> > > retdata;

	unsigned int Size(0);
	CAssetOperate tempAsset;
	Size = ::GetSerializeSize(tempAsset, SER_NETWORK, PROTOCOL_VERSION);

    if(!GetDataTableAssetOperate(L, -1, retdata) ||retdata.size() != 2|| (retdata.at(1).get()->size()%Size) != 0 || retdata.at(0).get()->size() != 34)
    {
			return RetFalse(string(__FUNCTION__)+"para err !");
    }
    CVmRunEvn* pVmRunEvn = GetVmRunEvn(L);
    if(NULL == pVmRunEvn)
    {
    	return RetFalse("pVmRunEvn is NULL");
    }

    CDataStream ss(*retdata.at(1),SER_DISK, CLIENT_VERSION);
    CAssetOperate assetOp;
    ss>>assetOp;

    vector<unsigned char> sendkey;
	vector<unsigned char> recvkey;
	CRegID script = pVmRunEvn->GetScriptRegID();

	CRegID sendRegID = pVmRunEvn->GetTxAccount();
	CKeyID SendKeyID = sendRegID.getKeyID(*pVmRunEvn->GetCatchView());
	string addr = SendKeyID.ToAddress();
	sendkey.assign(addr.c_str(), addr.c_str() + addr.length());

	recvkey.assign(retdata.at(0).get()->begin(), retdata.at(0).get()->end());

	std::string recvaddr( recvkey.begin(), recvkey.end() );
	if(addr == recvaddr)
	{
		LogPrint("vm", "%s\n", "send addr and recv addr is same !");
		return RetFalse(string(__FUNCTION__)+"send addr and recv addr is same !");
	}

	CKeyID RecvKeyID;
	bool bValid = GetKeyId(*pVmRunEvn->GetCatchView(), recvkey, RecvKeyID);
	if(!bValid)
	{
		LogPrint("vm", "%s\n", "recv addr is not valid !");
		return RetFalse(string(__FUNCTION__)+"recv addr is not valid !");
	}

	uint64_t uTransferMoney = assetOp.GetUint64Value();
	if(0 == uTransferMoney)
	{
		return RetFalse(string(__FUNCTION__)+"Transfer Money is not valid !");
	}

	int nHeight = assetOp.getheight();
	if(nHeight < 0) {
		return RetFalse(string(__FUNCTION__)+"outHeight is not valid !");
	}

	int i = 0;
	CAppFundOperate op;
	memset(&op, 0, sizeof(op));
	vector<unsigned char> vtag = assetOp.GetFundTagV();
	op.FundTaglen = vtag.size();

	for(i = 0; i < op.FundTaglen; i++) {
		op.vFundTag[i] = vtag[i];
	}

	op.mMoney = uTransferMoney;
	op.outheight = nHeight;
	op.appuserIDlen = sendkey.size();

	for (i = 0; i < op.appuserIDlen; i++) {
		op.vAppuser[i] = sendkey[i];
	}
	if (nHeight > 0)
		op.opeatortype = SUB_TAG_OP;
	else
		op.opeatortype = SUB_FREE_OP;
	pVmRunEvn->InsertOutAPPOperte(sendkey, op);

	if (nHeight > 0)
		op.opeatortype = ADD_TAG_OP;
	else
		op.opeatortype = ADD_FREE_OP;
	op.appuserIDlen = recvkey.size();
	for (i = 0; i < op.appuserIDlen; i++) {
		op.vAppuser[i] = recvkey[i];
	}
	pVmRunEvn->InsertOutAPPOperte(recvkey, op);

	return RetRstBooleanToLua(L, true);

}

static int ExGetBlockTimestamp(lua_State *L) {
	int height = 0;
    if(!GetDataInt(L,height)){
    	return RetFalse("ExGetBlcokTimestamp para err1");
    }

    if(height <= 0) {
    	height = chainActive.Height() + height;
        if(height < 0) {
        	return RetFalse("ExGetBlcokTimestamp para err2");
        }
    }


	CBlockIndex *pindex = chainActive[height];
	if(!pindex) {
		return RetFalse("ExGetBlcokTimestamp get time stamp error");
	}

	if (lua_checkstack(L, sizeof(lua_Integer))) {
		lua_pushinteger(L, (lua_Integer) pindex->nTime);
		return 1;
	}

	LogPrint("vm", "%s\r\n", "ExGetBlcokTimestamp stack overflow");
	return 0;
}

static const luaL_Reg mylib[] = { //
		{"Int64Mul", ExInt64MulFunc },			//
		{"Int64Add", ExInt64AddFunc },			//
		{"Int64Sub", ExInt64SubFunc },			//
		{"Int64Div", ExInt64DivFunc },			//
		{"Sha256", ExSha256Func },			//
		{"Des", ExDesFunc },			    //
		{"VerifySignature", ExVerifySignatureFunc },   //
		{"LogPrint", ExLogPrintFunc },         //
		{"GetTxContracts",ExGetTxContractsFunc},            //
		{"GetTxAccounts",ExGetTxAccountsFunc},
		{"GetAccountPublickey",ExGetAccountPublickeyFunc},
		{"QueryAccountBalance",ExQueryAccountBalanceFunc},
		{"GetTxConFirmHeight",ExGetTxConFirmHeightFunc},
		{"GetBlockHash",ExGetBlockHashFunc},

		{"GetCurRunEnvHeight",ExGetCurRunEnvHeightFunc},
		{"WriteData",ExWriteDataDBFunc},
		{"DeleteData",ExDeleteDataDBFunc},
		{"ReadData",ExReadDataValueDBFunc},
		{"GetCurTxHash",ExGetCurTxHash},
		{"ModifyData",ExModifyDataDBValueFunc},

		{"WriteOutput",ExWriteOutputFunc},
		{"GetAppData",ExGetAppDataFunc},
		{"GetAppRegID",ExGetAppRegIDFunc},
		{"GetCurTxAccount",ExGetCurTxAccountFunc},
		{"GetCurTxPayAmount",GetCurTxPayAmountFunc},

		{"GetUserAppAccValue",GetUserAppAccValue},
		{"GetUserAppAccFoudWithTag",GetUserAppAccFoudWithTag},
		{"WriteOutAppOperate",ExWriteOutAppOperateFunc},

		{"GetBase58Addr",ExGetBase58AddrFunc},
		{"ByteToInteger",ExByteToIntegerFunc},
		{"IntegerToByte4",ExIntegerToByte4Func},
		{"IntegerToByte8",ExIntegerToByte8Func},
		{"TransferContactAsset", ExTransferContactAsset},
		{"TransferSomeAsset", ExTransferSomeAsset},
		{"GetBlockTimestamp", ExGetBlockTimestamp},
		{NULL,NULL}

		};

/*
 * 注册一个新Lua模块*/
#ifdef WIN_DLL
extern "C" __declspec(dllexport)int luaopen_mylib(lua_State *L)
#else
LUAMOD_API int luaopen_mylib(lua_State *L)
#endif
{
	luaL_newlib(L,mylib);//生成一个table,把mylibs所有函数填充进去
	return 1;
}

