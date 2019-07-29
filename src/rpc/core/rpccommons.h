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

using namespace std;
using namespace json_spirit;

const int MAX_RPC_SIG_STR_LEN = 65 * 1024; // 65K

string RegIDToAddress(CUserID &userId);
bool GetKeyId(const string &addr, CKeyID &keyId);
Object GetTxDetailJSON(const uint256& txid);
Array GetTxAddressDetail(std::shared_ptr<CBaseTx> pBaseTx);

Object SubmitTx(CUserID &userId, CBaseTx &tx);

namespace JSON {
    const Value& GetObjectFieldValue(const Value &jsonObj, const string &fieldName);
}


namespace RPC_PARAM {

    // will throw error it check failed
    void CheckOrderCoinSymbol(const TokenSymbol &coinSymbol);

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
bool ParseRpcInpuAccountId(const string &comboAccountIdStr, tuple<AccountIDType, string> &comboAccountId);

// [symbol]:amount:[unit]
// [WICC(default)|WUSD|WGRT|...]:amount:[sawi(default)]
bool ParseRpcInputMoney(const string &comboMoneyStr, ComboMoney &comboMoney);

#endif  // RPC_CORE_COMMONS_H