#pragma once

/*
 * definitions from https://github.com/WebAssembly/design/blob/master/BinaryEncoding.md
 */

#include <eosio/vm/allocator.hpp>
#include <eosio/vm/exceptions.hpp>
#include <eosio/vm/stack_elem.hpp>
#include <eosio/vm/types.hpp>
#include <eosio/vm/utils.hpp>
#include <eosio/vm/vector.hpp>

namespace eosio { namespace vm {
   template <size_t Elems, typename Allocator>
   class fixed_stack {
    public:
      fixed_stack(Allocator& alloc) : _s(managed_vector<stack_elem, Allocator>{ alloc, Elems }) {}
      void        push(stack_elem e) { _s[_index++] = e; }
      stack_elem& get(uint32_t index) const {
         EOS_WB_ASSERT(index <= _index, wasm_interpreter_exception, "invalid stack index");
         return _s[index];
      }
      void set(uint32_t index, const stack_elem& el) {
         EOS_WB_ASSERT(index <= _index, wasm_interpreter_exception, "invalid stack index");
         _s[index] = el;
      }
      stack_elem        pop() { return _s[--_index]; }
      void              eat(uint32_t index) { _index = index; }
      uint16_t          current_index() const { return _index; }
      stack_elem&       peek() { return _s[_index - 1]; }
      const stack_elem& peek() const { return _s[_index - 1]; }
      stack_elem&       peek(size_t i) { return _s[_index - 1 - i]; }
      stack_elem        get_back(size_t i) { return _s[_index - 1 - i]; }
      void              trim(size_t amt) { _index -= amt; }
      uint16_t          size() const { return _index; }

    private:
      managed_vector<stack_elem, Allocator> _s;
      uint16_t                              _index = 0;
   };

   using control_stack = fixed_stack<constants::max_nested_structures, bounded_allocator>;
   using operand_stack = fixed_stack<constants::max_stack_size, bounded_allocator>;
   using call_stack    = fixed_stack<constants::max_call_depth, bounded_allocator>;

}} // namespace eosio::vm
