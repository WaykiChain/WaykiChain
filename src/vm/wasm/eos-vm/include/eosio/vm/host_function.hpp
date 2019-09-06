#pragma once

#include <eosio/vm/wasm_stack.hpp>

#include <cstddef>
#include <functional>
#include <optional>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <cxxabi.h>
#include <memory>
#include <string>

namespace eosio { namespace vm {

   template <typename Derived, typename Base>
   struct construct_derived {
      static auto value(Base& base) { return Derived(base); }
   };

   template <typename T>
   struct reduce_type {
      using type = T;
   };

   template <>
   struct reduce_type<bool> {
      using type = uint32_t;
   };

   // Workaround for compiler bug handling C++17 auto template parameters.
   // The parameter is not treated as being type-dependent in all contexts,
   // causing early evaluation of the containing expression.
   // Tested at Apple LLVM version 10.0.1 (clang-1001.0.46.4)
   template<class T, class U>
   inline constexpr U&& make_dependent(U&& u) { return static_cast<U&&>(u); }
#define AUTO_PARAM_WORKAROUND(X) make_dependent<decltype(X)>(X)

   template <typename... Args, size_t... Is>
   auto get_args_full(std::index_sequence<Is...>) {
      std::tuple<std::decay_t<Args>...> tup;
      return std::tuple<Args...>{ std::get<Is>(tup)... };
   }

   template <typename R, typename... Args>
   auto get_args_full(R(Args...)) {
      return get_args_full<Args...>(std::index_sequence_for<Args...>{});
   }

   template <typename R, typename Cls, typename... Args>
   auto get_args_full(R (Cls::*)(Args...)) {
      return get_args_full<Args...>(std::index_sequence_for<Args...>{});
   }

   template <typename R, typename Cls, typename... Args>
   auto get_args_full(R (Cls::*)(Args...) const) {
      return get_args_full<Args...>(std::index_sequence_for<Args...>{});
   }

   template <typename T>
   struct return_type_wrapper {
      using type = T;
   };

   template <typename R, typename... Args>
   auto get_return_t(R(Args...)) {
      return return_type_wrapper<R>{};
   }

   template <typename R, typename Cls, typename... Args>
   auto get_return_t(R (Cls::*)(Args...)) {
      return return_type_wrapper<R>{};
   }

   template <typename R, typename Cls, typename... Args>
   auto get_return_t(R (Cls::*)(Args...) const) {
      return return_type_wrapper<R>{};
   }

   template <typename R, typename... Args>
   auto get_args(R(Args...)) {
      return std::tuple<std::decay_t<Args>...>{};
   }

   template <typename R, typename Cls, typename... Args>
   auto get_args(R (Cls::*)(Args...)) {
      return std::tuple<std::decay_t<Args>...>{};
   }

   template <typename R, typename Cls, typename... Args>
   auto get_args(R (Cls::*)(Args...) const) {
      return std::tuple<std::decay_t<Args>...>{};
   }

   template <typename T>
   using reduce_type_t = typename reduce_type<T>::type;

   template <typename T>
   struct traits {
      static constexpr uint8_t is_integral_offset = 0;
      static constexpr uint8_t is_lval_ref_offset = 1;
      static constexpr uint8_t is_ptr_offset      = 2;
      static constexpr uint8_t is_float_offset    = 3;
      static constexpr uint8_t is_4_bytes_offset  = 4;
      static constexpr uint8_t value              = (std::is_integral_v<reduce_type_t<T>> << is_integral_offset) |
                                       (std::is_lvalue_reference_v<reduce_type_t<T>> << is_lval_ref_offset) |
                                       (std::is_pointer_v<reduce_type_t<T>> << is_ptr_offset) |
                                       (std::is_floating_point_v<reduce_type_t<T>> << is_float_offset) |
                                       ((sizeof(reduce_type_t<T>) == 4) << is_4_bytes_offset);
      static constexpr uint8_t i32_value_i  = (1 << is_integral_offset) | (1 << is_4_bytes_offset);
      static constexpr uint8_t i32_value_lv = (1 << is_lval_ref_offset) | (1 << is_4_bytes_offset);
      static constexpr uint8_t i32_value_p  = (1 << is_ptr_offset) | (1 << is_4_bytes_offset);
      static constexpr uint8_t i64_value_i  = (1 << is_integral_offset);
      static constexpr uint8_t i64_value_lv = (1 << is_lval_ref_offset);
      static constexpr uint8_t i64_value_p  = (1 << is_ptr_offset);
      static constexpr uint8_t f32_value    = (1 << is_float_offset) | (1 << is_4_bytes_offset);
      static constexpr uint8_t f64_value    = (1 << is_float_offset);
   };

