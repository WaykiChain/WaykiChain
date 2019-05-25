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

#define FUEL_STORE_ADDED        600
#define FUEL_STORE_RESET        150
#define FUEL_STORE_UNCHANGED    3
#define FUEL_STORE_GET          6
#define FUEL_STORE_REFUND       450

#define FUEL_MEM_ADDED          3 // fuel for burning memory  per new 32 bytes

#define FUEL_CALL_Int64Mul              5
#define FUEL_CALL_Int64Add              3
#define FUEL_CALL_Int64Sub              3
#define FUEL_CALL_Int64Div              5
#define FUEL_CALL_ByteToInteger	        10
#define FUEL_CALL_IntegerToByte4	    6
#define FUEL_CALL_IntegerToByte8	    8
#define FUEL_CALL_GetBlockHash	        4
#define FUEL_CALL_GetBlockTimestamp	    4
#define FUEL_CALL_GetCurTxHash	        4
#define FUEL_CALL_GetCurRunEnvHeight	4
#define FUEL_CALL_GetCurTxAccount	    4
#define FUEL_CALL_GetCurTxPayAmount	    4
#define FUEL_CALL_GetContractRegId	    4
#define FUEL_CALL_GetTxRegID	        200
#define FUEL_CALL_GetTxConfirmHeight	200
#define FUEL_CALL_GetAccountPublickey	200
#define FUEL_CALL_GetBase58Addr	        200

#define FUEL_CALL_Sha256Once            30
#define FUEL_DATA32_Sha256Once          6
#define FUEL_CALL_Sha256                50
#define FUEL_DATA32_Sha256              10
#define FUEL_CALL_VerifySignature       200
#define FUEL_DATA32_VerifySignature     10

#define FUEL_CALL_LogPrint              375
#define FUEL_DATA1_LogPrint             8
#define FUEL_CALL_GetCurTxContract      4
#define FUEL_CALL_GetTxContract         200
#define FUEL_DATA32_GetTxContract       3

#define FUEL_CALL_DesBasic              50
#define FUEL_DATA8_DesBasic             2
#define FUEL_CALL_DesTriple             140
#define FUEL_DATA8_DesTriple            2

#define FUEL_ACCOUNT_OPERATE            4500
#define FUEL_ACCTOUNT_NEW               25000
#define FUEL_ACCOUNT_GET_VALUE          400
#define FUEL_ACCOUNT_GET_FUND_TAG       600
#define FUEL_ACCTOUNT_UNCHANGED         200
 
#endif // FUEL_H