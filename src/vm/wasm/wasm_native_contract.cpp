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

    inline CRegID get_regid(uint64_t id){

        string id_str = wasm::name(id).to_string();
        string::size_type pos =  id_str.find(".");
        if( pos!= string::npos ) id_str.replace(pos, 1, "-");  
        CRegID regid(id_str);
        return regid;
    }


    inline void sub_balance(const uint64_t& from, wasm::asset quantity, wasm_context &context){

        CCacheWrapper &cw       = context.cache;
        //CValidationState &state = *context.pState;
        
        CRegID regid = Name2RegID(from);
        CAccount account;
        WASM_ASSERT(cw.accountCache.GetAccount(regid, account),
                    account_operation_exception,
                    "wasmnativecontract.sub_balance, from account does not exist, sender Id = %s",
                    regid.ToString().c_str()) 

        string symbol = quantity.sym.code().to_string();
        uint8_t precision = quantity.sym.precision();
        WASM_ASSERT(precision == 0,
                    account_operation_exception,
                    "wasmnativecontract.sub_balance, the precision of system coin %s must be %d",
                    symbol,
                    0) 

        uint64_t amount = quantity.amount;
        WASM_ASSERT(account.OperateBalance(symbol, BalanceOpType::SUB_FREE, amount),
                    account_operation_exception,
                    "wasmnativecontract.sub_balance, operate account failed ,regId=%s",
                    regid.ToString().c_str())  

        WASM_ASSERT(cw.accountCache.SetAccount(CUserID(account.keyid), account), account_operation_exception,
                    "%s",
                    "wasmnativecontract.Setcode, save account info error")     
    }

    inline void add_balance(const uint64_t& to, wasm::asset quantity, wasm_context &context){
         CCacheWrapper &cw       = context.cache;
        //CValidationState &state = *context.pState;
        
        CRegID regid = Name2RegID(to);
        CAccount account;
        WASM_ASSERT(cw.accountCache.GetAccount(regid, account),
                    account_operation_exception,
                    "wasmnativecontract.add_balance, from account does not exist, sender Id = %s",
                    regid.ToString().c_str()) 

        string symbol = quantity.sym.code().to_string();
        uint8_t precision = quantity.sym.precision();
        WASM_ASSERT(precision == 0,
                    account_operation_exception,
                    "wasmnativecontract.add_balance, the precision of system coin %s must be %d",
                    symbol,
                    0) 

        uint64_t amount = quantity.amount;
        WASM_ASSERT(account.OperateBalance(symbol, BalanceOpType::ADD_FREE, amount),
                    account_operation_exception,
                    "wasmnativecontract.add_balance, operate account failed ,regId=%s",
                    regid.ToString().c_str()) 

        WASM_ASSERT(cw.accountCache.SetAccount(CUserID(account.keyid), account), account_operation_exception,
                    "%s",
                    "wasmnativecontract.Setcode, save account info error")          
    }

    void wasm_native_setcode(wasm_context &context) {

        CCacheWrapper   &cw            = context.cache;
        CWasmContractTx &control_trx   = context.control_trx;

        CAccount sender;
        WASM_ASSERT(cw.accountCache.GetAccount(control_trx.txUid, sender),
                    account_operation_exception,
                    "wasmnativecontract.Setcode, sender account does not exist, sender Id = %s",
                    control_trx.txUid.ToString().c_str())

        WASM_ASSERT(sender.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, control_trx.llFees),
                    account_operation_exception,
                    "wasmnativecontract.Setcode, operate account failed ,regId=%s",
                    control_trx.txUid.ToString().c_str())

        WASM_ASSERT(cw.accountCache.SetAccount(CUserID(sender.keyid), sender), account_operation_exception,
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
        // if (!cw.accountCache.GetAccount(contractRegId, account)){
        //   return;
        // }

        CUniversalContract contract;
        cw.contractCache.GetContract(contractRegId, contract);

        if (contract.vm_type != VMType::WASM_VM) contract.vm_type = VMType::WASM_VM;

        if (code.size() > 0) contract.code = code;
        if (abi.size() > 0) contract.abi = abi;
        if (memo.size() > 0) contract.memo = memo;

        WASM_ASSERT(cw.contractCache.SaveContract(contractRegId, contract), account_operation_exception,
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

        wasm::abi_def wasmio_abi = wasmio_contract_abi();
        std::vector<char> abi = wasm::pack<wasm::abi_def>(wasmio_abi);
        json_spirit::Value val = wasm::abi_serializer::unpack(abi, "transfer", context.trx.data,
                                           max_serialization_time);

        //WASM_TRACE("%s", json_spirit::write_formatted(val).c_str());
        auto from = std::get<0>(transfer);
        auto to = std::get<1>(transfer);
        auto quantity = std::get<2>(transfer);
        auto memo = std::get<3>(transfer);

        WASM_TRACE("from %s, to %s", get_regid(from).ToString().c_str(), get_regid(from).ToString().c_str());
        from = wasm::RegID2Name(get_regid(from));
        to = wasm::RegID2Name(get_regid(to));

        WASM_ASSERT(from != to, wasm_assert_exception, "%s", "cannot transfer to self");
        context.require_auth(from); //from auth
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

        //auto payer = context.has_authorization(to) ? to : from;

        sub_balance( from, quantity, context );
        add_balance( to, quantity, context );

        //WASM_TRACE("%s", "wasm_native_transfer");

    }



}