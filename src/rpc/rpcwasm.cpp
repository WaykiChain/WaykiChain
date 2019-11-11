// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2017-2018 WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "commons/base58.h"
#include "rpc/core/rpcserver.h"
#include "rpc/core/rpccommons.h"
#include "init.h"
#include "net.h"
#include "netbase.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#include "persistence/blockdb.h"
#include "persistence/txdb.h"
#include "config/configuration.h"
#include "miner/miner.h"
#include "main.h"
//#include "vm/vmrunenv.h"
#include <stdint.h>
#include <chrono>

#include "entities/contract.h"

#include <boost/assign/list_of.hpp>
#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"
#include "commons/json/json_spirit_reader.h"
#include "commons/json/json_spirit_writer.h"

#include "datastream.hpp"
#include "abi_serializer.hpp"
#include "wasm_context.hpp"
#include "exceptions.hpp"
#include "types/name.hpp"
#include "types/asset.hpp"
#include "wasm_config.hpp"
#include "wasm_native_contract_abi.hpp"
#include "wasm_rpc_message.hpp"

using namespace std;
using namespace boost;
using namespace json_spirit;
using namespace boost::assign;
using std::chrono::microseconds;
// using namespace wasm;

#define JSON_RPC_ASSERT(expr , code, msg) \
    if( !( expr ) ){                     \
        throw JSONRPCError(code, msg);   \
    }

#define RESPONSE_RPC_HELP(expr , msg) \
    if( ( expr ) ){                     \
        throw runtime_error( msg);   \
    }

bool read_file_limit(const string& path, string& data, uint64_t max_size){
    try {
        if(path.empty()) return false;

        char byte;
        ifstream f(path, ios::binary);
        while (f.get(byte)) data.push_back(byte);
        size_t size = data.size();
        if (size == 0 || size > max_size){
            return false;
        }
        return true;

    } catch (...) {
        return false;
    } 
}

void read_and_validate_code(const string& path, string& code){
    try {
        WASM_ASSERT(read_file_limit(path, code, MAX_CONTRACT_CODE_SIZE), file_read_exception, "wasm code file is empty or larger than max limited %d bytes", MAX_CONTRACT_CODE_SIZE)
        vector <uint8_t> code_v;
        code_v.insert(code_v.begin(), code.begin(), code.end());
        wasm_interface wasmif;
        wasmif.validate(code_v);
    } catch (wasm::exception &e) {
        JSON_RPC_ASSERT(false, e.code(), e.detail())
    }    
}

void read_and_validate_abi(const string& abi_file, string& abi){
    try {
        WASM_ASSERT(read_file_limit(abi_file, abi, MAX_CONTRACT_CODE_SIZE), file_read_exception, "wasm abi file is empty or larger than max limited %d bytes", MAX_CONTRACT_CODE_SIZE)
        json_spirit::Value abiJson;
        json_spirit::read_string(abi, abiJson);

        abi_def abi_d;
        from_variant(abiJson, abi_d);
        wasm::abi_serializer abis(abi_d, max_serialization_time);

        //abi in vector
        std::vector<char> abi_v = wasm::pack<wasm::abi_def>(abi_d);
        abi = string(abi_v.begin(), abi_v.end());
    } catch (wasm::exception &e) {
        JSON_RPC_ASSERT(false, e.code(), e.detail())
    }  
}

