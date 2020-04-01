#include<map>
#include"wasm/wasm_router.hpp"
#include"wasm/wasm_context.hpp"


namespace wasm {

	using action_handler = std::function<void(wasm_context &,  uint64_t)>;
    typedef router<action_handler> action_router; 

    using abi_handler = std::function<std::vector<char>()>;
    typedef router<abi_handler> abi_router;

}