   template <uint8_t Traits>
   struct _to_wasm_t;

   template <>
   struct _to_wasm_t<traits<int>::i32_value_i> {
      typedef i32_const_t type;
   };

   template <>
   struct _to_wasm_t<traits<int>::i32_value_lv> {
      typedef i32_const_t type;
   };

   template <>
   struct _to_wasm_t<traits<int>::i32_value_p> {
      typedef i32_const_t type;
   };

   template <>
   struct _to_wasm_t<traits<int>::i64_value_i> {
      typedef i64_const_t type;
   };

   template <>
   struct _to_wasm_t<traits<int>::i64_value_lv> {
      typedef i32_const_t type;
   };

   template <>
   struct _to_wasm_t<traits<int>::i64_value_p> {
      typedef i32_const_t type;
   };

   template <>
   struct _to_wasm_t<traits<int>::f32_value> {
      typedef f32_const_t type;
   };

   template <>
   struct _to_wasm_t<traits<int>::f64_value> {
      typedef f64_const_t type;
   };

   template <typename T>
   using to_wasm_t = typename _to_wasm_t<traits<T>::value>::type;

   struct align_ptr_triple {
      void*  o = nullptr;
      void*  n = nullptr;
      size_t s;
   };

   // TODO clean this up
   template <typename S, typename Args, typename T, typename WAlloc>
   constexpr auto get_value(WAlloc* alloc, T&& val)
         -> std::enable_if_t<std::is_same_v<i32_const_t, T> && std::is_pointer_v<S>, S> {
      return (std::remove_const_t<S>)(alloc->template get_base_ptr<char>() + val.data.ui);
   }

   template <typename S, typename Args, typename T, typename WAlloc>
   constexpr auto get_value(WAlloc* alloc, T&& val)
         -> std::enable_if_t<std::is_same_v<i32_const_t, T> && std::is_lvalue_reference_v<S>, S> {
      return *(std::remove_const_t<std::remove_reference_t<S>>*)(alloc->template get_base_ptr<uint8_t>() + val.data.ui);
   }

   template <typename S, typename Args, typename T, typename WAlloc>
   constexpr auto get_value(WAlloc* alloc, T&& val)
         -> std::enable_if_t<std::is_same_v<i32_const_t, T> && std::is_fundamental_v<S> &&
                                   (!std::is_lvalue_reference_v<S> && !std::is_pointer_v<S>),
                             S> {
      return val.data.ui;
   }

   template <typename S, typename Args, typename T, typename WAlloc>
   constexpr auto get_value(WAlloc* walloc, T&& val)
         -> std::enable_if_t<std::is_same_v<i64_const_t, T> && std::is_fundamental_v<S>, S> {
      return val.data.ui;
   }

   template <typename S, typename Args, typename T, typename WAlloc>
   constexpr auto get_value(WAlloc* alloc, T&& val) -> std::enable_if_t<std::is_same_v<f32_const_t, T>, S> {
      return val.data.f;
   }

   template <typename S, typename Args, typename T, typename WAlloc>
   constexpr auto get_value(WAlloc* alloc, T&& val) -> std::enable_if_t<std::is_same_v<f64_const_t, T>, S> {
      return val.data.f;
   }

