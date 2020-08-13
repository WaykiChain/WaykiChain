#pragma once
#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#include <eosio/vm/allocator.hpp>
#include <eosio/vm/wasm_stack.hpp>
#include <eosio/vm/utils.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

// forward declaration of array_ptr
namespace eosio { namespace chain {
   template <typename T>
   struct array_ptr;
}}

namespace eosio { namespace vm {

   template <typename Derived, typename Base>
   struct construct_derived {
      static auto value(Base& base) { return Derived(&base); }
      typedef Base type;
   };

   template <typename Derived>
   struct construct_derived<Derived, nullptr_t> {
      static nullptr_t value(nullptr_t) { return nullptr; }
   };
   
   // Workaround for compiler bug handling C++g17 auto template parameters.
   // The parameter is not treated as being type-dependent in all contexts,
   // causing early evaluation of the containing expression.
   // Tested at Apple LLVM version 10.0.1 (clang-1001.0.46.4)
   template<class T, class U>
   inline constexpr U&& make_dependent(U&& u) { return static_cast<U&&>(u); }
#define AUTO_PARAM_WORKAROUND(X) make_dependent<decltype(X)>(X)

   template <typename R, typename... Args>
   auto get_args_full(R(Args...)) -> std::tuple<Args...>;

   template <typename R, typename Cls, typename... Args>
   auto get_args_full(R (Cls::*)(Args...)) -> std::tuple<Args...>;

   template <typename R, typename Cls, typename... Args>
   auto get_args_full(R (Cls::*)(Args...) const) -> std::tuple<Args...>;

   template <typename R, typename... Args>
   auto get_return_t(R(Args...)) -> R;

   template <typename R, typename Cls, typename... Args>
   auto get_return_t(R (Cls::*)(Args...)) -> R;

   template <typename R, typename Cls, typename... Args>
   auto get_return_t(R (Cls::*)(Args...) const) -> R;

   namespace detail {
      // try the pointer to force segfault early
      template <typename T>
      void validate_ptr( const void* ptr, uint32_t len ) {
         EOS_VM_ASSERT( len <= std::numeric_limits<std::uint32_t>::max() / (uint32_t)sizeof(T), wasm_interpreter_exception, "length will overflow" );
         uint32_t bytes = len * sizeof(T);
         // check the pointer
         volatile auto check = *(reinterpret_cast<const char*>(ptr) + bytes-1);
         ignore_unused_variable_warning(check);
      }

      inline void validate_c_str(const void* ptr) {
         volatile auto check = std::strlen(reinterpret_cast<const char*>(ptr));
         ignore_unused_variable_warning(check);
      }
   }

   template <typename T, std::size_t Align>
   struct aligned_ptr_wrapper {
      static_assert(Align % alignof(T) == 0, "Must align to at least the alignment of T");
      aligned_ptr_wrapper(void* ptr) : ptr(ptr) {
        if (reinterpret_cast<std::uintptr_t>(ptr) % Align != 0) {
            copy = T{};
            memcpy( &(*copy), ptr, sizeof(T) );
         }
      }
      ~aligned_ptr_wrapper() {
         if constexpr (!std::is_const_v<T>)
            if (copy)
               memcpy( ptr, &(*copy), sizeof(T) );
      }
      constexpr operator T*() const {
         if (copy)
            return &(*copy);
         else
            return static_cast<T*>(ptr);
      }

      void* ptr;
      mutable std::optional<std::remove_cv_t<T>> copy;

      aligned_ptr_wrapper(const aligned_ptr_wrapper&) = delete;
   };

