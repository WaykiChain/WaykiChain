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

	static uint64_t native_module_id = wasmio;//REGID(0-100);

	class wasm_native_module: public native_module {
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
	    			     setcode(context);
	    			     return;
					case N(setcoder):
	    			     setcoder(context);
	    			     return;
	    			default:
	    			     break;
	    		}

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
		                        {"contract", 	"regid"  },
								{"maintainer", 	"regid"  }, //optional for subsequent code update
								{"vmtype",		"uint8"},
		                        {"code",    	"bytes"  },
		                        {"abi",     	"bytes"  },
		                        {"memo",    	"string" }
		                }
		        });

				abi.structs.emplace_back(struct_def{
		                "setcoder", "", {
		                        {"contract", 	"regid"  },
								{"maintainer", 	"regid"  }
			            }
		        });

		        abi.actions.emplace_back("setcode",  "setcode",  "");
				abi.actions.emplace_back("setcoder", "setcoder", "");

		        auto abi_bytes = wasm::pack<wasm::abi_def>(abi);
		        return abi_bytes;

		    }

		    static void setcode(wasm_context &context) {

		         CHAIN_ASSERT( context._receiver == native_module_id,
		                       wasm_chain::native_contract_assert_exception,
		                       "expect contract '%s', but get '%s'",
		       		           wasm::regid(native_module_id).to_string(),
		                       wasm::regid(context._receiver).to_string());

		        auto &db_account   				= context.database.accountCache;
				auto &db_contract   			= context.database.contractCache;

		        auto set_code_data  			= wasm::unpack<std::tuple<regid, regid, uint8_t,
													string, string, string>>(context.trx.data);

		        auto contract_regid 			= std::get<0>(set_code_data);
				auto maintainer_regid   		= std::get<1>(set_code_data);
				auto vm_type 					= std::get<2>(set_code_data);
		        auto code           			= std::get<3>(set_code_data);
		        auto abi            			= std::get<4>(set_code_data);
		        auto memo           			= std::get<5>(set_code_data);

				CUniversalContractStore contractStore;
				uint64_t maintainer = maintainer_regid.value;
				CRegID contractRegId = CRegID(contract_regid.value);

				if (contract_regid.value == 0) {  //first-time deployment
					contractRegId = context.trx_cord;

					CHAIN_ASSERT( !db_contract.HasContract(contractRegId),
				 			  	wasm_chain::contract_exception,
		                      	"contract '%s' exists error",
		                      	contractRegId.ToString())

					contractStore.vm_type = (VMType) vm_type;
					contractStore.upgradable = true;

					// create a new contract account
					auto keyid = Hash160(contractRegId.GetRegIdRaw());
					auto spContractAccount = context.control_trx.NewAccount(context.database, keyid);
					spContractAccount->regid = contractRegId;
					CHAIN_ASSERT( db_account.NewRegId(spContractAccount->regid, spContractAccount->keyid),
								wasm_chain::contract_exception,
								"contract registers account (%s) error",
								contractRegId.ToString())

 				} else {					//upgrade contract
					CHAIN_ASSERT( db_contract.GetContract(contractRegId, contractStore),
				 			  	wasm_chain::contract_exception,
		                      	"contract '%s' not exist error",
		                      	contractRegId.ToString())

					//must be current maintainer to perform code upgrade
					auto currMaintainer = contractStore.maintainer.GetIntValue();
					CHAIN_ASSERT( maintainer == currMaintainer,
								wasm_chain::contract_exception,
		                      	"maintainer mismatch: given: %llu vs. curr: %llu",
							  	maintainer, currMaintainer);

					context.require_auth(currMaintainer);
				}

    			contractStore.maintainer 			= maintainer;
				contractStore.code 	 				= code;
				contractStore.abi  	 				= abi;
				contractStore.memo 	 				= memo;
				contractStore.code_hash 			= HashOnce(code.data(), code.size());

		        CHAIN_ASSERT( db_contract.SaveContract(contractRegId, contractStore),
		                      wasm_chain::contract_exception,
		                      "save contract '%s' error",
		                      contractRegId.ToString())

		    }

			static void setcoder(wasm_context &context) {

		         CHAIN_ASSERT( context._receiver == native_module_id,
		                       wasm_chain::native_contract_assert_exception,
		                       "expect contract '%s', but get '%s'",
		       		           wasm::regid(native_module_id).to_string(),
		                       wasm::regid(context._receiver).to_string());

		        auto &db_contract   	= context.database.contractCache;

		        auto set_code_data  	= wasm::unpack<std::tuple<regid, regid>>(context.trx.data);

		        regid contract_regid 	= std::get<0>(set_code_data);
				regid maintainer_regid 	= std::get<1>(set_code_data);

				CUniversalContractStore contractStore;

				CHAIN_ASSERT( db_contract.GetContract(CRegID(contract_regid.value), contractStore),
							  wasm_chain::native_contract_assert_exception,
		                      "contract store '%s' not exist",
		                      wasm::regid(contract_regid).to_string())

				//must be current maintainer to set a new maintainer
				auto currMaintainer = contractStore.maintainer.GetIntValue();
				context.require_auth(currMaintainer);

				//set new maintainer
				contractStore.maintainer.Set(maintainer_regid.value);
		        CHAIN_ASSERT( db_contract.SaveContract(CRegID(contract_regid.value), contractStore),
		                      wasm_chain::native_contract_assert_exception,
		                      "save contract '%s' error",
		                      wasm::regid(contract_regid).to_string())

		    }

	};
}
