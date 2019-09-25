
#include"tester.hpp"

void print_tests(validating_tester &tester ) {

	set_code(tester, N(testapi), "wasm/test_api.wasm");


    // test prints
    auto tx1_trace = CALL_TEST_FUNCTION( tester, "test_print", "test_prints", {});
    auto tx1_act_cnsl = tx1_trace->traces.front().console;
    WASM_CHECK(tx1_act_cnsl == "abcefg", "test_prints")

    // test prints_l
    auto tx2_trace = CALL_TEST_FUNCTION( tester, "test_print", "test_prints_l", {} );
    auto tx2_act_cnsl = tx2_trace->traces.front().console;
    WASM_CHECK(tx2_act_cnsl == "abatest", "test_prints_l");

    // test printi
    auto tx3_trace = CALL_TEST_FUNCTION( tester, "test_print", "test_printi", {} );
    auto tx3_act_cnsl = tx3_trace->traces.front().console;
    WASM_CHECK( tx3_act_cnsl.substr(0,1) == I64Str(0),  "test_printi1");
    WASM_CHECK( tx3_act_cnsl.substr(1,6) == I64Str(556644) , "test_printi2");
    WASM_CHECK( tx3_act_cnsl.substr(7, std::string::npos) == I64Str(-1) , "test_printi3");

   // test printui
   auto tx4_trace = CALL_TEST_FUNCTION( tester, "test_print", "test_printui", {} );
   auto tx4_act_cnsl = tx4_trace->traces.front().console;
   WASM_CHECK( tx4_act_cnsl.substr(0,1) == U64Str(0), "test_printui1");
   WASM_CHECK( tx4_act_cnsl.substr(1,6) == U64Str(556644), "test_printui2");
   WASM_CHECK( tx4_act_cnsl.substr(7, std::string::npos) == U64Str(-1), "test_printui3"); // "18446744073709551615"


   // test printn
   auto tx5_trace = CALL_TEST_FUNCTION( tester, "test_print", "test_printn", {} );
   auto tx5_act_cnsl = tx5_trace->traces.front().console;

   WASM_CHECK_EQUAl( tx5_act_cnsl.substr(0,1), "1" );
   WASM_CHECK_EQUAl( tx5_act_cnsl.substr(1,1), "5" );
   WASM_CHECK_EQUAl( tx5_act_cnsl.substr(2,1), "a" );
   WASM_CHECK_EQUAl( tx5_act_cnsl.substr(3,1), "z" );

   WASM_CHECK_EQUAl( tx5_act_cnsl.substr(4,3), "abc" );
   WASM_CHECK_EQUAl( tx5_act_cnsl.substr(7,3), "123" );

   WASM_CHECK_EQUAl( tx5_act_cnsl.substr(10,7), "abc.123" );
   WASM_CHECK_EQUAl( tx5_act_cnsl.substr(17,7), "123.abc" );

   WASM_CHECK_EQUAl( tx5_act_cnsl.substr(24,13), "12345abcdefgj" );
   WASM_CHECK_EQUAl( tx5_act_cnsl.substr(37,13), "ijklmnopqrstj" );
   WASM_CHECK_EQUAl( tx5_act_cnsl.substr(50,13), "vwxyz.12345aj" );

   WASM_CHECK_EQUAl( tx5_act_cnsl.substr(63, 13), "111111111111j" );
   WASM_CHECK_EQUAl( tx5_act_cnsl.substr(76, 13), "555555555555j" );
   WASM_CHECK_EQUAl( tx5_act_cnsl.substr(89, 13), "aaaaaaaaaaaaj" );
   WASM_CHECK_EQUAl( tx5_act_cnsl.substr(102,13), "zzzzzzzzzzzzj" );


}


int main( int argc, char **argv ) {

    validating_tester tester;
    print_tests(tester);

}