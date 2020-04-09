// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef VM_LUA_LMYLIB_H
#define VM_LUA_LMYLIB_H

#include <openssl/des.h>
#include <vector>

#include "lua/lua.hpp"
#include "luavmrunenv.h"
#include "commons/SafeInt3.hpp"

int32_t ExInt64MulFunc(lua_State *L);

int32_t ExInt64AddFunc(lua_State *L);

int32_t ExInt64SubFunc(lua_State *L);

int32_t ExInt64DivFunc(lua_State *L);

/**
 *bool SHA256(void const* pfrist, const unsigned short len, void * const pout)
 * This function receives an input param from a middle layer:
 *   1. The first param is the target string to be hashed twice in a BitCoin way
 */
int32_t ExSha256Func(lua_State *L);

/**
 *bool SHA256Once(void const* pfrist, const unsigned short len, void * const pout)
 * This function receives an input param from a middle layer:
 *   1. The first param is the target string to be hashed once
 */
int32_t ExSha256OnceFunc(lua_State *L);

/**
 *unsigned short Des(void const* pdata, unsigned short len, void const* pkey, unsigned short keylen, bool IsEn, void * const pOut,unsigned short poutlen)
 * 这个函数式从中间层传了三个个参数过来:
 * 1.第一个是要被加密数据或者解密数据
 * 2.第二格式加密或者解密的key值
 * 3.第三是标识符，是加密还是解密
 *
 * {
 *  dataLen = 0,
 *  data = {},
 *  keyLen = 0,
 *  key = {},
 *  flag = 0
 * }
 */
int32_t ExDesFunc(lua_State *L);

/**
 *bool SignatureVerify(void const* data, unsigned short datalen, void const* key, unsigned short keylen,
        void const* phash, unsigned short hashlen)
 * 这个函数式从中间层传了三个个参数过来:
 * 1.第一个是签名前的原始数据
 * 2.第二个是签名的公钥(public key)
 * 3.第三是已签名的数据
 *
 *{
 *  dataLen = 0,
 *  data = {},
 *  pubKeyLen = 0,
 *  pubKey = {},
 *  signatureLen = 0,
 *  signature = {}
 * }
 */
int32_t ExVerifySignatureFunc(lua_State *L);

int32_t ExGetTxContractFunc(lua_State *L);

/**
 *void LogPrint(const void *pdata, const unsigned short datalen,PRINT_FORMAT flag )
 * 这个函数式从中间层传了两个个参数过来:
 * 1.第一个是打印数据的表示符号，true是一十六进制打印,否则以字符串的格式打印
 * 2.第二个是打印的字符串
 */
int32_t ExLogPrintFunc(lua_State *L);

/**
 *unsigned short GetAccounts(const unsigned char *txid, void* const paccount, unsigned short maxlen)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是 hash
 */
int32_t ExGetTxRegIDFunc(lua_State *L);

int32_t ExByteToIntegerFunc(lua_State *L);

int32_t ExIntegerToByte4Func(lua_State *L);

int32_t ExIntegerToByte8Func(lua_State *L);

/**
 *unsigned short GetAccountPublickey(const void* const accounid,void * const pubkey,const unsigned short maxlength)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是 账户id,六个字节
 */
int32_t ExGetAccountPublickeyFunc(lua_State *L);

/**
 *bool QueryAccountBalance(const unsigned char* const account,Int64* const pBalance)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是 账户id,六个字节
 */
int32_t ExQueryAccountBalanceFunc(lua_State *L);

/**
 *unsigned long GetTxConfirmHeight(const void * const txid)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个入参: hash,32个字节
 */
int32_t ExGetTxConfirmHeightFunc(lua_State *L);


/**
 *bool GetBlockHash(const unsigned long height,void * const pblochHash)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是 int类型的参数
 */
int32_t ExGetBlockHashFunc(lua_State *L);

int32_t ExGetCurRunEnvHeightFunc(lua_State *L);

/**
 *bool WriteDataDB(const void* const key,const unsigned char keylen,const void * const value,const unsigned short valuelen,const unsigned long time)
 * 这个函数式从中间层传了三个个参数过来:
 * 1.第一个是 key值
 * 2.第二个是value值
 */
int32_t ExWriteDataDBFunc(lua_State *L);

/**
 *bool DeleteDataDB(const void* const key,const unsigned char keylen)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是 key值
 */
int32_t ExDeleteDataDBFunc(lua_State *L);

/**
 *unsigned short ReadDataValueDB(const void* const key,const unsigned char keylen, void* const value,unsigned short const maxbuffer)
 * 这个函数式从中间层传了一个参数过来:
 * 1.第一个是 key值
 */
int32_t ExReadDataDBFunc(lua_State *L);

int32_t ExGetCurTxHash(lua_State *L);

/**
 *bool ExModifyDataDBFunc(const void* const key,const unsigned char keylen, const void* const pvalue,const unsigned short valuelen)
 * 中间层传了两个参数
 * 1.第一个是 key
 * 2.第二个是 value
 */
int32_t ExModifyDataDBFunc(lua_State *L);

