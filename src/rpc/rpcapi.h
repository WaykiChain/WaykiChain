// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RPC_API_H_
#define RPC_API_H_

#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"
#include "core/rpcserver.h"

using namespace std;
using namespace json_spirit;

// class CBaseTx;

/***************************** Universal *******************************************/
extern Value submittx(const Array& params, bool fHelp);
extern Value submitsetcodetx(const Array& params, bool fHelp);

/***************************** Basic *******************************************/

extern Value submitaccountregistertx(const Array& params, bool fHelp);
extern Value submitaccountpermscleartx(const Array& params, bool fHelp);
extern Value submitcontractdeploytx_r2(const Array& params, bool fHelp);
extern Value submitcontractcalltx_r2(const Array& params, bool fHelp);
extern Value submitdelegatevotetx(const Array& params, bool fHelp);
extern Value submitucontractdeploytx(const Array& params, bool fHelp);
extern Value submitucontractcalltx(const Array& params, bool fHelp);

extern Value gettxdetail(const Array& params, bool fHelp);
extern Value getclosedcdp(const Array& params, bool fHelp);
extern Value sign(const Array& params, bool fHelp);
extern Value getaccountinfo(const Array& params, bool fHelp);
extern Value disconnectblock(const Array& params, bool fHelp);
extern Value reloadtxcache(const Array& params, bool fHelp);

extern Value getcontractinfo(const Array& params, bool fHelp);
extern Value getcontractdata(const Array& params, bool fHelp);
extern Value getcontractaccountinfo(const Array& params, bool fHelp);

extern Value saveblocktofile(const Array& params, bool fHelp);
extern Value gethash(const Array& params, bool fHelp);
extern Value validateaddr(const Array& params, bool fHelp);
extern Value gettotalcoins(const Array& params, bool fHelp);

extern Value getsignature(const Array& params, bool fHelp);

extern Value listaddr(const Array& params, bool fHelp);
extern Value listtx(const Array& params, bool fHelp);
extern Value listcontractassets(const Array& params, bool fHelp);
extern Value listcontracts(const Array& params, bool fHelp);
extern Value listtxcache(const Array& params, bool fHelp);
extern Value listdelegates(const Array& params, bool fHelp);

/*****************************  CDP ********************************************/
extern Value submitpricefeedtx(const Array& params, bool fHelp);
extern Value submitcoinstaketx(const Array& params, bool fHelp);

extern Value submitcdpstaketx(const Array& params, bool fHelp);
extern Value submitcdpredeemtx(const Array& params, bool fHelp);
extern Value submitcdpliquidatetx(const Array& params, bool fHelp);

extern Value getscoininfo(const Array& params, bool fHelp);
extern Value getcdpinfo(const Array& params, bool fHelp);
extern Value getusercdp(const Array& params, bool fHelp);

// extern Value submitassetissuetx(const Array& params, bool fHelp);
// extern Value submitassetupdatetx(const Array& params, bool fHelp);

extern Value getassetinfo(const Array& params, bool fHelp);
extern Value listassets(const Array& params, bool fHelp);

/******************************* DEX *******************************************/
extern Value submitdexbuylimitordertx(const Array& params, bool fHelp);
extern Value submitdexselllimitordertx(const Array& params, bool fHelp);
extern Value submitdexbuymarketordertx(const Array& params, bool fHelp);
extern Value submitdexsellmarketordertx(const Array& params, bool fHelp);

extern Value gendexoperatorordertx(const Array& params, bool fHelp);

extern Value submitdexcancelordertx(const Array& params, bool fHelp);
extern Value submitdexoperatorregtx(const Array& params, bool fHelp);
extern Value submitdexopupdatetx(const Array& params, bool fHelp);
extern Value submitdexsettletx(const Array& params, bool fHelp);

extern Value getdexorder(const Array& params, bool fHelp);
extern Value listdexorders(const Array& params, bool fHelp);
extern Value listdexsysorders(const Array& params, bool fHelp);
extern Value getdexoperator(const Array& params, bool fHelp);
extern Value getdexoperatorbyowner(const Array& params, bool fHelp);

extern Value getdexorderfee(const Array& params, bool fHelp);

/******************************  Misc ************************************/
extern Value getconnectioncount(const Array& params, bool fHelp); // in rpcnet.cpp
extern Value getpeerinfo(const Array& params, bool fHelp);
extern Value ping(const Array& params, bool fHelp);
extern Value addnode(const Array& params, bool fHelp);
extern Value getaddednodeinfo(const Array& params, bool fHelp);
extern Value getnettotals(const Array& params, bool fHelp);
extern Value getchaininfo(const Array& params, bool fHelp);

/******************************  wallet ************************************/
extern void EnsureWalletIsUnlocked();
extern Value dumpprivkey(const Array& params, bool fHelp); // in rpcdump.cpp
extern Value importprivkey(const Array& params, bool fHelp);
extern Value dumpwallet(const Array& params, bool fHelp);
extern Value importwallet(const Array& params, bool fHelp);
extern Value dropminermainkeys(const Array& params, bool fHelp);
extern Value dropprivkey(const Array& params, bool fHelp);

//extern Value getgenerate(const Array& params, bool fHelp); // in rpcmining.cpp
extern Value setgenerate(const Array& params, bool fHelp);
extern Value getmininginfo(const Array& params, bool fHelp);
extern Value submitblock(const Array& params, bool fHelp);
extern Value getminedblocks(const Array& params, bool fHelp);
extern Value getminerbyblocktime(const Array& params, bool fHelp);

