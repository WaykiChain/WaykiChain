#pragma once
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <eosio/vm/constants.hpp>
#include <eosio/vm/exceptions.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <set>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <sys/mman.h>
#include <unistd.h>

#include <iostream>

namespace eosio { namespace vm {
   class bounded_allocator {
    public:
      bounded_allocator(size_t size) {
         mem_size = size;
         raw      = std::unique_ptr<uint8_t[]>(new uint8_t[mem_size]);
      }
      template <typename T>
      T* alloc(size_t size = 1) {
         EOS_VM_ASSERT((sizeof(T) * size) + index <= mem_size, wasm_bad_alloc, "wasm failed to allocate native");
         T* ret = (T*)(raw.get() + index);
         index += sizeof(T) * size;
         return ret;
      }

      template <typename T>
      void reclaim(const T* ptr, size_t size=0) { /* noop for now */ }

      void free() {
         EOS_VM_ASSERT(index > 0, wasm_double_free, "double free");
         index = 0;
      }
      void                       reset() { index = 0; }
      size_t                     mem_size;
      std::unique_ptr<uint8_t[]> raw;
      size_t                     index = 0;
   };

   class contiguous_allocator {
      public:
         template<std::size_t align_amt>
         static constexpr size_t align_offset(size_t offset) { return (offset + align_amt - 1) & ~(align_amt - 1); }

         static std::size_t align_to_page(std::size_t offset) {
            std::size_t pagesize = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
            return (offset + pagesize - 1) & ~(pagesize - 1);
         }

         contiguous_allocator(size_t size) {
            _size = align_to_page(size);
            _base = (char*)mmap(NULL, _size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            EOS_VM_ASSERT(_base != MAP_FAILED, wasm_bad_alloc, "mmap failed.");
         }
         ~contiguous_allocator() { munmap(_base, align_to_page(_size)); }

         template <typename T>
         T* alloc(size_t size = 0) {
            _offset = align_offset<alignof(T)>(_offset);
            size_t aligned = (sizeof(T) * size) + _offset;
            if (aligned > _size) {
               size_t new_size = align_to_page(aligned);
               char* new_base = (char*)mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
               EOS_VM_ASSERT(new_base != MAP_FAILED, wasm_bad_alloc, "mmap failed.");
               memcpy(new_base, _base, _size);
               munmap(_base, _size);
               _size = new_size;
               _base = new_base;
            }
            T* ptr = (T*)(_base + _offset);
            _offset = aligned;
            return ptr;
         }
         template <typename T>
         void reclaim(const T* ptr, size_t size=0) { /* noop for now */ }
         void free() { /* noop for now */ }

      private:
         size_t _offset = 0;
         size_t _size   = 0;
         char*  _base;
   };

   class jit_allocator {
      static constexpr std::size_t segment_size = std::size_t{1024u} * 1024u * 1024u;
   public:
      // allocates page aligned memory with executable permission
      void * alloc(std::size_t size) {
         std::lock_guard l{_mutex};
         size = round_to_page(size);
         auto best = free_blocks_by_size.lower_bound(size);
         if(best == free_blocks_by_size.end()) {
            best = allocate_segment(size);
         }
         if (best->first > size) {
            best = split_block(best, size);
         }
         transfer_node(free_blocks, allocated_blocks, best->second);
         best = transfer_node(free_blocks_by_size, allocated_blocks_by_size, *best);
         return best->second;
      }
      // ptr must be previously allocated by a call to alloc
      void free(void* ptr) noexcept {
         std::lock_guard l{_mutex};
         auto pos = transfer_node(allocated_blocks, free_blocks, ptr);
         transfer_node(allocated_blocks_by_size, free_blocks_by_size, {pos->second, pos->first});

         // merge the freed block with adjacent free blocks
         if(pos != free_blocks.begin()) {
            auto prev = pos;
            --prev;
            pos = maybe_consolidate_blocks(prev, pos);
         }
         auto next = pos;
         ++next;
         if (next != free_blocks.end()) {
            maybe_consolidate_blocks(pos, next);
         }
      }
      static jit_allocator& instance() {
         static jit_allocator the_jit_allocator;
         return the_jit_allocator;
      }
   private:
      struct segment {
         segment(void * base, std::size_t size) : base(base), size(size) {}
         segment(segment&& other) : base(other.base), size(other.size) {
            other.base = nullptr;
            other.size = 0;
         }
         segment& operator=(const segment& other) = delete;
         ~segment() {
            if(base) {
               ::munmap(base, size);
            }
         }
         void * base;
         std::size_t size;
      };
      using block = std::pair<std::size_t, void*>;
      struct by_size {
         using is_transparent = void;
         bool operator()(const block& lhs, const block& rhs) const {
            return lhs.first < rhs.first || (lhs.first == rhs.first && std::less<void*>{}(lhs.second, rhs.second));
         }
         bool operator()(const block& lhs, std::size_t rhs) const {
            return lhs.first < rhs;
         }
         bool operator()(std::size_t lhs, const block& rhs) const {
            return lhs < rhs.first;
         }
      };
      std::vector<segment> _segments;
      std::set<block, by_size> free_blocks_by_size;
      std::set<block, by_size> allocated_blocks_by_size;
      std::map<void*, std::size_t> free_blocks;
      std::map<void*, std::size_t> allocated_blocks;
      std::mutex _mutex;
      using blocks_by_size_t = std::set<block, by_size>;
      using blocks_t = std::map<void*, size_t>;

