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

}