#pragma once

#include <cstdint>
#include <string>

using namespace std;

#define WASM_EXCEPTION_BUFFER_LENGTH 1024

#define WASM_ASSERT( expr, exc_type, ... )     \
   if ( !( expr ) ) {                          \
       char buf[WASM_EXCEPTION_BUFFER_LENGTH]; \
       sprintf( buf,  __VA_ARGS__ );           \
       throw exc_type( buf ) ; }

#define WASM_THROW( exc_type, ... )    \
       WASM_ASSERT( false, exc_type,  __VA_ARGS__ )

#define WASM_RETHROW_EXCEPTIONS( exc_type, ... )          \
    catch( exception& e ) {                         \
         char buf[WASM_EXCEPTION_BUFFER_LENGTH];          \
         char buf2[WASM_EXCEPTION_BUFFER_LENGTH];          \
         sprintf( buf,  __VA_ARGS__ );                    \
         sprintf( buf2,"%s , %s", e.detail(), buf);          \
         throw exc_type(buf2);                             \
    } catch( ... ) {                                      \
         char buf[WASM_EXCEPTION_BUFFER_LENGTH];          \
         sprintf( buf,  __VA_ARGS__ );                    \
         throw exc_type(buf);                             \
    }

#define WASM_CAPTURE_AND_RETHROW( ... )          \
    catch( exception& e ) {                       \
       throw;                                    \
    } catch( ... ) {                             \
         char buf[WASM_EXCEPTION_BUFFER_LENGTH]; \
         sprintf( buf,  __VA_ARGS__ );           \
         throw wasm_assert_exception( buf );     \
    }


namespace wasm {
    struct exception : public std::exception {
        virtual const char *what() const throw() = 0;
        virtual const char *detail() const throw() = 0;
        virtual uint32_t code() const throw() = 0;
    };
}


#define WASM_DECLARE_EXCEPTION( name, _code, _what )                              \
   struct name : public wasm::exception {                                         \
      name(const char* msg) : msg(string(msg)) {}                                 \
      virtual const char* what()const throw() { return _what; }                   \
      virtual const char* detail()const throw() { return msg.c_str(); }           \
      virtual uint32_t code()const throw() { return _code; }                      \
      const string msg;                                                           \
   };

namespace wasm {
    WASM_DECLARE_EXCEPTION(wasm_exception,                       5000000, "wasm exception")
    WASM_DECLARE_EXCEPTION(abi_parse_exception,                  5000001, "abi parse exception")
    WASM_DECLARE_EXCEPTION(abi_serialization_deadline_exception, 5000002, "abi serialization deadline exception")
    WASM_DECLARE_EXCEPTION(unsupport_abi_version_exception,      5000003, "unsupport abi version exception")
    WASM_DECLARE_EXCEPTION(invalid_type_inside_abi,              5000004, "invalid type inside abi")
    WASM_DECLARE_EXCEPTION(duplicate_abi_def_exception,          5000005, "duplicate abi def exception")
    WASM_DECLARE_EXCEPTION(abi_circular_def_exception,           5000006, "abi circular def exception")
    WASM_DECLARE_EXCEPTION(transaction_exception,                5000007, "transaction exception")
    WASM_DECLARE_EXCEPTION(unpack_exception,                     5000008, "unpack exception")
    WASM_DECLARE_EXCEPTION(account_operation_exception,          5000009, "account operation exception")
    WASM_DECLARE_EXCEPTION(wasm_assert_exception,                5000010, "wasm assert exception")
    WASM_DECLARE_EXCEPTION(symbol_type_exception,                5000011, "symbol type exception")
    WASM_DECLARE_EXCEPTION(array_size_exceeds_exception,         5000012, "array size exceeds exception")
    WASM_DECLARE_EXCEPTION(pack_exception,                       5000013, "pack exception")
    WASM_DECLARE_EXCEPTION(inline_transaction_too_big,           5000013, "inline transaction too big")
} //wasm
