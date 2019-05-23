/*
** Copyright (c) 2019- The WaykiChain Core Developers
** Distributed under the MIT/X11 software license, see the accompanying
** file COPYING or http://www.opensource.org/licenses/mit-license.php
*/
#ifndef L_BURNER_H
#define L_BURNER_H

#include "lua.h"

typedef void (*lua_burner_trace_cb) (lua_State *L, const char* caption, const char* format, ...);

struct lua_burner_state {
    int                 isStarted;          /** 0 is stoped, otherwise is started */
    int                 error;              /** 0 is ok, otherwise has error */
    int                 version;            /** burner version */
    unsigned long long  fuelLimit;          /** max fuel can be burned */
    unsigned long long  fuel;               /** burned fuel exclude memory */
    unsigned long long  fuelRefund;         /** the refund fuel */
    unsigned long long  allocMemSize;       /** total alloc memory size */
    /** detail */
    unsigned long long  allocMemTimes;      /** total alloc memory times */
    unsigned long long  freeMemSize;        /** total free memory size */
    unsigned long long  freeMemTimes;       /** total free memory times */
    unsigned long long  step;               /** total step */
    lua_burner_trace_cb tracer;              /** trace the burning */
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

LUA_API int lua_BurnStoreSet(lua_State *L, size_t oldSize, size_t newSize, int version);

LUA_API int lua_BurnStoreGet(lua_State *L, size_t size, int version);

/** get burned step */
LUA_API unsigned long long lua_GetBurnedFuel(lua_State *L);

/** check is burned out */
LUA_API int lua_IsBurnedOut(lua_State *L);

/** get the burned fuel of memory */
LUA_API unsigned long long lua_GetMemoryFuel(lua_State *L);

#define lua_CalcFuelBySize(size, unitSize, fuelPerUnit) \
    ( unitSize == 0 ? 0 : ((size + unitSize - 1) / unitSize) * fuelPerUnit )

LUA_API void lua_BurnError (lua_State *L, const char *fmt, ...);

/**
 * set the burner trace callback function, rerurn the old one
 */
LUA_API lua_burner_trace_cb lua_SetBurnerTracer(lua_State *L, lua_burner_trace_cb tracer);

#endif // L_BURNER_H