   template <typename T, std::size_t Align>
   struct aligned_array_wrapper {
      static_assert(Align % alignof(T) == 0, "Must align to at least the alignment of T");
      aligned_array_wrapper(void* ptr, uint32_t size) : ptr(ptr), size(size) {
         if (reinterpret_cast<std::uintptr_t>(ptr) % Align != 0) {
            copy.reset(new std::remove_cv_t<T>[size]);
            memcpy( copy.get(), ptr, sizeof(T) * size );
         }
      }
      ~aligned_array_wrapper() {
         if constexpr (!std::is_const_v<T>)
            if (copy)
               memcpy( ptr, copy.get(), sizeof(T) * size);
      }
      constexpr operator T*() const {
         if (copy)
            return copy.get();
         else
            return static_cast<T*>(ptr);
      }
      constexpr operator eosio::chain::array_ptr<T>() const {
        return eosio::chain::array_ptr<T>{static_cast<T*>(*this)};
      }

      void* ptr;
      std::unique_ptr<std::remove_cv_t<T>[]> copy = nullptr;
      std::size_t size;

      aligned_array_wrapper(const aligned_array_wrapper&) = delete;
   };

   template <typename T, std::size_t Align>
   struct aligned_ref_wrapper {
      constexpr aligned_ref_wrapper(void* ptr) : _impl(ptr) {}
      constexpr operator T&() const {
         return *static_cast<T*>(_impl);
      }

      aligned_ptr_wrapper<T, Align> _impl;
   };

   template<typename T, std::size_t Align = alignof(T)>
   struct aligned_ptr {
      aligned_ptr(const aligned_ptr_wrapper<T, Align>& wrapper) : _ptr(wrapper) {}
      operator T*() const { return _ptr; }
      T* _ptr;
   };

   template<typename T, std::size_t Align = alignof(T)>
   struct aligned_ref {
      aligned_ref(const aligned_ptr_wrapper<T, Align>& wrapper) : _ptr(wrapper) {}
      operator T&() const { return *_ptr; }
      T* _ptr;
   };

   // This class can be specialized to define a conversion to/from wasm.
   template <typename T>
   struct wasm_type_converter {};

   struct linear_memory_access {
      void* get_ptr(uint32_t val) const {
         return _alloc->template get_base_ptr<char>() + val;
      }
      template<typename T>
      void validate_ptr(const void* ptr, uint32_t len) {
         eosio::vm::detail::validate_ptr<T>(ptr, len);
      }
      void validate_c_str(const void* ptr) {
         eosio::vm::detail::validate_c_str(ptr);
      }
      wasm_allocator* _alloc;
   };

   namespace detail {
      template<typename T, typename U, typename... Args>
      auto from_wasm_type_impl(T (*)(U, Args...)) -> U;
      template<typename T>
      using from_wasm_type_impl_t = decltype(detail::from_wasm_type_impl(&wasm_type_converter<T>::from_wasm));

      template<typename T, typename U, typename... Args>
      auto to_wasm_type_impl(T (*)(U, Args...)) -> T;
      template<typename T>
      using to_wasm_type_impl_t = decltype(detail::to_wasm_type_impl(&wasm_type_converter<T>::to_wasm));

      // Extract the wasm type from wasm_type_converter and verify
      // that if both from_wasm and to_wasm are defined, they use
      // the same type.
      template<typename T, typename HasFromWasm = void, typename HasToWasm = void>
      struct get_wasm_type;
      template<typename T, typename HasToWasm>
      struct get_wasm_type<T, std::void_t<from_wasm_type_impl_t<T>>, HasToWasm> {
         using type = from_wasm_type_impl_t<T>;
      };
      template<typename T, typename HasFromWasm>
      struct get_wasm_type<T, HasFromWasm, std::void_t<from_wasm_type_impl_t<T>>> {
         using type = to_wasm_type_impl_t<T>;
      };
      template<typename T>
      struct get_wasm_type<T, std::void_t<from_wasm_type_impl_t<T>>, std::void_t<to_wasm_type_impl_t<T>>> {
         static_assert(std::is_same_v<from_wasm_type_impl_t<T>, to_wasm_type_impl_t<T>>,
                       "wasm_type_converter must use the same type for both from_wasm and to_wasm.");
         using type = from_wasm_type_impl_t<T>;
      };

