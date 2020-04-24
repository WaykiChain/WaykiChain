#pragma once
#include <wasm/wasm_log.hpp>
#include <wasm/wasm_constants.hpp>
#include <wasm/wasm_trace.hpp>
#include <wasm/types/name.hpp>
#include <wasm/types/uint128.hpp>
#include "wasm_context.hpp"
#include <cassert>
#include <fstream>

#define WASM_CHECK( expr, msg )               \
if(! (expr)){                                \
    WASM_TRACE("%s%s", msg , "[ failed ]")   \
    assert(false);                           \
} else {                                     \
    WASM_TRACE("%s%s", msg , "[ passed ]")   \
}

#define WASM_CHECK_EQUAl( t1, t2 )   \
if(! (t1 == t2)){                     \
    WASM_TRACE("%s", "[ failed ]")   \
    assert(false);                   \
} else {                             \
    WASM_TRACE("%s", "[ passed ]")   \
}


#define WASM_CHECK_EXCEPTION( expr, passed, ex, msg ) \
 passed = false;                              \
 try{                                         \
     expr  ;                                  \
 } catch(wasm_chain::exception &e){                 \
     if( ex(CHAIN_LOG_MESSAGE( log_level::warn, msg )).code() == e.code()) {        \
         WASM_TRACE("%s%s exception: %s", msg , "[ passed ]", e.to_detail_string())   \
         passed = true;                       \
     }else {                                  \
          WASM_TRACE("%s", e.to_detail_string())        \
     }                                        \
 }  catch(...){                               \
    WASM_TRACE("%s", "exception")             \
 }                                            \
 if (!passed) {                               \
     WASM_TRACE("%s%s", msg, "[ failed ]")    \
     assert(false);                           \
 }

 #define CHECK_EXCEPTION( expr, passed, ex, msg ) \
 passed = false;                              \
 try{                                         \
     expr  ;                                  \
 } catch(wasm_chain::exception &e){                 \
     if( ex(CHAIN_LOG_MESSAGE( log_level::warn, msg )).code() == e.code() && (string(e.to_detail_string()).find(string(msg),0)) != std::string::npos) {  \
         WASM_TRACE("%s%s exception: %s", msg , "[ passed ]", e.to_detail_string())   \
         passed = true;                                                     \
     }else {                                                                \
        WASM_TRACE("%s", e.to_detail_string())         \
     }                                       \
 } catch(...){                               \
    WASM_TRACE("%s", "exception")             \
 }                                            \
 if (!passed) {                               \
     WASM_TRACE("%s%s", msg, "[ failed ]")    \
     assert(false);                           \
 }


string I64Str(int64_t i)
{
    std::stringstream ss;
    ss << i;
    return ss.str();
}

string U64Str(uint64_t i)
{
   std::stringstream ss;
   ss << i;
   return ss.str();
}

string U128Str(unsigned __int128 i)
{
   return string(uint128(i));
}

class base_tester {
public:
    typedef string action_result;
};

class validating_tester : public base_tester {
public:
    CUniversalTx ctrl;
    transaction_trace trace;
};


template<uint64_t NAME>
struct test_api_action {
    static uint64_t get_account() {
        return N(testapi);
    }

    static uint64_t get_name() {
        return NAME;
    }
};

template<typename T>
shared_ptr<transaction_trace> CallFunction( validating_tester &test, T ac, const vector<char> &data, const vector <uint64_t> &scope = {N(testapi)} ) {
    {
        //WASM_TRACE("%ld", data.size())

        shared_ptr<transaction_trace> trx_trace = make_shared<transaction_trace>(transaction_trace{});
        inline_transaction trx;
        trx.contract = ac.get_account();
        trx.action   = ac.get_name();
        trx.data     = data;

        //std::cout << "CallFunction:" << wasm::name(trx.action).to_string() << std::endl;

        for (auto a : scope) {
            trx.authorization.emplace_back(a, wasmio_owner);
        }

        try {
            test.ctrl.ExecuteTx(*trx_trace, trx);
        } catch (wasm_chain::exception &e) {
            //WASM_TRACE("%s", e.to_detail_string())
            //WASM_CHECK(false, e.detail())
            throw;
        }

        return trx_trace;
    }
}

void set_code( validating_tester &tester, uint64_t account, string codeFile ) {

    char byte;
    vector<uint8_t> code;
    ifstream f(codeFile, ios::binary);
    while (f.get(byte)) code.push_back(byte);

    tester.ctrl.cache.SetCode(account, code);

}

static constexpr unsigned int DJBH( const char *cp ) {
    unsigned int hash = 5381;
    while (*cp)
        hash = 33 * hash ^ (unsigned char) *cp++;
    return hash;
}

constexpr uint64_t TEST_METHOD( const char *CLASS, const char *METHOD ) {
    return ((uint64_t(DJBH(CLASS)) << 32) | uint32_t(DJBH(METHOD)));
}

struct dummy_action {
   static uint64_t get_name() {
      return N(dummyaction);
   }
   static uint64_t get_account() {
      return N(testapi);
   }

  char a; //1
  uint64_t b; //8
  int32_t  c; //4
};

/**
 *  Serialize a checksum512 type
 *
 *  @brief Serialize a checksum512 type
 *  @param ds - The stream to write
 *  @param cs - The value to serialize
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
*/
template<typename Stream>
inline datastream<Stream> &operator<<( datastream<Stream> &ds, const dummy_action &t ) {
    ds.write((const char *) &t, sizeof(t));
    return ds;
}

/**
 *  Deserialize a checksum512 type
 *
 *  @brief Deserialize a checksum512 type
 *  @param ds - The stream to read
 *  @param cs - The destination for deserialized value
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
template<typename Stream>
inline datastream<Stream> &operator>>( datastream<Stream> &ds, dummy_action &t ) {
    ds.read((char *) &t, sizeof(t));
    return ds;
}

#define CALL_TEST_FUNCTION( _TESTER, CLS, MTH, DATA ) CallFunction(_TESTER, test_api_action<TEST_METHOD(CLS, MTH)>{}, DATA)
#define CALL_TEST_FUNCTION_ACTION( _TESTER, MTH, DATA ) CallFunction(_TESTER, test_api_action<MTH>{}, DATA)
