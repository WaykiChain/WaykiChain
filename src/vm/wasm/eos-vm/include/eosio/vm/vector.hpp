#pragma once

#include <eosio/vm/exceptions.hpp>
#include <eosio/vm/allocator.hpp>

#include <algorithm>
#include <utility>
#include <string>
#include <vector>

namespace eosio { namespace vm {
   namespace detail {
      template <typename T, typename Allocator> 
      class vector {
         public:
            constexpr vector(Allocator& allocator, size_t size=0) :
               _size(size),
               _allocator(&allocator),
               _data(allocator.template alloc<T>( _size )) {
            }

            constexpr vector(const vector& mv) = delete;
            constexpr vector(vector&& mv) = default;
            constexpr vector& operator=(vector&& mv) = default;

            constexpr inline void resize( size_t size ) {
               if (size > _size) {
                  _data = _allocator->template alloc<T>( size );
               } else {
                  _allocator->template reclaim<T>( _data + size, _size - size );
               }
               _size = size;
            }
            template <typename U, typename = std::enable_if_t<std::is_same_v<T, std::decay_t<U>>, int>> 
            constexpr inline void push_back( U&& val ) {
               // if the vector is unbounded don't assert
              if ( _index >= _size )
                  resize( _size * 2 );
               _data[_index++] = std::forward<U>(val);
            }
            constexpr inline void emplace_back( T&& val ) {
               // if the vector is unbounded don't assert
              if ( _index >= _size )
                  resize( _size * 2 );
               _data[_index++] = std::move(val);
            }

            constexpr inline void back() {
               return _data[_index];
            }

            constexpr inline void pop_back() {
               EOS_VM_ASSERT( _index >= 0, wasm_vector_oob_exception, "vector pop out of bounds" );
               _index--;
            }

            constexpr inline T& at( size_t i ) {
               EOS_VM_ASSERT( i < _size, wasm_vector_oob_exception, "vector read out of bounds" );
               return _data[i];
            }

            constexpr inline T& at( size_t i )const {
               EOS_VM_ASSERT( i < _size, wasm_vector_oob_exception, "vector read out of bounds" );
               return _data[i];
            }

            constexpr inline T& at_no_check( size_t i ) {
               return _data[i];
            }

            constexpr inline T& at_no_check( size_t i ) const {
               return _data[i];
            }

            constexpr inline T& operator[] (size_t i) const { return at(i); }
            constexpr inline T& operator[] (size_t i) { return at(i); }
            constexpr inline T* raw() const { return _data; }
            constexpr inline size_t size() const { return _size; }
            constexpr inline void set( T* data, size_t size, size_t index=-1 ) { _size = size; _data = data; _index = index == -1 ? size - 1 : index; }
            constexpr inline void copy( T* data, size_t size ) {
              resize(size);
              std::copy_n(data, size, _data);
              _index = size-1;
            }

         private:
            size_t _size  = 0;
            Allocator* _allocator = nullptr;
            T*     _data  = nullptr;
            size_t _index = 0;
      };

      struct unmanaged_base_member {
         using allocator = contiguous_allocator;
         unmanaged_base_member(size_t sz) : alloc(sz) {}
         allocator alloc;
      };
   } // ns detail

   template <typename T, typename Allocator>
   class managed_vector : public detail::vector<T, Allocator> {
      public:
         using detail::vector<T, Allocator>::vector;
         constexpr inline void set_owner( Allocator& alloc ) { detail::vector<T, Allocator>::_allocator = &alloc; }
   };

   template <typename T>
   using unmanaged_vector = std::vector<T>;

   template <typename T>
   std::string vector_to_string( T&& vec ) {
     std::string str;
     str.reserve(vec.size());
     for (int i=0; i < vec.size(); i++)
       str[i] = vec[i];
     return str;
   }
}} // namespace eosio::vm