// set code and abi
Value setcodewasmcontracttx( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() < 4 || params.size() > 7, wasm::rpc::set_code_wasm_contract_tx_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type)(str_type)(str_type));

    //pack tx and commit
    Object obj_return;
    try{
        auto database = pCdMan->pAccountCache;
        auto wallet = pWalletMain;

        //get code and abi
        string code_file = GetAbsolutePath(params[2].get_str()).string();
        string abi_file = GetAbsolutePath(params[3].get_str()).string();
        JSON_RPC_ASSERT( !(code_file.empty() || abi_file.empty()), RPC_SCRIPT_FILEPATH_NOT_EXIST, "Wasm code and abi file name are both empty!")

        //read and validate code, abi
        string code, abi;
        read_and_validate_code(code_file, code);
        read_and_validate_abi(abi_file, abi);

        JSON_RPC_ASSERT(wallet != NULL, RPC_WALLET_ERROR, "wallet error")
        EnsureWalletIsUnlocked();
 
        CWasmContractTx tx;
        {
            auto contract = wasm::name(params[1].get_str()).value;
            string memo = (params.size() > 5)?params[5].get_str():"";

            CAccount sender;
            CKeyID sender_key_id;
            CRegID sender_reg_id;
            
            JSON_RPC_ASSERT(memo.size() < MAX_CONTRACT_MEMO_SIZE, RPC_INVALID_PARAMETER, strprintf("Memo must less than %d bytes", MAX_CONTRACT_MEMO_SIZE))
            JSON_RPC_ASSERT(GetKeyId(params[0].get_str(), sender_key_id), RPC_INVALID_ADDRESS_OR_KEY, "Invalid sender address")
            JSON_RPC_ASSERT(wallet->HaveKey(sender_key_id), RPC_WALLET_ERROR, "Sender address is not in wallet")
            JSON_RPC_ASSERT(database->GetRegId(sender_key_id, sender_reg_id), RPC_WALLET_ERROR, "Cannot get sender regid error")
            JSON_RPC_ASSERT(database->GetAccount(sender_key_id, sender), RPC_WALLET_ERROR, "Cannot get sender account")
            JSON_RPC_ASSERT(sender.HaveOwnerPubKey(), RPC_WALLET_ERROR, "Sender account is unregistered")

            const ComboMoney &fee = RPC_PARAM::GetFee(params, 4, TxType::UCONTRACT_DEPLOY_TX);
            RPC_PARAM::CheckAccountBalance(sender, fee.symbol, SUB_FREE, fee.GetSawiAmount());

            tx.nTxType    = WASM_CONTRACT_TX;
            tx.txUid      = sender_reg_id;
            tx.fee_symbol = fee.symbol;
            tx.llFees     = fee.GetSawiAmount();
            tx.valid_height = chainActive.Tip()->height;
            tx.inlinetransactions.push_back({wasmio,
                                             wasm::N(setcode),
                                             std::vector<permission>{{wasmio, wasmio_owner}},
                                             wasm::pack(std::tuple(contract, code, abi, memo))});
            //tx.nRunStep = tx.data.size();
            JSON_RPC_ASSERT(wallet->Sign(sender_key_id, tx.ComputeSignatureHash(), tx.signature), RPC_WALLET_ERROR, "Sign failed")
        }

        std::tuple<bool, string> r = wallet->CommitTx((CBaseTx * ) & tx);
        JSON_RPC_ASSERT(std::get<0>(r), RPC_WALLET_ERROR, std::get<1>(r))

        json_spirit::Value v;
        json_spirit::read_string(std::get<1>(r), v);
        json_spirit::Config::add(obj_return, "result",  v);

    } catch(wasm::exception &e){
        JSON_RPC_ASSERT(false, e.code(), e.detail())
    } catch(...){
        throw;
    }
    return obj_return;
}

bool is_digital_string(const string s){
    if (s.length() > 10 || s.length() == 0) //int max is 4294967295 can not over 10
        return false;
    for (auto c : s) {
        if (!isdigit(c))
            return false;
    }
    return true ;
}

bool is_regid(const string& s){
    int len = s.length();
    if (len >= 3) {
        int pos = s.find('-');

        if (pos > len - 1) {
            return false;
        }
        string firstStr = s.substr(0, pos);
        string endStr = s.substr(pos + 1);

        return is_digital_string(firstStr) && is_digital_string(endStr) ;
    }
    return false;
}

