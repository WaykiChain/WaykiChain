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
    if( !( expr ) ){                      \
        throw JSONRPCError(code, msg);    \
    }

#define RESPONSE_RPC_HELP(expr , msg)   \
    if( ( expr ) ){                     \
        throw runtime_error( msg);      \
    }

bool read_file_limit(const string& path, string& data, uint64_t max_size){
    try {
        if(path.empty()) return false;

        char byte;
        ifstream f(path, ios::binary);

        streampos pos = f.tellg();
        f.seekg(0, ios::end);
        size_t size = f.tellg();
        if (size == 0 || size > max_size) return false;

        f.seekg(pos);
        while (f.get(byte)) data.push_back(byte);
        // size_t size = data.size();
        // if (size == 0 || size > max_size) return false;
        return true;
    } catch (...) {
        return false;
    } 
}

void read_and_validate_code(const string& path, string& code){
    try {
        WASM_ASSERT(read_file_limit(path, code, MAX_CONTRACT_CODE_SIZE), 
                    file_read_exception, 
                    "wasm code file is empty or larger than max limited %d bytes", MAX_CONTRACT_CODE_SIZE)
        vector <uint8_t> c;
        c.insert(c.begin(), code.begin(), code.end());
        wasm_interface wasmif;
        wasmif.validate(c);
    } catch (wasm::exception &e) {
        JSON_RPC_ASSERT(false, e.code(), e.detail())
    }    
}

void read_and_validate_abi(const string& abi_file, string& abi){
    try {
        WASM_ASSERT(read_file_limit(abi_file, abi, MAX_CONTRACT_CODE_SIZE), 
                    file_read_exception, 
                    "wasm abi file is empty or larger than max limited %d bytes", MAX_CONTRACT_CODE_SIZE)
        json_spirit::Value abi_json;
        json_spirit::read_string(abi, abi_json);

        abi_def abi_struct;
        from_variant(abi_json, abi_struct);
        wasm::abi_serializer abis(abi_struct, max_serialization_time);//validate in abi_serializer constructor

        std::vector<char> abi_bytes = wasm::pack<wasm::abi_def>(abi_struct);
        abi = string(abi_bytes.begin(), abi_bytes.end());
    } catch (wasm::exception &e) {
        JSON_RPC_ASSERT(false, e.code(), e.detail())
    }  
}

// set code and abi
Value setcodewasmcontracttx( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() < 4 || params.size() > 7, wasm::rpc::set_code_wasm_contract_tx_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type)(str_type)(str_type));

    //pack tx and commit
    try{
        auto database = pCdMan->pAccountCache;
        auto wallet   = pWalletMain;

        //read and validate code, abi
        string code_file = GetAbsolutePath(params[2].get_str()).string();
        string abi_file  = GetAbsolutePath(params[3].get_str()).string();
        JSON_RPC_ASSERT( !(code_file.empty() || abi_file.empty()), RPC_SCRIPT_FILEPATH_NOT_EXIST, "Wasm code and abi file name are both empty!")
        string code, abi;
        read_and_validate_code(code_file, code);
        read_and_validate_abi(abi_file, abi);

        JSON_RPC_ASSERT(wallet != NULL, RPC_WALLET_ERROR, "wallet error")
        EnsureWalletIsUnlocked();
 
        CWasmContractTx tx;
        {
            auto contract = wasm::name(params[1].get_str()).value;
            string memo   = (params.size() > 5)?params[5].get_str():"";

            CAccount sender;
            CKeyID sender_key_id;
            CRegID sender_reg_id;
            JSON_RPC_ASSERT(memo.size() < MAX_CONTRACT_MEMO_SIZE,             RPC_INVALID_PARAMETER, strprintf("Memo must less than %d bytes", MAX_CONTRACT_MEMO_SIZE))
            JSON_RPC_ASSERT(GetKeyId(params[0].get_str(), sender_key_id),     RPC_INVALID_ADDRESS_OR_KEY, "Invalid sender address")
            JSON_RPC_ASSERT(wallet->HaveKey(sender_key_id),                   RPC_WALLET_ERROR, "Sender address is not in wallet")
            JSON_RPC_ASSERT(database->GetRegId(sender_key_id, sender_reg_id), RPC_WALLET_ERROR, "Cannot get sender regid error")
            JSON_RPC_ASSERT(database->GetAccount(sender_key_id, sender),      RPC_WALLET_ERROR, "Cannot get sender account")
            JSON_RPC_ASSERT(sender.HaveOwnerPubKey(),                         RPC_WALLET_ERROR, "Sender account is unregistered")

            const ComboMoney &fee = RPC_PARAM::GetFee(params, 4, TxType::UCONTRACT_DEPLOY_TX);
            RPC_PARAM::CheckAccountBalance(sender, fee.symbol, SUB_FREE, fee.GetSawiAmount());

            tx.nTxType      = WASM_CONTRACT_TX;
            tx.txUid        = sender_reg_id;
            tx.fee_symbol   = fee.symbol;
            tx.llFees       = fee.GetSawiAmount();
            tx.valid_height = chainActive.Tip()->height;
            tx.inlinetransactions.push_back({wasmio, wasm::N(setcode), std::vector<permission>{{wasmio, wasmio_owner}},
                                             wasm::pack(std::tuple(contract, code, abi, memo))});
            //tx.nRunStep = tx.data.size();
            JSON_RPC_ASSERT(wallet->Sign(sender_key_id, tx.ComputeSignatureHash(), tx.signature), RPC_WALLET_ERROR, "Sign failed")
        }

        std::tuple<bool, string> r = wallet->CommitTx((CBaseTx * ) & tx);
        JSON_RPC_ASSERT(std::get<0>(r), RPC_WALLET_ERROR, std::get<1>(r))

        Object obj_return;
        json_spirit::Value v;
        json_spirit::read_string(std::get<1>(r), v);
        json_spirit::Config::add(obj_return, "result",  v);

        return obj_return;
    } catch(wasm::exception &e){
        JSON_RPC_ASSERT(false, e.code(), e.detail())
    } catch(...){
        throw;
    }
}