      // Allow from_wasm to return a wrapper that holds extra data.
      // StorageType must be implicitly convertible to T
      template<typename T, typename StorageType>
      struct cons_item {
         template<typename F>
         explicit cons_item(F&& f) : _value(f()) {}
         StorageType _value;
         T get() const { return _value; }
      };

      template<typename Car, typename Cdr>
      struct cons {
         // Make sure that we get mandatory RVO, so that we can guarantee
         // that user provided destructors run exactly once in the right sequence.
         template<typename F, typename FTail>
         cons(F&& f, FTail&& ftail) : cdr(ftail()), car(f(cdr)) {}
         static constexpr std::size_t size = Cdr::size + 1;
         // Reverse order to get the order of construction and destruction right
         // We want to construct in reverse order and destroy in forwards order.
         Cdr cdr;
         Car car;
         // We need mandatory RVO.  deleting the copy constructor should
         // cause an error if we messed that up.
         cons(const cons&) = delete;
         cons& operator=(const cons&) = delete;
      };

      template<>
      struct cons<void, void> { static constexpr std::size_t size = 0; };
      using nil_t = cons<void, void>;

      template<std::size_t I, typename Car, typename Cdr>
      decltype(auto) cons_get(const cons<Car, Cdr>& l) {
         if constexpr (I == 0) {
            return l.car.get();
         } else {
            return detail::cons_get<I-1>(l.cdr);
         }
      }

      // get the type of the Ith element of the cons list.
      template<std::size_t I, typename Cons>
      using cons_item_t = decltype(detail::cons_get<I>(std::declval<Cons>()));

      template<typename S, typename T, typename WAlloc, typename Cons>
      constexpr decltype(auto) get_value(WAlloc* alloc, T&& val, Cons& tail);

      template<typename WAlloc>
      void maybe_add_linear_memory_access(linear_memory_access* arg, WAlloc* walloc) {
         arg->_alloc = walloc;
      }
      inline void maybe_add_linear_memory_access(void*, void*) {}

      template<typename Converter, typename WAlloc>
      Converter&& init_wasm_type_converter(Converter&& c, WAlloc* walloc) {
         ::eosio::vm::detail::maybe_add_linear_memory_access(&c, walloc);
         return static_cast<Converter&&>(c);
      }

      // Calls from_wasm with the correct arguments
      template<typename A, typename SourceType, typename WAlloc, typename Tail, typename T, std::size_t... Is>
      decltype(auto) get_value_impl(std::index_sequence<Is...>, WAlloc* alloc, T&& val, const Tail& tail) {
         return detail::init_wasm_type_converter(wasm_type_converter<A>{}, alloc)
            .from_wasm(get_value<SourceType>(alloc, static_cast<T&&>(val), tail), cons_get<Is>(tail)...);
      }

      // Matches a specific overload of a function and deduces the first argument
      template<typename... Rest>
      struct match_from_wasm {
         template<typename R, typename U>
         static U apply(R(U, Rest...));
         template<typename R, typename C, typename U>
         static U apply(R (C::*)(U, Rest...));
         template<typename R, typename C, typename U>
         static U apply(R (C::*)(U, Rest...) const);
      };

      // Encodes how to convert a value from wasm to native.  LookaheadCount is the number
      // of extra arguments to pass to from_wasm.  SourceType is type to convert from.
      template<std::size_t LookaheadCount, typename SourceType>
      struct value_getter {
         template<typename A, typename WAlloc, typename T, typename Tail>
         decltype(auto) apply(WAlloc* alloc, T&& val, const Tail& tail) {
            return get_value_impl<A, SourceType>(std::make_index_sequence<LookaheadCount>{}, alloc, static_cast<T&&>(val), tail);
         }
      };

