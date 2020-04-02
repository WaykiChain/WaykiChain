#pragma once
#include "wasm/wasm_context.hpp"
#include "wasm/wasm_constants.hpp"
#include "wasm/abi_def.hpp"

#include "wasm/exception/exceptions.hpp"
#include "wasm/wasm_log.hpp"
#include "wasm/modules/wasm_router.hpp"
#include "wasm/modules/wasm_native_commons.hpp"
#include "wasm/types/asset.hpp"
#include "wasm/types/regid.hpp"
#include "entities/account.h"
#include "entities/receipt.h"

namespace wasm {

	static uint64_t native_module_id = wasmio;//REGID(800-1); 

	class wasm_native_module: public native_module {

		//uint64_t module_id = REGID(800-1); 
		public:
	        wasm_native_module()  {}
	        ~wasm_native_module() {}	
	        
	    public:	
	    	virtual void register_routes(abi_router& abi_r, action_router& act_r){
	    		abi_r.add_router(native_module_id, abi_handler);
                act_r.add_router(native_module_id, act_handler);
	    	}

	    public:
		  	static void act_handler(wasm_context &context, uint64_t action){
	    		switch (action){
	    			case N(setcode):
	    			     WASM_TRACE("%s", wasm::name(action).to_string())
	    			     setcode(context);
	    			     return;
	    			     break;
	    			default:
	    			     break;
	    		}


	    		WASM_TRACE("%s", wasm::name(N(setcode)).to_string())

	            CHAIN_ASSERT( false,
	                      wasm_chain::action_not_found_exception,
	                      "handler '%s' does not exist in native contract '%s'",
	                      wasm::name(action).to_string(),
	                      wasm::regid(native_module_id).to_string())
	  	     };

		    static std::vector<char> abi_handler() {
		        abi_def abi;

		        if (abi.version.size() == 0) {
		            abi.version = "wasm::abi/1.0";
		        }

		        abi.structs.emplace_back(struct_def{
		                "setcode", "", {
		                        {"account", "name"  },
		                        {"code",    "bytes" },
		                        {"abi",     "bytes" },
		                        {"memo",    "string"}
		                }
		        });

		        abi.actions.push_back(action_def{"setcode", "setcode", ""});

		        auto abi_bytes = wasm::pack<wasm::abi_def>(abi);
		        return abi_bytes;

		    }

		    static void setcode(wasm_context &context) {

		         CHAIN_ASSERT( context._receiver == native_module_id, 
		                       wasm_chain::native_contract_assert_exception, 
		                       "expect contract '%s', but get '%s'", 
		       		           wasm::regid(native_module_id).to_string(),
		                       wasm::regid(context._receiver).to_string());

		        auto &database_account         = context.database.accountCache;
		        auto &database_contract        = context.database.contractCache;
		        //auto &control_trx              = context.control_trx;
		        
		        //set_code_data_type set_code_data = wasm::unpack<std::tuple<uint64_t, string, string, string>>(context.trx.data);
		        auto set_code_data = wasm::unpack<std::tuple<uint64_t, string, string, string>>(context.trx.data);
		        auto contract_name               = std::get<0>(set_code_data);
		        auto code                        = std::get<1>(set_code_data);
		        auto abi                         = std::get<2>(set_code_data);
		        auto memo                        = std::get<3>(set_code_data);

		        context.require_auth(contract_name); 

		        CAccount contract;
		        CHAIN_ASSERT( database_account.GetAccount(CRegID(contract_name), contract),
		                      wasm_chain::account_access_exception,
		                      "contract '%s' does not exist",
		                      wasm::regid(contract_name).to_string()) 

		        CUniversalContract contract_store;
		        contract_store.vm_type = VMType::WASM_VM;
		        contract_store.code    = code;
		        contract_store.abi     = abi;
		        contract_store.memo    = memo;

		        CHAIN_ASSERT( database_contract.SaveContract(contract.regid, contract_store), 
		                      wasm_chain::account_access_exception,
		                      "save account '%s' error",
		                      wasm::regid(contract_name).to_string())
		    }

	};
}