   template <typename T, typename WAlloc>
   constexpr auto resolve_result(T&& res, WAlloc* alloc)
         -> std::enable_if_t<std::is_pointer_v<T> || std::is_reference_v<T>, i32_const_t> {
      if constexpr (std::is_reference_v<T>) {
         return i32_const_t{ (uint32_t)((uintptr_t)&res - (uintptr_t)alloc->template get_base_ptr<uint8_t>()) };
      } else {
         return i32_const_t{ (uint32_t)((uintptr_t)res - (uintptr_t)alloc->template get_base_ptr<uint8_t>()) };
      }
   }

   template <typename T, typename WAlloc>
   constexpr auto resolve_result(T&& res, WAlloc*) -> std::enable_if_t<
         !(std::is_pointer_v<T> || std::is_reference_v<T>)&&std::is_same_v<to_wasm_t<T>, i32_const_t>, i32_const_t> {
      if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
         return i32_const_t{ res };
      } else {
         static_assert(sizeof(T) == sizeof(uint32_t));
         return i32_const_t{ *(uint32_t*)&res };
      }
   }

   template <typename T, typename WAlloc>
   constexpr auto resolve_result(T&& res, WAlloc*) -> std::enable_if_t<
         !(std::is_pointer_v<T> || std::is_reference_v<T>)&&std::is_same_v<to_wasm_t<T>, i64_const_t>, i64_const_t> {
      static_assert(sizeof(T) == sizeof(uint64_t));
      return i64_const_t{ *(uint64_t*)&res };
   }

   template <typename T, typename WAlloc>
   constexpr auto resolve_result(T&& res, WAlloc*) -> std::enable_if_t<
         !(std::is_pointer_v<T> || std::is_reference_v<T>)&&std::is_same_v<to_wasm_t<T>, f32_const_t>, f32_const_t> {
      static_assert(sizeof(T) == sizeof(uint32_t));
      return f32_const_t{ *(uint32_t*)&res };
   }

   template <typename T, typename WAlloc>
   constexpr auto resolve_result(T&& res, WAlloc*) -> std::enable_if_t<
         !(std::is_pointer_v<T> || std::is_reference_v<T>)&&std::is_same_v<to_wasm_t<T>, f64_const_t>, f64_const_t> {
      static_assert(sizeof(T) == sizeof(uint64_t));
      return f64_const_t{ *(uint64_t*)&res };
   }

   static inline std::string demangle(const char* mangled_name) {
      size_t                                          len    = 0;
      int                                             status = 0;
      ::std::unique_ptr<char, decltype(&::std::free)> ptr(
            __cxxabiv1::__cxa_demangle(mangled_name, nullptr, &len, &status), &::std::free);
      return ptr.get();
   }

   template <typename WAlloc, typename Cls, typename Cls2, auto F, typename R, typename Args, size_t... Is>
   auto create_function(std::index_sequence<Is...>) {
      return std::function<void(Cls*, WAlloc*, operand_stack&)>{ [](Cls* self, WAlloc* walloc, operand_stack& os) {
         size_t i = sizeof...(Is) - 1;
         if constexpr (!std::is_same_v<R, void>) {
            if constexpr (std::is_same_v<Cls2, std::nullptr_t>) {
               R res = std::invoke(F, get_value<typename std::tuple_element<Is, Args>::type, Args>(
                                            walloc, std::move(os.get_back(i - Is).get<to_wasm_t<typename std::tuple_element<Is, Args>::type>>()))...);
               os.trim(sizeof...(Is));
               os.push(resolve_result<R>(std::move(res), walloc));
            } else {
               R res = std::invoke(F, construct_derived<Cls2, Cls>::value(*self),
                                   get_value<typename std::tuple_element<Is, Args>::type, Args>(
                                         walloc, std::move(os.get_back(i - Is).get<to_wasm_t<typename std::tuple_element<Is, Args>::type>>()))...);
               os.trim(sizeof...(Is));
               os.push(resolve_result<R>(std::move(res), walloc));
            }
         } else {
            if constexpr (std::is_same_v<Cls2, std::nullptr_t>) {
               std::invoke(F, get_value<typename std::tuple_element<Is, Args>::type, Args>(
                                    walloc, std::move(os.get_back(i - Is).get<to_wasm_t<typename std::tuple_element<Is, Args>::type>>()))...);
            } else {
               std::invoke(F, construct_derived<Cls2, Cls>::value(*self),
                           get_value<typename std::tuple_element<Is, Args>::type, Args>(
                                 walloc, std::move(os.get_back(i - Is).get<to_wasm_t<typename std::tuple_element<Is, Args>::type>>()))...);
            }
            os.trim(sizeof...(Is));
         }
      } };
   }