      // Detect whether there is a match of from_wasm with these arguments.
      // returns a value_getter or void if it didn't match.
      template<typename T, typename Cons, std::size_t... Is>
      auto try_value_getter(std::index_sequence<Is...>)
         -> value_getter<sizeof...(Is), decltype(match_from_wasm<cons_item_t<Is, Cons>...>::apply(&wasm_type_converter<T>::from_wasm))>;
      // Fallback
      template<typename T, typename Cons>
      auto try_value_getter(...) -> void;

      template<std::size_t N, typename T, typename Cons>
      using try_value_getter_t = decltype(try_value_getter<T, Cons>(std::make_index_sequence<N>{}));

      // Error type to hopefully make a somewhat understandable error message when
      // the user did not provide a correct overload of from_wasm.
      template<typename T, typename... Tail>
      struct no_viable_overload_of_from_wasm {};

      // Searches for a match of from_wasm following the principal of maximum munch.
      template<std::size_t N, typename T, typename Tail>
      constexpr auto make_value_getter_impl() {
         if constexpr(std::is_same_v<try_value_getter_t<N, T, Tail>, void>) {
            if constexpr (N == 0) {
               return no_viable_overload_of_from_wasm<T>{};
            } else {
               return make_value_getter_impl<N-1, T, Tail>();
            }
         } else {
            return try_value_getter_t<N, T, Tail>{};
         }
      }

      template<typename T, typename Tail>
      constexpr auto make_value_getter() {
         return make_value_getter_impl<Tail::size, T, Tail>();
      }

      // args should be a tuple
      // Constructs a cons of the translated arguments given a list of
      // argument types, a wasm stack, and a wasm allocator.
      template<typename Args>
      struct pack_args;

      template<typename T, typename F>
      auto make_cons_item(F&& f) {
         return [=](auto& arg) { return cons_item<T, decltype(f(arg))>{[&]() -> decltype(auto) { return f(arg); }}; };
      }

      template<typename T0, typename... T>
      struct pack_args<std::tuple<T0, T...>> {
         using next = pack_args<std::tuple<T...>>;
         template<typename WAlloc, typename Os>
         static auto apply(WAlloc* alloc, Os& os) {
            using next_result = decltype(next::apply(alloc, os));
            auto item_maker = make_cons_item<T0>([&](auto& tail) -> decltype(auto) { return get_value<T0>(alloc, os.get_back(sizeof...(T)), tail); });
            using result_type = cons<decltype(item_maker(std::declval<next_result&>())), next_result>;
            return result_type(item_maker,
              [&](){ return next::apply(alloc, os); } );
         }
      };

      template<>
      struct pack_args<std::tuple<>> {
         template<typename WAlloc, typename Os>
         static auto apply(WAlloc* alloc, Os& os) { return nil_t{}; }
      };

      template<typename T>
      std::decay_t<T> as_value(T&& arg) { return static_cast<T&&>(arg); }

      template<typename S, typename T, typename WAlloc, typename Cons>
      constexpr decltype(auto) get_value(WAlloc* alloc, T&& val, Cons& tail) {
         if constexpr (std::is_integral_v<S> && sizeof(S) == 4)
            return as_value(val.template get<i32_const_t>().data.ui);
         else if constexpr (std::is_integral_v<S> && sizeof(S) == 8)
            return as_value(val.template get<i64_const_t>().data.ui);
         else if constexpr (std::is_floating_point_v<S> && sizeof(S) == 4)
            return as_value(val.template get<f32_const_t>().data.f);
         else if constexpr (std::is_floating_point_v<S> && sizeof(S) == 8)
            return as_value(val.template get<f64_const_t>().data.f);
         else if constexpr (std::is_void_v<std::decay_t<std::remove_pointer_t<S>>>)
            return reinterpret_cast<S>(alloc->template get_base_ptr<char>() + val.template get<i32_const_t>().data.ui);
         //xiaoyu1998
         else if constexpr (std::is_pointer_v<std::decay_t<S>>)
            return reinterpret_cast<S>(alloc->template get_base_ptr<char>() + val.template get<i32_const_t>().data.ui);
         else if constexpr (std::is_lvalue_reference_v<S>)
            return *(std::remove_const_t<std::remove_reference_t<S>>*)(alloc->template get_base_ptr<uint8_t>() + val.template get<i32_const_t>().data.ui);
         else {
            return detail::make_value_getter<S, Cons>().template apply<S>(alloc, static_cast<T&&>(val), tail);
         }
      }

