#pragma once
#include "wasm/modules/wasm_handler.hpp"

namespace wasm {
	class native_module {
		public:
	        native_module()  {}
	        ~native_module() {}	
	        
	    public:	
	    	virtual void register_routes(abi_router& abi_r, action_router& act_r) = 0;
	};
}