Value callwasmcontracttx( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() != 5 , wasm::rpc::call_wasm_contract_tx_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type)(str_type)(str_type));

    EnsureWalletIsUnlocked();

    CKeyID sender;
    if (!GetKeyId(params[0].get_str(), sender))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid sendaddress");

    // if (!GetKeyId(params[1].get_str(), recvKeyId)) {
    //     throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid app regid");
    // }

    wasm::name contract;
    try{
        contract = wasm::name(params[1].get_str());
    } catch(wasm::exception& e){
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, e.detail());
    }

   std::vector<char> abi;
   if(wasm::wasmio == contract.value ) {

        //contract = wasm::wasmio;
        wasm::abi_def wasmio_abi = wasmio_contract_abi();
        abi = wasm::pack<wasm::abi_def>(wasmio_abi);

    } else {
        // CRegID contractRegID(params[1].get_str());
        // if (contractRegID.IsEmpty()) {
        //     throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid contract address");
        // }
        CNickID nick_name(contract.to_string());

        // if (!pCdMan->pContractCache->HaveContract(nick_name)) {
        //     throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to get contract");
        // }
        //contract = wasm::RegID2Name(contractRegID);

        CUniversalContract contractCode;
        auto spCW = std::make_shared<CCacheWrapper>(pCdMan);
        spCW->contractCache.GetContract(nick_name, *spCW.get(), contractCode);
        if (contractCode.vm_type != VMType::WASM_VM)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "the vm type must be wasm");

        abi.insert(abi.end(), contractCode.abi.begin(), contractCode.abi.end());

    }


    // else{
    //    throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid contract");
    // }

    uint64_t action = wasm::NAME(params[2].get_str().c_str());

    string arguments = params[3].get_str();
    if (arguments.empty() || arguments.size() > MAX_CONTRACT_ARGUMENT_SIZE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Arguments empty or the size out of range");
    }

    std::vector<char> data;
    if (abi.size() > 0)
        try {
            data = wasm::abi_serializer::pack(abi, wasm::name(action).to_string(), arguments,
                                              max_serialization_time);
        } catch (wasm::exception &e) {
            throw JSONRPCError(e.code(), e.detail());
        }
    else {
        data.insert(data.begin(), arguments.begin(), arguments.end());
    }


    ComboMoney fee = RPC_PARAM::GetFee(params, 4, TxType::UCONTRACT_INVOKE_TX);

    uint32_t height = chainActive.Height();

    CPubKey sendPubKey;
    if (!pWalletMain->GetPubKey(sender, sendPubKey)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Key not found in the local wallet.");
    }

    CUserID sendUserId;
    CRegID sendRegId;
    sendUserId = (pCdMan->pAccountCache->GetRegId(CUserID(sender), sendRegId) && sendRegId.IsMature(height + 1))
                 ? CUserID(sendRegId)
                 : CUserID(sendPubKey);

    // CRegID recvRegId;
    // if (!pCdMan->pAccountCache->GetRegId(CUserID(recvKeyId), recvRegId)) {
    //     throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid app regid");
    // }

    CWasmContractTx tx;
    tx.nTxType = WASM_CONTRACT_TX;
    tx.txUid = sendUserId;
    tx.valid_height = height;
    tx.fee_symbol = fee.symbol;
    tx.llFees = fee.GetSawiAmount();

    tx.inlinetransactions.push_back({contract.value,
                                     action,
                                     std::vector<permission>{{wasm::N(sender), wasmio_owner}},
                                     data});

    // tx.symbol = amount.symbol;
    // tx.amount = amount.GetSawiAmount();

    if (!pWalletMain->Sign(sender, tx.ComputeSignatureHash(), tx.signature)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
    }

    std::tuple<bool, string> ret = pWalletMain->CommitTx((CBaseTx * ) & tx);
    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));
    }

    Object obj;

    json_spirit::Value abi_v;
    json_spirit::read_string(std::get<1>(ret), abi_v);
    json_spirit::Config::add(obj, "result",  abi_v);

    return obj;
}

