#include <eosio/vm/backend.hpp>
#include <eosio/vm/error_codes.hpp>
#include <eosio/vm/host_function.hpp>
#include <eosio/vm/watchdog.hpp>

#include "wasm_interface.hpp"
#include "wasm_host_methods.hpp"
#include "types/name.hpp"
#include "exceptions.hpp"

#include<chrono>

using namespace eosio;
using namespace eosio::vm;

using std::chrono::system_clock;

namespace wasm {
    CWasmInterface::CWasmInterface() {}

//CWasmInterface::CWasmInterface(vmType type){}
    CWasmInterface::~CWasmInterface() {}
//CWasmInterface::exit() {}
    using backend_t = eosio::vm::backend<WasmHostMethods>;
    using rhf_t     = eosio::vm::registered_host_functions<WasmHostMethods>;

    void CWasmInterface::Initialize( vmType type ) {
        rhf_t::add<WasmHostMethods, &WasmHostMethods::abort, wasm_allocator>("env", "abort");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::wasm_assert, wasm_allocator>("env", "wasm_assert");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::wasm_assert_code, wasm_allocator>("env", "wasm_assert_code");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::current_time, wasm_allocator>("env", "current_time");

        rhf_t::add<WasmHostMethods, &WasmHostMethods::read_action_data, wasm_allocator>("env", "read_action_data");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::action_data_size, wasm_allocator>("env", "action_data_size");

        rhf_t::add<WasmHostMethods, &WasmHostMethods::current_receiver, wasm_allocator>("env", "current_receiver");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::db_store, wasm_allocator>("env", "db_store");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::db_remove, wasm_allocator>("env", "db_remove");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::db_get, wasm_allocator>("env", "db_get");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::db_update, wasm_allocator>("env", "db_update");

        rhf_t::add<WasmHostMethods, &WasmHostMethods::memcpy, wasm_allocator>("env", "memcpy");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::memmove, wasm_allocator>("env", "memmove");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::memcmp, wasm_allocator>("env", "memcmp");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::memset, wasm_allocator>("env", "memset");

        rhf_t::add<WasmHostMethods, &WasmHostMethods::printn, wasm_allocator>("env", "printn");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::printui, wasm_allocator>("env", "printui");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::printi, wasm_allocator>("env", "printi");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::prints, wasm_allocator>("env", "prints");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::prints_l, wasm_allocator>("env", "prints_l");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::printi128, wasm_allocator>("env", "printi128");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::printui128, wasm_allocator>("env", "printui128");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::printsf, wasm_allocator>("env", "printsf");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::printdf, wasm_allocator>("env", "printdf");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::printhex, wasm_allocator>("env", "printhex");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::printqf, wasm_allocator>("env", "printqf");

        rhf_t::add<WasmHostMethods, &WasmHostMethods::has_authorization, wasm_allocator>("env", "has_auth");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::require_auth, wasm_allocator>("env", "require_auth");
        // rhf_t::add<WasmHostMethods, &WasmHostMethods::require_auth2, wasm_allocator>("env", "require_auth2");

        rhf_t::add<WasmHostMethods, &WasmHostMethods::require_recipient, wasm_allocator>("env", "require_recipient");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::is_account, wasm_allocator>("env", "is_account");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::send_inline, wasm_allocator>("env", "send_inline");

        rhf_t::add<WasmHostMethods, &WasmHostMethods::__ashlti3, wasm_allocator>("env", "__ashlti3");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__ashrti3, wasm_allocator>("env", "__ashrti3");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__lshlti3, wasm_allocator>("env", "__lshlti3");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__lshrti3, wasm_allocator>("env", "__lshrti3");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__divti3, wasm_allocator>("env", "__divti3");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__udivti3, wasm_allocator>("env", "__udivti3");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__multi3, wasm_allocator>("env", "__multi3");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__modti3, wasm_allocator>("env", "__modti3");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__umodti3, wasm_allocator>("env", "__umodti3");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__addtf3, wasm_allocator>("env", "__addtf3");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__subtf3, wasm_allocator>("env", "__subtf3");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__multf3, wasm_allocator>("env", "__multf3");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__divtf3, wasm_allocator>("env", "__divtf3");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__negtf2, wasm_allocator>("env", "__negtf2");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__extendsftf2, wasm_allocator>("env", "__extendsftf2");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__extenddftf2, wasm_allocator>("env", "__extenddftf2");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__trunctfdf2, wasm_allocator>("env", "__trunctfdf2");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__trunctfsf2, wasm_allocator>("env", "__trunctfsf2");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__fixtfsi, wasm_allocator>("env", "__fixtfsi");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__fixtfdi, wasm_allocator>("env", "__fixtfdi");

        rhf_t::add<WasmHostMethods, &WasmHostMethods::__fixtfti, wasm_allocator>("env", "__fixtfti");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__fixunstfsi, wasm_allocator>("env", "__fixunstfsi");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__fixunstfdi, wasm_allocator>("env", "__fixunstfdi");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__fixunstfti, wasm_allocator>("env", "__fixunstfti");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__fixsfti, wasm_allocator>("env", "__fixsfti");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__fixdfti, wasm_allocator>("env", "__fixdfti");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__fixunssfti, wasm_allocator>("env", "__fixunssfti");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__fixunsdfti, wasm_allocator>("env", "__fixunsdfti");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__floatsidf, wasm_allocator>("env", "__floatsidf");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__floatsitf, wasm_allocator>("env", "__floatsitf");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__floatditf, wasm_allocator>("env", "__floatditf");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__floatunsitf, wasm_allocator>("env", "__floatunsitf");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__floatunditf, wasm_allocator>("env", "__floatunditf");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__floattidf, wasm_allocator>("env", "__floattidf");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__floatuntidf, wasm_allocator>("env", "__floatuntidf");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__eqtf2, wasm_allocator>("env", "__eqtf2");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__netf2, wasm_allocator>("env", "__netf2");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__getf2, wasm_allocator>("env", "__getf2");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__gttf2, wasm_allocator>("env", "__gttf2");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__letf2, wasm_allocator>("env", "__letf2");

        rhf_t::add<WasmHostMethods, &WasmHostMethods::__lttf2, wasm_allocator>("env", "__lttf2");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__cmptf2, wasm_allocator>("env", "__cmptf2");
        rhf_t::add<WasmHostMethods, &WasmHostMethods::__unordtf2, wasm_allocator>("env", "__unordtf2");
    }


