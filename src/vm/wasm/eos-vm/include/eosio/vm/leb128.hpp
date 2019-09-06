#pragma once

#include <eosio/vm/exceptions.hpp>
#include <eosio/vm/guarded_ptr.hpp>

#include <iostream>
#include <array>

namespace eosio { namespace vm {
   template <size_t N>
   inline size_t constexpr bytes_needed() {
      if constexpr (N == 1 || N == 7)
         return 1;
      else if constexpr (N == 32)
         return 5;
      else
         return 10;
   }

   template <size_t N>
   class varuint {
      public:
         static_assert(N == 1  || N == 7 || N == 32, "N not valid");

         inline constexpr explicit varuint(bool v) { from(v); }
         inline constexpr explicit varuint(uint8_t v) { from(v); }
         inline constexpr explicit varuint(uint32_t v) { from(v); }
         inline constexpr varuint( guarded_ptr<uint8_t>& code ) { from(code); }
         
         inline constexpr void from(bool v) { storage[0] = v; }
         inline constexpr void from(uint8_t v) {
            storage[0] = v & 0x7f;
         }
         inline constexpr void from(uint32_t v) {
	    bytes_used = 0;
            #pragma unroll
            for (; bytes_used < bytes_needed<N>(); bytes_used++) {
               storage[bytes_used] = v & 0x7f;
               v >>= 7;
               if (v!= 0)
                  storage[bytes_used] |= 0x80;
	       else
                  break;
            }
            bytes_used++;
         }
         
         inline constexpr void from( guarded_ptr<uint8_t>& code ) {
            uint8_t cnt = 0;
            for (; cnt < bytes_needed<N>(); cnt++) {
               EOS_WB_ASSERT( code.offset()+cnt < code.bounds(), wasm_interpreter_exception, "pointer out of bounds" );
               storage[cnt] = code[cnt];
               if ((storage[cnt] & 0x80) == 0) {
                  break;
               }
            }
            code += cnt+1;
            bytes_used = cnt+1;
         }

         size_t size()const { return bytes_used; }

         template <size_t M=N, typename = typename std::enable_if_t<M == 1, int>>
         inline constexpr bool to() { return storage[0]; }

         template <size_t M=N, typename = typename std::enable_if_t<M == 7, int>>
         inline constexpr uint8_t to() { return storage[0] & 0x7f; }

         template <size_t M=N, typename = typename std::enable_if_t<M == 32, int>>
         inline constexpr uint32_t to() { 
            uint32_t ret = 0;
            #pragma unroll
            for (int i=bytes_used-1; i >= 0; i--) {
               ret <<= 7;
               ret |= storage[i] & 0x7f;
            }
            return ret;
         }

         void print()const {
            for (int i=0; i < bytes_used; i++) {
               std::cout << std::hex << "0x" << (int)storage[i] << ' ';
            }
            std::cout << std::endl;
         }
      
      private:
         std::array<uint8_t, bytes_needed<N>()> storage;
         uint8_t bytes_used = bytes_needed<N>(); 
   };

   template <size_t N>
   class varint {
      public:
         static_assert(N == 7 || N == 32 || N == 64, "N not valid");
         
         inline constexpr explicit varint(int8_t v) { from(v); }
         inline constexpr explicit varint(int32_t v) { from(v); }
         inline constexpr explicit varint(int64_t v) { from(v); }
         inline constexpr varint( guarded_ptr<uint8_t>& code ) { from(code); }
         
         inline constexpr void from(int8_t v) {
            static_assert(N >= 7, "cant use this constructor with N < 7");
            storage[0] = v & 0x7f;
         }
         inline constexpr void from(int32_t v) {
            static_assert(N >= 32, "cant use this constructor with N < 32");
            _from(v);
         }
         inline constexpr void from(int64_t v) {
            static_assert(N >= 64, "cant use this constructor with N < 32");
            _from(v);
         }
         
         inline constexpr void from( guarded_ptr<uint8_t>& code ) {
            uint8_t cnt = 0;
            for (; cnt < bytes_needed<N>(); cnt++) {
               EOS_WB_ASSERT( code.offset()+cnt < code.bounds(), wasm_interpreter_exception, "pointer out of bounds" );
               storage[cnt] = code[cnt];
               if ((storage[cnt] & 0x80) == 0) {
                  break;
               }
            }
            code += cnt+1;
            bytes_used = cnt+1;
         }
         
         size_t size()const { return bytes_used; }

         template <size_t M=N, typename = typename std::enable_if_t<M == 1, int>>
         inline constexpr bool to() { return storage[0]; }

         template <size_t M=N, typename = typename std::enable_if_t<M == 7, int>>
         inline constexpr int8_t to() {
            if (storage[0] & 0x40)
               return storage[0] | (~0u << 7);
            return storage[0];
         }

         template <size_t M=N, typename = typename std::enable_if_t<M == 32, int>>
         inline constexpr int32_t to() { return _to<int32_t>(); }

         template <size_t M=N, typename = typename std::enable_if_t<M == 64, int>>
         inline constexpr int64_t to() { return _to<int64_t>(); }

         void print()const {
            for (int i=0; i < bytes_used; i++) {
               std::cout << std::hex << "0x" << (int)storage[i] << ' ';
            }
            std::cout << std::endl;
         }
      
      private:
         template <typename T>
         inline constexpr void _from(T v) {
            bool is_neg = v < 0;
            bytes_used = 0;
            #pragma unroll
            for (; bytes_used < bytes_needed<N>(); bytes_used++) {
	       storage[bytes_used] = v & 0x7f;
	       v >>= 7;
	       if ((v == -1 && (storage[bytes_used] & 0x40)) || (v == 0 && !(storage[bytes_used] & 0x40)))
                  break;
	       storage[bytes_used] |= 0x80;
	    }
            bytes_used++;
         }

         template <typename T>
         inline constexpr T _to() {
            typename std::make_unsigned<T>::type ret = 0;
            #pragma unroll
            for (int i=bytes_used-1; i >= 0; i--) {
               ret <<= 7;
               ret |= storage[i] & 0x7f;
            }
            if (bytes_used >= 1 && bytes_used < bytes_needed<N>()) {
               size_t shift = ((bytes_used) * 7);
               if (storage[bytes_used-1] & 0x40)
                  ret |= (-1ull) << shift;
            }
            return *(T*)&ret;
         }

         std::array<uint8_t, bytes_needed<N>()> storage;
         uint8_t bytes_used = bytes_needed<N>(); 
   };

}} // ns eosio::vm
