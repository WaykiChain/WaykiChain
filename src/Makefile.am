include Makefile.include

AM_CPPFLAGS += -I$(builddir)

noinst_LIBRARIES = \
  liblua53.a \
  libcoin_server.a \
  libcoin_common.a \
  libcoin_cli.a
if ENABLE_WALLET
noinst_LIBRARIES += libcoin_wallet.a
endif

bin_PROGRAMS =

if BUILD_BITCOIND
  bin_PROGRAMS += coind
endif

if BUILD_COINLUA
  bin_PROGRAMS += coinlua
endif

SUBDIRS = . $(BUILD_QT) $(BUILD_P_TEST) $(BUILD_TEST)
#DIST_SUBDIRS = . qt test
DIST_SUBDIRS = . test ptest

#secp256k1
DIST_SUBDIRS += secp256k1
LIBSECP256K1=secp256k1/libsecp256k1.la
AM_CPPFLAGS += -I$(srcdir)/secp256k1/include -I$(srcdir)/secp256k1/contrib

$(LIBSECP256K1): $(wildcard secp256k1/src/*.h) $(wildcard secp256k1/src/*.c) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

.PHONY: FORCE
# waykichain core #
coin_CORE_H = \
  accounts/account.h \
  accounts/cdp.h \
  accounts/id.h \
  accounts/key.h \
  accounts/keystore.h \
  accounts/vote.h \
  addrman.h \
  alert.h \
  allocators.h \
  base58.h \
  commons/bloom.h \
  commons/openssl.hpp \
  commons/serialize.h \
  chainparams.h \
  clientversion.h \
  compat\compat.h \
  core.h \
  database.h \
  crypter.h \
  crypto/sha256.h \
  crypto/hash.h \
  init.h \
  leveldbwrapper.h \
  limitedmap.h \
  main.h \
  miner/miner.h \
  mruset.h \
  netbase.h \
  net.h \
  noui.h \
  persistence/accountdb.h \
  persistence/blockdb.h \
  persistence/contractdb.h \
  protocol.h \
  random.h   \
  rpc/httpserver.h \
  rpc/rpcclient.h \
  rpc/rpcprotocol.h \
  rpc/rpcserver.h \
  rpc/rpcvm.h \
  support/cleanse.h \
  sigcache.h \
  tx/accountreg.h \
  tx/blockreward.h \
  tx/bcoin.h \
  tx/contract.h \
  tx/delegate.h \
  tx/fcoin.h \
  tx/pricefeed.h \
  tx/scoin.h \
  tx/tx.h \
  tx/txmempool.h \
  sync.h \
  threadsafety.h \
  tinyformat.h \
  ui_interface.h \
  uint256.h \
  util.h \
  version.h \
  wallet/wallet.h \
  wallet/db.h \
  arith_uint256.h \
  configuration.h

JSON_H = \
  json/json_spirit.h \
  json/json_spirit_error_position.h \
  json/json_spirit_reader.h \
  json/json_spirit_reader_template.h \
  json/json_spirit_stream_reader.h \
  json/json_spirit_utils.h \
  json/json_spirit_value.h \
  json/json_spirit_writer.h \
  json/json_spirit_writer_template.h

# VmScript #
VMLUA_H = \
  vm/lua/lapi.h \
  vm/lua/lauxlib.h \
  vm/lua/lburner.h \
  vm/lua/lcode.h \
  vm/lua/lctype.h \
  vm/lua/ldebug.h \
  vm/lua/ldo.h \
  vm/lua/lfunc.h \
  vm/lua/lgc.h \
  vm/lua/llex.h \
  vm/lua/llimits.h \
  vm/lua/lmem.h \
  vm/lua/lobject.h \
  vm/lua/lopcodes.h \
  vm/lua/lparser.h \
  vm/lua/lprefix.h \
  vm/lua/lstate.h \
  vm/lua/lstring.h \
  vm/lua/ltable.h \
  vm/lua/ltm.h \
  vm/lua/lua.h \
  vm/lua/lua.hpp \
  vm/lua/luaconf.h \
  vm/lua/lualib.h \
  vm/lua/lundump.h \
  vm/lua/lvm.h \
  vm/lua/lzio.h

VMLUA_C = \
  vm/lua/lapi.c \
  vm/lua/lburner.c \
  vm/lua/lauxlib.c \
  vm/lua/lbaselib.c \
  vm/lua/lbitlib.c \
  vm/lua/lcode.c \
  vm/lua/lcorolib.c \
  vm/lua/lctype.c \
  vm/lua/ldblib.c \
  vm/lua/ldebug.c \
  vm/lua/ldo.c \
  vm/lua/ldump.c \
  vm/lua/lfunc.c \
  vm/lua/lgc.c \
  vm/lua/linit.c \
  vm/lua/liolib.c \
  vm/lua/llex.c \
  vm/lua/lmathlib.c \
  vm/lua/lmem.c \
  vm/lua/loadlib.c \
  vm/lua/lobject.c \
  vm/lua/lopcodes.c \
  vm/lua/loslib.c \
  vm/lua/lparser.c \
  vm/lua/lstate.c \
  vm/lua/lstring.c \
  vm/lua/lstrlib.c \
  vm/lua/ltable.c \
  vm/lua/ltablib.c \
  vm/lua/ltm.c \
  vm/lua/lundump.c \
  vm/lua/lutf8lib.c \
  vm/lua/lvm.c \
  vm/lua/lzio.c

VM_H = \
  vm/vmrunenv.h \
  vm/script.h \
  vm/appaccount.h \
  vm/vmlua.h


VM_CPP = \
  vm/vmrunenv.cpp \
  vm/script.cpp \
  vm/appaccount.cpp \
  vm/lmylib.cpp \
  vm/vmlua.cpp

obj/build.h: FORCE
	@$(MKDIR_P) $(abs_top_builddir)/src/obj
	@$(top_srcdir)/share/genbuild.sh $(abs_top_builddir)/src/obj/build.h \
	  $(abs_top_srcdir)
version.o: obj/build.h

liblua53_a_SOURCES = \
  $(VMLUA_C)

libcoin_server_a_CPPFLAGS = $(AM_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
libcoin_server_a_SOURCES = \
  accounts/account.cpp \
  accounts/id.cpp \
  accounts/cdp.cpp \
  addrman.cpp \
  alert.cpp \
  configuration.cpp \
  crypto/sha256.cpp \
  cuiserver.cpp	\
  database.cpp \
  init.cpp \
  leveldbwrapper.cpp \
  main.cpp \
  miner/miner.cpp \
  net.cpp \
  noui.cpp \
  persistence/accountdb.cpp \
  persistence/blockdb.cpp \
  persistence/contractdb.cpp \
  rpc/httpserver.cpp \
  rpc/rpcblockchain.cpp \
  rpc/rpcmining.cpp \
  rpc/rpcmisc.cpp \
  rpc/rpcnet.cpp \
  rpc/rpcrawtransaction.cpp \
  rpc/rpcserver.cpp \
  rpc/rpcvm.cpp \
  sigcache.cpp \
  tx/accountreg.cpp \
  tx/blockreward.cpp \
  tx/bcoin.cpp \
  tx/contract.cpp \
  tx/delegate.cpp \
  tx/fcoin.cpp \
  tx/pricefeed.cpp \
  tx/scoin.cpp \
  tx/tx.cpp \
  tx/txmempool.cpp \
  $(VMLUA_H) \
  $(VM_CPP) \
  $(VM_H) \
  $(JSON_H) \
  $(COIN_CORE_H)

libcoin_wallet_a_SOURCES = \
  accounts/keystore.cpp \
  crypter.cpp \
  rpc/rpcdump.cpp \
  rpc/rpcwallet.cpp \
  rpc/rpctx.cpp \
  wallet/db.cpp  \
  wallet/wallet.cpp \
  wallet/walletdb.cpp \
  $(COIN_CORE_H)

libcoin_common_a_SOURCES = \
  accounts/key.cpp \
  commons/base58.cpp \
  commons/allocators.cpp \
  commons/arith_uint256.cpp \
  commons/random.cpp  \
  commons/uint256.cpp \
  commons/bloom.cpp \
  crypto/hash.cpp \
  chainparams.cpp \
  configuration.cpp\
  core.cpp \
  netbase.cpp \
  protocol.cpp \
  rpc/rpcprotocol.cpp \
  support/cleanse.cpp \
  support/events.cpp \
  sync.cpp \
  syncdata.cpp \
  syncdatadb.cpp \
  util.cpp \
  version.cpp \
  json/json_spirit_reader.cpp \
  json/json_spirit_value.cpp \
  json/json_spirit_writer.cpp \
  $(COIN_CORE_H)

if GLIBC_BACK_COMPAT
libcoin_common_a_SOURCES += compat/glibc_compat.cpp
libcoin_common_a_SOURCES += compat/glibcxx_compat.cpp
endif

libcoin_cli_a_SOURCES = \
  rpc/rpcclient.cpp \
  $(COIN_CORE_H)

nodist_libcoin_common_a_SOURCES = $(top_srcdir)/src/obj/build.h
#

# coin binary #
coind_LDADD = \
  libcoin_server.a \
  libcoin_wallet.a \
  libcoin_cli.a \
  libcoin_common.a \
  liblua53.a \
  $(LIBLEVELDB) \
  $(LIBMEMENV) \
  $(LIBSECP256K1)
#if ENABLE_WALLET
#coin_LDADD += libcoin_wallet.a
#endif
coind_SOURCES = coind.cpp

if TARGET_WINDOWS
coind_SOURCES += coind-res.rc
endif

liblua53_a_CFLAGS = -fPIC -DLUA_USE_POSIX -Wl,-E

AM_CPPFLAGS += $(BDB_CPPFLAGS)
coind_LDADD += $(BOOST_LIBS) $(BDB_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS)

# coinlua binary
coinlua_LDADD = liblua53.a -lm
coinlua_SOURCES = vm/lua/coinlua.c
coinlua_CFLAGS = $(AM_CFLAGS) -fPIC -DLUA_USE_POSIX -Wl,-E -ldl

# NOTE: This dependency is not strictly necessary, but without it make may try to build both in parallel, which breaks the LevelDB build system in a race
$(LIBLEVELDB): $(LIBMEMENV)

leveldb/%.a:
	@echo "Building LevelDB ..." && $(MAKE) -C $(@D) $(@F) CXX="$(CXX)" \
	  CC="$(CC)" PLATFORM=$(TARGET_OS) AR="$(AR)" $(LEVELDB_TARGET_FLAGS) \
	  OPT="$(CXXFLAGS) $(CPPFLAGS)"

qt/bitcoinstrings.cpp: $(libcoin_server_a_SOURCES) $(libcoin_common_a_SOURCES) $(libcoin_cli_a_SOURCES)
	@test -n $(XGETTEXT) || echo "xgettext is required for updating translations"
	@cd $(top_srcdir); XGETTEXT=$(XGETTEXT) share/qt/extract_strings_qt.py

CLEANFILES = leveldb/libleveldb.a leveldb/libmemenv.a *.gcda *.gcno

DISTCLEANFILES = obj/build.h

EXTRA_DIST = leveldb Makefile.include

clean-local:
	-$(MAKE) -C secp256k1 clean
	-$(MAKE) -C leveldb clean
	rm -f leveldb/*/*.gcno leveldb/helpers/memenv/*.gcno
