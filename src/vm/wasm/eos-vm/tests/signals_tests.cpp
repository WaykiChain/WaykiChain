#include <eosio/vm/signals.hpp>
#include <chrono>
#include <csignal>
#include <thread>
#include <iostream>

#include <catch2/catch.hpp>

struct test_exception {};

TEST_CASE("Testing signals", "[invoke_with_signal_handler]") {
   bool okay = false;
   try {
      eosio::vm::invoke_with_signal_handler([]() {
         std::raise(SIGSEGV);
      }, [](int sig) {
         throw test_exception{};
      });
   } catch(test_exception&) {
      okay = true;
   }
   CHECK(okay);
}
