// Copyright (c) 2019 xiaoyu
// Copyright (c) 2017-2019 WaykiChain Developers
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
#include "wasm_native_contract.hpp"
#include "wasm_rpc_message.hpp"
#include "wasm_variant_trace.hpp"

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

#define JSON_RPC_CAPTURE_AND_RETHROW                      \
    catch(wasm::exception &e){                            \
            JSON_RPC_ASSERT(false, e.code(), e.detail())  \
        } catch(...){                                     \
            throw;                                        \
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
        return true;
    } catch (...) {
        return false;
    }
}

void read_and_validate_code(const string& path, string& code){

    try {
        WASM_ASSERT(read_file_limit(path, code, MAX_CONTRACT_CODE_SIZE),
                    file_read_exception,
                    "rpcwasm.read_and_validate_code, wasm code file is empty or larger than max limited '%d' bytes", MAX_CONTRACT_CODE_SIZE)
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
                    "rpcwasm.read_and_validate_abi, wasm abi file is empty or larger than max limited '%d' bytes", MAX_CONTRACT_CODE_SIZE)
        json_spirit::Value abi_json;
        json_spirit::read_string(abi, abi_json);

        abi_def abi_struct;
        from_variant(abi_json, abi_struct);
        wasm::abi_serializer abis(abi_struct, max_serialization_time);//validate in abi_serializer constructor

        std::vector<char> abi_bytes = wasm::pack<wasm::abi_def>(abi_struct);
        abi                         = string(abi_bytes.begin(), abi_bytes.end());
    } catch (wasm::exception &e) {
        JSON_RPC_ASSERT(false, e.code(), e.detail())
    }
}

void get_contract(CAccountDBCache*   database_account,
                 CContractDBCache*   database_contract,
                 const wasm::name&   contract_name,
                 CAccount&           contract,
                 CUniversalContract& contract_store){

    WASM_ASSERT(database_account->GetAccount(nick_name(contract_name.to_string()), contract),
                account_operation_exception,
                "rpcwasm.get_contract, contract '%s' does not exist",
                contract_name.to_string().c_str())
    //JSON_RPC_ASSERT(database_contract->HaveContract(contract.regid),                RPC_WALLET_ERROR,  strprintf("Cannot get contract %s", contract_name.to_string().c_str()))
    JSON_RPC_ASSERT(database_contract->GetContract(contract.regid, contract_store),
                    RPC_WALLET_ERROR,
                    strprintf("rpcwasm.get_contract, cannot get contract '%s'", 
                    contract_name.to_string().c_str()))
    JSON_RPC_ASSERT(contract_store.vm_type == VMType::WASM_VM,
                    RPC_WALLET_ERROR,
                   "rpcwasm.get_contract, must be wasm VM")
    JSON_RPC_ASSERT(contract_store.abi.size() > 0,
                    RPC_WALLET_ERROR,
                    "rpcwasm.get_contract, contract lose abi")
    //JSON_RPC_ASSERT(contract_store.code.size() > 0,                                 RPC_WALLET_ERROR,  "contract lose code")
}

