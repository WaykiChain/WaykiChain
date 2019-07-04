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

SUBDIRS = . $(BUILD_QT) $(BUILD_P_TEST)
#DIST_SUBDIRS = . qt test
#DIST_SUBDIRS = . test ptest

#secp256k1
DIST_SUBDIRS = secp256k1
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
  commons/arith_uint256.h \
  commons/bloom.h \
  commons/openssl.hpp \
  commons/serialize.h \
  config/scoin.h \
  chainparams.h \
  clientversion.h \
  compat\compat.h \
  crypter.h \
  crypto/sha256.h \
  crypto/hash.h \
  init.h \
  limitedmap.h \
  main.h \
  miner/miner.h \
  mruset.h \
  netbase.h \
  net.h \
  persistence/leveldbwrapper.h \
  persistence/accountdb.h \
  persistence/block.h \
  persistence/blockdb.h \
  persistence/cdpdb.h \
  persistence/contractdb.h \
  persistence/delegatedb.h \
  persistence/disk.h \
  persistence/pricefeeddb.h \
  persistence/txdb.h \
  persistence/dbaccess.h \
  persistence/dbconf.h \
  persistence/dexdb.h \
  persistence/logdb.h \
  protocol.h \
  random.h   \
  rpc/core/httpserver.h \
  rpc/core/rpcclient.h \
  rpc/core/rpccommons.h \
  rpc/core/rpcprotocol.h \
  rpc/core/rpcserver.h \
  rpc/rpcblockchain.h \
  rpc/rpcdump.h \
  rpc/rpcmining.h \
  rpc/rpcmisc.h \
  rpc/rpcnet.h \
  rpc/rpcscoin.h \
  rpc/rpctx.h \
  rpc/rpcvm.h \
  rpc/rpcwallet.h \
  support/cleanse.h \
  sigcache.h \
  tx/accountregtx.h \
  tx/blockrewardtx.h \
  tx/blockpricemediantx.h \
  tx/bcointx.h \
  tx/cdptx.h \
  tx/coinrewardtx.h \
  tx/contracttx.h \
  tx/cointransfertx.h \
  tx/delegatetx.h \
  tx/dextx.h \
  tx/fcoinstaketx.h \
  tx/merkletx.h \
  tx/mulsigtx.h \
  tx/multicoinblockrewardtx.h \
  tx/pricefeedtx.h \
  tx/tx.h \
  tx/txmempool.h \
  sync.h \
  threadsafety.h \
  tinyformat.h \
  uint256.h \
  util.h \
  version.h \
  wallet/wallet.h \
  wallet/db.h \
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
  vm/lua/fuel.h \
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
  init.cpp \
  main.cpp \
  miner/miner.cpp \
  net.cpp \
  rpc/core/httpserver.cpp \
  rpc/core/rpcclient.cpp \
  rpc/core/rpccommons.cpp \
  rpc/core/rpcprotocol.cpp \
  rpc/core/rpcserver.cpp \
  rpc/rpcblockchain.cpp \
  rpc/rpcdump.cpp \
  rpc/rpcmining.cpp \
  rpc/rpcmisc.cpp \
  rpc/rpcnet.cpp \
  rpc/rpcscoin.cpp \
  rpc/rpctx.cpp \
  rpc/rpcvm.cpp \
  rpc/rpcwallet.cpp \
  sigcache.cpp \
  tx/accountregtx.cpp \
  tx/blockrewardtx.cpp \
  tx/blockpricemediantx.cpp \
  tx/bcointx.cpp \
  tx/cdptx.cpp \
  tx/coinrewardtx.cpp \
  tx/contracttx.cpp \
  tx/cointransfertx.cpp \
  tx/delegatetx.cpp \
  tx/dextx.cpp \
  tx/fcoinstaketx.cpp \
  tx/mulsigtx.cpp \
  tx/multicoinblockrewardtx.cpp \
  tx/pricefeedtx.cpp \
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
  netbase.cpp \
  protocol.cpp \
  persistence/block.cpp \
  persistence/cdpdb.cpp \
  persistence/accountdb.cpp \
  persistence/blockdb.cpp \
  persistence/contractdb.cpp \
  persistence/delegatedb.cpp \
  persistence/pricefeeddb.cpp \
  persistence/txdb.cpp \
  persistence/leveldbwrapper.cpp \
  persistence/dexdb.cpp \
  persistence/logdb.cpp \
  support/cleanse.cpp \
  support/events.cpp \
  sync.cpp \
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
  rpc/core/rpcclient.cpp \
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

if BUILD_TESTS
include Makefile_tests.am
endif

if BUILD_UNIT_TESTS
include Makefile_unit_tests.am
endif

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
