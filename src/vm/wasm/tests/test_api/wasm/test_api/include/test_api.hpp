
#include <wasm.hpp>
#include <string>

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