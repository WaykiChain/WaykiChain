// Copyright (c) 2017-2019 The WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef RPC_CORE_COMMONS_H
#define RPC_CORE_COMMONS_H

#include <string>
#include <tuple>
#include <vector>

#include "entities/id.h"
#include "json/json_spirit.h"
#include "entities/asset.h"
#include "entities/account.h"
#include "tx/tx.h"
#include "persistence/dexdb.h"

using namespace std;
using namespace json_spirit;

string RegIDToAddress(CUserID &userId);
bool GetKeyId(const string &addr, CKeyID &keyId);
Object GetTxDetailJSON(const uint256& txid);
Array GetTxAddressDetail(std::shared_ptr<CBaseTx> pBaseTx);

Object SubmitTx(const CUserID &userId, CBaseTx &tx);


namespace JSON {
    const Value& GetObjectFieldValue(const Value &jsonObj, const string &fieldName);
    const char* GetValueTypeName(const Value_type &valueType);
}

namespace RPC_PARAM {

    ComboMoney GetComboMoney(const Value &jsonValue, const TokenSymbol &defaultSymbol = SYMB::WICC);

    ComboMoney GetFee(const Array& params, size_t index, TxType txType);
    uint64_t GetWiccFee(const Array& params, size_t index, TxType txType);

    CUserID GetUserId(const Value &jsonValue);

    uint64_t GetPrice(const Value &jsonValue);

    uint256 GetTxid(const Value &jsonValue, const string &paramName, bool canBeEmpty = false);

    CAccount GetUserAccount(CAccountDBCache &accountCache, const CUserID &userId);

    // will throw error it check failed
    TokenSymbol GetOrderCoinSymbol(const Value &jsonValue);
    TokenSymbol GetOrderAssetSymbol(const Value &jsonValue);

    TokenSymbol GetAssetIssueSymbol(const Value &jsonValue);
    TokenName GetAssetName(const Value &jsonValue);

    string GetBinStrFromHex(const Value &jsonValue, const string &paramName);

    void CheckAccountBalance(CAccount &account, const TokenSymbol &tokenSymbol,
                            const BalanceOpType opType, const uint64_t &value);

    void CheckActiveOrderExisted(CDexDBCache &dexCache, const uint256 &orderTxid);

    // parse hex str
    bool ParseHex(const string &hexStr, string &binStrOut, string &errStrOut);
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

// [N|R|A]:address
// NickID (default) | RegID | Address
bool ParseRpcInputAccountId(const string &comboAccountIdStr, tuple<AccountIDType, string> &comboAccountId);

// [symbol]:amount:[unit]
// [WICC(default)|WUSD|WGRT|...]:amount:[sawi(default)]
bool ParseRpcInputMoney(const string &comboMoneyStr, ComboMoney &comboMoney, const TokenSymbol defaultSymbol = SYMB::WICC);

#endif  // RPC_CORE_COMMONS_H