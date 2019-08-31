#pragma once

#include <eosio/vm/exceptions.hpp>

namespace eosio { namespace vm {
   template <typename T>
   struct guarded_ptr {
      T* raw_ptr;
      T* orig_ptr;
      T* bnds;
      guarded_ptr( T* rp, size_t bnds ) : raw_ptr(rp), orig_ptr(rp), bnds(rp + bnds) {}

      void set(T* rp, size_t bnds) {
         raw_ptr = rp;
         orig_ptr = rp;
         bnds = orig_ptr+bnds;
      }

      inline guarded_ptr& operator+=(size_t i) {
         EOS_WB_ASSERT((uintptr_t)raw_ptr + i <= (uintptr_t)bnds, guarded_ptr_exception, "overbounding pointer");
         raw_ptr += i;
         return *this;
      }

      inline guarded_ptr& operator++() {
         EOS_WB_ASSERT((uintptr_t)raw_ptr + 1 <= (uintptr_t)bnds, guarded_ptr_exception, "overbounding pointer");
         raw_ptr += 1;
         return *this;
      }

      inline guarded_ptr operator++(int) {
         guarded_ptr tmp = *this;
         ++*this;
         return tmp;
      }

      inline guarded_ptr operator+(size_t i) const {
         guarded_ptr tmp = *this;
         tmp += i;
         return tmp; 
      }
      
      inline T& operator* () {
         return *raw_ptr;
      }
      
      inline T* operator-> () {
         return raw_ptr;
      }
      
      inline T& operator= (const guarded_ptr<T>& ptr) {
         raw_ptr = ptr.raw_ptr;
         orig_ptr = ptr.orig_ptr;
         bnds     = ptr.bnds;
         return *this;
      }

      inline T* raw() {
         return raw_ptr;
      }
      
      inline size_t offset() {
         return raw_ptr - orig_ptr;
      }
      
      inline void add_bounds(size_t n) {
         bnds += n;
      } 
      
      inline void fit_bounds() {
         bnds = raw_ptr;
      }
      
      inline void fit_bounds(size_t n) {
         bnds = raw_ptr + n;
      }

      inline size_t bounds() {
         return bnds - orig_ptr;
      }

      inline T at(size_t index) const {
         EOS_WB_ASSERT((uintptr_t)raw_ptr + index <= (uintptr_t)bnds, guarded_ptr_exception, "accessing out of bounds");
         return raw_ptr[index];
      }
      
      inline T at() const {
         return *raw_ptr;
      }

      inline T operator[](size_t index) const {
         return at(index);
      }
   };
}} // ns eosio::vm
