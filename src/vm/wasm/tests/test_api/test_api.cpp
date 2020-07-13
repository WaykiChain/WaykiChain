#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <boost/test/unit_test.hpp>
#pragma GCC diagnostic pop

#include"tester.hpp"
#include<limits>

extern void wasm_code_cache_free();

using namespace boost;

BOOST_AUTO_TEST_SUITE( test_api )

BOOST_FIXTURE_TEST_CASE( print_tests, validating_tester ) {

    set_code(*this, NAME(testapi), "wasm/test_api.wasm");

    // test prints
    auto tx1_trace = CALL_TEST_FUNCTION( *this, "test_print", "test_prints", {});
    auto tx1_act_cnsl = tx1_trace->traces.front().console;
    BOOST_CHECK_EQUAL(tx1_act_cnsl == "abcefg", true);

    // test prints_l
    auto tx2_trace = CALL_TEST_FUNCTION( *this, "test_print", "test_prints_l", {} );
    auto tx2_act_cnsl = tx2_trace->traces.front().console;
    BOOST_CHECK_EQUAL(tx2_act_cnsl == "abatest", true);

    // test printi
    auto tx3_trace = CALL_TEST_FUNCTION( *this, "test_print", "test_printi", {} );
    auto tx3_act_cnsl = tx3_trace->traces.front().console;
    BOOST_CHECK_EQUAL( tx3_act_cnsl.substr(0,1) , I64Str(0) );
    BOOST_CHECK_EQUAL( tx3_act_cnsl.substr(1,6) , I64Str(556644) );
    BOOST_CHECK_EQUAL( tx3_act_cnsl.substr(7, std::string::npos) , I64Str(-1) );

   // test printui
   auto tx4_trace = CALL_TEST_FUNCTION( *this, "test_print", "test_printui", {} );
   auto tx4_act_cnsl = tx4_trace->traces.front().console;
   BOOST_CHECK_EQUAL( tx4_act_cnsl.substr(0,1) , U64Str(0) );
   BOOST_CHECK_EQUAL( tx4_act_cnsl.substr(1,6) , U64Str(556644) );
   BOOST_CHECK_EQUAL( tx4_act_cnsl.substr(7, std::string::npos) , U64Str(-1) ); // "18446744073709551615"


   // test printn
   auto tx5_trace = CALL_TEST_FUNCTION( *this, "test_print", "test_printn", {} );
   auto tx5_act_cnsl = tx5_trace->traces.front().console;

   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(0,1), "1" );
   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(1,1), "5" );
   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(2,1), "a" );
   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(3,1), "z" );

   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(4,3), "abc" );
   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(7,3), "123" );

   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(10,7), "abc.123" );
   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(17,7), "123.abc" );

   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(24,13), "12345abcdefgj" );
   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(37,13), "ijklmnopqrstj" );
   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(50,13), "vwxyz.12345aj" );

   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(63, 13), "111111111111j" );
   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(76, 13), "555555555555j" );
   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(89, 13), "aaaaaaaaaaaaj" );
   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(102,13), "zzzzzzzzzzzzj" );

   //test printi128
   auto tx6_trace = CALL_TEST_FUNCTION( *this, "test_print", "test_printi128", {} );
   auto tx6_act_cnsl = tx6_trace->traces.front().console;
   size_t start = 0;
   size_t end = tx6_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx6_act_cnsl.substr(start, end-start), U128Str(1) );
   start = end + 1; end = tx6_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx6_act_cnsl.substr(start, end-start), U128Str(0) );
   start = end + 1; end = tx6_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx6_act_cnsl.substr(start, end-start), "-" + U128Str(static_cast<unsigned __int128>(std::numeric_limits<__int128>::lowest())) );
   start = end + 1; end = tx6_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx6_act_cnsl.substr(start, end-start), "-" + U128Str(87654323456) );


   // // test printui128
   auto tx7_trace = CALL_TEST_FUNCTION( *this, "test_print", "test_printui128", {} );
   auto tx7_act_cnsl = tx7_trace->traces.front().console;
   start = 0; end = tx7_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx7_act_cnsl.substr(start, end-start), U128Str(std::numeric_limits<unsigned __int128>::max()) );
   start = end + 1; end = tx7_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx7_act_cnsl.substr(start, end-start), U128Str(0) );
   start = end + 1; end = tx7_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx7_act_cnsl.substr(start, end-start), U128Str(87654323456) );

   // test printsf
   auto tx8_trace = CALL_TEST_FUNCTION( *this, "test_print", "test_printsf", {} );
   auto tx8_act_cnsl = tx8_trace->traces.front().console;
   start = 0; end = tx8_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx8_act_cnsl.substr(start, end-start), "5.000000e-01" );
   start = end + 1; end = tx8_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx8_act_cnsl.substr(start, end-start), "-3.750000e+00" );
   start = end + 1; end = tx8_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx8_act_cnsl.substr(start, end-start), "6.666667e-07" );

   // test printdf
   auto tx9_trace = CALL_TEST_FUNCTION( *this, "test_print", "test_printdf", {} );
   auto tx9_act_cnsl = tx9_trace->traces.front().console;
   start = 0; end = tx9_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx9_act_cnsl.substr(start, end-start), "5.000000000000000e-01" );
   start = end + 1; end = tx9_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx9_act_cnsl.substr(start, end-start), "-3.750000000000000e+00" );
   start = end + 1; end = tx9_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx9_act_cnsl.substr(start, end-start), "6.666666666666666e-07" );

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
   auto tx10_trace = CALL_TEST_FUNCTION( *this, "test_print", "test_printqf", {} );
   auto tx10_act_cnsl = tx10_trace->traces.front().console;
   start = 0; end = tx10_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx10_act_cnsl.substr(start, end-start), expect1 );
   start = end + 1; end = tx10_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx10_act_cnsl.substr(start, end-start), expect2 );
   start = end + 1; end = tx10_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx10_act_cnsl.substr(start, end-start), expect3 );


}

