#pragma once
#include "wasm/wasm_context.hpp"
#include "wasm/wasm_constants.hpp"
#include "wasm/abi_def.hpp"

#include "wasm/exception/exceptions.hpp"
#include "wasm/wasm_log.hpp"
#include "wasm/wasm_router.hpp"

#include "wasm/types/asset.hpp"
#include "wasm/types/regid.hpp"
#include "entities/account.h"
#include "entities/receipt.h"

#include "wasm/modules/wasm_module.hpp"
#include "wasm/modules/wasm_handler.hpp"
#include "wasm/modules/wasm_native_lib.hpp"

namespace wasm {

	static uint64_t bank_native_module_id = wasmio_bank;//REGID(800-2); 

	class wasm_bank_native_module: public native_module {

		public:
	        wasm_bank_native_module()  {}
	        ~wasm_bank_native_module() {}	
	        
	    public:	
	    	void register_routes(abi_router& abi_r, action_router& act_r){
	    		abi_r.add_router(bank_native_module_id, abi_handler);
                act_r.add_router(bank_native_module_id, act_handler);
	    	}

	    public:
		  	static void act_handler(wasm_context &context, uint64_t action){
		        switch (action){
		            case N(transfer):
		                 tansfer(context);
		                 return;
		                 break;
		            default:
		                 break;
		        }

		        CHAIN_ASSERT( false,
		                      wasm_chain::action_not_found_exception,
		                      "handler '%s' does not exist in native contract '%s'",
		                      wasm::name(action).to_string(),
		                      wasm::regid(bank_native_module_id).to_string())
	  	     };

		    static std::vector<char> abi_handler() {
		        abi_def abi;

		        if (abi.version.size() == 0) {
		            abi.version = "wasm::abi/1.0";
		        }

		        abi.structs.emplace_back(struct_def{
		            "transfer", "", {
		                    {"from",     "regid"  },
		                    {"to",       "regid"  },
		                    {"quantity", "asset" },
		                    {"memo",     "string"}
		            }
		        });

		        abi.actions.push_back(action_def{"transfer", "transfer", ""});

		        auto abi_bytes = wasm::pack<wasm::abi_def>(abi);
		        return abi_bytes;
		    }

		    static void tansfer(wasm_context &context) {
		    	
		        CHAIN_ASSERT( context._receiver == bank_native_module_id,
		                      wasm_chain::native_contract_assert_exception, 
		                      "expect contract '%s', but get '%s'", 
		                      wasm::regid(bank_native_module_id).to_string(),
		                      wasm::name(context._receiver).to_string());

		        auto &database                = context.database.accountCache;
		        context.control_trx.run_cost += context.trx.GetSerializeSize(SER_DISK, CLIENT_VERSION) * store_fuel_fee_per_byte;

		        //transfer_data_type transfer_data = wasm::unpack<std::tuple<uint64_t, uint64_t, wasm::asset, string>>(context.trx.data);
		        auto transfer_data = wasm::unpack<std::tuple <uint64_t, uint64_t, wasm::asset, string >>(context.trx.data);
		        auto from                        = std::get<0>(transfer_data);
		        auto to                          = std::get<1>(transfer_data);
		        auto quantity                    = std::get<2>(transfer_data);
		        auto memo                        = std::get<3>(transfer_data);

		        //WASM_TRACE("%s", quantity.to_string().c_str() )
		        context.require_auth(from); //from auth
		        CHAIN_ASSERT(from != to,             wasm_chain::native_contract_assert_exception, "cannot transfer to self");
		        CHAIN_ASSERT(context.is_account(to), wasm_chain::native_contract_assert_exception, "to account '%s' does not exist", wasm::name(to).to_string() );
		        CHAIN_ASSERT(quantity.is_valid(),    wasm_chain::native_contract_assert_exception, "invalid quantity");
		        CHAIN_ASSERT(quantity.amount > 0,    wasm_chain::native_contract_assert_exception, "must transfer positive quantity");
		        CHAIN_ASSERT(memo.size()  <= 256,    wasm_chain::native_contract_assert_exception, "memo has more than 256 bytes");

		        CAccount from_account;
		        CHAIN_ASSERT( database.GetAccount(CRegID(from), from_account),
		                      wasm_chain::native_contract_assert_exception,
		                      "from account '%s' does not exist",
		                      wasm::regid(from).to_string())
		        sub_balance( from_account, quantity, database, context.control_trx.receipts );

		        CAccount to_account;
		        CHAIN_ASSERT( database.GetAccount(CRegID(to), to_account),
		                      wasm_chain::native_contract_assert_exception,
		                      "to account '%s' does not exist",
		                      wasm::regid(to).to_string())
		        add_balance( to_account, quantity, database, context.control_trx.receipts   );

		        context.require_recipient(from);
		        context.require_recipient(to);

		    }
	};
}
