#include <eosio/eosio.hpp>

extern "C" {
   int32_t _size() {
      return 0;
   }

   void _store_at_zero() {
      return;
   }

   int32_t _load_at_zero() {
      return 0;
   }

   void _store_at_page_size() {
      return;
   }

   int32_t _load_at_page_size() {
      return 0;
   }

   int32_t _grow(int32_t) {
      return 0;
   }

   void sub_apply_0() {
      _store_at_zero();

   }
   void sub_apply_1() {
      _load_at_zero();

   }
   void sub_apply_2() {
      _store_at_page_size();

   }
   void sub_apply_3() {
      _load_at_page_size();

   }
   void sub_apply_4() {
      _store_at_page_size();

   }
   void sub_apply_5() {
      _load_at_page_size();

   }
   void sub_apply_6() {
      int32_t x1 = _size();
      eosio::check(x1 == (int32_t)1, "_size fail 1");

      int32_t x2 = _grow((int32_t)1);
      eosio::check(x2 == (int32_t)1, "_grow fail 2");

      int32_t x3 = _size();
      eosio::check(x3 == (int32_t)2, "_size fail 3");

      int32_t x4 = _load_at_zero();
      eosio::check(x4 == (int32_t)0, "_load_at_zero fail 4");

      _store_at_zero();

      int32_t x6 = _load_at_zero();
      eosio::check(x6 == (int32_t)2, "_load_at_zero fail 6");

      int32_t x7 = _grow((int32_t)4);
      eosio::check(x7 == (int32_t)2, "_grow fail 7");

      int32_t x8 = _size();
      eosio::check(x8 == (int32_t)6, "_size fail 8");

      int32_t x9 = _load_at_zero();
      eosio::check(x9 == (int32_t)2, "_load_at_zero fail 9");

      _store_at_zero();

      int32_t x11 = _load_at_zero();
      eosio::check(x11 == (int32_t)2, "_load_at_zero fail 11");

      int32_t x12 = _load_at_page_size();
      eosio::check(x12 == (int32_t)0, "_load_at_page_size fail 12");

      _store_at_page_size();

      int32_t x14 = _load_at_page_size();
      eosio::check(x14 == (int32_t)3, "_load_at_page_size fail 14");

   }
   void apply(uint64_t, uint64_t, uint64_t test_to_run) {
      volatile uint64_t* r = (uint64_t*)0;
      *r = 0;
      switch(test_to_run) {
         case 0:
            sub_apply_0();
            break;
         case 1:
            sub_apply_1();
            break;
         case 2:
            sub_apply_2();
            break;
         case 3:
            sub_apply_3();
            break;
         case 4:
            sub_apply_4();
            break;
         case 5:
            sub_apply_5();
            break;
         case 6:
            sub_apply_6();
            break;
      }
   }
}
