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
#include "lopcodes.h"

typedef struct {
    const char *name;
    unsigned long long fuel;
}OpFuelInfo;

#define OP_TOTAL_COUNT      OP_EXTRAARG+1
static OpFuelInfo g_opFuelList[OP_TOTAL_COUNT] = {
/*--------------------------------------------------------------------------------------------------
    name_str            fuel               name         args    description
----------------------------------------------------------------------------------------------------*/
    {"OP_MOVE",     0},                 /* OP_MOVE,     A B     R(A) := R(B)                        */
    {"OP_LOADK",    0},                 /* OP_LOADK,    A Bx    R(A) := Kst(Bx)                     */
    {"OP_LOADKX",   0},                 /* OP_LOADKX,   A       R(A) := Kst(extra arg)              */
    {"OP_LOADBOOL", 0},                 /* OP_LOADBOOL, A B C   R(A) := (Bool)B; if (C) pc++        */
    {"OP_LOADNIL",  0},                 /* OP_LOADNIL,  A B     R(A), R(A+1), ..., R(A+B) := nil    */
    {"OP_GETUPVAL", 0},                 /* OP_GETUPVAL, A B     R(A) := UpValue[B]                  */
    {"OP_GETTABUP", 0},                 /* OP_GETTABUP, A B C   R(A) := UpValue[B][RK(C)]           */
    {"OP_GETTABLE", 0},                 /* OP_GETTABLE, A B C   R(A) := R(B)[RK(C)]                 */
    {"OP_SETTABUP", 0},                 /* OP_SETTABUP, A B C   UpValue[A][RK(B)] := RK(C)          */
    {"OP_SETUPVAL", 0},                 /* OP_SETUPVAL, A B     UpValue[B] := R(A)                  */
    {"OP_SETTABLE", 0},                 /* OP_SETTABLE, A B C   R(A)[RK(B)] := RK(C)                */

    {"OP_NEWTABLE", 0},                 /* OP_NEWTABLE, A B C   R(A) := {} (size = B,C)             */

    {"OP_SELF",     0},                 /* OP_SELF,     A B C   R(A+1) := R(B); R(A) := R(B)[RK(C)] */

    {"OP_ADD",      FUEL_OP_ADD},       /* OP_ADD,      A B C   R(A) := RK(B) + RK(C)                */
    {"OP_SUB",      FUEL_OP_SUB},       /* OP_SUB,      A B C   R(A) := RK(B) - RK(C)                */
    {"OP_MUL",      FUEL_OP_MUL},       /* OP_MUL,      A B C   R(A) := RK(B) * RK(C)                */
    {"OP_MOD",      FUEL_OP_MOD},       /* OP_MOD,      A B C   R(A) := RK(B) % RK(C)                */
    {"OP_POW",      FUEL_OP_POW},       /* OP_POW,      A B C   R(A) := RK(B) ^ RK(C)                */
    {"OP_DIV",      FUEL_OP_DIV},       /* OP_DIV,      A B C   R(A) := RK(B) / RK(C)                */
    {"OP_IDIV",     FUEL_OP_IDIV},      /* OP_IDIV,     A B C   R(A) := RK(B) // RK(C)               */
    {"OP_BAND",     FUEL_OP_BAND},      /* OP_BAND,     A B C   R(A) := RK(B) & RK(C)                */
    {"OP_BOR",      FUEL_OP_BOR},       /* OP_BOR,      A B C   R(A) := RK(B) | RK(C)                */
    {"OP_BXOR",     FUEL_OP_BXOR},      /* OP_BXOR,     A B C   R(A) := RK(B) ~ RK(C)                */
    {"OP_SHL",      FUEL_OP_SHL},       /* OP_SHL,      A B C   R(A) := RK(B) << RK(C)               */
    {"OP_SHR",      FUEL_OP_SHR},       /* OP_SHR,      A B C   R(A) := RK(B) >> RK(C)               */
    {"OP_UNM",      FUEL_OP_UNM},       /* OP_UNM,      A B     R(A) := -R(B)                        */
    {"OP_BNOT",     FUEL_OP_BNOT},      /* OP_BNOT,     A B     R(A) := ~R(B)                        */
    {"OP_NOT",      FUEL_OP_NOT},       /* OP_NOT,      A B     R(A) := not R(B)                     */
    {"OP_LEN",      FUEL_OP_LEN},       /* OP_LEN,      A B     R(A) := length of R(B)               */
    {"OP_CONCAT",   FUEL_OP_CONCAT},    /* OP_CONCAT,   A B C   R(A) := R(B).. ... ..R(C)            */

    {"OP_JMP",      0},                 /* OP_JMP,      A sBx   pc+=sBx;
                                                                if (A) then
                                                                    close all upvalues >= R(A - 1)   */
    {"OP_EQ",       FUEL_OP_EQ},        /* OP_EQ,       A B C   if ((RK(B) == RK(C)) ~= A) then pc++ */
    {"OP_LT",       FUEL_OP_LT},        /* OP_LT,       A B C   if ((RK(B) <  RK(C)) ~= A) then pc++ */
    {"OP_LE",       FUEL_OP_LE},        /* OP_LE,       A B C   if ((RK(B) <= RK(C)) ~= A) then pc++ */

    {"OP_TEST",     FUEL_OP_TEST},      /* OP_TEST,     A C     if not (R(A) <=> C) then pc++        */
    {"OP_TESTSET",  FUEL_OP_TESTSET},   /* OP_TESTSET,  A B C   if (R(B) <=> C) then
                                                                    R(A) := R(B) else pc++           */

    {"OP_CALL",     0},                 /* OP_CALL,     A B C   R(A), ... ,
                                                                R(A+C-2) := R(A)(R(A+1),
                                                                ... ,R(A+B-1))                       */
    {"OP_TAILCALL", 0},                 /* OP_TAILCALL, A B C   return R(A)(R(A+1), ... ,R(A+B-1))   */
    {"OP_RETURN",   0},                 /* OP_RETURN,   A B     return R(A), ... ,R(A+B-2)
                                                                (see note)                           */

    {"OP_FORLOOP",  0},                 /* OP_FORLOOP,  A sBx   R(A)+=R(A+2);
                                                                if R(A) <?= R(A+1)
                                                                    then { pc+=sBx; R(A+3)=R(A) }    */
    {"OP_FORPREP",  0},                 /* OP_FORPREP,  A sBx   R(A)-=R(A+2); pc+=sBx                */

    {"OP_TFORCALL", 0},                 /* OP_TFORCALL, A C     R(A+3), ... ,
                                                                R(A+2+C) := R(A)(R(A+1), R(A+2));    */
    {"OP_TFORLOOP", 0},                 /* OP_TFORLOOP, A sBx   if R(A+1) ~= nil then
                                                                    { R(A)=R(A+1); pc += sBx }       */

    {"OP_SETLIST",  0},                 /* OP_SETLIST,  A B C   R(A)[(C-1)*FPF+i] := R(A+i),
                                                                1 <= i <= B                         */

    {"OP_CLOSURE",  0},                 /* OP_CLOSURE,  A Bx    R(A) := closure(KPROTO[Bx])         */

    {"OP_VARARG",   0},                 /* OP_VARARG,   A B     R(A), R(A+1), ...,
                                                                R(A+B-2) = vararg                   */

    {"OP_EXTRAARG", 0}                  /* OP_EXTRAARG  Ax      extra (larger) argument for
                                                                    previous opcode                 */
};

