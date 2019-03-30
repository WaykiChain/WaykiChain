// ComDirver.cpp: implementation of the CComDirver class.
//
//////////////////////////////////////////////////////////////////////

#include "vmlua.h"
#include "lua/lua.hpp"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <openssl/des.h>
#include <vector>
#include "hash.h"
#include "key.h"
#include "main.h"
#include "tx.h"
#include "vmrunenv.h"
//#include "Typedef.h"

#if 0
typedef struct NumArray{
	int size;          // 数组大小
	double values[1];  //数组缓冲区
}NumArray;

/*
 * 获取userdatum*/
static NumArray *checkarray(lua_State *L){
	//检查在栈中指定位置的对象是否为带有给定名字的metatable的userdatum
    void *ud = luaL_checkudata(L,1,"LuaBook.array");
    luaL_argcheck(L,ud != NULL,1,"'array' expected");
    return (NumArray *)ud;
}
/*
 * 获取索引处的指针*/
static double *getelem(lua_State *L){
    NumArray *a = checkarray(L);
    int index = luaL_checkint(L,2);
    luaL_argcheck(L,1 <= index && index <= a->size,2,"index out of range");
    /*return element address*/
    return &a->values[index - 1];
}
/*
 * 创建新数组*/
int newarray(lua_State *L){
   int n = luaL_checkint(L,1);   //检查证实的luaL_checknumber的变体
   size_t nbytes = sizeof(NumArray) + (n -1) * sizeof(double);

	/*一个userdatum 提供一个在Lua中没有预定义操作的raw内存区域；
          按照指定的大小分配一块内存，将对应的userdatum放到栈内,并返回内存块的地址*/
   NumArray *a = (NumArray *)lua_newuserdata(L,nbytes);
   luaL_getmetatable(L,"LuaBook.array");   //获取registry中的tname对应的metatable
   lua_setmetatable(L,-2);   //将表出栈并将其设置为给定位置的对象的metatable  就是新的userdatum
   a->size = n;
   return 1; /*new userdatnum is already on the statck*/
}
/*
 * 存储元素,array.set(array,index,value)*/
int setarray(lua_State *L){
#if 0
	NumArray *a = (NumArray *)lua_touserdata(L,1);
	int index = luaL_checkint(L,2);
    double value = luaL_checknumber(L,3);

    luaL_argcheck(L,a != NULL,1,"'array' expected");
    luaL_argcheck(L,1 <= index && index <= a->size,2,"index out of range");
    a->values[index -1] = value;
#else
    double newvalue = luaL_checknumber(L,3);
    *getelem(L) = newvalue;
#endif
	return 0;
}
/*
 * 获取一个数组元素*/
int getarray(lua_State *L){
#if 0
	NumArray *a = (NumArray *)lua_touserdata(L,1);
    int index = luaL_checkint(L,2);

    luaL_argcheck(L,a != NULL,1,"'array' expected");
    luaL_argcheck(L,1 <= index && index <= a->size,2,"index out of range");
    lua_pushnumber(L,a->values[index - 1]);
#else
    lua_pushnumber(L,*getelem(L));
#endif
    return 1;
}
/*
 * 获取数组的大小*/
int getsize(lua_State *L){
#if 0
	NumArray *a = (NumArray *)lua_touserdata(L,1);
	luaL_argcheck(L,a != NULL,1,"'array' expected");
#else
	NumArray *a = checkarray(L);
#endif
	lua_pushnumber(L,a->size);
	return 1;
}
static const struct luaL_Reg arraylib[] = {
		{"new",newarray},
		{"set",setarray},
		{"get",getarray},
		{"size",getsize},
		{NULL,NULL}
};
static int luaopen_array(lua_State *L){
	/*创建数组userdata将要用到的metatable*/
	luaL_newmetatable(L,"LuaBook.array");
	luaL_openlib(L,"array",arraylib,0);

	/*now the statck has the metatable at index 1 and
	 * 'array' at index 2*/
    lua_pushstring(L,"__index");
    lua_pushstring(L,"get");
    lua_gettable(L,2); /*get array.get*/
    lua_settable(L,1); /*metatable.__index - array.get*/

    lua_pushstring(L,"__newindex");
    lua_pushstring(L,"set");
    lua_gettable(L,2); /*get array.get*/
    lua_settable(L,1); /*metatable.__newindex - array.get*/
	return 0;
}

#endif

CVmlua::CVmlua(const vector<unsigned char> &vContractScript,
               const vector<unsigned char> &vContractCallParams) {
    unsigned long len = 0;
    memset(contractCallArguments, 0, sizeof(contractCallArguments));
    memset(contractScript, 0, sizeof(contractScript));

    len = vContractScript.size();
    if (len >= sizeof(contractScript)) {
        throw runtime_error("CVmlua::CVmlua() length of vContractScript exception");
    }
    memcpy(contractScript, &vContractScript[0], len);

    // must be no more than kContractArgumentMaxSize
    unsigned short count = vContractCallParams.size();
    if (count > kContractArgumentMaxSize) {
        throw runtime_error("CVmlua::CVmlua() length of vContractCallParams exception");
    }
    memcpy(contractCallArguments, &count, 2);
    memcpy(&contractCallArguments[2], &vContractCallParams[0], count);
}

CVmlua::~CVmlua() {}