/**
 * WriteOutput - lua api
 * bool WriteOutput( vmOperTable )
 * @param vmOperTable:          operation param table
 * {
 *      addrType: (number, required),     address type of accountIdTbl, enum(REGID=1,ADDR=2)
 *      accountIdTbl: (array, required)   account id, array format
 *      operatorType: (number, required)  operator type, enum(ADD_FREE=1, SUB_FREE=2)
 *      outHeight: (number, required)     timeout height, use by contract script
 *      moneyTbl: (array, required)       money amount, serialized format of int64 (little endian)
 *      moneySymbol: (string, optional)   money symbol, must be valid symbol, such as WICC|WUSD, default is WICC
 * }
 * @return write succeed or not
 */
int32_t ExWriteOutputFunc(lua_State *L);

/**
 *bool GetContractData(const void* const scriptID,void* const pkey,short len,void* const pvalve,short maxlen)
 * 中间层传了两个个参数
 * 1.脚本的id号
 * 2.数据库的key值
 */
int32_t ExGetContractDataFunc(lua_State *L);

/**
 * 取目的账户ID
 * @param ipara
 * @param pVmEvn
 * @return
 */
int32_t ExGetContractRegIdFunc(lua_State *L);

int32_t ExGetCurTxPayAmountFunc(lua_State *L);


int32_t ExGetUserAppAccValueFunc(lua_State *L);


int32_t ExGetUserAppAccFundWithTagFunc(lua_State *L);

/**
 * 写应用操作输出到 pVmRunEnv->mapAppFundOperate[0]
 * @param ipara
 * @param pVmEvn
 * @return
 */
int32_t ExWriteOutAppOperateFunc(lua_State *L);

int32_t ExGetBase58AddrFunc(lua_State *L);

int32_t ExTransferContractAsset(lua_State *L);
int32_t ExTransferSomeAsset(lua_State *L);
int32_t ExGetBlockTimestamp(lua_State *L);

int32_t ExLimitedRequire(lua_State *L);

int32_t ExLuaPrint(lua_State *L);

///////////////////////////////////////////////////////////////////////////////
// new function added in MAJOR_VER_R2

/**
 * TransferAccountAsset - lua api
 * boolean TransferAccountAsset( transferTable )
 * @param transferTable:          transfer param table
 * {
 *   isContractAccount: (boolean, required), Is contract account or tx sender' account
 *   toAddressType: (number, required)       The to address type of the transfer, REGID = 1, BASE58 = 2
 *   toAddress: (array, required)            The to address of the transfer, array format
 *   tokenType: (string, required)           Token type of the transfer, such as WICC | WUSD
 *   tokenAmount: (array, required)          Token amount of the transfer
 * }
 * @return succeed or not
 */
int32_t ExTransferAccountAssetFunc(lua_State *L);

/**
 * TransferAccountAssets - lua api
 * boolean TransferAccountAssets( transferTables )
 * @param transferTables:          array of transfer param table
 * [
 *   {
 *     isContractAccount: (boolean, required), Is contract account or tx sender' account
 *     toAddressType: (number, required)       The to address type of the transfer, REGID = 1, BASE58 = 2
 *     toAddress: (array, required)            The to address of the transfer, array format
 *     tokenType: (string, required)           Token type of the transfer, such as WICC | WUSD
 *     tokenAmount: (array, required)          Token amount of the transfer
 *   },
 *   ...
 * ]
 * @return succeed or not
 */
int32_t ExTransferAccountAssetsFunc(lua_State *L);

/**
 * GetCurTxInputAsset - lua api
 * table GetCurTxTransferAsset()
 * get symbol and amount of current tx asset input by sender of tx
 * @return table of asset info:
 * {
 *     symbol: (string), symbol of current tx input asset
 *     amount: (array)   amount of current tx input asset, array format
 * },
 */
int32_t ExGetCurTxInputAssetFunc(lua_State *L);

/**
 * GetAccountAsset - lua api
 * boolean GetAccountAsset( paramTable )
 * get asset of account by address
 * @param paramTable: table    get asset param table
 * {
 *   addressType: (number, required)       address type, REGID = 1, BASE58 = 2
 *   address: (array, required)            address, array format
 *   tokenType: (string, required)         Token type of the transfer, such as WICC | WUSD
 * }
 * @return asset info table or none
 * {
 *     symbol: (string), transfer symbol of current tx
 *     amount: (integer)   transfer amount of current tx
 * },
 */
int32_t ExGetAccountAssetFunc(lua_State *L);


///////////////////////////////////////////////////////////////////////////////
// new function added in MAJOR_VER_R3

/**
 * GetAssetPrice - lua api
 * int GetAssetPrice( paramTable )
 * get asset price of baseSymbol/quoteSymbol pair
 * @param paramTable: table     get asset price param table
 * {
 *   baseSymbol: (string, required)       base symbol of price, such as WICC
 *   quoteSymbol: (array, required)        quote symbol of price, such as USD
 * }
 * @return price (int)
 */
int32_t ExGetAssetPriceFunc(lua_State *L);

lua_CFunction GetLuaMylib(HeightType height);

#endif //VM_LUA_LMYLIB_H