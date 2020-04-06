#include <test_api.hpp>
#include <../core/print.hpp>
#include <../core/check.hpp>
#include <../core/datastream.hpp>
#include <../core/name.hpp>

#define DUMMY_ACTION_DEFAULT_A 0x45
#define DUMMY_ACTION_DEFAULT_B 0xab11cd1244556677
#define DUMMY_ACTION_DEFAULT_C 0x7451ae12

void test_action::read_action_normal() {

   char buffer[100];
   uint32_t total = 0;

   check( action_data_size() == sizeof(dummy_action), "action_size() == sizeof(dummy_action)" );

   total = read_action_data( buffer, 30 );
   check( total == sizeof(dummy_action) , "read_action(30)" );

   total = read_action_data( buffer, 100 );
   check(total == sizeof(dummy_action) , "read_action(100)" );

   total = read_action_data(buffer, 5);
   check( total == 5 , "read_action(5)" );

   total = read_action_data(buffer, sizeof(dummy_action) );
   check( total == sizeof(dummy_action), "read_action(sizeof(dummy_action))" );

   dummy_action *dummy13 = reinterpret_cast<dummy_action *>(buffer);

   check( dummy13->a == DUMMY_ACTION_DEFAULT_A, "dummy13->a == DUMMY_ACTION_DEFAULT_A" );
   check( dummy13->b == DUMMY_ACTION_DEFAULT_B, "dummy13->b == DUMMY_ACTION_DEFAULT_B" );
   check( dummy13->c == DUMMY_ACTION_DEFAULT_C, "dummy13->c == DUMMY_ACTION_DEFAULT_C" );
}

void test_action::read_action_to_0() {
   read_action_data( (void *)0, action_data_size() );
}

void test_action::read_action_to_64k() {
   read_action_data( (void *)((1<<16)-2), action_data_size());
}

void test_action::require_notice( uint64_t receiver, uint64_t code, uint64_t action ) {
   (void)code;(void)action;
   if( receiver == "testapi"_n.value ) {
      wasm::notify_recipient( "acc1"_n );
      wasm::notify_recipient( "acc2"_n );
      wasm::notify_recipient( "acc1"_n, "acc2"_n );
      check( false, "Should've failed" );
   } else if ( receiver == "acc1"_n.value || receiver == "acc2"_n.value ) {
      return;
   }
   check( false, "Should've failed" );
}

void test_action::require_notice_tests( uint64_t receiver, uint64_t code, uint64_t action ) {
   wasm::print( "require_notice_tests" );
   if( receiver == "testapi"_n.value ) {
      wasm::print("notify_recipient( \"acc5\"_n )");
      wasm::notify_recipient("acc5"_n);
   } else if( receiver == "acc5"_n.value ) {
      wasm::print("notify_recipient( \"testapi\"_n )");
      wasm::notify_recipient("testapi"_n);
   }
}

// void test_action::require_auth() {
//    prints("require_auth");
//    wasm::require_auth("acc3"_n);
//    wasm::require_auth("acc4"_n);
// }

void test_action::assert_false() {
   check( false, "test_action::assert_false" );
}

void test_action::assert_true() {
   check( true, "test_action::assert_true" );
}


void test_action::test_abort() {
   abort();
   check( false, "should've aborted" );
}


void test_action::test_current_receiver( uint64_t receiver, uint64_t code, uint64_t action ) {
   (void)code;(void)action;
   name cur_rec;
   read_action_data( &cur_rec, sizeof(name) );

   check( receiver == cur_rec.value, "the current receiver does not match" );
}