Value gettablewasmcontracttx( const Array &params, bool fHelp ) {
    if (fHelp || params.size() < 2 || params.size() > 4) {
        throw runtime_error(
                "gettablewasmcontracttx \"contract\" \"table\" \"numbers\" \"begin_key\" \n"
                "1.\"contract\": (string, required) contract name\n"
                "2.\"table\":   (string, required) table name\n"
                "3.\"numbers\":   (numberic, optional) numbers\n"
                "4.\"begin_key\":   (string, optional) smallest key in Hex\n"
                "\nResult:\n"
                "\"rows\":        (string)\n"
                "\"more\":        (bool)\n"
                "\nExamples:\n" +
                HelpExampleCli("gettablewasmcontracttx",
                               " \"411994-1\" \"stat\" 10") +
                "\nAs json rpc call\n" +
                HelpExampleRpc("gettablewasmcontracttx",
                               "\"411994-1\", \"stat\", 10"));
        // 1.contract(id)
        // 2.table
        // 3.number
        // 4.begin_key
    }

    RPCTypeCheck(params, list_of(str_type)(str_type));

    // std::cout << "rpccall gettablerowwasmcontracttx "
    //           << " contract:" << params[0].get_str()
    //           << " table:" << params[1].get_str()
    //           << " numbers:" << params[2].get_str()
    //           //<< " last_key:" << params[3].get_str()
    //           << std::endl;

    //EnsureWalletIsUnlocked();

    // CRegID contractRegID(params[0].get_str());
    // if (contractRegID.IsEmpty()) {
    //     throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid contract address");
    // }

    // uint64_t contract = wasm::RegID2Name(contractRegID);

    // CUniversalContract contractCode;
    // if (!pCdMan->pContractCache->GetContract(contractRegID, contractCode))
    //     throw JSONRPCError(READ_SCRIPT_FAIL, "can not get contract code");

    wasm::name contract;
    CNickID contract_nick_id;
    CUniversalContract contractCode;
    auto spCW = std::make_shared<CCacheWrapper>(pCdMan);
    try{
        contract_nick_id = CNickID(params[0].get_str());
        contract = wasm::name(params[0].get_str());

        spCW->contractCache.GetContract(contract_nick_id, *spCW.get(), contractCode);
        if (contractCode.vm_type != VMType::WASM_VM)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "the vm type must be wasm");   

    } catch(wasm::exception& e){
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, e.detail());
    }


    std::vector<char> abi(contractCode.abi.begin(), contractCode.abi.end());
    if (abi.size() == 0)
        throw JSONRPCError(READ_SCRIPT_FAIL, "this contract didn't set abi");


    uint64_t table = wasm::NAME(params[1].get_str().c_str());

    uint64_t numbers = default_query_rows;
    if (params.size() > 2)
        numbers = std::atoi(params[2].get_str().data());

    string keyPrefix;
    std::vector<char> k = wasm::pack(std::tuple(contract, table));
    keyPrefix.insert(keyPrefix.end(), k.begin(), k.end());

    string lastKey = ""; // TODO: get last key
    if (params.size() > 3) {
        lastKey = FromHex(params[3].get_str());
    }


    //auto spCW = std::make_shared<CCacheWrapper>(pCdMan);
    // auto pGetter =  spCW->contractCache.CreateContractDatasGetter(account, keyPrefix, numbers, lastKey);
    // if (!pGetter || !pGetter->Execute()) {
    //     throw JSONRPCError(RPC_INVALID_PARAMS, "get contract datas error! contract_regid=%s, ");
    // }

    json_spirit::Object object;
    // try {
    //     json_spirit::Array vars;
    //     string last_key;

    //     for (auto item : pGetter->data_list) {
    //         last_key = pGetter->GetKey(item);
    //         const string &value = pGetter->GetValue(item);

    //         std::vector<char> row(value.begin(), value.end());
    //         //row.insert(row.end(), value.begin(), value.end());
    //         json_spirit::Value v = wasm::abi_serializer::unpack(abi, table, row, max_serialization_time);

    //         json_spirit::Object &obj = v.get_obj();
    //         obj.push_back(Pair("key", ToHex(last_key, "")));
    //         obj.push_back(Pair("value", ToHex(value, "")));

    //         vars.push_back(v);
    //     }

    //     object.push_back(Pair("rows", vars));
    //     object.push_back(Pair("more", pGetter->has_more));

    // } catch (wasm::exception &e) {
    //     throw JSONRPCError( e.code(), e.detail() );
    // }

    return object;


}

