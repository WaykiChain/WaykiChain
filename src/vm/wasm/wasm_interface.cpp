#include <eosio/vm/backend.hpp>
#include <eosio/vm/error_codes.hpp>

#include "softfloat.hpp"
#include "compiler_builtins/compiler_builtins.hpp"
#include "wasm/wasm_context_interface.hpp"
#include "wasm/datastream.hpp"
#include "wasm/types/uint128.hpp"
#include "wasm/wasm_log.hpp"
#include "wasm/wasm_constants.hpp"
#include "wasm/wasm_runtime.hpp"
#include "wasm/wasm_interface.hpp"
#include "wasm/wasm_variant.hpp"

#include "wasm/exception/exceptions.hpp"

#include "crypto/hash.h"
#include <openssl/ripemd.h>
#include <openssl/sha.h>

using namespace eosio;
using namespace eosio::vm;

#define CHECK_WASM_IN_MEMORY(DATA, LENGTH) \
    if(LENGTH > 0){                        \
        CHAIN_ASSERT( pWasmContext->is_memory_in_wasm_allocator(reinterpret_cast<uint64_t>(DATA) + LENGTH - 1), \
                      wasm_chain::wasm_memory_exception, "access violation" )}

#define CHECK_WASM_DATA_SIZE(LENGTH, DATA_NAME ) \
    CHAIN_ASSERT( LENGTH <= max_wasm_api_data_bytes,                 \
                  wasm_chain::wasm_api_data_size_exceeds_exception,  \
                  "%s size must be <= %ld, but get %ld",              \
                  DATA_NAME, max_wasm_api_data_bytes, LENGTH )

namespace wasm {
    using code_version_t     = uint256;
    using backend_validate_t = backend<wasm::wasm_context_interface, vm::interpreter>;
    using rhf_t              = eosio::vm::registered_host_functions<wasm_context_interface>;

    std::optional<std::map <code_version_t, std::shared_ptr<wasm_instantiated_module_interface>>>& get_wasm_instantiation_cache(){
        static std::optional<std::map <code_version_t, std::shared_ptr<wasm_instantiated_module_interface>>> wasm_instantiation_cache;
        return wasm_instantiation_cache;
    }

    std::shared_ptr <wasm_runtime_interface>& get_runtime_interface(){
        static std::shared_ptr <wasm_runtime_interface> runtime_interface;
        return runtime_interface;
    }


    wasm_interface::wasm_interface() {}
    wasm_interface::~wasm_interface() {}

    void wasm_interface::exit() {
        get_runtime_interface()->immediately_exit_currently_running_module();
    }

    std::shared_ptr <wasm_instantiated_module_interface> get_instantiated_backend(const vector <uint8_t> &code) {

        try {
            if (!get_wasm_instantiation_cache().has_value()){
                 get_wasm_instantiation_cache() = std::map <code_version_t, std::shared_ptr<wasm_instantiated_module_interface>>{};
            }

            auto code_id = Hash(code.begin(), code.end());
            auto it = get_wasm_instantiation_cache()->find(code_id);
            if (it == get_wasm_instantiation_cache()->end()) {
                get_wasm_instantiation_cache().value()[code_id] = get_runtime_interface()->instantiate_module((const char*)code.data(), code.size());
                return get_wasm_instantiation_cache().value()[code_id];
            }
            return it->second;
        } catch (...) {
            throw;
        }

    }

    void wasm_interface::execute(const vector <uint8_t> &code, wasm_context_interface *pWasmContext) {

        pWasmContext->pause_billing_timer();
        auto pInstantiated_module = get_instantiated_backend(code);
        pWasmContext->resume_billing_timer();

        //system_clock::time_point start = system_clock::now();
        pInstantiated_module->apply(pWasmContext);
        // system_clock::time_point end = system_clock::now();
        // std::cout << std::string("wasm duration:")
        //           << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

    }

