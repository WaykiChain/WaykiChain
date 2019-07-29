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

enum AccountIDType {
    NULL_ID = 0,
    NICK_ID,
    REG_ID,
    ADDRESS
};

/*
std::string split implementation by using delimeter as a character.
*/
std::vector<std::string> split(std::string strToSplit, char delimeter)
{
    std::stringstream ss(strToSplit);
    std::string item;
	std::vector<std::string> splittedStrings;
    while (std::getline(ss, item, delimeter))
	{
		splittedStrings.push_back(item);
    }
	return splittedStrings;
}

/*
std::string split implementation by using delimeter as an another string
*/
std::vector<std::string> split(std::string stringToBeSplitted, std::string delimeter)
{
	std::vector<std::string> splittedString;
	int startIndex = 0;
	int  endIndex = 0;
	while( (endIndex = stringToBeSplitted.find(delimeter, startIndex)) < stringToBeSplitted.size() ) {
		std::string val = stringToBeSplitted.substr(startIndex, endIndex - startIndex);
		splittedString.push_back(val);
		startIndex = endIndex + delimeter.size();
	}

	if(startIndex < stringToBeSplitted.size()) {
		std::string val = stringToBeSplitted.substr(startIndex);
		splittedString.push_back(val);
	}
	return splittedString;

}

bool is_number(const std::string& s)
{
    return !s.empty() && std::find_if(s.begin(),
        s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

// [N|R|A]:address
// NickID (default) | RegID | Address
bool ParseRpcInputAccountId(const string &comboAccountIdStr, tuple<AccountIDType, string> &comboAccountId) {
    vector<string> comboAccountIdArr = split(comboAccountIdStr, ':');
    switch (comboMoneyArr.size()) {
        case 0: {
            get<0>(comboAccountId) = AccountIDType::NICK_ID;
            get<1>(comboAccountId) = comboAccountIdArr[0];
            break;
        }
        case 1: {
            if (comboAccountIdArr[0].size() > 1)
                return false;

            char tag = toupper(comboAccountIdArr[0][0]);
            if (tag == 'N') {
                get<0>(comboAccountId) = AccountIDType::NICK_ID;

            } else if (tag == 'R') {
                get<0>(comboAccountId) = AccountIDType::REG_ID;

            } else if (tag == 'A') {
                get<0>(comboAccountId) = AccountIDType::ADDRESS;

            } else
                return false;

            get<1>(comboAccountId) = comboAccountIdArr[1];

            break;
        }
        default: break;
    }

    return true;
}

// [symbol]:amount:[unit]
// [WICC(default)|WUSD|WGRT|...]:amount:[sawi(default)]
bool ParseRpcInputValue(const string &comboMoneyStr, tuple<TokenSymbol, int64_t amount, CoinUnitName> &comboMoney) {
	vector<string> comboMoneyArr = split(comboMoneyStr, ':');

    switch (comboMoneyArr.size()) {
        case 0: {
            if (!is_number(comboMoneyArr[0])
                return false;

            int64_t iValue = std::atoll(comboMoneyArr[0].c_str());
            if (iValue < 0)
                return false;

            get<0>(comboMoney) = "WICC";
            get<1>(comboMoney) = (uint64_t) iValue;
            get<2>(comboMoney) = "sawi";
            break;
        }
        case 1: {
            if (is_number(comboMoneyArr[0])) {
                int64_t iValue = std::atoll(comboMoneyArr[0].c_str());
                if (iValue < 0)
                    return false;

                if (!CoinUnitTypeTable.count(comboMoneyArr[1]))
                    return false;

                get<0>(comboMoney) = "WICC";
                get<1>(comboMoney) = (uint64_t) iValue;
                get<2>(comboMoney) = comboMoneyArr[1];
            } else if (is_number(comboMoneyArr[1])) {
                int64_t iValue = std::atoll(comboMoneyArr[1].c_str());
                if (iValue < 0)
                    return false;

                get<0>(comboMoney) = comboMoneyArr[0];
                get<1>(comboMoney) = (uint64_t) iValue;
                get<2>(comboMoney) = "sawi";
            } else {
                return false;
            }

            break;
        }
        case 2: {
            if (comboMoneyArr.size() > 12)
                return false;

            if (!is_number(comboMoneyArr[1]))
                return false;

            int64_t iValue = std::atoll(comboMoneyArr[1].c_str());
            if (iValue < 0)
                return false;

            if (!CoinUnitTypeTable.count(comboMoneyArr[2]))
                return false;

            get<0>(comboMoney) = comboMoneyArr[0];
            get<1>(comboMoney) = (uint64_t) iValue;
            get<2>(comboMoney) = comboMoneyArr[2];
            break;
        }
        default:
            return false;
    }

    return true;
}

#endif  // RPC_CORE_COMMONS_H