#ifndef max
#define max(a, b) ( (a) > (b)? (a) : (b))
#endif

#ifndef min
#define min(a, b) ( (a) < (b)? (a) : (b))
#endif

#define IsBurnerStarted(L) (L->burnerState.isStarted != 0)

#define IsBurnerRuning(L) (L->burnerState.isStarted != 0 && L->burnerState.error == 0)

#define TraceBurning(L, caption, format, ...) \
    if (L->burnerState.tracer != NULL) {\
        L->burnerState.tracer(L, caption, format, __VA_ARGS__);\
    }

static int CheckBurnedOk(lua_State *L, const char *errMsg, ...) {
    if (lua_IsBurnedOut(L)) {
        L->burnerState.error = 1;
        va_list argp;
        va_start(argp, errMsg);
        luaL_where(L, 1);
        lua_pushvfstring(L, errMsg, argp);
        va_end(argp);
        lua_concat(L, 2);

        lua_lock(L);
        api_checknelems(L, 1);

        luaD_throw(L, LUA_ERR_BURNEDOUT);
        return 0;
    }
    return 1;
}


LUA_API int lua_StartBurner(lua_State *L, unsigned long long fuelLimit, int version) {
    assert(L->burnerState.isStarted == 0 && "burner has been started");

    L->burnerState.isStarted        = 1;
    L->burnerState.error            = 0;
    L->burnerState.fuelLimit        = fuelLimit;
    L->burnerState.version          = version;
    L->burnerState.fuel             = 0;
    L->burnerState.fuelRefund       = 0;
    L->burnerState.fuelStep         = 0;
    L->burnerState.allocMemSize     = 0;
    L->burnerState.fuelOperator     = 0;
    L->burnerState.fuelStore        = 0;
    L->burnerState.fuelAccount      = 0;
    L->burnerState.fuelFunction     = 0;
    L->burnerState.tracer           = NULL;
    return 1;
}