Value abijsontobinwasmcontracttx( const Array &params, bool fHelp ) {
    if (fHelp || params.size() < 2 || params.size() > 4) {
        throw runtime_error(
                "gettablewasmcontracttx \"contract\" \"action\" \"data\" \n"
                "1.\"contract\": (string, required) contract name\n"
                "2.\"action\":   (string, required) action name\n"
                "3.\"data\":   (json string, required) action data\n"
                "\nResult:\n"
                "\"data\":        (string)\n"
                "\nExamples:\n" +
                HelpExampleCli("gettablewasmcontracttx",
                               " \"411994-1\" \"transfer\" \'[\"walk\",\"mark\",\"1000.0000 EOS\",\"transfer to mark\"]\' ") +
                "\nAs json rpc call\n" +
                HelpExampleRpc("gettablewasmcontracttx",
                               "\"411994-1\", \"transfer\", \'[\"walk\",\"mark\",\"1000.0000 EOS\",\"transfer to mark\"]\' "));
        // 1.contract(id)
        // 2.action
        // 3.data
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type));

    CNickID contract;
    try{
        contract = CNickID(params[0].get_str());
    } catch(wasm::exception& e){
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, e.detail());
    }

    //uint64_t contract = wasm::RegID2Name(contractRegID);
    uint64_t action = wasm::NAME(params[1].get_str().c_str());

    string arguments = params[2].get_str();
    if (arguments.empty() || arguments.size() > MAX_CONTRACT_ARGUMENT_SIZE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Arguments empty or the size out of range");
    }

    CUniversalContract contractCode;
    auto spCW = std::make_shared<CCacheWrapper>(pCdMan);
    spCW->contractCache.GetContract(contract, *spCW.get(), contractCode);
    if (contractCode.vm_type != VMType::WASM_VM)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "the vm type must be wasm");

    std::vector<char> abi(contractCode.abi.begin(), contractCode.abi.end());
    if (abi.size() == 0)
        throw JSONRPCError(READ_SCRIPT_FAIL, "this contract didn't set abi");

    std::vector<char> data;
    try {
        data = wasm::abi_serializer::pack(abi, wasm::name(action).to_string(), arguments,
                                          max_serialization_time);
    } catch (wasm::exception &e) {
        throw JSONRPCError(e.code(), e.detail());
    }


    json_spirit::Object object;
    object.push_back(Pair("data", wasm::ToHex(data,"")));

    return object;
}


Value abibintojsonwasmcontracttx( const Array &params, bool fHelp ) {
    if (fHelp || params.size() < 2 || params.size() > 4) {
        throw runtime_error(
                "gettablewasmcontracttx \"contract\" \"action\" \"data\" \n"
                "1.\"contract\": (string, required) contract name\n"
                "2.\"action\":   (string, required) action name\n"
                "3.\"data\":   (binary hex string, required) action data\n"
                "\nResult:\n"
                "\"data\":        (string)\n"
                "\nExamples:\n" +
                HelpExampleCli("gettablewasmcontracttx",
                               " \"411994-1\" \"transfer\"  \"000000809a438deb000000000000af91809698000000000004454f5300000000107472616e7366657220746f206d61726b\" ") +
                "\nAs json rpc call\n" +
                HelpExampleRpc("gettablewasmcontracttx",
                               "\"411994-1\", \"transfer\", \"000000809a438deb000000000000af91809698000000000004454f5300000000107472616e7366657220746f206d61726b\" "));
        // 1.contract(id)
        // 2.action
        // 3.data
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type));

    CNickID contract;
    try{
        contract = CNickID(params[0].get_str());
    } catch(wasm::exception& e){
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, e.detail());
    }

    //uint64_t contract = wasm::RegID2Name(contractRegID);
    uint64_t action = wasm::NAME(params[1].get_str().c_str());

    string arguments = params[2].get_str();
    if (arguments.empty() || arguments.size() > MAX_CONTRACT_ARGUMENT_SIZE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Arguments empty or the size out of range");
    }

    CUniversalContract contractCode;
    auto spCW = std::make_shared<CCacheWrapper>(pCdMan);
    spCW->contractCache.GetContract(contract, *spCW.get(), contractCode);
    if (contractCode.vm_type != VMType::WASM_VM)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "the vm type must be wasm");

    std::vector<char> abi(contractCode.abi.begin(), contractCode.abi.end());
    if (abi.size() == 0)
        throw JSONRPCError(READ_SCRIPT_FAIL, "this contract didn't set abi");

    json_spirit::Object object;
    try {
        string binary = FromHex(arguments);
        std::vector<char> data(binary.begin(), binary.end());

        json_spirit::Value v = wasm::abi_serializer::unpack(abi, wasm::name(action).to_string(), data, max_serialization_time);

        object.push_back(Pair("data", v));

    } catch (wasm::exception &e) {
        throw JSONRPCError(e.code(), e.detail());
    }


    return object;

}

