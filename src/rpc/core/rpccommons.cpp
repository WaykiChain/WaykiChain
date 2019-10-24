// Copyright (c) 2017-2019 The WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "rpccommons.h"

#include "entities/key.h"
#include "init.h"
#include "main.h"
#include "rpcserver.h"
#include "vm/luavm/luavmrunenv.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"

#include <fstream>

using namespace std;

/*
std::string split implementation by using delimeter as a character.
*/
std::vector<std::string> split(std::string strToSplit, char delimeter) {
    std::stringstream ss(strToSplit);
    std::string item;
    std::vector<std::string> splittedStrings;
    while (std::getline(ss, item, delimeter)) {
        splittedStrings.push_back(item);
    }
    return splittedStrings;
}

/*
std::string split implementation by using delimeter as an another string
*/
std::vector<std::string> split(std::string stringToBeSplitted, std::string delimeter) {
    std::vector<std::string> splittedString;
    size_t startIndex = 0;
    size_t endIndex   = 0;
    while ((endIndex = stringToBeSplitted.find(delimeter, startIndex)) < stringToBeSplitted.size()) {
        std::string val = stringToBeSplitted.substr(startIndex, endIndex - startIndex);
        splittedString.push_back(val);
        startIndex = endIndex + delimeter.size();
    }

    if (startIndex < stringToBeSplitted.size()) {
        std::string val = stringToBeSplitted.substr(startIndex);
        splittedString.push_back(val);
    }

    return splittedString;
}

// [N|R|A]:address
// NickID (default) | RegID | Address
bool ParseRpcInputAccountId(const string &comboAccountIdStr, tuple<AccountIDType, string> &comboAccountId) {
    vector<string> comboAccountIdArr = split(comboAccountIdStr, ':');
    switch (comboAccountIdArr.size()) {
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
bool ParseRpcInputMoney(const string &comboMoneyStr, ComboMoney &comboMoney, const TokenSymbol defaultSymbol) {
	vector<string> comboMoneyArr = split(comboMoneyStr, ':');

    switch (comboMoneyArr.size()) {
        case 1: {
            if (!is_number(comboMoneyArr[0]))
                return false;

            int64_t iValue = std::atoll(comboMoneyArr[0].c_str());
            if (iValue < 0)
                return false;

            comboMoney.symbol = defaultSymbol;
            comboMoney.amount = (uint64_t)iValue;
            comboMoney.unit   = COIN_UNIT::SAWI;
            break;
        }
        case 2: {
            if (is_number(comboMoneyArr[0])) {
                int64_t iValue = std::atoll(comboMoneyArr[0].c_str());
                if (iValue < 0)
                    return false;

                string strUnit = comboMoneyArr[1];
                std::for_each(strUnit.begin(), strUnit.end(), [](char &c) { c = ::tolower(c); });
                if (!CoinUnitTypeTable.count(strUnit))
                    return false;

                comboMoney.symbol = defaultSymbol;
                comboMoney.amount = (uint64_t) iValue;
                comboMoney.unit   = strUnit;
            } else if (is_number(comboMoneyArr[1])) {
                if (comboMoneyArr[0].size() > MAX_TOKEN_SYMBOL_LEN) // check symbol len
                    return false;

                int64_t iValue = std::atoll(comboMoneyArr[1].c_str());
                if (iValue < 0)
                    return false;

                string strSymbol = comboMoneyArr[0];
                std::for_each(strSymbol.begin(), strSymbol.end(), [](char &c) { c = ::toupper(c); });

                comboMoney.symbol = strSymbol;
                comboMoney.amount = (uint64_t)iValue;
                comboMoney.unit   = COIN_UNIT::SAWI;

            } else {
                return false;
            }

            break;
        }
        case 3: {
            if (comboMoneyArr[0].size() > MAX_TOKEN_SYMBOL_LEN) // check symbol len
                return false;

            if (!is_number(comboMoneyArr[1]))
                return false;

            string strSymbol = comboMoneyArr[0];
            std::for_each(strSymbol.begin(), strSymbol.end(), [](char &c) { c = ::toupper(c); });

            int64_t iValue = std::atoll(comboMoneyArr[1].c_str());
            if (iValue < 0)
                return false;

            string strUnit = comboMoneyArr[2];
            std::for_each(strUnit.begin(), strUnit.end(), [](char &c) { c = ::tolower(c); });
            if (!CoinUnitTypeTable.count(strUnit))
                return false;

            comboMoney.symbol = strSymbol;
            comboMoney.amount = (uint64_t)iValue;
            comboMoney.unit   = strUnit;

            break;
        }
        default:
            return false;
    }

    return true;
}

Object SubmitTx(const CKeyID &keyid, CBaseTx &tx) {
    if (!pWalletMain->HaveKey(keyid)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Sender address not found in wallet");
    }

    if (!pWalletMain->Sign(keyid, tx.ComputeSignatureHash(), tx.signature)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
    }

    std::tuple<bool, string> ret = pWalletMain->CommitTx((CBaseTx *)&tx);
    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR,
                           strprintf("SubmitTx failed: txid=%s, %s", tx.GetHash().GetHex(), std::get<1>(ret)));
    }

    Object obj;
    obj.push_back(Pair("txid", std::get<1>(ret)));

    return obj;
}