#ifdef WIN_DLL
extern "C" __declspec(dllexport) int luaopen_mylib(lua_State *L);
#else
LUAMOD_API int luaopen_mylib(lua_State *L);
#endif

/** ommited lua lib for safety reasons
 *
//	   {LUA_COLIBNAME, luaopen_coroutine},
//	   {LUA_IOLIBNAME, luaopen_io},
//	   {LUA_OSLIBNAME, luaopen_os},
//	   {LUA_STRLIBNAME, luaopen_string},
//	   {LUA_MATHLIBNAME, luaopen_math},
//	   {LUA_UTF8LIBNAME, luaopen_utf8},
//	   {LUA_DBLIBNAME, luaopen_debug},
 *
 */
void vm_openlibs(lua_State *L) {
    static const luaL_Reg lualibs[] = {
        {"base", luaopen_base},           {LUA_LOADLIBNAME, luaopen_package},
        {LUA_TABLIBNAME, luaopen_table},  {LUA_MATHLIBNAME, luaopen_math},
        {LUA_STRLIBNAME, luaopen_string}, {NULL, NULL}};

    const luaL_Reg *lib;
    for (lib = lualibs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1); /* remove lib */
    }
}

tuple<bool, string> CVmlua::CheckScriptSyntax(const char *filePath) {
    lua_State *lua_state = luaL_newstate();
    if (NULL == lua_state) {
        LogPrint("vm", "luaL_newstate error\n");
        return std::make_tuple(false, string("luaL_newstate error\n"));
    }
    vm_openlibs(lua_state);
    luaL_requiref(lua_state, "mylib", luaopen_mylib, 1);

    int nRet = luaL_loadfile(lua_state, filePath);
    if (nRet) {
        const char *errStr = lua_tostring(lua_state, -1);
        lua_close(lua_state);
        return std::make_tuple(false, string(errStr));
    }

    lua_close(lua_state);

    return std::make_tuple(true, string("OK"));
}

tuple<uint64_t, string> CVmlua::Run(uint64_t maxstep, CVmRunEnv *pVmRunEnv) {
    if (maxstep == 0) {
        return std::make_tuple(-1, string("maxstep == 0\n"));
    }
    if (NULL == pVmRunEnv) {
        return std::make_tuple(-1, string("pVmRunEnv == NULL\n"));
    }

    // 1.创建Lua运行环境
    lua_State *lua_state = luaL_newstate();
    if (NULL == lua_state) {
        LogPrint("vm", "luaL_newstate error\n");
        return std::make_tuple(-1, string("luaL_newstate error\n"));
    }
    /*
       //3.注册Lua标准库并清空栈
       const luaL_Reg *lib = lualibs;
       for(;lib->func != NULL;lib++)
       {
               lib->func(lua_state);
               lua_settop(lua_state,0);
       }
    */
    //打开需要的库
    vm_openlibs(lua_state);

    // 3.注册自定义模块
    luaL_requiref(lua_state, "mylib", luaopen_mylib, 1);

    // 4.往lua脚本传递合约内容
    lua_newtable(lua_state);  //新建一个表,压入栈顶
    lua_pushnumber(lua_state, -1);
    lua_rawseti(lua_state, -2, 0);

    unsigned short count = 0;
    memcpy(&count, contractCallArguments, 2);  //外面已限制，合约内容小于4096字节
    for (unsigned short n = 0; n < count; n++) {
        lua_pushinteger(lua_state, contractCallArguments[2 + n]);  // value值放入
        lua_rawseti(lua_state, -2, n + 1);                         // set table at key 'n + 1'
    }
    lua_setglobal(lua_state, "contract");

    // 传递pVmScriptRun指针，以便后面代码引用，去掉了使用全局变量保存该指针
    lua_pushlightuserdata(lua_state, pVmRunEnv);
    lua_setglobal(lua_state, "VmScriptRun");
    LogPrint("vm", "pVmRunEnv=%p\n", pVmRunEnv);

    // 5. Load the contract script
    long long step = maxstep;
    if (luaL_loadbuffer(lua_state, (char *)contractScript, strlen((char *)contractScript), "line") ||
        lua_pcallk(lua_state, 0, 0, 0, 0, NULL, &step)) {
        const char *pError = lua_tostring(lua_state, -1);
        string strError    = strprintf("luaL_loadbuffer failed: %s\n", pError ? pError : "unknown");
        LogPrint("vm", "%s", strError);
        step = -1;
        lua_close(lua_state);
        LogPrint("vm", "run step=%ld\n", step);
        return std::make_tuple(step, strError);
    }

    // 6. account balance check setting: default is closed if not such setting in the script
    pVmRunEnv->SetCheckAccount(false);
    int res = lua_getglobal(lua_state, "gCheckAccount");
    LogPrint("vm", "lua_getglobal:%d\n", res);
    if (LUA_TBOOLEAN == res) {
        if (lua_isboolean(lua_state, -1)) {
            bool bCheck = lua_toboolean(lua_state, -1);
            LogPrint("vm", "lua_toboolean:%d\n", bCheck);
            pVmRunEnv->SetCheckAccount(bCheck);
        }
    }
    lua_pop(lua_state, 1);

    // 7. shudown the Lua VM
    lua_close(lua_state);
    LogPrint("vm", "run step=%ld\n", step);

    if (step < 0) {
        return std::make_tuple(step, string("execute tx contract run step exceeds the max step limit\n"));
    }

    return std::make_tuple(step, string("script runs ok"));
}