      // moves an element from one associative container to another
      // @pre key must be present in from, but not in to
      template<typename C>
      static typename C::iterator transfer_node(C& from, C& to, typename C::key_type key) noexcept {
         auto node = from.extract(key);
         assert(node);
         auto [pos, inserted, _] = to.insert(std::move(node));
         assert(inserted);
         return pos;
      }

      blocks_by_size_t::iterator allocate_segment(std::size_t min_size) {
         std::size_t size = std::max(min_size, segment_size);
         void* base = mmap(nullptr, size, PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
         segment s{base, size};
         EOS_VM_ASSERT(base != MAP_FAILED, wasm_bad_alloc, "failed to allocate jit segment");
         _segments.emplace_back(std::move(s));
         bool success = true;
         auto guard_1 = scope_guard{[&] { if(!success) { _segments.pop_back(); } }};
         auto pos2 = free_blocks_by_size.insert({size, base}).first;
         auto guard_2 = scope_guard{[&] { if(!success) { free_blocks_by_size.erase(pos2); }}};
         free_blocks.insert({base, size});
         success = true;
         return pos2;
      }

      blocks_by_size_t::iterator split_block(blocks_by_size_t::iterator pos, std::size_t size) {
         bool success = false;
         auto new1 = free_blocks_by_size.insert({size, pos->second}).first;
         auto guard1 = scope_guard{[&]{ if(!success) { free_blocks_by_size.erase(new1); } }};
         auto new2 = free_blocks_by_size.insert({pos->first - size, static_cast<char*>(pos->second) + size}).first;
         auto guard2 = scope_guard{[&]{ if(!success) { free_blocks_by_size.erase(new2); } }};
         free_blocks.insert({new2->second, new2->first});
         // the rest is nothrow
         free_blocks_by_size.erase(pos);
         free_blocks[new1->second] = new1->first;
         success = true;
         return new1;
      }

      blocks_t::iterator maybe_consolidate_blocks(blocks_t::iterator lhs, blocks_t::iterator rhs) noexcept {
         if(static_cast<char*>(lhs->first) + lhs->second == rhs->first) {
            // merge blocks in free_blocks_by_size
            auto node = free_blocks_by_size.extract({lhs->second, lhs->first});
            assert(node);
            node.value().first += rhs->second;
            free_blocks_by_size.insert(std::move(node));
            free_blocks_by_size.erase({rhs->second, rhs->first});
            // merge the blocks in free_blocks
            lhs->second += rhs->second;
            free_blocks.erase(rhs);
            return lhs;
         } else {
            return rhs;
         }
      }

      static std::size_t round_to_page(std::size_t offset) {
         std::size_t pagesize = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
         return (offset + pagesize - 1) & ~(pagesize - 1);
      }
   };

   class growable_allocator {
     public:
    //   static constexpr size_t max_memory_size = 1024 * 1024 * 1024; // 1GB
      static constexpr size_t max_memory_size = 4 * 1024 * 1024;    // 4MB
      static constexpr size_t chunk_size      = 128 * 1024;         // 128KB
      template<std::size_t align_amt>
      static constexpr size_t align_offset(size_t offset) { return (offset + align_amt - 1) & ~(align_amt - 1); }

      static std::size_t align_to_page(std::size_t offset) {
         std::size_t pagesize = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
         assert(max_memory_size % page_size == 0);
         return (offset + pagesize - 1) & ~(pagesize - 1);
      }

