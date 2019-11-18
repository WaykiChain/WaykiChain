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

         WASM_ASSERT(context._receiver == wasmio, wasm_assert_exception, "exception comtract wasmio,but get %s", wasm::name(context._receiver).to_string().c_str());


        auto &database_account         = context.database.accountCache;
        auto &database_contract        = context.database.contractCache;
        auto &control_trx              = context.control_trx;

        //charger fee should move to tx.execute
        // CAccount sender;
        // WASM_ASSERT(database_account.GetAccount(control_trx.txUid, sender),
        //             account_operation_exception,
        //             "wasmnativecontract.Setcode, sender account does not exist, sender Id = %s",
        //             control_trx.txUid.ToString().c_str())  
        // auto quantity = wasm::asset(control_trx.llFees, wasm::symbol(SYMB::WICC, 0)); 
        // sub_balance(sender, quantity, context);

        //set contract code and abi
        std::tuple<uint64_t, string, string, string> set_code = wasm::unpack<std::tuple<uint64_t, string, string, string>>(context.trx.data);
        auto contract_name = wasm::name(std::get<0>(set_code));
        auto code          = std::get<1>(set_code);
        auto abi           = std::get<2>(set_code);
        auto memo          = std::get<3>(set_code);

        context.require_auth(contract_name.value); 

        CAccount contract;
        WASM_ASSERT(database_account.GetAccount(nick_name(contract_name.to_string()), contract),
                    account_operation_exception,
                    "wasmnativecontract.Setcode, contract account does not exist, contract = %s",
                    contract_name.to_string().c_str()) 

        CUniversalContract contract_store;
        // ban reset code
        // WASM_ASSERT(!cw.contractCache.GetContract(contract.regid, contract_store),
        //             account_operation_exception,
        //             "wasmnativecontract.Setcode, can not reset code, contract = %s",
        //             contract_name.c_str()) 
        contract_store.vm_type = VMType::WASM_VM;
        contract_store.code = code;
        contract_store.abi  = abi;
        contract_store.memo = memo;

        WASM_ASSERT(database_contract.SaveContract(contract.regid, contract_store), account_operation_exception,
                    "%s",
                    "wasmnativecontract.Setcode, save account info error")
    }
    
    void wasmio_bank_native_transfer(wasm_context &context) {

        WASM_ASSERT(context._receiver == wasmio_bank, wasm_assert_exception, "exception comtract wasmio_bank,but get %s", wasm::name(context._receiver).to_string().c_str());

        auto &database = context.database.accountCache;

        std::tuple<uint64_t, uint64_t, wasm::asset, string> transfer = wasm::unpack<std::tuple<uint64_t, uint64_t, wasm::asset, string>>(context.trx.data);
        auto from     = std::get<0>(transfer);
        auto to       = std::get<1>(transfer);
        auto quantity = std::get<2>(transfer);
        auto memo     = std::get<3>(transfer);

        // wasm::abi_def wasmio_abi = wasmio_contract_abi();
        // std::vector<char> abi = wasm::pack<wasm::abi_def>(wasmio_abi);
        // json_spirit::Value val = wasm::abi_serializer::unpack(abi, "transfer", context.trx.data,
        //                                    max_serialization_time);
        //WASM_TRACE("%s", json_spirit::write_formatted(val).c_str());

        WASM_ASSERT(from != to, wasm_assert_exception, "%s", "cannot transfer to self");
        context.require_auth(from); //from auth
        WASM_ASSERT(context.is_account(to), wasm_assert_exception, "%s", "to account does not exist");
        auto sym = quantity.sym.code();

        context.require_recipient(from);
        context.require_recipient(to);

        WASM_ASSERT(quantity.is_valid(), wasm_assert_exception, "%s", "invalid quantity");
        WASM_ASSERT(quantity.amount > 0, wasm_assert_exception, "%s", "must transfer positive quantity");
        //check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
        WASM_ASSERT(memo.size() <= 256, wasm_assert_exception, "%s", "memo has more than 256 bytes");

        //auto payer = context.has_authorization(to) ? to : from;
        CAccount from_account;
        WASM_ASSERT(database.GetAccount(nick_name(wasm::name(from).to_string()), from_account),
                    account_operation_exception,
                    "wasmnativecontract.Setcode, sender account does not exist, sender Id = %s",
                    from_account.nickid.ToString().c_str())
        sub_balance( from_account, quantity, database );


        CAccount to_account;
        WASM_ASSERT(database.GetAccount(nick_name(wasm::name(to).to_string()), to_account),
                    account_operation_exception,
                    "wasmnativecontract.Setcode, sender account does not exist, sender Id = %s",
                    to_account.nickid.ToString().c_str())
        add_balance( to_account, quantity, database );

    }



}