/*
** Copyright (c) 2019- The WaykiChain Core Developers
** Distributed under the MIT/X11 software license, see the accompanying
** file COPYING or http://www.opensource.org/licenses/mit-license.php
*/
#ifndef L_BURNER_H
#define L_BURNER_H

#include "lua.h"
#include "fuel.h"

typedef void (*lua_burner_trace_cb) (lua_State *L, const char* caption, const char* format, ...);

struct lua_burner_state {
    int                 isStarted;          /** 0 is stoped, otherwise is started */
    int                 error;              /** 0 is ok, otherwise has error */
    int                 version;            /** burner version */
    unsigned long long  fuelLimit;          /** max fuel can be burned */
    unsigned long long  fuel;               /** burned fuel except memory */
    unsigned long long  fuelRefund;         /** the refund fuel */
    unsigned long long  allocMemSize;       /** total alloc memory size */
    unsigned long long  fuelStep;           /** total step */
    unsigned long long  fuelOperator;       /** total fuel of operator of lua */
    unsigned long long  fuelStore;          /** total fuel of store operation */
    unsigned long long  fuelAccount;        /** total fuel of account operation (transfer output) */
    unsigned long long  fuelFunction;       /** total fuel of extended functions except store operation
                                                and account operation(transfer output) functions */

    lua_burner_trace_cb tracer;             /** trace the burning */
};

typedef struct lua_burner_state lua_burner_state;

/**
 * start burner until current lua stop or burned-out
 * fuelLimit      the max step for burning
 * version      the burner version
 */
int lua_StartBurner(lua_State *L, unsigned long long  fuelLimit, int version);

lua_burner_state* lua_GetBurnerState(lua_State *L);

/**
 * burn memory
 * burned out if return 0, otherwise is burned ok.
 * the burned memory will be convert to burned step to check if burned out
 */
LUA_API int lua_BurnMemory(lua_State *L, void *block, size_t osize, size_t nsize, int version);

/**
 * burn step
 * burned out if return 0, otherwise is burned ok.
 */
LUA_API int lua_BurnStep(lua_State *L, unsigned long long step, int version);

LUA_API int lua_BurnOperator(lua_State *L, int op, int version);

LUA_API int lua_BurnStoreSet(lua_State *L, size_t keySize, size_t oldDataSize, size_t newDataSize, int version);

LUA_API int lua_BurnStoreUnchanged(lua_State *L, size_t keySize, size_t dataSize, int version);

LUA_API int lua_BurnStoreGet(lua_State *L, size_t keySize, size_t dataSize, int version);

LUA_API int lua_BurnAccountOperate(lua_State *L, const char *funcName, size_t count, int version);
#define LUA_BurnAccountOperate(L, count, version) lua_BurnAccountOperate(L, __FUNCTION__, count, version)

LUA_API int lua_BurnAccountGet(lua_State *L, const char *funcName, unsigned long long fuel, int version);
#define LUA_BurnAccountGet(L, fuel, version) lua_BurnAccountGet(L, __FUNCTION__, fuel, version)

LUA_API int lua_BurnFuncCall(lua_State *L, const char* funcName, unsigned long long fuel, int version);
#define LUA_BurnFuncCall(L, fuel, version) lua_BurnFuncCall(L, __FUNCTION__, fuel, version)

#define lua_BurnFuncCallTag(L, tag, fuel, version) \
    lua_BurnFuncCall(L, __FUNCTION__ ## " " ## tag, fuel, version)

LUA_API int lua_BurnFuncData(lua_State *L, const char *funcName, unsigned long long callFuel,
    size_t dataSize, size_t unitSize, unsigned long long fuelPerUnit, int version);

#define LUA_BurnFuncData(L, callFuel, dataSize, unitSize, fuelPerUnit,  version) \
    lua_BurnFuncData(L, __FUNCTION__, callFuel, dataSize, unitSize, fuelPerUnit,  version)


#define LUA_BurnFuncDataTag(L, tag, callFuel, dataSize, unitSize, fuelPerUnit,  version) \
    lua_BurnFuncData(L, __FUNCTION__ ## " " ## tag, callFuel, dataSize, unitSize, fuelPerUnit,  version)

/** get burned step */
LUA_API unsigned long long lua_GetBurnedFuel(lua_State *L);

/** check is burned out */
LUA_API int lua_IsBurnedOut(lua_State *L);

/** get the burned fuel of memory */
LUA_API unsigned long long lua_GetMemoryFuel(lua_State *L);

#define lua_CalcFuelBySize(size, unitSize, fuelPerUnit) \
    ( unitSize == 0 ? 0 : ((size + unitSize - 1) / unitSize) * fuelPerUnit )

/**
 * set the burner trace callback function, rerurn the old one
 */
LUA_API lua_burner_trace_cb lua_SetBurnerTracer(lua_State *L, lua_burner_trace_cb tracer);

#endif // L_BURNER_H