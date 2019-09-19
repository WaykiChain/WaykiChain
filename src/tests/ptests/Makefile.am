include $(top_srcdir)/src/Makefile.include

AM_CPPFLAGS += -I$(top_srcdir)/src

bin_PROGRAMS = coin_ptest

TESTS = test

# test_bitcoin binary #
coin_ptest_CPPFLAGS = $(AM_CPPFLAGS) $(TESTDEFS)
coin_ptest_LDADD = \
			   $(LIBBITCOIN_SERVER) \
               $(LIBBITCOIN_WALLET)   \
			   $(LIBBITCOIN_CLI) \
			   $(LIBBITCOIN_COMMON) \
			   $(LIBLUA53) \
			   $(LIBLEVELDB) \
			   $(LIBMEMENV) \
               $(BOOST_LIBS) \
			   $(BOOST_UNIT_TEST_FRAMEWORK_LIB)
#if ENABLE_WALLET
#coin_ptest_LDADD += $(LIBBITCOIN_WALLET)
#endif
coin_ptest_LDADD += $(BDB_LIBS)

coin_ptest_SOURCES = \
  P_test.cpp \
  Main_tests.cpp \
  UpdateAccount_tests.cpp \
  Mining_tests.cpp	\
  CreateTx_tests.cpp \
  CBlackHalo_tests.cpp	\
  CAnony_tests.cpp     \
  ../test/systestbase.cpp \
  CycleSesureTrade_tests.cpp	\
  CycleTestBase.cpp	\
  CycleTestManger.cpp \
  CycleTestManger.h	\
  CycleTestBase.h	\
  CycleSesureTrade_tests.h	\
  CBlackHalo_tests.h	\
  CAnony_tests.h   \
  CreateTx_tests.h  \
  ../test/systestbase.h \
  CycleP2PBet_tests.cpp \
  CycleP2PBet_tests.h \
  CreateMinterKey_tests.cpp \
  CreateMinterKey_tests.h  \
  CIpo_tests.h \
  CIpo_tests.cpp \
  CRedPacket_tests.cpp \
  CGuarantee_tests.h \
  CGuarantee_tests.cpp \
  Delegatetx_tests.cpp 
  