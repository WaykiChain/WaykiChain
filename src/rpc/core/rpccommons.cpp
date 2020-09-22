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

#include "datastream.hpp"
#include "abi_serializer.hpp"
#include "wasm_context.hpp"
#include "wasm_variant_trace.hpp"
#include "wasm/exception/exceptions.hpp"

#include <regex>
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


bool is_decimal(const string& s ){
    std::regex r("([1-9][0-9]*|0)\\.[0-9]+");
    return regex_match(s, r);
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
bool pow10(string comboAmountStr, const unsigned int precision, int64_t& sawiAmount){

    auto v = split(comboAmountStr, ".");
    if(v.size() == 1)
        v.push_back("");
    string& intPart = v[0];
    string& decimalPart = v[1];

    if(decimalPart.size() > precision )
        return false;
    unsigned int i = 0;
    for(;i<precision;i++){
        if(decimalPart.size() > i)
            intPart.push_back(decimalPart[i]);
        else
            intPart.push_back('0');
    }
    sawiAmount =  atoll(intPart.data());
    return true;
}

bool parseAmountAndUnit( vector<string>& comboMoneyArr, ComboMoney& comboMoney,const TokenSymbol &defaultSymbol = SYMB::WICC){

    int64_t iValue = 0;

    string strUnit = comboMoneyArr[1];
    std::for_each(strUnit.begin(), strUnit.end(), [](char &c) { c = ::tolower(c); });
    if (!CoinUnitTypeMap.count(strUnit))
        return false;

    if(!pow10(comboMoneyArr[0].c_str(), CoinUnitPrecisionTable.find(strUnit)->second, iValue))
        return false;

    if (iValue < 0)
        return false;

    comboMoney.symbol = defaultSymbol;
    comboMoney.amount = (uint64_t)iValue;
    comboMoney.unit   = COIN_UNIT::SAWI;

    return true;
}

bool parseSymbolAndAmount(vector<string>& comboMoneyArr, ComboMoney& comboMoney){

    if (comboMoneyArr[0].size() > MAX_TOKEN_SYMBOL_LEN) // check symbol len
        return false;

    int64_t iValue = std::atoll(comboMoneyArr[1].c_str());
    if (iValue < 0)
        return false;

    string strSymbol = comboMoneyArr[0];
   // std::for_each(strSymbol.begin(), strSymbol.end(), [](char &c) { c = ::toupper(c); });

    comboMoney.symbol = strSymbol;
    comboMoney.amount = (uint64_t)iValue;
    comboMoney.unit   = COIN_UNIT::SAWI;

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
            if (is_number(comboMoneyArr[0]) || is_decimal(comboMoneyArr[0])) {

                return parseAmountAndUnit(comboMoneyArr, comboMoney);

            } else if (is_number(comboMoneyArr[1])) {
                return parseSymbolAndAmount(comboMoneyArr,comboMoney);

            } else {
                return false;
            }

            break;
        }
        case 3: {
            if (comboMoneyArr[0].size() > MAX_TOKEN_SYMBOL_LEN) // check symbol len
                return false;

            if (!is_number(comboMoneyArr[1]) && !is_decimal(comboMoneyArr[1]))
                return false;

            string strSymbol = comboMoneyArr[0];
          //  std::for_each(strSymbol.begin(), strSymbol.end(), [](char &c) { c = ::toupper(c); });
            vector<string> amountAndUnit;
            amountAndUnit.push_back(comboMoneyArr[1]);
            amountAndUnit.push_back(comboMoneyArr[2]);
            return parseAmountAndUnit(amountAndUnit,comboMoney,strSymbol);

            break;
        }
        default:
            return false;
    }

    return true;
}


Object SubmitTx(const CKeyID &keyid, CBaseTx &tx) {
    if (!pWalletMain->HasKey(keyid)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Sender address not found in wallet");
    }

    if (!pWalletMain->Sign(keyid, tx.GetHash(), tx.signature)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
    }

    CValidationState state;
    if (!pWalletMain->CommitTx((CBaseTx *)&tx, state))
        throw JSONRPCError(RPC_WALLET_ERROR,
                           strprintf("CommitTx failed: code=%d, reason=%s", state.GetRejectCode(),
                                     state.GetRejectReason()));

    Object obj;
    obj.push_back(Pair("txid", tx.GetHash().GetHex()));

    return obj;
}

