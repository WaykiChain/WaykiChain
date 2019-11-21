
#include <wasm.hpp>
#include <string>

struct dummy_action {
   static uint64_t get_name() {
      return name("dummyaction").value;
   }
   static uint64_t get_account() {
      return name("testapi").value;
   }

   char a; //1
   uint64_t b; //8
   int32_t  c; //4
};

static constexpr unsigned int DJBH(const char* cp)
{
	unsigned int hash = 5381;
	while (*cp)
		hash = 33 * hash ^ (unsigned char) *cp++;
	return hash;
}

static constexpr unsigned long long WASM_TEST_ACTION(const char* cls, const char* method)
{
	return static_cast<unsigned long long>(DJBH(cls)) << 32 | static_cast<unsigned long long>(DJBH(method));
}

#define WASM_TEST_HANDLER(CLASS, METHOD) \
	if( action == WASM_TEST_ACTION(#CLASS, #METHOD) ) { \
		CLASS::METHOD(); \
		return; \
	}

#define WASM_TEST_HANDLER_EX(CLASS, METHOD) \
  if( action == WASM_TEST_ACTION(#CLASS, #METHOD) ) { \
     CLASS::METHOD(receiver, code, action); \
     return; \
  }

struct test_print {
	static void test_prints();
	static void test_prints_l();
	static void test_printi();
	static void test_printui();
	static void test_printi128();
	static void test_printui128();
	static void test_printn();
	static void test_printsf();
	static void test_printdf();
	static void test_printqf();
	static void test_print_simple();
};

struct test_types {
   static void types_size();
   static void char_to_symbol();
   static void string_to_name();
};

struct test_datastream {
   static void test_basic();
};

struct test_action {
   static void read_action_normal();
   static void read_action_to_0();
   static void read_action_to_64k();
   // static void test_dummy_action();
   // static void test_cf_action();
   static void require_notice(uint64_t receiver, uint64_t code, uint64_t action);
   static void require_notice_tests(uint64_t receiver, uint64_t code, uint64_t action);
   // static void require_auth();
   static void assert_false();
   static void assert_true();
   // static void assert_true_cf();
   // static void test_current_time();
   static void test_abort() __attribute__ ((noreturn)) ;
   static void test_current_receiver(uint64_t receiver, uint64_t code, uint64_t action);
   // static void test_publication_time();
   // static void test_assert_code();
   // static void test_ram_billing_in_notify(uint64_t receiver, uint64_t code, uint64_t action);
};