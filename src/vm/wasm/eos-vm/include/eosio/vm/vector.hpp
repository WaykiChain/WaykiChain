#pragma once

#include <eosio/vm/exceptions.hpp>

#include <utility>
#include <string>

namespace eosio { namespace vm {
   
   template <typename T, typename Allocator> 
   class managed_vector {
      public:
         constexpr managed_vector(Allocator& allocator, size_t size=0) :
            _size(size),
            _allocator(&allocator),
            _data(allocator.template alloc<T>( _size )) {
         }

         constexpr managed_vector(const managed_vector& mv) = delete;
         constexpr managed_vector(managed_vector&& mv) {
            _size  = mv._size;
            _allocator = mv._allocator;
            _data  = mv._data;
         }

         constexpr managed_vector& operator=(managed_vector&& mv) {
            _size  = mv._size;
            _allocator = mv._allocator;
            _data  = mv._data;
            return *this;
         }

         constexpr inline void set_owner( Allocator& alloc ) { _allocator = &alloc; }
         constexpr inline void resize( size_t size ) {
            if (size > _size) {
               T* ptr = _allocator->template alloc<T>( size );
               if (_size == 0)
                 _data = ptr;
            }
            _size = size;
         }

         constexpr inline void push_back( const T& val ) {
            EOS_WB_ASSERT( _index < _size, wasm_vector_oob_exception, "vector write out of bounds" );
            _data[_index++] = val;
         }

         constexpr inline void emplace_back( T&& val ) {
            EOS_WB_ASSERT( _index < _size, wasm_vector_oob_exception, "vector write out of bounds" );
            _data[_index++] = std::move(val);
         }

         constexpr inline void back() {
            return _data[_index];
         }

         constexpr inline void pop_back() {
            EOS_WB_ASSERT( _index >= 0, wasm_vector_oob_exception, "vector pop out of bounds" );
         }

         constexpr inline T& at( size_t i ) {
            EOS_WB_ASSERT( i < _size, wasm_vector_oob_exception, "vector read out of bounds" );
            return _data[i];
         }

         constexpr inline T& at( size_t i )const {
            EOS_WB_ASSERT( i < _size, wasm_vector_oob_exception, "vector read out of bounds" );
            return _data[i];
         }

         constexpr inline T& at_no_check( size_t i ) {
            return _data[i];
         }

         constexpr inline T& at_no_check( size_t i ) const {
            return _data[i];
         }

         constexpr inline T& operator[] (size_t i) const { return at(i); }
         constexpr inline T* raw() const { return _data; }
         constexpr inline size_t size() const { return _size; }
         constexpr inline void set( T* data, size_t size ) { _size = size; _data = data; _index = size-1; }
         constexpr inline void copy( T* data, size_t size ) {
           resize(size);
           for (int i=0; i < size; i++)
             _data[i] = data[i];
           _index = size-1;
         }

      private:
         size_t _size  = 0;
         Allocator* _allocator = nullptr;
         T*     _data  = nullptr;
         size_t _index = 0;
   };

   template <typename T>
   std::string vector_to_string( T&& vec ) {
     std::string str;
     str.reserve(vec.size());
     for (int i=0; i < vec.size(); i++)
       str[i] = vec[i];
     return str;
   }
}} // namespace eosio::vm
