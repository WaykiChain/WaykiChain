#pragma once

#include <eosio/vm/error_codes_def.hpp>

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

namespace eosio { namespace vm {
   //TODO use Spoons accurate timer
   template <typename TimeUnits>
   class watchdog {
    public:
      watchdog() = default;
      template <typename F>
      watchdog(const TimeUnits& duration, F&& cb) : _duration(duration), _callback(cb) {}
      ~watchdog() {
         _should_exit = true;
         _timer.detach();
      }
      void      set_duration(const TimeUnits& duration) { _duration = duration; }
      TimeUnits get_duration() const { return _duration; }
      void      join() { _timer.join(); }
      void      detach() { _timer.detach(); }
      template <typename F>
      void set_callback(F&& callback) {
         _callback = callback;
      }
      void run() {
         _start      = std::chrono::high_resolution_clock::now();
         _is_running = true;
      }

    private:
      void runner() {
         while (!_should_exit) {
            if (_is_running) {
               auto _now = std::chrono::high_resolution_clock::now();
               if (std::chrono::duration_cast<TimeUnits>(_now - _start) >= _duration) {
                  _is_running = false;
                  if (_callback)
                     _callback();
               }
            }
         }
      }
      using time_point_type              = std::chrono::time_point<std::chrono::high_resolution_clock>;
      std::thread           _timer       = std::thread(&watchdog::runner, this);
      std::atomic<bool>     _should_exit = false;
      std::function<void()> _callback;
      bool                  _is_running = false;
      TimeUnits             _duration;
      time_point_type       _start;
   };
}} // namespace eosio::vm
