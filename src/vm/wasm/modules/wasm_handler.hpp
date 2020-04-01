#pragma once
#include<map>
#include"wasm/wasm_router.hpp"
#include"wasm/wasm_context.hpp"


namespace wasm {

	using action_handler_t = std::function<void(wasm_context &,  uint64_t)>;
    typedef router<action_handler_t> action_router; 

    using abi_handler_t = std::function<std::vector<char>()>;
    typedef router<abi_handler_t> abi_router;

}
