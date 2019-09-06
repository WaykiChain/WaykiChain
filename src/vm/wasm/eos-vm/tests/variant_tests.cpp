#include <eosio/vm/variant.hpp>

#include <catch2/catch.hpp>

using namespace eosio;
using namespace eosio::vm;

TEST_CASE("Testing variant with stateless class", "[variant_stateless_tests]") {
   struct vis {
      int operator()(signed char v) { return v + 1; }
      int operator()(short v) { return v + 2; }
      int operator()(int v) { return v + 3; }
      int operator()(long v) { return v + 4; }
      int operator()(float v) { return v + 5.0f; }
      int operator()(double v) { return v + 6.0; }
   };

   using variant_type = variant<signed char, short, int, long, float, double>;

   {
      variant_type v((signed char)42);
      CHECK(visit(vis{}, v) == 43);
   }
   {
      variant_type v((short)142);
      CHECK(visit(vis{}, v) == 144);
   }
   {
      variant_type v((int)242);
      CHECK(visit(vis{}, v) == 245);
   }
   {
      variant_type v((long)342);
      CHECK(visit(vis{}, v) == 346);
   }
   {
      variant_type v((float)442);
      CHECK(visit(vis{}, v) == 447);
   }
   {
      variant_type v((double)542);
      CHECK(visit(vis{}, v) == 548);
   }
}


TEST_CASE("Testing argument forwarding for visit", "[variant_forward_tests]") {

   using variant_type = variant<signed char, short, int, long, float, double>;

   // visitor forwarding

   // lvalue
   {
      struct vis {
         int r = -1;
         void operator()(double v) & { r = v + 1; }
         void operator()(...) {}
      };
      variant_type v((double)42);
      vis f;
      visit(f, v);
      CHECK(f.r == 43);
   }
   // rvalue
   {
      struct vis {
         int r = -1;
         void operator()(double v) && { r = v + 1; }
         void operator()(...) {}
      };
      variant_type v((double)42);
      vis f;
      visit(std::move(f), v);
      CHECK(f.r == 43);
   }
   // const lvalue
   {
      struct vis {
         mutable int r = -1;
         void operator()(double v) const & { r = v + 1; }
         void operator()(...) {}
      };
      variant_type v((double)42);
      const vis f;
      visit(f, v);
      CHECK(f.r == 43);
   }

   // variant forwarding

   // lvalue
   {
      struct vis {
         void operator()(double& v) { v = v + 1; }
         void operator()(...) {}
      };
      variant_type v((double)42);
      visit(vis{}, v);
      CHECK(v.get<double>() == 43);
   }

   // rvalue
   {
      struct vis {
         void operator()(double&& v) {
            v = v + 1;
         }
         void operator()(...) {}
      };
      variant_type v((double)42);
      visit(vis{}, std::move(v));
      CHECK(v.get<double>() == 43);
   }

   // const lvalue
   {
      struct vis {
         void operator()(const double& v) {
            const_cast<double&>(v) = v + 1;
         }
         void operator()(...) {}
      };
      // Don't declare this const, because that would make casting away const illegal.
      variant_type v((double)42);
      visit(vis{}, static_cast<const variant_type&>(v));
      CHECK(v.get<double>() == 43);
   }
}

template<typename V, typename T>
constexpr bool variant_constexpr_assign(T&& t) {
   V v{t};
   T expected{t};
   bool result = true;
   v = static_cast<T&&>(t);
   result = result && (v.template get<T>() == expected);
   v = t;
   result = result && (v.template get<T>() == expected);
   v = static_cast<const T&>(t);
   result = result && (v.template get<T>() == expected);

   // copy and move assignment
   v = static_cast<V&>(v);
   result = result && (v.template get<T>() == expected);
   v = static_cast<V&&>(v);
   result = result && (v.template get<T>() == expected);
   v = static_cast<const V&>(v);
   result = result && (v.template get<T>() == expected);
   return result;
}

template<typename V, typename T>
constexpr bool variant_constexpr_get(T&& t) {
   V v{t};
   T expected{t};
   bool result = true;
   static_assert(std::is_same_v<decltype(v.template get<T>()), T&>, "check get result type");
   static_assert(std::is_same_v<decltype(static_cast<V&&>(v).template get<T>()), T&&>, "check get result type");
   static_assert(std::is_same_v<decltype(static_cast<const V&>(v).template get<T>()), const T&>, "check get result type");
   static_assert(std::is_same_v<decltype(static_cast<const V&&>(v).template get<T>()), const T&&>, "check get result type");

   result = result && (v.template get<T>() == expected);
   result = result && (static_cast<V&&>(v).template get<T>() == expected);
   result = result && (static_cast<const V&>(v).template get<T>() == expected);
   result = result && (static_cast<const V&&>(v).template get<T>() == expected);
   return result;
}

TEST_CASE("Testing constexpr variant", "[variant_constexpr_tests]") {
   using variant_type = variant<signed char, short, int, long, float, double>;

   {
      constexpr variant_type v{(signed char)2};
      constexpr signed char result_lv{v.get<signed char>()};
      CHECK(result_lv == 2);
      constexpr signed char result_rv{variant_type{(signed char)4}.get<signed char>()};
      CHECK(result_rv == 4);
   }
   {
      constexpr variant_type v{(double)2};
      constexpr double result_lv{v.get<double>()};
      CHECK(result_lv == 2);
      constexpr double result_rv{variant_type{(double)4}.get<double>()};
      CHECK(result_rv == 4);
   }
   constexpr bool assign0 = variant_constexpr_assign<variant_type>((signed char)2);
   CHECK(assign0);
   constexpr bool assign1 = variant_constexpr_assign<variant_type>((double)4);
   CHECK(assign1);

   constexpr bool get0 = variant_constexpr_get<variant_type>((signed char)2);
   CHECK(get0);
   constexpr bool get1 = variant_constexpr_get<variant_type>((double)4);
   CHECK(get1);

#ifdef CONSTEXPR_STD_INVOKE
   {
      struct vis {
         constexpr int operator()(signed char v) { return v + 1; }
         constexpr int operator()(short v) { return v + 2; }
         constexpr int operator()(int v) { return v + 3; }
         constexpr int operator()(long v) { return v + 4; }
         constexpr int operator()(float v) { return v + 5.0f; }
         constexpr int operator()(double v) { return v + 6.0; }
      };

      using variant_type = variant<signed char, short, int, long, float, double>;

      {
         constexpr int test = visit(vis{}, variant_type{(signed char)2});
         CHECK(test == 3);
      }
   }
#endif
}

// Minimal requirements.  Delete everything that shouldn't be needed by visit
struct minimal_vis {
   minimal_vis(const minimal_vis&) = delete;
   minimal_vis& operator=(const minimal_vis&) = delete;
   template<typename T>
   int operator()(T v) const {
      return v + 1;
   }
   static minimal_vis& make() { static minimal_vis singleton; return singleton; }
 private:
   minimal_vis() = default;
   ~minimal_vis() = default;
};
void check_requirements() {
   using variant_type = variant<signed char, short, int, long, float, double>;
   variant_type v((double)42);
   visit(minimal_vis::make(), v);
   visit(std::move(minimal_vis::make()), v);
   visit(const_cast<const minimal_vis&>(minimal_vis::make()), v);
}
