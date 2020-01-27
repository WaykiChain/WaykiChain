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

class CBaseTx;

/***************************** Basic *******************************************/

extern Value submitaccountregistertx(const Array& params, bool fHelp);
extern Value submitnickidregistertx(const Array& params, bool fHelp);
extern Value submitcontractdeploytx(const Array& params, bool fHelp);
extern Value submitcontractcalltx(const Array& params, bool fHelp);
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
extern Object TxToJSON(CBaseTx *pTx);
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
extern Value getcdp(const Array& params, bool fHelp);
extern Value getusercdp(const Array& params, bool fHelp);

extern Value listcdps(const Array& params, bool fHelp);
extern Value listcdpstoliquidate(const Array& params, bool fHelp);


extern Value submitassetissuetx(const Array& params, bool fHelp);
extern Value submitassetupdatetx(const Array& params, bool fHelp);

extern Value getasset(const Array& params, bool fHelp);
extern Value getassets(const Array& params, bool fHelp);

/******************************* DEX *******************************************/
extern Value submitdexbuylimitordertx(const Array& params, bool fHelp);
extern Value submitdexselllimitordertx(const Array& params, bool fHelp);
extern Value submitdexbuymarketordertx(const Array& params, bool fHelp);
extern Value submitdexsellmarketordertx(const Array& params, bool fHelp);

extern Value gendexoperatorordertx(const Array& params, bool fHelp);

extern Value submitdexcancelordertx(const Array& params, bool fHelp);
extern Value submitdexoperatorregtx(const Array& params, bool fHelp);
extern Value submitdexoperatorupdatetx(const Array& params, bool fHelp);
extern Value submitdexsettletx(const Array& params, bool fHelp);

extern Value getdexorder(const Array& params, bool fHelp);
extern Value getdexorders(const Array& params, bool fHelp);
extern Value getdexsysorders(const Array& params, bool fHelp);
extern Value getdexoperator(const Array& params, bool fHelp);
extern Value getdexoperatorbyowner(const Array& params, bool fHelp);

extern Value getdexorderfee(const Array& params, bool fHelp);

/*************************** Proposal ***********************************/
extern Value submitparamgovernproposal(const Array& params, bool fHelp) ;
extern Value submitgovernerupdateproposal(const Array& params, bool fHelp) ;
extern Value submitdexswitchproposal(const Array& params, bool fHelp) ;
extern Value submitminerfeeproposal(const Array& params, bool fHelp) ;
extern Value submitproposalassenttx(const Array& params, bool fHelp) ;

extern Value getsysparam(const Array& params, bool fHelp) ;
extern Value getproposal(const Array& params, bool fHelp) ;

extern Value getminminerfee(const Array& params, bool fHelp) ;

/******************************  WASM VM *********************************/
extern Value vmexecutescript(const json_spirit::Array& params, bool fHelp);

extern Value submitwasmcontractdeploytx(const Array& params, bool fHelp);
extern Value submitwasmcontractcalltx(const Array& params, bool fHelp);
extern Value gettablewasm(const Array& params, bool fHelp);

/******************************  Misc ************************************/

extern void EnsureWalletIsUnlocked();

extern Value getconnectioncount(const json_spirit::Array& params, bool fHelp); // in rpcnet.cpp
extern Value getpeerinfo(const json_spirit::Array& params, bool fHelp);
extern Value ping(const json_spirit::Array& params, bool fHelp);
extern Value addnode(const json_spirit::Array& params, bool fHelp);
extern Value getaddednodeinfo(const json_spirit::Array& params, bool fHelp);
extern Value getnettotals(const json_spirit::Array& params, bool fHelp);
extern Value getchaininfo(const json_spirit::Array& params, bool fHelp);

extern Value dumpprivkey(const json_spirit::Array& params, bool fHelp); // in rpcdump.cpp
extern Value importprivkey(const json_spirit::Array& params, bool fHelp);
extern Value dumpwallet(const json_spirit::Array& params, bool fHelp);
extern Value importwallet(const json_spirit::Array& params, bool fHelp);
extern Value dropminerkeys(const json_spirit::Array& params, bool fHelp);
extern Value dropprivkey(const json_spirit::Array& params, bool fHelp);

//extern Value getgenerate(const json_spirit::Array& params, bool fHelp); // in rpcmining.cpp
extern Value setgenerate(const json_spirit::Array& params, bool fHelp);
extern Value gethashespersec(const json_spirit::Array& params, bool fHelp);
extern Value getmininginfo(const json_spirit::Array& params, bool fHelp);
extern Value submitblock(const json_spirit::Array& params, bool fHelp);
extern Value getminedblocks(const json_spirit::Array& params, bool fHelp);
extern Value getminerbyblocktime(const json_spirit::Array& params, bool fHelp);

extern Value getnewaddr(const json_spirit::Array& params, bool fHelp); // in rpcwallet.cpp
extern Value getaccount(const json_spirit::Array& params, bool fHelp);
extern Value verifymessage(const json_spirit::Array& params, bool fHelp);
extern Value getcoinunitinfo(const json_spirit::Array& params, bool fHelp);
extern Value addmulsigaddr(const json_spirit::Array& params, bool fHelp);
extern Value createmulsig(const json_spirit::Array& params, bool fHelp);