string RegIDToAddress(CUserID &userId) {
    CKeyID keyId;
    if (pCdMan->pAccountCache->GetKeyId(userId, keyId))
        return keyId.ToAddress();

    return "cannot get address from given RegId";
}

Object GetTxDetailJSON(CCacheWrapper &cw, const CBlockHeader &header,
                       const shared_ptr<CBaseTx> pBaseTx, const CTxCord &txCord) {
    Object obj;
    auto txid = pBaseTx->GetHash();
    //obj = pBaseTx->IsMultiSignSupport()?pBaseTx->ToJsonMultiSign(*database):pBaseTx->ToJson(*pCdMan->pAccountCache);
    obj = pBaseTx->ToJson(cw);


    obj.push_back(Pair("confirmed_height",  (int32_t)header.GetHeight()));
    obj.push_back(Pair("confirmed_time",    (int32_t)header.GetTime()));
    obj.push_back(Pair("block_hash",        header.GetHash().GetHex()));
    obj.push_back(Pair("tx_cord", txCord.ToString()));

    if (SysCfg().IsGenReceipt()) {
        vector<CReceipt> receipts;
        pCdMan->pReceiptCache->GetTxReceipts(txid, receipts);
        obj.push_back(Pair("receipts", JSON::ToJson(cw.accountCache, receipts)));
    }

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << pBaseTx;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
    obj.push_back(Pair("confirmations",     chainActive.Height() - (int32_t)header.GetHeight()));

    string trace;
    auto database = std::make_shared<CCacheWrapper>(pCdMan);
    auto resolver = make_resolver(*database);
    if(database->contractCache.GetContractTraces(txid, trace)){

        json_spirit::Value value_json;
        std::vector<char>  trace_bytes = std::vector<char>(trace.begin(), trace.end());
        transaction_trace  trace       = wasm::unpack<transaction_trace>(trace_bytes);
        to_variant(trace, value_json, resolver);
        obj.push_back(Pair("tx_trace", value_json));
    }

    return obj;
}

Object GetTxDetailJSON(const uint256& txid) {
    auto pCw = make_shared<CCacheWrapper>(pCdMan);
    Object obj;
    {
        std::shared_ptr<CBaseTx> pBaseTx;

        LOCK(cs_main);
        if (SysCfg().IsTxIndex()) {
            CDiskTxPos postx;
            if (pCw->blockCache.ReadTxIndex(txid, postx)) {
                CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
                CBlockHeader header;

                try {
                    file >> header;
                    fseek(file, postx.nTxOffset, SEEK_CUR);
                    file >> pBaseTx;
                    obj = GetTxDetailJSON(*pCw, header, pBaseTx, postx.tx_cord);
                } catch (std::exception &e) {
                    throw runtime_error(strprintf("%s : Deserialize or I/O error - %s", __func__, e.what()).c_str());
                }

                return obj;
            }
        }

        {
            pBaseTx = mempool.Lookup(txid);
            if (pBaseTx.get()) {
                obj = pBaseTx->ToJson(*pCw);
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
                obj = genesisblock.vptx[i]->ToJson(*pCw);


                obj.push_back(Pair("confirmed_height",  chainActive.Height()));
                obj.push_back(Pair("confirmed_time",    (int32_t)genesisblock.GetTime()));
                obj.push_back(Pair("block_hash",        genesisblock.GetHash().GetHex()));
                obj.push_back(Pair("tx_cord", CTxCord(0, i).ToString()));

                CDataStream ds(SER_DISK, CLIENT_VERSION);
                ds << genesisblock.vptx[i];
                if (SysCfg().IsGenReceipt()) {
                    vector<CReceipt> receipts;
                    pCdMan->pReceiptCache->GetTxReceipts(txid, receipts);
                    obj.push_back(Pair("receipts", JSON::ToJson(pCw->accountCache, receipts)));
                }
                obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
                obj.push_back(Pair("confirmations",     chainActive.Height()));

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
    if (jsonValue.type() == null_type) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("field %s not found in json object", fieldName));
    }

    return jsonValue;
}


bool JSON::GetObjectFieldValue(const Value &jsonObj, const string &fieldName,Value& returnValue) {
    const Value& jsonValue = find_value(jsonObj.get_obj(), fieldName);
    if (jsonValue.type() == null_type) {

        return false;
    }
    returnValue = jsonValue;
    return true;

}

const char* JSON::GetValueTypeName(const Value_type &valueType) {
    if (valueType >= 0 && valueType <= json_spirit::Value_type::null_type) {
        return json_spirit::Value_type_name[valueType];
    }
    return "unknown";
}


Object JSON::ToJson(const CAccountDBCache &accountCache, const CReceipt &receipt) {
    CKeyID fromKeyId, toKeyId;
    if (!receipt.from_uid.IsEmpty())
        accountCache.GetKeyId(receipt.from_uid, fromKeyId);
    if (!receipt.to_uid.IsEmpty())
        accountCache.GetKeyId(receipt.to_uid, toKeyId);

    Object obj;
    obj.push_back(Pair("receipt_type",  (uint64_t)receipt.receipt_type));
    obj.push_back(Pair("receipt_type_name", GetReceiptTypeName(receipt.receipt_type)));
    obj.push_back(Pair("op_type",       (uint64_t) receipt.op_type));
    obj.push_back(Pair("op_type_name",  GetBalanceOpTypeName(receipt.op_type)));
    obj.push_back(Pair("from_addr",     fromKeyId.ToAddress()));
    obj.push_back(Pair("to_addr",       toKeyId.ToAddress()));
    obj.push_back(Pair("coin_symbol",   receipt.coin_symbol));
    obj.push_back(Pair("coin_amount",   JsonValueFromAmount(receipt.coin_amount)));

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

uint32_t RPC_PARAM::GetUint32(const Value &jsonValue) {
    int64_t ret = jsonValue.get_int64();
    if (ret < 0 || ret > uint32_t(-1)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("value=%lld out of range", ret));
    }
    return uint32_t(ret);
}

uint64_t RPC_PARAM::GetUint64(const Value &jsonValue) {
    int64_t ret = jsonValue.get_int64();
    if (ret < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("value=%lld out of range", ret));
    }
    return uint64_t(ret);
}