lua_burner_state *lua_GetBurnerState(lua_State *L) {
    if (IsBurnerStarted(L)) {
        return &L->burnerState;
    }
    return NULL;
}

LUA_API int lua_BurnMemory(lua_State *L, void *block, size_t osize, size_t nsize, int version) {

    if (IsBurnerRuning(L) && version <= L->burnerState.version) {
        if (nsize > 0) {  // alloc memory
            L->burnerState.allocMemSize += nsize;
            TraceBurning(L, "lua_BurnMemory", "alloc memory, version=%d, size=%llu\n",
                version, nsize);
            return CheckBurnedOk(L, "Burned-out lua_BurnMemory");
        }
    }
    return 1;
}

LUA_API int lua_BurnStep(lua_State *L, unsigned long long step, int version) {
    if (IsBurnerRuning(L) && version <= L->burnerState.version) {
        L->burnerState.fuel += step;
        L->burnerState.fuelStep += step;
        TraceBurning(L, "lua_BurnStep", "version=%d, step=%llu\n", version, step);
        return CheckBurnedOk(L, "Burned-out lua_BurnStep");
    }
    return 1;
}

LUA_API int lua_BurnOperator(lua_State *L, int op, int version) {
    if (IsBurnerRuning(L) && version <= L->burnerState.version) {
        if (op >= 0 && op <= OP_TOTAL_COUNT - 1) {
            unsigned long long fuel = g_opFuelList[op].fuel;
            L->burnerState.fuel += fuel;
            L->burnerState.fuelOperator += fuel;
            TraceBurning(L, "lua_BurnOperator", "version=%d, op=%s, fuel=%llu\n",
                version, g_opFuelList[op].name, fuel);
            return CheckBurnedOk(L, "Burned-out lua_BurnOperator");
        } else {
            TraceBurning(L, "lua_BurnOperator", "unknown op, version=%d, op=%d\n",
                version, op);
        }
    }
    return 1;
}

LUA_API int lua_BurnStoreSet(lua_State *L, size_t keySize, size_t oldDataSize, size_t newDataSize, int version) {
    if (IsBurnerRuning(L) && version <= L->burnerState.version) {
        unsigned long long fuel = 0;
        unsigned long long refund = 0;
        const char *action = "unchanged";
        if (newDataSize == oldDataSize) {
            action = "reset";
            fuel = newDataSize * FUEL_STORE_RESET;
        } else if (newDataSize > oldDataSize) {
            if (oldDataSize == 0) {
                action = "new";
                fuel = (keySize + newDataSize) * FUEL_STORE_ADDED;
            } else { // increase
                action = "increase";
                fuel = (newDataSize - oldDataSize) * FUEL_STORE_ADDED + oldDataSize * FUEL_STORE_RESET;
            }
        } else { // (newDataSize < oldDataSize) // decrease, refund
            if (newDataSize == 0) {
                refund = (oldDataSize) * FUEL_STORE_REFUND;
            } else {
                refund = (oldDataSize - newDataSize) * FUEL_STORE_REFUND;
                fuel = newDataSize * FUEL_STORE_RESET;
            }
        }
        L->burnerState.fuel += fuel;
        L->burnerState.fuelStore += fuel;
        L->burnerState.fuelRefund += refund;
        TraceBurning(L, "lua_BurnStoreSet", "%s, version=%d, keySize=%u, oldDataSize=%u, newDataSize=%u, fuel=%llu, refund=%llu\n",
            action, version, keySize, oldDataSize, newDataSize, refund);
        return CheckBurnedOk(L, "Burned-out lua_BurnStoreSet");
    }
    return 1;
}

LUA_API int lua_BurnStoreUnchanged(lua_State *L, size_t keySize, size_t dataSize, int version) {
    if (IsBurnerRuning(L) && version <= L->burnerState.version) {
        unsigned long long fuel = (keySize + dataSize) * FUEL_STORE_UNCHANGED;
        L->burnerState.fuel += FUEL_STORE_UNCHANGED;
        L->burnerState.fuelStore += fuel;
        TraceBurning(L, "lua_BurnStoreUnchanged", "version=%d, keySize=%u, dataSize=%u, fuel=%llu\n",
            version, fuel);
        return CheckBurnedOk(L, "Burned-out lua_BurnStoreUnchanged");
    }
    return 1;
}

