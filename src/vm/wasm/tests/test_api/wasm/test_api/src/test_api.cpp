#include <test_api.hpp>


extern "C" {
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) {


      //test_types
      WASM_TEST_HANDLER( test_types, types_size     );
      WASM_TEST_HANDLER( test_types, char_to_symbol );
      WASM_TEST_HANDLER( test_types, string_to_name );

      //test_print
      WASM_TEST_HANDLER( test_print, test_prints     );
      WASM_TEST_HANDLER( test_print, test_prints_l   );
      WASM_TEST_HANDLER( test_print, test_printi     );
      WASM_TEST_HANDLER( test_print, test_printui    );
      WASM_TEST_HANDLER( test_print, test_printi128  );
      WASM_TEST_HANDLER( test_print, test_printui128 );
      WASM_TEST_HANDLER( test_print, test_printn     );
      WASM_TEST_HANDLER( test_print, test_printsf    );
      WASM_TEST_HANDLER( test_print, test_printdf    );
      WASM_TEST_HANDLER( test_print, test_printqf    );

      // test datastream
      WASM_TEST_HANDLER( test_datastream, test_basic );

      //test_action
      WASM_TEST_HANDLER   ( test_action, read_action_normal         );
      WASM_TEST_HANDLER   ( test_action, read_action_to_0           );
      WASM_TEST_HANDLER   ( test_action, read_action_to_64k         );
      WASM_TEST_HANDLER_EX( test_action, require_notice             );
      WASM_TEST_HANDLER_EX( test_action, require_notice_tests       );
      // WASM_TEST_HANDLER   ( test_action, require_auth               );
      WASM_TEST_HANDLER   ( test_action, assert_false               );
      WASM_TEST_HANDLER   ( test_action, assert_true                );
      // WASM_TEST_HANDLER   ( test_action, test_current_time          );
      WASM_TEST_HANDLER   ( test_action, test_abort                 );
      WASM_TEST_HANDLER_EX( test_action, test_current_receiver      );
      // WASM_TEST_HANDLER   ( test_action, test_publication_time      );
      // WASM_TEST_HANDLER   ( test_action, test_assert_code           );
      // WASM_TEST_HANDLER_EX( test_action, test_ram_billing_in_notify );

      check( false, "Unknown Test" );

   }
}