string RegIDToAddress(CUserID &userId) {
    CKeyID keyId;
    if (pCdMan->pAccountCache->GetKeyId(userId, keyId))
        return keyId.ToAddress();

    return "cannot get address from given RegId";
}

bool GetKeyId(const string &addr, CKeyID &keyId) {
    if (!CRegID::GetKeyId(addr, keyId)) {
        keyId = CKeyID(addr);
        if (keyId.IsEmpty())
            return false;
    }

    return true;
}

Object GetTxDetailJSON(const uint256& txid) {
    Object obj;
    {
        std::shared_ptr<CBaseTx> pBaseTx;

        LOCK(cs_main);
        if (SysCfg().IsTxIndex()) {
            CDiskTxPos postx;
            if (pCdMan->pBlockCache->ReadTxIndex(txid, postx)) {
                CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
                CBlockHeader header;

                try {
                    file >> header;
                    fseek(file, postx.nTxOffset, SEEK_CUR);
                    file >> pBaseTx;
                    obj = pBaseTx->ToJson(*pCdMan->pAccountCache);

                    obj.push_back(Pair("confirmations",     chainActive.Height() - (int32_t)header.GetHeight()));
                    obj.push_back(Pair("confirmed_height",  (int32_t)header.GetHeight()));
                    obj.push_back(Pair("confirmed_time",    (int32_t)header.GetTime()));
                    obj.push_back(Pair("block_hash",        header.GetHash().GetHex()));

                    if (SysCfg().IsGenReceipt()) {
                        vector<CReceipt> receipts;
                        pCdMan->pReceiptCache->GetTxReceipts(txid, receipts);
                        obj.push_back(Pair("receipts", JSON::ToJson(*pCdMan->pAccountCache, receipts)));
                    }

                    CDataStream ds(SER_DISK, CLIENT_VERSION);
                    ds << pBaseTx;
                    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
                } catch (std::exception &e) {
                    throw runtime_error(tfm::format("%s : Deserialize or I/O error - %s", __func__, e.what()).c_str());
                }

                return obj;
            }
        }

        {
            pBaseTx = mempool.Lookup(txid);
            if (pBaseTx.get()) {
                obj = pBaseTx->ToJson(*pCdMan->pAccountCache);
                CDataStream ds(SER_DISK, CLIENT_VERSION);
                ds << pBaseTx;
                obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
                return obj;
            }
        }

        /* try */
        CBlock genesisblock;
        CBlockIndex* pGenesisBlockIndex = mapBlockIndex[SysCfg().GetGenesisBlockHash()];
        ReadBlockFromDisk(pGenesisBlockIndex, genesisblock);
        assert(genesisblock.GetMerkleRootHash() == genesisblock.BuildMerkleTree());
        for (uint32_t i = 0; i < genesisblock.vptx.size(); ++i) {
            if (txid == genesisblock.GetTxid(i)) {
                obj = genesisblock.vptx[i]->ToJson(*pCdMan->pAccountCache);

                obj.push_back(Pair("confirmations",     chainActive.Height()));
                obj.push_back(Pair("confirmed_height",  chainActive.Height()));
                obj.push_back(Pair("confirmed_time",    (int32_t)genesisblock.GetTime()));
                obj.push_back(Pair("block_hash",        genesisblock.GetHash().GetHex()));

                CDataStream ds(SER_DISK, CLIENT_VERSION);
                ds << genesisblock.vptx[i];
                obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

                return obj;
            }
        }
    }

    return obj;
}

///////////////////////////////////////////////////////////////////////////////
// namespace JSON

