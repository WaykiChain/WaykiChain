// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RPC_APICONF_H_
#define RPC_APICONF_H_

#include "rpcapi.h"

//
// Call Table
//

static const CRPCCommand vRPCCommands[] =
{ //  name                      actor (function)                            okSafeMode threadSafe reqWallet
  //  ------------------------  -----------------------                     ---------- ---------- ---------
    /* Overall control/query calls */
    { "help",                           &help,                              true,      true,        false   },
    { "getinfo",                        &getinfo,                           true,      false,       false   }, /* uses wallet if enabled */
    { "stop",                           &stop,                              true,      true,        false   },
    { "validateaddr",                   &validateaddr,                      true,      true,        false   },
    { "createmulsig",                   &createmulsig,                      true,      true ,       false   },

    /* P2P networking */
    { "getnetworkinfo",                 &getnetworkinfo,                    true,      false,       false   },
    { "addnode",                        &addnode,                           true,      true,        false   },
    { "getaddednodeinfo",               &getaddednodeinfo,                  true,      true,        false   },
    { "getconnectioncount",             &getconnectioncount,                true,      false,       false   },
    { "getnettotals",                   &getnettotals,                      true,      true,        false   },
    { "getpeerinfo",                    &getpeerinfo,                       true,      false,       false   },
    { "ping",                           &ping,                              true,      false,       false   },
    { "getchaininfo",                   &getchaininfo,                      true,      false,       false   },

    /* Block chain and UTXO */
    { "getfcoingenesistxinfo",          &getfcoingenesistxinfo,             true,      true,        false   },
    { "getblockcount",                  &getblockcount,                     true,      true,        false   },
    { "getblock",                       &getblock,                          true,      false,       false   },
    { "getrawmempool",                  &getrawmempool,                     true,      false,       false   },
    { "verifychain",                    &verifychain,                       true,      false,       false   },
    { "getblockundo",                   &getblockundo,                      true,      false,       false   },
    { "getswapcoindetail",              &getswapcoindetail,                 true,      false,        false   },

    { "gettotalcoins",                  &gettotalcoins,                     true,      false,       false   },
    { "invalidateblock",                &invalidateblock,                   true,      true,        false   },
    { "reconsiderblock",                &reconsiderblock,                   true,      true,        false   },
    /* Mining */
    { "getmininginfo",                  &getmininginfo,                     true,      false,       false   },
    { "submitblock",                    &submitblock,                       true,      false,       false   },
    { "getminedblocks",                 &getminedblocks,                    true,      true,        false   },
    { "getminerbyblocktime",            &getminerbyblocktime,               true,      true,        false   },
    /* uses wallet if enabled */
    { "getaccountinfo",                 &getaccountinfo,                    true,      false,       true    },
    { "getnewaddr",                     &getnewaddr,                        false,     false,       true    },
    { "gettxdetail",                    &gettxdetail,                       true,      false,       true    },
    { "getclosedcdp",                   &getclosedcdp,                      true,      false,       true    },
    { "getwalletinfo",                  &getwalletinfo,                     true,      false,       true    },

    { "dumpprivkey",                    &dumpprivkey,                       false,     false,       true    },
    { "importprivkey",                  &importprivkey,                     false,     false,       true    },
    { "dropminermainkeys",              &dropminermainkeys,                 false,     false,       true    },
    { "dropprivkey",                    &dropprivkey,                       false,     false,       true    },
    { "backupwallet",                   &backupwallet,                      false,     false,       true    },
    { "dumpwallet",                     &dumpwallet,                        false,     false,       true    },
    { "importwallet",                   &importwallet,                      false,     false,       true    },
    { "encryptwallet",                  &encryptwallet,                     false,     false,       true    },
    { "walletlock",                     &walletlock,                        false,     false,       true    },
    { "walletpassphrasechange",         &walletpassphrasechange,            false,     false,       true    },
    { "walletpassphrase",               &walletpassphrase,                  false,     false,       true    },

    { "listaddr",                       &listaddr,                          true,      false,       true    },
    { "listtx",                         &listtx,                            true,      false,       true    },
    { "setgenerate",                    &setgenerate,                       true,      true,        false   },
    { "listcontracts",                  &listcontracts,                     true,      false,       true    },
    { "getcontractinfo",                &getcontractinfo,                   true,      false,       true    },
    { "listtxcache",                    &listtxcache,                       true,      false,       true    },
    { "getcontractdata",                &getcontractdata,                   true,      false,       true    },
    { "signmessage",                    &signmessage,                       false,     false,       true    },
    { "verifymessage",                  &verifymessage,                     true,      false,       false   },
    { "getcoinunitinfo",                &getcoinunitinfo,                   true,      false,       false   },
    { "getcontractassets",              &getcontractassets,                 true,      false,       true    },
    { "listcontractassets",             &listcontractassets,                true,      false,       true    },
    { "getcontractaccountinfo",         &getcontractaccountinfo,            true,      false,       true    },
    { "getsignature",                   &getsignature,                      true,      false,       true    },
    { "listdelegates",                  &listdelegates,                     true,      false,       true    },
    { "decodetxraw",                    &decodetxraw,                       true,       false,      false   },
    { "signtxraw",                      &signtxraw,                         true,      false,       true    },
    { "submittxraw",                    &submittxraw,                       true,       false,      false   },
    /* basic tx */
    { "submitsendtx",                   &submitsendtx,                      false,      false,      true    },
    { "submitsendmultitx",              &submitsendmultitx,                 false,      false,      true    },
    { "submitpasswordprooftx",          &submitpasswordprooftx,             false,      false,      true    },
    { "submitutxotransfertx",           &submitutxotransfertx,              false,      false,      true    },
    { "submitaccountregistertx",        &submitaccountregistertx,           false,      false,      true    },
    { "submitaccountpermscleartx",      &submitaccountpermscleartx,         false,      false,      true    },

    { "submitcontractdeploytx_r2",      &submitcontractdeploytx_r2,         false,      false,      true    }, //deprecated
    { "submitcontractcalltx_r2",        &submitcontractcalltx_r2,           false,      false,      true    },
    { "submitdelegatevotetx",           &submitdelegatevotetx,              false,      false,      true    },
    { "submitucontractdeploytx",        &submitucontractdeploytx,           false,      false,      true    },
    { "submitucontractcalltx",          &submitucontractcalltx,             false,      false,      true    },
    { "submitparamgovernproposal",      &submitparamgovernproposal,         false,      false,      true    },
    { "submitcdpparamgovernproposal",   &submitcdpparamgovernproposal,      false,      false,      true    },
    { "submittotalbpssizeupdateproposal",&submittotalbpssizeupdateproposal, false,      false,      true    },
    { "submitcointransferproposal",     &submitcointransferproposal,        false,      false,      true    },

    { "submitgovernorupdateproposal",   &submitgovernorupdateproposal,      false,      false,      true    },
    { "submitdexswitchproposal",        &submitdexswitchproposal,           false,      false,      true    },
    { "submitfeedcoinpairproposal",     &submitfeedcoinpairproposal,        false,      false,      true    },
    { "submitminerfeeproposal",         &submitminerfeeproposal,            false,      false,      true    },
    { "submitaxcinproposal",            &submitaxcinproposal,               false,      false,      true    },
    { "submitaxcoutproposal",           &submitaxcoutproposal,              false,      false,      true    },
    { "submitaxccoinproposal",          &submitaxccoinproposal,             false,      false,      true    },
    { "submitdiaissueproposal",         &submitdiaissueproposal,            false,      false,      true    },

    { "submitaccountpermproposal",      &submitaccountpermproposal,         false,      false,      true    },
    { "submitassetpermproposal",        &submitassetpermproposal,           false,      false,      true    },

    { "submitproposalapprovaltx",       &submitproposalapprovaltx,          false,      false,      true    },
    /* for CDP */
    { "submitpricefeedtx",              &submitpricefeedtx,                 false,      false,      true    },
    { "submitcoinstaketx",              &submitcoinstaketx,                 false,      false,      true    },
    { "submitcdpstaketx",               &submitcdpstaketx,                  false,      false,      true    },
    { "submitcdpredeemtx",              &submitcdpredeemtx,                 false,      false,      true    },
    { "submitcdpliquidatetx",           &submitcdpliquidatetx,              false,      false,      true    },
    { "getscoininfo",                   &getscoininfo,                      true,       false,      false   },
    { "getcdpinfo",                     &getcdpinfo,                        true,       false,      false   },
    { "getusercdp",                     &getusercdp,                        true,       false,      false   },
    { "getsysparam",                    &getsysparam,                       true,       false,      false   },
    { "getcdpparam",                    &getcdpparam,                       true,       false,      false   },
    { "getproposal",                    &getproposal,                       true,       false,      false   },
    { "getgovernors",                   &getgovernors,                      true,       false,      false   },
    { "listmintxfees",                  &listmintxfees,                     true,       false,      false   },
    /* for dex */
    { "submitdexbuylimitordertx",       &submitdexbuylimitordertx,          false,      false,      false   },
    { "submitdexselllimitordertx",      &submitdexselllimitordertx,         false,      false,      false   },
    { "submitdexbuymarketordertx",      &submitdexbuymarketordertx,         false,      false,      false   },
    { "submitdexsellmarketordertx",     &submitdexsellmarketordertx,        false,      false,      false   },

    { "gendexoperatorordertx",          &gendexoperatorordertx,             false,      false,      false   },
    { "submitdexsettletx",              &submitdexsettletx,                 false,      false,      false   },
    { "submitdexcancelordertx",         &submitdexcancelordertx,            false,      false,      false   },
    { "submitdexoperatorregtx",         &submitdexoperatorregtx,            false,      false,      false   },
    { "submitdexopupdatetx",            &submitdexopupdatetx,               false,      false,      false   },
    { "getdexorder",                    &getdexorder,                       true,       false,      false   },
    { "listdexsysorders",               &listdexsysorders,                  true,       false,      false   },
    { "listdexorders",                  &listdexorders,                     true,       false,      false   },
    { "getdexoperator",                 &getdexoperator,                    true,       false,      false   },
    { "getdexoperatorbyowner",          &getdexoperatorbyowner,             true,       false,      false   },
    { "getdexorderfee",                 &getdexorderfee,                    true,       false,      false   },
    { "getdexbaseandquotecoins",        &getdexbaseandquotecoins,           true,       false,      false   },
    { "gettotalbpssize",                &gettotalbpssize,                   true,       false,      false   },
    { "getfeedcoinpairs",               &getfeedcoinpairs,                  true,       false,      false   },
        /* for asset */
    { "submitassetissuetx",             &submitassetissuetx,                false,      false,      false   },
    { "submitassetupdatetx",            &submitassetupdatetx,               false,      false,      false   },
    { "getassetinfo",                   &getassetinfo,                      true,       false,      false   },
    { "listassets",                     &listassets,                        true,       false,      false   },

    /* for wasm-based universal contract deploy & invocation tx submission */
    { "submitcontractdeploytx",         &submitcontractdeploytx,            true,       false,      true    },
    { "submitcontracttx",               &submitcontracttx,                  true,       false,      true    },

    { "wasm_gettable",                  &wasm_gettable,                      true,       false,      true    },
    { "wasm_json2bin",                  &wasm_json2bin,                      true,       false,      true    },
    { "wasm_bin2json",                  &wasm_bin2json,                      true,       false,      true    },
    { "wasm_getcode",                   &wasm_getcode,                       true,       false,      true    },
    { "wasm_getabi",                    &wasm_getabi,                        true,       false,      true    },
    { "wasm_gettxtrace",                &wasm_gettxtrace,                    true,       false,      true    },
    { "wasm_abidefjson2bin",            &wasm_abidefjson2bin,                true,       false,      true    },
    /* for test code */
    { "disconnectblock",                &disconnectblock,                   true,       false,      true    },
    { "reloadtxcache",                  &reloadtxcache,                     true,       false,      true    },
    { "getcontractregid",               &getcontractregid,                  true,       false,      false   },
    { "saveblocktofile",                &saveblocktofile,                   true,       false,      true    },
    { "gethash",                        &gethash,                           true,       false,      true    },
    { "startcommontpstest",             &startcommontpstest,                true,       true,       false   },
    { "startcontracttpstest",           &startcontracttpstest,              true,       true,       false   },
    { "getblockfailures",               &getblockfailures,                  true,       false,      false   },
    /* vm functions work in vm simulator */
    { "luavm_executescript",            &luavm_executescript,                   true,       true,       true    },
    /* debug */
    { "dumpdb",                         &dumpdb,                            true,       true,       true    },
    /* UTXO */
    { "genutxomultiinputcondhash",      &genutxomultiinputcondhash,         true,       true,       false   },
    { "genutxomultisignaddr",           &genutxomultisignaddr,              true,       true,       false   },
    { "genutxomultisignature",          &genutxomultisignature,             true,       true,       false   },
    /* abi tx serializer*/
    { "genunsignedtxraw",                &genunsignedtxraw,                   true,      false,       false   },


};

#endif //RPC_APICONF_H_