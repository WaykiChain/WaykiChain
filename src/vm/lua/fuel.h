/*
** Copyright (c) 2019- The WaykiChain Core Developers
** Distributed under the MIT/X11 software license, see the accompanying
** file COPYING or http://www.opensource.org/licenses/mit-license.php
*/
#ifndef FUEL_H
#define FUEL_H

typedef int lua_burner_version;

/* the burn version belong to the Fearure Fork Version */
/* burn lua stack step only */
#define BURN_VER_R1                (10001)

/* burn lua base resource, include instruction, memory, store */
#define BURN_VER_R2                (10002)

/* enable all version on */
#define BURN_VER_NEWEST            BURN_VER_R2

/** burn memory unit size */
#define BURN_MEM_UNIT_SIZE          32

/** burn store unit size */
#define BURN_STORE_UNIT_SIZE        32

#define BURN_VER_STEP_V1    BURN_VER_R1
#define BURN_VER_STEP_V2    BURN_VER_R2

#define FUEL_STEP1              1
#define FUEL_OP_ADD             3
#define FUEL_OP_SUB             3    
#define FUEL_OP_MUL             5
#define FUEL_OP_DIV             5
#define FUEL_OP_IDIV            5
#define FUEL_OP_MOD             8
#define FUEL_OP_POW             10
#define FUEL_OP_BXOR            3
#define FUEL_OP_UNM             3
#define FUEL_OP_BAND            3
#define FUEL_OP_BOR             3
#define FUEL_OP_SHR             3
#define FUEL_OP_SHL             3
#define FUEL_OP_BNOT            3
#define FUEL_OP_EQ              3   /* ==, ~= */
#define FUEL_OP_LT              3   /* <, > */

#define FUEL_OP_LE              3   /* <=, >= */
#define FUEL_OP_TEST            3   /* and, or */
#define FUEL_OP_TESTSET         3   /* and, or */

#define FUEL_OP_NOT             3   /* not */
#define FUEL_OP_CONCAT          3   /* .. */
#define FUEL_OP_LEN             32  /* # */


#define FUEL_MEM_ADDED          3
#define FUEL_STORE_ADDED        20000
#define FUEL_STORE_UNCHANGED    200
#define FUEL_STORE_GET          200
#define FUEL_STORE_REFUND       10000


#endif // FUEL_H