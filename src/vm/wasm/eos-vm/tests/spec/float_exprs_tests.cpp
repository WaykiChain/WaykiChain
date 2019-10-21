#include <algorithm>
#include <vector>
#include <iostream>
#include <iterator>
#include <cmath>
#include <cstdlib>
#include <catch2/catch.hpp>
#include <utils.hpp>
#include <wasm_config.hpp>
#include <eosio/vm/backend.hpp>

using namespace eosio;
using namespace eosio::vm;
extern wasm_allocator wa;

BACKEND_TEST_CASE( "Testing wasm <float_exprs_0_wasm>", "[float_exprs_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_contraction", bit_cast<double>(UINT64_C(13369472591878845359)), bit_cast<double>(UINT64_C(7598224971858294334)), bit_cast<double>(UINT64_C(7009968021366006149)))->to_f64()) == UINT64_C(16360919150252594323));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_contraction", bit_cast<double>(UINT64_C(4845207016438394692)), bit_cast<double>(UINT64_C(3163224970157846858)), bit_cast<double>(UINT64_C(3251145870828527841)))->to_f64()) == UINT64_C(3401457070760597396));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_contraction", bit_cast<double>(UINT64_C(11159707324127586240)), bit_cast<double>(UINT64_C(7011538096610110295)), bit_cast<double>(UINT64_C(4140382893275160737)))->to_f64()) == UINT64_C(13564076370790560102));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_contraction", bit_cast<double>(UINT64_C(4300281701552927458)), bit_cast<double>(UINT64_C(13379479906516703876)), bit_cast<double>(UINT64_C(3629658278272971302)))->to_f64()) == UINT64_C(13072631228492738408));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_contraction", bit_cast<double>(UINT64_C(9554523352352050493)), bit_cast<double>(UINT64_C(18042841594766434431)), bit_cast<double>(UINT64_C(4368037109959396445)))->to_f64()) == UINT64_C(4544162191519938727));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_1_wasm>", "[float_exprs_1_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.1.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fma", bit_cast<float>(UINT32_C(2111029761)), bit_cast<float>(UINT32_C(879215268)), bit_cast<float>(UINT32_C(1967953261)))->to_f32()) == UINT32_C(1968345878));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fma", bit_cast<float>(UINT32_C(838240978)), bit_cast<float>(UINT32_C(2796592697)), bit_cast<float>(UINT32_C(329493464)))->to_f32()) == UINT32_C(2569667420));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fma", bit_cast<float>(UINT32_C(1381446097)), bit_cast<float>(UINT32_C(962187981)), bit_cast<float>(UINT32_C(1155576972)))->to_f32()) == UINT32_C(1278680110));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fma", bit_cast<float>(UINT32_C(999635965)), bit_cast<float>(UINT32_C(3403528619)), bit_cast<float>(UINT32_C(3222888213)))->to_f32()) == UINT32_C(3338748778));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fma", bit_cast<float>(UINT32_C(2123679707)), bit_cast<float>(UINT32_C(2625733638)), bit_cast<float>(UINT32_C(3500197619)))->to_f32()) == UINT32_C(3684076259));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fma", bit_cast<double>(UINT64_C(7118716943724900052)), bit_cast<double>(UINT64_C(6546073043412611735)), bit_cast<double>(UINT64_C(18275705786238687882)))->to_f64()) == UINT64_C(9054581441422375136));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fma", bit_cast<double>(UINT64_C(7984371788751700236)), bit_cast<double>(UINT64_C(4021745400549737956)), bit_cast<double>(UINT64_C(7188568268293775252)))->to_f64()) == UINT64_C(7398962198428541884));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fma", bit_cast<double>(UINT64_C(1362668175782178275)), bit_cast<double>(UINT64_C(18385570095786966502)), bit_cast<double>(UINT64_C(5677031731722859914)))->to_f64()) == UINT64_C(15141616602947129037));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fma", bit_cast<double>(UINT64_C(12093403956019835987)), bit_cast<double>(UINT64_C(15826077508588652458)), bit_cast<double>(UINT64_C(4856562394320338043)))->to_f64()) == UINT64_C(4867219230351674394));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fma", bit_cast<double>(UINT64_C(4843589256781277081)), bit_cast<double>(UINT64_C(7695653093478086834)), bit_cast<double>(UINT64_C(16938438850771988744)))->to_f64()) == UINT64_C(7932313162666085329));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_10_wasm>", "[float_exprs_10_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.10.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_neg0_sub", bit_cast<float>(UINT32_C(2141192192)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_neg0_sub", bit_cast<double>(UINT64_C(9219994337134247936)))));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_11_wasm>", "[float_exprs_11_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.11.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_neg1_mul", bit_cast<float>(UINT32_C(2141192192)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_neg1_mul", bit_cast<double>(UINT64_C(9219994337134247936)))));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_12_wasm>", "[float_exprs_12_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.12.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "f32.no_fold_eq_self", bit_cast<float>(UINT32_C(2143289344)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.no_fold_eq_self", bit_cast<double>(UINT64_C(9221120237041090560)))->to_ui32() == UINT32_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_13_wasm>", "[float_exprs_13_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.13.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "f32.no_fold_ne_self", bit_cast<float>(UINT32_C(2143289344)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.no_fold_ne_self", bit_cast<double>(UINT64_C(9221120237041090560)))->to_ui32() == UINT32_C(1));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_14_wasm>", "[float_exprs_14_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.14.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_sub_self", bit_cast<float>(UINT32_C(2139095040)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_sub_self", bit_cast<float>(UINT32_C(2143289344)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_sub_self", bit_cast<double>(UINT64_C(9218868437227405312)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_sub_self", bit_cast<double>(UINT64_C(9221120237041090560)))));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_15_wasm>", "[float_exprs_15_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.15.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_self", bit_cast<float>(UINT32_C(2139095040)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_self", bit_cast<float>(UINT32_C(2143289344)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_self", bit_cast<float>(UINT32_C(0)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_self", bit_cast<float>(UINT32_C(2147483648)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_self", bit_cast<double>(UINT64_C(9218868437227405312)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_self", bit_cast<double>(UINT64_C(9221120237041090560)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_self", bit_cast<double>(UINT64_C(0)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_self", bit_cast<double>(UINT64_C(9223372036854775808)))));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_16_wasm>", "[float_exprs_16_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.16.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_3", bit_cast<float>(UINT32_C(3634023955)))->to_f32()) == UINT32_C(3620628505));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_3", bit_cast<float>(UINT32_C(4000459555)))->to_f32()) == UINT32_C(3986780695));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_3", bit_cast<float>(UINT32_C(2517965963)))->to_f32()) == UINT32_C(2504446137));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_3", bit_cast<float>(UINT32_C(2173683100)))->to_f32()) == UINT32_C(2160046629));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_3", bit_cast<float>(UINT32_C(2750097330)))->to_f32()) == UINT32_C(2736571681));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_3", bit_cast<double>(UINT64_C(16679796490173820099)))->to_f64()) == UINT64_C(16672802667330368301));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_3", bit_cast<double>(UINT64_C(13081777497422760306)))->to_f64()) == UINT64_C(13074664638073319671));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_3", bit_cast<double>(UINT64_C(674365394458900388)))->to_f64()) == UINT64_C(667250911628840899));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_3", bit_cast<double>(UINT64_C(18365700772251870524)))->to_f64()) == UINT64_C(18358201936817915643));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_3", bit_cast<double>(UINT64_C(6476267216527259981)))->to_f64()) == UINT64_C(6468791534604471399));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_17_wasm>", "[float_exprs_17_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.17.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_factor", bit_cast<float>(UINT32_C(3550941609)), bit_cast<float>(UINT32_C(3628209942)), bit_cast<float>(UINT32_C(1568101121)))->to_f32()) == UINT32_C(4131116008));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_factor", bit_cast<float>(UINT32_C(3168433147)), bit_cast<float>(UINT32_C(1028017286)), bit_cast<float>(UINT32_C(3141035521)))->to_f32()) == UINT32_C(3095417249));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_factor", bit_cast<float>(UINT32_C(2869115159)), bit_cast<float>(UINT32_C(536308199)), bit_cast<float>(UINT32_C(2100177580)))->to_f32()) == UINT32_C(3904015703));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_factor", bit_cast<float>(UINT32_C(2684117842)), bit_cast<float>(UINT32_C(369386499)), bit_cast<float>(UINT32_C(2061166438)))->to_f32()) == UINT32_C(3679965352));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_factor", bit_cast<float>(UINT32_C(2510116111)), bit_cast<float>(UINT32_C(476277495)), bit_cast<float>(UINT32_C(1237750930)))->to_f32()) == UINT32_C(649094375));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_factor", bit_cast<double>(UINT64_C(2698691837980592503)), bit_cast<double>(UINT64_C(2529920934327896545)), bit_cast<double>(UINT64_C(12819783413251458936)))->to_f64()) == UINT64_C(10911876679403600666));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_factor", bit_cast<double>(UINT64_C(1626864102540432200)), bit_cast<double>(UINT64_C(9287829620889669687)), bit_cast<double>(UINT64_C(9524500187773169472)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_factor", bit_cast<double>(UINT64_C(12326480769054961745)), bit_cast<double>(UINT64_C(12563546453737163926)), bit_cast<double>(UINT64_C(15990519985875741037)))->to_f64()) == UINT64_C(5500432744005058080));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_factor", bit_cast<double>(UINT64_C(12532477544855171977)), bit_cast<double>(UINT64_C(3439526350000314825)), bit_cast<double>(UINT64_C(12694541248380731909)))->to_f64()) == UINT64_C(11527035460272583044));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_factor", bit_cast<double>(UINT64_C(1871759566187673434)), bit_cast<double>(UINT64_C(2002968319587025494)), bit_cast<double>(UINT64_C(16033202089880281080)))->to_f64()) == UINT64_C(13429277897969282899));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_18_wasm>", "[float_exprs_18_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.18.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_distribute", bit_cast<float>(UINT32_C(3550941609)), bit_cast<float>(UINT32_C(3628209942)), bit_cast<float>(UINT32_C(1568101121)))->to_f32()) == UINT32_C(4131116009));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_distribute", bit_cast<float>(UINT32_C(3168433147)), bit_cast<float>(UINT32_C(1028017286)), bit_cast<float>(UINT32_C(3141035521)))->to_f32()) == UINT32_C(3095417248));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_distribute", bit_cast<float>(UINT32_C(2869115159)), bit_cast<float>(UINT32_C(536308199)), bit_cast<float>(UINT32_C(2100177580)))->to_f32()) == UINT32_C(3904015704));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_distribute", bit_cast<float>(UINT32_C(2684117842)), bit_cast<float>(UINT32_C(369386499)), bit_cast<float>(UINT32_C(2061166438)))->to_f32()) == UINT32_C(3679965351));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_distribute", bit_cast<float>(UINT32_C(2510116111)), bit_cast<float>(UINT32_C(476277495)), bit_cast<float>(UINT32_C(1237750930)))->to_f32()) == UINT32_C(649094374));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_distribute", bit_cast<double>(UINT64_C(2698691837980592503)), bit_cast<double>(UINT64_C(2529920934327896545)), bit_cast<double>(UINT64_C(12819783413251458936)))->to_f64()) == UINT64_C(10911876679403600667));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_distribute", bit_cast<double>(UINT64_C(1626864102540432200)), bit_cast<double>(UINT64_C(9287829620889669687)), bit_cast<double>(UINT64_C(9524500187773169472)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_distribute", bit_cast<double>(UINT64_C(12326480769054961745)), bit_cast<double>(UINT64_C(12563546453737163926)), bit_cast<double>(UINT64_C(15990519985875741037)))->to_f64()) == UINT64_C(5500432744005058079));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_distribute", bit_cast<double>(UINT64_C(12532477544855171977)), bit_cast<double>(UINT64_C(3439526350000314825)), bit_cast<double>(UINT64_C(12694541248380731909)))->to_f64()) == UINT64_C(11527035460272583043));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_distribute", bit_cast<double>(UINT64_C(1871759566187673434)), bit_cast<double>(UINT64_C(2002968319587025494)), bit_cast<double>(UINT64_C(16033202089880281080)))->to_f64()) == UINT64_C(13429277897969282898));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_19_wasm>", "[float_exprs_19_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.19.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_regroup_div_mul", bit_cast<float>(UINT32_C(2249624147)), bit_cast<float>(UINT32_C(2678828342)), bit_cast<float>(UINT32_C(95319815)))->to_f32()) == UINT32_C(538190437));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_regroup_div_mul", bit_cast<float>(UINT32_C(3978470300)), bit_cast<float>(UINT32_C(2253997363)), bit_cast<float>(UINT32_C(3824852100)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_regroup_div_mul", bit_cast<float>(UINT32_C(3350590135)), bit_cast<float>(UINT32_C(3042588643)), bit_cast<float>(UINT32_C(2186448635)))->to_f32()) == UINT32_C(4206661932));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_regroup_div_mul", bit_cast<float>(UINT32_C(2430706172)), bit_cast<float>(UINT32_C(1685220483)), bit_cast<float>(UINT32_C(1642018044)))->to_f32()) == UINT32_C(2473922297));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_regroup_div_mul", bit_cast<float>(UINT32_C(2011387707)), bit_cast<float>(UINT32_C(1274956446)), bit_cast<float>(UINT32_C(3811596788)))->to_f32()) == UINT32_C(3768838261));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_regroup_div_mul", bit_cast<double>(UINT64_C(2703215631877943472)), bit_cast<double>(UINT64_C(13295603997208052007)), bit_cast<double>(UINT64_C(1719211436532588593)))->to_f64()) == UINT64_C(14279677686886620461));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_regroup_div_mul", bit_cast<double>(UINT64_C(6126139291059848917)), bit_cast<double>(UINT64_C(2596039250849921421)), bit_cast<double>(UINT64_C(17423258659719899654)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_regroup_div_mul", bit_cast<double>(UINT64_C(2451868557331674239)), bit_cast<double>(UINT64_C(8672326445062988097)), bit_cast<double>(UINT64_C(2593279393835739385)))->to_f64()) == UINT64_C(9218868437227405312));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_regroup_div_mul", bit_cast<double>(UINT64_C(15994259208199847538)), bit_cast<double>(UINT64_C(16584156163346075677)), bit_cast<double>(UINT64_C(17596923907238870430)))->to_f64()) == UINT64_C(14981548491626301009));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_regroup_div_mul", bit_cast<double>(UINT64_C(1912002771029783751)), bit_cast<double>(UINT64_C(655387110450354003)), bit_cast<double>(UINT64_C(10060746190138762841)))->to_f64()) == UINT64_C(10953754119023888080));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_2_wasm>", "[float_exprs_2_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.2.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_zero", bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_zero", bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(0));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_zero", bit_cast<float>(UINT32_C(2141192192)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_zero", bit_cast<double>(UINT64_C(9219994337134247936)))));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_20_wasm>", "[float_exprs_20_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.20.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_regroup_mul_div", bit_cast<float>(UINT32_C(2249624147)), bit_cast<float>(UINT32_C(2678828342)), bit_cast<float>(UINT32_C(95319815)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_regroup_mul_div", bit_cast<float>(UINT32_C(3978470300)), bit_cast<float>(UINT32_C(2253997363)), bit_cast<float>(UINT32_C(3824852100)))->to_f32()) == UINT32_C(2408382580));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_regroup_mul_div", bit_cast<float>(UINT32_C(3350590135)), bit_cast<float>(UINT32_C(3042588643)), bit_cast<float>(UINT32_C(2186448635)))->to_f32()) == UINT32_C(4206661933));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_regroup_mul_div", bit_cast<float>(UINT32_C(2430706172)), bit_cast<float>(UINT32_C(1685220483)), bit_cast<float>(UINT32_C(1642018044)))->to_f32()) == UINT32_C(2473922298));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_regroup_mul_div", bit_cast<float>(UINT32_C(2011387707)), bit_cast<float>(UINT32_C(1274956446)), bit_cast<float>(UINT32_C(3811596788)))->to_f32()) == UINT32_C(4286578688));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_regroup_mul_div", bit_cast<double>(UINT64_C(2703215631877943472)), bit_cast<double>(UINT64_C(13295603997208052007)), bit_cast<double>(UINT64_C(1719211436532588593)))->to_f64()) == UINT64_C(14279677686886620462));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_regroup_mul_div", bit_cast<double>(UINT64_C(6126139291059848917)), bit_cast<double>(UINT64_C(2596039250849921421)), bit_cast<double>(UINT64_C(17423258659719899654)))->to_f64()) == UINT64_C(9746029336072872080));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_regroup_mul_div", bit_cast<double>(UINT64_C(2451868557331674239)), bit_cast<double>(UINT64_C(8672326445062988097)), bit_cast<double>(UINT64_C(2593279393835739385)))->to_f64()) == UINT64_C(8531093589128288889));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_regroup_mul_div", bit_cast<double>(UINT64_C(15994259208199847538)), bit_cast<double>(UINT64_C(16584156163346075677)), bit_cast<double>(UINT64_C(17596923907238870430)))->to_f64()) == UINT64_C(18442240474082181120));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_regroup_mul_div", bit_cast<double>(UINT64_C(1912002771029783751)), bit_cast<double>(UINT64_C(655387110450354003)), bit_cast<double>(UINT64_C(10060746190138762841)))->to_f64()) == UINT64_C(9223372036854775808));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_21_wasm>", "[float_exprs_21_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.21.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_reassociate_add", bit_cast<float>(UINT32_C(3585064686)), bit_cast<float>(UINT32_C(1354934024)), bit_cast<float>(UINT32_C(3612934982)), bit_cast<float>(UINT32_C(3557837641)))->to_f32()) == UINT32_C(3614520891));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_reassociate_add", bit_cast<float>(UINT32_C(997006780)), bit_cast<float>(UINT32_C(3156314493)), bit_cast<float>(UINT32_C(1031916275)), bit_cast<float>(UINT32_C(3157700435)))->to_f32()) == UINT32_C(1027365261));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_reassociate_add", bit_cast<float>(UINT32_C(3506363549)), bit_cast<float>(UINT32_C(3562765939)), bit_cast<float>(UINT32_C(1440782572)), bit_cast<float>(UINT32_C(1388583643)))->to_f32()) == UINT32_C(1439168977));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_reassociate_add", bit_cast<float>(UINT32_C(1460378878)), bit_cast<float>(UINT32_C(1481791683)), bit_cast<float>(UINT32_C(3506843934)), bit_cast<float>(UINT32_C(1493913729)))->to_f32()) == UINT32_C(1497931771));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_reassociate_add", bit_cast<float>(UINT32_C(1975099005)), bit_cast<float>(UINT32_C(4120668550)), bit_cast<float>(UINT32_C(1947708458)), bit_cast<float>(UINT32_C(4008073260)))->to_f32()) == UINT32_C(1958779787));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_reassociate_add", bit_cast<double>(UINT64_C(17619937326421449126)), bit_cast<double>(UINT64_C(8424880666975634327)), bit_cast<double>(UINT64_C(8461713040394112626)), bit_cast<double>(UINT64_C(17692076622886930107)))->to_f64()) == UINT64_C(17689770886425413754));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_reassociate_add", bit_cast<double>(UINT64_C(2161744272815763681)), bit_cast<double>(UINT64_C(2160815018984030177)), bit_cast<double>(UINT64_C(11389452991481170854)), bit_cast<double>(UINT64_C(11158554735757873927)))->to_f64()) == UINT64_C(11367213592018398582));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_reassociate_add", bit_cast<double>(UINT64_C(15816220208145029204)), bit_cast<double>(UINT64_C(6443786499090728432)), bit_cast<double>(UINT64_C(15798639273395365185)), bit_cast<double>(UINT64_C(6395820899158300605)))->to_f64()) == UINT64_C(15816713260997571051));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_reassociate_add", bit_cast<double>(UINT64_C(12406188505172681730)), bit_cast<double>(UINT64_C(3227622722685619614)), bit_cast<double>(UINT64_C(12653209142287077985)), bit_cast<double>(UINT64_C(3439058911346459774)))->to_f64()) == UINT64_C(3437283564188778523));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_reassociate_add", bit_cast<double>(UINT64_C(16720963389015391005)), bit_cast<double>(UINT64_C(16597092572968550980)), bit_cast<double>(UINT64_C(7518944085377596897)), bit_cast<double>(UINT64_C(16733407756820198530)))->to_f64()) == UINT64_C(7516931113564586278));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_22_wasm>", "[float_exprs_22_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.22.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_reassociate_mul", bit_cast<float>(UINT32_C(97158612)), bit_cast<float>(UINT32_C(796388711)), bit_cast<float>(UINT32_C(4071607776)), bit_cast<float>(UINT32_C(603464324)))->to_f32()) == UINT32_C(2373950135));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_reassociate_mul", bit_cast<float>(UINT32_C(598526039)), bit_cast<float>(UINT32_C(4072603010)), bit_cast<float>(UINT32_C(2166864805)), bit_cast<float>(UINT32_C(3802968051)))->to_f32()) == UINT32_C(3152274558));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_reassociate_mul", bit_cast<float>(UINT32_C(666201298)), bit_cast<float>(UINT32_C(3678968917)), bit_cast<float>(UINT32_C(2879732647)), bit_cast<float>(UINT32_C(1703934016)))->to_f32()) == UINT32_C(1439591542));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_reassociate_mul", bit_cast<float>(UINT32_C(191948150)), bit_cast<float>(UINT32_C(1717012201)), bit_cast<float>(UINT32_C(3682645872)), bit_cast<float>(UINT32_C(3713382507)))->to_f32()) == UINT32_C(1814709127));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_reassociate_mul", bit_cast<float>(UINT32_C(2384301792)), bit_cast<float>(UINT32_C(656878874)), bit_cast<float>(UINT32_C(3239861549)), bit_cast<float>(UINT32_C(1564466295)))->to_f32()) == UINT32_C(355327948));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_reassociate_mul", bit_cast<double>(UINT64_C(10830726381612138752)), bit_cast<double>(UINT64_C(18293529276079591087)), bit_cast<double>(UINT64_C(12137662286027993114)), bit_cast<double>(UINT64_C(16821646709291069775)))->to_f64()) == UINT64_C(7368793799369880819));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_reassociate_mul", bit_cast<double>(UINT64_C(6653164799371160764)), bit_cast<double>(UINT64_C(2285295038358358170)), bit_cast<double>(UINT64_C(9783304669150272403)), bit_cast<double>(UINT64_C(16266005085991502709)))->to_f64()) == UINT64_C(2720645287366687760));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_reassociate_mul", bit_cast<double>(UINT64_C(2352911459797566465)), bit_cast<double>(UINT64_C(17379873157362463143)), bit_cast<double>(UINT64_C(1179129869275935356)), bit_cast<double>(UINT64_C(14228398113747850351)))->to_f64()) == UINT64_C(2873103656912958703));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_reassociate_mul", bit_cast<double>(UINT64_C(7724499817746503804)), bit_cast<double>(UINT64_C(2704005046640722176)), bit_cast<double>(UINT64_C(5612860422806321751)), bit_cast<double>(UINT64_C(13727818095548724091)))->to_f64()) == UINT64_C(15948568678460814092));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_reassociate_mul", bit_cast<double>(UINT64_C(3553622953022765407)), bit_cast<double>(UINT64_C(1044040287824900408)), bit_cast<double>(UINT64_C(17112762794520509437)), bit_cast<double>(UINT64_C(11134095486440145773)))->to_f64()) == UINT64_C(576919682754813073));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_23_wasm>", "[float_exprs_23_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.23.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_0", bit_cast<float>(UINT32_C(1065353216)))->to_f32()) == UINT32_C(2139095040));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_0", bit_cast<float>(UINT32_C(3212836864)))->to_f32()) == UINT32_C(4286578688));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_0", bit_cast<float>(UINT32_C(2139095040)))->to_f32()) == UINT32_C(2139095040));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_0", bit_cast<float>(UINT32_C(4286578688)))->to_f32()) == UINT32_C(4286578688));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_0", bit_cast<float>(UINT32_C(0)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_0", bit_cast<float>(UINT32_C(2147483648)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_0", bit_cast<float>(UINT32_C(2141192192)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_0", bit_cast<float>(UINT32_C(2143289344)))));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_0", bit_cast<double>(UINT64_C(4607182418800017408)))->to_f64()) == UINT64_C(9218868437227405312));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_0", bit_cast<double>(UINT64_C(13830554455654793216)))->to_f64()) == UINT64_C(18442240474082181120));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_0", bit_cast<double>(UINT64_C(9218868437227405312)))->to_f64()) == UINT64_C(9218868437227405312));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_0", bit_cast<double>(UINT64_C(18442240474082181120)))->to_f64()) == UINT64_C(18442240474082181120));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_0", bit_cast<double>(UINT64_C(0)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_0", bit_cast<double>(UINT64_C(9223372036854775808)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_0", bit_cast<double>(UINT64_C(9221120237041090560)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_0", bit_cast<double>(UINT64_C(9219994337134247936)))));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_24_wasm>", "[float_exprs_24_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.24.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_neg0", bit_cast<float>(UINT32_C(1065353216)))->to_f32()) == UINT32_C(4286578688));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_neg0", bit_cast<float>(UINT32_C(3212836864)))->to_f32()) == UINT32_C(2139095040));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_neg0", bit_cast<float>(UINT32_C(2139095040)))->to_f32()) == UINT32_C(4286578688));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_neg0", bit_cast<float>(UINT32_C(4286578688)))->to_f32()) == UINT32_C(2139095040));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_neg0", bit_cast<float>(UINT32_C(0)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_neg0", bit_cast<float>(UINT32_C(2147483648)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_neg0", bit_cast<float>(UINT32_C(2141192192)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_neg0", bit_cast<float>(UINT32_C(2143289344)))));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_neg0", bit_cast<double>(UINT64_C(4607182418800017408)))->to_f64()) == UINT64_C(18442240474082181120));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_neg0", bit_cast<double>(UINT64_C(13830554455654793216)))->to_f64()) == UINT64_C(9218868437227405312));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_neg0", bit_cast<double>(UINT64_C(9218868437227405312)))->to_f64()) == UINT64_C(18442240474082181120));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_neg0", bit_cast<double>(UINT64_C(18442240474082181120)))->to_f64()) == UINT64_C(9218868437227405312));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_neg0", bit_cast<double>(UINT64_C(0)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_neg0", bit_cast<double>(UINT64_C(9223372036854775808)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_neg0", bit_cast<double>(UINT64_C(9221120237041090560)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_neg0", bit_cast<double>(UINT64_C(9219994337134247936)))));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_25_wasm>", "[float_exprs_25_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.25.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_to_hypot", bit_cast<float>(UINT32_C(392264092)), bit_cast<float>(UINT32_C(497028527)))->to_f32()) == UINT32_C(497028710));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_to_hypot", bit_cast<float>(UINT32_C(2623653512)), bit_cast<float>(UINT32_C(2317012712)))->to_f32()) == UINT32_C(476165425));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_to_hypot", bit_cast<float>(UINT32_C(2261577829)), bit_cast<float>(UINT32_C(2641790518)))->to_f32()) == UINT32_C(494307108));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_to_hypot", bit_cast<float>(UINT32_C(3255678581)), bit_cast<float>(UINT32_C(1210720351)))->to_f32()) == UINT32_C(1210720352));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_to_hypot", bit_cast<float>(UINT32_C(432505039)), bit_cast<float>(UINT32_C(2618036612)))->to_f32()) == UINT32_C(470544734));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_to_hypot", bit_cast<double>(UINT64_C(1743351192697472785)), bit_cast<double>(UINT64_C(2202602366606243153)))->to_f64()) == UINT64_C(2202599296765198670));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_to_hypot", bit_cast<double>(UINT64_C(6389333765198869657)), bit_cast<double>(UINT64_C(15677343373020056630)))->to_f64()) == UINT64_C(6453971336171062178));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_to_hypot", bit_cast<double>(UINT64_C(2195337108264055819)), bit_cast<double>(UINT64_C(10384237061545402288)))->to_f64()) == UINT64_C(2195504818343116800));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_to_hypot", bit_cast<double>(UINT64_C(11486582223361829725)), bit_cast<double>(UINT64_C(1308532122426122043)))->to_f64()) == UINT64_C(2263210186506929210));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_to_hypot", bit_cast<double>(UINT64_C(1591440107418864392)), bit_cast<double>(UINT64_C(11515806374387309036)))->to_f64()) == UINT64_C(2292434337532533215));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_26_wasm>", "[float_exprs_26_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.26.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_approximate_reciprocal", bit_cast<float>(UINT32_C(3130294363)))->to_f32()) == UINT32_C(3294406762));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_approximate_reciprocal", bit_cast<float>(UINT32_C(2138280080)))->to_f32()) == UINT32_C(2204223));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_approximate_reciprocal", bit_cast<float>(UINT32_C(2434880051)))->to_f32()) == UINT32_C(3989512051));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_approximate_reciprocal", bit_cast<float>(UINT32_C(1705936409)))->to_f32()) == UINT32_C(423346609));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_approximate_reciprocal", bit_cast<float>(UINT32_C(2528120561)))->to_f32()) == UINT32_C(3896123071));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_27_wasm>", "[float_exprs_27_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.27.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_approximate_reciprocal_sqrt", bit_cast<float>(UINT32_C(708147349)))->to_f32()) == UINT32_C(1243088746));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_approximate_reciprocal_sqrt", bit_cast<float>(UINT32_C(1005852643)))->to_f32()) == UINT32_C(1094279611));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_approximate_reciprocal_sqrt", bit_cast<float>(UINT32_C(517799246)))->to_f32()) == UINT32_C(1338168541));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_approximate_reciprocal_sqrt", bit_cast<float>(UINT32_C(704281251)))->to_f32()) == UINT32_C(1245118689));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_approximate_reciprocal_sqrt", bit_cast<float>(UINT32_C(347001813)))->to_f32()) == UINT32_C(1423641701));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fuse_reciprocal_sqrt", bit_cast<double>(UINT64_C(8611259114887405475)))->to_f64()) == UINT64_C(2604695339663988000));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fuse_reciprocal_sqrt", bit_cast<double>(UINT64_C(6008428610859539631)))->to_f64()) == UINT64_C(3906084647186679832));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fuse_reciprocal_sqrt", bit_cast<double>(UINT64_C(5077495674931581012)))->to_f64()) == UINT64_C(4371518865190387497));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fuse_reciprocal_sqrt", bit_cast<double>(UINT64_C(7616219057857077123)))->to_f64()) == UINT64_C(3102407657946187309));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fuse_reciprocal_sqrt", bit_cast<double>(UINT64_C(5267858027841559467)))->to_f64()) == UINT64_C(4276321761661248681));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_28_wasm>", "[float_exprs_28_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.28.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_approximate_sqrt_reciprocal", bit_cast<float>(UINT32_C(1574069443)))->to_f32()) == UINT32_C(810003811));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_approximate_sqrt_reciprocal", bit_cast<float>(UINT32_C(992487567)))->to_f32()) == UINT32_C(1100869283));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_approximate_sqrt_reciprocal", bit_cast<float>(UINT32_C(1644769121)))->to_f32()) == UINT32_C(774822585));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_approximate_sqrt_reciprocal", bit_cast<float>(UINT32_C(1180509736)))->to_f32()) == UINT32_C(1007269771));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_approximate_sqrt_reciprocal", bit_cast<float>(UINT32_C(1940205041)))->to_f32()) == UINT32_C(627137240));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_29_wasm>", "[float_exprs_29_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.29.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "i32.no_fold_f32_s", UINT32_C(16777216))->to_ui32() == UINT32_C(16777216));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.no_fold_f32_s", UINT32_C(16777217))->to_ui32() == UINT32_C(16777216));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.no_fold_f32_s", UINT32_C(4026531856))->to_ui32() == UINT32_C(4026531856));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.no_fold_f32_u", UINT32_C(16777216))->to_ui32() == UINT32_C(16777216));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.no_fold_f32_u", UINT32_C(16777217))->to_ui32() == UINT32_C(16777216));
   CHECK(bkend.call_with_return(nullptr, "env", "i32.no_fold_f32_u", UINT32_C(4026531856))->to_ui32() == UINT32_C(4026531840));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_f64_s", UINT64_C(9007199254740992))->to_ui64() == UINT32_C(9007199254740992));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_f64_s", UINT64_C(9007199254740993))->to_ui64() == UINT32_C(9007199254740992));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_f64_s", UINT64_C(17293822569102705664))->to_ui64() == UINT32_C(17293822569102705664));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_f64_u", UINT64_C(9007199254740992))->to_ui64() == UINT32_C(9007199254740992));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_f64_u", UINT64_C(9007199254740993))->to_ui64() == UINT32_C(9007199254740992));
   CHECK(bkend.call_with_return(nullptr, "env", "i64.no_fold_f64_u", UINT64_C(17293822569102705664))->to_ui64() == UINT32_C(17293822569102704640));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_3_wasm>", "[float_exprs_3_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.3.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_zero_sub", bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_zero_sub", bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_zero_sub", bit_cast<float>(UINT32_C(2141192192)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_zero_sub", bit_cast<double>(UINT64_C(9219994337134247936)))));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_30_wasm>", "[float_exprs_30_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.30.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_sub", bit_cast<float>(UINT32_C(677030386)), bit_cast<float>(UINT32_C(2998136214)))->to_f32()) == UINT32_C(677380096));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_sub", bit_cast<float>(UINT32_C(3025420904)), bit_cast<float>(UINT32_C(913921807)))->to_f32()) == UINT32_C(3025420912));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_sub", bit_cast<float>(UINT32_C(3908960888)), bit_cast<float>(UINT32_C(4063404061)))->to_f32()) == UINT32_C(3909091328));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_sub", bit_cast<float>(UINT32_C(415467473)), bit_cast<float>(UINT32_C(602055819)))->to_f32()) == UINT32_C(415236096));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_sub", bit_cast<float>(UINT32_C(2307650739)), bit_cast<float>(UINT32_C(2511328013)))->to_f32()) == UINT32_C(2315255808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_sub", bit_cast<double>(UINT64_C(9894695622864460712)), bit_cast<double>(UINT64_C(747900745977727688)))->to_f64()) == UINT64_C(9894695622864404480));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_sub", bit_cast<double>(UINT64_C(2152218683357821298)), bit_cast<double>(UINT64_C(2238360073507307376)))->to_f64()) == UINT64_C(2152218683357790208));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_sub", bit_cast<double>(UINT64_C(13697521605206502242)), bit_cast<double>(UINT64_C(13818850255013161909)))->to_f64()) == UINT64_C(13697521605247238144));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_sub", bit_cast<double>(UINT64_C(12298280617237492384)), bit_cast<double>(UINT64_C(3233965342858558382)))->to_f64()) == UINT64_C(12298280617463775232));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_sub", bit_cast<double>(UINT64_C(11043298296128683688)), bit_cast<double>(UINT64_C(11182857345495207592)))->to_f64()) == UINT64_C(11043298296775835648));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_31_wasm>", "[float_exprs_31_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.31.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_sub_add", bit_cast<float>(UINT32_C(3291029084)), bit_cast<float>(UINT32_C(1137280182)))->to_f32()) == UINT32_C(3291029085));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_sub_add", bit_cast<float>(UINT32_C(2287045896)), bit_cast<float>(UINT32_C(272248696)))->to_f32()) == UINT32_C(2287075328));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_sub_add", bit_cast<float>(UINT32_C(1285466516)), bit_cast<float>(UINT32_C(1361849144)))->to_f32()) == UINT32_C(1285466624));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_sub_add", bit_cast<float>(UINT32_C(740009747)), bit_cast<float>(UINT32_C(2989707904)))->to_f32()) == UINT32_C(740007936));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_sub_add", bit_cast<float>(UINT32_C(1041827798)), bit_cast<float>(UINT32_C(3335914317)))->to_f32()) == UINT32_C(1041891328));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_sub_add", bit_cast<double>(UINT64_C(5758126085282503565)), bit_cast<double>(UINT64_C(14997141603873875659)))->to_f64()) == UINT64_C(5758126085282503568));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_sub_add", bit_cast<double>(UINT64_C(1609380455481879691)), bit_cast<double>(UINT64_C(1695875689930159213)))->to_f64()) == UINT64_C(1609380455482130432));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_sub_add", bit_cast<double>(UINT64_C(5738179408840599949)), bit_cast<double>(UINT64_C(15186085143903012996)))->to_f64()) == UINT64_C(5738148875223433216));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_sub_add", bit_cast<double>(UINT64_C(4492841470376833908)), bit_cast<double>(UINT64_C(13773869588765591068)))->to_f64()) == UINT64_C(4492841470376837120));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_sub_add", bit_cast<double>(UINT64_C(2955729038738127538)), bit_cast<double>(UINT64_C(12208627806665035010)))->to_f64()) == UINT64_C(2955729038738127552));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_32_wasm>", "[float_exprs_32_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.32.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_div", bit_cast<float>(UINT32_C(3672556237)), bit_cast<float>(UINT32_C(674649243)))->to_f32()) == UINT32_C(3672556236));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_div", bit_cast<float>(UINT32_C(2995104604)), bit_cast<float>(UINT32_C(178524966)))->to_f32()) == UINT32_C(2995104594));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_div", bit_cast<float>(UINT32_C(2817764014)), bit_cast<float>(UINT32_C(3620253920)))->to_f32()) == UINT32_C(2817764013));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_div", bit_cast<float>(UINT32_C(1507152519)), bit_cast<float>(UINT32_C(3723483599)))->to_f32()) == UINT32_C(1507152518));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_div", bit_cast<float>(UINT32_C(2442510077)), bit_cast<float>(UINT32_C(2906531411)))->to_f32()) == UINT32_C(2442510079));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_div", bit_cast<double>(UINT64_C(10062123074470382106)), bit_cast<double>(UINT64_C(12910565991996555404)))->to_f64()) == UINT64_C(10062123074470422078));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_div", bit_cast<double>(UINT64_C(6340937764684870564)), bit_cast<double>(UINT64_C(7244253720027059594)))->to_f64()) == UINT64_C(6340937764684870565));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_div", bit_cast<double>(UINT64_C(14905228263410157971)), bit_cast<double>(UINT64_C(11346251643264732732)))->to_f64()) == UINT64_C(14905228263410157970));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_div", bit_cast<double>(UINT64_C(3862352046163709780)), bit_cast<double>(UINT64_C(531112307488385734)))->to_f64()) == UINT64_C(3862079437827029803));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_div", bit_cast<double>(UINT64_C(16807035693954817236)), bit_cast<double>(UINT64_C(12360222454864961326)))->to_f64()) == UINT64_C(16807035693954817237));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_33_wasm>", "[float_exprs_33_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.33.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_mul", bit_cast<float>(UINT32_C(3538825650)), bit_cast<float>(UINT32_C(1315641462)))->to_f32()) == UINT32_C(3538825649));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_mul", bit_cast<float>(UINT32_C(2777664539)), bit_cast<float>(UINT32_C(3062588018)))->to_f32()) == UINT32_C(2777664540));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_mul", bit_cast<float>(UINT32_C(14863254)), bit_cast<float>(UINT32_C(3278582479)))->to_f32()) == UINT32_C(14863367));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_mul", bit_cast<float>(UINT32_C(2593594703)), bit_cast<float>(UINT32_C(3709508810)))->to_f32()) == UINT32_C(2593594656));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_mul", bit_cast<float>(UINT32_C(250394049)), bit_cast<float>(UINT32_C(1296755844)))->to_f32()) == UINT32_C(250394050));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_mul", bit_cast<double>(UINT64_C(665690489208775809)), bit_cast<double>(UINT64_C(14660005164454413124)))->to_f64()) == UINT64_C(665690577722002880));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_mul", bit_cast<double>(UINT64_C(10617267697387344269)), bit_cast<double>(UINT64_C(4370684778829606254)))->to_f64()) == UINT64_C(10617267697387344270));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_mul", bit_cast<double>(UINT64_C(13179336828827425934)), bit_cast<double>(UINT64_C(6536345148565138764)))->to_f64()) == UINT64_C(13179336828827425933));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_mul", bit_cast<double>(UINT64_C(12582623625647949669)), bit_cast<double>(UINT64_C(15106746174896642041)))->to_f64()) == UINT64_C(12582623625647949668));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_mul", bit_cast<double>(UINT64_C(16624217782795067216)), bit_cast<double>(UINT64_C(9062205521150975866)))->to_f64()) == UINT64_C(16624217782795067215));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_34_wasm>", "[float_exprs_34_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.34.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div2_mul2", bit_cast<float>(UINT32_C(16777215)))->to_f32()) == UINT32_C(16777216));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div2_mul2", bit_cast<double>(UINT64_C(9007199254740991)))->to_f64()) == UINT64_C(9007199254740992));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_35_wasm>", "[float_exprs_35_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.35.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "no_fold_demote_promote", bit_cast<double>(UINT64_C(13235495337234861917)))->to_f64()) == UINT64_C(13235495326728585216));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "no_fold_demote_promote", bit_cast<double>(UINT64_C(13448204151038380655)))->to_f64()) == UINT64_C(13448204151146151936));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "no_fold_demote_promote", bit_cast<double>(UINT64_C(5090364081358261697)))->to_f64()) == UINT64_C(5090364081378951168));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "no_fold_demote_promote", bit_cast<double>(UINT64_C(13436295269174285872)))->to_f64()) == UINT64_C(13436295269301878784));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "no_fold_demote_promote", bit_cast<double>(UINT64_C(5076240020598306430)))->to_f64()) == UINT64_C(5076240020759642112));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_36_wasm>", "[float_exprs_36_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.36.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "no_fold_promote_demote", bit_cast<float>(UINT32_C(2141192192)))));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_fold_promote_demote", bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_fold_promote_demote", bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_fold_promote_demote", bit_cast<float>(UINT32_C(1)))->to_f32()) == UINT32_C(1));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_fold_promote_demote", bit_cast<float>(UINT32_C(2147483649)))->to_f32()) == UINT32_C(2147483649));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_fold_promote_demote", bit_cast<float>(UINT32_C(8388607)))->to_f32()) == UINT32_C(8388607));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_fold_promote_demote", bit_cast<float>(UINT32_C(2155872255)))->to_f32()) == UINT32_C(2155872255));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_fold_promote_demote", bit_cast<float>(UINT32_C(8388608)))->to_f32()) == UINT32_C(8388608));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_fold_promote_demote", bit_cast<float>(UINT32_C(2155872256)))->to_f32()) == UINT32_C(2155872256));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_fold_promote_demote", bit_cast<float>(UINT32_C(2139095039)))->to_f32()) == UINT32_C(2139095039));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_fold_promote_demote", bit_cast<float>(UINT32_C(4286578687)))->to_f32()) == UINT32_C(4286578687));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_fold_promote_demote", bit_cast<float>(UINT32_C(2139095040)))->to_f32()) == UINT32_C(2139095040));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_fold_promote_demote", bit_cast<float>(UINT32_C(4286578688)))->to_f32()) == UINT32_C(4286578688));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_37_wasm>", "[float_exprs_37_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.37.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_demote_mixed_add", bit_cast<double>(UINT64_C(4183652368636204281)), bit_cast<float>(UINT32_C(69183310)))->to_f32()) == UINT32_C(276467023));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_demote_mixed_add", bit_cast<double>(UINT64_C(4773927428111915216)), bit_cast<float>(UINT32_C(1387972204)))->to_f32()) == UINT32_C(1392270651));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_demote_mixed_add", bit_cast<double>(UINT64_C(4072985553596038423)), bit_cast<float>(UINT32_C(2202918851)))->to_f32()) == UINT32_C(66813087));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_demote_mixed_add", bit_cast<double>(UINT64_C(13740716732336801211)), bit_cast<float>(UINT32_C(822392741)))->to_f32()) == UINT32_C(3045484077));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_demote_mixed_add", bit_cast<double>(UINT64_C(13742514716462174325)), bit_cast<float>(UINT32_C(2870112826)))->to_f32()) == UINT32_C(3048850075));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_demote_mixed_add_commuted", bit_cast<float>(UINT32_C(69183310)), bit_cast<double>(UINT64_C(4183652368636204281)))->to_f32()) == UINT32_C(276467023));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_demote_mixed_add_commuted", bit_cast<float>(UINT32_C(1387972204)), bit_cast<double>(UINT64_C(4773927428111915216)))->to_f32()) == UINT32_C(1392270651));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_demote_mixed_add_commuted", bit_cast<float>(UINT32_C(2202918851)), bit_cast<double>(UINT64_C(4072985553596038423)))->to_f32()) == UINT32_C(66813087));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_demote_mixed_add_commuted", bit_cast<float>(UINT32_C(822392741)), bit_cast<double>(UINT64_C(13740716732336801211)))->to_f32()) == UINT32_C(3045484077));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_demote_mixed_add_commuted", bit_cast<float>(UINT32_C(2870112826)), bit_cast<double>(UINT64_C(13742514716462174325)))->to_f32()) == UINT32_C(3048850075));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_38_wasm>", "[float_exprs_38_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.38.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_demote_mixed_sub", bit_cast<double>(UINT64_C(4979303437048015281)), bit_cast<float>(UINT32_C(1583535740)))->to_f32()) == UINT32_C(1758482618));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_demote_mixed_sub", bit_cast<double>(UINT64_C(13967600632962086462)), bit_cast<float>(UINT32_C(1214924370)))->to_f32()) == UINT32_C(3468107136));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_demote_mixed_sub", bit_cast<double>(UINT64_C(13860263758943608426)), bit_cast<float>(UINT32_C(969848030)))->to_f32()) == UINT32_C(3268174805));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_demote_mixed_sub", bit_cast<double>(UINT64_C(4364064588997139903)), bit_cast<float>(UINT32_C(472962692)))->to_f32()) == UINT32_C(612510881));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "no_demote_mixed_sub", bit_cast<double>(UINT64_C(4673175763235896759)), bit_cast<float>(UINT32_C(1198952676)))->to_f32()) == UINT32_C(3339501185));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_39_wasm>", "[float_exprs_39_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.39.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.i32.no_fold_trunc_s_convert_s", bit_cast<float>(UINT32_C(1069547520)))->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.i32.no_fold_trunc_s_convert_s", bit_cast<float>(UINT32_C(3217031168)))->to_f32()) == UINT32_C(3212836864));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.i32.no_fold_trunc_u_convert_s", bit_cast<float>(UINT32_C(1069547520)))->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.i32.no_fold_trunc_u_convert_s", bit_cast<float>(UINT32_C(3204448256)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.i32.no_fold_trunc_s_convert_u", bit_cast<float>(UINT32_C(1069547520)))->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.i32.no_fold_trunc_s_convert_u", bit_cast<float>(UINT32_C(3217031168)))->to_f32()) == UINT32_C(1333788672));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.i32.no_fold_trunc_u_convert_u", bit_cast<float>(UINT32_C(1069547520)))->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.i32.no_fold_trunc_u_convert_u", bit_cast<float>(UINT32_C(3204448256)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.i32.no_fold_trunc_s_convert_s", bit_cast<double>(UINT64_C(4609434218613702656)))->to_f64()) == UINT64_C(4607182418800017408));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.i32.no_fold_trunc_s_convert_s", bit_cast<double>(UINT64_C(13832806255468478464)))->to_f64()) == UINT64_C(13830554455654793216));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.i32.no_fold_trunc_u_convert_s", bit_cast<double>(UINT64_C(4609434218613702656)))->to_f64()) == UINT64_C(4607182418800017408));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.i32.no_fold_trunc_u_convert_s", bit_cast<double>(UINT64_C(13826050856027422720)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.i32.no_fold_trunc_s_convert_u", bit_cast<double>(UINT64_C(4609434218613702656)))->to_f64()) == UINT64_C(4607182418800017408));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.i32.no_fold_trunc_s_convert_u", bit_cast<double>(UINT64_C(13832806255468478464)))->to_f64()) == UINT64_C(4751297606873776128));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.i32.no_fold_trunc_u_convert_u", bit_cast<double>(UINT64_C(4609434218613702656)))->to_f64()) == UINT64_C(4607182418800017408));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.i32.no_fold_trunc_u_convert_u", bit_cast<double>(UINT64_C(13826050856027422720)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.i64.no_fold_trunc_s_convert_s", bit_cast<float>(UINT32_C(1069547520)))->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.i64.no_fold_trunc_s_convert_s", bit_cast<float>(UINT32_C(3217031168)))->to_f32()) == UINT32_C(3212836864));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.i64.no_fold_trunc_u_convert_s", bit_cast<float>(UINT32_C(1069547520)))->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.i64.no_fold_trunc_u_convert_s", bit_cast<float>(UINT32_C(3204448256)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.i64.no_fold_trunc_s_convert_u", bit_cast<float>(UINT32_C(1069547520)))->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.i64.no_fold_trunc_s_convert_u", bit_cast<float>(UINT32_C(3217031168)))->to_f32()) == UINT32_C(1602224128));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.i64.no_fold_trunc_u_convert_u", bit_cast<float>(UINT32_C(1069547520)))->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.i64.no_fold_trunc_u_convert_u", bit_cast<float>(UINT32_C(3204448256)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.i64.no_fold_trunc_s_convert_s", bit_cast<double>(UINT64_C(4609434218613702656)))->to_f64()) == UINT64_C(4607182418800017408));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.i64.no_fold_trunc_s_convert_s", bit_cast<double>(UINT64_C(13832806255468478464)))->to_f64()) == UINT64_C(13830554455654793216));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.i64.no_fold_trunc_u_convert_s", bit_cast<double>(UINT64_C(4609434218613702656)))->to_f64()) == UINT64_C(4607182418800017408));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.i64.no_fold_trunc_u_convert_s", bit_cast<double>(UINT64_C(13826050856027422720)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.i64.no_fold_trunc_s_convert_u", bit_cast<double>(UINT64_C(4609434218613702656)))->to_f64()) == UINT64_C(4607182418800017408));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.i64.no_fold_trunc_s_convert_u", bit_cast<double>(UINT64_C(13832806255468478464)))->to_f64()) == UINT64_C(4895412794951729152));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.i64.no_fold_trunc_u_convert_u", bit_cast<double>(UINT64_C(4609434218613702656)))->to_f64()) == UINT64_C(4607182418800017408));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.i64.no_fold_trunc_u_convert_u", bit_cast<double>(UINT64_C(13826050856027422720)))->to_f64()) == UINT64_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_4_wasm>", "[float_exprs_4_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.4.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_sub_zero", bit_cast<float>(UINT32_C(2141192192)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_sub_zero", bit_cast<double>(UINT64_C(9219994337134247936)))));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_40_wasm>", "[float_exprs_40_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.40.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

bkend(nullptr, "env", "init", UINT32_C(0), bit_cast<float>(UINT32_C(1097963930)));
bkend(nullptr, "env", "init", UINT32_C(4), bit_cast<float>(UINT32_C(1098068787)));
bkend(nullptr, "env", "init", UINT32_C(8), bit_cast<float>(UINT32_C(1098173645)));
bkend(nullptr, "env", "init", UINT32_C(12), bit_cast<float>(UINT32_C(1098278502)));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "check", UINT32_C(0))->to_f32()) == UINT32_C(1097963930));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "check", UINT32_C(4))->to_f32()) == UINT32_C(1098068787));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "check", UINT32_C(8))->to_f32()) == UINT32_C(1098173645));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "check", UINT32_C(12))->to_f32()) == UINT32_C(1098278502));
bkend(nullptr, "env", "run", UINT32_C(16), bit_cast<float>(UINT32_C(1077936128)));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "check", UINT32_C(0))->to_f32()) == UINT32_C(1084297489));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "check", UINT32_C(4))->to_f32()) == UINT32_C(1084367394));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "check", UINT32_C(8))->to_f32()) == UINT32_C(1084437299));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "check", UINT32_C(12))->to_f32()) == UINT32_C(1084507204));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_41_wasm>", "[float_exprs_41_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.41.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

bkend(nullptr, "env", "init", UINT32_C(0), bit_cast<double>(UINT64_C(4624690162351420211)));
bkend(nullptr, "env", "init", UINT32_C(8), bit_cast<double>(UINT64_C(4624746457346762342)));
bkend(nullptr, "env", "init", UINT32_C(16), bit_cast<double>(UINT64_C(4624802752342104474)));
bkend(nullptr, "env", "init", UINT32_C(24), bit_cast<double>(UINT64_C(4624859047337446605)));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "check", UINT32_C(0))->to_f64()) == UINT64_C(4624690162351420211));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "check", UINT32_C(8))->to_f64()) == UINT64_C(4624746457346762342));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "check", UINT32_C(16))->to_f64()) == UINT64_C(4624802752342104474));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "check", UINT32_C(24))->to_f64()) == UINT64_C(4624859047337446605));
bkend(nullptr, "env", "run", UINT32_C(32), bit_cast<double>(UINT64_C(4613937818241073152)));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "check", UINT32_C(0))->to_f64()) == UINT64_C(4617353047958495778));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "check", UINT32_C(8))->to_f64()) == UINT64_C(4617390577955390532));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "check", UINT32_C(16))->to_f64()) == UINT64_C(4617428107952285287));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "check", UINT32_C(24))->to_f64()) == UINT64_C(4617465637949180041));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_42_wasm>", "[float_exprs_42_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.42.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "f32.ult", bit_cast<float>(UINT32_C(1077936128)), bit_cast<float>(UINT32_C(1073741824)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.ult", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(1073741824)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.ult", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(1077936128)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.ult", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(2143289344)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.ule", bit_cast<float>(UINT32_C(1077936128)), bit_cast<float>(UINT32_C(1073741824)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.ule", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(1073741824)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.ule", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(1077936128)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.ule", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(2143289344)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.ugt", bit_cast<float>(UINT32_C(1077936128)), bit_cast<float>(UINT32_C(1073741824)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.ugt", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(1073741824)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.ugt", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(1077936128)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.ugt", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(2143289344)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.uge", bit_cast<float>(UINT32_C(1077936128)), bit_cast<float>(UINT32_C(1073741824)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.uge", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(1073741824)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.uge", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(1077936128)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.uge", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(2143289344)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.ult", bit_cast<double>(UINT64_C(4613937818241073152)), bit_cast<double>(UINT64_C(4611686018427387904)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.ult", bit_cast<double>(UINT64_C(4611686018427387904)), bit_cast<double>(UINT64_C(4611686018427387904)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.ult", bit_cast<double>(UINT64_C(4611686018427387904)), bit_cast<double>(UINT64_C(4613937818241073152)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.ult", bit_cast<double>(UINT64_C(4611686018427387904)), bit_cast<double>(UINT64_C(9221120237041090560)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.ule", bit_cast<double>(UINT64_C(4613937818241073152)), bit_cast<double>(UINT64_C(4611686018427387904)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.ule", bit_cast<double>(UINT64_C(4611686018427387904)), bit_cast<double>(UINT64_C(4611686018427387904)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.ule", bit_cast<double>(UINT64_C(4611686018427387904)), bit_cast<double>(UINT64_C(4613937818241073152)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.ule", bit_cast<double>(UINT64_C(4611686018427387904)), bit_cast<double>(UINT64_C(9221120237041090560)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.ugt", bit_cast<double>(UINT64_C(4613937818241073152)), bit_cast<double>(UINT64_C(4611686018427387904)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.ugt", bit_cast<double>(UINT64_C(4611686018427387904)), bit_cast<double>(UINT64_C(4611686018427387904)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.ugt", bit_cast<double>(UINT64_C(4611686018427387904)), bit_cast<double>(UINT64_C(4613937818241073152)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.ugt", bit_cast<double>(UINT64_C(4611686018427387904)), bit_cast<double>(UINT64_C(9221120237041090560)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.uge", bit_cast<double>(UINT64_C(4613937818241073152)), bit_cast<double>(UINT64_C(4611686018427387904)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.uge", bit_cast<double>(UINT64_C(4611686018427387904)), bit_cast<double>(UINT64_C(4611686018427387904)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.uge", bit_cast<double>(UINT64_C(4611686018427387904)), bit_cast<double>(UINT64_C(4613937818241073152)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.uge", bit_cast<double>(UINT64_C(4611686018427387904)), bit_cast<double>(UINT64_C(9221120237041090560)))->to_ui32() == UINT32_C(1));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_43_wasm>", "[float_exprs_43_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.43.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_lt_select", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2143289344)))->to_f32()) == UINT32_C(2143289344));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_lt_select", bit_cast<float>(UINT32_C(2143289344)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_lt_select", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_lt_select", bit_cast<float>(UINT32_C(2147483648)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_le_select", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2143289344)))->to_f32()) == UINT32_C(2143289344));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_le_select", bit_cast<float>(UINT32_C(2143289344)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_le_select", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_le_select", bit_cast<float>(UINT32_C(2147483648)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_gt_select", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2143289344)))->to_f32()) == UINT32_C(2143289344));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_gt_select", bit_cast<float>(UINT32_C(2143289344)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_gt_select", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_gt_select", bit_cast<float>(UINT32_C(2147483648)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_ge_select", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2143289344)))->to_f32()) == UINT32_C(2143289344));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_ge_select", bit_cast<float>(UINT32_C(2143289344)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_ge_select", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_ge_select", bit_cast<float>(UINT32_C(2147483648)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_lt_select", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9221120237041090560)))->to_f64()) == UINT64_C(9221120237041090560));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_lt_select", bit_cast<double>(UINT64_C(9221120237041090560)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_lt_select", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_lt_select", bit_cast<double>(UINT64_C(9223372036854775808)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_le_select", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9221120237041090560)))->to_f64()) == UINT64_C(9221120237041090560));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_le_select", bit_cast<double>(UINT64_C(9221120237041090560)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_le_select", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_le_select", bit_cast<double>(UINT64_C(9223372036854775808)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_gt_select", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9221120237041090560)))->to_f64()) == UINT64_C(9221120237041090560));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_gt_select", bit_cast<double>(UINT64_C(9221120237041090560)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_gt_select", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_gt_select", bit_cast<double>(UINT64_C(9223372036854775808)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_ge_select", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9221120237041090560)))->to_f64()) == UINT64_C(9221120237041090560));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_ge_select", bit_cast<double>(UINT64_C(9221120237041090560)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_ge_select", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_ge_select", bit_cast<double>(UINT64_C(9223372036854775808)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(9223372036854775808));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_44_wasm>", "[float_exprs_44_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.44.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_lt_if", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2143289344)))->to_f32()) == UINT32_C(2143289344));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_lt_if", bit_cast<float>(UINT32_C(2143289344)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_lt_if", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_lt_if", bit_cast<float>(UINT32_C(2147483648)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_le_if", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2143289344)))->to_f32()) == UINT32_C(2143289344));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_le_if", bit_cast<float>(UINT32_C(2143289344)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_le_if", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_le_if", bit_cast<float>(UINT32_C(2147483648)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_gt_if", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2143289344)))->to_f32()) == UINT32_C(2143289344));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_gt_if", bit_cast<float>(UINT32_C(2143289344)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_gt_if", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_gt_if", bit_cast<float>(UINT32_C(2147483648)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_ge_if", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2143289344)))->to_f32()) == UINT32_C(2143289344));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_ge_if", bit_cast<float>(UINT32_C(2143289344)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_ge_if", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_ge_if", bit_cast<float>(UINT32_C(2147483648)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_lt_if", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9221120237041090560)))->to_f64()) == UINT64_C(9221120237041090560));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_lt_if", bit_cast<double>(UINT64_C(9221120237041090560)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_lt_if", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_lt_if", bit_cast<double>(UINT64_C(9223372036854775808)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_le_if", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9221120237041090560)))->to_f64()) == UINT64_C(9221120237041090560));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_le_if", bit_cast<double>(UINT64_C(9221120237041090560)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_le_if", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_le_if", bit_cast<double>(UINT64_C(9223372036854775808)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_gt_if", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9221120237041090560)))->to_f64()) == UINT64_C(9221120237041090560));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_gt_if", bit_cast<double>(UINT64_C(9221120237041090560)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_gt_if", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_gt_if", bit_cast<double>(UINT64_C(9223372036854775808)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_ge_if", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9221120237041090560)))->to_f64()) == UINT64_C(9221120237041090560));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_ge_if", bit_cast<double>(UINT64_C(9221120237041090560)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_ge_if", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_ge_if", bit_cast<double>(UINT64_C(9223372036854775808)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(9223372036854775808));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_45_wasm>", "[float_exprs_45_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.45.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_lt_select_to_abs", bit_cast<float>(UINT32_C(2141192192)))->to_f32()) == UINT32_C(2141192192));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_lt_select_to_abs", bit_cast<float>(UINT32_C(4290772992)))->to_f32()) == UINT32_C(4290772992));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_lt_select_to_abs", bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_lt_select_to_abs", bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_le_select_to_abs", bit_cast<float>(UINT32_C(2141192192)))->to_f32()) == UINT32_C(2141192192));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_le_select_to_abs", bit_cast<float>(UINT32_C(4290772992)))->to_f32()) == UINT32_C(4290772992));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_le_select_to_abs", bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_le_select_to_abs", bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_gt_select_to_abs", bit_cast<float>(UINT32_C(2141192192)))->to_f32()) == UINT32_C(4288675840));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_gt_select_to_abs", bit_cast<float>(UINT32_C(4290772992)))->to_f32()) == UINT32_C(2143289344));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_gt_select_to_abs", bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_gt_select_to_abs", bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_ge_select_to_abs", bit_cast<float>(UINT32_C(2141192192)))->to_f32()) == UINT32_C(4288675840));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_ge_select_to_abs", bit_cast<float>(UINT32_C(4290772992)))->to_f32()) == UINT32_C(2143289344));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_ge_select_to_abs", bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_ge_select_to_abs", bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_lt_select_to_abs", bit_cast<double>(UINT64_C(9219994337134247936)))->to_f64()) == UINT64_C(9219994337134247936));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_lt_select_to_abs", bit_cast<double>(UINT64_C(18444492273895866368)))->to_f64()) == UINT64_C(18444492273895866368));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_lt_select_to_abs", bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_lt_select_to_abs", bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_le_select_to_abs", bit_cast<double>(UINT64_C(9219994337134247936)))->to_f64()) == UINT64_C(9219994337134247936));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_le_select_to_abs", bit_cast<double>(UINT64_C(18444492273895866368)))->to_f64()) == UINT64_C(18444492273895866368));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_le_select_to_abs", bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_le_select_to_abs", bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_gt_select_to_abs", bit_cast<double>(UINT64_C(9219994337134247936)))->to_f64()) == UINT64_C(18443366373989023744));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_gt_select_to_abs", bit_cast<double>(UINT64_C(18444492273895866368)))->to_f64()) == UINT64_C(9221120237041090560));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_gt_select_to_abs", bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_gt_select_to_abs", bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_ge_select_to_abs", bit_cast<double>(UINT64_C(9219994337134247936)))->to_f64()) == UINT64_C(18443366373989023744));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_ge_select_to_abs", bit_cast<double>(UINT64_C(18444492273895866368)))->to_f64()) == UINT64_C(9221120237041090560));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_ge_select_to_abs", bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_ge_select_to_abs", bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(9223372036854775808));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_46_wasm>", "[float_exprs_46_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.46.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_lt_if_to_abs", bit_cast<float>(UINT32_C(2141192192)))->to_f32()) == UINT32_C(2141192192));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_lt_if_to_abs", bit_cast<float>(UINT32_C(4290772992)))->to_f32()) == UINT32_C(4290772992));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_lt_if_to_abs", bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_lt_if_to_abs", bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_le_if_to_abs", bit_cast<float>(UINT32_C(2141192192)))->to_f32()) == UINT32_C(2141192192));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_le_if_to_abs", bit_cast<float>(UINT32_C(4290772992)))->to_f32()) == UINT32_C(4290772992));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_le_if_to_abs", bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_le_if_to_abs", bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_gt_if_to_abs", bit_cast<float>(UINT32_C(2141192192)))->to_f32()) == UINT32_C(4288675840));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_gt_if_to_abs", bit_cast<float>(UINT32_C(4290772992)))->to_f32()) == UINT32_C(2143289344));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_gt_if_to_abs", bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_gt_if_to_abs", bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_ge_if_to_abs", bit_cast<float>(UINT32_C(2141192192)))->to_f32()) == UINT32_C(4288675840));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_ge_if_to_abs", bit_cast<float>(UINT32_C(4290772992)))->to_f32()) == UINT32_C(2143289344));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_ge_if_to_abs", bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_ge_if_to_abs", bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_lt_if_to_abs", bit_cast<double>(UINT64_C(9219994337134247936)))->to_f64()) == UINT64_C(9219994337134247936));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_lt_if_to_abs", bit_cast<double>(UINT64_C(18444492273895866368)))->to_f64()) == UINT64_C(18444492273895866368));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_lt_if_to_abs", bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_lt_if_to_abs", bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_le_if_to_abs", bit_cast<double>(UINT64_C(9219994337134247936)))->to_f64()) == UINT64_C(9219994337134247936));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_le_if_to_abs", bit_cast<double>(UINT64_C(18444492273895866368)))->to_f64()) == UINT64_C(18444492273895866368));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_le_if_to_abs", bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_le_if_to_abs", bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_gt_if_to_abs", bit_cast<double>(UINT64_C(9219994337134247936)))->to_f64()) == UINT64_C(18443366373989023744));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_gt_if_to_abs", bit_cast<double>(UINT64_C(18444492273895866368)))->to_f64()) == UINT64_C(9221120237041090560));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_gt_if_to_abs", bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_gt_if_to_abs", bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_ge_if_to_abs", bit_cast<double>(UINT64_C(9219994337134247936)))->to_f64()) == UINT64_C(18443366373989023744));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_ge_if_to_abs", bit_cast<double>(UINT64_C(18444492273895866368)))->to_f64()) == UINT64_C(9221120237041090560));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_ge_if_to_abs", bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_ge_if_to_abs", bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(9223372036854775808));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_47_wasm>", "[float_exprs_47_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.47.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.incorrect_correction")->to_f32()) == UINT32_C(872415232));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.incorrect_correction")->to_f64()) == UINT64_C(13596367275031527424));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_48_wasm>", "[float_exprs_48_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.48.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "calculate")->to_f32()) == UINT32_C(3286857379));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_49_wasm>", "[float_exprs_49_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.49.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "calculate")->to_f64()) == UINT64_C(13870293918930799763));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_5_wasm>", "[float_exprs_5_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.5.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_zero", bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_zero", bit_cast<float>(UINT32_C(3212836864)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_zero", bit_cast<float>(UINT32_C(3221225472)))->to_f32()) == UINT32_C(2147483648));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_zero", bit_cast<float>(UINT32_C(2141192192)))));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_zero", bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_zero", bit_cast<double>(UINT64_C(13830554455654793216)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_zero", bit_cast<double>(UINT64_C(13835058055282163712)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_zero", bit_cast<double>(UINT64_C(9219994337134247936)))));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_50_wasm>", "[float_exprs_50_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.50.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "llvm_pr26746", bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_51_wasm>", "[float_exprs_51_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.51.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "llvm_pr27153", UINT32_C(33554434))->to_f32()) == UINT32_C(1270874112));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_52_wasm>", "[float_exprs_52_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.52.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "llvm_pr27036", UINT32_C(4269932491), UINT32_C(14942208))->to_f32()) == UINT32_C(3407478836));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_53_wasm>", "[float_exprs_53_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.53.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "thepast0", bit_cast<double>(UINT64_C(9007199254740992)), bit_cast<double>(UINT64_C(4607182418800017407)), bit_cast<double>(UINT64_C(4611686018427387904)), bit_cast<double>(UINT64_C(4602678819172646912)))->to_f64()) == UINT64_C(9007199254740991));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "thepast1", bit_cast<double>(UINT64_C(4363988038922010624)), bit_cast<double>(UINT64_C(4607182418800017407)), bit_cast<double>(UINT64_C(4363988038922010624)))->to_f64()) == UINT64_C(13348669295526150144));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "thepast2", bit_cast<float>(UINT32_C(16777216)), bit_cast<float>(UINT32_C(1056964608)), bit_cast<float>(UINT32_C(1065353216)))->to_f32()) == UINT32_C(8388608));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_54_wasm>", "[float_exprs_54_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.54.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "inverse", bit_cast<float>(UINT32_C(1119879168)))->to_f32()) == UINT32_C(1009429163));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_55_wasm>", "[float_exprs_55_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.55.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32_sqrt_minus_2", bit_cast<float>(UINT32_C(1082130432)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64_sqrt_minus_2", bit_cast<double>(UINT64_C(4616189618054758400)))->to_f64()) == UINT64_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_56_wasm>", "[float_exprs_56_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.56.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_recip_recip", bit_cast<float>(UINT32_C(3765723020)))->to_f32()) == UINT32_C(3765723019));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_recip_recip", bit_cast<float>(UINT32_C(426844452)))->to_f32()) == UINT32_C(426844451));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_recip_recip", bit_cast<float>(UINT32_C(535132276)))->to_f32()) == UINT32_C(535132277));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_recip_recip", bit_cast<float>(UINT32_C(3253941441)))->to_f32()) == UINT32_C(3253941442));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_recip_recip", bit_cast<float>(UINT32_C(1660734603)))->to_f32()) == UINT32_C(1660734602));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_recip_recip", bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_recip_recip", bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_recip_recip", bit_cast<float>(UINT32_C(4286578688)))->to_f32()) == UINT32_C(4286578688));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_recip_recip", bit_cast<float>(UINT32_C(2139095040)))->to_f32()) == UINT32_C(2139095040));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_recip_recip", bit_cast<double>(UINT64_C(14500888369201570768)))->to_f64()) == UINT64_C(14500888369201570769));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_recip_recip", bit_cast<double>(UINT64_C(14132092565459057123)))->to_f64()) == UINT64_C(14132092565459057122));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_recip_recip", bit_cast<double>(UINT64_C(5359183527603521526)))->to_f64()) == UINT64_C(5359183527603521525));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_recip_recip", bit_cast<double>(UINT64_C(1521566147669375634)))->to_f64()) == UINT64_C(1521566147669375633));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_recip_recip", bit_cast<double>(UINT64_C(8671785631545870379)))->to_f64()) == UINT64_C(8671785631545870378));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_recip_recip", bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_recip_recip", bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_recip_recip", bit_cast<double>(UINT64_C(18442240474082181120)))->to_f64()) == UINT64_C(18442240474082181120));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_recip_recip", bit_cast<double>(UINT64_C(9218868437227405312)))->to_f64()) == UINT64_C(9218868437227405312));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_57_wasm>", "[float_exprs_57_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.57.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_algebraic_factoring", bit_cast<float>(UINT32_C(2759308231)), bit_cast<float>(UINT32_C(618704988)))->to_f32()) == UINT32_C(2315864577));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_algebraic_factoring", bit_cast<float>(UINT32_C(3415653214)), bit_cast<float>(UINT32_C(1274676302)))->to_f32()) == UINT32_C(3625675853));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_algebraic_factoring", bit_cast<float>(UINT32_C(1446924633)), bit_cast<float>(UINT32_C(3607373982)))->to_f32()) == UINT32_C(4000155759));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_algebraic_factoring", bit_cast<float>(UINT32_C(1212067608)), bit_cast<float>(UINT32_C(3278094810)))->to_f32()) == UINT32_C(1359874131));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_algebraic_factoring", bit_cast<float>(UINT32_C(3278732464)), bit_cast<float>(UINT32_C(3379389272)))->to_f32()) == UINT32_C(3546030359));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_algebraic_factoring", bit_cast<double>(UINT64_C(2467435761933928117)), bit_cast<double>(UINT64_C(2526113756828458004)))->to_f64()) == UINT64_C(9668435399096543331));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_algebraic_factoring", bit_cast<double>(UINT64_C(2911983657790464931)), bit_cast<double>(UINT64_C(2814431682419759911)))->to_f64()) == UINT64_C(1217162942843921803));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_algebraic_factoring", bit_cast<double>(UINT64_C(12131637044948792058)), bit_cast<double>(UINT64_C(12170782965730311956)))->to_f64()) == UINT64_C(10511676135434922533));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_algebraic_factoring", bit_cast<double>(UINT64_C(14639789466354372633)), bit_cast<double>(UINT64_C(5456963169336729236)))->to_f64()) == UINT64_C(15530333405173431543));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_algebraic_factoring", bit_cast<double>(UINT64_C(5121779675912507154)), bit_cast<double>(UINT64_C(14237286623175920791)))->to_f64()) == UINT64_C(5636689734063865714));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_58_wasm>", "[float_exprs_58_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.58.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_algebraic_factoring", bit_cast<float>(UINT32_C(684135946)), bit_cast<float>(UINT32_C(744319693)))->to_f32()) == UINT32_C(2571075368));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_algebraic_factoring", bit_cast<float>(UINT32_C(3560929481)), bit_cast<float>(UINT32_C(3496840229)))->to_f32()) == UINT32_C(1762604185));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_algebraic_factoring", bit_cast<float>(UINT32_C(968841772)), bit_cast<float>(UINT32_C(3106497100)))->to_f32()) == UINT32_C(870712803));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_algebraic_factoring", bit_cast<float>(UINT32_C(697514723)), bit_cast<float>(UINT32_C(2834753933)))->to_f32()) == UINT32_C(327914662));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_algebraic_factoring", bit_cast<float>(UINT32_C(1498230729)), bit_cast<float>(UINT32_C(3650453580)))->to_f32()) == UINT32_C(4080583891));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_algebraic_factoring", bit_cast<double>(UINT64_C(6212515167506370409)), bit_cast<double>(UINT64_C(15348474890798978273)))->to_f64()) == UINT64_C(7818515589337550196));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_algebraic_factoring", bit_cast<double>(UINT64_C(15222970140370015722)), bit_cast<double>(UINT64_C(15325207139996136125)))->to_f64()) == UINT64_C(16819892485880140289));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_algebraic_factoring", bit_cast<double>(UINT64_C(4797439202963874050)), bit_cast<double>(UINT64_C(14009643534571442918)))->to_f64()) == UINT64_C(4987747999326390045));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_algebraic_factoring", bit_cast<double>(UINT64_C(14653559129294038194)), bit_cast<double>(UINT64_C(14581996260169223461)))->to_f64()) == UINT64_C(6253339631158964222));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_algebraic_factoring", bit_cast<double>(UINT64_C(12768321634751930140)), bit_cast<double>(UINT64_C(12767602092732820937)))->to_f64()) == UINT64_C(2473652960990319032));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_59_wasm>", "[float_exprs_59_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.59.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(!bkend.call_with_return(nullptr, "env", "f32.simple_x4_sum", UINT32_C(0), UINT32_C(16), UINT32_C(32)));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.load", UINT32_C(32))->to_f32()) == UINT32_C(2));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.load", UINT32_C(36))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.load", UINT32_C(40))->to_f32()) == UINT32_C(1));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.load", UINT32_C(44))->to_f32()) == UINT32_C(2147483649));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_6_wasm>", "[float_exprs_6_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.6.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_one", bit_cast<float>(UINT32_C(2141192192)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_one", bit_cast<double>(UINT64_C(9219994337134247936)))));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_60_wasm>", "[float_exprs_60_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.60.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(!bkend.call_with_return(nullptr, "env", "f64.simple_x4_sum", UINT32_C(0), UINT32_C(32), UINT32_C(64)));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.load", UINT32_C(64))->to_f64()) == UINT64_C(2));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.load", UINT32_C(72))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.load", UINT32_C(80))->to_f64()) == UINT64_C(1));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.load", UINT32_C(88))->to_f64()) == UINT64_C(9223372036854775809));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_61_wasm>", "[float_exprs_61_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.61.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.kahan_sum", UINT32_C(0), UINT32_C(256))->to_f32()) == UINT32_C(4085779725));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.plain_sum", UINT32_C(0), UINT32_C(256))->to_f32()) == UINT32_C(4082113053));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_62_wasm>", "[float_exprs_62_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.62.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.kahan_sum", UINT32_C(0), UINT32_C(256))->to_f64()) == UINT64_C(9105671289202277512));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.plain_sum", UINT32_C(0), UINT32_C(256))->to_f64()) == UINT64_C(9105671289202539655));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_63_wasm>", "[float_exprs_63_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.63.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_neg_sub", bit_cast<float>(UINT32_C(2147483648)), bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_neg_sub", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_neg_sub", bit_cast<float>(UINT32_C(2147483648)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_neg_sub", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_neg_sub", bit_cast<double>(UINT64_C(9223372036854775808)), bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_neg_sub", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_neg_sub", bit_cast<double>(UINT64_C(9223372036854775808)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_neg_sub", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(9223372036854775808));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_64_wasm>", "[float_exprs_64_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.64.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_neg_add", bit_cast<float>(UINT32_C(2147483648)), bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_neg_add", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_neg_add", bit_cast<float>(UINT32_C(2147483648)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_neg_add", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_neg_add", bit_cast<double>(UINT64_C(9223372036854775808)), bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_neg_add", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_neg_add", bit_cast<double>(UINT64_C(9223372036854775808)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_neg_add", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(9223372036854775808));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_65_wasm>", "[float_exprs_65_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.65.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_neg_neg", bit_cast<float>(UINT32_C(2147483648)), bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_neg_neg", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_neg_neg", bit_cast<float>(UINT32_C(2147483648)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_neg_neg", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_neg_neg", bit_cast<double>(UINT64_C(9223372036854775808)), bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_neg_neg", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_neg_neg", bit_cast<double>(UINT64_C(9223372036854775808)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_neg_neg", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(9223372036854775808));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_66_wasm>", "[float_exprs_66_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.66.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_neg", bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_neg", bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(0));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_neg", bit_cast<float>(UINT32_C(2139095040)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_neg", bit_cast<float>(UINT32_C(4286578688)))));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_neg", bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_neg", bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(0));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_neg", bit_cast<double>(UINT64_C(9218868437227405312)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_neg", bit_cast<double>(UINT64_C(18442240474082181120)))));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_67_wasm>", "[float_exprs_67_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.67.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_6x_via_add", bit_cast<float>(UINT32_C(4046243078)))->to_f32()) == UINT32_C(4068578245));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_6x_via_add", bit_cast<float>(UINT32_C(2573857750)))->to_f32()) == UINT32_C(2595190497));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_6x_via_add", bit_cast<float>(UINT32_C(419462401)))->to_f32()) == UINT32_C(440449921));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_6x_via_add", bit_cast<float>(UINT32_C(2955475482)))->to_f32()) == UINT32_C(2977789734));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_6x_via_add", bit_cast<float>(UINT32_C(3883931973)))->to_f32()) == UINT32_C(3904906727));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_6x_via_add", bit_cast<double>(UINT64_C(14137662215323058150)))->to_f64()) == UINT64_C(14149352706895019994));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_6x_via_add", bit_cast<double>(UINT64_C(11424134044545165748)))->to_f64()) == UINT64_C(11435767596137037638));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_6x_via_add", bit_cast<double>(UINT64_C(15055410132664937138)))->to_f64()) == UINT64_C(15066699987142021125));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_6x_via_add", bit_cast<double>(UINT64_C(7991451501228919438)))->to_f64()) == UINT64_C(8003319959635773419));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_6x_via_add", bit_cast<double>(UINT64_C(14886926859367497770)))->to_f64()) == UINT64_C(14898679235615764511));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_68_wasm>", "[float_exprs_68_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.68.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_div", bit_cast<float>(UINT32_C(3875242260)), bit_cast<float>(UINT32_C(3086869257)), bit_cast<float>(UINT32_C(3301317576)))->to_f32()) == UINT32_C(3911440926));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_div", bit_cast<float>(UINT32_C(485052055)), bit_cast<float>(UINT32_C(1996083391)), bit_cast<float>(UINT32_C(2276616712)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_div", bit_cast<float>(UINT32_C(1430470604)), bit_cast<float>(UINT32_C(186144382)), bit_cast<float>(UINT32_C(1953564780)))->to_f32()) == UINT32_C(2139095040));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_div", bit_cast<float>(UINT32_C(3101818893)), bit_cast<float>(UINT32_C(4258133430)), bit_cast<float>(UINT32_C(2855958950)))->to_f32()) == UINT32_C(2411777082));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_div", bit_cast<float>(UINT32_C(1458407223)), bit_cast<float>(UINT32_C(1537931089)), bit_cast<float>(UINT32_C(4260989344)))->to_f32()) == UINT32_C(2147507000));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_div", bit_cast<double>(UINT64_C(6128077243319875447)), bit_cast<double>(UINT64_C(7240092044185667120)), bit_cast<double>(UINT64_C(10312472494987686942)))->to_f64()) == UINT64_C(16236150182064455170));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_div", bit_cast<double>(UINT64_C(17395933367696573535)), bit_cast<double>(UINT64_C(4478922858584402707)), bit_cast<double>(UINT64_C(6032094754408482817)))->to_f64()) == UINT64_C(16098470347548634769));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_div", bit_cast<double>(UINT64_C(13843263185226986279)), bit_cast<double>(UINT64_C(17796742619038211051)), bit_cast<double>(UINT64_C(5375701731263473827)))->to_f64()) == UINT64_C(44472927));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_div", bit_cast<double>(UINT64_C(17547288444310957340)), bit_cast<double>(UINT64_C(911654786857739111)), bit_cast<double>(UINT64_C(8937284546802896640)))->to_f64()) == UINT64_C(18442240474082181120));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_div", bit_cast<double>(UINT64_C(9835707468114203513)), bit_cast<double>(UINT64_C(1924400690116523912)), bit_cast<double>(UINT64_C(13208934041167870811)))->to_f64()) == UINT64_C(3916014548332337260));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_69_wasm>", "[float_exprs_69_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.69.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_divs", bit_cast<float>(UINT32_C(2304917983)), bit_cast<float>(UINT32_C(301403678)), bit_cast<float>(UINT32_C(331350955)), bit_cast<float>(UINT32_C(3251297465)))->to_f32()) == UINT32_C(148760966));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_divs", bit_cast<float>(UINT32_C(4068974897)), bit_cast<float>(UINT32_C(1276265036)), bit_cast<float>(UINT32_C(930821438)), bit_cast<float>(UINT32_C(1044692964)))->to_f32()) == UINT32_C(3742862674));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_divs", bit_cast<float>(UINT32_C(3496980369)), bit_cast<float>(UINT32_C(3548280607)), bit_cast<float>(UINT32_C(3461305482)), bit_cast<float>(UINT32_C(3298174616)))->to_f32()) == UINT32_C(1176926862));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_divs", bit_cast<float>(UINT32_C(4135236702)), bit_cast<float>(UINT32_C(787270424)), bit_cast<float>(UINT32_C(932959293)), bit_cast<float>(UINT32_C(1724950821)))->to_f32()) == UINT32_C(4286578688));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_divs", bit_cast<float>(UINT32_C(622783177)), bit_cast<float>(UINT32_C(2677642769)), bit_cast<float>(UINT32_C(307759154)), bit_cast<float>(UINT32_C(768171421)))->to_f32()) == UINT32_C(2844661464));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_divs", bit_cast<double>(UINT64_C(10143060558527560466)), bit_cast<double>(UINT64_C(11745059379675007839)), bit_cast<double>(UINT64_C(16295837305232663584)), bit_cast<double>(UINT64_C(5444961058358534642)))->to_f64()) == UINT64_C(13856326607560224491));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_divs", bit_cast<double>(UINT64_C(14349445329289351080)), bit_cast<double>(UINT64_C(468238185841254727)), bit_cast<double>(UINT64_C(15463559257629249878)), bit_cast<double>(UINT64_C(15937497686185055572)))->to_f64()) == UINT64_C(18442240474082181120));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_divs", bit_cast<double>(UINT64_C(15220380342429201729)), bit_cast<double>(UINT64_C(14697937818549468616)), bit_cast<double>(UINT64_C(13203624158275174657)), bit_cast<double>(UINT64_C(17131104131485469546)))->to_f64()) == UINT64_C(1202126128702318245));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_divs", bit_cast<double>(UINT64_C(14414969397981384765)), bit_cast<double>(UINT64_C(12269327994486371199)), bit_cast<double>(UINT64_C(298707625567048656)), bit_cast<double>(UINT64_C(5613107161545919917)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_divs", bit_cast<double>(UINT64_C(4529089342618677929)), bit_cast<double>(UINT64_C(3361245300043094097)), bit_cast<double>(UINT64_C(1815899012046749567)), bit_cast<double>(UINT64_C(15418396504351552390)))->to_f64()) == UINT64_C(10619033301585441215));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_7_wasm>", "[float_exprs_7_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.7.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_zero_div", bit_cast<float>(UINT32_C(0)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_zero_div", bit_cast<float>(UINT32_C(2147483648)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_zero_div", bit_cast<float>(UINT32_C(2143289344)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_zero_div", bit_cast<float>(UINT32_C(2141192192)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_zero_div", bit_cast<double>(UINT64_C(0)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_zero_div", bit_cast<double>(UINT64_C(9223372036854775808)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_zero_div", bit_cast<double>(UINT64_C(9221120237041090560)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_zero_div", bit_cast<double>(UINT64_C(9219994337134247936)))));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_70_wasm>", "[float_exprs_70_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.70.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_divs", bit_cast<float>(UINT32_C(1136439096)), bit_cast<float>(UINT32_C(3173274359)), bit_cast<float>(UINT32_C(4274852390)))->to_f32()) == UINT32_C(2221638875));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_divs", bit_cast<float>(UINT32_C(2690073844)), bit_cast<float>(UINT32_C(2809448479)), bit_cast<float>(UINT32_C(3608905030)))->to_f32()) == UINT32_C(264862203));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_divs", bit_cast<float>(UINT32_C(2830184964)), bit_cast<float>(UINT32_C(530019033)), bit_cast<float>(UINT32_C(3623253973)))->to_f32()) == UINT32_C(272108594));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_divs", bit_cast<float>(UINT32_C(2365787800)), bit_cast<float>(UINT32_C(245111369)), bit_cast<float>(UINT32_C(3952003433)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_divs", bit_cast<float>(UINT32_C(982471119)), bit_cast<float>(UINT32_C(1045692415)), bit_cast<float>(UINT32_C(37216954)))->to_f32()) == UINT32_C(2073319791));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_divs", bit_cast<double>(UINT64_C(15770585325769044278)), bit_cast<double>(UINT64_C(6564157675451289455)), bit_cast<double>(UINT64_C(8712254759989822359)))->to_f64()) == UINT64_C(2458462832069881218));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_divs", bit_cast<double>(UINT64_C(14069844870254671283)), bit_cast<double>(UINT64_C(4634122757084803708)), bit_cast<double>(UINT64_C(9524897388132352235)))->to_f64()) == UINT64_C(9152039358940941283));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_divs", bit_cast<double>(UINT64_C(9479648703296052622)), bit_cast<double>(UINT64_C(214573661502224386)), bit_cast<double>(UINT64_C(6877551490107761946)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_divs", bit_cast<double>(UINT64_C(6019502660029506228)), bit_cast<double>(UINT64_C(15316513033818836241)), bit_cast<double>(UINT64_C(4039967192182502935)))->to_f64()) == UINT64_C(15883525310425977300));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_divs", bit_cast<double>(UINT64_C(10555667216821129841)), bit_cast<double>(UINT64_C(1207418919037494573)), bit_cast<double>(UINT64_C(4296330408727545598)))->to_f64()) == UINT64_C(10866511466898347555));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_71_wasm>", "[float_exprs_71_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.71.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_sqrt_square", bit_cast<float>(UINT32_C(2662226315)))->to_f32()) == UINT32_C(514742673));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_sqrt_square", bit_cast<float>(UINT32_C(2606267634)))->to_f32()) == UINT32_C(458819801));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_sqrt_square", bit_cast<float>(UINT32_C(2624528574)))->to_f32()) == UINT32_C(477049564));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_sqrt_square", bit_cast<float>(UINT32_C(347235385)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_sqrt_square", bit_cast<float>(UINT32_C(1978715378)))->to_f32()) == UINT32_C(2139095040));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_sqrt_square", bit_cast<double>(UINT64_C(2225189009770021885)))->to_f64()) == UINT64_C(2225189011649283571));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_sqrt_square", bit_cast<double>(UINT64_C(11517048459773840771)))->to_f64()) == UINT64_C(2293676422919064961));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_sqrt_square", bit_cast<double>(UINT64_C(11484764485761855006)))->to_f64()) == UINT64_C(2261392448906973069));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_sqrt_square", bit_cast<double>(UINT64_C(11056484744549647728)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_sqrt_square", bit_cast<double>(UINT64_C(8465406758332488378)))->to_f64()) == UINT64_C(9218868437227405312));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_72_wasm>", "[float_exprs_72_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.72.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_sqrts", bit_cast<float>(UINT32_C(24047316)), bit_cast<float>(UINT32_C(2517821717)))));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_sqrts", bit_cast<float>(UINT32_C(295749258)), bit_cast<float>(UINT32_C(803416494)))->to_f32()) == UINT32_C(549395357));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_sqrts", bit_cast<float>(UINT32_C(329708528)), bit_cast<float>(UINT32_C(1120042892)))->to_f32()) == UINT32_C(724841268));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_sqrts", bit_cast<float>(UINT32_C(1916535951)), bit_cast<float>(UINT32_C(994115420)))->to_f32()) == UINT32_C(1455324620));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_sqrts", bit_cast<float>(UINT32_C(598482176)), bit_cast<float>(UINT32_C(990534933)))->to_f32()) == UINT32_C(794443079));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_sqrts", bit_cast<double>(UINT64_C(10974446854152441278)), bit_cast<double>(UINT64_C(13797896470155574122)))));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_sqrts", bit_cast<double>(UINT64_C(1712959863583927241)), bit_cast<double>(UINT64_C(2792003944717853898)))->to_f64()) == UINT64_C(2252469008297979510));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_sqrts", bit_cast<double>(UINT64_C(4208351758938831157)), bit_cast<double>(UINT64_C(497361189565243603)))->to_f64()) == UINT64_C(2352856462697312748));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_sqrts", bit_cast<double>(UINT64_C(2976792199849816182)), bit_cast<double>(UINT64_C(2030444188042608984)))->to_f64()) == UINT64_C(2503613111125550255));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_sqrts", bit_cast<double>(UINT64_C(4717634334691577101)), bit_cast<double>(UINT64_C(6919598687070693285)))->to_f64()) == UINT64_C(5818898567902921651));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_73_wasm>", "[float_exprs_73_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.73.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_sqrts", bit_cast<float>(UINT32_C(3428799709)), bit_cast<float>(UINT32_C(2733489079)))));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_sqrts", bit_cast<float>(UINT32_C(1339867611)), bit_cast<float>(UINT32_C(1296568207)))->to_f32()) == UINT32_C(1086203643));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_sqrts", bit_cast<float>(UINT32_C(65679161)), bit_cast<float>(UINT32_C(1196795110)))->to_f32()) == UINT32_C(498959746));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_sqrts", bit_cast<float>(UINT32_C(1566143010)), bit_cast<float>(UINT32_C(816694667)))->to_f32()) == UINT32_C(1439333972));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_sqrts", bit_cast<float>(UINT32_C(130133331)), bit_cast<float>(UINT32_C(208189588)))->to_f32()) == UINT32_C(1025844032));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_sqrts", bit_cast<double>(UINT64_C(10629913473787695463)), bit_cast<double>(UINT64_C(12991130264919696663)))));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_sqrts", bit_cast<double>(UINT64_C(1966780663211935584)), bit_cast<double>(UINT64_C(7043916066229883379)))->to_f64()) == UINT64_C(2068364230648818889));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_sqrts", bit_cast<double>(UINT64_C(6965599900716272009)), bit_cast<double>(UINT64_C(4118781927977980600)))->to_f64()) == UINT64_C(6030491425828883991));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_sqrts", bit_cast<double>(UINT64_C(962551478168675351)), bit_cast<double>(UINT64_C(5918292176617055751)))->to_f64()) == UINT64_C(2129092583060403799));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_sqrts", bit_cast<double>(UINT64_C(1056821405580891413)), bit_cast<double>(UINT64_C(8865548665903786673)))->to_f64()) == UINT64_C(702724841785532050));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_74_wasm>", "[float_exprs_74_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.74.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_sqrt_div", bit_cast<float>(UINT32_C(3900330981)), bit_cast<float>(UINT32_C(1843416431)))->to_f32()) == UINT32_C(4286578688));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_sqrt_div", bit_cast<float>(UINT32_C(2210946958)), bit_cast<float>(UINT32_C(256302916)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_sqrt_div", bit_cast<float>(UINT32_C(1312995444)), bit_cast<float>(UINT32_C(2371494)))->to_f32()) == UINT32_C(1849105549));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_sqrt_div", bit_cast<float>(UINT32_C(3576537897)), bit_cast<float>(UINT32_C(2010442638)))->to_f32()) == UINT32_C(3104219421));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_sqrt_div", bit_cast<float>(UINT32_C(3284697858)), bit_cast<float>(UINT32_C(1124488329)))->to_f32()) == UINT32_C(3255461622));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_sqrt_div", bit_cast<double>(UINT64_C(7751219282814906463)), bit_cast<double>(UINT64_C(8023732701704228537)))->to_f64()) == UINT64_C(9218868437227405312));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_sqrt_div", bit_cast<double>(UINT64_C(10108528314069607083)), bit_cast<double>(UINT64_C(1595930056995453707)))->to_f64()) == UINT64_C(9223372036854775808));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_sqrt_div", bit_cast<double>(UINT64_C(2695209648295623224)), bit_cast<double>(UINT64_C(7133480874314061811)))->to_f64()) == UINT64_C(1432338140829931582));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_sqrt_div", bit_cast<double>(UINT64_C(15416524255949334213)), bit_cast<double>(UINT64_C(2434442666062773630)))->to_f64()) == UINT64_C(16502590179898118478));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_sqrt_div", bit_cast<double>(UINT64_C(5076901024782455083)), bit_cast<double>(UINT64_C(8399438310541178654)))->to_f64()) == UINT64_C(3180744754328846996));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_75_wasm>", "[float_exprs_75_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.75.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_flush_intermediate_subnormal", bit_cast<float>(UINT32_C(8388608)), bit_cast<float>(UINT32_C(872415232)), bit_cast<float>(UINT32_C(1258291200)))->to_f32()) == UINT32_C(8388608));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_flush_intermediate_subnormal", bit_cast<double>(UINT64_C(4503599627370496)), bit_cast<double>(UINT64_C(4372995238176751616)), bit_cast<double>(UINT64_C(4841369599423283200)))->to_f64()) == UINT64_C(4503599627370496));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_76_wasm>", "[float_exprs_76_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.76.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "f32.recoding_eq", bit_cast<float>(UINT32_C(4286578688)), bit_cast<float>(UINT32_C(1077936128)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.recoding_le", bit_cast<float>(UINT32_C(4286578688)), bit_cast<float>(UINT32_C(1077936128)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.recoding_lt", bit_cast<float>(UINT32_C(4286578688)), bit_cast<float>(UINT32_C(1077936128)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.recoding_eq", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(1065353216)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.recoding_le", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(1065353216)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.recoding_lt", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(1065353216)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.recoding_eq", bit_cast<double>(UINT64_C(18442240474082181120)), bit_cast<double>(UINT64_C(4613937818241073152)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.recoding_le", bit_cast<double>(UINT64_C(18442240474082181120)), bit_cast<double>(UINT64_C(4613937818241073152)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.recoding_lt", bit_cast<double>(UINT64_C(18442240474082181120)), bit_cast<double>(UINT64_C(4613937818241073152)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.recoding_eq", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(4607182418800017408)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.recoding_le", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(4607182418800017408)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.recoding_lt", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(4607182418800017408)))->to_ui32() == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "recoding_demote", bit_cast<double>(UINT64_C(4014054135371399168)), bit_cast<float>(UINT32_C(1150853120)))->to_f32()) == UINT32_C(46548238));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_77_wasm>", "[float_exprs_77_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.77.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "f32.no_extended_precision_div", bit_cast<float>(UINT32_C(1077936128)), bit_cast<float>(UINT32_C(1088421888)), bit_cast<float>(UINT32_C(1054567863)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.no_extended_precision_div", bit_cast<double>(UINT64_C(4613937818241073152)), bit_cast<double>(UINT64_C(4619567317775286272)), bit_cast<double>(UINT64_C(4601392076421969627)))->to_ui32() == UINT32_C(1));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_78_wasm>", "[float_exprs_78_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.78.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_distribute_exact", bit_cast<float>(UINT32_C(2147483648)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_distribute_exact", bit_cast<double>(UINT64_C(9223372036854775808)))->to_f64()) == UINT64_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_79_wasm>", "[float_exprs_79_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.79.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.sqrt", bit_cast<float>(UINT32_C(1073741824)))->to_f32()) == UINT32_C(1068827891));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.xkcd_sqrt_2", bit_cast<float>(UINT32_C(1077936128)), bit_cast<float>(UINT32_C(1084227584)), bit_cast<float>(UINT32_C(1078530011)), bit_cast<float>(UINT32_C(1088421888)))->to_f32()) == UINT32_C(1068827946));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.sqrt", bit_cast<float>(UINT32_C(1077936128)))->to_f32()) == UINT32_C(1071494103));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.xkcd_sqrt_3", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(1076754516)), bit_cast<float>(UINT32_C(1078530011)))->to_f32()) == UINT32_C(1071481194));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.sqrt", bit_cast<float>(UINT32_C(1084227584)))->to_f32()) == UINT32_C(1074731965));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.xkcd_sqrt_5", bit_cast<float>(UINT32_C(1073741824)), bit_cast<float>(UINT32_C(1076754516)), bit_cast<float>(UINT32_C(1077936128)))->to_f32()) == UINT32_C(1074730668));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.xkcd_better_sqrt_5", bit_cast<float>(UINT32_C(1095761920)), bit_cast<float>(UINT32_C(1082130432)), bit_cast<float>(UINT32_C(1078530011)), bit_cast<float>(UINT32_C(1103101952)))->to_f32()) == UINT32_C(1074731965));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.sqrt", bit_cast<double>(UINT64_C(4611686018427387904)))->to_f64()) == UINT64_C(4609047870845172685));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.xkcd_sqrt_2", bit_cast<double>(UINT64_C(4613937818241073152)), bit_cast<double>(UINT64_C(4617315517961601024)), bit_cast<double>(UINT64_C(4614256656552045848)), bit_cast<double>(UINT64_C(4619567317775286272)))->to_f64()) == UINT64_C(4609047900099118431));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.sqrt", bit_cast<double>(UINT64_C(4613937818241073152)))->to_f64()) == UINT64_C(4610479282544200874));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.xkcd_sqrt_3", bit_cast<double>(UINT64_C(4611686018427387904)), bit_cast<double>(UINT64_C(4613303445314885481)), bit_cast<double>(UINT64_C(4614256656552045848)))->to_f64()) == UINT64_C(4610472352185749397));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.sqrt", bit_cast<double>(UINT64_C(4617315517961601024)))->to_f64()) == UINT64_C(4612217596255138984));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.xkcd_sqrt_5", bit_cast<double>(UINT64_C(4611686018427387904)), bit_cast<double>(UINT64_C(4613303445314885481)), bit_cast<double>(UINT64_C(4613937818241073152)))->to_f64()) == UINT64_C(4612216900234722254));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.xkcd_better_sqrt_5", bit_cast<double>(UINT64_C(4623507967449235456)), bit_cast<double>(UINT64_C(4616189618054758400)), bit_cast<double>(UINT64_C(4614256656552045848)), bit_cast<double>(UINT64_C(4627448617123184640)))->to_f64()) == UINT64_C(4612217595876713891));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_8_wasm>", "[float_exprs_8_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.8.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_one", bit_cast<float>(UINT32_C(2141192192)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_one", bit_cast<double>(UINT64_C(9219994337134247936)))));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_80_wasm>", "[float_exprs_80_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.80.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.compute_radix", bit_cast<float>(UINT32_C(1065353216)), bit_cast<float>(UINT32_C(1065353216)))->to_f32()) == UINT32_C(1073741824));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.compute_radix", bit_cast<double>(UINT64_C(4607182418800017408)), bit_cast<double>(UINT64_C(4607182418800017408)))->to_f64()) == UINT64_C(4611686018427387904));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_81_wasm>", "[float_exprs_81_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.81.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_sub1_mul_add", bit_cast<float>(UINT32_C(796917760)), bit_cast<float>(UINT32_C(1065353216)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_sub1_mul_add", bit_cast<double>(UINT64_C(4318952042648305664)), bit_cast<double>(UINT64_C(4607182418800017408)))->to_f64()) == UINT64_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_82_wasm>", "[float_exprs_82_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.82.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_le_monotonicity", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2143289344)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.no_fold_add_le_monotonicity", bit_cast<float>(UINT32_C(2139095040)), bit_cast<float>(UINT32_C(4286578688)), bit_cast<float>(UINT32_C(2139095040)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_le_monotonicity", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9221120237041090560)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.no_fold_add_le_monotonicity", bit_cast<double>(UINT64_C(9218868437227405312)), bit_cast<double>(UINT64_C(18442240474082181120)), bit_cast<double>(UINT64_C(9218868437227405312)))->to_ui32() == UINT32_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_83_wasm>", "[float_exprs_83_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.83.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "f32.not_lt", bit_cast<float>(UINT32_C(2143289344)), bit_cast<float>(UINT32_C(0)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.not_le", bit_cast<float>(UINT32_C(2143289344)), bit_cast<float>(UINT32_C(0)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.not_gt", bit_cast<float>(UINT32_C(2143289344)), bit_cast<float>(UINT32_C(0)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.not_ge", bit_cast<float>(UINT32_C(2143289344)), bit_cast<float>(UINT32_C(0)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.not_lt", bit_cast<double>(UINT64_C(9221120237041090560)), bit_cast<double>(UINT64_C(0)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.not_le", bit_cast<double>(UINT64_C(9221120237041090560)), bit_cast<double>(UINT64_C(0)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.not_gt", bit_cast<double>(UINT64_C(9221120237041090560)), bit_cast<double>(UINT64_C(0)))->to_ui32() == UINT32_C(1));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.not_ge", bit_cast<double>(UINT64_C(9221120237041090560)), bit_cast<double>(UINT64_C(0)))->to_ui32() == UINT32_C(1));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_84_wasm>", "[float_exprs_84_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.84.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.epsilon")->to_f32()) == UINT32_C(3019898880));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.epsilon")->to_f64()) == UINT64_C(4372995238176751616));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_85_wasm>", "[float_exprs_85_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.85.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.epsilon")->to_f32()) == UINT32_C(872415232));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.epsilon")->to_f64()) == UINT64_C(4372995238176751616));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_86_wasm>", "[float_exprs_86_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.86.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "f32.no_trichotomy_lt", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2143289344)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.no_trichotomy_le", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2143289344)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.no_trichotomy_gt", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2143289344)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.no_trichotomy_ge", bit_cast<float>(UINT32_C(0)), bit_cast<float>(UINT32_C(2143289344)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.no_trichotomy_lt", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9221120237041090560)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.no_trichotomy_le", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9221120237041090560)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.no_trichotomy_gt", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9221120237041090560)))->to_ui32() == UINT32_C(0));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.no_trichotomy_ge", bit_cast<double>(UINT64_C(0)), bit_cast<double>(UINT64_C(9221120237041090560)))->to_ui32() == UINT32_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_87_wasm>", "[float_exprs_87_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.87.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "f32.arithmetic_nan_bitpattern", UINT32_C(2139107856), UINT32_C(2139107856))->to_ui32() == UINT32_C(2143289344));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.canonical_nan_bitpattern", UINT32_C(0), UINT32_C(0))->to_ui32() == UINT32_C(2143289344));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.canonical_nan_bitpattern", UINT32_C(2143289344), UINT32_C(2143289344))->to_ui32() == UINT32_C(2143289344));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.canonical_nan_bitpattern", UINT32_C(4290772992), UINT32_C(2143289344))->to_ui32() == UINT32_C(2143289344));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.canonical_nan_bitpattern", UINT32_C(2143289344), UINT32_C(4290772992))->to_ui32() == UINT32_C(2143289344));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.canonical_nan_bitpattern", UINT32_C(4290772992), UINT32_C(4290772992))->to_ui32() == UINT32_C(2143289344));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.nonarithmetic_nan_bitpattern", UINT32_C(2143302160))->to_ui32() == UINT32_C(4290785808));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.nonarithmetic_nan_bitpattern", UINT32_C(4290785808))->to_ui32() == UINT32_C(2143302160));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.nonarithmetic_nan_bitpattern", UINT32_C(2139107856))->to_ui32() == UINT32_C(4286591504));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.nonarithmetic_nan_bitpattern", UINT32_C(4286591504))->to_ui32() == UINT32_C(2139107856));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.arithmetic_nan_bitpattern", UINT64_C(9218868437227418128), UINT64_C(9218868437227418128))->to_ui64() == UINT32_C(9221120237041090560));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.canonical_nan_bitpattern", UINT64_C(0), UINT64_C(0))->to_ui64() == UINT32_C(9221120237041090560));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.canonical_nan_bitpattern", UINT64_C(9221120237041090560), UINT64_C(9221120237041090560))->to_ui64() == UINT32_C(9221120237041090560));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.canonical_nan_bitpattern", UINT64_C(18444492273895866368), UINT64_C(9221120237041090560))->to_ui64() == UINT32_C(9221120237041090560));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.canonical_nan_bitpattern", UINT64_C(9221120237041090560), UINT64_C(18444492273895866368))->to_ui64() == UINT32_C(9221120237041090560));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.canonical_nan_bitpattern", UINT64_C(18444492273895866368), UINT64_C(18444492273895866368))->to_ui64() == UINT32_C(9221120237041090560));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.nonarithmetic_nan_bitpattern", UINT64_C(9221120237041103376))->to_ui64() == UINT32_C(18444492273895879184));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.nonarithmetic_nan_bitpattern", UINT64_C(18444492273895879184))->to_ui64() == UINT32_C(9221120237041103376));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.nonarithmetic_nan_bitpattern", UINT64_C(9218868437227418128))->to_ui64() == UINT32_C(18442240474082193936));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.nonarithmetic_nan_bitpattern", UINT64_C(18442240474082193936))->to_ui64() == UINT32_C(9218868437227418128));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.no_fold_sub_zero", UINT32_C(2141192192))->to_ui32() == UINT32_C(2143289344));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.no_fold_neg0_sub", UINT32_C(2141192192))->to_ui32() == UINT32_C(2143289344));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.no_fold_mul_one", UINT32_C(2141192192))->to_ui32() == UINT32_C(2143289344));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.no_fold_neg1_mul", UINT32_C(2141192192))->to_ui32() == UINT32_C(2143289344));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_one", UINT32_C(2141192192))->to_ui32() == UINT32_C(2143289344));
   CHECK(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_neg1", UINT32_C(2141192192))->to_ui32() == UINT32_C(2143289344));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.no_fold_sub_zero", UINT64_C(9219994337134247936))->to_ui64() == UINT32_C(9221120237041090560));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.no_fold_neg0_sub", UINT64_C(9219994337134247936))->to_ui64() == UINT32_C(9221120237041090560));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.no_fold_mul_one", UINT64_C(9219994337134247936))->to_ui64() == UINT32_C(9221120237041090560));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.no_fold_neg1_mul", UINT64_C(9219994337134247936))->to_ui64() == UINT32_C(9221120237041090560));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_one", UINT64_C(9219994337134247936))->to_ui64() == UINT32_C(9221120237041090560));
   CHECK(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_neg1", UINT64_C(9219994337134247936))->to_ui64() == UINT32_C(9221120237041090560));
   CHECK(bkend.call_with_return(nullptr, "env", "no_fold_promote_demote", UINT32_C(2141192192))->to_ui32() == UINT32_C(2143289344));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_88_wasm>", "[float_exprs_88_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.88.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "dot_product_example", bit_cast<double>(UINT64_C(4719355144821538816)), bit_cast<double>(UINT64_C(4607182418800017408)), bit_cast<double>(UINT64_C(13830554455654793216)), bit_cast<double>(UINT64_C(4725141118604279808)), bit_cast<double>(UINT64_C(4720637518976909312)), bit_cast<double>(UINT64_C(4607182418800017408)), bit_cast<double>(UINT64_C(13830554455654793216)), bit_cast<double>(UINT64_C(13938223582048944128)))->to_f64()) == UINT64_C(4611686018427387904));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "with_binary_sum_collapse", bit_cast<double>(UINT64_C(4719355144821538816)), bit_cast<double>(UINT64_C(4607182418800017408)), bit_cast<double>(UINT64_C(13830554455654793216)), bit_cast<double>(UINT64_C(4725141118604279808)), bit_cast<double>(UINT64_C(4720637518976909312)), bit_cast<double>(UINT64_C(4607182418800017408)), bit_cast<double>(UINT64_C(13830554455654793216)), bit_cast<double>(UINT64_C(13938223582048944128)))->to_f64()) == UINT64_C(4611686018427387904));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_89_wasm>", "[float_exprs_89_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.89.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.contract2fma", bit_cast<float>(UINT32_C(1065353216)), bit_cast<float>(UINT32_C(1065353216)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.contract2fma", bit_cast<float>(UINT32_C(1066192077)), bit_cast<float>(UINT32_C(1066192077)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.contract2fma", bit_cast<float>(UINT32_C(1067030937)), bit_cast<float>(UINT32_C(1067030937)))->to_f32()) == UINT32_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.contract2fma", bit_cast<double>(UINT64_C(4607182418800017408)), bit_cast<double>(UINT64_C(4607182418800017408)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.contract2fma", bit_cast<double>(UINT64_C(4607632778762754458)), bit_cast<double>(UINT64_C(4607632778762754458)))->to_f64()) == UINT64_C(0));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.contract2fma", bit_cast<double>(UINT64_C(4608083138725491507)), bit_cast<double>(UINT64_C(4608083138725491507)))->to_f64()) == UINT64_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_9_wasm>", "[float_exprs_9_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.9.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f32.no_fold_div_neg1", bit_cast<float>(UINT32_C(2141192192)))));
   CHECK(check_nan(bkend.call_with_return(nullptr, "env", "f64.no_fold_div_neg1", bit_cast<double>(UINT64_C(9219994337134247936)))));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_90_wasm>", "[float_exprs_90_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.90.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.division_by_small_number", bit_cast<float>(UINT32_C(1289068416)), bit_cast<float>(UINT32_C(1203982336)), bit_cast<float>(UINT32_C(980151802)))->to_f32()) == UINT32_C(1230570368));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.division_by_small_number", bit_cast<double>(UINT64_C(4727288602252279808)), bit_cast<double>(UINT64_C(4681608360884174848)), bit_cast<double>(UINT64_C(4561440258104740754)))->to_f64()) == UINT64_C(4695882709507797376));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_91_wasm>", "[float_exprs_91_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.91.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.golden_ratio", bit_cast<float>(UINT32_C(1056964608)), bit_cast<float>(UINT32_C(1065353216)), bit_cast<float>(UINT32_C(1084227584)))->to_f32()) == UINT32_C(1070537661));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.golden_ratio", bit_cast<double>(UINT64_C(4602678819172646912)), bit_cast<double>(UINT64_C(4607182418800017408)), bit_cast<double>(UINT64_C(4617315517961601024)))->to_f64()) == UINT64_C(4609965796441453736));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_92_wasm>", "[float_exprs_92_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.92.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.silver_means", bit_cast<float>(UINT32_C(0)))->to_f32()) == UINT32_C(1065353216));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.silver_means", bit_cast<float>(UINT32_C(1065353216)))->to_f32()) == UINT32_C(1070537661));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.silver_means", bit_cast<float>(UINT32_C(1073741824)))->to_f32()) == UINT32_C(1075479162));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.silver_means", bit_cast<float>(UINT32_C(1077936128)))->to_f32()) == UINT32_C(1079206061));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.silver_means", bit_cast<float>(UINT32_C(1082130432)))->to_f32()) == UINT32_C(1082625502));
   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.silver_means", bit_cast<float>(UINT32_C(1084227584)))->to_f32()) == UINT32_C(1084631458));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.silver_means", bit_cast<double>(UINT64_C(0)))->to_f64()) == UINT64_C(4607182418800017408));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.silver_means", bit_cast<double>(UINT64_C(4607182418800017408)))->to_f64()) == UINT64_C(4609965796441453736));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.silver_means", bit_cast<double>(UINT64_C(4611686018427387904)))->to_f64()) == UINT64_C(4612618744449965542));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.silver_means", bit_cast<double>(UINT64_C(4613937818241073152)))->to_f64()) == UINT64_C(4614619608365706490));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.silver_means", bit_cast<double>(UINT64_C(4616189618054758400)))->to_f64()) == UINT64_C(4616455406968633940));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.silver_means", bit_cast<double>(UINT64_C(4617315517961601024)))->to_f64()) == UINT64_C(4617532346471836922));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_93_wasm>", "[float_exprs_93_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.93.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bkend.call_with_return(nullptr, "env", "point_four", bit_cast<double>(UINT64_C(4616189618054758400)), bit_cast<double>(UINT64_C(4621819117588971520)))->to_ui32() == UINT32_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_94_wasm>", "[float_exprs_94_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.94.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "tau", UINT32_C(10))->to_f64()) == UINT64_C(4618760256179416340));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "tau", UINT32_C(11))->to_f64()) == UINT64_C(4618760256179416344));
}

BACKEND_TEST_CASE( "Testing wasm <float_exprs_95_wasm>", "[float_exprs_95_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "float_exprs.95.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f32.no_fold_conditional_inc", bit_cast<float>(UINT32_C(2147483648)), bit_cast<float>(UINT32_C(3212836864)))->to_f32()) == UINT32_C(2147483648));
   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f64.no_fold_conditional_inc", bit_cast<double>(UINT64_C(9223372036854775808)), bit_cast<double>(UINT64_C(13830554455654793216)))->to_f64()) == UINT64_C(9223372036854775808));
}