void get_contract(CAccountDBCache* database_account, CContractDBCache* database_contract, const wasm::name& contract_name, CAccount& contract, CUniversalContract& contract_store){
    //CAccount contract;
    WASM_ASSERT(database_account->GetAccount(nick_name(contract_name.to_string()), contract), account_operation_exception,
                "wasmnativecontract.Setcode, contract account does not exist, contract = %s",contract_name.to_string().c_str())
    JSON_RPC_ASSERT(database_contract->HaveContract(contract.regid),                RPC_WALLET_ERROR,  strprintf("cannot get contract with regid = %s", contract.regid.ToString().c_str()))
    JSON_RPC_ASSERT(database_contract->GetContract(contract.regid, contract_store), RPC_WALLET_ERROR,  strprintf("cannot get contract with regid = %s", contract.regid.ToString().c_str()))
    JSON_RPC_ASSERT(contract_store.vm_type == VMType::WASM_VM,                      RPC_WALLET_ERROR,  "VM type must be wasm")
    JSON_RPC_ASSERT(contract_store.abi.size() > 0,                                  RPC_WALLET_ERROR,  "contract lose abi")
}

Value callwasmcontracttx( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() != 5 , wasm::rpc::call_wasm_contract_tx_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type)(str_type)(str_type));

    try {
        auto database_account = pCdMan->pAccountCache;
        auto database_contract = pCdMan->pContractCache;
        auto wallet = pWalletMain;

        //serialize action data
        std::vector<char> abi;
        wasm::name contract_name = wasm::name(params[1].get_str());
        if(wasm::wasmio == contract_name.value ) {
            wasm::abi_def wasmio_abi = wasmio_contract_abi();
            abi = wasm::pack<wasm::abi_def>(wasmio_abi);
        } else {
            CAccount contract;
            CUniversalContract contract_store;
            get_contract(database_account, database_contract, contract_name, contract, contract_store );
            abi = std::vector<char>(contract_store.abi.begin(), contract_store.abi.end());
        }

        EnsureWalletIsUnlocked();
        CWasmContractTx tx;
        {
            uint64_t action = wasm::name(params[2].get_str()).value;
            std::vector<char> action_data(params[3].get_str().begin(), params[3].get_str().end() );
            JSON_RPC_ASSERT(!action_data.empty() && action_data.size() < MAX_CONTRACT_ARGUMENT_SIZE, RPC_WALLET_ERROR,  "Arguments empty or the size out of range")
            if( abi.size() > 0 ) action_data = wasm::abi_serializer::pack(abi, wasm::name(action).to_string(), params[3].get_str(), max_serialization_time);

            CAccount sender; //should be authorizer(s)
            CKeyID sender_key_id;
            CRegID sender_reg_id;
            JSON_RPC_ASSERT(GetKeyId(params[0].get_str(), sender_key_id),             RPC_INVALID_ADDRESS_OR_KEY, "Invalid sender address")
            JSON_RPC_ASSERT(wallet->HaveKey(sender_key_id),                           RPC_WALLET_ERROR, "Sender address is not in wallet")
            JSON_RPC_ASSERT(database_account->GetRegId(sender_key_id, sender_reg_id), RPC_WALLET_ERROR, "Cannot get sender regid error")
            JSON_RPC_ASSERT(database_account->GetAccount(sender_key_id, sender),      RPC_WALLET_ERROR, "Cannot get sender account")
            JSON_RPC_ASSERT(sender.HaveOwnerPubKey(),                                 RPC_WALLET_ERROR, "Sender account is unregistered")

            ComboMoney fee = RPC_PARAM::GetFee(params, 4, TxType::UCONTRACT_INVOKE_TX);

            tx.nTxType      = WASM_CONTRACT_TX;
            tx.txUid        = sender_reg_id;
            tx.valid_height = chainActive.Height();
            tx.fee_symbol   = fee.symbol;
            tx.llFees       = fee.GetSawiAmount();
            tx.inlinetransactions.push_back({contract_name.value, action, std::vector<permission>{{wasm::N(sender), wasmio_owner}},
                                             action_data});
            // tx.symbol = amount.symbol;
            // tx.amount = amount.GetSawiAmount();
            JSON_RPC_ASSERT(wallet->Sign(sender_key_id, tx.ComputeSignatureHash(), tx.signature), RPC_WALLET_ERROR, "Sign failed")
        }

        std::tuple<bool, string> ret = pWalletMain->CommitTx((CBaseTx * ) & tx);
        JSON_RPC_ASSERT(std::get<0>(ret), RPC_WALLET_ERROR, std::get<1>(ret))

        Object obj_return;
        json_spirit::Value v;
        json_spirit::read_string(std::get<1>(ret), v);
        json_spirit::Config::add(obj_return, "result",  v);
        return obj_return;
    } catch(wasm::exception &e){
        JSON_RPC_ASSERT(false, e.code(), e.detail())
    } catch(...){
        throw;
    }
}

