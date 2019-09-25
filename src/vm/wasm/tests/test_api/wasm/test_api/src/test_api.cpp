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

      check( false, "Unknown Test" );

   }
}