// set code and abi
Value submitwasmcontractdeploytx( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() < 4 || params.size() > 5, wasm::rpc::submit_wasm_contract_deploy_tx_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type)(str_type)(str_type));

    try{
        auto database = pCdMan->pAccountCache;
        auto wallet   = pWalletMain;

        string code, abi;
        read_and_validate_code(GetAbsolutePath(params[2].get_str()).string(), code);
        read_and_validate_abi (GetAbsolutePath(params[3].get_str()).string(), abi );

        JSON_RPC_ASSERT(wallet != NULL, RPC_WALLET_ERROR, "rpcwasm.submitwasmcontractdeploytx, wallet error")
        EnsureWalletIsUnlocked();

        CWasmContractTx tx;
        {
            CAccount payer;
            auto              contract   = wasm::name(params[1].get_str());
            auto              payer_name = wasm::name(params[0].get_str());
            const ComboMoney& fee        = RPC_PARAM::GetFee(params, 4, TxType::UCONTRACT_DEPLOY_TX);

            WASM_ASSERT(database->GetAccount(nick_name(payer_name.value), payer), 
                        account_operation_exception,
                        "rpcwasm.submitwasmcontractdeploytx, sender '%s' does not exist ",
                        payer_name.to_string().c_str())
            RPC_PARAM::CheckAccountBalance(payer, fee.symbol, SUB_FREE, fee.GetSawiAmount());

            tx.nTxType      = WASM_CONTRACT_TX;
            tx.txUid        = payer.regid;
            tx.fee_symbol   = fee.symbol;
            tx.llFees       = fee.GetSawiAmount();
            tx.valid_height = chainActive.Tip()->height;
            tx.inline_transactions.push_back({wasmio, wasm::N(setcode), std::vector<permission>{{payer_name.value, wasmio_owner}},
                                             wasm::pack(std::tuple(contract.value, code, abi, ""))});

            JSON_RPC_ASSERT(wallet->Sign(payer.keyid, tx.ComputeSignatureHash(), tx.signature),
                           RPC_WALLET_ERROR, "rpcwasm.submitwasmcontractdeploytx, sign failed")

            tx.signatures.push_back({payer_name.value, tx.signature});
        }

        std::tuple<bool, string> ret = wallet->CommitTx((CBaseTx * ) & tx);
        JSON_RPC_ASSERT(std::get<0>(ret), RPC_WALLET_ERROR, std::get<1>(ret))

        Object obj_return;
        json_spirit::Config::add(obj_return, "txid", std::get<1>(ret) );
        return obj_return;

    } JSON_RPC_CAPTURE_AND_RETHROW;
}


Value submitwasmcontractcalltx( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() < 4 || params.size() > 5 , wasm::rpc::submit_wasm_contract_call_tx_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type)(str_type)(str_type));

    try {
        auto database_account  = pCdMan->pAccountCache;
        auto database_contract = pCdMan->pContractCache;
        auto wallet            = pWalletMain;

        //get abi
        std::vector<char> abi;
        wasm::name        contract_name = wasm::name(params[1].get_str());
        if(!get_native_contract_abi(contract_name.value, abi)){
            CAccount           contract;
            CUniversalContract contract_store;
            get_contract(database_account, database_contract, contract_name, contract, contract_store );
            abi = std::vector<char>(contract_store.abi.begin(), contract_store.abi.end());
        }

        EnsureWalletIsUnlocked();
        CWasmContractTx tx;
        {      
            CAccount payer;
            auto     payer_name   = wasm::name(params[0].get_str());
            auto     action       = wasm::name(params[2].get_str());
            WASM_ASSERT(database_account->GetAccount(nick_name(payer_name.value), payer), account_operation_exception,
                "rpcwasm.submitwasmcontractcalltx, payer '%s' does not exist",payer_name.to_string().c_str())

            std::vector<char> action_data(params[3].get_str().begin(), params[3].get_str().end());
            JSON_RPC_ASSERT(!action_data.empty() && action_data.size() < MAX_CONTRACT_ARGUMENT_SIZE, RPC_WALLET_ERROR,
                            "rpcwasm.submitwasmcontractcalltx, arguments is empty or out of size range")
            if( abi.size() > 0 ) 
                action_data = wasm::abi_serializer::pack(abi, action.to_string(), params[3].get_str(), max_serialization_time);

            ComboMoney fee  = RPC_PARAM::GetFee(params, 4, TxType::UCONTRACT_INVOKE_TX);

            tx.nTxType      = WASM_CONTRACT_TX;
            tx.txUid        = payer.regid;
            tx.valid_height = chainActive.Height();
            tx.fee_symbol   = fee.symbol;
            tx.llFees       = fee.GetSawiAmount();

            //for(int i = 0; i < 10000 ; i++)
            tx.inline_transactions.push_back({contract_name.value, action.value, std::vector<permission>{{payer_name.value, wasmio_owner}}, action_data});

            JSON_RPC_ASSERT(wallet->Sign(payer.keyid, tx.ComputeSignatureHash(), tx.signature), RPC_WALLET_ERROR,
                            "rpcwasm.submitwasmcontractcalltx, sign failed")

            tx.signatures.push_back({payer_name.value, tx.signature});
        }

        std::tuple<bool, string> ret = wallet->CommitTx((CBaseTx * ) & tx);
        JSON_RPC_ASSERT(std::get<0>(ret), RPC_WALLET_ERROR, std::get<1>(ret))

        Object obj_return;
        // json_spirit::Value value_json;
        // json_spirit::read_string(std::get<1>(ret), value_json);
        // json_spirit::Config::add(obj_return, "result",  value_json);
        json_spirit::Config::add(obj_return, "txid", std::get<1>(ret) );
        return obj_return;

    } JSON_RPC_CAPTURE_AND_RETHROW;

}