const Value& JSON::GetObjectFieldValue(const Value &jsonObj, const string &fieldName) {
    const Value& jsonValue = find_value(jsonObj.get_obj(), fieldName);
    if (jsonValue.type() == null_type || jsonValue == null_type) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("field %s not found in json object", fieldName));
    }

    return jsonValue;
}

const char* JSON::GetValueTypeName(const Value_type &valueType) {
    if (valueType >= 0 && valueType <= json_spirit::Value_type::null_type) {
        return json_spirit::Value_type_name[valueType];
    }
    return "unknown";
}


Object JSON::ToJson(const CAccountDBCache &accountCache, const CReceipt &receipt) {
    CKeyID fromKeyId, toKeyId;
    accountCache.GetKeyId(receipt.from_uid, fromKeyId);
    accountCache.GetKeyId(receipt.to_uid, toKeyId);

    Object obj;
    obj.push_back(Pair("from_addr",     fromKeyId.ToAddress()));
    obj.push_back(Pair("to_addr",       toKeyId.ToAddress()));
    obj.push_back(Pair("coin_symbol",   receipt.coin_symbol));
    obj.push_back(Pair("coin_amount",   receipt.coin_amount));
    obj.push_back(Pair("receipt_code",  (uint64_t)receipt.code));
    obj.push_back(Pair("memo",          GetReceiptCodeName(receipt.code)));
    return obj;
}

Array JSON::ToJson(const CAccountDBCache &accountCache, const vector<CReceipt> &receipts) {
    Array array;
    for (const auto &receipt : receipts) {
        array.push_back(ToJson(accountCache, receipt));
    }
    return array;
}


///////////////////////////////////////////////////////////////////////////////
// namespace RPC_PARAM

ComboMoney RPC_PARAM::GetComboMoney(const Value &jsonValue,
                                    const TokenSymbol &defaultSymbol) {
    ComboMoney money;
    Value_type valueType = jsonValue.type();
    if (valueType == json_spirit::Value_type::int_type ) {
        money.symbol = defaultSymbol;
        money.amount = AmountToRawValue(jsonValue.get_int64());
        money.unit   = COIN_UNIT::SAWI;

    } else if (valueType == json_spirit::Value_type::str_type) {
        if (!ParseRpcInputMoney(jsonValue.get_str(), money, defaultSymbol)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid combo money format");
        }
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid json value type: %s", JSON::GetValueTypeName(valueType)));
    }

    return money;
}

ComboMoney RPC_PARAM::GetFee(const Array& params, const size_t index, const TxType txType) {
    ComboMoney fee;
    if (params.size() > index) {
        fee = GetComboMoney(params[index], SYMB::WICC);
        if (!kFeeSymbolSet.count(fee.symbol))
            throw JSONRPCError(RPC_INVALID_PARAMS,
                strprintf("Fee symbol is %s, but expect %s", fee.symbol, GetFeeSymbolSetStr()));

        uint64_t minFee;
        if (!GetTxMinFee(txType, chainActive.Height(), fee.symbol, minFee))
            throw JSONRPCError(RPC_INVALID_PARAMS,
                strprintf("Can not find the min tx fee! symbol=%s", fee.symbol));
        if (fee.GetSawiAmount() < minFee)
            throw JSONRPCError(RPC_INVALID_PARAMS,
                strprintf("The given fee is too small: %llu < %llu sawi", fee.amount, minFee));
    } else {
        uint64_t minFee;
        if (!GetTxMinFee(txType, chainActive.Height(), SYMB::WICC, minFee))
            throw JSONRPCError(RPC_INVALID_PARAMS,
                strprintf("Can not find the min tx fee! symbol=%s", SYMB::WICC));
        fee.symbol = SYMB::WICC;
        fee.amount = minFee;
        fee.unit = COIN_UNIT::SAWI;
    }

    return fee;
}

uint64_t RPC_PARAM::GetWiccFee(const Array& params, const size_t index, const TxType txType) {
    uint64_t fee, minFee;
    if (!GetTxMinFee(txType, chainActive.Height(), SYMB::WICC, minFee))
        throw JSONRPCError(RPC_INVALID_PARAMS,
            strprintf("Can not find the min tx fee! symbol=%s", SYMB::WICC));
    if (params.size() > index) {
        fee = AmountToRawValue(params[index]);
        if (fee < minFee)
            throw JSONRPCError(RPC_INVALID_PARAMS,
                strprintf("The given fee is too small: %llu < %llu sawi", fee, minFee));
    } else {
        fee = minFee;
    }

    return fee;
}

