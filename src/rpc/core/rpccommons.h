// Copyright (c) 2017-2019 The WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef RPC_CORE_COMMONS_H
#define RPC_CORE_COMMONS_H

#include <string>
#include <tuple>
#include <vector>

#include "entities/id.h"
#include "commons/json/json_spirit.h"
#include "entities/asset.h"
#include "entities/account.h"
#include "entities/cdp.h"
#include "tx/tx.h"
#include "persistence/dexdb.h"
#include "persistence/pricefeeddb.h"
#include "persistence/contractdb.h"

using namespace std;
using namespace json_spirit;
class CBlockHeader;

#define JSON_RPC_ASSERT(expr, code, ...)                                                           \
    if (!(expr)) {                                                                                 \
        string msg = tfm::format(__VA_ARGS__);                                                     \
        std::ostringstream o;                                                                      \
        o << msg;                                                                                  \
        throw JSONRPCError(code, o.str().c_str());                                                 \
    }

#define JSON_RPC_CAPTURE_AND_RETHROW                                                               \
    catch (wasm_chain::exception & e) {                                                            \
        JSON_RPC_ASSERT(false, e.code(), e.to_detail_string())                                     \
    }                                                                                              \
    catch (...) {                                                                                  \
        throw;                                                                                     \
    }

#define RESPONSE_RPC_HELP(expr, msg)                                                               \
    if ((expr)) {                                                                                  \
        throw runtime_error(msg);                                                                  \
    }

string RegIDToAddress(CUserID &userId);
Object GetTxDetailJSON(CCacheWrapper &cw, const CBlockHeader &block,
                       const shared_ptr<CBaseTx> pBaseTx, const CTxCord &txCord);
Object GetTxDetailJSON(const uint256& txid);
Array GetTxAddressDetail(std::shared_ptr<CBaseTx> pBaseTx);

Object SubmitTx(const CKeyID &keyid, CBaseTx &tx);

namespace JSON {
    const Value& GetObjectFieldValue(const Value &jsonObj, const string &fieldName);
    bool  GetObjectFieldValue(const Value &jsonObj, const string &fieldName,Value& returnValue);
    const char* GetValueTypeName(const Value_type &valueType);

    Object ToJson(const CAccountDBCache &accountCache, const CReceipt &receipt);
    Array ToJson(const CAccountDBCache &accountCache, const vector<CReceipt> &receipts);
}

namespace RPC_PARAM {

    uint32_t GetUint32(const Value &jsonValue);
    uint64_t GetUint64(const Value &jsonValue);

    void CheckTokenAmount(const TokenSymbol &symbol, const uint64_t amount);

    ComboMoney GetComboMoney(const Value &jsonValue, const TokenSymbol &defaultSymbol = SYMB::WICC);

    uint64_t GetTxMinFeeBy(const TxType txType, const TokenSymbol &symbol);

    // @return pair(ComboMoney txFee, uint64_t minTxFeeAmount)
    pair<ComboMoney, uint64_t> ParseTxFee(const Array &params, const size_t index,
                                          const TxType txType);
    ComboMoney GetFee(const Array& params, const size_t index, const TxType txType);
    uint64_t GetWiccFee(const Array& params, const size_t index, const TxType txType);


    CUserID ParseUserIdByAddr(const Value &jsonValue);
    CRegID ParseRegId(const Value &jsonValue, const string &title);
    CRegID ParseRegId(const Array& params, const size_t index, const string &title, const CRegID &defaultValue);

    CKeyID GetUserKeyId(const CUserID &userId);

    CUserID ParseUserId(const Value &jsonValue);
    CUserID GetUserId(const Value &jsonValue, const bool senderUid = false);
    CUserID GetRegId(const Value &jsonValue);
    CRegID  GetRegId(const Array& params, const size_t index, const CRegID &defaultRegId);

    string GetLuaContractScript(const Value &jsonValue);
    CKeyID GetKeyId(const Value &jsonValue);

    uint64_t GetPrice(const Value &jsonValue);

    uint256 GetTxid(const Value &jsonValue, const string &paramName, const bool canBeEmpty = false);

    CAccount GetUserAccount(CAccountDBCache &accountCache, const CUserID &userId);

    // will throw error it check failed
    TokenSymbol GetOrderCoinSymbol(const Value &jsonValue);
    TokenSymbol GetOrderAssetSymbol(const Value &jsonValue);

    TokenSymbol GetAssetIssueSymbol(const Value &jsonValue);
    TokenName GetAssetName(const Value &jsonValue);

    string GetBinStrFromHex(const Value &jsonValue, const string &paramName);

    void CheckAccountBalance(CAccount &account, const TokenSymbol &tokenSymbol, const BalanceOpType opType,
                             const uint64_t value);

    void CheckActiveOrderExisted(CDexDBCache &dexCache, const uint256 &orderTxid);

    void CheckOrderSymbols(const string &title, const TokenSymbol &coinSymbol,
                           const TokenSymbol &assetSymbol);
    // parse hex str
    bool ParseHex(const string &hexStr, string &binStrOut, string &errStrOut);

    TxType ParseTxType(const Value &jsonValue);

    Value GetWasmContractArgs(const Value &jsonValue);

    uint64_t GetPriceByCdp(CPriceFeedCache &priceFeedCache, CUserCDP &cdp);

    CUniversalContractStore GetWasmContract(CContractDBCache &contractCache, const CRegID &regid);

    CBlock ReadBlock(CBlockIndex* pBlockIndex);
}

/*
std::string split implementation by using delimeter as a character.
*/
std::vector<std::string> split(std::string strToSplit, char delimeter);

/*
std::string split implementation by using delimeter as an another string
*/
std::vector<std::string> split(std::string stringToBeSplitted, std::string delimeter);

inline bool is_number(const std::string& s) {
    return !s.empty() && std::find_if(s.begin(),
        s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}


bool is_decimal(const string& s );

// [symbol]:amount:[unit]
// [WICC(default)|WUSD|WGRT|...]:amount:[sawi(default)]
bool ParseRpcInputMoney(const string &comboMoneyStr, ComboMoney &comboMoney, const TokenSymbol defaultSymbol = SYMB::WICC);

#endif  // RPC_CORE_COMMONS_H