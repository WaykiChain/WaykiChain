#pragma GCC diagnostic ignored "-Wunused-variable"

#include "wasm/wasm_runtime.hpp"
#include "eosio/vm/watchdog.hpp"
#include "wasm/wasm_log.hpp"
#include "wasm/exception/exceptions.hpp"
#include "commons/util/util.h"

using namespace eosio;
using namespace eosio::vm;
namespace wasm {
    struct wasm_exit {
        int32_t code = 0;
    };

    wasm_instantiated_module_interface::~wasm_instantiated_module_interface() {}

    wasm_runtime_interface::~wasm_runtime_interface() {}

    template<typename Impl>
    class wasm_vm_instantiated_module : public wasm_instantiated_module_interface {
        using backend_t = backend<wasm::wasm_context_interface, Impl>;
    public:

        wasm_vm_instantiated_module(wasm_vm_runtime <Impl> *runtime, std::shared_ptr <backend_t> mod) :
                _runtime(runtime),
                _instantiated_module(std::move(mod)) {}

        void apply(wasm::wasm_context_interface *pContext) override {


            auto bm_wasm_init = MAKE_BENCHMARK("execute wasm vm -- init");
            //WASM_TRACE("receiver:%d contract:%d action:%d",pContext->receiver(), pContext->contract(), pContext->action() )
            _instantiated_module->set_wasm_allocator(pContext->get_wasm_allocator());
            _runtime->_bkend = _instantiated_module.get();
            _runtime->_bkend->initialize(pContext);
            // clamp WASM memory to maximum_linear_memory/wasm_page_size
            auto &module = _runtime->_bkend->get_module();
            if (module.memories.size() &&
                ((module.memories.at(0).limits.maximum >
                  wasm_constraints::maximum_linear_memory / wasm_constraints::wasm_page_size)
                 || !module.memories.at(0).limits.flags)) {
                module.memories.at(0).limits.flags = true;
                module.memories.at(0).limits.maximum =
                        wasm_constraints::maximum_linear_memory / wasm_constraints::wasm_page_size;
            }
            auto fn = [&]() {
                const auto &res = _runtime->_bkend->call(
                        pContext, "env", "apply", pContext->receiver(),
                        pContext->contract(),
                        pContext->action());
            };
            if (bm_wasm_init) bm_wasm_init->end();

            auto bm_wasm_run = MAKE_BENCHMARK("execute wasm vm -- run");
            try {
                watchdog wd(pContext->get_max_transaction_duration());
                _runtime->_bkend->timed_run(wd, fn);
            } catch (vm::timeout_exception &) {
                CHAIN_THROW(wasm_chain::wasm_timeout_exception, "timeout exception");
            } catch (vm::wasm_memory_exception &e) {
                WASM_TRACE("%s", "access violation")
                CHAIN_THROW(wasm_chain::wasm_memory_exception, "access violation");
            } catch ( vm::exception &e ) {
                // FIXME: Do better translation
                CHAIN_THROW(wasm_chain::wasm_execution_error, "something went wrong...");
            }
            _runtime->_bkend = nullptr;
        }

    private:
        wasm_vm_runtime <Impl> *    _runtime;
        std::shared_ptr <backend_t> _instantiated_module;
    };

    template<typename Impl>
    wasm_vm_runtime<Impl>::wasm_vm_runtime() {}

    template<typename Impl>
    void wasm_vm_runtime<Impl>::immediately_exit_currently_running_module() {
        throw wasm_exit{};
    }

    template<typename Impl>
    void wasm_vm_runtime<Impl>::validate(const vector <uint8_t> &code) {}

    template<typename Impl>
    std::shared_ptr <wasm_instantiated_module_interface>
    wasm_vm_runtime<Impl>::instantiate_module(const char *code_bytes, size_t code_size) {
        using backend_t = backend<wasm::wasm_context_interface, Impl>;
        try {
            wasm_code_ptr code((uint8_t *) code_bytes, code_size);
            std::shared_ptr <backend_t> bkend = std::make_shared<backend_t>(code, code_size);
            registered_host_functions<wasm_context_interface>::resolve(bkend->get_module());
            return std::make_shared<wasm_vm_instantiated_module<Impl>>(this, std::move(bkend));
        } catch (vm::exception &e) {
            CHAIN_THROW(wasm_chain::wasm_execution_error, "Error building eos-vm interp: %s", e.what());
        }
    }

    template
    class wasm_vm_runtime<vm::interpreter>;

    template
    class wasm_vm_runtime<vm::jit>;
}//wasm