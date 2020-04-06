#pragma once
#include<map>

namespace wasm {

  template<typename Handler>
	class router {
	    public:
			map <uint64_t, Handler> routes;
		public:
			router* add_router(uint64_t r, Handler h) {
				routes[r] = h;
				return this;
			}
			Handler* route(uint64_t r) {
				auto iter = routes.find(r);
				if(iter == routes.end()) return nullptr;
				return &iter->second;
			}
	};

    
	using action_handler_t = std::function<void(wasm_context &,  uint64_t)>;
    typedef router<action_handler_t> action_router; 

    using abi_handler_t = std::function<std::vector<char>()>;
    typedef router<abi_handler_t> abi_router;

	class native_module {
		public:
	        native_module()  {}
	        ~native_module() {}	
	        
	    public:	
	    	virtual void register_routes(abi_router& abi_r, action_router& act_r) = 0;
	};

  using transfer_data_t = std::tuple <uint64_t, uint64_t, wasm::asset, string >;
  using setcode_data_t = std::tuple<uint64_t, string, string, string>;	

}