#include <test_api.hpp>
#include <../core/print.hpp>
#include <../core/check.hpp>

void test_print::test_prints_l() {
  char ab[] = { 'a', 'b' };
  const char test[] = "test";
  internal_use_do_not_use::prints_l(ab, 2);
  internal_use_do_not_use::prints_l(ab, 1);
  internal_use_do_not_use::prints_l(ab, 0);
  internal_use_do_not_use::prints_l(test, sizeof(test)-1);
}

void test_print::test_prints() {
   internal_use_do_not_use::prints("ab");
   internal_use_do_not_use::prints("c\0test_prints");
   internal_use_do_not_use::prints("efg");
}

void test_print::test_printi() {
   internal_use_do_not_use::printi(0);
   internal_use_do_not_use::printi(556644);
   internal_use_do_not_use::printi(-1);
}

void test_print::test_printui() {
   internal_use_do_not_use::printui(0);
   internal_use_do_not_use::printui(556644);
   internal_use_do_not_use::printui((uint64_t)-1);
}

void test_print::test_printi128() {
  int128_t a(1);
  int128_t b(0);
  int128_t c(std::numeric_limits<int128_t>::lowest());
  int128_t d(-87654323456);
  internal_use_do_not_use::printi128(&a);
  internal_use_do_not_use::prints("\n");
  internal_use_do_not_use::printi128(&b);
  internal_use_do_not_use::prints("\n");
  internal_use_do_not_use::printi128(&c);
  internal_use_do_not_use::prints("\n");
  internal_use_do_not_use::printi128(&d);
  internal_use_do_not_use::prints("\n");
}

void test_print::test_printui128() {
  uint128_t a((uint128_t)-1);
  uint128_t b(0);
  uint128_t c(87654323456);
  internal_use_do_not_use::printui128(&a);
  internal_use_do_not_use::prints("\n");
  internal_use_do_not_use::printui128(&b);
  internal_use_do_not_use::prints("\n");
  internal_use_do_not_use::printui128(&c);
  internal_use_do_not_use::prints("\n");
}

void test_print::test_printn() {
   internal_use_do_not_use::printn(wasm::name{"1"}.value);
   internal_use_do_not_use::printn(wasm::name{"5"}.value);
   internal_use_do_not_use::printn(wasm::name{"a"}.value);
   internal_use_do_not_use::printn(wasm::name{"z"}.value);

   internal_use_do_not_use::printn(wasm::name{"abc"}.value);
   internal_use_do_not_use::printn(wasm::name{"123"}.value);

   internal_use_do_not_use::printn(wasm::name{"abc.123"}.value);
   internal_use_do_not_use::printn(wasm::name{"123.abc"}.value);

   internal_use_do_not_use::printn(wasm::name{"12345abcdefgj"}.value);
   internal_use_do_not_use::printn(wasm::name{"ijklmnopqrstj"}.value);
   internal_use_do_not_use::printn(wasm::name{"vwxyz.12345aj"}.value);

   internal_use_do_not_use::printn(wasm::name{"111111111111j"}.value);
   internal_use_do_not_use::printn(wasm::name{"555555555555j"}.value);
   internal_use_do_not_use::printn(wasm::name{"aaaaaaaaaaaaj"}.value);
   internal_use_do_not_use::printn(wasm::name{"zzzzzzzzzzzzj"}.value);
}


void test_print::test_printsf() {
   float x = 1.0f / 2.0f;
   float y = 5.0f * -0.75f;
   float z = 2e-6f / 3.0f;
   internal_use_do_not_use::printsf(x);
   internal_use_do_not_use::prints("\n");
   internal_use_do_not_use::printsf(y);
   internal_use_do_not_use::prints("\n");
   internal_use_do_not_use::printsf(z);
   internal_use_do_not_use::prints("\n");
}

void test_print::test_printdf() {
   double x = 1.0 / 2.0;
   double y = 5.0 * -0.75;
   double z = 2e-6 / 3.0;
   internal_use_do_not_use::printdf(x);
   internal_use_do_not_use::prints("\n");
   internal_use_do_not_use::printdf(y);
   internal_use_do_not_use::prints("\n");
   internal_use_do_not_use::printdf(z);
   internal_use_do_not_use::prints("\n");
}

void test_print::test_printqf() {
   long double x = 1.0l / 2.0l;
   long double y = 5.0l * -0.75l;
   long double z = 2e-6l / 3.0l;
   internal_use_do_not_use::printqf(&x);
   internal_use_do_not_use::prints("\n");
   internal_use_do_not_use::printqf(&y);
   internal_use_do_not_use::prints("\n");
   internal_use_do_not_use::printqf(&z);
   internal_use_do_not_use::prints("\n");
}

void test_print::test_print_simple() {
    const std::string cvalue = "cvalue";
    wasm::print(cvalue);
    std::string value = "value";
    wasm::print(std::move(value));
}