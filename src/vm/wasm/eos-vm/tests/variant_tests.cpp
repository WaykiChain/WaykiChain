#include <eosio/vm/debug_visitor.hpp>
#include <eosio/vm/dispatcher.hpp>
#include <eosio/vm/variant.hpp>

#include <catch2/catch.hpp>

using namespace eosio;
using namespace eosio::vm;

TEST_CASE("Testing variant with stateless class", "[variant_stateless_tests]") {
   struct vis {
      int operator()(int v) {
         std::cout << "Int " << v << "\n";
         return v + 10;
      }
      int operator()(float v) { return v + 10.0f; }
      int operator()(double v) { return v + 13.0; }
      int operator()(uint32_t v) { return v + 234; }
      int operator()(uint64_t v) {
         std::cout << "I64 " << v << "\n";
         return v + 13;
      }
   };

   variant<int, float, double, uint32_t, uint64_t> v((uint64_t)53);
   std::cout << "VISIT " << visit(vis{}, v) << "\n";
   opcode2        opc(i32_const_t{ (uint32_t)32 });
   debug_visitor2 dv;
   DISPATCH(dv, opc);
}

TEST_CASE("Testing variant with lambda", "[variant_lambda_tests]") {
   variant<int, float, double, uint64_t> v((int)53);
   uint64_t                              s = 33;
   visit(overloaded{ [](int v) { std::cout << "Visiting int from lambda " << v << "\n"; },
                     [](float v) { std::cout << "Visiting float from lambda " << v << "\n"; },
                     [](double v) { std::cout << "Visiting double from lambda " << v << "\n"; },
                     [&](auto v) {
                        std::cout << "Visiting other \n";
                        s += 15;
                     } },
         v);
   std::cout << "S " << s << "\n";
}