    void wasm_interface::validate(const vector <uint8_t> &code) {

        try {
             auto          code_bytes = (uint8_t*)code.data();
             size_t        code_size  = code.size();
             wasm_code_ptr code_ptr(code_bytes, code_size);
             auto bkend               = backend_validate_t(code_ptr, code_size);
             rhf_t::resolve(bkend.get_module());
        } catch (vm::exception &e) {
            CHAIN_THROW(wasm_chain::code_parse_exception, e.detail())
        }
        CHAIN_RETHROW_EXCEPTIONS(wasm_chain::code_parse_exception, "wasm code parse exception")

    }

    void wasm_interface::initialize(vm_type vm) {

        if (vm == wasm::vm_type::eos_vm)
            get_runtime_interface() = std::make_shared<wasm::wasm_vm_runtime<vm::interpreter>>();
        else if (vm == wasm::vm_type::eos_vm_jit)
            get_runtime_interface() = std::make_shared<wasm::wasm_vm_runtime<vm::jit>>();
        else
            get_runtime_interface() = std::make_shared<wasm::wasm_vm_runtime<vm::interpreter>>();

    }

    class wasm_host_methods {

    public:
        wasm_host_methods( wasm_context_interface *pCtx ) {
            pWasmContext = pCtx;
            print_ignore = !pCtx->contracts_console();
        };

        ~wasm_host_methods() {};

        template<typename T>
        static void AddPrefix( T t, string &key ) {
            std::vector<char> prefix = wasm::pack(t);
                              key    = string((const char *) prefix.data(), prefix.size()) + key;
        }

        //system
        void abort() {
            CHAIN_ASSERT( false, wasm_chain::abort_called, "abort() called" )
        }

        void wasm_assert( uint32_t test, const void *msg ) {
            CHAIN_ASSERT( test, wasm_chain::wasm_assert_exception, (char *)msg )
        }

        void wasm_assert_message( uint32_t test,  const void *msg, uint32_t msg_len ) {
            if (!test) {
                //CHECK_WASM_IN_MEMORY( msg,     msg_len)
                CHECK_WASM_DATA_SIZE( msg_len, "msg"  )

                std::string str = string((const char *) msg, msg_len);
                CHAIN_ASSERT( false, wasm_chain::wasm_assert_message_exception, str)
            }
        }

        void wasm_assert_code( uint32_t test, uint64_t code ) {
            if (!test) {
                std::ostringstream o;
                o << code;
                CHAIN_ASSERT(false, wasm_chain::wasm_assert_code_exception, o.str())
            }
        }

        void wasm_exit( int32_t code ){
            //pWasmContext->exit();
        }

        uint64_t current_time() {
            //return static_cast<uint64_t>( context.control.pending_block_time().time_since_epoch().count() );
            //std::cout << "current_time" << std::endl;
            //return 0;
            return pWasmContext->pending_block_time();
        }

        //action
        uint32_t read_action_data( void* data, uint32_t data_len ) {
            uint32_t s = pWasmContext->get_action_data_size();
            if (data_len == 0) return s;

            uint32_t copy_len = std::min(data_len, s);

            //CHECK_WASM_IN_MEMORY(data,     copy_len)
            CHECK_WASM_DATA_SIZE(copy_len, "data"  )

            std::memcpy(data, pWasmContext->get_action_data(), copy_len);
            return copy_len;
        }

        uint32_t action_data_size() {
            auto size = pWasmContext->get_action_data_size();
            return size;
        }

        uint64_t current_receiver() {
            return pWasmContext->receiver();
        }


        void assert_sha1(const void * data, uint32_t data_len, void* hash_val) {
            //CHECK_WASM_IN_MEMORY(data,     data_len)
            //CHECK_WASM_IN_MEMORY(hash_val, 20      )
            CHECK_WASM_DATA_SIZE(data_len, "data"  )

            checksum160_type checksum;
            SHA1((const unsigned char*)data, data_len, (unsigned char *)&checksum);
            CHAIN_ASSERT( std::memcmp((unsigned char *)&checksum, (unsigned char *)hash_val, 20) == 0, crypto_api_exception, "hash mismatch" );
        }

