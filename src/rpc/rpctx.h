/*
 * rpctx.h
 *
 *  Created on: Sep 3, 2014
 *      Author: leo
 */

#ifndef RPCTX_H_
#define RPCTX_H_

#include <boost/assign/list_of.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

class CBaseTransaction;

extern Value registeraccounttx(const Array& params, bool fHelp);
extern Value getsendtoaddressraw(const Array& params, bool fHelp);
extern Value createcontracttx(const Array& params, bool fHelp);
extern Value signcontracttx(const Array& params, bool fHelp);
extern Value createfreezetx(const Array& params, bool fHelp);
extern Value registerapptx(const Array& params, bool fHelp);
extern Value createdelegatetx(const Array& params, bool fHelp);
extern Value getdelegatetxraw(const Array& params, bool fHelp);

extern Value listaddr(const Array& params, bool fHelp);
extern Value listtx(const Array& params, bool fHelp);
extern Value listasset(const Array& params, bool fHelp);
extern Value getaddrinfo(const Array& params, bool fHelp);
extern Value listunconfirmedtx(const Array& params, bool fHelp);
extern Value gettxdetail(const Array& params, bool fHelp);
extern Value listregid(const Array& params, bool fHelp);
extern Value sign(const Array& params, bool fHelp);
extern Value getaccountinfo(const Array& params, bool fHelp);
extern Value disconnectblock(const Array& params, bool fHelp);
extern Value listapp(const Array& params, bool fHelp);
extern Value getaddrbalance(const Array& params, bool fHelp);
extern Value generateblock(const Array& params, bool fHelp);
//extern Value getpublickey(const Array& params, bool fHelp);
extern Value listtxcache(const Array& params, bool fHelp);
extern Value reloadtxcache(const Array& params, bool fHelp);
extern Value getappinfo(const Array& params, bool fHelp);
extern Value getappdata(const Array& params, bool fHelp);
extern Value getappdataraw(const Array& params, bool fHelp);
extern Value getappconfirmdata(const Array& params, bool fHelp);
extern Value saveblocktofile(const Array& params, bool fHelp);
extern Value getappdbsize(const Array& params, bool fHelp);
extern Value getalltxinfo(const Array& params, bool fHelp);
extern Value listauthor(const Array& params, bool fHelp);
extern Value getappaccountinfo(const Array& params, bool fHelp);
extern Value gethash(const Array& params, bool fHelp);
extern Value getappkeyvalue(const Array& params, bool fHelp);
extern Value gencheckpoint(const Array& params, bool fHelp);
extern Value setcheckpoint(const Array& params, bool fHelp);
extern Value validateaddress(const Array& params, bool fHelp);
extern Object TxToJSON(CBaseTransaction *pTx);
extern Value gettotalcoin(const Array& params, bool fHelp);
extern Value gettotalassets(const Array& params, bool fHelp);
extern Value gettxhashbyaddress(const Array& params, bool fHelp);
extern Value getrawtx(const Array& params, bool fHelp);
extern Value listtransactions(const Array& params, bool fHelp);
extern Value listtransactionsv2(const Array& params, bool fHelp);
extern Value gettransaction(const Array& params, bool fHelp);
extern Value listcontracttx(const Array& params, bool fHelp);
extern Value getsignature(const Array& params, bool fHelp);
extern Value getdelegatelist(const Array& params, bool fHelp);
#endif /* RPCTX_H_ */
