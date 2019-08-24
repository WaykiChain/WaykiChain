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


config/build.h: FORCE
	@$(top_srcdir)/share/genbuild.sh $(abs_top_builddir)/src/config/build.h $(abs_top_srcdir)

config/version.o: config/build.h

.PHONY: FORCE
# waykichain core #
coin_CORE_H = \
  entities/account.h \
  entities/assett.h \
  entities/cdp.h \
  entities/contract.h \
  entities/dexorder.h \
  entities/id.h \
  entities/key.h \
  entities/keystore.h \
  entities/receipt.h \
  entities/vote.h \
  addrman.h \
  alert.h \
  allocators.h \
  base58.h \
  commons/arith_uint256.h \
  commons/bloom.h \
  commons/openssl.hpp \
  commons/serialize.h \
  commons/types.h \
  commons/util.h \
  config/configuration.h \
  config/const.h \
  config/errorcode.h \
  config/scoin.h \
  config/txbase.h \
  config/clientversion.h \
  config/version.h \
  config/chainparams.h \
  compat\compat.h \
  wallet/crypter.h \
  crypto/sha256.h \
  crypto/hash.h \
  init.h \
  limitedmap.h \
  main.h \
  p2p/chainmessage.h \
  miner/miner.h \
  mruset.h \
  netbase.h \
  net.h \
  persistence/assetdb.h \
  persistence/leveldbwrapper.h \
  persistence/accountdb.h \
  persistence/block.h \
  persistence/blockdb.h \
  persistence/cachewrapper.h \
  persistence/cdpdb.h \
  persistence/contractdb.h \
  persistence/delegatedb.h \
  persistence/txreceiptdb.h \
  persistence/disk.h \
  persistence/pricefeeddb.h \
  persistence/sysparamdb.h \
  persistence/txdb.h \
  persistence/dbaccess.h \
  persistence/dbconf.h \
  persistence/dbiterator.h \
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
  tx/assettx.h \
  tx/accountregtx.h \
  tx/blockrewardtx.h \
  tx/blockpricemediantx.h \
  tx/cdptx.h \
  tx/coinrewardtx.h \
  tx/contracttx.h \
  tx/cointransfertx.h \
  tx/delegatetx.h \
  tx/dextx.h \
  tx/fcoinstaketx.h \
  tx/merkletx.h \
  tx/mulsigtx.h \
  tx/pricefeedtx.h \
  tx/tx.h \
  tx/txmempool.h \
  sync.h \
  threadsafety.h \
  tinyformat.h \
  uint256.h \
  wallet/wallet.h \
  wallet/db.h

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
  vm/luavm/lua/fuel.h \
  vm/luavm/lua/lapi.h \
  vm/luavm/lua/lauxlib.h \
  vm/luavm/lua/lburner.h \
  vm/luavm/lua/lcode.h \
  vm/luavm/lua/lctype.h \
  vm/luavm/lua/ldebug.h \
  vm/luavm/lua/ldo.h \
  vm/luavm/lua/lfunc.h \
  vm/luavm/lua/lgc.h \
  vm/luavm/lua/llex.h \
  vm/luavm/lua/llimits.h \
  vm/luavm/lua/lmem.h \
  vm/luavm/lua/lobject.h \
  vm/luavm/lua/lopcodes.h \
  vm/luavm/lua/lparser.h \
  vm/luavm/lua/lprefix.h \
  vm/luavm/lua/lstate.h \
  vm/luavm/lua/lstring.h \
  vm/luavm/lua/ltable.h \
  vm/luavm/lua/ltm.h \
  vm/luavm/lua/lua.h \
  vm/luavm/lua/lua.hpp \
  vm/luavm/lua/luaconf.h \
  vm/luavm/lua/lualib.h \
  vm/luavm/lua/lundump.h \
  vm/luavm/lua/lvm.h \
  vm/luavm/lua/lzio.h