CUserID RPC_PARAM::GetUserId(const Value &jsonValue, const bool senderUid) {
    auto pUserId = CUserID::ParseUserId(jsonValue.get_str());
    if (!pUserId) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    /**
     * Attention: feature enable in stable coin release!
     *
     * We need to choose the proper field as the sender/receiver's account according to
     * the two factor: whether the sender's account is registered or not, whether the
     * RegID is mature or not.
     *
     * |-------------------------------|-------------------|-------------------|
     * |                               |      SENDER       |      RECEIVER     |
     * |-------------------------------|-------------------|-------------------|
     * | NOT registered                |     Public Key    |      Key ID       |
     * |-------------------------------|-------------------|-------------------|
     * | registered BUT immature       |     Public Key    |      Key ID       |
     * |-------------------------------|-------------------|-------------------|
     * | registered AND mature         |     Reg ID        |      Reg ID       |
     * |-------------------------------|-------------------|-------------------|
     */
    CRegID regid;
    if (GetFeatureForkVersion(chainActive.Height()) == MAJOR_VER_R1) {
        if (pCdMan->pAccountCache->GetRegId(*pUserId, regid)) {
            return CUserID(regid);
        } else {
            return *pUserId;
        }
    } else { // MAJOR_VER_R2
        if (pCdMan->pAccountCache->GetRegId(*pUserId, regid) && regid.IsMature(chainActive.Height())) {
            return CUserID(regid);
        } else {
            if (senderUid && pUserId->is<CKeyID>()) {
                CPubKey sendPubKey;
                if (!pWalletMain->GetPubKey(pUserId->get<CKeyID>(), sendPubKey) || !sendPubKey.IsFullyValid())
                    throw JSONRPCError(RPC_WALLET_ERROR, "Key not found in the local wallet");

                return CUserID(sendPubKey);
            } else {
                return *pUserId;
            }
        }
    }
}

string RPC_PARAM::GetLuaContractScript(const Value &jsonValue) {
    string filePath = GetAbsolutePath(jsonValue.get_str()).string();
    if (filePath.empty())
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_NOT_EXIST, "Lua Script file not exist");

    if (filePath.compare(0, LUA_CONTRACT_LOCATION_PREFIX.size(), LUA_CONTRACT_LOCATION_PREFIX.c_str()) != 0)
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_INVALID, "Lua Script file not inside /tmp/lua dir or its subdir");

    std::tuple<bool, string> result = CLuaVM::CheckScriptSyntax(filePath.c_str());
    if (!std::get<0>(result))
        throw JSONRPCError(RPC_INVALID_PARAMETER, std::get<1>(result));

    bool success = false;
    string contractScript;
    do {
        streampos begin, end;
        ifstream fin(filePath.c_str(), ios::binary | ios::in);
        if (!fin)
            break;

        begin = fin.tellg();
        fin.seekg(0, ios::end);
        end = fin.tellg();

        streampos length = end - begin;
        if (length == 0 || length > MAX_CONTRACT_CODE_SIZE) {
            fin.close();
            break;
        }

        fin.seekg(0, ios::beg);
        char *buffer = new char[length];
        fin.read(buffer, length);

        contractScript.assign(buffer, length);
        free(buffer);
        fin.close();

        success = true;
    } while (false);

    if (!success)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to acquire contract script");

    return contractScript;
}

uint64_t RPC_PARAM::GetPrice(const Value &jsonValue) {
    // TODO: check price range??
    return AmountToRawValue(jsonValue);
}

uint256 RPC_PARAM::GetTxid(const Value &jsonValue, const string &paramName, const bool canBeEmpty) {
    string binStr, errStr;
    if (!ParseHex(jsonValue.get_str(), binStr, errStr))
        throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("Get param %s error! %s", paramName, errStr));

    if (binStr.empty()) {
        if (!canBeEmpty)
            throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("Get param %s error! Can not be emtpy", paramName));
        return uint256();
    }

    if (binStr.size() != uint256::WIDTH) {
        throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("Get param %s error! The hex str size should be %d",
            paramName, uint256::WIDTH * 2));
    }
    return uint256(binStr.rbegin(), binStr.rend());
}

CAccount RPC_PARAM::GetUserAccount(CAccountDBCache &accountCache, const CUserID &userId) {
    CAccount account;
    if (!accountCache.GetAccount(userId, account))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                           strprintf("The account not exists! userId=%s", userId.ToString()));

    assert(!account.keyid.IsEmpty());
    return account;
}