extern Value getnewaddr(const Array& params, bool fHelp); // in rpcwallet.cpp
extern Value getaccount(const Array& params, bool fHelp);
extern Value verifymessage(const Array& params, bool fHelp);
extern Value getcoinunitinfo(const Array& params, bool fHelp);
extern Value addmulsigaddr(const Array& params, bool fHelp);
extern Value createmulsig(const Array& params, bool fHelp);
extern Value getswapcoindetail(const Array& params, bool fHelp);

extern Value backupwallet(const Array& params, bool fHelp);
extern Value walletpassphrase(const Array& params, bool fHelp);
extern Value walletpassphrasechange(const Array& params, bool fHelp);
extern Value walletlock(const Array& params, bool fHelp);
extern Value encryptwallet(const Array& params, bool fHelp);
extern Value getinfo(const Array& params, bool fHelp);
extern Value getwalletinfo(const Array& params, bool fHelp);
extern Value getnetworkinfo(const Array& params, bool fHelp);

extern Value signtxraw(const Array& params, bool fHelp);
extern Value submittxraw(const Array& params, bool fHelp);
extern Value droptxfrommempool(const Array& params, bool fHelp);
extern Value decodetxraw(const Array& params, bool fHelp);
extern Value submitsendtx(const Array& params, bool fHelp);
extern Value signmessage(const Array& params, bool fHelp);
extern Value getcontractassets(const  Array& params, bool fHelp);

extern Value getfcoingenesistxinfo(const Array& params, bool fHelp);
extern Value getblockcount(const Array& params, bool fHelp);
extern Value getrawmempool(const Array& params, bool fHelp);
extern Value getblock(const Array& params, bool fHelp);
extern Value verifychain(const Array& params, bool fHelp);
extern Value getcontractregid(const Array& params, bool fHelp);
extern Value invalidateblock(const Array& params, bool fHelp);
extern Value reconsiderblock(const Array& params, bool fHelp);

extern Value getblockfailures(const Array& params, bool fHelp);
extern Value getblockundo(const Array& params, bool fHelp);

/******************************  Lua VM *********************************/
extern Value luavm_executescript(const Array& params, bool fHelp);
extern Value luavm_executecontract(const Array& params, bool fHelp);


/******************************  WASM & General Module Access *********************************/
extern Value wasm_gettable(const Array& params, bool fHelp);
extern Value wasm_getrow(const Array& params, bool fHelp);
extern Value wasm_json2bin(const Array& params, bool fHelp);
extern Value wasm_bin2json(const Array& params, bool fHelp);
extern Value wasm_getcode(const Array& params, bool fHelp);
extern Value wasm_getabi(const Array& params, bool fHelp);
extern Value wasm_gettxtrace(const Array& params, bool fHelp);
extern Value wasm_abidefjson2bin(const Array& params, bool fHelp);

/******************************  UTXO *********************************/
extern Value genutxomultiinputcondhash(const Array& params, bool fHelp);
extern Value genutxomultisignaddr( const Array& params, bool fHelp);
extern Value genutxomultisignature(const Array& params, bool fHelp);
extern Value submitutxotransfertx(const Array& params, bool fHelp);
extern Value submitpasswordprooftx(const Array& params, bool fHelp);


/******************************  DeGov *********************************/
extern Value submitaccountpermproposal(const Array& params , bool fHelp);
extern Value submitassetpermproposal(const Array& params , bool fHelp);
extern Value submitaxcinproposal(const Array& params, bool fHelp);
extern Value submitaxcoutproposal(const Array& params, bool fHelp);

extern Value getsysparam(const Array& params, bool fHelp);
extern Value getdexbaseandquotecoins(const Array& params, bool fHelp);
extern Value gettotalbpssize(const Array& params, bool fHelp);
extern Value getfeedcoinpairs(const Array& params, bool fHelp);
extern Value submitsendmultitx(const Array& params, bool fHelp);
extern Value genunsignedtxraw(const Array& params, bool fHelp);

/*************************** Proposal ***********************************/
extern Value submitparamgovernproposal(const Array& params, bool fHelp);

extern Value submitcdpparamgovernproposal(const Array& params, bool fHelp);
extern Value submitdexswitchproposal(const Array& params, bool fHelp);
extern Value submitfeedcoinpairproposal(const Array& params, bool fHelp);
extern Value submitminerfeeproposal(const Array& params, bool fHelp);
extern Value submittotalbpssizeupdateproposal(const Array& params, bool fHelp);
extern Value submitcointransferproposal( const Array& params, bool fHelp);
extern Value submitgovernorupdateproposal(const Array& params, bool fHelp);
extern Value submitproposalapprovaltx(const Array& params, bool fHelp);
extern Value submitaxccoinproposal(const Array& params, bool fHelp);
extern Value submitdiaissueproposal(const Array& params, bool fHelp);
extern Value submitcancelorderproposal(const Array& params, bool fHelp);

extern Value getcdpparam(const Array& params, bool fHelp);
extern Value getproposal(const Array& params, bool fHelp);
extern Value getgovernors(const Array& params, bool fHelp);
extern Value listmintxfees(const Array& params, bool fHelp);

// debug only
extern Value dumpdb(const Array& params, bool fHelp);
extern Value startcommontpstest(const Array& params, bool fHelp);
extern Value startcontracttpstest(const Array& params, bool fHelp);

#endif /* RPC_API_H_ */
