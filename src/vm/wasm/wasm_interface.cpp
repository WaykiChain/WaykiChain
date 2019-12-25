#include <eosio/vm/backend.hpp>
#include <eosio/vm/error_codes.hpp>

#include "softfloat.hpp"
#include "compiler_builtins/compiler_builtins.hpp"
#include "wasm/wasm_context_interface.hpp"
#include "wasm/datastream.hpp"
#include "wasm/types/uint128.hpp"
#include "wasm/exceptions.hpp"
#include "wasm/wasm_log.hpp"
#include "wasm/wasm_config.hpp"
#include "wasm/wasm_runtime.hpp"
#include "wasm/wasm_interface.hpp"
#include "wasm/wasm_variant.hpp"

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
        pWasmContext->pause_billing_timer();
        std::shared_ptr <wasm_instantiated_module_interface> pInstantiated_module = get_instantiated_backend(code);
        pWasmContext->resume_billing_timer();

        //system_clock::time_point start = system_clock::now();
        pInstantiated_module->apply(pWasmContext);
        // system_clock::time_point end = system_clock::now();
        // std::cout << std::string("wasm duration:")
        //           << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

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

    class wasm_host_methods {
        
    public:
        wasm_host_methods( wasm_context_interface *pCtx ) {
            pWasmContext = pCtx;
            print_ignore = !pCtx->contracts_console();
        };

        ~wasm_host_methods() {};

        template<typename T>
        static void AddPrefix( T t, string &k ) {
            std::vector<char> key = wasm::pack(t);
            k = string((const char *) key.data(), key.size()) + k;
        }

        //system
        void abort() {
            WASM_ASSERT(false, abort_called, "wasm-assert-fail:%s", "abort() called")
        }

        void wasm_assert( uint32_t test, const void *msg ) {
            WASM_ASSERT(test, wasm_assert_exception, "wasm-assert-fail:%s", (char *)msg)
        }

        void wasm_assert_message( uint32_t test,  const void *msg, uint32_t msg_len ) {
            if (!test) {             
                WASM_ASSERT( pWasmContext->is_memory_in_wasm_allocator(reinterpret_cast<const char*>(msg) + msg_len), 
                             wasm_memory_exception, "%s", "access violation")

                WASM_ASSERT( msg_len < max_wasm_api_data_bytes, wasm_api_data_too_big, "%s", "msg size too big")

                std::string o = string((const char *) msg, msg_len);
                WASM_ASSERT( false, wasm_assert_code_exception, "wasm_assert_code:%s", o.c_str())
            }
        }

        void wasm_assert_code( uint32_t test, uint64_t code ) {
            if (!test) {
                std::ostringstream o;
                o << code;
                WASM_ASSERT(false, wasm_assert_code_exception, "wasm_assert_code:%s", o.str().c_str())
            }
        }

        void wasm_exit( int32_t code ){
            pWasmContext->exit();
        }

        uint64_t current_time() {
            //return static_cast<uint64_t>( context.control.pending_block_time().time_since_epoch().count() );
            //std::cout << "current_time" << std::endl;
            //return 0;
            return pWasmContext->block_time();
        }

        //action
        uint32_t read_action_data( void* memory, uint32_t buf_len ) {
            uint32_t s = pWasmContext->get_action_data_size();
            if (buf_len == 0) return s;

            uint32_t copy_len = std::min(buf_len, s);
            WASM_ASSERT( pWasmContext->is_memory_in_wasm_allocator(reinterpret_cast<const char*>(memory) + copy_len), 
                         wasm_memory_exception, "%s", "access violation")

            std::memcpy(memory, pWasmContext->get_action_data(), copy_len);
            return copy_len;
        }

        uint32_t action_data_size() {
            auto size = pWasmContext->get_action_data_size();
            return size;
        }

        uint64_t current_receiver() {
            return pWasmContext->receiver();
        }

        void sha256( const void *data, uint32_t data_len, void *hash_val ) {

            // string k = string((const char *) data, data_len);
            // SHA256(k.data(), k.size(), hash_val.begin());

        }

        //database
        int32_t db_store( const uint64_t payer, const void *key, uint32_t key_len, const void *val, uint32_t val_len ) {
            WASM_ASSERT( pWasmContext->is_memory_in_wasm_allocator(reinterpret_cast<const char*>(key) + key_len), 
                         wasm_memory_exception, "%s", "access violation")
            WASM_ASSERT( pWasmContext->is_memory_in_wasm_allocator(reinterpret_cast<const char*>(val) + val_len), 
                         wasm_memory_exception, "%s", "access violation")

            WASM_ASSERT(key_len < max_wasm_api_data_bytes, wasm_api_data_too_big, "%s", "key size too big");
            WASM_ASSERT(val_len < max_wasm_api_data_bytes, wasm_api_data_too_big, "%s", "value size too big");

            string k = string((const char *) key, key_len);
            string v = string((const char *) val, val_len);

            AddPrefix(pWasmContext->receiver(), k);
            const uint64_t contract = pWasmContext->receiver();

            wasm_assert(pWasmContext->set_data(contract, k, v), ("wasm db_store SetContractData failed, key:" +
                                                                ToHex(k)).c_str());      //wasmContext.AppendUndo(contract, k, oldValue);
            pWasmContext->update_storage_usage(payer, k.size() + v.size());

            return 1;
        }

        int32_t db_remove( const uint64_t payer, const void *key, uint32_t key_len ) {
            WASM_ASSERT( pWasmContext->is_memory_in_wasm_allocator(reinterpret_cast<const char*>(key) + key_len), 
                         wasm_memory_exception, "%s", "access violation")

            WASM_ASSERT(key_len < max_wasm_api_data_bytes, wasm_api_data_too_big, "%s", "key size too big");

            string k = string((const char *) key, key_len);
            AddPrefix(pWasmContext->receiver(), k);

            const uint64_t contract = pWasmContext->receiver();

            // string v;
            // wasm_assert(pWasmContext->get_data(contract, k, v),
            //             ("wasm db_remove get_data key fail, key:" + ToHex(k, "")).c_str());

            wasm_assert(pWasmContext->erase_data(contract, k),
                        ("wasm db_remove EraseContractData failed, key:" + ToHex(k, " ")).c_str());

            pWasmContext->update_storage_usage(payer, k.size());

            return 1;
        }

        int32_t db_get( const void *key, uint32_t key_len, void *val, uint32_t val_len ) {
            WASM_ASSERT( pWasmContext->is_memory_in_wasm_allocator(reinterpret_cast<const char*>(key) + key_len), 
                         wasm_memory_exception, "%s", "access violation")
            WASM_ASSERT( pWasmContext->is_memory_in_wasm_allocator(reinterpret_cast<const char*>(val) + val_len), 
                         wasm_memory_exception, "%s", "access violation")

            WASM_ASSERT(key_len < max_wasm_api_data_bytes, wasm_api_data_too_big, "%s", "key size too big");
            WASM_ASSERT(val_len < max_wasm_api_data_bytes, wasm_api_data_too_big, "%s", "value size too big");

            string k = string((const char *) key, key_len);
            AddPrefix(pWasmContext->receiver(), k);

            string v;
            const uint64_t contract = pWasmContext->receiver();

            if (!pWasmContext->get_data(contract, k, v)) return 0;

            auto size = v.size();
            if (val_len == 0) return size;

            auto val_size = val_len > size ? size : val_len;
            memcpy(val, v.data(), val_size);

            //pWasmContext->update_storage_usage(payer, k.size() + v.size());
            return val_size;
        }

        int32_t db_update( const uint64_t payer, const void *key, uint32_t key_len, const void *val, uint32_t val_len ) {
            WASM_ASSERT( pWasmContext->is_memory_in_wasm_allocator(reinterpret_cast<const char*>(key) + key_len), 
                         wasm_memory_exception, "%s", "access violation")
            WASM_ASSERT( pWasmContext->is_memory_in_wasm_allocator(reinterpret_cast<const char*>(val) + val_len), 
                         wasm_memory_exception, "%s", "access violation")

            WASM_ASSERT(key_len < max_wasm_api_data_bytes, wasm_api_data_too_big, "%s",
                        "key size too big");
            WASM_ASSERT(val_len < max_wasm_api_data_bytes, wasm_api_data_too_big, "%s",
                        "value size too big");

            string k = string((const char *) key, key_len);
            string v = string((const char *) val, val_len);

            AddPrefix(pWasmContext->receiver(), k);

            const uint64_t contract = pWasmContext->receiver();
            // string old_value;
            // wasm_assert(pWasmContext->get_data(contract, k, old_value),
            //             ("wasm db_update db_get key fail, key:" + ToHex(k, "")).c_str());
            wasm_assert(pWasmContext->set_data(contract, k, v),
                        ("wasm db_update set_data key fail, key:" + ToHex(k, "")).c_str());

            //pWasmContext->update_storage_usage(payer, v.size() - old_value.size());
            pWasmContext->update_storage_usage(payer, k.size() + v.size());

            return 1;
        }


        //memory
        void *memcpy( void *dest, const void *src, int len ) {
            WASM_ASSERT((size_t)(std::abs((ptrdiff_t)dest - (ptrdiff_t)src)) >= len,
                  overlapping_memory_error, "%s", "memcpy can only accept non-aliasing pointers");

            return (char *) std::memcpy(dest, src, len);
        }

        void *memmove( void *dest, const void *src, int len ) {
            return (char *) std::memmove(dest, src, len);
        }

        int memcmp( const void *dest, const void *src, int len ) {
            int ret = std::memcmp(dest, src, len);
            if (ret < 0)
                return -1;
            if (ret > 0)
                return 1;
            return 0;
        }

        void *memset( void *dest, int val, int len ) {
            return (char *) std::memset(dest, val, len);
        }

        void printn( uint64_t val ) {//should be name
            if (!print_ignore) {
                pWasmContext->console_append(wasm::name(val).to_string());
            }
        }

        void printui( uint64_t val ) {
            if (!print_ignore) {
                std::ostringstream o;
                o << val;
                pWasmContext->console_append(o.str());
            }
        }

        void printi( int64_t val ) {
            if (!print_ignore) {
                std::ostringstream o;
                o << val;
                pWasmContext->console_append(o.str());
            }
        }

        void prints( const void *str ) {
            WASM_ASSERT(strlen((const char*)str) < max_wasm_api_data_bytes, 
                        wasm_api_data_too_big, "%s", "string size too big");

            if (!print_ignore) {
                std::ostringstream o;
                o << (const char*)str;
                pWasmContext->console_append(o.str());
            }
        }

        void prints_l( const void *str, uint32_t str_len ) {
            WASM_ASSERT( pWasmContext->is_memory_in_wasm_allocator(reinterpret_cast<const char*>(str) + str_len), 
                wasm_memory_exception, "%s", "access violation")

            WASM_ASSERT(str_len < max_wasm_api_data_bytes, wasm_api_data_too_big, "%s", "string size too big");
            if (!print_ignore) {
                pWasmContext->console_append(string((const char*)str, str_len));
            }
        }

        void printi128( const __int128 &val ) {
            if (!print_ignore) {
                bool is_negative = (val < 0);
                unsigned __int128 val_magnitude;

                if (is_negative)
                    val_magnitude = static_cast<unsigned __int128>(-val); // Works even if val is at the lowest possible value of a int128_t
                else
                    val_magnitude = static_cast<unsigned __int128>(val);

                wasm::uint128 v(val_magnitude >> 64, static_cast<uint64_t>(val_magnitude));

                if (is_negative) {
                    pWasmContext->console_append(string("-"));
                }

                pWasmContext->console_append(string(v));
            }
        }

        void printui128( const unsigned __int128 &val ) {
            if (!print_ignore) {
                wasm::uint128 v(val >> 64, static_cast<uint64_t>(val));
                pWasmContext->console_append(string(v));
            }
        }

        void printsf( float val ) {
            if (!print_ignore) {
                // Assumes float representation on native side is the same as on the WASM side
                std::ostringstream o;
                o.setf(std::ios::scientific, std::ios::floatfield);
                o.precision(std::numeric_limits<float>::digits10);
                o << val;

                pWasmContext->console_append(o.str());
            }
        }

        void printdf( double val ) {
            if (!print_ignore) {
                // Assumes double representation on native side is the same as on the WASM side
                std::ostringstream o;
                o.setf(std::ios::scientific, std::ios::floatfield);
                o.precision(std::numeric_limits<double>::digits10);
                o << val;
                pWasmContext->console_append(o.str());
            }
        }

        void printqf( const float128_t& val ) {
            /*
             * Native-side long double uses an 80-bit extended-precision floating-point number.
             * The easiest solution for now was to use the Berkeley softfloat library to round the 128-bit
             * quadruple-precision floating-point number to an 80-bit extended-precision floating-point number
             * (losing precision) which then allows us to simply cast it into a long double for printing purposes.
             *
             * Later we might find a better solution to print the full quadruple-precision floating-point number.
             * Maybe with some compilation flag that turns long double into a quadruple-precision floating-point number,
             * or maybe with some library that allows us to print out quadruple-precision floating-point numbers without
             * having to deal with long doubles at all.
             */

            if (!print_ignore) {

                std::ostringstream o;
                o.setf(std::ios::scientific, std::ios::floatfield);
                o.precision(std::numeric_limits<long double>::digits10);

#ifdef __x86_64__
                extFloat80_t val_approx;
                f128M_to_extF80M(&val, &val_approx);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
                o << *(long double*)(&val_approx);
#pragma GCC diagnostic pop
#else
                double val_approx = from_softfloat64(f128M_to_f64(&val));
                o << *(long double *) (&val_approx);
#endif
                pWasmContext->console_append(o.str());
            }
        }

        void printhex( const char *data, uint32_t data_len ) {
            if (!print_ignore) {

                WASM_ASSERT(data_len < max_wasm_api_data_bytes, wasm_api_data_too_big, "%s",
                            "wasm api data too big");

                string str((const char*)data, data_len);
                pWasmContext->console_append(ToHex(str, ""));
            }
        }
        
        //authorization
        void require_auth( uint64_t account ) {
            pWasmContext->require_auth(account);
        }

        void require_auth2( uint64_t account, uint64_t permission ) {
            pWasmContext->require_auth(account);
        }

        bool has_authorization( uint64_t account ) const {
            return pWasmContext->has_authorization(account);
        }

        void require_recipient( uint64_t recipient ) {

            WASM_ASSERT( is_account(recipient), account_operation_exception, 
                         "can not send a receipt to a non-exist account '%s'",
                         wasm::name(recipient).to_string().c_str());

            pWasmContext->require_recipient(recipient);

        }

        bool is_account( uint64_t account ) {
            return pWasmContext->is_account(account);
        }

        //transaction
        void send_inline( void *data, uint32_t data_len ) {
            WASM_ASSERT(data_len < max_inline_transaction_bytes, inline_transaction_too_big, "%s",
                        "inline transaction too big");
            inline_transaction trx = wasm::unpack<inline_transaction>((const char *) data, data_len);
            pWasmContext->execute_inline(trx);

        }

        uint32_t get_active_producers(void *producers, uint32_t data_len){
            
            //get active producers
            std::vector<uint64_t> active_producers = pWasmContext->get_active_producers();
            // active_producers.push_back(wasm::name("xiaoyu"));
            // active_producers.push_back(wasm::name("walker"));
            // active_producers = pWasmContext->get_active_producers();

            size_t len = active_producers.size() * sizeof(uint64_t);
            if(data_len == 0) return len;

            auto copy_len = std::min( static_cast<size_t>(data_len), len );

            WASM_ASSERT( pWasmContext->is_memory_in_wasm_allocator(reinterpret_cast<const char*>(producers) + copy_len), 
                         wasm_memory_exception, "%s", "access violation")
            WASM_ASSERT(copy_len < max_wasm_api_data_bytes, wasm_api_data_too_big, "%s",
                        "wasm api data too big");

            std::memcpy(producers, active_producers.data(), copy_len);
            return copy_len;
        }

        //llvm compiler builtins rt apis( GCC low-level runtime library ), eg. std:string in contract
        void __ashlti3( __int128 &ret, uint64_t low, uint64_t high, uint32_t shift ) {
            uint128 i(high, low);
            i <<= shift;
            ret = (unsigned __int128) i;
        }

        void __ashrti3( __int128 &ret, uint64_t low, uint64_t high, uint32_t shift ) {
            // retain the signedness
            ret = high;
            ret <<= 64;
            ret |= low;
            ret >>= shift;
        }

        void __lshlti3( __int128 &ret, uint64_t low, uint64_t high, uint32_t shift ) {
            uint128 i(high, low);
            i <<= shift;
            ret = (unsigned __int128) i;
        }

        void __lshrti3( __int128 &ret, uint64_t low, uint64_t high, uint32_t shift ) {
            uint128 i(high, low);
            i >>= shift;
            ret = (unsigned __int128) i;
        }

        void __divti3( __int128 &ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
            __int128 lhs = ha;
            __int128 rhs = hb;

            lhs <<= 64;
            lhs |= la;

            rhs <<= 64;
            rhs |= lb;

            wasm_assert(rhs != 0, "divide by zero");

            lhs /= rhs;

            ret = lhs;
        }

        void __udivti3( unsigned __int128 &ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
            unsigned __int128 lhs = ha;
            unsigned __int128 rhs = hb;

            lhs <<= 64;
            lhs |= la;

            rhs <<= 64;
            rhs |= lb;

            wasm_assert(rhs != 0, "divide by zero");

            lhs /= rhs;
            ret = lhs;
        }

        void __multi3( __int128 &ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
            __int128 lhs = ha;
            __int128 rhs = hb;

            lhs <<= 64;
            lhs |= la;

            rhs <<= 64;
            rhs |= lb;

            lhs *= rhs;
            ret = lhs;
        }

        void __modti3( __int128 &ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
            __int128 lhs = ha;
            __int128 rhs = hb;

            lhs <<= 64;
            lhs |= la;

            rhs <<= 64;
            rhs |= lb;

            wasm_assert(rhs != 0, "divide by zero");

            lhs %= rhs;
            ret = lhs;
        }

        void __umodti3( unsigned __int128 &ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
            unsigned __int128 lhs = ha;
            unsigned __int128 rhs = hb;

            lhs <<= 64;
            lhs |= la;

            rhs <<= 64;
            rhs |= lb;

            wasm_assert(rhs != 0, "divide by zero");

            lhs %= rhs;
            ret = lhs;
        }

        // arithmetic long double
        void __addtf3( float128_t &ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
            float128_t a = {{la, ha}};
            float128_t b = {{lb, hb}};
            ret = f128_add(a, b);
        }

        void __subtf3( float128_t &ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
            float128_t a = {{la, ha}};
            float128_t b = {{lb, hb}};
            ret = f128_sub(a, b);
        }

        void __multf3( float128_t &ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
            float128_t a = {{la, ha}};
            float128_t b = {{lb, hb}};
            ret = f128_mul(a, b);
        }

        void __divtf3( float128_t &ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
            float128_t a = {{la, ha}};
            float128_t b = {{lb, hb}};
            ret = f128_div(a, b);
        }

        void __negtf2( float128_t &ret, uint64_t la, uint64_t ha ) {
            ret = {{la, (ha ^ (uint64_t) 1 << 63)}};
        }

        // conversion long double
        void __extendsftf2( float128_t &ret, float f ) {
            ret = f32_to_f128(to_softfloat32(f));
        }

        void __extenddftf2( float128_t &ret, double d ) {
            ret = f64_to_f128(to_softfloat64(d));
        }

        double __trunctfdf2( uint64_t l, uint64_t h ) {
            float128_t f = {{l, h}};
            return from_softfloat64(f128_to_f64(f));
        }

        float __trunctfsf2( uint64_t l, uint64_t h ) {
            float128_t f = {{l, h}};
            return from_softfloat32(f128_to_f32(f));
        }

        int32_t __fixtfsi( uint64_t l, uint64_t h ) {
            float128_t f = {{l, h}};
            return f128_to_i32(f, 0, false);
        }

        int64_t __fixtfdi( uint64_t l, uint64_t h ) {
            float128_t f = {{l, h}};
            return f128_to_i64(f, 0, false);
        }

        void __fixtfti( __int128 &ret, uint64_t l, uint64_t h ) {
            float128_t f = {{l, h}};
            ret = ___fixtfti(f);
        }

        uint32_t __fixunstfsi( uint64_t l, uint64_t h ) {
            float128_t f = {{l, h}};
            return f128_to_ui32(f, 0, false);
        }

        uint64_t __fixunstfdi( uint64_t l, uint64_t h ) {
            float128_t f = {{l, h}};
            return f128_to_ui64(f, 0, false);
        }

        void __fixunstfti( unsigned __int128 &ret, uint64_t l, uint64_t h ) {
            float128_t f = {{l, h}};
            ret = ___fixunstfti(f);
        }

        void __fixsfti( __int128 &ret, float a ) {
            ret = ___fixsfti(to_softfloat32(a).v);
        }

        void __fixdfti( __int128 &ret, double a ) {
            ret = ___fixdfti(to_softfloat64(a).v);
        }

        void __fixunssfti( unsigned __int128 &ret, float a ) {
            ret = ___fixunssfti(to_softfloat32(a).v);
        }

        void __fixunsdfti( unsigned __int128 &ret, double a ) {
            ret = ___fixunsdfti(to_softfloat64(a).v);
        }

        double __floatsidf( int32_t i ) {
            return from_softfloat64(i32_to_f64(i));
        }

        void __floatsitf( float128_t &ret, int32_t i ) {
            ret = i32_to_f128(i);
        }

        void __floatditf( float128_t &ret, uint64_t a ) {
            ret = i64_to_f128(a);
        }

        void __floatunsitf( float128_t &ret, uint32_t i ) {
            ret = ui32_to_f128(i);
        }

        void __floatunditf( float128_t &ret, uint64_t a ) {
            ret = ui64_to_f128(a);
        }

        double __floattidf( uint64_t l, uint64_t h ) {
            uint128 v(h, l);
            unsigned __int128 val = (unsigned __int128) v;
            return ___floattidf(*(__int128 *) &val);
        }

        double __floatuntidf( uint64_t l, uint64_t h ) {
            uint128 v(h, l);
            return ___floatuntidf((unsigned __int128) v);
        }

        int ___cmptf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb, int return_value_if_nan ) {
            float128_t a = {{la, ha}};
            float128_t b = {{lb, hb}};
            if (__unordtf2(la, ha, lb, hb))
                return return_value_if_nan;
            if (f128_lt(a, b))
                return -1;
            if (f128_eq(a, b))
                return 0;
            return 1;
        }

        int __eqtf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
            return ___cmptf2(la, ha, lb, hb, 1);
        }

        int __netf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
            return ___cmptf2(la, ha, lb, hb, 1);
        }

        int __getf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
            return ___cmptf2(la, ha, lb, hb, -1);
        }

        int __gttf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
            return ___cmptf2(la, ha, lb, hb, 0);
        }

        int __letf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
            return ___cmptf2(la, ha, lb, hb, 1);
        }

        int __lttf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
            return ___cmptf2(la, ha, lb, hb, 0);
        }

        int __cmptf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
            return ___cmptf2(la, ha, lb, hb, 1);
        }

        int __unordtf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
            float128_t a = {{la, ha}};
            float128_t b = {{lb, hb}};
            if (is_nan(a) || is_nan(b))
                return 1;
            return 0;
        }

        static bool is_nan( const float32_t f ) {
            return f32_is_nan(f);
        }

        static bool is_nan( const float64_t f ) {
            return f64_is_nan(f);
        }

        static bool is_nan( const float128_t &f ) {
            return f128_is_nan(f);
        }

    public:
        wasm_context_interface *pWasmContext;

    private:
        bool print_ignore;

    };

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, abort,            abort)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, wasm_exit,        wasm_exit)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, wasm_assert,      wasm_assert)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, wasm_assert_code, wasm_assert_code)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, current_time,     current_time)

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, read_action_data, read_action_data)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, action_data_size, action_data_size)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, current_receiver, current_receiver)

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, db_store,  db_store)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, db_remove, db_remove)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, db_get,    db_get)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, db_update, db_update)

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, memcpy,  memcpy)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, memmove, memmove)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, memcmp,  memcmp)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, memset,  memset) 
    
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, printn,     printn)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, printui,    printui)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, printi,     printi)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, prints,     prints)     
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, prints_l,   prints_l)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, printi128,  printi128)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, printui128, printui128)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, printsf,    printsf) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, printdf,    printdf)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, printhex,   printhex)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, printqf,    printqf) 

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, is_account,   is_account)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, send_inline,  send_inline) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, require_auth, require_auth)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, require_recipient,    require_recipient) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, has_authorization,    has_auth)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, get_active_producers, get_active_producers) 

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __ashlti3, __ashlti3)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __ashrti3, __ashrti3)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __lshlti3, __lshlti3) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __lshrti3, __lshrti3)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __divti3,  __divti3) 

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __udivti3, __udivti3)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __multi3,  __multi3)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __modti3,  __modti3) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __umodti3, __umodti3)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __addtf3,  __addtf3)  

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __subtf3, __subtf3)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __multf3, __multf3)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __divtf3, __divtf3) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __negtf2, __negtf2)

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __extendsftf2, __extendsftf2)  
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __extenddftf2, __extenddftf2)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __trunctfdf2,  __trunctfdf2)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __trunctfsf2,  __trunctfsf2) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __fixtfsi,     __fixtfsi)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __fixtfdi,     __fixtfdi) 

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __fixtfti,    __fixtfti)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __fixunstfsi, __fixunstfsi)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __fixunstfdi, __fixunstfdi) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __fixunstfti, __fixunstfti)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __fixsfti,    __fixsfti)  

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __fixdfti,    __fixdfti)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __fixunssfti, __fixunssfti)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __fixunsdfti, __fixunsdfti) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __floatsidf,  __floatsidf)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __floatsitf,  __floatsitf)    

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __floatditf,   __floatditf)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __floatunsitf, __floatunsitf)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __floatunditf, __floatunditf) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __floattidf,   __floattidf)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __floatuntidf, __floatuntidf) 

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __eqtf2, __eqtf2)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __netf2, __netf2)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __getf2, __getf2) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __gttf2, __gttf2)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __letf2, __letf2)   
    
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __lttf2,    __lttf2) 
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __cmptf2,   __cmptf2)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, __unordtf2, __unordtf2) 

}//wasm
