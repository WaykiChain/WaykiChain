#pragma once

#include <eosio/vm/exceptions.hpp>
#include <eosio/vm/utils.hpp>

#include <atomic>
#include <cstdlib>
#include <exception>
#include <utility>
#include <signal.h>
#include <setjmp.h>

namespace eosio { namespace vm {

   inline thread_local std::atomic<sigjmp_buf*> signal_dest = ATOMIC_VAR_INIT(nullptr);
   inline thread_local std::exception_ptr saved_exception{nullptr};
   template<int Sig>
   inline struct sigaction prev_signal_handler;

   inline void signal_handler(int sig, siginfo_t* info, void* uap) {
      sigjmp_buf* dest = std::atomic_load(&signal_dest);
      if (dest) {
         siglongjmp(*dest, sig);
      } else {
         struct sigaction* prev_action;
         switch(sig) {
            case SIGSEGV: prev_action = &prev_signal_handler<SIGSEGV>; break;
            case SIGBUS: prev_action = &prev_signal_handler<SIGBUS>; break;
            case SIGFPE: prev_action = &prev_signal_handler<SIGFPE>; break;
            default: std::abort();
         }
         if (!prev_action) std::abort();
         if (prev_action->sa_flags & SA_SIGINFO) {
            // FIXME: We need to be at least as strict as the original
            // flags and relax the mask as needed.
            prev_action->sa_sigaction(sig, info, uap);
         } else {
            if(prev_action->sa_handler == SIG_DFL) {
               // The default for all three signals is to terminate the process.
               sigaction(sig, prev_action, nullptr);
               raise(sig);
            } else if(prev_action->sa_handler == SIG_IGN) {
               // Do nothing
            } else {
               prev_action->sa_handler(sig);
            }
         }
      }
   }

   // only valid inside invoke_with_signal_handler.
   // This is a workaround for the fact that it
   // is currently unsafe to throw an exception through
   // a jit frame.
   template<typename F>
   inline void longjmp_on_exception(F&& f) {
      static_assert(std::is_trivially_destructible_v<std::decay_t<F>>, "longjmp has undefined behavior when it bypasses destructors.");
      bool caught_exception = false;
      try {
         f();
      } catch(...) {
         saved_exception = std::current_exception();
         // Cannot safely longjmp from inside the catch,
         // as that will leak the exception.
         caught_exception = true;
      }
      if (caught_exception) {
         sigjmp_buf* dest = std::atomic_load(&signal_dest);
         siglongjmp(*dest, -1);
      }
   }

   template<typename E>
   [[noreturn]] inline void throw_(const char* msg) {
      saved_exception = std::make_exception_ptr(E{msg});
      sigjmp_buf* dest = std::atomic_load(&signal_dest);
      siglongjmp(*dest, -1);
   }

   inline void setup_signal_handler_impl() {
      struct sigaction sa;
      sa.sa_sigaction = &signal_handler;
      sigemptyset(&sa.sa_mask);
      sa.sa_flags = SA_NODEFER | SA_SIGINFO;
      sigaction(SIGSEGV, &sa, &prev_signal_handler<SIGSEGV>);
      sigaction(SIGBUS, &sa, &prev_signal_handler<SIGBUS>);
      sigaction(SIGFPE, &sa, &prev_signal_handler<SIGFPE>);
   }

   inline void setup_signal_handler() {
      static int init_helper = (setup_signal_handler_impl(), 0);
      ignore_unused_variable_warning(init_helper);
      static_assert(std::atomic<sigjmp_buf*>::is_always_lock_free, "Atomic pointers must be lock-free to be async signal safe.");
   }

   /// Call a function with a signal handler installed.  If this thread is
   /// signalled during the execution of f, the function e will be called with
   /// the signal number as an argument.  If f creates any automatic variables
   /// with non-trivial destructors, then it must mask the relevant signals
   /// during the lifetime of these objects or the behavior is undefined.
   ///
   /// signals handled: SIGSEGV, SIGBUS, SIGFPE
   ///
   // Make this noinline to prevent possible corruption of the caller's local variables.
   // It's unlikely, but I'm not sure that it can definitely be ruled out if both
   // this and f are inlined and f modifies locals from the caller.
   template<typename F, typename E>
   [[gnu::noinline]] auto invoke_with_signal_handler(F&& f, E&& e) {
      setup_signal_handler();
      sigjmp_buf dest;
      sigjmp_buf* volatile old_signal_handler = nullptr;
      int sig;
      if((sig = sigsetjmp(dest, 1)) == 0) {
         // Note: Cannot use RAII, as non-trivial destructors w/ longjmp
         // have undefined behavior. [csetjmp.syn]
         //
         // Warning: The order of operations is critical here.
         // We also have to register signal_dest before unblocking
         // signals to make sure that only our signal handler is executed
         // if the caller has previously blocked signals.
         old_signal_handler = std::atomic_exchange(&signal_dest, &dest);
         sigset_t unblock_mask, old_sigmask; // Might not be preserved across longjmp
         sigemptyset(&unblock_mask);
         sigaddset(&unblock_mask, SIGSEGV);
         sigaddset(&unblock_mask, SIGBUS);
         sigaddset(&unblock_mask, SIGFPE);
         pthread_sigmask(SIG_UNBLOCK, &unblock_mask, &old_sigmask);
         try {
            f();
            pthread_sigmask(SIG_SETMASK, &old_sigmask, nullptr);
            std::atomic_store(&signal_dest, old_signal_handler);
         } catch(...) {
            pthread_sigmask(SIG_SETMASK, &old_sigmask, nullptr);
            std::atomic_store(&signal_dest, old_signal_handler);
            throw;
         }
      } else {
         std::atomic_store(&signal_dest, old_signal_handler);
         if (sig == -1) {
            std::exception_ptr exception = std::move(saved_exception);
            saved_exception = nullptr;
            std::rethrow_exception(exception);
         } else {
            e(sig);
         }
      }
   }

}} // namespace eosio::vm
