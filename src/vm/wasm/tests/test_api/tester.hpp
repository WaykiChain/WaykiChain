#pragma once
#include <wasm/wasm_log.hpp>
#include <wasm/wasm_config.hpp>
#include <wasm/wasm_trace.hpp>
#include <wasm/types/name.hpp>
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


#define WASM_CHECK_EXCEPTION( expr, passed, ex, msg ) \
 passed = false;                              \
 try{                                         \
     expr  ;                                  \
 } catch(wasm::exception &e){                 \
     if( ex(msg).code() == e.code()) {        \
         WASM_TRACE("%s%s exception: %s", msg , "[ passed ]", e.detail())   \
         passed = true;                       \
     }else {                                  \
          WASM_TRACE("%s", e.detail())        \
     }                                        \
 }  catch(...){                               \
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

class base_tester {
public:
    typedef string action_result;
};

class validating_tester : public base_tester {
public:
    CWasmContractTx ctrl;
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
void CallFunction( validating_tester &test, T ac, const vector<char> &data, const vector <uint64_t> &scope = {N(testapi)} ) {
    {
        //transaction_trace trx_trace;
        inline_transaction trx;
        trx.contract = ac.get_account();
        trx.action = ac.get_name();
        trx.data = data;

        for (auto a:scope) {
            trx.authorization.push_back(permission{a, wasmio_owner});
        }

        try {
            test.ctrl.ExecuteTx(test.trace, trx);
        } catch (wasm::exception &e) {
            //WASM_TRACE("%s", e.detail())
            WASM_CHECK(false, e.detail())
        }

        //return &trx_trace;
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

#define CALL_TEST_FUNCTION( _TESTER, CLS, MTH, DATA ) CallFunction(_TESTER, test_api_action<TEST_METHOD(CLS, MTH)>{}, DATA)