TokenSymbol RPC_PARAM::GetOrderCoinSymbol(const Value &jsonValue) {
    return jsonValue.get_str();
}

TokenSymbol RPC_PARAM::GetOrderAssetSymbol(const Value &jsonValue) {
    return jsonValue.get_str();
}

TokenSymbol RPC_PARAM::GetAssetIssueSymbol(const Value &jsonValue) {
    TokenSymbol symbol = jsonValue.get_str();
    if (symbol.empty() || symbol.size() > MAX_TOKEN_SYMBOL_LEN)
        throw JSONRPCError(RPC_INVALID_PARAMS,
                           "asset_symbol=%s must be composed of 6 or 7 capital letters [A-Z]");
    return symbol;
}

TokenName RPC_PARAM::GetAssetName(const Value &jsonValue) {
    TokenName name = jsonValue.get_str();
    if (name.empty() || name.size() > MAX_ASSET_NAME_LEN)
        throw JSONRPCError(RPC_INVALID_PARAMS,
                           strprintf("asset name is empty or len=%d greater than %d", name.size(),
                                     MAX_ASSET_NAME_LEN));
    return name;
}

string RPC_PARAM::GetBinStrFromHex(const Value &jsonValue, const string &paramName) {
    string binStr, errStr;
    if (!ParseHex(jsonValue.get_str(), binStr, errStr))
        throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("Get param %s error! %s", paramName, errStr));
    return binStr;
}

void RPC_PARAM::CheckAccountBalance(CAccount &account, const TokenSymbol &tokenSymbol, const BalanceOpType opType,
                                    const uint64_t value) {
    if (pCdMan->pAssetCache->CheckTransferCoinSymbol(tokenSymbol))
        throw JSONRPCError(RPC_WALLET_ERROR, strprintf("Unsupported coin symbol: %s", tokenSymbol));


    if (!account.OperateBalance(tokenSymbol, opType, value))
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, strprintf("Account does not have enough %s", tokenSymbol));
}

void RPC_PARAM::CheckActiveOrderExisted(CDexDBCache &dexCache, const uint256 &orderTxid) {
    if (!dexCache.HaveActiveOrder(orderTxid))
        throw JSONRPCError(RPC_DEX_ORDER_INACTIVE, "Order is inactive or not existed");
}

void RPC_PARAM::CheckOrderSymbols(const string &title, const TokenSymbol &coinSymbol,
                                  const TokenSymbol &assetSymbol) {
    if (coinSymbol.empty() || coinSymbol.size() > MAX_TOKEN_SYMBOL_LEN || kCoinTypeSet.count(coinSymbol) == 0) {
        throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("%s invalid order coin symbol=%s", title, coinSymbol));
    }

    if (assetSymbol.empty() || assetSymbol.size() > MAX_TOKEN_SYMBOL_LEN || kCoinTypeSet.count(assetSymbol) == 0) {
        throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("%s invalid order asset symbol=%s", title, assetSymbol));
    }

    if (kTradingPairSet.count(make_pair(assetSymbol, coinSymbol)) == 0) {
        throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("%s unsupport trading pair! coin_symbol=%s, asset_symbol=%s",
            title, coinSymbol, assetSymbol));
    }
}

bool RPC_PARAM::ParseHex(const string &hexStr, string &binStrOut, string &errStrOut) {
    int32_t begin = 0;
    int32_t end   = hexStr.size();
    // skip left spaces
    while (begin != end && isspace(hexStr[begin]))
        begin++;
    // skip right spaces
    while (begin != end && isspace(hexStr[end - 1]))
        end--;

    // skip 0x
    if (begin + 1 < end && hexStr[begin] == '0' && tolower(hexStr[begin + 1]) == 'x')
        begin += 2;

    if (begin == end) return true; // ignore empty hex str

    if ((end - begin) % 2 != 0) {
        errStrOut = "Invalid hex format! Hex digit count is not even number";
        return false;
    }

    binStrOut.reserve((end - begin) / 2);
    while (begin != end) {
        uint8_t c1 = HexDigit(hexStr[begin]);
        uint8_t c2 = HexDigit(hexStr[begin + 1]);
        if (c1 == (uint8_t)-1 || c2 == (uint8_t)-1) {
            errStrOut = strprintf("Invalid hex char in the position %d or %d", begin, begin + 1);
            return false;
        }
        binStrOut.push_back((c1 << 4) | c2);
        begin += 2;
    }
    return true;
}
