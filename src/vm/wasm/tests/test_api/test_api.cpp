
#include"tester.hpp"
#include<limits>

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

   //test printi128
   auto tx6_trace = CALL_TEST_FUNCTION( tester, "test_print", "test_printi128", {} );
   auto tx6_act_cnsl = tx6_trace->traces.front().console;
   size_t start = 0;
   size_t end = tx6_act_cnsl.find('\n', start);
   WASM_CHECK_EQUAl( tx6_act_cnsl.substr(start, end-start), U128Str(1) );
   start = end + 1; end = tx6_act_cnsl.find('\n', start);
   WASM_CHECK_EQUAl( tx6_act_cnsl.substr(start, end-start), U128Str(0) );
   start = end + 1; end = tx6_act_cnsl.find('\n', start);
   WASM_CHECK_EQUAl( tx6_act_cnsl.substr(start, end-start), "-" + U128Str(static_cast<unsigned __int128>(std::numeric_limits<__int128>::lowest())) );
   start = end + 1; end = tx6_act_cnsl.find('\n', start);
   WASM_CHECK_EQUAl( tx6_act_cnsl.substr(start, end-start), "-" + U128Str(87654323456) );


   // // test printui128
   auto tx7_trace = CALL_TEST_FUNCTION( tester, "test_print", "test_printui128", {} );
   auto tx7_act_cnsl = tx7_trace->traces.front().console;
   start = 0; end = tx7_act_cnsl.find('\n', start);
   WASM_CHECK_EQUAl( tx7_act_cnsl.substr(start, end-start), U128Str(std::numeric_limits<unsigned __int128>::max()) );
   start = end + 1; end = tx7_act_cnsl.find('\n', start);
   WASM_CHECK_EQUAl( tx7_act_cnsl.substr(start, end-start), U128Str(0) );
   start = end + 1; end = tx7_act_cnsl.find('\n', start);
   WASM_CHECK_EQUAl( tx7_act_cnsl.substr(start, end-start), U128Str(87654323456) );

   // test printsf
   auto tx8_trace = CALL_TEST_FUNCTION( tester, "test_print", "test_printsf", {} );
   auto tx8_act_cnsl = tx8_trace->traces.front().console;
   start = 0; end = tx8_act_cnsl.find('\n', start);
   WASM_CHECK_EQUAl( tx8_act_cnsl.substr(start, end-start), "5.000000e-01" );
   start = end + 1; end = tx8_act_cnsl.find('\n', start);
   WASM_CHECK_EQUAl( tx8_act_cnsl.substr(start, end-start), "-3.750000e+00" );
   start = end + 1; end = tx8_act_cnsl.find('\n', start);
   WASM_CHECK_EQUAl( tx8_act_cnsl.substr(start, end-start), "6.666667e-07" );

   // test printdf
   auto tx9_trace = CALL_TEST_FUNCTION( tester, "test_print", "test_printdf", {} );
   auto tx9_act_cnsl = tx9_trace->traces.front().console;
   start = 0; end = tx9_act_cnsl.find('\n', start);
   WASM_CHECK_EQUAl( tx9_act_cnsl.substr(start, end-start), "5.000000000000000e-01" );
   start = end + 1; end = tx9_act_cnsl.find('\n', start);
   WASM_CHECK_EQUAl( tx9_act_cnsl.substr(start, end-start), "-3.750000000000000e+00" );
   start = end + 1; end = tx9_act_cnsl.find('\n', start);
   WASM_CHECK_EQUAl( tx9_act_cnsl.substr(start, end-start), "6.666666666666666e-07" );

   // test printqf
#ifdef __x86_64__
   std::string expect1 = "5.000000000000000000e-01";
   std::string expect2 = "-3.750000000000000000e+00";
   std::string expect3 = "6.666666666666666667e-07";
#else
   std::string expect1 = "5.000000000000000e-01";
   std::string expect2 = "-3.750000000000000e+00";
   std::string expect3 = "6.666666666666667e-07";
#endif
   auto tx10_trace = CALL_TEST_FUNCTION( tester, "test_print", "test_printqf", {} );
   auto tx10_act_cnsl = tx10_trace->traces.front().console;
   start = 0; end = tx10_act_cnsl.find('\n', start);
   WASM_CHECK_EQUAl( tx10_act_cnsl.substr(start, end-start), expect1 );
   start = end + 1; end = tx10_act_cnsl.find('\n', start);
   WASM_CHECK_EQUAl( tx10_act_cnsl.substr(start, end-start), expect2 );
   start = end + 1; end = tx10_act_cnsl.find('\n', start);
   WASM_CHECK_EQUAl( tx10_act_cnsl.substr(start, end-start), expect3 );

}

void types_tests(validating_tester &tester ) {
	set_code(tester, N(testapi), "wasm/test_api.wasm");
	CALL_TEST_FUNCTION( tester, "test_types", "types_size", {});
	CALL_TEST_FUNCTION( tester, "test_types", "char_to_symbol", {});
	CALL_TEST_FUNCTION( tester, "test_types", "string_to_name", {});
}

void datastream_tests(validating_tester &tester ) {
	set_code(tester, N(testapi), "wasm/test_api.wasm");
	CALL_TEST_FUNCTION( tester, "test_datastream", "test_basic", {} );
}

#define DUMMY_ACTION_DEFAULT_A 0x45
#define DUMMY_ACTION_DEFAULT_B 0xab11cd1244556677
#define DUMMY_ACTION_DEFAULT_C 0x7451ae12

void action_tests(validating_tester &tester ) {
  set_code(tester, N(testapi), "wasm/test_api.wasm");
  CALL_TEST_FUNCTION( tester, "test_action", "assert_true", {} );

  bool passed;
  CHECK_EXCEPTION(CALL_TEST_FUNCTION( tester, "test_action", "assert_false", {} ), passed, wasm_exception, "test_action::assert_false")

  dummy_action dummy13{DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C};
  CALL_TEST_FUNCTION( tester, "test_action", "read_action_normal", wasm::pack(dummy13));
}


int main( int argc, char **argv ) {

    validating_tester tester;
    print_tests(tester);
    types_tests(tester);
    datastream_tests(tester);

    action_tests(tester);

}