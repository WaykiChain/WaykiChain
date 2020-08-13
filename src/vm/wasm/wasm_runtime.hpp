#pragma once

#include <eosio/vm/backend.hpp>
#include "wasm/wasm_context_interface.hpp"


namespace wasm {

    class wasm_instantiated_module_interface {
       public:
          virtual int64_t apply(wasm_context_interface* context) = 0;
          virtual ~wasm_instantiated_module_interface();
    };

   class wasm_runtime_interface {
   public:
      virtual std::shared_ptr<wasm_instantiated_module_interface> instantiate_module(const char* code_bytes, size_t code_size, const uint256 &hash) = 0;
       virtual void immediately_exit_currently_running_module() = 0;
      virtual void validate(const vector <uint8_t> &code) = 0;

      virtual ~wasm_runtime_interface();
   };

    template<typename Backend>
    class wasm_vm_runtime : public wasm_runtime_interface{
    public:
        wasm_vm_runtime();
        std::shared_ptr <wasm_instantiated_module_interface> instantiate_module(const char *code_bytes, size_t code_size, const uint256 &hash) override;
        void immediately_exit_currently_running_module() override;
        void validate(const vector <uint8_t> &code) override;

    public:
        std::shared_ptr <module> get_instantiated_module(const vector <uint8_t> &code, const uint256 &hash);

    public:
        backend <wasm::wasm_context_interface, Backend> *_bkend = nullptr;  // non owning pointer to allow for immediate exit
    };

    template<typename Backend>
    class wasm_vm_runtime2 : public wasm_runtime_interface{
    public:
        wasm_vm_runtime2();
        std::shared_ptr <wasm_instantiated_module_interface> instantiate_module(const char *code_bytes, size_t code_size, const uint256 &hash) override;
        void immediately_exit_currently_running_module() override;
        void validate(const vector <uint8_t> &code) override;

    public:
        std::shared_ptr <module> get_instantiated_module(const char *code_bytes, size_t code_size, const uint256 &hash);

    public:
        backend2 <wasm::wasm_context_interface, Backend> *_bkend = nullptr;  // non owning pointer to allow for immediate exit
    };



} //wasm

#define __INTRINSIC_NAME(LBL, SUF) LBL##SUF
#define _INTRINSIC_NAME(LBL, SUF) __INTRINSIC_NAME(LBL, SUF)
#define REGISTER_WASM_VM_INTRINSIC(CLS, MOD, METHOD, NAME) \
   vm::registered_function<wasm::wasm_context_interface, CLS, &CLS::METHOD> _INTRINSIC_NAME(__wasm_vm_intrinsic_fn, __COUNTER__)(std::string(#MOD), std::string(#NAME));

