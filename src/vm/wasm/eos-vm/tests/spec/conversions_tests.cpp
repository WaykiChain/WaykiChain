#include <algorithm>
#include <catch2/catch.hpp>
#include <cmath>
#include <cstdlib>
#include <eosio/vm/backend.hpp>
#include <iostream>
#include <iterator>
#include <utils.hpp>
#include <vector>
#include <wasm_config.hpp>

using namespace eosio;
using namespace eosio::vm;
extern wasm_allocator wa;
using backend_t = backend<std::nullptr_t>;

TEST_CASE("Testing wasm <conversions_0_wasm>", "[conversions_0_wasm_tests]") {
   auto      code = backend_t::read_wasm(conversions_0_wasm);
   backend_t bkend(code);
   bkend.set_wasm_allocator(&wa);

   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_s", static_cast<uint32_t>(0))) ==
         static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_s", static_cast<uint32_t>(10000))) ==
         static_cast<uint64_t>(10000));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_s", static_cast<uint32_t>(4294957296))) ==
         static_cast<uint64_t>(18446744073709541616));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_s", static_cast<uint32_t>(4294967295))) ==
         static_cast<uint64_t>(18446744073709551615));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_s", static_cast<uint32_t>(2147483647))) ==
         static_cast<uint64_t>(2147483647));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_s", static_cast<uint32_t>(2147483648))) ==
         static_cast<uint64_t>(18446744071562067968));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_u", static_cast<uint32_t>(0))) ==
         static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_u", static_cast<uint32_t>(10000))) ==
         static_cast<uint64_t>(10000));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_u", static_cast<uint32_t>(4294957296))) ==
         static_cast<uint64_t>(4294957296));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_u", static_cast<uint32_t>(4294967295))) ==
         static_cast<uint64_t>(4294967295));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_u", static_cast<uint32_t>(2147483647))) ==
         static_cast<uint64_t>(2147483647));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_u", static_cast<uint32_t>(2147483648))) ==
         static_cast<uint64_t>(2147483648));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", static_cast<uint64_t>(18446744073709551615))) ==
         static_cast<uint32_t>(4294967295));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", static_cast<uint64_t>(18446744073709451616))) ==
         static_cast<uint32_t>(4294867296));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", static_cast<uint64_t>(2147483648))) ==
         static_cast<uint32_t>(2147483648));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", static_cast<uint64_t>(18446744071562067967))) ==
         static_cast<uint32_t>(2147483647));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", static_cast<uint64_t>(18446744069414584320))) ==
         static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", static_cast<uint64_t>(18446744069414584319))) ==
         static_cast<uint32_t>(4294967295));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", static_cast<uint64_t>(18446744069414584321))) ==
         static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", static_cast<uint64_t>(0))) ==
         static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", static_cast<uint64_t>(1311768467463790320))) ==
         static_cast<uint32_t>(2596069104));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", static_cast<uint64_t>(4294967295))) ==
         static_cast<uint32_t>(4294967295));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", static_cast<uint64_t>(4294967296))) ==
         static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", static_cast<uint64_t>(4294967297))) ==
         static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s",
                                        static_cast<float>(type_converter32(0).to_f()))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s",
                                        static_cast<float>(type_converter32(2147483648).to_f()))) ==
         static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s",
                                        static_cast<float>(type_converter32(1).to_f()))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s",
                                        static_cast<float>(type_converter32(2147483649).to_f()))) ==
         static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s",
                                        static_cast<float>(type_converter32(1065353216).to_f()))) ==
         static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s",
                                        static_cast<float>(type_converter32(1066192077).to_f()))) ==
         static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s",
                                        static_cast<float>(type_converter32(1069547520).to_f()))) ==
         static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s",
                                        static_cast<float>(type_converter32(3212836864).to_f()))) ==
         static_cast<uint32_t>(4294967295));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s",
                                        static_cast<float>(type_converter32(3213675725).to_f()))) ==
         static_cast<uint32_t>(4294967295));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s",
                                        static_cast<float>(type_converter32(3217031168).to_f()))) ==
         static_cast<uint32_t>(4294967295));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s",
                                        static_cast<float>(type_converter32(3220386611).to_f()))) ==
         static_cast<uint32_t>(4294967295));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s",
                                        static_cast<float>(type_converter32(3221225472).to_f()))) ==
         static_cast<uint32_t>(4294967294));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s",
                                        static_cast<float>(type_converter32(1325400063).to_f()))) ==
         static_cast<uint32_t>(2147483520));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s",
                                        static_cast<float>(type_converter32(3472883712).to_f()))) ==
         static_cast<uint32_t>(2147483648));
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.trunc_f32_s", static_cast<float>(type_converter32(1325400064).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.trunc_f32_s", static_cast<float>(type_converter32(3472883713).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.trunc_f32_s", static_cast<float>(type_converter32(2139095040).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.trunc_f32_s", static_cast<float>(type_converter32(4286578688).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.trunc_f32_s", static_cast<float>(type_converter32(2143289344).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.trunc_f32_s", static_cast<float>(type_converter32(2141192192).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.trunc_f32_s", static_cast<float>(type_converter32(4290772992).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.trunc_f32_s", static_cast<float>(type_converter32(4288675840).to_f())),
                   std::exception);
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u",
                                        static_cast<float>(type_converter32(0).to_f()))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u",
                                        static_cast<float>(type_converter32(2147483648).to_f()))) ==
         static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u",
                                        static_cast<float>(type_converter32(1).to_f()))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u",
                                        static_cast<float>(type_converter32(2147483649).to_f()))) ==
         static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u",
                                        static_cast<float>(type_converter32(1065353216).to_f()))) ==
         static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u",
                                        static_cast<float>(type_converter32(1066192077).to_f()))) ==
         static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u",
                                        static_cast<float>(type_converter32(1069547520).to_f()))) ==
         static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u",
                                        static_cast<float>(type_converter32(1072902963).to_f()))) ==
         static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u",
                                        static_cast<float>(type_converter32(1073741824).to_f()))) ==
         static_cast<uint32_t>(2));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u",
                                        static_cast<float>(type_converter32(1325400064).to_f()))) ==
         static_cast<uint32_t>(2147483648));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u",
                                        static_cast<float>(type_converter32(1333788671).to_f()))) ==
         static_cast<uint32_t>(4294967040));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u",
                                        static_cast<float>(type_converter32(3211159142).to_f()))) ==
         static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u",
                                        static_cast<float>(type_converter32(3212836863).to_f()))) ==
         static_cast<uint32_t>(0));
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.trunc_f32_u", static_cast<float>(type_converter32(1333788672).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.trunc_f32_u", static_cast<float>(type_converter32(3212836864).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.trunc_f32_u", static_cast<float>(type_converter32(2139095040).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.trunc_f32_u", static_cast<float>(type_converter32(4286578688).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.trunc_f32_u", static_cast<float>(type_converter32(2143289344).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.trunc_f32_u", static_cast<float>(type_converter32(2141192192).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.trunc_f32_u", static_cast<float>(type_converter32(4290772992).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i32.trunc_f32_u", static_cast<float>(type_converter32(4288675840).to_f())),
                   std::exception);
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s",
                                        static_cast<double>(type_converter64(0).to_f()))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s",
                                        static_cast<double>(type_converter64(9223372036854775808).to_f()))) ==
         static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s",
                                        static_cast<double>(type_converter64(1).to_f()))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s",
                                        static_cast<double>(type_converter64(9223372036854775809).to_f()))) ==
         static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s",
                                        static_cast<double>(type_converter64(4607182418800017408).to_f()))) ==
         static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s",
                                        static_cast<double>(type_converter64(4607632778762754458).to_f()))) ==
         static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s",
                                        static_cast<double>(type_converter64(4609434218613702656).to_f()))) ==
         static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s",
                                        static_cast<double>(type_converter64(13830554455654793216).to_f()))) ==
         static_cast<uint32_t>(4294967295));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s",
                                        static_cast<double>(type_converter64(13831004815617530266).to_f()))) ==
         static_cast<uint32_t>(4294967295));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s",
                                        static_cast<double>(type_converter64(13832806255468478464).to_f()))) ==
         static_cast<uint32_t>(4294967295));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s",
                                        static_cast<double>(type_converter64(13834607695319426662).to_f()))) ==
         static_cast<uint32_t>(4294967295));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s",
                                        static_cast<double>(type_converter64(13835058055282163712).to_f()))) ==
         static_cast<uint32_t>(4294967294));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s",
                                        static_cast<double>(type_converter64(4746794007244308480).to_f()))) ==
         static_cast<uint32_t>(2147483647));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s",
                                        static_cast<double>(type_converter64(13970166044103278592).to_f()))) ==
         static_cast<uint32_t>(2147483648));
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i32.trunc_f64_s", static_cast<double>(type_converter64(4746794007248502784).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i32.trunc_f64_s", static_cast<double>(type_converter64(13970166044105375744).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i32.trunc_f64_s", static_cast<double>(type_converter64(9218868437227405312).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i32.trunc_f64_s", static_cast<double>(type_converter64(18442240474082181120).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i32.trunc_f64_s", static_cast<double>(type_converter64(9221120237041090560).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i32.trunc_f64_s", static_cast<double>(type_converter64(9219994337134247936).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i32.trunc_f64_s", static_cast<double>(type_converter64(18444492273895866368).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i32.trunc_f64_s", static_cast<double>(type_converter64(18443366373989023744).to_f())),
         std::exception);
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u",
                                        static_cast<double>(type_converter64(0).to_f()))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u",
                                        static_cast<double>(type_converter64(9223372036854775808).to_f()))) ==
         static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u",
                                        static_cast<double>(type_converter64(1).to_f()))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u",
                                        static_cast<double>(type_converter64(9223372036854775809).to_f()))) ==
         static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u",
                                        static_cast<double>(type_converter64(4607182418800017408).to_f()))) ==
         static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u",
                                        static_cast<double>(type_converter64(4607632778762754458).to_f()))) ==
         static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u",
                                        static_cast<double>(type_converter64(4609434218613702656).to_f()))) ==
         static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u",
                                        static_cast<double>(type_converter64(4611235658464650854).to_f()))) ==
         static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u",
                                        static_cast<double>(type_converter64(4611686018427387904).to_f()))) ==
         static_cast<uint32_t>(2));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u",
                                        static_cast<double>(type_converter64(4746794007248502784).to_f()))) ==
         static_cast<uint32_t>(2147483648));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u",
                                        static_cast<double>(type_converter64(4751297606873776128).to_f()))) ==
         static_cast<uint32_t>(4294967295));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u",
                                        static_cast<double>(type_converter64(13829653735729319117).to_f()))) ==
         static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u",
                                        static_cast<double>(type_converter64(13830554455654793215).to_f()))) ==
         static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u",
                                        static_cast<double>(type_converter64(4726483295884279808).to_f()))) ==
         static_cast<uint32_t>(100000000));
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i32.trunc_f64_u", static_cast<double>(type_converter64(4751297606875873280).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i32.trunc_f64_u", static_cast<double>(type_converter64(13830554455654793216).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i32.trunc_f64_u", static_cast<double>(type_converter64(4846369599423283200).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i32.trunc_f64_u", static_cast<double>(type_converter64(5055640609639927018).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i32.trunc_f64_u", static_cast<double>(type_converter64(4890909195324358656).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i32.trunc_f64_u", static_cast<double>(type_converter64(9218868437227405312).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i32.trunc_f64_u", static_cast<double>(type_converter64(18442240474082181120).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i32.trunc_f64_u", static_cast<double>(type_converter64(9221120237041090560).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i32.trunc_f64_u", static_cast<double>(type_converter64(9219994337134247936).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i32.trunc_f64_u", static_cast<double>(type_converter64(18444492273895866368).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i32.trunc_f64_u", static_cast<double>(type_converter64(18443366373989023744).to_f())),
         std::exception);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s",
                                        static_cast<float>(type_converter32(0).to_f()))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s",
                                        static_cast<float>(type_converter32(2147483648).to_f()))) ==
         static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s",
                                        static_cast<float>(type_converter32(1).to_f()))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s",
                                        static_cast<float>(type_converter32(2147483649).to_f()))) ==
         static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s",
                                        static_cast<float>(type_converter32(1065353216).to_f()))) ==
         static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s",
                                        static_cast<float>(type_converter32(1066192077).to_f()))) ==
         static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s",
                                        static_cast<float>(type_converter32(1069547520).to_f()))) ==
         static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s",
                                        static_cast<float>(type_converter32(3212836864).to_f()))) ==
         static_cast<uint64_t>(18446744073709551615));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s",
                                        static_cast<float>(type_converter32(3213675725).to_f()))) ==
         static_cast<uint64_t>(18446744073709551615));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s",
                                        static_cast<float>(type_converter32(3217031168).to_f()))) ==
         static_cast<uint64_t>(18446744073709551615));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s",
                                        static_cast<float>(type_converter32(3220386611).to_f()))) ==
         static_cast<uint64_t>(18446744073709551615));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s",
                                        static_cast<float>(type_converter32(3221225472).to_f()))) ==
         static_cast<uint64_t>(18446744073709551614));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s",
                                        static_cast<float>(type_converter32(1333788672).to_f()))) ==
         static_cast<uint64_t>(4294967296));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s",
                                        static_cast<float>(type_converter32(3481272320).to_f()))) ==
         static_cast<uint64_t>(18446744069414584320));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s",
                                        static_cast<float>(type_converter32(1593835519).to_f()))) ==
         static_cast<uint64_t>(9223371487098961920));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s",
                                        static_cast<float>(type_converter32(3741319168).to_f()))) ==
         static_cast<uint64_t>(9223372036854775808));
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.trunc_f32_s", static_cast<float>(type_converter32(1593835520).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.trunc_f32_s", static_cast<float>(type_converter32(3741319169).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.trunc_f32_s", static_cast<float>(type_converter32(2139095040).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.trunc_f32_s", static_cast<float>(type_converter32(4286578688).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.trunc_f32_s", static_cast<float>(type_converter32(2143289344).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.trunc_f32_s", static_cast<float>(type_converter32(2141192192).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.trunc_f32_s", static_cast<float>(type_converter32(4290772992).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.trunc_f32_s", static_cast<float>(type_converter32(4288675840).to_f())),
                   std::exception);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u",
                                        static_cast<float>(type_converter32(0).to_f()))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u",
                                        static_cast<float>(type_converter32(2147483648).to_f()))) ==
         static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u",
                                        static_cast<float>(type_converter32(1).to_f()))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u",
                                        static_cast<float>(type_converter32(2147483649).to_f()))) ==
         static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u",
                                        static_cast<float>(type_converter32(1065353216).to_f()))) ==
         static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u",
                                        static_cast<float>(type_converter32(1066192077).to_f()))) ==
         static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u",
                                        static_cast<float>(type_converter32(1069547520).to_f()))) ==
         static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u",
                                        static_cast<float>(type_converter32(1333788672).to_f()))) ==
         static_cast<uint64_t>(4294967296));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u",
                                        static_cast<float>(type_converter32(1602224127).to_f()))) ==
         static_cast<uint64_t>(18446742974197923840));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u",
                                        static_cast<float>(type_converter32(3211159142).to_f()))) ==
         static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u",
                                        static_cast<float>(type_converter32(3212836863).to_f()))) ==
         static_cast<uint64_t>(0));
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.trunc_f32_u", static_cast<float>(type_converter32(1602224128).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.trunc_f32_u", static_cast<float>(type_converter32(3212836864).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.trunc_f32_u", static_cast<float>(type_converter32(2139095040).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.trunc_f32_u", static_cast<float>(type_converter32(4286578688).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.trunc_f32_u", static_cast<float>(type_converter32(2143289344).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.trunc_f32_u", static_cast<float>(type_converter32(2141192192).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.trunc_f32_u", static_cast<float>(type_converter32(4290772992).to_f())),
                   std::exception);
   CHECK_THROWS_AS(bkend(nullptr, "env", "i64.trunc_f32_u", static_cast<float>(type_converter32(4288675840).to_f())),
                   std::exception);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s",
                                        static_cast<double>(type_converter64(0).to_f()))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s",
                                        static_cast<double>(type_converter64(9223372036854775808).to_f()))) ==
         static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s",
                                        static_cast<double>(type_converter64(1).to_f()))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s",
                                        static_cast<double>(type_converter64(9223372036854775809).to_f()))) ==
         static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s",
                                        static_cast<double>(type_converter64(4607182418800017408).to_f()))) ==
         static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s",
                                        static_cast<double>(type_converter64(4607632778762754458).to_f()))) ==
         static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s",
                                        static_cast<double>(type_converter64(4609434218613702656).to_f()))) ==
         static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s",
                                        static_cast<double>(type_converter64(13830554455654793216).to_f()))) ==
         static_cast<uint64_t>(18446744073709551615));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s",
                                        static_cast<double>(type_converter64(13831004815617530266).to_f()))) ==
         static_cast<uint64_t>(18446744073709551615));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s",
                                        static_cast<double>(type_converter64(13832806255468478464).to_f()))) ==
         static_cast<uint64_t>(18446744073709551615));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s",
                                        static_cast<double>(type_converter64(13834607695319426662).to_f()))) ==
         static_cast<uint64_t>(18446744073709551615));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s",
                                        static_cast<double>(type_converter64(13835058055282163712).to_f()))) ==
         static_cast<uint64_t>(18446744073709551614));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s",
                                        static_cast<double>(type_converter64(4751297606875873280).to_f()))) ==
         static_cast<uint64_t>(4294967296));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s",
                                        static_cast<double>(type_converter64(13974669643730649088).to_f()))) ==
         static_cast<uint64_t>(18446744069414584320));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s",
                                        static_cast<double>(type_converter64(4890909195324358655).to_f()))) ==
         static_cast<uint64_t>(9223372036854774784));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s",
                                        static_cast<double>(type_converter64(14114281232179134464).to_f()))) ==
         static_cast<uint64_t>(9223372036854775808));
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i64.trunc_f64_s", static_cast<double>(type_converter64(4890909195324358656).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i64.trunc_f64_s", static_cast<double>(type_converter64(14114281232179134465).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i64.trunc_f64_s", static_cast<double>(type_converter64(9218868437227405312).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i64.trunc_f64_s", static_cast<double>(type_converter64(18442240474082181120).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i64.trunc_f64_s", static_cast<double>(type_converter64(9221120237041090560).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i64.trunc_f64_s", static_cast<double>(type_converter64(9219994337134247936).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i64.trunc_f64_s", static_cast<double>(type_converter64(18444492273895866368).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i64.trunc_f64_s", static_cast<double>(type_converter64(18443366373989023744).to_f())),
         std::exception);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u",
                                        static_cast<double>(type_converter64(0).to_f()))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u",
                                        static_cast<double>(type_converter64(9223372036854775808).to_f()))) ==
         static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u",
                                        static_cast<double>(type_converter64(1).to_f()))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u",
                                        static_cast<double>(type_converter64(9223372036854775809).to_f()))) ==
         static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u",
                                        static_cast<double>(type_converter64(4607182418800017408).to_f()))) ==
         static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u",
                                        static_cast<double>(type_converter64(4607632778762754458).to_f()))) ==
         static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u",
                                        static_cast<double>(type_converter64(4609434218613702656).to_f()))) ==
         static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u",
                                        static_cast<double>(type_converter64(4751297606873776128).to_f()))) ==
         static_cast<uint64_t>(4294967295));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u",
                                        static_cast<double>(type_converter64(4751297606875873280).to_f()))) ==
         static_cast<uint64_t>(4294967296));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u",
                                        static_cast<double>(type_converter64(4895412794951729151).to_f()))) ==
         static_cast<uint64_t>(18446744073709549568));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u",
                                        static_cast<double>(type_converter64(13829653735729319117).to_f()))) ==
         static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u",
                                        static_cast<double>(type_converter64(13830554455654793215).to_f()))) ==
         static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u",
                                        static_cast<double>(type_converter64(4726483295884279808).to_f()))) ==
         static_cast<uint64_t>(100000000));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u",
                                        static_cast<double>(type_converter64(4846369599423283200).to_f()))) ==
         static_cast<uint64_t>(10000000000000000));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u",
                                        static_cast<double>(type_converter64(4890909195324358656).to_f()))) ==
         static_cast<uint64_t>(9223372036854775808));
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i64.trunc_f64_u", static_cast<double>(type_converter64(4895412794951729152).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i64.trunc_f64_u", static_cast<double>(type_converter64(13830554455654793216).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i64.trunc_f64_u", static_cast<double>(type_converter64(9218868437227405312).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i64.trunc_f64_u", static_cast<double>(type_converter64(18442240474082181120).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i64.trunc_f64_u", static_cast<double>(type_converter64(9221120237041090560).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i64.trunc_f64_u", static_cast<double>(type_converter64(9219994337134247936).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i64.trunc_f64_u", static_cast<double>(type_converter64(18444492273895866368).to_f())),
         std::exception);
   CHECK_THROWS_AS(
         bkend(nullptr, "env", "i64.trunc_f64_u", static_cast<double>(type_converter64(18443366373989023744).to_f())),
         std::exception);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_s", static_cast<uint32_t>(1))) ==
         static_cast<float>(type_converter32(1065353216).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_s", static_cast<uint32_t>(4294967295))) ==
         static_cast<float>(type_converter32(3212836864).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_s", static_cast<uint32_t>(0))) ==
         static_cast<float>(type_converter32(0).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_s", static_cast<uint32_t>(2147483647))) ==
         static_cast<float>(type_converter32(1325400064).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_s", static_cast<uint32_t>(2147483648))) ==
         static_cast<float>(type_converter32(3472883712).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_s", static_cast<uint32_t>(1234567890))) ==
         static_cast<float>(type_converter32(1318267910).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_s", static_cast<uint32_t>(16777217))) ==
         static_cast<float>(type_converter32(1266679808).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_s", static_cast<uint32_t>(4278190079))) ==
         static_cast<float>(type_converter32(3414163456).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_s", static_cast<uint32_t>(16777219))) ==
         static_cast<float>(type_converter32(1266679810).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_s", static_cast<uint32_t>(4278190077))) ==
         static_cast<float>(type_converter32(3414163458).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s", static_cast<uint64_t>(1))) ==
         static_cast<float>(type_converter32(1065353216).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s",
                                        static_cast<uint64_t>(18446744073709551615))) ==
         static_cast<float>(type_converter32(3212836864).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s", static_cast<uint64_t>(0))) ==
         static_cast<float>(type_converter32(0).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s",
                                        static_cast<uint64_t>(9223372036854775807))) ==
         static_cast<float>(type_converter32(1593835520).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s",
                                        static_cast<uint64_t>(9223372036854775808))) ==
         static_cast<float>(type_converter32(3741319168).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s", static_cast<uint64_t>(314159265358979))) ==
         static_cast<float>(type_converter32(1468980468).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s", static_cast<uint64_t>(16777217))) ==
         static_cast<float>(type_converter32(1266679808).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s",
                                        static_cast<uint64_t>(18446744073692774399))) ==
         static_cast<float>(type_converter32(3414163456).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s", static_cast<uint64_t>(16777219))) ==
         static_cast<float>(type_converter32(1266679810).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s",
                                        static_cast<uint64_t>(18446744073692774397))) ==
         static_cast<float>(type_converter32(3414163458).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s",
                                        static_cast<uint64_t>(9223371212221054977))) ==
         static_cast<float>(type_converter32(1593835519).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s",
                                        static_cast<uint64_t>(9223372311732682753))) ==
         static_cast<float>(type_converter32(3741319167).to_f()));
   CHECK(to_f32(
               *bkend.call_with_return(nullptr, "env", "f32.convert_i64_s", static_cast<uint64_t>(9007199791611905))) ==
         static_cast<float>(type_converter32(1509949441).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s",
                                        static_cast<uint64_t>(18437736873917939711))) ==
         static_cast<float>(type_converter32(3657433089).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_s", static_cast<uint32_t>(1))) ==
         static_cast<double>(type_converter64(4607182418800017408).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_s", static_cast<uint32_t>(4294967295))) ==
         static_cast<double>(type_converter64(13830554455654793216).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_s", static_cast<uint32_t>(0))) ==
         static_cast<double>(type_converter64(0).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_s", static_cast<uint32_t>(2147483647))) ==
         static_cast<double>(type_converter64(4746794007244308480).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_s", static_cast<uint32_t>(2147483648))) ==
         static_cast<double>(type_converter64(13970166044103278592).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_s", static_cast<uint32_t>(987654321))) ==
         static_cast<double>(type_converter64(4741568253304766464).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_s", static_cast<uint64_t>(1))) ==
         static_cast<double>(type_converter64(4607182418800017408).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_s",
                                        static_cast<uint64_t>(18446744073709551615))) ==
         static_cast<double>(type_converter64(13830554455654793216).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_s", static_cast<uint64_t>(0))) ==
         static_cast<double>(type_converter64(0).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_s",
                                        static_cast<uint64_t>(9223372036854775807))) ==
         static_cast<double>(type_converter64(4890909195324358656).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_s",
                                        static_cast<uint64_t>(9223372036854775808))) ==
         static_cast<double>(type_converter64(14114281232179134464).to_f()));
   CHECK(to_f64(
               *bkend.call_with_return(nullptr, "env", "f64.convert_i64_s", static_cast<uint64_t>(4669201609102990))) ==
         static_cast<double>(type_converter64(4841535201405015694).to_f()));
   CHECK(to_f64(
               *bkend.call_with_return(nullptr, "env", "f64.convert_i64_s", static_cast<uint64_t>(9007199254740993))) ==
         static_cast<double>(type_converter64(4845873199050653696).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_s",
                                        static_cast<uint64_t>(18437736874454810623))) ==
         static_cast<double>(type_converter64(14069245235905429504).to_f()));
   CHECK(to_f64(
               *bkend.call_with_return(nullptr, "env", "f64.convert_i64_s", static_cast<uint64_t>(9007199254740995))) ==
         static_cast<double>(type_converter64(4845873199050653698).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_s",
                                        static_cast<uint64_t>(18437736874454810621))) ==
         static_cast<double>(type_converter64(14069245235905429506).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", static_cast<uint32_t>(1))) ==
         static_cast<float>(type_converter32(1065353216).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", static_cast<uint32_t>(0))) ==
         static_cast<float>(type_converter32(0).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", static_cast<uint32_t>(2147483647))) ==
         static_cast<float>(type_converter32(1325400064).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", static_cast<uint32_t>(2147483648))) ==
         static_cast<float>(type_converter32(1325400064).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", static_cast<uint32_t>(305419896))) ==
         static_cast<float>(type_converter32(1301390004).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", static_cast<uint32_t>(4294967295))) ==
         static_cast<float>(type_converter32(1333788672).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", static_cast<uint32_t>(2147483776))) ==
         static_cast<float>(type_converter32(1325400064).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", static_cast<uint32_t>(2147483777))) ==
         static_cast<float>(type_converter32(1325400065).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", static_cast<uint32_t>(2147483778))) ==
         static_cast<float>(type_converter32(1325400065).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", static_cast<uint32_t>(4294966912))) ==
         static_cast<float>(type_converter32(1333788670).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", static_cast<uint32_t>(4294966913))) ==
         static_cast<float>(type_converter32(1333788671).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", static_cast<uint32_t>(4294966914))) ==
         static_cast<float>(type_converter32(1333788671).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", static_cast<uint32_t>(16777217))) ==
         static_cast<float>(type_converter32(1266679808).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", static_cast<uint32_t>(16777219))) ==
         static_cast<float>(type_converter32(1266679810).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_u", static_cast<uint64_t>(1))) ==
         static_cast<float>(type_converter32(1065353216).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_u", static_cast<uint64_t>(0))) ==
         static_cast<float>(type_converter32(0).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_u",
                                        static_cast<uint64_t>(9223372036854775807))) ==
         static_cast<float>(type_converter32(1593835520).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_u",
                                        static_cast<uint64_t>(9223372036854775808))) ==
         static_cast<float>(type_converter32(1593835520).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_u",
                                        static_cast<uint64_t>(18446744073709551615))) ==
         static_cast<float>(type_converter32(1602224128).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_u", static_cast<uint64_t>(16777217))) ==
         static_cast<float>(type_converter32(1266679808).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_u", static_cast<uint64_t>(16777219))) ==
         static_cast<float>(type_converter32(1266679810).to_f()));
   CHECK(to_f32(
               *bkend.call_with_return(nullptr, "env", "f32.convert_i64_u", static_cast<uint64_t>(9007199791611905))) ==
         static_cast<float>(type_converter32(1509949441).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_u",
                                        static_cast<uint64_t>(9223371761976868863))) ==
         static_cast<float>(type_converter32(1593835519).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_u",
                                        static_cast<uint64_t>(9223372586610589697))) ==
         static_cast<float>(type_converter32(1593835521).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_u",
                                        static_cast<uint64_t>(18446742424442109953))) ==
         static_cast<float>(type_converter32(1602224127).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_u", static_cast<uint32_t>(1))) ==
         static_cast<double>(type_converter64(4607182418800017408).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_u", static_cast<uint32_t>(0))) ==
         static_cast<double>(type_converter64(0).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_u", static_cast<uint32_t>(2147483647))) ==
         static_cast<double>(type_converter64(4746794007244308480).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_u", static_cast<uint32_t>(2147483648))) ==
         static_cast<double>(type_converter64(4746794007248502784).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_u", static_cast<uint32_t>(4294967295))) ==
         static_cast<double>(type_converter64(4751297606873776128).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u", static_cast<uint64_t>(1))) ==
         static_cast<double>(type_converter64(4607182418800017408).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u", static_cast<uint64_t>(0))) ==
         static_cast<double>(type_converter64(0).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u",
                                        static_cast<uint64_t>(9223372036854775807))) ==
         static_cast<double>(type_converter64(4890909195324358656).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u",
                                        static_cast<uint64_t>(9223372036854775808))) ==
         static_cast<double>(type_converter64(4890909195324358656).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u",
                                        static_cast<uint64_t>(18446744073709551615))) ==
         static_cast<double>(type_converter64(4895412794951729152).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u",
                                        static_cast<uint64_t>(9223372036854776832))) ==
         static_cast<double>(type_converter64(4890909195324358656).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u",
                                        static_cast<uint64_t>(9223372036854776833))) ==
         static_cast<double>(type_converter64(4890909195324358657).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u",
                                        static_cast<uint64_t>(9223372036854776834))) ==
         static_cast<double>(type_converter64(4890909195324358657).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u",
                                        static_cast<uint64_t>(18446744073709548544))) ==
         static_cast<double>(type_converter64(4895412794951729150).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u",
                                        static_cast<uint64_t>(18446744073709548545))) ==
         static_cast<double>(type_converter64(4895412794951729151).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u",
                                        static_cast<uint64_t>(18446744073709548546))) ==
         static_cast<double>(type_converter64(4895412794951729151).to_f()));
   CHECK(to_f64(
               *bkend.call_with_return(nullptr, "env", "f64.convert_i64_u", static_cast<uint64_t>(9007199254740993))) ==
         static_cast<double>(type_converter64(4845873199050653696).to_f()));
   CHECK(to_f64(
               *bkend.call_with_return(nullptr, "env", "f64.convert_i64_u", static_cast<uint64_t>(9007199254740995))) ==
         static_cast<double>(type_converter64(4845873199050653698).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32",
                                        static_cast<float>(type_converter32(0).to_f()))) ==
         static_cast<double>(type_converter64(0).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32",
                                        static_cast<float>(type_converter32(2147483648).to_f()))) ==
         static_cast<double>(type_converter64(9223372036854775808).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32",
                                        static_cast<float>(type_converter32(1).to_f()))) ==
         static_cast<double>(type_converter64(3936146074321813504).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32",
                                        static_cast<float>(type_converter32(2147483649).to_f()))) ==
         static_cast<double>(type_converter64(13159518111176589312).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32",
                                        static_cast<float>(type_converter32(1065353216).to_f()))) ==
         static_cast<double>(type_converter64(4607182418800017408).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32",
                                        static_cast<float>(type_converter32(3212836864).to_f()))) ==
         static_cast<double>(type_converter64(13830554455654793216).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32",
                                        static_cast<float>(type_converter32(4286578687).to_f()))) ==
         static_cast<double>(type_converter64(14407015207421345792).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32",
                                        static_cast<float>(type_converter32(2139095039).to_f()))) ==
         static_cast<double>(type_converter64(5183643170566569984).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32",
                                        static_cast<float>(type_converter32(67108864).to_f()))) ==
         static_cast<double>(type_converter64(4071254063142928384).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32",
                                        static_cast<float>(type_converter32(2118632255).to_f()))) ==
         static_cast<double>(type_converter64(5172657297058430976).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32",
                                        static_cast<float>(type_converter32(2139095040).to_f()))) ==
         static_cast<double>(type_converter64(9218868437227405312).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32",
                                        static_cast<float>(type_converter32(4286578688).to_f()))) ==
         static_cast<double>(type_converter64(18442240474082181120).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(0).to_f()))) ==
         static_cast<float>(type_converter32(0).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(9223372036854775808).to_f()))) ==
         static_cast<float>(type_converter32(2147483648).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(1).to_f()))) ==
         static_cast<float>(type_converter32(0).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(9223372036854775809).to_f()))) ==
         static_cast<float>(type_converter32(2147483648).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(4607182418800017408).to_f()))) ==
         static_cast<float>(type_converter32(1065353216).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(13830554455654793216).to_f()))) ==
         static_cast<float>(type_converter32(3212836864).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(4039728865214464000).to_f()))) ==
         static_cast<float>(type_converter32(8388608).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(13263100902069239808).to_f()))) ==
         static_cast<float>(type_converter32(2155872256).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(4039728865214463999).to_f()))) ==
         static_cast<float>(type_converter32(8388607).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(13263100902069239807).to_f()))) ==
         static_cast<float>(type_converter32(2155872255).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(3936146074321813504).to_f()))) ==
         static_cast<float>(type_converter32(1).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(13159518111176589312).to_f()))) ==
         static_cast<float>(type_converter32(2147483649).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(5183643170298134528).to_f()))) ==
         static_cast<float>(type_converter32(2139095038).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(14407015207152910336).to_f()))) ==
         static_cast<float>(type_converter32(4286578686).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(5183643170298134529).to_f()))) ==
         static_cast<float>(type_converter32(2139095039).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(14407015207152910337).to_f()))) ==
         static_cast<float>(type_converter32(4286578687).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(5183643170566569984).to_f()))) ==
         static_cast<float>(type_converter32(2139095039).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(14407015207421345792).to_f()))) ==
         static_cast<float>(type_converter32(4286578687).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(5183643170835005439).to_f()))) ==
         static_cast<float>(type_converter32(2139095039).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(14407015207689781247).to_f()))) ==
         static_cast<float>(type_converter32(4286578687).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(5183643170835005440).to_f()))) ==
         static_cast<float>(type_converter32(2139095040).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(14407015207689781248).to_f()))) ==
         static_cast<float>(type_converter32(4286578688).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(4071254063142928384).to_f()))) ==
         static_cast<float>(type_converter32(67108864).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(5172657297058430976).to_f()))) ==
         static_cast<float>(type_converter32(2118632255).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(9218868437227405312).to_f()))) ==
         static_cast<float>(type_converter32(2139095040).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(18442240474082181120).to_f()))) ==
         static_cast<float>(type_converter32(4286578688).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(4607182418800017409).to_f()))) ==
         static_cast<float>(type_converter32(1065353216).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(4607182418800017407).to_f()))) ==
         static_cast<float>(type_converter32(1065353216).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(4607182419068452864).to_f()))) ==
         static_cast<float>(type_converter32(1065353216).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(4607182419068452865).to_f()))) ==
         static_cast<float>(type_converter32(1065353217).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(4607182419605323775).to_f()))) ==
         static_cast<float>(type_converter32(1065353217).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(4607182419605323776).to_f()))) ==
         static_cast<float>(type_converter32(1065353218).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(4607182420142194688).to_f()))) ==
         static_cast<float>(type_converter32(1065353218).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(4715268810125344768).to_f()))) ==
         static_cast<float>(type_converter32(1266679808).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(4715268810125344769).to_f()))) ==
         static_cast<float>(type_converter32(1266679809).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(4715268810662215679).to_f()))) ==
         static_cast<float>(type_converter32(1266679809).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(4715268810662215680).to_f()))) ==
         static_cast<float>(type_converter32(1266679810).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(5094955347580439664).to_f()))) ==
         static_cast<float>(type_converter32(1973901096).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(4101111194527827589).to_f()))) ==
         static_cast<float>(type_converter32(122722105).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(4038806939559600639).to_f()))) ==
         static_cast<float>(type_converter32(7529997).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(13836913116900734306).to_f()))) ==
         static_cast<float>(type_converter32(3224680794).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(14338315240173327556).to_f()))) ==
         static_cast<float>(type_converter32(4158615026).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(4503599627370496).to_f()))) ==
         static_cast<float>(type_converter32(0).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(9227875636482146304).to_f()))) ==
         static_cast<float>(type_converter32(2147483648).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(3931642474694443008).to_f()))) ==
         static_cast<float>(type_converter32(0).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(13155014511549218816).to_f()))) ==
         static_cast<float>(type_converter32(2147483648).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(3931642474694443009).to_f()))) ==
         static_cast<float>(type_converter32(1).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.demote_f64",
                                        static_cast<double>(type_converter64(13155014511549218817).to_f()))) ==
         static_cast<float>(type_converter32(2147483649).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.reinterpret_i32", static_cast<uint32_t>(0))) ==
         static_cast<float>(type_converter32(0).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.reinterpret_i32", static_cast<uint32_t>(2147483648))) ==
         static_cast<float>(type_converter32(2147483648).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.reinterpret_i32", static_cast<uint32_t>(1))) ==
         static_cast<float>(type_converter32(1).to_f()));
   CHECK(isnan(
         to_f32(*bkend.call_with_return(nullptr, "env", "f32.reinterpret_i32", static_cast<uint32_t>(4294967295)))));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.reinterpret_i32", static_cast<uint32_t>(123456789))) ==
         static_cast<float>(type_converter32(123456789).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.reinterpret_i32", static_cast<uint32_t>(2147483649))) ==
         static_cast<float>(type_converter32(2147483649).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.reinterpret_i32", static_cast<uint32_t>(2139095040))) ==
         static_cast<float>(type_converter32(2139095040).to_f()));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.reinterpret_i32", static_cast<uint32_t>(4286578688))) ==
         static_cast<float>(type_converter32(4286578688).to_f()));
   CHECK(isnan(
         to_f32(*bkend.call_with_return(nullptr, "env", "f32.reinterpret_i32", static_cast<uint32_t>(2143289344)))));
   CHECK(isnan(
         to_f32(*bkend.call_with_return(nullptr, "env", "f32.reinterpret_i32", static_cast<uint32_t>(4290772992)))));
   CHECK(isnan(
         to_f32(*bkend.call_with_return(nullptr, "env", "f32.reinterpret_i32", static_cast<uint32_t>(2141192192)))));
   CHECK(isnan(
         to_f32(*bkend.call_with_return(nullptr, "env", "f32.reinterpret_i32", static_cast<uint32_t>(4288675840)))));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.reinterpret_i64", static_cast<uint64_t>(0))) ==
         static_cast<double>(type_converter64(0).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.reinterpret_i64", static_cast<uint64_t>(1))) ==
         static_cast<double>(type_converter64(1).to_f()));
   CHECK(isnan(to_f64(
         *bkend.call_with_return(nullptr, "env", "f64.reinterpret_i64", static_cast<uint64_t>(18446744073709551615)))));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.reinterpret_i64",
                                        static_cast<uint64_t>(9223372036854775808))) ==
         static_cast<double>(type_converter64(9223372036854775808).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.reinterpret_i64", static_cast<uint64_t>(1234567890))) ==
         static_cast<double>(type_converter64(1234567890).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.reinterpret_i64",
                                        static_cast<uint64_t>(9223372036854775809))) ==
         static_cast<double>(type_converter64(9223372036854775809).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.reinterpret_i64",
                                        static_cast<uint64_t>(9218868437227405312))) ==
         static_cast<double>(type_converter64(9218868437227405312).to_f()));
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.reinterpret_i64",
                                        static_cast<uint64_t>(18442240474082181120))) ==
         static_cast<double>(type_converter64(18442240474082181120).to_f()));
   CHECK(isnan(to_f64(
         *bkend.call_with_return(nullptr, "env", "f64.reinterpret_i64", static_cast<uint64_t>(9221120237041090560)))));
   CHECK(isnan(to_f64(
         *bkend.call_with_return(nullptr, "env", "f64.reinterpret_i64", static_cast<uint64_t>(18444492273895866368)))));
   CHECK(isnan(to_f64(
         *bkend.call_with_return(nullptr, "env", "f64.reinterpret_i64", static_cast<uint64_t>(9219994337134247936)))));
   CHECK(isnan(to_f64(
         *bkend.call_with_return(nullptr, "env", "f64.reinterpret_i64", static_cast<uint64_t>(18443366373989023744)))));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.reinterpret_f32",
                                        static_cast<float>(type_converter32(0).to_f()))) == static_cast<uint32_t>(0));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.reinterpret_f32",
                                        static_cast<float>(type_converter32(2147483648).to_f()))) ==
         static_cast<uint32_t>(2147483648));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.reinterpret_f32",
                                        static_cast<float>(type_converter32(1).to_f()))) == static_cast<uint32_t>(1));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.reinterpret_f32",
                                        static_cast<float>(type_converter32(4294967295).to_f()))) ==
         static_cast<uint32_t>(4294967295));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.reinterpret_f32",
                                        static_cast<float>(type_converter32(2147483649).to_f()))) ==
         static_cast<uint32_t>(2147483649));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.reinterpret_f32",
                                        static_cast<float>(type_converter32(1065353216).to_f()))) ==
         static_cast<uint32_t>(1065353216));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.reinterpret_f32",
                                        static_cast<float>(type_converter32(1078530010).to_f()))) ==
         static_cast<uint32_t>(1078530010));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.reinterpret_f32",
                                        static_cast<float>(type_converter32(2139095039).to_f()))) ==
         static_cast<uint32_t>(2139095039));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.reinterpret_f32",
                                        static_cast<float>(type_converter32(4286578687).to_f()))) ==
         static_cast<uint32_t>(4286578687));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.reinterpret_f32",
                                        static_cast<float>(type_converter32(2139095040).to_f()))) ==
         static_cast<uint32_t>(2139095040));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.reinterpret_f32",
                                        static_cast<float>(type_converter32(4286578688).to_f()))) ==
         static_cast<uint32_t>(4286578688));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.reinterpret_f32",
                                        static_cast<float>(type_converter32(2143289344).to_f()))) ==
         static_cast<uint32_t>(2143289344));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.reinterpret_f32",
                                        static_cast<float>(type_converter32(4290772992).to_f()))) ==
         static_cast<uint32_t>(4290772992));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.reinterpret_f32",
                                        static_cast<float>(type_converter32(2141192192).to_f()))) ==
         static_cast<uint32_t>(2141192192));
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32.reinterpret_f32",
                                        static_cast<float>(type_converter32(4288675840).to_f()))) ==
         static_cast<uint32_t>(4288675840));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.reinterpret_f64",
                                        static_cast<double>(type_converter64(0).to_f()))) == static_cast<uint64_t>(0));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.reinterpret_f64",
                                        static_cast<double>(type_converter64(9223372036854775808).to_f()))) ==
         static_cast<uint64_t>(9223372036854775808));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.reinterpret_f64",
                                        static_cast<double>(type_converter64(1).to_f()))) == static_cast<uint64_t>(1));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.reinterpret_f64",
                                        static_cast<double>(type_converter64(18446744073709551615).to_f()))) ==
         static_cast<uint64_t>(18446744073709551615));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.reinterpret_f64",
                                        static_cast<double>(type_converter64(9223372036854775809).to_f()))) ==
         static_cast<uint64_t>(9223372036854775809));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.reinterpret_f64",
                                        static_cast<double>(type_converter64(4607182418800017408).to_f()))) ==
         static_cast<uint64_t>(4607182418800017408));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.reinterpret_f64",
                                        static_cast<double>(type_converter64(4614256656552045841).to_f()))) ==
         static_cast<uint64_t>(4614256656552045841));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.reinterpret_f64",
                                        static_cast<double>(type_converter64(9218868437227405311).to_f()))) ==
         static_cast<uint64_t>(9218868437227405311));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.reinterpret_f64",
                                        static_cast<double>(type_converter64(18442240474082181119).to_f()))) ==
         static_cast<uint64_t>(18442240474082181119));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.reinterpret_f64",
                                        static_cast<double>(type_converter64(9218868437227405312).to_f()))) ==
         static_cast<uint64_t>(9218868437227405312));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.reinterpret_f64",
                                        static_cast<double>(type_converter64(18442240474082181120).to_f()))) ==
         static_cast<uint64_t>(18442240474082181120));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.reinterpret_f64",
                                        static_cast<double>(type_converter64(9221120237041090560).to_f()))) ==
         static_cast<uint64_t>(9221120237041090560));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.reinterpret_f64",
                                        static_cast<double>(type_converter64(18444492273895866368).to_f()))) ==
         static_cast<uint64_t>(18444492273895866368));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.reinterpret_f64",
                                        static_cast<double>(type_converter64(9219994337134247936).to_f()))) ==
         static_cast<uint64_t>(9219994337134247936));
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.reinterpret_f64",
                                        static_cast<double>(type_converter64(18443366373989023744).to_f()))) ==
         static_cast<uint64_t>(18443366373989023744));
}
