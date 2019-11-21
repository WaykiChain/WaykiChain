#pragma once

#include <alloca.h>
#include <string>
#include "wasm/exceptions.hpp"

namespace wasm {

   /**
    *  @defgroup system System
    *  @ingroup core
    *  @brief Defines wrappers over wasm_assert
    */

   /**
    *  Assert if the predicate fails and use the supplied message.
    *
    *  @ingroup system
    *
    *  Example:
    *  @code
    *  wasm::check(a == b, "a does not equal b");
    *  @endcode
    */
   inline void check(bool pred, const char* msg) {
      if (!pred) {
         WASM_ASSERT(false, wasm_assert_exception, "wasm-assert-fail:%s", msg)
      }
   }

    /**
    *  Assert if the predicate fails and use the supplied message.
    *
    *  @ingroup system
    *
    *  Example:
    *  @code
    *  wasm::check(a == b, "a does not equal b");
    *  @endcode
    */
   inline void check(bool pred, const std::string& msg) {
      if (!pred) {
         WASM_ASSERT(false, wasm_assert_exception, "wasm-assert-fail:%s", msg.c_str())
      }
   }

   /**
    *  Assert if the predicate fails and use the supplied message.
    *
    *  @ingroup system
    *
    *  Example:
    *  @code
    *  wasm::check(a == b, "a does not equal b");
    *  @endcode
    */
   inline void check(bool pred, std::string&& msg) {
      if (!pred) {
         WASM_ASSERT(false, wasm_assert_exception, "wasm-assert-fail:%s", msg.c_str())
      }
   }

} // namespace wasm