Value gettablewasm( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() < 2 || params.size() > 4 , wasm::rpc::get_table_wasm_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type)(str_type));

    try{
        auto database_account  = pCdMan->pAccountCache;
        auto database_contract = pCdMan->pContractCache;

        auto contract_name     = wasm::name(params[0].get_str());
        auto contract_table    = wasm::name(params[1].get_str());

        JSON_RPC_ASSERT(!is_native_contract(contract_name.value), RPC_INVALID_PARAMS,
                        strprintf("rpcwasm.gettablewasmcontracttx, cannot get table from native contract '%s'", contract_name.to_string().c_str()))

        CAccount contract;
        CUniversalContract contract_store;
        get_contract(database_account, database_contract, contract_name, contract, contract_store );
        std::vector<char> abi = std::vector<char>(contract_store.abi.begin(), contract_store.abi.end());

        uint64_t numbers = default_query_rows;
        if (params.size() > 2) numbers = std::atoi(params[2].get_str().data());

        std::vector<char> key_prefix = wasm::pack(std::tuple(contract_name.value, contract_table.value));
        string search_key(key_prefix.data(),key_prefix.size());
        string start_key = (params.size() > 3) ? FromHex(params[3].get_str()) : "";

        auto pContractDataIt = database_contract->CreateContractDataIterator(contract.regid, search_key);
        JSON_RPC_ASSERT(pContractDataIt, RPC_INVALID_PARAMS,
                        strprintf("rpcwasm.gettablewasmcontracttx, cannot get table from contract '%s'", contract_name.to_string().c_str()))

        bool                hasMore = false;
        json_spirit::Object object_return;
        json_spirit::Array  row_json;
        for (pContractDataIt->SeekUpper(&start_key); pContractDataIt->IsValid(); pContractDataIt->Next()) {
            if (pContractDataIt->GotCount() > numbers) {
                hasMore = true;
                break;
            }
            const string &key   = pContractDataIt->GetContractKey();
            const string &value = pContractDataIt->GetValue();

            //unpack value in bytes to json
            std::vector<char> value_bytes(value.begin(), value.end());
            json_spirit::Value value_json    = wasm::abi_serializer::unpack(abi, contract_table.value, value_bytes, max_serialization_time);
            json_spirit::Object &object_json = value_json.get_obj();

            //append key and value
            object_json.push_back(Pair("key",   ToHex(key, "")));
            object_json.push_back(Pair("value", ToHex(value, "")));

            row_json.push_back(value_json);
        }

        object_return.push_back(Pair("rows", row_json));
        object_return.push_back(Pair("more", hasMore));
        return object_return;

    } JSON_RPC_CAPTURE_AND_RETHROW;

}

Value jsontobinwasm( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() < 2 || params.size() > 4 , wasm::rpc::json_to_bin_wasm_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type));

    try{
        auto database_account  = pCdMan->pAccountCache;
        auto database_contract = pCdMan->pContractCache;

        auto contract_name     = wasm::name(params[0].get_str());
        auto contract_action   = wasm::name(params[1].get_str());

        std::vector<char>  abi;
        CAccount           contract;
        CUniversalContract contract_store;
        if(!get_native_contract_abi(contract_name.value, abi)){
            get_contract(database_account, database_contract, contract_name, contract, contract_store );
            abi = std::vector<char>(contract_store.abi.begin(), contract_store.abi.end());
        }

        string arguments = params[2].get_str();
        JSON_RPC_ASSERT(!arguments.empty() && arguments.size() < MAX_CONTRACT_ARGUMENT_SIZE,
                        RPC_INVALID_PARAMETER,
                        "rpcwasm.abijsontobinwasmcontracttx, arguments is empty or out of size range")
        std::vector<char> action_data(arguments.begin(), arguments.end() );
        if( abi.size() > 0 ) action_data = wasm::abi_serializer::pack(abi, contract_action.to_string(), arguments, max_serialization_time);

        json_spirit::Object object_return;
        object_return.push_back(Pair("data", wasm::ToHex(action_data,"")));
        return object_return;

    } JSON_RPC_CAPTURE_AND_RETHROW;

}