   template <typename T>
   constexpr auto to_wasm_type() -> std::enable_if_t<std::is_integral<T>::value, uint8_t> {
      if constexpr (sizeof(T) == 4)
         return types::i32;
      else if constexpr (sizeof(T) == 8)
         return types::i64;
      else
         throw wasm_parse_exception{ "incompatible type" };
   }

   template <typename T>
   constexpr auto to_wasm_type() -> std::enable_if_t<std::is_floating_point<T>::value, uint8_t> {
      if constexpr (sizeof(T) == 4)
         return types::f32;
      else if constexpr (sizeof(T) == 8)
         return types::f64;
      else
         throw wasm_parse_exception{ "incompatible type" };
   }

   template <typename T>
   constexpr auto to_wasm_type() -> std::enable_if_t<std::is_lvalue_reference<T>::value, uint8_t> {
      return types::i32;
   }

   template <typename T>
   constexpr auto to_wasm_type() -> std::enable_if_t<std::is_rvalue_reference<T>::value, uint8_t> {
      if constexpr (sizeof(std::decay_t<T>) == 4)
         return types::i32;
      else if constexpr (sizeof(std::decay_t<T>) == 8)
         return types::i64;
      else
         throw wasm_parse_exception{ "incompatible type" };
   }

   template <typename T>
   constexpr auto to_wasm_type() -> std::enable_if_t<std::is_pointer<T>::value, uint8_t> {
      return types::i32;
   }

   template <typename T>
   constexpr auto to_wasm_type() -> std::enable_if_t<std::is_same<T, void>::value, uint8_t> {
      return types::ret_void;
   }

   template <typename T>
   constexpr bool is_return_void() {
      if constexpr (std::is_same<T, void>::value)
         return true;
      return false;
   }

   template <typename... Args>
   struct to_wasm_type_array {
      static constexpr uint8_t value[] = { to_wasm_type<Args>()... };
   };

   template <typename T>
   constexpr auto to_wasm_type_v = to_wasm_type<T>();

   template <typename T>
   constexpr auto is_return_void_v = is_return_void<T>();

   struct host_function {
      void*                   ptr;
      std::vector<value_type> params;
      std::vector<value_type> ret;
   };

   template <typename Ret, typename... Args>
   host_function function_types_provider() {
      host_function hf;
      hf.ptr    = (void*)func;
      hf.params = { to_wasm_type_v<Args>... };
      if constexpr (to_wasm_type_v<Ret> != types::ret_void) {
         hf.ret = { to_wasm_type_v<Ret> };
      }
      return hf;
   }

   template <typename Ret, typename... Args>
   host_function function_types_provider(Ret (*func)(Args...)) {
      return function_types_provider<Ret, Args...>();
   }

   template <typename Ret, typename Cls, typename... Args>
   host_function function_types_provider(Ret (*func)(Args...)) {
      return function_types_provider<Ret, Args...>();
   }