BOOST_FIXTURE_TEST_CASE( types_tests, validating_tester ) {
	set_code(*this, NAME(testapi), "wasm/test_api.wasm");
	CALL_TEST_FUNCTION( *this, "test_types", "types_size", {});
	CALL_TEST_FUNCTION( *this, "test_types", "char_to_symbol", {});
	CALL_TEST_FUNCTION( *this, "test_types", "string_to_name", {});

}

BOOST_FIXTURE_TEST_CASE( datastream_tests, validating_tester ) {
	set_code(*this, NAME(testapi), "wasm/test_api.wasm");
	CALL_TEST_FUNCTION( *this, "test_datastream", "test_basic", {} );

}

#define DUMMY_ACTION_DEFAULT_A 0x45
#define DUMMY_ACTION_DEFAULT_B 0xab11cd1244556677
#define DUMMY_ACTION_DEFAULT_C 0x7451ae12

BOOST_FIXTURE_TEST_CASE( action_tests, validating_tester ) {

    set_code(*this, NAME(testapi), "wasm/test_api.wasm");
    CALL_TEST_FUNCTION( *this, "test_action", "assert_true", {} );

    bool passed;
    // test assert_false
    CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "assert_false", {} ), passed, wasm_assert_exception, "test_action::assert_false")

    // test read_action_normal
    dummy_action dummy13{DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C};
    CALL_TEST_FUNCTION( *this, "test_action", "read_action_normal", wasm::pack(dummy13));

    // test read_action_to_0
    std::vector<char> raw_bytes((1<<16));
    CALL_TEST_FUNCTION( *this, "test_action", "read_action_to_0", raw_bytes );

    raw_bytes.resize((1<<16)+1);
    CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "read_action_to_0", raw_bytes), passed, wasm_chain::wasm_api_data_size_exceeds_exception, "size")

    // test read_action_to_64k
    raw_bytes.resize(1);
    CALL_TEST_FUNCTION( *this, "test_action", "read_action_to_64k", raw_bytes );

    //test read_action_to_64k
    raw_bytes.resize(3);
    CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "read_action_to_64k", raw_bytes), passed, wasm_chain::wasm_memory_exception, "access violation")

    //test require_notice
    auto test_require_notice = [](auto& test, std::vector<char>& data){
      CALL_TEST_FUNCTION( test, "test_action", "require_notice", data);
    };

    CHECK_EXCEPTION(test_require_notice(*this, raw_bytes), passed, wasm_assert_exception, "Should've failed")

    // test test_current_receiver
    CALL_TEST_FUNCTION( *this, "test_action", "test_current_receiver", wasm::pack(NAME(testapi)));

    // test test_abort
    CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "test_abort", {} ), passed, abort_called, "abort() called")

    wasm_code_cache_free();
}

BOOST_FIXTURE_TEST_CASE( require_notice_tests, validating_tester ) {
  set_code(*this, NAME(testapi), "wasm/test_api.wasm");
  set_code(*this, NAME(acc5),    "wasm/test_api.wasm");
  CALL_TEST_FUNCTION( *this, "test_action", "require_notice_tests", {});

  wasm_code_cache_free();
}

BOOST_AUTO_TEST_SUITE_END()
