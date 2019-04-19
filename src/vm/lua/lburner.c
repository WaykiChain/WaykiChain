/*
** Copyright (c) 2017-2019 The WaykiChain Core Developers
** Distributed under the MIT/X11 software license, see the accompanying
** file COPYING or http://www.opensource.org/licenses/mit-license.php
*/

#define lapi_c
#define LUA_CORE

#include "lua.h"
#include "lstate.h"
#include <assert.h>

#define IS_BURNING_STARTED(L) (L->burningState.is_started != 0)

LUA_API int lua_startburner(lua_State *L, unsigned long long maxStep) {
    assert(L->burningState.is_started == 0 && "burner has been started twice");
    L->burningState.is_started    = 1;
    L->burningState.maxStep       = maxStep;
    L->burningState.step          = 0;
    L->burningState.allocMemSize  = 0;
    L->burningState.allocMemTimes = 0;
    L->burningState.freeMemSize   = 0;
    L->burningState.freeMemTimes  = 0;
    return 1;
}

lua_burner_state *lua_getburnerstate(lua_State *L) {
    if (IS_BURNING_STARTED(L)) {
        return &L->burningState;
    }
    return NULL;
}

LUA_API int lua_burnmemory(lua_State *L, void *block, size_t osize, size_t nsize) {
    if (!IS_BURNING_STARTED(L)) {
        return 1;
    }
    if (nsize > 0) {  // alloc memory
        L->burningState.allocMemSize += nsize;
        L->burningState.allocMemTimes++;
    } else {
        if (block != NULL) {
            L->burningState.freeMemSize += osize;
            L->burningState.freeMemTimes++;
        }
    }
    return 1;
}

LUA_API int lua_burnstep(lua_State *L, unsigned long long step) {
    if (IS_BURNING_STARTED(L)) {
        L->burningState.step += step;
        return !lua_isburnedout(L);
    }
    return 1;
}

LUA_API unsigned long long lua_getburnedstep(lua_State *L) {
    if (IS_BURNING_STARTED(L)) {
        return L->burningState.step;
    }
    return 0;
}

int lua_isburnedout(lua_State *L) {
    if (IS_BURNING_STARTED(L) && L->burningState.step >= L->burningState.maxStep) {
        return 1;
    }
    return 0;
}
