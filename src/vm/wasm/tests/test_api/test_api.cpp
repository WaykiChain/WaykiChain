
#include"tester.hpp"

void print_tests(validating_tester &tester ) {

	set_code(tester, N(testapi), "wasm/test_api.wasm");


    // test prints
    CALL_TEST_FUNCTION( tester, "test_print", "test_prints", {});
    auto act_cnsl = tester.trace.traces.front().console;
    WASM_CHECK(act_cnsl == "abcefg", "test_prints")
    tester.trace.traces.erase(tester.trace.traces.begin());

    // test prints_l
    CALL_TEST_FUNCTION( tester, "test_print", "test_prints_l", {} );
    auto tx2_act_cnsl = tester.trace.traces.front().console;
    WASM_CHECK(tx2_act_cnsl == "abatest", "test_prints_l");
    tester.trace.traces.erase(tester.trace.traces.begin());

    // test printi
    CALL_TEST_FUNCTION( tester, "test_print", "test_printi", {} );
    auto tx3_act_cnsl = tester.trace.traces.front().console;
    WASM_CHECK( tx3_act_cnsl.substr(0,1) == I64Str(0),  "test_printi1");
    WASM_CHECK( tx3_act_cnsl.substr(1,6) == I64Str(556644) , "test_printi2");
    WASM_CHECK( tx3_act_cnsl.substr(7, std::string::npos) == I64Str(-1) , "test_printi3");
    tester.trace.traces.erase(tester.trace.traces.begin());

   // test printui
   CALL_TEST_FUNCTION( tester, "test_print", "test_printui", {} );
   auto tx4_act_cnsl = tester.trace.traces.front().console;
   WASM_CHECK( tx4_act_cnsl.substr(0,1) == U64Str(0), "test_printui1");
   WASM_CHECK( tx4_act_cnsl.substr(1,6) == U64Str(556644), "test_printui2");
   WASM_CHECK( tx4_act_cnsl.substr(7, std::string::npos) == U64Str(-1), "test_printui3"); // "18446744073709551615"
   tester.trace.traces.erase(tester.trace.traces.begin());


}


int main( int argc, char **argv ) {

    validating_tester tester;
    print_tests(tester);



}