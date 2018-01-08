include $(top_srcdir)/src/Makefile.include

AM_CPPFLAGS += -I$(top_srcdir)/src

bin_PROGRAMS = coin_test

TESTS = test

JSON_TEST_FILES = \
  data/script_valid.json.h \
  data/base58_keys_valid.json.h \
  data/sig_canonical.json.h \
  data/sig_noncanonical.json.h \
  data/base58_encode_decode.json.h \
  data/base58_keys_invalid.json.h \
  data/script_invalid.json.h \
  data/tx_invalid.json.h \
  data/tx_valid.json.h \
  data/sighash.json.h	\
  systestbase.h \
RAW_TEST_FILES = data/alertTests.raw.h

#BUILT_SOURCES = $(JSON_TEST_FILES:.json=.json.h) $(RAW_TEST_FILES:.raw=.raw.h)

# test_dspay binary #
coin_test_CPPFLAGS = $(AM_CPPFLAGS) $(TESTDEFS)
coin_test_LDADD = $(LIBBITCOIN_SERVER) $(LIBBITCOIN_WALLET) $(LIBBITCOIN_CLI) $(LIBBITCOIN_COMMON) $(LIBLUA53) $(LIBLEVELDB) $(LIBMEMENV) \
  $(BOOST_LIBS) $(BOOST_UNIT_TEST_FRAMEWORK_LIB)
#if ENABLE_WALLET
#coin_test_LDADD += $(LIBBITCOIN_WALLET)
#endif
coin_test_LDADD += $(BDB_LIBS)

coin_test_SOURCES = \
  allocator_tests.cpp \
  base32_tests.cpp \
  base58_tests.cpp \
  base64_tests.cpp \
  bignum_tests.cpp \
  bloom_tests.cpp \
  canonical_tests.cpp \
  checkblock_tests.cpp \
  Checkpoints_tests.cpp \
  DoS_tests.cpp \
  key_tests.cpp \
  main_tests.cpp \
  mruset_tests.cpp \
  multisig_tests.cpp \
  netbase_tests.cpp \
  serialize_tests.cpp \
  sigopcount_tests.cpp \
  test_coin.cpp \
  uint256_tests.cpp \
  util_tests.cpp \
  sighash_tests.cpp \
  chainparams_tests.cpp \
  accountview_tests.cpp \
  scriptdb_tests.cpp \
  betroll_test.cpp \
  system_test.cpp	\
  systestbase.cpp	\
  systestscript.cpp \
  systemacct_test.cpp \
  pressure_tests.cpp \
  scriptdbex_tests.cpp	\
  account_tests.cpp \
  appacc_test.cpp \
  $(JSON_TEST_FILES) $(RAW_TEST_FILES)

#if ENABLE_WALLET
#coin_test_SOURCES += 
#endif

#nodist_coin_test_SOURCES = $(BUILT_SOURCES)

#CLEANFILES = *.gcda *.gcno $(BUILT_SOURCES)
CLEANFILES = *.gcda *.gcno