        void assert_sha256(const void * data, uint32_t data_len, void* hash_val) {
            //CHECK_WASM_IN_MEMORY(data,     data_len)
            //CHECK_WASM_IN_MEMORY(hash_val, 32      )
            CHECK_WASM_DATA_SIZE(data_len, "data"  )

            checksum256_type checksum;
            SHA256((const unsigned char*)data, data_len, (unsigned char *)&checksum);
            CHAIN_ASSERT( std::memcmp((unsigned char *)&checksum, (unsigned char *)hash_val, 20) == 0, crypto_api_exception, "hash mismatch" );
        }

        void assert_sha512(const void * data, uint32_t data_len, void* hash_val) {
            //CHECK_WASM_IN_MEMORY(data,     data_len)
            //CHECK_WASM_IN_MEMORY(hash_val, 64      )
            CHECK_WASM_DATA_SIZE(data_len, "data"  )

            checksum512_type checksum;
            SHA512((const unsigned char*)data, data_len, (unsigned char *)&checksum);
            CHAIN_ASSERT( std::memcmp((unsigned char *)&checksum, (unsigned char *)hash_val, 20) == 0, crypto_api_exception, "hash mismatch" );
        }

        void assert_ripemd160(const void * data, uint32_t data_len, void* hash_val) {
            //CHECK_WASM_IN_MEMORY(data,     data_len)
            //CHECK_WASM_IN_MEMORY(hash_val, 20      )
            CHECK_WASM_DATA_SIZE(data_len, "data"  )

            checksum160_type checksum;
            RIPEMD160((const unsigned char*)data, data_len, (unsigned char *)&checksum);
            CHAIN_ASSERT( std::memcmp((unsigned char *)&checksum, (unsigned char *)hash_val, 20) == 0, crypto_api_exception, "hash mismatch" );
        }

        void sha1( const void *data, uint32_t data_len, void *hash_val ) {
            //CHECK_WASM_IN_MEMORY(data,     data_len)
            //CHECK_WASM_IN_MEMORY(hash_val, 20      )
            CHECK_WASM_DATA_SIZE(data_len, "data"  )

            SHA1((const unsigned char*)data, data_len, (unsigned char *)hash_val);
        }

        void sha256( const void *data, uint32_t data_len, void *hash_val ) {
            //CHECK_WASM_IN_MEMORY(data,     data_len)
            //CHECK_WASM_IN_MEMORY(hash_val, 32      )
            CHECK_WASM_DATA_SIZE(data_len, "data"  )

            SHA256((const unsigned char*)data, data_len, (unsigned char *)hash_val);
        }

        void sha512( const void *data, uint32_t data_len, void *hash_val ) {
            //CHECK_WASM_IN_MEMORY(data,     data_len)
            //CHECK_WASM_IN_MEMORY(hash_val, 64      )
            CHECK_WASM_DATA_SIZE(data_len, "data"  )

            SHA512((const unsigned char*)data, data_len, (unsigned char *)hash_val);
        }

        void ripemd160( const void *data, uint32_t data_len, void *hash_val ) {
            //CHECK_WASM_IN_MEMORY(data,     data_len)
            //CHECK_WASM_IN_MEMORY(hash_val, 20      )
            CHECK_WASM_DATA_SIZE(data_len, "data"  )

            RIPEMD160((const unsigned char*)data, data_len, (unsigned char *)hash_val);
        }

        //database
        int32_t db_store( const uint64_t payer, const void *key, uint32_t key_len, const void *val, uint32_t val_len ) {

            //CHECK_WASM_IN_MEMORY(key, key_len)
            //CHECK_WASM_IN_MEMORY(val, val_len)

            CHECK_WASM_DATA_SIZE(key_len, "key"  )
            CHECK_WASM_DATA_SIZE(val_len, "value")

            string k        = string((const char *) key, key_len);
            string v        = string((const char *) val, val_len);
            auto   contract = pWasmContext->receiver();

            AddPrefix(contract, k);

            CHAIN_ASSERT( pWasmContext->set_data(contract, k, v),
                          wasm_chain::wasm_assert_exception,
                          "db_store failed, key: %s", to_hex(k))

            pWasmContext->update_storage_usage(payer, k.size() + v.size());
            return 1;
        }