void RPC_PARAM::CheckTokenAmount(const TokenSymbol &symbol, const uint64_t amount) {
    if (!CheckCoinRange(symbol, amount))
        throw JSONRPCError(RPC_INVALID_PARAMETER,
            strprintf("token amount is too small, symbol=%s, amount=%llu, min_amount=%llu",
                    symbol, amount, MIN_DEX_ORDER_AMOUNT));
}

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
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid combo money format=%s", jsonValue.get_str()));
        }
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid json value type: %s", JSON::GetValueTypeName(valueType)));
    }

    return money;
}

uint64_t RPC_PARAM::GetTxMinFeeBy(const TxType txType, const TokenSymbol &symbol) {
    uint64_t minFee = 0;
    auto spCw = std::make_shared<CCacheWrapper>(pCdMan);
    if (!::GetTxMinFee(*spCw, txType, chainActive.Height(), symbol, minFee))
        throw JSONRPCError(RPC_INVALID_PARAMS,
            strprintf("Can not find the min tx fee! symbol=%s", symbol));
    return minFee;
}

// @return pair(ComboMoney txFee, uint64_t minTxFeeAmount)
pair<ComboMoney, uint64_t> RPC_PARAM::ParseTxFee(const Array &params, const size_t index,
                                                 const TxType txType) {

    ComboMoney txFee;
    uint64_t minTxFeeAmount;
    if (params.size() > index) {
        txFee = GetComboMoney(params[index], SYMB::WICC);
        if (!kFeeSymbolSet.count(txFee.symbol))
            throw JSONRPCError(RPC_INVALID_PARAMS,
                strprintf("Fee symbol is %s, but expect %s", txFee.symbol, GetFeeSymbolSetStr()));

        minTxFeeAmount = GetTxMinFeeBy(txType, txFee.symbol);
    } else {
        minTxFeeAmount = GetTxMinFeeBy(txType, SYMB::WICC);
        txFee = {SYMB::WICC, minTxFeeAmount, COIN_UNIT::SAWI};
    }
    return {txFee, minTxFeeAmount};
}

ComboMoney RPC_PARAM::GetFee(const Array& params, const size_t index, const TxType txType) {
    const auto [txFee, minTxFeeAmount] = ParseTxFee(params, index, txType);
    if (txFee.GetAmountInSawi() < minTxFeeAmount)
        throw JSONRPCError(RPC_INVALID_PARAMS,
            strprintf("The given fee is too small: %llu < %llu sawi", txFee.amount, minTxFeeAmount));
    return txFee;
}

