#include <eosio/vm/backend.hpp>
#include <eosio/vm/error_codes.hpp>

#include "wasm/wasm_host_methods.hpp"
#include "wasm/wasm_interface.hpp"
#include "wasm/exceptions.hpp"

#include "crypto/hash.h"

using namespace eosio;
using namespace eosio::vm;

namespace wasm {
    using code_version       = uint256;
    using backend_validate_t = backend<wasm::wasm_context_interface, vm::interpreter>;
    using rhf_t              = eosio::vm::registered_host_functions<wasm_context_interface>;
    std::map <code_version, std::shared_ptr<wasm_instantiated_module_interface>> wasm_instantiation_cache;
    std::shared_ptr <wasm_runtime_interface> runtime_interface;

    wasm_interface::wasm_interface() {}
    wasm_interface::~wasm_interface() {}

    void wasm_interface::exit() {
        runtime_interface->immediately_exit_currently_running_module();
    }

    std::shared_ptr <wasm_instantiated_module_interface> get_instantiated_backend(const vector <uint8_t> &code) {

        try {
            auto code_id = Hash(code.begin(), code.end());
            auto it = wasm_instantiation_cache.find(code_id);
            if (it == wasm_instantiation_cache.end()) {
                wasm_instantiation_cache[code_id] = runtime_interface->instantiate_module((const char*)code.data(), code.size());
                return wasm_instantiation_cache[code_id];
            }
            return it->second;
        } catch (...) {
            throw;
        }

    }

    void wasm_interface::execute(const vector <uint8_t> &code, wasm_context_interface *pWasmContext) {
        std::shared_ptr <wasm_instantiated_module_interface> pInstantiated_module = get_instantiated_backend(code);

        system_clock::time_point start = system_clock::now();
        pInstantiated_module->apply(pWasmContext);
        system_clock::time_point end = system_clock::now();
        std::cout << std::string("wasm duration:")
                  << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

    }

    void wasm_interface::validate(const vector <uint8_t> &code) {

        try {
             auto code_bytes = (uint8_t*)code.data();
             size_t code_size = code.size();
             wasm_code_ptr code_ptr(code_bytes, code_size);
             auto bkend = backend_validate_t(code_ptr, code_size);
             rhf_t::resolve(bkend.get_module()); 
        } catch (vm::exception &e) {
             WASM_THROW(wasm_exception, "%s", e.detail())
        }
        WASM_RETHROW_EXCEPTIONS(wasm_exception, "%s", "wasm code parse exception")

    }

    void wasm_interface::initialize(vm_type vm) {

        if (vm == wasm::vm_type::eos_vm)
            runtime_interface = std::make_shared<wasm::wasm_vm_runtime<vm::interpreter>>();
        else if (vm == wasm::vm_type::eos_vm_jit)
            runtime_interface = std::make_shared<wasm::wasm_vm_runtime<vm::jit>>();
        else
            runtime_interface = std::make_shared<wasm::wasm_vm_runtime<vm::interpreter>>();

    }

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, abort, abort)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, wasm_assert, wasm_assert)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, wasm_assert_code, wasm_assert_code)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, current_time, current_time)

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, read_action_data, read_action_data)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, action_data_size, action_data_size)

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, current_receiver, current_receiver)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, db_store, db_store)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, db_remove, db_remove)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, db_get, db_get)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, db_update, db_update)

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, memcpy, memcpy)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, memmove, memmove)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, memcmp, memcmp)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, memset, memset) 
    
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, printn, printn)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, printui, printui)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, printi, printi)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, prints, prints)     
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, prints_l, prints_l)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, printi128, printi128)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, printui128, printui128)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, printsf, printsf) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, printdf, printdf)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, printhex, printhex)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, printqf, printqf) 

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, has_authorization, has_auth)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, require_auth, require_auth)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, require_recipient, require_recipient) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, is_account, is_account)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, send_inline, send_inline) 

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __ashlti3, __ashlti3)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __ashrti3, __ashrti3)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __lshlti3, __lshlti3) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __lshrti3, __lshrti3)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __divti3, __divti3) 

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __udivti3, __udivti3)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __multi3, __multi3)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __modti3, __modti3) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __umodti3, __umodti3)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __addtf3, __addtf3)  

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __subtf3, __subtf3)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __multf3, __multf3)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __divtf3, __divtf3) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __negtf2, __negtf2)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __extendsftf2, __extendsftf2)  

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __extenddftf2, __extenddftf2)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __trunctfdf2, __trunctfdf2)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __trunctfsf2, __trunctfsf2) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __fixtfsi, __fixtfsi)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __fixtfdi, __fixtfdi) 

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __fixtfti, __fixtfti)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __fixunstfsi, __fixunstfsi)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __fixunstfdi, __fixunstfdi) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __fixunstfti, __fixunstfti)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __fixsfti, __fixsfti)  

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __fixdfti, __fixdfti)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __fixunssfti, __fixunssfti)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __fixunsdfti, __fixunsdfti) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __floatsidf, __floatsidf)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __floatsitf, __floatsitf)    

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __floatditf, __floatditf)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __floatunsitf, __floatunsitf)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __floatunditf, __floatunditf) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __floattidf, __floattidf)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __floatuntidf, __floatuntidf) 

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __eqtf2, __eqtf2)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __netf2, __netf2)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __getf2, __getf2) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __gttf2, __gttf2)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __letf2, __letf2)   
    
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __lttf2, __lttf2) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __cmptf2, __cmptf2)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __unordtf2, __unordtf2) 

}//wasm