        int32_t db_remove( const uint64_t payer, const void *key, uint32_t key_len ) {

            //CHECK_WASM_IN_MEMORY(key,     key_len)
            CHECK_WASM_DATA_SIZE(key_len, "key"  )

            string k        = string((const char *) key, key_len);
            auto   contract = pWasmContext->receiver();
            AddPrefix(contract, k);

            CHAIN_ASSERT( pWasmContext->erase_data(contract, k),
                          wasm_chain::wasm_assert_exception,
                          "db_remove failed, key: %s", to_hex(k))

            pWasmContext->update_storage_usage(payer, k.size());
            return 1;
        }

        int32_t db_get( const void *key, uint32_t key_len, void *val, uint32_t val_len ) {

            //CHECK_WASM_IN_MEMORY(key,     key_len)
            CHECK_WASM_DATA_SIZE(key_len, "key"  )

            string k        = string((const char *) key, key_len);
            auto   contract = pWasmContext->receiver();
            AddPrefix(contract, k);

            string v;
            if (!pWasmContext->get_data(contract, k, v)) return 0;

            auto size    = v.size();
            if (val_len == 0) return size;

            //CHECK_WASM_IN_MEMORY(val,     val_len)
            CHECK_WASM_DATA_SIZE(val_len, "value")

            auto val_size = val_len > size ? size : val_len;
            std::memcpy(val, v.data(), val_size);

            //pWasmContext->update_storage_usage(payer, k.size() + v.size());
            return val_size;
        }

        int32_t db_update( const uint64_t payer, const void *key, uint32_t key_len, const void *val, uint32_t val_len ) {

            //CHECK_WASM_IN_MEMORY(key,     key_len)
            //CHECK_WASM_IN_MEMORY(val,     val_len)
            CHECK_WASM_DATA_SIZE(key_len, "key"  )
            CHECK_WASM_DATA_SIZE(val_len, "value")

            string k        = string((const char *) key, key_len);
            string v        = string((const char *) val, val_len);
            auto   contract = pWasmContext->receiver();
            AddPrefix(contract, k);

            CHAIN_ASSERT( pWasmContext->set_data(contract, k, v),
                          wasm_chain::wasm_assert_exception,
                          "db_update failed, key: %s", to_hex(k))

            pWasmContext->update_storage_usage(payer, k.size() + v.size());
            return 1;
        }


        //memory
        void *memcpy( void *dest, const void *src, int len ) {
            CHAIN_ASSERT( (size_t)(std::abs((ptrdiff_t)dest - (ptrdiff_t)src)) >= len,
                          wasm_chain::overlapping_memory_error,
                          "memcpy can only accept non-aliasing pointers");

            //CHECK_WASM_IN_MEMORY(dest, len)
            //CHECK_WASM_IN_MEMORY(src,  len)

            return (char *) std::memcpy(dest, src, len);
        }

        void *memmove( void *dest, const void *src, int len ) {
            //CHECK_WASM_IN_MEMORY(dest, len)
            //CHECK_WASM_IN_MEMORY(src,  len)

            return (char *) std::memmove(dest, src, len);
        }

        int memcmp( const void *dest, const void *src, int len ) {
            //CHECK_WASM_IN_MEMORY(dest, len)
            //CHECK_WASM_IN_MEMORY(src,  len)

            int ret = std::memcmp(dest, src, len);
            if (ret < 0)
                return -1;
            if (ret > 0)
                return 1;
            return 0;
        }

