#include <eosio/vm/guarded_ptr.hpp>

#include <catch2/catch.hpp>

using namespace eosio::vm;

struct S { int value; };

TEST_CASE("Testing guarded_ptr", "[guarded_ptr_tests]") {
   int a[10];
   {
      guarded_ptr<int> ptr(a, sizeof(a)/sizeof(a[0]));
      ptr += 10;
      CHECK(ptr.raw() == a + 10);
   }
   {
      guarded_ptr<int> ptr(a, sizeof(a)/sizeof(a[0]));
      CHECK_THROWS_AS(ptr += 11, guarded_ptr_exception);
   }
   {
      guarded_ptr<int> ptr(a, sizeof(a)/sizeof(a[0]));
      ptr += 0;
      CHECK(ptr.raw() == a + 0);
   }
   // operator++
   {
      guarded_ptr<int> ptr(a, 1);
      auto& result = ++ptr;
      CHECK(ptr.raw() == a + 1);
      CHECK(&result == &ptr);
   }
   {
      guarded_ptr<int> ptr(a, 0);
      CHECK_THROWS_AS(++ptr, guarded_ptr_exception);
   }
   {
      guarded_ptr<int> ptr(a, 1);
      auto result = ptr++;
      CHECK(ptr.raw() == a + 1);
      CHECK(result.raw() == a);
   }
   {
      guarded_ptr<int> ptr(a, 0);
      CHECK_THROWS_AS(ptr++, guarded_ptr_exception);
   }
   // operator+
   {
      const guarded_ptr<int> ptr(a, 10);
      auto result = ptr + 10;
      CHECK(result.raw() == a + 10);
   }
   {
      const guarded_ptr<int> ptr(a, 10);
      CHECK_THROWS_AS(ptr + 11, guarded_ptr_exception);
   }
   {
      const guarded_ptr<int> ptr(a, 10);
      auto result = 10 + ptr;
      CHECK(result.raw() == a + 10);
   }
   {
      const guarded_ptr<int> ptr(a, 10);
      CHECK_THROWS_AS(11 + ptr, guarded_ptr_exception);
   }
   // operator*
   {
      const guarded_ptr<int> ptr(a, 1);
      *ptr = 42;
      CHECK(a[0] == 42);
   }
   {
      const guarded_ptr<int> ptr(a, 0);
      CHECK_THROWS_AS(*ptr, guarded_ptr_exception);
   }
   // operator->
   S s[10];
   {
      const guarded_ptr<S> ptr(s, 1);
      ptr->value = 42;
      CHECK(s[0].value == 42);
   }
   {
      const guarded_ptr<S> ptr(s, 0);
      CHECK_THROWS_AS(ptr->value, guarded_ptr_exception);
   }
   // at
   {
      const guarded_ptr<int> ptr(a, 1);
      a[0] = 42;
      CHECK(ptr.at() == 42);
   }
   {
      const guarded_ptr<int> ptr(a, 0);
      CHECK_THROWS_AS(ptr.at(), guarded_ptr_exception);
   }
   {
      const guarded_ptr<int> ptr(a, 10);
      a[9] = 43;
      CHECK(ptr.at(9) == 43);
   }
   {
      const guarded_ptr<int> ptr(a, 10);
      CHECK_THROWS_AS(ptr.at(10), guarded_ptr_exception);
   }
   // TODO: add_bounds/fit_bounds/bounds/offset
}