      template <typename T, typename WAlloc>
      constexpr auto resolve_result(T&& res, WAlloc* alloc) {
         if constexpr (std::is_integral_v<T> && sizeof(T) == 4)
            return i32_const_t{ static_cast<uint32_t>(res) };
         else if constexpr (std::is_integral_v<T> && sizeof(T) == 8)
            return i64_const_t{ static_cast<uint64_t>(res) };
         else if constexpr (std::is_floating_point_v<T> && sizeof(T) == 4)
            return f32_const_t{ static_cast<float>(res) };
         else if constexpr (std::is_floating_point_v<T> && sizeof(T) == 8)
            return f64_const_t{ static_cast<double>(res) };
         else if constexpr (std::is_void_v<std::decay_t<std::remove_pointer_t<T>>>)
            return i32_const_t{ static_cast<uint32_t>(reinterpret_cast<uintptr_t>(res) -
                                                      reinterpret_cast<uintptr_t>(alloc->template get_base_ptr<char>())) };
         //xiaoyu1998
         else if constexpr (std::is_pointer_v<std::decay_t<T>>)
            return i32_const_t{ static_cast<uint32_t>(reinterpret_cast<uintptr_t>(res) -
                                                      reinterpret_cast<uintptr_t>(alloc->template get_base_ptr<char>())) };
         else
            return detail::resolve_result(detail::init_wasm_type_converter(wasm_type_converter<T>{}, alloc)
                                          .to_wasm(static_cast<T&&>(res)), alloc);
      }
   } //ns detail

   template <>
   struct wasm_type_converter<bool> {
      static bool from_wasm(uint32_t val) { return val != 0; }
      static uint32_t to_wasm(bool val) { return val? 1 : 0; }
   };

   template<typename T, std::size_t Align>
   struct wasm_type_converter<aligned_ptr<T, Align>> {
      aligned_ptr_wrapper<T, Align> from_wasm(void* ptr) { return {ptr}; }
   };

   template<typename T, std::size_t Align>
   struct wasm_type_converter<aligned_ref<T, Align>> {
      aligned_ptr_wrapper<T, Align> from_wasm(void* ptr) { return {ptr}; }
   };

   template <typename T>
   inline constexpr auto to_wasm_type() -> uint8_t {
      if constexpr (std::is_same_v<T, void>)
         return types::ret_void;
      else if constexpr (std::is_same_v<T, bool>)
         return types::i32;
      else if constexpr (std::is_integral_v<T> && sizeof(T) == 4)
         return types::i32;
      else if constexpr (std::is_integral_v<T> && sizeof(T) == 8)
         return types::i64;
      else if constexpr (std::is_floating_point_v<T> && sizeof(T) == 4)
         return types::f32;
      else if constexpr (std::is_floating_point_v<T> && sizeof(T) == 8)
         return types::f64;
      else if constexpr (std::is_pointer_v<T> || std::is_reference_v<T>)
         return types::i32;
      else
         return vm::to_wasm_type<typename detail::get_wasm_type<T>::type>();
   }

   template <uint8_t Type>
   struct _to_wasm_t;

   template <>
   struct _to_wasm_t<types::i32> {
      typedef i32_const_t type;
   };

   template <>
   struct _to_wasm_t<types::i64> {
      typedef i64_const_t type;
   };