    void CWasmInterface::Execute( vector <uint8_t> code, CWasmContextInterface *pWasmContext ) {

        // Thread specific `allocator` used for wasm linear memory.
        wasm_allocator wa;
        watchdog wd{std::chrono::seconds(1)};
        try {
            // Instaniate a new backend using the wasm provided.
            backend_t bkend(code);
            //wd.set_callback([&]() { bkend.get_context().exit(); });

            // Point the backend to the allocator you want it to use.
            bkend.set_wasm_allocator(&wa);
            bkend.initialize();

            // Resolve the host functions indices.
            rhf_t::resolve(bkend.get_module());

            // Instaniate a "host"
            WasmHostMethods ehm(pWasmContext);

            // std::cout << std::string("receiver:") << wasm::name(pWasmContext->Receiver()).to_string()
            //   << std::string(" contract:") << wasm::name(pWasmContext->Contract()).to_string()
            //   << std::string(" action:") << wasm::name(pWasmContext->Action()).to_string()<< std::endl;

            system_clock::time_point start = system_clock::now();
            // Execute apply.
            bkend(&ehm, "env", "apply", pWasmContext->Receiver(), pWasmContext->Contract(), pWasmContext->Action());

            system_clock::time_point end = system_clock::now();
            std::cerr << std::string("duration:")
                      << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

            return;

        } catch (vm::exception &e) {
            WASM_THROW(wasm_exception, e.detail())
        } catch (wasm::exception &e) {
            //WASM_THROW(wasm_exception, e.detail())
            throw;
        }
        WASM_THROW(wasm_exception, "wasm fail")

    }

    void CWasmInterface::validate( vector <uint8_t> code ) {

        try {
            backend_t bkend(code);
        } catch (vm::exception &e) {
            WASM_THROW(wasm_exception, e.detail())
        }
        WASM_RETHROW_EXCEPTIONS(wasm_exception, "wasm code parse exception")

    }
}//wasm