LUA_API int lua_BurnStoreGet(lua_State *L, size_t keySize, size_t dataSize, int version) {
    if (IsBurnerRuning(L) && version <= L->burnerState.version) {
        unsigned long long fuel = (keySize + dataSize) * FUEL_STORE_GET;
        L->burnerState.fuel += fuel;
        L->burnerState.fuelStore += fuel;
        TraceBurning(L, "lua_BurnStoreGet", "version=%d, keySize=%u, dataSize=%u, fuel=%llu\n",
            version, keySize, dataSize, fuel);
        return CheckBurnedOk(L, "Burned-out lua_BurnStoreGet");
    }
    return 1;
}

LUA_API int lua_BurnAccountOperate(lua_State *L, const char *funcName, size_t count, int version) {
    if (IsBurnerRuning(L) && version <= L->burnerState.version) {
        unsigned long long fuel = count * FUEL_ACCOUNT_OPERATE;
        L->burnerState.fuel += fuel;
        L->burnerState.fuelAccount += fuel;
        TraceBurning(L, "lua_BurnAccountOperate", "%s, version=%d, count=%u, fuel=%llu\n",
            funcName, version, count, fuel);
        return CheckBurnedOk(L, "Burned-out lua_BurnAccountOperate %s()", funcName);
    }
    return 1;
}

LUA_API int lua_BurnAccountGet(lua_State *L, const char *funcName, unsigned long long fuel, int version) {
        if (IsBurnerRuning(L) && version <= L->burnerState.version) {
        L->burnerState.fuel += fuel;
        L->burnerState.fuelAccount += fuel;
        TraceBurning(L, "lua_BurnAccountGet", "%s, version=%d, fuel=%llu\n",
            funcName, version, fuel);
        return CheckBurnedOk(L, "Burned-out lua_BurnAccountGet %s()", funcName);
    }
    return 1;
}

LUA_API int lua_BurnFuncCall(lua_State *L, const char *funcName, unsigned long long fuel,
                               int version) {
    if (IsBurnerRuning(L) && version <= L->burnerState.version) {
        L->burnerState.fuel += fuel;
        L->burnerState.fuelFunction += fuel;
        TraceBurning(L, "lua_BurnFuncCall", "%s, version=%d, fuel=%llu\n",
                     funcName, version, fuel);
        return CheckBurnedOk(L, "Burned-out lua_BurnFuncCall %s", funcName);
    }
    return 1;
}

LUA_API int lua_BurnFuncData(lua_State *L, const char *funcName, unsigned long long callFuel,
    size_t dataSize, size_t unitSize, unsigned long long fuelPerUnit, int version) {

    if (IsBurnerRuning(L) && version <= L->burnerState.version) {
        unsigned long long fuel = callFuel + lua_CalcFuelBySize(dataSize, unitSize, fuelPerUnit);
        L->burnerState.fuel += fuel;
        L->burnerState.fuelFunction += fuel;
        TraceBurning(L, "lua_BurnFuncData",
                     "%s, version=%d, dataSize=%u, unitSize=%u, fuelPerUnit=%llu, fuel=%llu\n",
                     funcName, dataSize, unitSize, fuelPerUnit, version, fuel);
        return CheckBurnedOk(L, "Burned-out lua_BurnFuncData %s", funcName);
    }
    return 1;
}

LUA_API unsigned long long lua_GetBurnedFuel(lua_State *L) {
    if (IsBurnerStarted(L)) {
        unsigned long long fuel = L->burnerState.fuel + lua_GetMemoryFuel(L);
        unsigned long long refund = min(fuel / 2, L->burnerState.fuelRefund);
        assert(fuel >= refund);
        return fuel - refund;
    }
    return 0;
}

int lua_IsBurnedOut(lua_State *L) {
    if (IsBurnerStarted(L) &&
        lua_GetBurnedFuel(L) > L->burnerState.fuelLimit) {
        return 1;
    }
    return 0;
}

LUA_API unsigned long long lua_GetMemoryFuel(lua_State *L) {
    return lua_CalcFuelBySize(L->burnerState.allocMemSize, BURN_MEM_UNIT_SIZE, FUEL_MEM_ADDED);
}

LUA_API lua_burner_trace_cb lua_SetBurnerTracer(lua_State *L, lua_burner_trace_cb tracer) {
    lua_burner_trace_cb oldTracer = L->burnerState.tracer;
    L->burnerState.tracer = tracer;
    return oldTracer;
}