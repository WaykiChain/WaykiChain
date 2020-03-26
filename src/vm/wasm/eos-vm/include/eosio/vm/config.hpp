#pragma once

namespace eosio { namespace vm {

// create constexpr flags for whether the backend should obey alignment hints
#ifdef EOS_VM_ALIGN_MEMORY_OPS
   inline constexpr bool should_align_memory_ops = true;
#else
   inline constexpr bool should_align_memory_ops = false;
#endif


#ifdef EOS_VM_SOFTFLOAT
   inline constexpr bool use_softfloat = true;
#else
   inline constexpr bool use_softfloat = false;
#endif

#ifdef EOS_VM_FULL_DEBUG
   inline constexpr bool eos_vm_debug = true;
#else
   inline constexpr bool eos_vm_debug = false;
#endif

}} // namespace eosio::vm