   template <char... Str>
   struct host_function_name {
      static constexpr const char value[] = { Str... };
      static constexpr size_t     len     = sizeof...(Str);
      static constexpr bool       is_same(const char* nm, size_t l) {
         if (len == l) {
            bool is_not_same = false;
            for (int i = 0; i < len; i++) { is_not_same |= nm[i] != value[i]; }
            return !is_not_same;
         }
         return false;
      }
   };

#if defined __clang__
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wgnu-string-literal-operator-template"
#endif
   template <typename T, T... Str>
   static constexpr host_function_name<Str...> operator""_hfn() {
      constexpr auto hfn = host_function_name<Str...>{};
      return hfn;
   }
#if defined __clang__
#   pragma clang diagnostic pop
#endif

   template <typename C, auto C::*MP, typename Name>
   struct registered_member_function {
      static constexpr auto function  = MP;
      static constexpr auto name      = Name{};
      using name_t                    = Name;
      static constexpr bool is_member = true;
   };

   using host_func_pair = std::pair<std::string, std::string>;
   struct host_func_pair_hash {
      template <class T, class U>
      std::size_t operator()(const std::pair<T, U>& p) const {
         return std::hash<T>()(p.first) ^ std::hash<U>()(p.second);
      }
   };

   template <typename Cls>
   struct registered_host_functions {
      template <typename WAlloc>
      struct mappings {
         std::unordered_map<std::pair<std::string, std::string>, uint32_t, host_func_pair_hash> named_mapping;
         std::vector<host_function>                                                             host_functions;
         std::vector<std::function<void(Cls*, WAlloc*, operand_stack&)>>                        functions;
         size_t                                                                                 current_index = 0;
      };

      template <typename WAlloc>
      static mappings<WAlloc>& get_mappings() {
         static mappings<WAlloc> _mappings;
         return _mappings;
      }

      template <typename Cls2, auto Func, typename WAlloc>
      static void add(const std::string& mod, const std::string& name) {
         using deduced_full_ts                         = decltype(get_args_full(AUTO_PARAM_WORKAROUND(Func)));
         using deduced_ts                              = decltype(get_args(AUTO_PARAM_WORKAROUND(Func)));
         using res_t                                   = typename decltype(get_return_t(AUTO_PARAM_WORKAROUND(Func)))::type;
         static constexpr auto is                      = std::make_index_sequence<std::tuple_size<deduced_ts>::value>();
         auto&                 current_mappings        = get_mappings<WAlloc>();
         current_mappings.named_mapping[{ mod, name }] = current_mappings.current_index++;
         current_mappings.functions.push_back(create_function<WAlloc, Cls, Cls2, Func, res_t, deduced_full_ts>(is));
      }

      template <typename Module>
      static void resolve(Module& mod) {
         decltype(mod.import_functions) imports          = { mod.allocator, mod.get_imported_functions_size() };
         auto&                          current_mappings = get_mappings<wasm_allocator>();
         for (int i = 0; i < mod.imports.size(); i++) {
            std::string mod_name =
                  std::string((char*)mod.imports[i].module_str.raw(), mod.imports[i].module_str.size());
            std::string fn_name = std::string((char*)mod.imports[i].field_str.raw(), mod.imports[i].field_str.size());
            EOS_WB_ASSERT(current_mappings.named_mapping.count({ mod_name, fn_name }), wasm_link_exception,
                          "no mapping for imported function");
            imports[i] = current_mappings.named_mapping[{ mod_name, fn_name }];
         }
         mod.import_functions = std::move(imports);
      }

      template <typename Execution_Context>
      void operator()(Cls* host, Execution_Context& ctx, uint32_t index) {
         const auto& _func = get_mappings<wasm_allocator>().functions[index];
         std::invoke(_func, host, ctx.get_wasm_allocator(), ctx.get_operand_stack());
      }
   };

   template <typename Cls, typename Cls2, auto F>
   struct registered_function {
      registered_function(std::string mod, std::string name) {
         registered_host_functions<Cls>::template add<Cls2, F, wasm_allocator>(mod, name);
      }
   };

#undef AUTO_PARAM_WORKAROUND

}} // namespace eosio::vm