extern Value backupwallet(const json_spirit::Array& params, bool fHelp);
extern Value walletpassphrase(const json_spirit::Array& params, bool fHelp);
extern Value walletpassphrasechange(const json_spirit::Array& params, bool fHelp);
extern Value walletlock(const json_spirit::Array& params, bool fHelp);
extern Value encryptwallet(const json_spirit::Array& params, bool fHelp);
extern Value getinfo(const json_spirit::Array& params, bool fHelp);
extern Value getwalletinfo(const json_spirit::Array& params, bool fHelp);
extern Value getnetworkinfo(const json_spirit::Array& params, bool fHelp);

extern Value signmessage(const json_spirit::Array& params, bool fHelp);
extern Value getcontractassets(const json_spirit:: Array& params, bool fHelp);
extern Value submitsendtx(const json_spirit::Array& params, bool fHelp);
extern Value submitutxotx(const json_spirit::Array& params, bool fHelp);
extern Value genmulsigtx(const json_spirit::Array& params, bool fHelp);

extern Value submittxraw(const json_spirit::Array& params, bool fHelp);

extern Value signtxraw(const json_spirit::Array& params, bool fHelp);
extern Value decodetxraw(const json_spirit::Array& params, bool fHelp);
extern Value decodemulsigscript(const json_spirit::Array& params, bool fHelp);

extern Value getfcoingenesistxinfo(const json_spirit::Array& params, bool fHelp);
extern Value getblockcount(const json_spirit::Array& params, bool fHelp);
extern Value getdifficulty(const json_spirit::Array& params, bool fHelp);
extern Value getrawmempool(const json_spirit::Array& params, bool fHelp);
extern Value getblock(const json_spirit::Array& params, bool fHelp);
extern Value verifychain(const json_spirit::Array& params, bool fHelp);
extern Value getcontractregid(const json_spirit::Array& params, bool fHelp);
extern Value invalidateblock(const json_spirit::Array& params, bool fHelp);
extern Value reconsiderblock(const json_spirit::Array& params, bool fHelp);
extern Value startcommontpstest(const json_spirit::Array& params, bool fHelp);
extern Value startcontracttpstest(const json_spirit::Array& params, bool fHelp);
extern Value getblockfailures(const json_spirit::Array& params, bool fHelp);
extern Value getblockundo(const json_spirit::Array& params, bool fHelp);

extern Value submitpricefeedtx(const json_spirit::Array& params, bool fHelp);
extern Value submitcoinstaketx(const json_spirit::Array& params, bool fHelp);

extern Value submitdexbuylimitordertx(const json_spirit::Array& params, bool fHelp);
extern Value submitdexselllimitordertx(const json_spirit::Array& params, bool fHelp);
extern Value submitdexbuymarketordertx(const json_spirit::Array& params, bool fHelp);
extern Value submitdexsellmarketordertx(const json_spirit::Array& params, bool fHelp);
extern Value gendexoperatorordertx(const json_spirit::Array& params, bool fHelp);

extern Value submitdexcancelordertx(const json_spirit::Array& params, bool fHelp);
extern Value submitdexsettletx(const json_spirit::Array& params, bool fHelp);
extern Value getdexorder(const json_spirit::Array& params, bool fHelp);
extern Value getdexsysorders(const json_spirit::Array& params, bool fHelp);
extern Value getdexorders(const json_spirit::Array& params, bool fHelp);
extern Value submitdexoperatorregtx(const json_spirit::Array& params, bool fHelp);
extern Value submitdexoperatorupdatetx(const json_spirit::Array& params, bool fHelp);
extern Value getdexoperator(const json_spirit::Array& params, bool fHelp);
extern Value getdexoperatorbyowner(const json_spirit::Array& params, bool fHelp);
extern Value getdexorderfee(const json_spirit::Array& params, bool fHelp);

extern Value submitcdpstaketx(const json_spirit::Array& params, bool fHelp);
extern Value submitcdpredeemtx(const json_spirit::Array& params, bool fHelp);
extern Value submitcdpliquidatetx(const json_spirit::Array& params, bool fHelp);
extern Value getscoininfo(const json_spirit::Array& params, bool fHelp);
extern Value getcdp(const json_spirit::Array& params, bool fHelp);
extern Value getusercdp(const json_spirit::Array& params, bool fHelp);

extern Value submitassetissuetx(const json_spirit::Array& params, bool fHelp);
extern Value submitassetupdatetx(const json_spirit::Array& params, bool fHelp);

extern Value getasset(const json_spirit::Array& params, bool fHelp);
extern Value getassets(const json_spirit::Array& params, bool fHelp);

extern Value jsontobinwasm(const json_spirit::Array& params, bool fHelp);
extern Value bintojsonwasm(const json_spirit::Array& params, bool fHelp);
extern Value getcodewasm(const json_spirit::Array& params, bool fHelp);
extern Value getabiwasm(const json_spirit::Array& params, bool fHelp);
extern Value gettxtrace(const json_spirit::Array& params, bool fHelp);
extern Value abidefjsontobinwasm(const json_spirit::Array& params, bool fHelp);

extern Value submitparamgovernproposal(const Array& params, bool fHelp) ; //in rpcproposal.cpp
extern Value submitgovernerupdateproposal(const Array& params, bool fHelp) ;
extern Value submitdexswitchproposal(const Array& params, bool fHelp) ;
extern Value submitminerfeeproposal(const Array& params, bool fHelp) ;

extern Value submitproposalassenttx(const Array& params, bool fHelp) ;
extern Value getsysparam(const Array& params, bool fHelp) ;
extern Value getproposal(const Array& params, bool fHelp) ;
extern Value getminminerfee(const Array& params, bool fHelp) ;

#endif /* RPC_API_H_ */