      // size in bytes
      growable_allocator(size_t size) {
         EOS_VM_ASSERT(size <= max_memory_size, wasm_bad_alloc, "Too large initial memory size");
         _base = (char*)mmap(NULL, max_memory_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
         EOS_VM_ASSERT(_base != MAP_FAILED, wasm_bad_alloc, "mmap failed.");
         if (size != 0) {
            size_t chunks_to_alloc = (align_offset<chunk_size>(size) / chunk_size);
            _size += (chunk_size * chunks_to_alloc);
            mprotect((char*)_base, _size, PROT_READ | PROT_WRITE);
         }
         _capacity = max_memory_size;
      }

      ~growable_allocator() {
         munmap(_base, _capacity);
         if (is_jit) {
            //fixme:will crash while system out
            //jit_allocator::instance().free(_code_base);
         }
      }

      // TODO use Outcome library
      template <typename T>
      T* alloc(size_t size = 0) {
         static_assert(max_memory_size % alignof(T) == 0, "alignment must divide max_memory_size.");
         _offset = align_offset<alignof(T)>(_offset);
         // Evaluating the inequality in this form cannot cause integer overflow.
         // Once this assertion passes, the rest of the function is safe.
         EOS_VM_ASSERT ((max_memory_size - _offset) / sizeof(T) >= size, wasm_bad_alloc, "Allocated too much memory");
         size_t aligned = (sizeof(T) * size) + _offset;
         if (aligned > _size) {
            size_t chunks_to_alloc = align_offset<chunk_size>(aligned - _size) / chunk_size;
            mprotect((char*)_base + _size, (chunk_size * chunks_to_alloc), PROT_READ | PROT_WRITE);
            _size += (chunk_size * chunks_to_alloc);
         }

         T* ptr  = (T*)(_base + _offset);
         _offset = aligned;
         return ptr;
      }

      void * start_code() {
         _offset = align_to_page(_offset);
         return _base + _offset;
      }
      template<bool IsJit>
      void end_code(void * code_base) {
         assert((char*)code_base >= _base);
         assert((char*)code_base <= (_base+_offset));
         _offset = align_to_page(_offset);
         _code_base = (char*)code_base;
         _code_size = _offset - ((char*)code_base - _base);
         if constexpr (IsJit) {
            auto & jit_alloc = jit_allocator::instance();
            void * executable_code = jit_alloc.alloc(_code_size);
            int err = mprotect(executable_code, _code_size, PROT_READ | PROT_WRITE);
            EOS_VM_ASSERT(err == 0, wasm_bad_alloc, "mprotect failed");
            std::memcpy(executable_code, _code_base, _code_size);
            is_jit = true;
            _code_base = (char*)executable_code;
            _offset = (char*)code_base - _base;
         }
         enable_code(IsJit);
      }

      // Sets protection on code pages to allow them to be executed.
      void enable_code(bool is_jit) {
         mprotect(_code_base, _code_size, is_jit?PROT_EXEC:(PROT_READ|PROT_WRITE));
      }
      // Make code pages unexecutable
      void disable_code() {
         mprotect(_code_base, _code_size, PROT_NONE);
      }

      /* different semantics than free,
       * the memory must be at the end of the most recently allocated block.
       */
      template <typename T>
      void reclaim(const T* ptr, size_t size=0) {
         EOS_VM_ASSERT( _offset / sizeof(T) >= size, wasm_bad_alloc, "reclaimed too much memory" );
         EOS_VM_ASSERT( size == 0 || (char*)(ptr + size) == (_base + _offset), wasm_bad_alloc, "reclaiming memory must be strictly LIFO");
         if ( size != 0 )
            _offset = ((char*)ptr - _base);
      }

      /*
       * Finalize the memory by unmapping any excess pages, this means that the allocator will no longer grow
       */
      void finalize() {
         if(_capacity != _offset) {
            std::size_t final_size = align_to_page(_offset);
            EOS_VM_ASSERT(munmap(_base + final_size, _capacity - final_size) == 0, wasm_bad_alloc, "failed to finalize growable_allocator");
            _capacity = _size = _offset = final_size;
         }
      }

      void free() { EOS_VM_ASSERT(false, wasm_bad_alloc, "unimplemented"); }

      void reset() { _offset = 0; }

      size_t _offset = 0;
      size_t _size   = 0;
      std::size_t _capacity = 0;
      char*  _base;
      char*  _code_base = nullptr;
      size_t _code_size = 0;
      bool is_jit = false;
   };