uint64_t RPC_PARAM::GetWiccFee(const Array& params, const size_t index, const TxType txType) {
    uint64_t fee, minFee;
    auto spCw = std::make_shared<CCacheWrapper>(pCdMan);
    if (!GetTxMinFee(*spCw, txType, chainActive.Height(), SYMB::WICC, minFee))
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

CUserID RPC_PARAM::ParseUserIdByAddr(const Value &jsonValue) {
    auto pUserId = CUserID::ParseUserId(jsonValue.get_str());
    if (!pUserId) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("Invalid address=%s", jsonValue.get_str()));
    }
    return *pUserId;
}

CRegID RPC_PARAM::ParseRegId(const Value &jsonValue, const string &title) {

    const auto &idStr = jsonValue.get_str();
    if (!CRegID::IsSimpleRegIdStr(idStr))
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("invalid regid fomat of %s=%s", title, idStr));
    return CRegID(idStr);
}

CRegID RPC_PARAM::ParseRegId(const Array& params, const size_t index, const string &title, const CRegID &defaultValue) {
    if (params.size() > index && !params[index].get_str().empty()) {
        return ParseRegId(params[index], title);
    }
    return defaultValue;
}

CUserID RPC_PARAM::GetUserId(const Value &jsonValue, const bool bSenderUid ) {
    auto userId = ParseUserIdByAddr(jsonValue);

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
    if (GetFeatureForkVersion(chainActive.Height()) >= MAJOR_VER_R2) {
        if (pCdMan->pAccountCache->GetRegId(userId, regid) && regid.IsMature(chainActive.Height()))
            return CUserID(regid);

        if (bSenderUid && userId.is<CKeyID>()) {
            CPubKey sendPubKey;
            if (!pWalletMain->GetPubKey(userId.get<CKeyID>(), sendPubKey) || !sendPubKey.IsFullyValid())
                throw JSONRPCError(RPC_WALLET_ERROR, "Key not found in the local wallet");

            return CUserID(sendPubKey);
        }

        return userId;

    } else { // MAJOR_VER_R1
        if (pCdMan->pAccountCache->GetRegId(userId, regid)) {
            return CUserID(regid);
        } else {
            return userId;
        }
    }
}

CKeyID RPC_PARAM::GetUserKeyId(const CUserID &uid) {
    CKeyID keyid;
    if (!pCdMan->pAccountCache->GetKeyId(uid, keyid))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                           strprintf("Get account keyid by (%s) failed", uid.ToString()));
    return keyid;
}

CKeyID RPC_PARAM::GetKeyId(const Value &jsonValue){
    return GetUserKeyId(ParseUserIdByAddr(jsonValue));
}

CUserID RPC_PARAM::GetRegId(const Value &jsonValue) {

    auto uid = ParseUserIdByAddr(jsonValue);
    CRegID regid;
    if (!pCdMan->pAccountCache->GetRegId(uid, regid))
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("get regid by uid=%s failed", uid.ToString()));

    return regid;
}

CRegID RPC_PARAM::GetRegId(const Array& params, const size_t index, const CRegID &defaultRegId) {
   if (params.size() <= index)
        return defaultRegId;

    CRegID regid;
    auto uid = ParseUserIdByAddr(params[index]);

    if (!pCdMan->pAccountCache->GetRegId(uid, regid))
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("get regid by uid=%s failed", uid.ToString()));

    return regid;
}

string RPC_PARAM::GetLuaContractScript(const Value &jsonValue) {
    string filePath = GetAbsolutePath(jsonValue.get_str()).string();
    if (filePath.empty())
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_NOT_EXIST, "Lua Script file not exist");

    if (filePath.compare(0, LUA_CONTRACT_LOCATION_PREFIX.size(), LUA_CONTRACT_LOCATION_PREFIX.c_str()) != 0)
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_INVALID, "Lua Script file not inside /tmp/lua dir or its subdir");

    std::tuple<bool, string> result = CLuaVM::CheckScriptSyntax(filePath.c_str(), chainActive.Height());
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
                           strprintf("The account not exist (never received coins before)! userId=%s", userId.ToString()));

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
    return jsonValue.get_str();
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
    if (!pCdMan->pAssetCache->CheckAsset(tokenSymbol))
        throw JSONRPCError(RPC_WALLET_ERROR, strprintf("Unsupported coin symbol: %s", tokenSymbol));

    ReceiptList receipts;
    if (!account.OperateBalance(tokenSymbol, opType, value, ReceiptType::NULL_RECEIPT_TYPE, receipts))
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, strprintf("Account does not have enough %s", tokenSymbol));
}

