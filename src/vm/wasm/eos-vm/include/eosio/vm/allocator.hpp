#pragma once

#include <eosio/vm/constants.hpp>
#include <eosio/vm/exceptions.hpp>

#include <csignal>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <sys/mman.h>

namespace eosio { namespace vm {
   class bounded_allocator {
    public:
      bounded_allocator(size_t size) {
         mem_size = size;
         raw      = std::unique_ptr<uint8_t[]>(new uint8_t[mem_size]);
      }
      template <typename T>
      T* alloc(size_t size = 1) {
         EOS_WB_ASSERT((sizeof(T) * size) + index <= mem_size, wasm_bad_alloc, "wasm failed to allocate native");
         T* ret = (T*)(raw.get() + index);
         index += sizeof(T) * size;
         return ret;
      }
      void free() {
         EOS_WB_ASSERT(index > 0, wasm_double_free, "double free");
         index = 0;
      }
      void                       reset() { index = 0; }
      size_t                     mem_size;
      std::unique_ptr<uint8_t[]> raw;
      size_t                     index = 0;
   };

   class growable_allocator {
    public:
      static constexpr size_t max_memory_size = 1024 * 1024 * 1024; // 1GB
      static constexpr size_t chunk_size      = 128 * 1024;         // 128KB
      static constexpr size_t align_amt       = 16;
      static constexpr size_t align_offset(size_t offset) { return (offset + align_amt - 1) & ~(align_amt - 1); }

      // size in bytes
      growable_allocator(size_t size) {
         _base = (char*)mmap(NULL, max_memory_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
         if (size != 0) {
            size_t chunks_to_alloc = (size / chunk_size) + 1;
            _size += (chunk_size * chunks_to_alloc);
            mprotect((char*)_base, _size, PROT_READ | PROT_WRITE);
         }
      }

      ~growable_allocator() { munmap(_base, max_memory_size); }

      // TODO use Outcome library
      template <typename T>
      T* alloc(size_t size = 0) {
         size_t aligned = align_offset((sizeof(T) * size) + _offset);
         if (aligned >= _size) {
            size_t chunks_to_alloc = aligned / chunk_size;
            mprotect((char*)_base + _size, (chunk_size * chunks_to_alloc), PROT_READ | PROT_WRITE);
            _size += (chunk_size * chunks_to_alloc);
         }

         T* ptr  = (T*)(_base + _offset);
         _offset = aligned;
         return ptr;
      }

      void free() { EOS_WB_ASSERT(false, wasm_bad_alloc, "unimplemented"); }

      void reset() { _offset = 0; }

      size_t _offset = 0;
      size_t _size   = 0;
      char*  _base;
   };

   template <typename T>
   class fixed_stack_allocator {
    private:
      T*     raw      = nullptr;
      size_t max_size = 0;
      void   set_up_signals() {
         struct sigaction sa;
         sa.sa_sigaction = [](int sig, siginfo_t*, void*) {
            throw stack_memory_exception{ "stack memory out-of-bounds" };
         };
         sigemptyset(&sa.sa_mask);
         sa.sa_flags = SA_NODEFER | SA_SIGINFO;
         sigaction(SIGSEGV, &sa, NULL);
         sigaction(SIGBUS, &sa, NULL);
      }

    public:
      template <typename U>
      void free() {
         munmap(raw, max_memory);
      }
      fixed_stack_allocator(size_t max_size) : max_size(max_size) {
         set_up_signals();
         raw = (T*)mmap(NULL, max_memory, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
         mprotect(raw, max_size * sizeof(T), PROT_READ | PROT_WRITE);
      }
      inline T* get_base_ptr() const { return raw; }
   };

   class wasm_allocator {
    private:
      char*   raw       = nullptr;
      char*   _previous = raw;
      int32_t page      = 0;

      void set_up_signals() {
         struct sigaction sa;
         sa.sa_sigaction = [](int sig, siginfo_t*, void*) {
            throw wasm_memory_exception{ "wasm memory out-of-bounds" };
         };
         sigemptyset(&sa.sa_mask);
         sa.sa_flags = SA_NODEFER | SA_SIGINFO;
         sigaction(SIGSEGV, &sa, NULL);
         sigaction(SIGBUS, &sa, NULL);
      }

    public:
      template <typename T>
      T* alloc(size_t size = 1 /*in pages*/) {
         EOS_WB_ASSERT(page + size <= max_pages, wasm_bad_alloc, "exceeded max number of pages");
         mprotect(raw + (page_size * page), (page_size * size), PROT_READ | PROT_WRITE);
         T* ptr    = (T*)_previous;
         _previous = (raw + (page_size * page));
         page += size;
         return ptr;
      }
      void free() { munmap(raw, max_memory); }
      wasm_allocator() {
         set_up_signals();
         raw       = (char*)mmap(NULL, max_memory, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
         _previous = raw;
         mprotect(raw, 1 * page_size, PROT_READ | PROT_WRITE);
         page = 1;
      }
      void reset() {
         uint64_t size = page_size * page;
         _previous     = raw + page_size;
         memset(raw, 0, size);
         page = 1;
         mprotect(raw, size, PROT_NONE);
         mprotect(raw, 1 * page_size, PROT_READ | PROT_WRITE);
      }
      template <typename T>
      inline T* get_base_ptr() const {
         return reinterpret_cast<T*>(raw);
      }
      inline int32_t get_current_page() const { return page; }
   };
}} // namespace eosio::vm
