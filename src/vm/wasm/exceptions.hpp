#pragma once

#include <cstdint>
#include <string>
// #include "commons/tinyformat.h"

using namespace std;

namespace wasm {

    static const uint8_t WASM_ASSERT_FAIL = 0x71;
    static const uint8_t ABI_PARSE_FAIL = 0x72;
    static const uint8_t ABI_SERIALIZATION_DEADLINE_EXCEPTION = 0x73;
    static const uint8_t UNSUPPORTED_ABI_VERSION_EXCEPTION = 0x74;
    static const uint8_t INVALID_TYPE_INSIDE_ABI = 0x75;
    static const uint8_t DUPLICATE_ABI_DEF_EXCEPTION = 0x76;
    static const uint8_t ABI_CIRCULAR_DEF_EXCEPTION = 0x77;
    static const uint8_t TRANSACTION_EXCEPTION = 0x78;
    static const uint8_t UNPACK_EXCEPTION = 0x79;

    class CException {
    public:
        uint64_t errCode;
        string errMsg;
        string errShort;

        CException( uint64_t code, string msg, string s ) : errCode(code), errMsg(msg), errShort(s) {}
    };


#define WASM_ASSERT( expr, errCode, errShort, ... )    \
   if( !( expr ) )  {                                  \
       char buf[1024];                               \
       sprintf( buf,  __VA_ARGS__ );            \
       throw CException( errCode, buf, errShort) ; }

#define WASM_THROW( errCode, errShort, ... )    \
       WASM_ASSERT( false, errCode, errShort, __VA_ARGS__ )


#define WASM_CAPTURE_AND_RETHROW( ... )                                \
   catch( CException& e ) {                                            \
       throw CException( ABI_PARSE_FAIL, e.errMsg, "WASM_ABI_PARSE_FAIL"); \
    } catch( ... ) {                                                     \
         char buf[1024];                                                 \
         sprintf( buf,  __VA_ARGS__ );                                   \
         throw CException( ABI_PARSE_FAIL, buf, "WASM_ABI_PARSE_FAIL" );  \
    }


#define EOS_RETHROW_EXCEPTIONS( errCode, errShort, ... ) \
    catch( ... ) {                                        \
         char buf[1024];                                  \
         sprintf( buf,  __VA_ARGS__ );                    \
         throw CException( errCode, buf, errShort );      \
    }

}//wasm
  