Value bintojsonwasm( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() < 2 || params.size() > 4 , wasm::rpc::bin_to_json_wasm_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type));

    try{
        auto database_account  = pCdMan->pAccountCache;
        auto database_contract = pCdMan->pContractCache;

        auto contract_name     = wasm::name(params[0].get_str());
        auto contract_action   = wasm::name(params[1].get_str());

        std::vector<char>  abi;
        CAccount           contract;
        CUniversalContract contract_store;
        if(!get_native_contract_abi(contract_name.value, abi)){
            get_contract(database_account, database_contract, contract_name, contract, contract_store );
            abi = std::vector<char>(contract_store.abi.begin(), contract_store.abi.end());
        }

        string arguments = FromHex(params[2].get_str());
        JSON_RPC_ASSERT(!arguments.empty() && arguments.size() < MAX_CONTRACT_ARGUMENT_SIZE,
                        RPC_INVALID_PARAMETER,
                        "rpcwasm.abibintojsonwasmcontracttx, arguments is empty or out of size range")

        json_spirit::Object object_return;
        std::vector<char>   action_data(arguments.begin(), arguments.end() );
        json_spirit::Value  value = wasm::abi_serializer::unpack(abi, contract_action.to_string(), action_data, max_serialization_time);
        object_return.push_back(Pair("data", value));
        return object_return;

    } JSON_RPC_CAPTURE_AND_RETHROW;

}

Value getcodewasm( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() != 1 , wasm::rpc::get_code_wasm_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type));

    try{
        auto database_account  = pCdMan->pAccountCache;
        auto database_contract = pCdMan->pContractCache;

        auto contract_name     = wasm::name(params[0].get_str());
        JSON_RPC_ASSERT(!is_native_contract(contract_name.value),
                        RPC_INVALID_PARAMS,
                        strprintf("rpcwasm.getcodewasmcontracttx, cannot get code from native contract '%s'", contract_name.to_string().c_str()))

        CAccount           contract;
        CUniversalContract contract_store;
        get_contract(database_account, database_contract, contract_name, contract, contract_store );

        json_spirit::Object object_return;
        object_return.push_back(Pair("code", wasm::ToHex(contract_store.code,"")));
        return object_return;

    } JSON_RPC_CAPTURE_AND_RETHROW;

}

Value getabiwasm( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() != 1 , wasm::rpc::get_abi_wasm_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type));

    try{
        auto database_account  = pCdMan->pAccountCache;
        auto database_contract = pCdMan->pContractCache;

        auto contract_name     = wasm::name(params[0].get_str());

        vector<char>       abi;
        CAccount           contract;
        CUniversalContract contract_store;
        if(!get_native_contract_abi(contract_name.value, abi)){
            get_contract(database_account, database_contract, contract_name, contract, contract_store );
            abi = std::vector<char>(contract_store.abi.begin(), contract_store.abi.end());
        }

        json_spirit::Object object_return;
        json_spirit::Value  abi_json;
        abi_def             abi_struct = wasm::unpack<wasm::abi_def>(abi);
        wasm::to_variant(abi_struct, abi_json);
        object_return.push_back(Pair("abi", abi_json));
        return object_return;

    } JSON_RPC_CAPTURE_AND_RETHROW;

}

Value gettxtrace( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() != 1 , wasm::rpc::get_tx_trace_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type));

    try{
        auto database  = std::make_shared<CCacheWrapper>(pCdMan);
        auto resolver  = make_resolver(database);

        auto trx_id    = uint256S(params[0].get_str());

        string  trace_string;
        WASM_ASSERT( database->contractCache.GetContractTraces(trx_id, trace_string),
                     wasm_exception,
                     "rpc::gettxtrace, get tx trace failed! txid '%s'",
                     trx_id.ToString().c_str())

        json_spirit::Object object_return;
        json_spirit::Value  value_json;
        std::vector<char>   trace_bytes = std::vector<char>(trace_string.begin(), trace_string.end());
        transaction_trace   trace       = wasm::unpack<transaction_trace>(trace_bytes);

        to_variant(trace, value_json, resolver);
        object_return.push_back(Pair("tx_trace", value_json));

        return object_return;

    } JSON_RPC_CAPTURE_AND_RETHROW;

}