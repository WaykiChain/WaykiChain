#pragma once
#pragma GCC diagnostic ignored "-Wint-in-bool-context"
#include <chrono>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>


extern void RenameThread(const char* name);


namespace eosio { namespace vm {

   /// \brief Triggers a callback after a given time elapses.
   ///

   //class eosio::vm::guard;
   sighandler_t old_signal_handler = NULL;
   std::function<void()> guard_callback;
   //eosio::vm::guard::state_t guard_run_state;
   void signal_handler(int signo){
       switch (signo){
          case SIGALRM:
              //if (guard_run_state == guard::state_t::running) {
                 //_run_state = interrupted;
                 guard_callback();
              //}     
              break;
       }
   }

   class watchdog_with_settimer {
      class guard;
    public:
     /// \tparam TimeUnits must be a chrono duration type
     /// \pre duration must be a non-negative value.
      template <typename TimeUnits>
      explicit watchdog_with_settimer(const TimeUnits& duration) : _duration(duration) {}
      /// Starts the timer.  If the timer
      /// expires during the lifetime of the returned object, the callback
      /// will be executed.  The callback is executed asynchronously.
      /// If invoking the callback throws an exception, terminate will
      /// be called.
      template<typename F>
      [[nodiscard]] guard scoped_run(F&& callback) {
         return guard(_duration, static_cast<F&&>(callback));

    };

    private:
      class guard {
       public:
         guard(const guard&) = delete;
         guard& operator=(const guard&) = delete;

         template <typename TimeUnits, typename F>
         guard(const TimeUnits& duration, F&& callback)
            : _callback(static_cast<F&&>(callback)),
              _run_state(running),
              _duration(duration),
              _start(std::chrono::steady_clock::now()) {
            // Must be started after all other initialization to avoid races.

            //guard_run_state = _run_state;
            guard_callback  = _callback;
            old_signal_handler = signal(SIGALRM, signal_handler);

            new_value.it_value.tv_sec     = 0;
            new_value.it_value.tv_usec    = _duration.count();
            new_value.it_interval.tv_sec  = 0;
            new_value.it_interval.tv_usec = 0;
            setitimer(ITIMER_REAL, &new_value, &old_value);

         }
         ~guard() {
            {
               _run_state = stopped;
               //guard_run_state = _run_state;
            }
            setitimer(ITIMER_REAL, &old_value, NULL);
            signal(SIGALRM, old_signal_handler);
         }

       private:

         enum state_t { running, interrupted, stopped };
         using time_point_type              = std::chrono::time_point<std::chrono::steady_clock>;
         using duration_type = std::chrono::steady_clock::duration;
         std::mutex            _mutex;
         std::function<void()> _callback;
         bool                  _run_state = stopped;
         duration_type         _duration;
         time_point_type       _start;

         struct itimerval new_value,old_value;

      };
      std::chrono::steady_clock::duration _duration;
   };



   class watchdog {
      class guard;
    public:
     /// \tparam TimeUnits must be a chrono duration type
     /// \pre duration must be a non-negative value.
      template <typename TimeUnits>
      explicit watchdog(const TimeUnits& duration) : _duration(duration) {}
      /// Starts the timer.  If the timer
      /// expires during the lifetime of the returned object, the callback
      /// will be executed.  The callback is executed asynchronously.
      /// If invoking the callback throws an exception, terminate will
      /// be called.
      template<typename F>
      [[nodiscard]] guard scoped_run(F&& callback) {
         return guard(_duration, static_cast<F&&>(callback));

    };

    private:
      class guard {
       public:
         guard(const guard&) = delete;
         guard& operator=(const guard&) = delete;

         template <typename TimeUnits, typename F>
         guard(const TimeUnits& duration, F&& callback)
            : _callback(static_cast<F&&>(callback)),
              _run_state(running),
              _duration(duration),
              _start(std::chrono::steady_clock::now()) {
            // Must be started after all other initialization to avoid races.
            _timer = std::thread(&guard::runner, this);
         }
         ~guard() {
            {
               auto _lock = std::unique_lock(_mutex);
               _run_state = stopped;
            }
            _cond.notify_one();
            auto bm = MAKE_BENCHMARK("-------------waiting for thread end-------------");
            _timer.join();
            if(bm) bm->end();
         }

       private:
         // worker thread
         // Implementation notes: This implementation creates a thread for
         // each watchdog.  If there are many watchdogs, it might be more
         // efficient to create a single thread for them all.
         void runner() {
            RenameThread("coin-wasmtimer");
            auto _lock = std::unique_lock(_mutex);
            // wait until the timer expires or someone stops it.
            _cond.wait_until(_lock, _start + _duration, [&]() { return _run_state != running; });
            if (_run_state == running) {
               _run_state = interrupted;
               _callback();
            }
         }

         enum state_t { running, interrupted, stopped };
         using time_point_type              = std::chrono::time_point<std::chrono::steady_clock>;
         using duration_type = std::chrono::steady_clock::duration;
         std::thread           _timer;
         std::mutex            _mutex;
         std::condition_variable _cond;
         std::function<void()> _callback;
         bool                  _run_state = stopped;
         duration_type         _duration;
         time_point_type       _start;

         struct itimerval new_value,old_value;

      };
      std::chrono::steady_clock::duration _duration;
   };

   class null_watchdog {
    public:
      template<typename F>
      null_watchdog scoped_run(F&&) { return *this; }
      ~null_watchdog() {} // avoid unused variable warnings
   };

}} // namespace eosio::vm
