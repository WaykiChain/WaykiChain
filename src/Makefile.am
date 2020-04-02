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
DIST_SUBDIRS = crypto/secp256k1
LIBSECP256K1 = crypto/secp256k1/libsecp256k1.la
AM_CPPFLAGS += -I$(srcdir)/crypto/secp256k1/include -I$(srcdir)/crypto/secp256k1/contrib

$(LIBSECP256K1): $(wildcard secp256k1/src/*.h) $(wildcard secp256k1/src/*.c) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)


# softloat
include Makefile_softfloat

#wasm
include Makefile_wasm

VM_PATH = $(srcdir)/vm
WASM_PATH = $(VM_PATH)/wasm

# WASM_INTERFACE_SRC = \
# $(WASM_PATH)/wasminterface.cpp \
# $(WASM_PATH)/wasminterface.hpp \
# $(WASM_PATH)/wasmhostmethods.hpp \
# $(WASM_PATH)/wasmcontextinterface.hpp

# $(WASMLIB): $(WASM_INTERFACE_SRC)
#   $(MAKE) -C $(srcdir)/vm/wasm


WASM_VM_INCLUDE = $(WASM_PATH)/eos-vm/include
WASM_COMPILER_BUILTINS = $(WASM_PATH)/eos-vm/external/compiler_builtins
WASM_BERKELEY_SOFTFLOAT = $(srcdir)/external/softfloat/source/include
WASM_EXCEPTION_INCLUDE = $(WASM_PATH)/exception

WASM_OUTCOME_DIR = $(srcdir)/external/outcome/single-header

WASM_CPPFLAGS = -std=gnu++1z -I$(VM_PATH) \
                           -I$(WASM_PATH) \
                           -I$(WASM_VM_INCLUDE) \
                           -I$(WASM_OUTCOME_DIR) \
                           -I$(WASM_COMPILER_BUILTINS) \
                           -I$(WASM_BERKELEY_SOFTFLOAT) \
                           -I$(WASM_EXCEPTION_INCLUDE)

AM_CPPFLAGS +=   -I$(VM_PATH)

config/build.h: FORCE
	@$(top_srcdir)/share/genbuild.sh $(abs_top_builddir)/src/config/build.h $(abs_top_srcdir)

config/version.o: config/build.h

.PHONY: FORCE
# waykichain core #
coin_CORE_H = \
  chain/blockdelegates.h \
  chain/chain.h \
  chain/merkletree.h \
  entities/account.h \
  entities/asset.h \
  entities/cdp.h \
  entities/contract.h \
  entities/dexorder.h \
  entities/id.h \
  entities/key.h \
  entities/keystore.h \
  entities/price.h \
  entities/receipt.h \
  entities/proposal.h \
  entities/utxo.h \
  entities/vote.h \
  alert.h \
  allocators.h \
  base58.h \
  commons/arith_uint256.h \
  commons/bloom.h \
  commons/openssl.hpp \
  commons/serialize.h \
  commons/leb128.h \
  commons/types.h \
  commons/util/enumhelper.hpp \
  commons/util/util.h \
  commons/util/threadnames.h \
  commons/util/time.h \
  commons/compat/byteswap.h \
  commons/compat/compat.h \
  commons/compat/endian.h \
  config/configuration.h \
  config/const.h \
  config/errorcode.h \
  config/scoin.h \
  config/sysparams.h \
  config/txbase.h \
  config/clientversion.h \
  config/version.h \
  config/chainparams.h \
  wallet/crypter.h \
  crypto/sha256.h \
  crypto/hash.h \
  fs.h \
  init.h \
  limitedmap.h \
  main.h \
  p2p/addrman.h \
  p2p/chainmessage.h \
  p2p/protocol.h \
  p2p/node.h \
  p2p/netmessage.h \
  miner/miner.h \
  miner/pbftcontext.h \
  miner/pbftmanager.h \
  mruset.h \
  netbase.h \
  net.h \
  nodeinfo.h \
  persistence/assetdb.h \
  persistence/axcdb.h \
  persistence/leveldbwrapper.h \
  persistence/accountdb.h \
  persistence/block.h \
  persistence/blockdb.h \
  persistence/blockundo.h \
  persistence/cachewrapper.h \
  persistence/cdpdb.h \
  persistence/contractdb.h \
  persistence/dbaccess.h \
  persistence/dbconf.h \
  persistence/dbiterator.h \
  persistence/dexdb.h \
  persistence/delegatedb.h \
  persistence/txreceiptdb.h \
  persistence/disk.h \
  persistence/pricefeeddb.h \
  persistence/txdb.h \
  persistence/logdb.h \
  persistence/sysgoverndb.h \
  persistence/sysparamdb.h \
  persistence/txutxodb.h \
  random.h   \
  rpc/core/httpserver.h \
  rpc/core/rpcclient.h \
  rpc/core/rpccommons.h \
  rpc/core/rpcprotocol.h \
  rpc/core/rpcserver.h \
  rpc/rpcblockchain.h \
  rpc/rpcdump.h \
  rpc/rpcapi.h \
  rpc/rpcapiconf.h \
  rpc/rpcmining.h \
  rpc/rpcmisc.h \
  rpc/rpcnet.h \
  rpc/rpcvm.h \
  rpc/rpcwallet.h \
  commons/support/cleanse.h \
  sigcache.h \
  tx/assettx.h \
  tx/accountregtx.h \
  tx/accountpermscleartx.h \
  tx/blockrewardtx.h \
  tx/blockpricemediantx.h \
  tx/cdptx.h \
  tx/coinminttx.h \
  tx/contracttx.h \
  tx/cointransfertx.h \
  tx/coinutxotx.h \
  tx/delegatetx.h \
  tx/dextx.h \
  tx/dexoperatortx.h \
  tx/coinstaketx.h \
  tx/merkletx.h \
  tx/pricefeedtx.h \
  tx/tx.h \
  tx/txmempool.h \
  tx/txserializer.h \
  tx/proposaltx.h \
  sync.h \
  threadsafety.h \
  tinyformat.h \
  uint256.h \
  wallet/wallet.h \
  wallet/db.h \
  logging.h

JSON_H = \
  commons/json/json_spirit.h \
  commons/json/json_spirit_error_position.h \
  commons/json/json_spirit_reader.h \
  commons/json/json_spirit_reader_template.h \
  commons/json/json_spirit_stream_reader.h \
  commons/json/json_spirit_utils.h \
  commons/json/json_spirit_value.h \
  commons/json/json_spirit_writer.h \
  commons/json/json_spirit_writer_template.h

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
  vm/luavm/luavmrunenv.h \
  vm/luavm/appaccount.h \
  vm/luavm/lmylib.h \
  vm/luavm/luavm.h


VM_CPP = \
  vm/luavm/luavmrunenv.cpp \
  vm/luavm/appaccount.cpp \
  vm/luavm/lmylib.cpp \
  vm/luavm/luavm.cpp

WASM_H = \
  vm/wasm/abi_def.hpp \
  vm/wasm/abi_serializer.hpp \
  vm/wasm/datastream.hpp \
  vm/wasm/exceptions.hpp \
  vm/wasm/receipt.hpp \
  vm/wasm/wasm_config.hpp \
  vm/wasm/wasm_context.hpp \
  vm/wasm/wasm_context_interface.hpp \
  vm/wasm/wasm_host_methods.hpp \
  vm/wasm/wasm_interface.hpp \
  vm/wasm/wasm_trace.hpp \
  vm/wasm/wasm_rpc_message.hpp \
  vm/wasm/modules/wasm_native_dispatch.hpp \
  vm/wasm/modules/wasm_native_bank_module.hpp \
  vm/wasm/modules/wasm_native_module.hpp 


WASM_CPP = \
  vm/wasm/abi_serializer.cpp \
  vm/wasm/wasm_context.cpp \
  vm/wasm/abi_serializer.cpp \
  vm/wasm/exception/exception.cpp \
  vm/wasm/exception/log_message.cpp

liblua53_a_SOURCES = \
  $(VMLUA_C)

libcoin_server_a_CPPFLAGS = $(AM_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS) $(WASM_CPPFLAGS)
libcoin_server_a_SOURCES = \
  chain/blockdelegates.cpp \
  chain/chain.cpp \
  chain/merkletree.cpp \
  entities/account.cpp \
  entities/cdp.cpp \
  entities/contract.cpp \
  entities/dexorder.cpp \
  entities/id.cpp \
  entities/key.cpp \
  entities/keystore.cpp \
  entities/proposal.cpp \
  alert.cpp \
  config/configuration.cpp \
  crypto/sha256.cpp \
  init.cpp \
  main.cpp \
  miner/miner.cpp \
  miner/pbftcontext.cpp \
  miner/pbftmanager.cpp \
  net.cpp \
  p2p/addrman.cpp \
  p2p/protocol.cpp \
  p2p/node.cpp \
  p2p/netmessage.cpp \
  rpc/core/httpserver.cpp \
  rpc/core/rpcclient.cpp \
  rpc/core/rpccommons.cpp \
  rpc/core/rpcprotocol.cpp \
  rpc/core/rpcserver.cpp \
  rpc/rpcblockchain.cpp \
  rpc/rpcdex.cpp \
  rpc/rpcdump.cpp \
  rpc/rpcmining.cpp \
  rpc/rpcmisc.cpp \
  rpc/rpcnet.cpp \
  rpc/rpcscoin.cpp \
  rpc/rpctx.cpp \
  rpc/rpcutxo.cpp \
  rpc/rpcvm.cpp \
  rpc/rpcwallet.cpp \
  rpc/rpcwasm.cpp \
  rpc/rpcproposal.cpp \
  rpc/rpctxserializer.cpp \
  sigcache.cpp \
  tx/assettx.cpp \
  tx/accountregtx.cpp \
  tx/accountpermscleartx.cpp \
  tx/blockrewardtx.cpp \
  tx/blockpricemediantx.cpp \
  tx/cdptx.cpp \
  tx/coinminttx.cpp \
  tx/contracttx.cpp \
  tx/cointransfertx.cpp \
  tx/coinutxotx.cpp \
  tx/delegatetx.cpp \
  tx/dextx.cpp \
  tx/dexoperatortx.cpp \
  tx/coinstaketx.cpp \
  tx/proposaltx.cpp \
  tx/pricefeedtx.cpp \
  tx/tx.cpp \
  tx/txmempool.cpp \
  tx/wasmcontracttx.cpp \
  logging.cpp \
  $(VMLUA_H) \
  $(VM_CPP) \
  $(VM_H) \
  $(JSON_H) \
  $(COIN_CORE_H) \
  $(WASM_H) \
  $(WASM_CPP)

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
  commons/util/util.cpp \
  commons/util/threadnames.cpp \
  commons/util/time.cpp \
  crypto/hash.cpp \
  config/chainparams.cpp \
  config/configuration.cpp \
  config/version.cpp \
  fs.cpp \
  netbase.cpp \
  p2p/protocol.cpp \
  persistence/accountdb.cpp \
  persistence/assetdb.cpp \
  persistence/block.cpp \
  persistence/blockdb.cpp \
  persistence/blockundo.cpp \
  persistence/cachewrapper.cpp \
  persistence/cdpdb.cpp \
  persistence/contractdb.cpp \
  persistence/delegatedb.cpp \
  persistence/dexdb.cpp \
  persistence/disk.cpp \
  persistence/txreceiptdb.cpp \
  persistence/pricefeeddb.cpp \
  persistence/txdb.cpp \
  persistence/leveldbwrapper.cpp \
  persistence/logdb.cpp \
  persistence/txutxodb.cpp \
  commons/support/cleanse.cpp \
  commons/support/events.cpp \
  commons/json/json_spirit_reader.cpp \
  commons/json/json_spirit_value.cpp \
  commons/json/json_spirit_writer.cpp \
  sync.cpp \
  tx/coinminttx.cpp \
  $(COIN_CORE_H)

if GLIBC_BACK_COMPAT
libcoin_common_a_SOURCES += commons/compat/glibc_compat.cpp
libcoin_common_a_SOURCES += commons/compat/glibcxx_compat.cpp
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
  $(WASMLIB) \
  $(LIBLEVELDB) \
  $(LIBMEMENV) \
  $(LIBSECP256K1) \
  $(LIBSOFTFLOAT)
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

persistence/leveldb/%.a:
	@echo "Building LevelDB ..." && $(MAKE) -C $(@D) $(@F) CXX="$(CXX)" \
	  CC="$(CC)" PLATFORM=$(TARGET_OS) AR="$(AR)" $(LEVELDB_TARGET_FLAGS) \
	  OPT="$(CXXFLAGS) $(CPPFLAGS)"

qt/bitcoinstrings.cpp: $(libcoin_server_a_SOURCES) $(libcoin_common_a_SOURCES) $(libcoin_cli_a_SOURCES)
	@test -n $(XGETTEXT) || echo "xgettext is required for updating translations"
	@cd $(top_srcdir); XGETTEXT=$(XGETTEXT) share/qt/extract_strings_qt.py

CLEANFILES = persistence/leveldb/libleveldb.a leveldb/libmemenv.a *.gcda *.gcno

DISTCLEANFILES = config/build.h

EXTRA_DIST = leveldb Makefile.include

clean-local:
	-$(MAKE) -C secp256k1 clean
	-$(MAKE) -C leveldb clean
	rm -f persistence/leveldb/*/*.gcno persistence/leveldb/helpers/memenv/*.gcno