   template <>
   struct _to_wasm_t<types::f32> {
      typedef f32_const_t type;
   };

   template <>
   struct _to_wasm_t<types::f64> {
      typedef f64_const_t type;
   };

   template <typename T>
   using to_wasm_t = typename _to_wasm_t<to_wasm_type<T>()>::type;

   template<auto F, typename Derived, typename Host, typename... T>
   decltype(auto) invoke_with_host(Host* host, T&&... args) {
      if constexpr (std::is_same_v<Derived, nullptr_t>)
         return std::invoke(F, static_cast<T&&>(args)...);
      else
         return std::invoke(F, construct_derived<Derived, Host>::value(*host), static_cast<T&&>(args)...);
   }

   template<auto F, typename Derived, typename Host, typename Cons, std::size_t... Is>
   decltype(auto) invoke_with_cons(Host* host, Cons&& args, std::index_sequence<Is...>) {
      return invoke_with_host<F, Derived>(host, detail::cons_get<Is>(args)...);
   }

   template<typename T, typename Os, typename WAlloc>
   void maybe_push_result(T&& res, Os& os, WAlloc* walloc, std::size_t trim_amt) {
      if constexpr (!std::is_same_v<std::decay_t<T>, maybe_void_t>) {
         os.trim(trim_amt);
         os.push(detail::resolve_result(static_cast<T&&>(res), walloc));
      } else {
         os.trim(trim_amt);
      }
   }

   template <typename Walloc, typename Cls, typename Cls2, auto F, typename R, typename Args, size_t... Is>
   auto create_function(std::index_sequence<Is...>) {
      return std::function<void(Cls*, Walloc*, operand_stack&)>{ [](Cls* self, Walloc* walloc, operand_stack& os) {
         maybe_push_result(
            (invoke_with_cons<F, Cls2>(self, detail::pack_args<Args>::apply(walloc, os), std::index_sequence<Is...>{}), maybe_void),
            os,
            walloc,
            sizeof...(Is));
      } };
   }

   template <typename T>
   constexpr auto to_wasm_type_v = to_wasm_type<T>();

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
         using res_t                                   = decltype(get_return_t(AUTO_PARAM_WORKAROUND(Func)));
         static constexpr auto is                      = std::make_index_sequence<std::tuple_size_v<deduced_full_ts>>();
         auto&                 current_mappings        = get_mappings<WAlloc>();
         current_mappings.named_mapping[{ mod, name }] = current_mappings.current_index++;
         current_mappings.functions.push_back(create_function<WAlloc, Cls, Cls2, Func, res_t, deduced_full_ts>(is));
      }

      template <typename Module>
      static void resolve(Module& mod) {
         auto& imports          = mod.import_functions;
         auto& current_mappings = get_mappings<wasm_allocator>();
         for (int i = 0; i < mod.imports.size(); i++) {
            std::string mod_name =
                  std::string((char*)mod.imports[i].module_str.raw(), mod.imports[i].module_str.size());
            std::string fn_name = std::string((char*)mod.imports[i].field_str.raw(), mod.imports[i].field_str.size());

            //std::cout << "mod_name:" << mod_name << " fn_name:" << fn_name << std::endl;
            std::string err_msg = "no mapping for imported function:"+mod_name+"."+fn_name;
            EOS_VM_ASSERT(current_mappings.named_mapping.count({ mod_name, fn_name }), wasm_link_exception,
                          err_msg.c_str());
            imports[i] = current_mappings.named_mapping[{ mod_name, fn_name }];
         }
      }

      template <typename Execution_Context>
      void operator()(Cls* host, Execution_Context& ctx, uint32_t index) {
         const auto& _func = get_mappings<wasm_allocator>().functions[index];
         std::invoke(_func, host, ctx.get_wasm_allocator(), ctx.get_operand_stack());
         // std::cout << "host:" << (uint64_t)host 
         //           << std::endl;
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
