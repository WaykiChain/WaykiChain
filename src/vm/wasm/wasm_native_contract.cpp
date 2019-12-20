#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#include "config/errorcode.h"
#include "wasm/wasm_native_contract.hpp"
#include "wasm/exceptions.hpp"
#include "wasm/datastream.hpp"
#include "wasm/types/asset.hpp"
#include "persistence/cachewrapper.h"
#include "wasm/wasm_log.hpp"
#include "wasm/wasm_native_contract_abi.hpp"
#include "wasm/abi_def.hpp"
#include "wasm/abi_serializer.hpp"

using namespace std;
using namespace wasm;

namespace wasm {

    void wasmio_native_setcode(wasm_context &context) {

         WASM_ASSERT(context._receiver == wasmio, wasm_assert_exception, 
                     "wasmio_native_setcode.setcode, Except contract wasmio, But get %s", wasm::name(context._receiver).to_string().c_str());

        auto &database_account         = context.database.accountCache;
        auto &database_contract        = context.database.contractCache;
        auto &control_trx              = context.control_trx;

        std::tuple<uint64_t, string, string, string> set_code_data = wasm::unpack<std::tuple<uint64_t, string, string, string>>(context.trx.data);
        auto contract_name = wasm::name(std::get<0>(set_code_data));
        auto code          = std::get<1>(set_code_data);
        auto abi           = std::get<2>(set_code_data);
        auto memo          = std::get<3>(set_code_data);

        context.require_auth(contract_name.value); 

        CAccount contract;
        WASM_ASSERT(database_account.GetAccount(nick_name(contract_name.to_string()), contract),
                    account_operation_exception,
                    "wasmio_native_setcode.setcode, Contract does not exist, contract = %s",contract_name.to_string().c_str()) 

        CUniversalContract contract_store;
        // ban code reset
        // WASM_ASSERT(!cw.contractCache.GetContract(contract.regid, contract_store),
        //             account_operation_exception,
        //             "wasmnativecontract.Setcode, can not reset code, contract = %s",
        //             contract_name.c_str()) 
        contract_store.vm_type = VMType::WASM_VM;
        contract_store.code = code;
        contract_store.abi  = abi;
        contract_store.memo = memo;

        WASM_ASSERT(database_contract.SaveContract(contract.regid, contract_store), 
                    account_operation_exception,
                    "%s","wasmio_native_setcode.setcode, Save account error")
    }
    
    void wasmio_bank_native_transfer(wasm_context &context) {

        WASM_ASSERT(context._receiver == wasmio_bank, wasm_assert_exception, 
                    "wasmio_bank_native_transfer.transfer, Except contract wasmio.bank, But get %s", wasm::name(context._receiver).to_string().c_str());

        auto &database = context.database.accountCache;

        context.control_trx.nRunStep += (16 + context.trx.data.size()) * fuel_store_fee_per_byte;//16 bytes are contract and action

        std::tuple<uint64_t, uint64_t, wasm::asset, string> transfer_data = wasm::unpack<std::tuple<uint64_t, uint64_t, wasm::asset, string>>(context.trx.data);
        auto from     = std::get<0>(transfer_data);
        auto to       = std::get<1>(transfer_data);
        auto quantity = std::get<2>(transfer_data);
        auto memo     = std::get<3>(transfer_data);

        context.require_auth(from); //from auth
        WASM_ASSERT(from != to, wasm_assert_exception,"%s", "cannot transfer to self");
        WASM_ASSERT(context.is_account(to), wasm_assert_exception, "to %s account does not exist", wasm::name(to).to_string().c_str() );
        WASM_ASSERT(quantity.is_valid(), wasm_assert_exception, "%s", "invalid quantity");
        WASM_ASSERT(quantity.amount > 0, wasm_assert_exception, "%s", "must transfer positive quantity");
        WASM_ASSERT(memo.size() <= 256, wasm_assert_exception, "%s", "memo has more than 256 bytes");
        //check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

        //auto payer = context.has_authorization(to) ? to : from;
        CAccount from_account;
        WASM_ASSERT(database.GetAccount(nick_name(wasm::name(from).to_string()), from_account),
                    account_operation_exception,
                    "wasmio_bank_native_transfer.Transfer, from account does not exist, from Id = %s",
                    wasm::name(from).to_string().c_str())
        sub_balance( from_account, quantity, database );

        CAccount to_account;
        WASM_ASSERT(database.GetAccount(nick_name(wasm::name(to).to_string()), to_account),
                    account_operation_exception,
                    "wasmio_bank_native_transfer.Transfer, to account does not exist, to Id = %s",
                    wasm::name(to).to_string().c_str())
        add_balance( to_account, quantity, database );

        context.require_recipient(from);
        context.require_recipient(to);

    }



}