Value gettablewasmcontracttx( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() < 2 || params.size() > 4 , wasm::rpc::get_table_wasm_contract_tx_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type)(str_type));

    try{
        auto database_account = pCdMan->pAccountCache;
        auto database_contract = pCdMan->pContractCache;

        wasm::name contract_name  = wasm::name(params[0].get_str());
        wasm::name contract_table = wasm::name(params[1].get_str());

        std::vector<char> abi;
        CAccount contract;
        CUniversalContract contract_store;
        get_contract(database_account, database_contract, contract_name, contract, contract_store );
        abi = std::vector<char>(contract_store.abi.begin(), contract_store.abi.end());

        uint64_t numbers = default_query_rows;
        if (params.size() > 2) numbers = std::atoi(params[2].get_str().data());

        std::vector<char> key_prefix = wasm::pack(std::tuple(contract_name.value, contract_table.value));
        string search_key(key_prefix.data(),key_prefix.size());
        string start_key = (params.size() > 3) ? FromHex(params[3].get_str()) : "";

        auto pGetter = database_contract->CreateContractDatasGetter(contract.regid, search_key, numbers, start_key);
        JSON_RPC_ASSERT(pGetter && pGetter->Execute(), RPC_INVALID_PARAMS,  strprintf("cannot get contract table with name = %s", contract_name.to_string().c_str()))

        json_spirit::Object object_return;
        json_spirit::Array rows_json;
        for (auto item : pGetter->data_list) {
            const string &key   = pGetter->GetKey(item);
            const string &value = pGetter->GetValue(item);

            //unpack value in bytes to json
            std::vector<char> value_bytes(value.begin(), value.end());
            json_spirit::Value value_json    = wasm::abi_serializer::unpack(abi, contract_table.value, value_bytes, max_serialization_time);
            json_spirit::Object &object_json = value_json.get_obj(); 

            //append key and value 
            object_json.push_back(Pair("key",   ToHex(key, "")));
            object_json.push_back(Pair("value", ToHex(value, "")));

            rows_json.push_back(value_json);
        }

        object_return.push_back(Pair("rows", rows_json));
        object_return.push_back(Pair("more", pGetter->has_more));

        return object_return;
    } catch(wasm::exception &e){
        JSON_RPC_ASSERT(false, e.code(), e.detail())
    } catch(...){
        throw;
    }
}

Value abijsontobinwasmcontracttx( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() < 2 || params.size() > 4 , wasm::rpc::abi_json_to_bin_wasm_contract_tx_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type));

    try{
        auto database_account = pCdMan->pAccountCache;
        auto database_contract = pCdMan->pContractCache;

        wasm::name contract_name  = wasm::name(params[0].get_str());
        wasm::name contract_action = wasm::name(params[1].get_str());

        std::vector<char> abi;
        CAccount contract;
        CUniversalContract contract_store;
        get_contract(database_account, database_contract, contract_name, contract, contract_store );
        abi = std::vector<char>(contract_store.abi.begin(), contract_store.abi.end());

        std::vector<char> action_data(params[2].get_str().begin(), params[2].get_str().end() );
        JSON_RPC_ASSERT(!action_data.empty() && action_data.size() < MAX_CONTRACT_ARGUMENT_SIZE, RPC_WALLET_ERROR,  "Arguments empty or the size out of range")
        if( abi.size() > 0 ) action_data = wasm::abi_serializer::pack(abi, contract_action.to_string(), params[2].get_str(), max_serialization_time);

        json_spirit::Object object_return;
        object_return.push_back(Pair("data", wasm::ToHex(action_data,"")));
        return object_return;
    } catch(wasm::exception &e){
        JSON_RPC_ASSERT(false, e.code(), e.detail())
    } catch(...){
        throw;
    }
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