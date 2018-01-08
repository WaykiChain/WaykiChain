include Makefile.include

AM_CPPFLAGS += -I$(builddir)

noinst_LIBRARIES = \
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


SUBDIRS = . $(BUILD_QT) $(BUILD_P_TEST) $(BUILD_TEST)
#DIST_SUBDIRS = . qt test
DIST_SUBDIRS = . test ptest
.PHONY: FORCE
# bitcoin core #
coin_CORE_H = \
  addrman.h \
  alert.h \
  allocators.h \
  base58.h bignum.h \
  bloom.h \
  chainparams.h \
  checkqueue.h \
  clientversion.h \
  compat\compat.h \
  core.h \
  database.h \
  crypter.h \
  hash.h \
  init.h \
  key.h \
  keystore.h \
  leveldbwrapper.h \
  limitedmap.h \
  main.h \
  miner.h \
  mruset.h \
  netbase.h \
  net.h \
  noui.h \
  protocol.h \
  random.h   \
  rpc/rpcclient.h \
  rpc/rpcprotocol.h \
  rpc/rpcserver.h \
  support/cleanse.h \
  serialize.h \
  sync.h \
  threadsafety.h \
  tinyformat.h \
  txdb.h \
  txmempool.h \
  ui_interface.h \
  uint256.h \
  util.h \
  version.h \
  wallet/wallet.h \
  wallet/db.h  \
  arith_uint256.h 
  configuration.h \
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
# VmScript #
VMLUA_H = \
	vm/lua/lapi.h \
	vm/lua/lauxlib.h \
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

	
VM_H = \
	vm/vmrunevn.h \
	vm/script.h \
	vm/appaccount.h \
	vm/vmlua.h
	

VM_CPP = \
	vm/vmrunevn.cpp \
	vm/script.cpp \
	vm/appaccount.cpp \
	vm/lmylib.cpp \
	vm/vmlua.cpp 
	
	
  

obj/build.h: FORCE
	@$(MKDIR_P) $(abs_top_builddir)/src/obj
	@$(top_srcdir)/share/genbuild.sh $(abs_top_builddir)/src/obj/build.h \
	  $(abs_top_srcdir)
version.o: obj/build.h

libcoin_server_a_SOURCES = \
  tx.cpp \
  addrman.cpp \
  alert.cpp \
  bloom.cpp \
  configuration.cpp \
  database.cpp \
  init.cpp \
  leveldbwrapper.cpp \
  main.cpp \
  miner.cpp \
  net.cpp \
  noui.cpp \
  rpc/rpcblockchain.cpp \
  rpc/rpcmining.cpp \
  rpc/rpcmisc.cpp \
  rpc/rpcnet.cpp \
  rpc/rpcrawtransaction.cpp \
  rpc/rpcserver.cpp \
  txdb.cpp \
  txmempool.cpp \
  cuiserver.cpp	\
  $(VMLUA_H) \
  $(VM_CPP) \
  $(VM_H) \
  $(JSON_H) \
  $(COIN_CORE_H)

libcoin_wallet_a_SOURCES = \
  wallet/db.cpp  \
  crypter.cpp \
  rpc/rpcdump.cpp \
  rpc/rpcwallet.cpp \
  keystore.cpp \ 
  wallet/wallet.cpp \
  wallet/walletdb.cpp \
  rpc/rpctx.cpp \
  $(COIN_CORE_H)

libcoin_common_a_SOURCES = \
  base58.cpp \
  allocators.cpp \
  arith_uint256.cpp \
  chainparams.cpp \
  configuration.cpp\
  core.cpp \
  hash.cpp \
  key.cpp \
  netbase.cpp \
  protocol.cpp \
  random.cpp  \
  rpc/rpcprotocol.cpp \
  support/cleanse.cpp \
  sync.cpp \
  syncdata.cpp \
  syncdatadb.cpp \
  util.cpp \
  uint256.cpp \
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
  $(LIBMEMENV)
#if ENABLE_WALLET
#coin_LDADD += libcoin_wallet.a
#endif
coind_SOURCES = coind.cpp
#

if TARGET_WINDOWS
coind_SOURCES += coind-res.rc
endif

AM_CPPFLAGS += $(BDB_CPPFLAGS)
coind_LDADD += $(BOOST_LIBS) $(BDB_LIBS)



# NOTE: This dependency is not strictly necessary, but without it make may try to build both in parallel, which breaks the LevelDB build system in a race
leveldb/libleveldb.a: leveldb/libmemenv.a

leveldb/%.a:
	@echo "Building LevelDB ..." && $(MAKE) -C $(@D) $(@F) CXX="$(CXX)" \
	  CC="$(CC)" PLATFORM=$(TARGET_OS) AR="$(AR)" $(LEVELDB_TARGET_FLAGS) \
	  OPT="$(CXXFLAGS) $(CPPFLAGS)"

qt/bitcoinstrings.cpp: $(libcoin_server_a_SOURCES) $(libcoin_common_a_SOURCES) $(libcoin_cli_a_SOURCES)
	@test -n $(XGETTEXT) || echo "xgettext is required for updating translations"
	@cd $(top_srcdir); XGETTEXT=$(XGETTEXT) share/qt/extract_strings_qt.py

#
#CLEANFILES = leveldb/libleveldb.a leveldb/libmemenv.a *.gcda *.gcno
CLEANFILES =  *.gcda *.gcno
#

DISTCLEANFILES = obj/build.h

EXTRA_DIST = leveldb Makefile.include

clean-local:
	-$(MAKE) -C leveldb clean
	rm -f leveldb/*/*.gcno leveldb/helpers/memenv/*.gcno
