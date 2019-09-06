#include <eosio/eosio.hpp>

extern "C" {
   int32_t _size() {
      return 0;
   }

   void _grow(int32_t) {
      return;
   }

   void sub_apply_0() {
      int32_t x1 = _size();
      eosio::check(x1 == (int32_t)1, "_size fail 1");

      _grow((int32_t)1);

      int32_t x3 = _size();
      eosio::check(x3 == (int32_t)2, "_size fail 3");

      _grow((int32_t)4);

      int32_t x5 = _size();
      eosio::check(x5 == (int32_t)6, "_size fail 5");

      _grow((int32_t)0);

      int32_t x7 = _size();
      eosio::check(x7 == (int32_t)6, "_size fail 7");

   }
   void apply(uint64_t, uint64_t, uint64_t test_to_run) {
      volatile uint64_t* r = (uint64_t*)0;
      *r = 0;
      switch(test_to_run) {
         case 0:
            sub_apply_0();
            break;
      }
   }
}