void RPC_PARAM::CheckActiveOrderExisted(CDexDBCache &dexCache, const uint256 &orderTxid) {
    if (!dexCache.HaveActiveOrder(orderTxid))
        throw JSONRPCError(RPC_DEX_ORDER_INACTIVE, "Order is inactive or not existed");
}

void RPC_PARAM::CheckOrderSymbols(const string &title, const TokenSymbol &coinSymbol,
                                  const TokenSymbol &assetSymbol) {
    if (coinSymbol == assetSymbol)
        throw JSONRPCError(REJECT_INVALID,
                           strprintf("%s, coin_symbol=%s is same to asset_symbol=%s", title,
                                     coinSymbol, assetSymbol));

    if (!pCdMan->pAssetCache->CheckDexQuoteSymbol(coinSymbol))
        throw JSONRPCError(REJECT_INVALID, strprintf("%s, unsupport coin_symbol=%s", title, coinSymbol));

    if (!pCdMan->pAssetCache->CheckDexBaseSymbol(assetSymbol))
        throw JSONRPCError(REJECT_INVALID, strprintf("%s, unsupport asset_symbol=%s", title, assetSymbol));
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

TxType RPC_PARAM::ParseTxType(const Value &jsonValue) {
    TxType ret = ::ParseTxType(jsonValue.get_str());
    if (ret == TxType::NULL_TX)
        throw JSONRPCError(REJECT_INVALID, strprintf("unsupport tx_type=%s", jsonValue.get_str()));
    return ret;
}

Value RPC_PARAM::GetWasmContractArgs(const Value &jsonValue) {
    auto valueType = jsonValue.type();

    if (valueType == json_spirit::obj_type || valueType == json_spirit::array_type) {
        return jsonValue;
    } else if (valueType == json_spirit::str_type) {
        json_spirit::Value newValue;
        try {
            json_spirit::read_string_or_throw(jsonValue.get_str(), newValue);
        } catch (json_spirit::Error_position &e) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Parse json of contract data error! line[%d:%d] %s",
                    e.line_, e.column_, e.reason_));
        }
        auto newValueType = newValue.type();
        if (newValueType == json_spirit::obj_type || newValueType == json_spirit::array_type) {
            return newValue;
        }
    }
    throw JSONRPCError(RPC_INVALID_PARAMETER, "The contract action args type must be object or array,"
                                              " or string of object or array");
}

uint64_t RPC_PARAM::GetPriceByCdp(CPriceFeedCache &priceFeedCache, CUserCDP &cdp) {
    auto quoteSymbol = GetQuoteSymbolByCdpScoin(cdp.scoin_symbol);
    if (quoteSymbol.empty())  {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           strprintf("scoin symbol=%s does not have corresponding quote symbol!",
                                     cdp.scoin_symbol));
    }

    return priceFeedCache.GetMedianPrice(PriceCoinPair(cdp.bcoin_symbol, quoteSymbol));
}


CUniversalContractStore RPC_PARAM::GetWasmContract(CContractDBCache &contractCache, const CRegID &regid) {
    CUniversalContractStore contract_store;
    CHAIN_ASSERT( contractCache.GetContract(regid, contract_store),
                  wasm_chain::contract_exception,
                  "cannot get contract '%s'",
                  regid.ToString())

    CHAIN_ASSERT( contract_store.vm_type == VMType::WASM_VM,
                  wasm_chain::vm_type_mismatch, "vm type must be WASM_VM")

    CHAIN_ASSERT( contract_store.abi.size() > 0,
                  wasm_chain::abi_not_found_exception, "contract abi is empty")
    return contract_store;
}

CBlock RPC_PARAM::ReadBlock(CBlockIndex* pBlockIndex) {
    CBlock block;
    if (!ReadBlockFromDisk(pBlockIndex, block)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Can't read block from disk");
    }
    return block;
}


Value RPC_PARAM::GetGenunsignedArgs(const Value &jsonValue) {
    auto newValue = jsonValue;
    if (newValue.type() == json_spirit::str_type){
        json_spirit::read_string(jsonValue.get_str(), newValue);
    }

    if (newValue.type() == json_spirit::obj_type){
        return newValue;
    }

    throw JSONRPCError(RPC_INVALID_PARAMETER, "The genunsignedtxraw args type must be object,"
                                              " or string of object");
}