   template <typename T>
   class fixed_stack_allocator {
    private:
      T*     raw      = nullptr;
      size_t max_size = 0;

    public:
      template <typename U>
      void free() {
         munmap(raw, max_memory);
      }
      fixed_stack_allocator(size_t max_size) : max_size(max_size) {
         raw = (T*)mmap(NULL, max_memory, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
         EOS_VM_ASSERT( raw != MAP_FAILED, wasm_bad_alloc, "mmap failed to alloca pages" );
         mprotect(raw, max_size * sizeof(T), PROT_READ | PROT_WRITE);
      }
      inline T* get_base_ptr() const { return raw; }
   };

   class wasm_allocator {
    private:
      char*   raw       = nullptr;
      int32_t page      = 0;

    public:
      template <typename T>
      void alloc(size_t size = 1 /*in pages*/) {
         if (size == 0) return;
         EOS_VM_ASSERT(page != -1, wasm_bad_alloc, "require memory to allocate");
         EOS_VM_ASSERT(size <= max_pages - page, wasm_bad_alloc, "exceeded max number of pages");
         int err = mprotect(raw + (page_size * page), (page_size * size), PROT_READ | PROT_WRITE);
         EOS_VM_ASSERT(err == 0, wasm_bad_alloc, "mprotect failed");
         T* ptr    = (T*)(raw + (page_size * page));
         memset(ptr, 0, page_size * size);
         page += size;
      }
      template <typename T>
      void free(std::size_t size) {
         if (size == 0) return;
         EOS_VM_ASSERT(page != -1, wasm_bad_alloc, "require memory to deallocate");
         EOS_VM_ASSERT(size <= page, wasm_bad_alloc, "freed too many pages");
         page -= size;
         int err = mprotect(raw + (page_size * page), (page_size * size), PROT_NONE);
         EOS_VM_ASSERT(err == 0, wasm_bad_alloc, "mprotect failed");
      }
      void free() {
         std::size_t syspagesize = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
         munmap(raw - syspagesize, max_memory + 2*syspagesize);
      }
      wasm_allocator() {
         std::size_t syspagesize = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
         raw  = (char*)mmap(NULL, max_memory + 2*syspagesize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
         EOS_VM_ASSERT( raw != MAP_FAILED, wasm_bad_alloc, "mmap failed to alloca pages" );
         int err = mprotect(raw, syspagesize, PROT_READ);
         EOS_VM_ASSERT(err == 0, wasm_bad_alloc, "mprotect failed");
         raw += syspagesize;
         page = 0;
      }
      void reset(uint32_t new_pages) {
         if (page != -1) {
            memset(raw, '\0', page_size * page); // zero the memory
         } else {
            std::size_t syspagesize = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
            int err = mprotect(raw - syspagesize, syspagesize, PROT_READ);
            EOS_VM_ASSERT(err == 0, wasm_bad_alloc, "mprotect failed");
         }
         // no need to mprotect if the size hasn't changed
         if (new_pages != page && page > 0) {
            int err = mprotect(raw, page_size * page, PROT_NONE); // protect the entire region of memory
            EOS_VM_ASSERT(err == 0, wasm_bad_alloc, "mprotect failed");
         }
         page = 0;
      }
      // Signal no memory defined
      void reset() {
         if (page != -1) {
            std::size_t syspagesize = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
            memset(raw, '\0', page_size * page); // zero the memory
            int err = mprotect(raw - syspagesize, page_size * page + syspagesize, PROT_NONE);
            EOS_VM_ASSERT(err == 0, wasm_bad_alloc, "mprotect failed");
         }
         page = -1;
      }
      template <typename T>
      inline T* get_base_ptr() const {
         return reinterpret_cast<T*>(raw);
      }
      template <typename T>
      inline T* create_pointer(uint32_t offset) { return reinterpret_cast<T*>(raw + offset); }
      inline int32_t get_current_page() const { return page; }
      bool is_in_region(const char* p) { return p >= raw && p < raw + max_memory;       }
      bool is_in_range(const char* p)  { 
         //std::cout << "page:" << page << " raw:" << reinterpret_cast<uint64_t>(raw) << " end:" << reinterpret_cast<uint64_t>(raw) +  page * page_size << std::endl;
         return p >= raw && p < raw + page * page_size; }
   };
}} // namespace eosio::vm
