#include <eosio/vm/watchdog.hpp>
#include <atomic>
#include <chrono>

#include <catch2/catch.hpp>

using eosio::vm::watchdog;

TEST_CASE("watchdog interrupt", "[watchdog_interrupt]") {
  std::atomic<bool> okay = false;
  watchdog w{std::chrono::milliseconds(50)};
  {
    auto g = w.scoped_run([&]() { okay = true; });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  CHECK(okay);
}

TEST_CASE("watchdog no interrupt", "[watchdog_no_interrupt]") {
  std::atomic<bool> okay = true;
  watchdog w{std::chrono::milliseconds(50)};
  {
    auto g = w.scoped_run([&]() { okay = false; });
  } // the guard goes out of scope here, cancelling the timer
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  CHECK(okay);
}
