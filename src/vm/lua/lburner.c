/*
** Copyright (c) 2017-2019 The WaykiChain Core Developers
** Distributed under the MIT/X11 software license, see the accompanying
** file COPYING or http://www.opensource.org/licenses/mit-license.php
*/

#define lapi_c
#define LUA_CORE

#include <assert.h>
#include "lburner.h"
#include "lstate.h"
#include "lauxlib.h"
#include "lapi.h"
#include "ldo.h"

#ifndef max
#define max(a, b) ( (a) > (b)? (a) : (b))
#endif

#define IS_BURNING_STARTED(L) (L->burnerState.is_started != 0)

LUA_API int lua_StartBurner(lua_State *L, unsigned long long fuelLimit, int version) {
    assert(L->burnerState.is_started == 0 && "burner has been started twice");

    L->burnerState.is_started       = 1;
    L->burnerState.fuelLimit        = fuelLimit;
    L->burnerState.version          = version;
    L->burnerState.fuel             = 0;    
    L->burnerState.fuelRefund       = 0;
    L->burnerState.step             = 0;
    L->burnerState.allocMemSize     = 0;
    L->burnerState.allocMemTimes    = 0;
    L->burnerState.freeMemSize      = 0;
    L->burnerState.freeMemTimes     = 0;
    return 1;
}

lua_burner_state *lua_GetBurnerState(lua_State *L) {
    if (IS_BURNING_STARTED(L)) {
        return &L->burnerState;
    }
    return NULL;
}

LUA_API int lua_BurnMemory(lua_State *L, void *block, size_t osize, size_t nsize, int version) {
    if (!IS_BURNING_STARTED(L) || version >= L->burnerState.version) {
        return 1;
    }
    if (nsize > 0) {  // alloc memory
        L->burnerState.allocMemSize += nsize;
        L->burnerState.allocMemTimes++;
    } else {
        if (block != NULL) {
            L->burnerState.freeMemSize += osize;
            L->burnerState.freeMemTimes++;
        }
    }
    return 1;
}

LUA_API int lua_BurnStep(lua_State *L, unsigned long long step, int version) {
    if (IS_BURNING_STARTED(L) || version >= L->burnerState.version) {
        L->burnerState.fuel += step;
        L->burnerState.step += step;
        return !lua_IsBurnedOut(L);
    }
    return 1;
}

#define CalcStoreFuelAdded(size) lua_CalcFuelBySize(size, BURN_STORE_UNIT_SIZE, FUEL_STORE_ADDED)

#define CalcStoreFuelUnchanged(size) lua_CalcFuelBySize(size, BURN_STORE_UNIT_SIZE, FUEL_STORE_UNCHANGED)

#define CalcStoreFuelGet(size) lua_CalcFuelBySize(size, BURN_STORE_UNIT_SIZE, FUEL_STORE_GET)

#define CalcStoreRefund(size) lua_CalcFuelBySize(size, BURN_STORE_UNIT_SIZE, FUEL_STORE_REFUND)

LUA_API int lua_BurnStoreSet(lua_State *L, size_t oldSize, size_t newSize, int version) {
    if (IS_BURNING_STARTED(L) || version >= L->burnerState.version) {
        if (newSize == oldSize) { // unchanged
            L->burnerState.fuel += CalcStoreFuelUnchanged(max(newSize, 1));
        } else if (newSize > oldSize) { // increase
            if (oldSize == 0) { // new
                L->burnerState.fuel += CalcStoreFuelAdded(newSize);
            } else { // increase
                L->burnerState.fuel += CalcStoreFuelAdded(newSize) - CalcStoreFuelAdded(oldSize);
            }
        } else { // (newSize < oldSize) // decrease, refund
            L->burnerState.fuelRefund += CalcStoreRefund(oldSize) - CalcStoreRefund(newSize);
        }
        return !lua_IsBurnedOut(L);
    }
    return 1;
}


LUA_API int lua_BurnStoreGet(lua_State *L, size_t size, int version) {
    if (IS_BURNING_STARTED(L) || version >= L->burnerState.version) {
        L->burnerState.fuel += CalcStoreFuelGet(max(size, 1));
        return !lua_IsBurnedOut(L);
    }
    return 1;
}

LUA_API unsigned long long lua_GetBurnedFuel(lua_State *L) {
    if (IS_BURNING_STARTED(L)) {
        unsigned long long fuel = L->burnerState.fuel + lua_GetMemoryFuel(allocMemSize);
        if (fuel > L->burnerState.fuelRefund) {
            return fuel - L->burnerState.fuelRefund;
        }
    }
    return 0;
}

int lua_IsBurnedOut(lua_State *L) {
    if (IS_BURNING_STARTED(L) &&
        lua_GetBurnedFuel(L) > L->burnerState.fuelLimit) {
        return 1;
    }
    return 0;
}

void lua_BurnError(lua_State *L, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  luaL_where(L, 1);
  lua_pushvfstring(L, fmt, argp);
  va_end(argp);
  lua_concat(L, 2);

  lua_lock(L);
  api_checknelems(L, 1);
  
  luaD_throw(L, LUA_ERR_BURNEDOUT);
}

