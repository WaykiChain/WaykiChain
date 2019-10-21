#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#include "config/errorcode.h"
#include "wasm/wasm_native_contract.hpp"
#include "wasm/exceptions.hpp"
#include "wasm/datastream.hpp"
#include "wasm/types/asset.hpp"
#include "persistence/cachewrapper.h"

using namespace std;
using namespace wasm;

namespace wasm {

    void wasm_native_setcode(wasm_context &context) {

        CAccount sender;
        WASM_ASSERT(context.cache.accountCache.GetAccount(context.control_trx.txUid, sender),
                    account_operation_exception,
                    "wasmnativecontract.Setcode, sender account does not exist, sender Id = %s",
                    context.control_trx.txUid.ToString().c_str())

        WASM_ASSERT(sender.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, context.control_trx.llFees),
                    account_operation_exception,
                    "wasmnativecontract.Setcode, operate account failed ,regId=%s",
                    context.control_trx.txUid.ToString().c_str())

        WASM_ASSERT(context.cache.accountCache.SetAccount(CUserID(sender.keyid), sender), account_operation_exception,
                    "%s",
                    "wasmnativecontract.Setcode, save account info error")

        using SetCode = std::tuple<uint64_t, string, string, string>;
        SetCode setcode = wasm::unpack<SetCode>(context.trx.data);

        auto nick_name = std::get<0>(setcode);
        auto code = std::get<1>(setcode);
        auto abi = std::get<2>(setcode);
        auto memo = std::get<3>(setcode);

        CAccount account;
        CRegID contractRegId = wasm::Name2RegID(nick_name);
        // if (!context.cache.accountCache.GetAccount(contractRegId, account)){
        //   return;
        // }

        CUniversalContract contract;
        context.cache.contractCache.GetContract(contractRegId, contract);

        if (contract.vm_type != VMType::WASM_VM) contract.vm_type = VMType::WASM_VM;

        if (code.size() > 0) contract.code = code;
        if (abi.size() > 0) contract.abi = abi;
        if (memo.size() > 0) contract.memo = memo;

        WASM_ASSERT(context.cache.contractCache.SaveContract(contractRegId, contract), account_operation_exception,
                    "%s",
                    "wasmnativecontract.Setcode, save account info error")

        // CUniversalContract contractTest;
        // context.cache.contractCache.GetContract(contractRegId, contractTest);

        // std::cout << "WasmNativeSetcode ----------------------------"
        //           << "  contract:" << nick_name
        //           << " contractRegId:" << contractRegId.ToString()
        //           << " abi:"<< contractTest.abi
        //           // << " data:"<< params[3].get_str()
        //           << " \n";

        //context.control_trx.nRunStep = contract.GetContractSize();
        //if (!SaveTxAddresses(height, index, cw, state, {txUid})) return false;
    }


    void wasm_native_transfer(wasm_context &context) {

        using Transfer = std::tuple<uint64_t, uint64_t, wasm::asset, string>;
        Transfer transfer = wasm::unpack<Transfer>(context.trx.data);

        auto from = std::get<0>(transfer);
        auto to = std::get<1>(transfer);
        auto quantity = std::get<2>(transfer);
        auto memo = std::get<3>(transfer);

        WASM_ASSERT(from != to, wasm_assert_exception, "%s", "cannot transfer to self");
        context.require_auth(from);
        WASM_ASSERT(context.is_account(to), wasm_assert_exception, "%s", "to account does not exist");
        auto sym = quantity.sym.code();

        // currency_stats st;
        // stats statstable( _self, sym.raw() );
        // statstable.get( st, sym.raw() );

        context.require_recipient(from);
        context.require_recipient(to);

        WASM_ASSERT(quantity.is_valid(), wasm_assert_exception, "%s", "invalid quantity");
        WASM_ASSERT(quantity.amount > 0, wasm_assert_exception, "%s", "must transfer positive quantity");
        //check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
        WASM_ASSERT(memo.size() <= 256, wasm_assert_exception, "%s", "memo has more than 256 bytes");

        auto payer = context.has_authorization(to) ? to : from;
        // sub_balance( from, quantity );
        // add_balance( to, quantity, payer );

    }

}