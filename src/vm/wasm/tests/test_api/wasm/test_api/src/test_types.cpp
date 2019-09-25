#include <test_api.hpp>

void test_types::types_size() {

   check( sizeof(int64_t)   ==  8, "int64_t size != 8"   );
   check( sizeof(uint64_t)  ==  8, "uint64_t size != 8"  );
   check( sizeof(uint32_t)  ==  4, "uint32_t size != 4"  );
   check( sizeof(int32_t)   ==  4, "int32_t size != 4"   );
   check( sizeof(uint128_t) == 16, "uint128_t size != 16");
   check( sizeof(int128_t)  == 16, "int128_t size != 16" );
   check( sizeof(uint8_t)   ==  1, "uint8_t size != 1"   );

   check( sizeof(wasm::name) ==  8, "name size !=  8");
}

void test_types::char_to_symbol() {

   check( wasm::name::char_to_value('1') ==  1, "wasm::char_to_symbol('1') !=  1" );
   check( wasm::name::char_to_value('2') ==  2, "wasm::char_to_symbol('2') !=  2" );
   check( wasm::name::char_to_value('3') ==  3, "wasm::char_to_symbol('3') !=  3" );
   check( wasm::name::char_to_value('4') ==  4, "wasm::char_to_symbol('4') !=  4" );
   check( wasm::name::char_to_value('5') ==  5, "wasm::char_to_symbol('5') !=  5" );
   check( wasm::name::char_to_value('a') ==  6, "wasm::char_to_symbol('a') !=  6" );
   check( wasm::name::char_to_value('b') ==  7, "wasm::char_to_symbol('b') !=  7" );
   check( wasm::name::char_to_value('c') ==  8, "wasm::char_to_symbol('c') !=  8" );
   check( wasm::name::char_to_value('d') ==  9, "wasm::char_to_symbol('d') !=  9" );
   check( wasm::name::char_to_value('e') == 10, "wasm::char_to_symbol('e') != 10" );
   check( wasm::name::char_to_value('f') == 11, "wasm::char_to_symbol('f') != 11" );
   check( wasm::name::char_to_value('g') == 12, "wasm::char_to_symbol('g') != 12" );
   check( wasm::name::char_to_value('h') == 13, "wasm::char_to_symbol('h') != 13" );
   check( wasm::name::char_to_value('i') == 14, "wasm::char_to_symbol('i') != 14" );
   check( wasm::name::char_to_value('j') == 15, "wasm::char_to_symbol('j') != 15" );
   check( wasm::name::char_to_value('k') == 16, "wasm::char_to_symbol('k') != 16" );
   check( wasm::name::char_to_value('l') == 17, "wasm::char_to_symbol('l') != 17" );
   check( wasm::name::char_to_value('m') == 18, "wasm::char_to_symbol('m') != 18" );
   check( wasm::name::char_to_value('n') == 19, "wasm::char_to_symbol('n') != 19" );
   check( wasm::name::char_to_value('o') == 20, "wasm::char_to_symbol('o') != 20" );
   check( wasm::name::char_to_value('p') == 21, "wasm::char_to_symbol('p') != 21" );
   check( wasm::name::char_to_value('q') == 22, "wasm::char_to_symbol('q') != 22" );
   check( wasm::name::char_to_value('r') == 23, "wasm::char_to_symbol('r') != 23" );
   check( wasm::name::char_to_value('s') == 24, "wasm::char_to_symbol('s') != 24" );
   check( wasm::name::char_to_value('t') == 25, "wasm::char_to_symbol('t') != 25" );
   check( wasm::name::char_to_value('u') == 26, "wasm::char_to_symbol('u') != 26" );
   check( wasm::name::char_to_value('v') == 27, "wasm::char_to_symbol('v') != 27" );
   check( wasm::name::char_to_value('w') == 28, "wasm::char_to_symbol('w') != 28" );
   check( wasm::name::char_to_value('x') == 29, "wasm::char_to_symbol('x') != 29" );
   check( wasm::name::char_to_value('y') == 30, "wasm::char_to_symbol('y') != 30" );
   check( wasm::name::char_to_value('z') == 31, "wasm::char_to_symbol('z') != 31" );

   for(unsigned char i = 0; i<255; i++) {
      if( (i >= 'a' && i <= 'z') || (i >= '1' || i <= '5') ) continue;
      check( wasm::name::char_to_value((char)i) == 0, "wasm::char_to_symbol() != 0" );
   }
}

void test_types::string_to_name() {
   return;
   check( wasm::name("a") == "a"_n, "wasm::string_to_name(a)" );
   check( wasm::name("ba") == "ba"_n, "wasm::string_to_name(ba)" );
   check( wasm::name("cba") == "cba"_n, "wasm::string_to_name(cba)" );
   check( wasm::name("dcba") == "dcba"_n, "wasm::string_to_name(dcba)" );
   check( wasm::name("edcba") == "edcba"_n, "wasm::string_to_name(edcba)" );
   check( wasm::name("fedcba") == "fedcba"_n, "wasm::string_to_name(fedcba)" );
   check( wasm::name("gfedcba") == "gfedcba"_n, "wasm::string_to_name(gfedcba)" );
   check( wasm::name("hgfedcba") == "hgfedcba"_n, "wasm::string_to_name(hgfedcba)" );
   check( wasm::name("ihgfedcba") == "ihgfedcba"_n, "wasm::string_to_name(ihgfedcba)" );
   check( wasm::name("jihgfedcba") == "jihgfedcba"_n, "wasm::string_to_name(jihgfedcba)" );
   check( wasm::name("kjihgfedcba") == "kjihgfedcba"_n, "wasm::string_to_name(kjihgfedcba)" );
   check( wasm::name("lkjihgfedcba") == "lkjihgfedcba"_n, "wasm::string_to_name(lkjihgfedcba)" );
   check( wasm::name("mlkjihgfedcba") == "mlkjihgfedcba"_n, "wasm::string_to_name(mlkjihgfedcba)" );
}