Value getcodewasmcontracttx( const Array &params, bool fHelp ) {
    if (fHelp || params.size() != 1 ) {
        throw runtime_error(
                "getcodewasmcontracttx \"contract\" \n"
                "1.\"contract\": (string, required) contract name\n"
                "\nResult:\n"
                "\"code\":        (string)\n"
                "\nExamples:\n" +
                HelpExampleCli("getcodewasmcontracttx",
                               " \"411994-1\" ") +
                "\nAs json rpc call\n" +
                HelpExampleRpc("getcodewasmcontracttx",
                               "\"411994-1\""));
        // 1.contract(id)
    }

    RPCTypeCheck(params, list_of(str_type));

    // CRegID contractRegID(params[0].get_str());
    // if (contractRegID.IsEmpty()) {
    //     throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid contract address");
    // }

    CNickID contract;
    try{
        contract = CNickID(params[0].get_str());
    } catch(wasm::exception& e){
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, e.detail());
    }

    CUniversalContract contractCode;
    auto spCW = std::make_shared<CCacheWrapper>(pCdMan);
    spCW->contractCache.GetContract(contract, *spCW.get(), contractCode);
    if (contractCode.vm_type != VMType::WASM_VM)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "the vm type must be wasm");

    if (contractCode.code.size() == 0)
        throw JSONRPCError(READ_SCRIPT_FAIL, "this contract didn't set code");

    json_spirit::Object object;

    object.push_back(Pair("code", wasm::ToHex(contractCode.code,"")));

    return object;

}

Value getabiwasmcontracttx( const Array &params, bool fHelp ) {
    if (fHelp || params.size() != 1 ) {
        throw runtime_error(
                "getcodewasmcontracttx \"contract\" \n"
                "1.\"contract\": (string, required) contract name\n"
                "\nResult:\n"
                "\"code\":        (string)\n"
                "\nExamples:\n" +
                HelpExampleCli("getcodewasmcontracttx",
                               " \"411994-1\" ") +
                "\nAs json rpc call\n" +
                HelpExampleRpc("getcodewasmcontracttx",
                               "\"411994-1\""));
        // 1.contract(id)
    }

    RPCTypeCheck(params, list_of(str_type));

    CNickID contract;
    try{
        contract = CNickID(params[0].get_str());
    } catch(wasm::exception& e){
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, e.detail());
    }

    CUniversalContract contractCode;
    auto spCW = std::make_shared<CCacheWrapper>(pCdMan);
    spCW->contractCache.GetContract(contract, *spCW.get(), contractCode);

    if (contractCode.vm_type != VMType::WASM_VM)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "the vm type must be wasm");

    if (contractCode.code.size() == 0)
        throw JSONRPCError(READ_SCRIPT_FAIL, "this contract didn't set code");

    json_spirit::Object object;


    std::vector<char> abi(contractCode.abi.begin(), contractCode.abi.end());
    abi_def abi_d = wasm::unpack<wasm::abi_def>(abi);

    json_spirit::Value v;
    wasm::to_variant(abi_d, v);
    object.push_back(Pair("abi", v));

    return object;

}