        void *memset( void *dest, int val, int len ) {
            //CHECK_WASM_IN_MEMORY(dest, len)

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

            //fixme:should be have the limited of the length str
            auto size = strlen((const char*)str);
            //CHECK_WASM_IN_MEMORY(str,  size )
            CHECK_WASM_DATA_SIZE(size, "str")

            if (!print_ignore) {
                std::ostringstream o;
                o << (const char*)str;
                pWasmContext->console_append(o.str());
            }
        }

        void prints_l( const void *str, uint32_t str_len ) {

            //CHECK_WASM_IN_MEMORY(str,     str_len)
            CHECK_WASM_DATA_SIZE(str_len, "str"  )

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

            //CHECK_WASM_IN_MEMORY(data,     data_len)
            CHECK_WASM_DATA_SIZE(data_len, "data"  )

            if (!print_ignore) {
                string str((const char*)data, data_len);
                pWasmContext->console_append(to_hex(str));
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

        void notify_recipient( uint64_t recipient ) {
            CHAIN_ASSERT( is_account(recipient),
                          wasm_chain::account_access_exception,
                          "can not send a receipt to a non-exist account '%s'",
                          wasm::name(recipient).to_string());

            pWasmContext->notify_recipient(recipient);

        }

        bool is_account( uint64_t account ) {
            return pWasmContext->is_account(account);
        }

        //transaction
        void send_inline( void *data, uint32_t data_len ) {

            //CHECK_WASM_IN_MEMORY(data,     data_len)
            CHECK_WASM_DATA_SIZE(data_len, "data"  )

            inline_transaction trx = wasm::unpack<inline_transaction>((const char *) data, data_len);

            pWasmContext->execute_inline(trx);

        }

        uint32_t get_active_producers(void *producers, uint32_t data_len){

            //get active producers
            std::vector<uint64_t> active_producers = pWasmContext->get_active_producers();

            size_t len   = active_producers.size() * sizeof(uint64_t);
            if (data_len == 0) return len;

            auto copy_len = std::min( static_cast<size_t>(data_len), len );

            //CHECK_WASM_IN_MEMORY(producers, copy_len)
            CHECK_WASM_DATA_SIZE(copy_len,  "data"  )

            std::memcpy(producers, active_producers.data(), copy_len);
            return copy_len;
        }


        uint64_t get_maintainer(uint64_t contract) {
            return pWasmContext->get_maintainer(contract);
        }

        uint32_t get_txid(void *data) {
            TxID txid = pWasmContext->get_txid();
            string str_txid = txid.ToString();
            uint32_t len = str_txid.size();
            CHECK_WASM_DATA_SIZE(len,  "data")
            std::memcpy(data, str_txid.c_str(), len);

            return len;
        }

        uint32_t get_system_asset_price(uint64_t base_symble, uint64_t quote_symble, void* data, uint32_t data_len ) {

            std::vector<char> price;
            bool success = pWasmContext->get_system_asset_price(base_symble, quote_symble, price);
            if (!success) return 0;

            size_t len = price.size();
            if (data_len == 0) return len;

            auto copy_len = std::min( static_cast<size_t>(data_len), len );
            CHECK_WASM_DATA_SIZE(copy_len,  "data")

            std::memcpy(data, price.data(), copy_len);
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
        wasm_context_interface *pWasmContext = nullptr;

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

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, assert_sha1,      assert_sha1)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, assert_sha256,    assert_sha256)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, assert_sha512,    assert_sha512)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, assert_ripemd160, assert_ripemd160)

    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, sha1,      sha1)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, sha256,    sha256)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, sha512,    sha512)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, ripemd160, ripemd160)

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
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, notify_recipient,    notify_recipient)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, has_authorization,    has_auth)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, get_active_producers,   get_active_producers)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, get_txid,               get_txid)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, get_maintainer,         get_maintainer)
    REGISTER_WASM_VM_INTRINSIC(wasm_host_methods, env, get_system_asset_price, get_system_asset_price)

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

extern  void wasm_code_cache_free() {
     //free heap before shut down
     wasm::get_wasm_instantiation_cache().reset();
}