VMLUA_C = \
  vm/luavm/lua/lapi.c \
  vm/luavm/lua/lburner.c \
  vm/luavm/lua/lauxlib.c \
  vm/luavm/lua/lbaselib.c \
  vm/luavm/lua/lbitlib.c \
  vm/luavm/lua/lcode.c \
  vm/luavm/lua/lcorolib.c \
  vm/luavm/lua/lctype.c \
  vm/luavm/lua/ldblib.c \
  vm/luavm/lua/ldebug.c \
  vm/luavm/lua/ldo.c \
  vm/luavm/lua/ldump.c \
  vm/luavm/lua/lfunc.c \
  vm/luavm/lua/lgc.c \
  vm/luavm/lua/linit.c \
  vm/luavm/lua/liolib.c \
  vm/luavm/lua/llex.c \
  vm/luavm/lua/lmathlib.c \
  vm/luavm/lua/lmem.c \
  vm/luavm/lua/loadlib.c \
  vm/luavm/lua/lobject.c \
  vm/luavm/lua/lopcodes.c \
  vm/luavm/lua/loslib.c \
  vm/luavm/lua/lparser.c \
  vm/luavm/lua/lstate.c \
  vm/luavm/lua/lstring.c \
  vm/luavm/lua/lstrlib.c \
  vm/luavm/lua/ltable.c \
  vm/luavm/lua/ltablib.c \
  vm/luavm/lua/ltm.c \
  vm/luavm/lua/lundump.c \
  vm/luavm/lua/lutf8lib.c \
  vm/luavm/lua/lvm.c \
  vm/luavm/lua/lzio.c

VM_H = \
  vm/luavm/vmrunenv.h \
  vm/luavm/appaccount.h \
  vm/luavm/vmlua.h


VM_CPP = \
  vm/luavm/vmrunenv.cpp \
  vm/luavm/appaccount.cpp \
  vm/luavm/lmylib.cpp \
  vm/luavm/vmlua.cpp

liblua53_a_SOURCES = \
  $(VMLUA_C)

libcoin_server_a_CPPFLAGS = $(AM_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
libcoin_server_a_SOURCES = \
  entities/account.cpp \
  entities/cdp.cpp \
  entities/contract.cpp \
  entities/dexorder.cpp \
  entities/id.cpp \
  entities/key.cpp \
  entities/keystore.cpp \
  addrman.cpp \
  alert.cpp \
  config/configuration.cpp \
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
  tx/assettx.cpp \
  tx/accountregtx.cpp \
  tx/blockrewardtx.cpp \
  tx/blockpricemediantx.cpp \
  tx/cdptx.cpp \
  tx/coinrewardtx.cpp \
  tx/contracttx.cpp \
  tx/cointransfertx.cpp \
  tx/delegatetx.cpp \
  tx/dextx.cpp \
  tx/fcoinstaketx.cpp \
  tx/mulsigtx.cpp \
  tx/pricefeedtx.cpp \
  tx/tx.cpp \
  tx/txmempool.cpp \
  $(VMLUA_H) \
  $(VM_CPP) \
  $(VM_H) \
  $(JSON_H) \
  $(COIN_CORE_H)

libcoin_wallet_a_SOURCES = \
  entities/keystore.cpp \
  rpc/rpcdump.cpp \
  rpc/rpcwallet.cpp \
  rpc/rpctx.cpp \
  wallet/crypter.cpp \
  wallet/db.cpp  \
  wallet/wallet.cpp \
  wallet/walletdb.cpp \
  $(COIN_CORE_H)

libcoin_common_a_SOURCES = \
  entities/key.cpp \
  commons/base58.cpp \
  commons/allocators.cpp \
  commons/arith_uint256.cpp \
  commons/random.cpp  \
  commons/uint256.cpp \
  commons/bloom.cpp \
  commons/util.cpp \
  crypto/hash.cpp \
  config/chainparams.cpp \
  config/configuration.cpp \
  config/version.cpp \
  netbase.cpp \
  protocol.cpp \
  persistence/block.cpp \
  persistence/cdpdb.cpp \
  persistence/accountdb.cpp \
  persistence/assetdb.cpp \
  persistence/blockdb.cpp \
  persistence/cachewrapper.cpp \
  persistence/contractdb.cpp \
  persistence/delegatedb.cpp \
  persistence/txreceiptdb.cpp \
  persistence/pricefeeddb.cpp \
  persistence/txdb.cpp \
  persistence/leveldbwrapper.cpp \
  persistence/dexdb.cpp \
  persistence/logdb.cpp \
  support/cleanse.cpp \
  support/events.cpp \
  sync.cpp \
  tx/coinrewardtx.cpp \
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

nodist_libcoin_common_a_SOURCES = $(top_srcdir)/src/config/build.h

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
coinlua_SOURCES = vm/luavm/lua/coinlua.c
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

DISTCLEANFILES = config/build.h

EXTRA_DIST = leveldb Makefile.include

clean-local:
	-$(MAKE) -C secp256k1 clean
	-$(MAKE) -C leveldb clean
	rm -f leveldb/*/*.gcno leveldb/helpers/memenv/*.gcno
