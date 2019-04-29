/*
** Copyright (c) 2019- The WaykiChain Core Developers
** Distributed under the MIT/X11 software license, see the accompanying
** file COPYING or http://www.opensource.org/licenses/mit-license.php
*/
#ifndef L_BURNER_H
#define L_BURNER_H

#include "lua.h"

// burn lua stack step only
#define BURN_VER_1_1                (10001)

// burn lua base res, include instruction, memory, store
#define BURN_VER_2_1                (20001)

// enable all version on
#define BURN_VER_ALL                INT_MAX

// enable all fearure off
#define BURN_VER_NONE               0

#define BURN_MEM_UNIT_SIZE          32

#define BURN_STORE_UNIT_SIZE        32

#define FUEL_STEP1                  1
#define FUEL_MEM_ADDED              3
#define FUEL_STORE_ADDED            20000
#define FUEL_STORE_UNCHANGED        200
#define FUEL_STORE_GET              200
#define FUEL_STORE_REFUND           10000

struct lua_burner_state {
    int                 isStarted;         /** 0 is stoped, otherwise is started */
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

void lua_BurnError (lua_State *L, const char *fmt, ...);


#endif // L_BURNER_H