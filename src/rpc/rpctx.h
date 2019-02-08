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

extern Value callcontracttx(const Array& params, bool fHelp);
extern Value signcontracttx(const Array& params, bool fHelp);
extern Value createfreezetx(const Array& params, bool fHelp);
extern Value registercontracttx(const Array& params, bool fHelp);
extern Value votedelegatetx(const Array& params, bool fHelp);

extern Value gensendtoaddresstxraw(const Array& params, bool fHelp);
extern Value genregisteraccounttxraw(const Array& params, bool fHelp);
extern Value genregistercontracttxraw(const Array& params, bool fHelp);
extern Value gencallcontracttxraw(const Array& params, bool fHelp);
extern Value genvotedelegatetxraw(const Array& params, bool fHelp);


extern Value getaddrinfo(const Array& params, bool fHelp);

extern Value gettxdetail(const Array& params, bool fHelp);
// extern Value listregid(const Array& params, bool fHelp);
extern Value sign(const Array& params, bool fHelp);
extern Value getaccountinfo(const Array& params, bool fHelp);
extern Value disconnectblock(const Array& params, bool fHelp);

extern Value getaddrbalance(const Array& params, bool fHelp);
extern Value generateblock(const Array& params, bool fHelp);
//extern Value getpublickey(const Array& params, bool fHelp);

extern Value reloadtxcache(const Array& params, bool fHelp);
extern Value getcontractinfo(const Array& params, bool fHelp);
extern Value getcontractdata(const Array& params, bool fHelp);
extern Value getcontractdataraw(const Array& params, bool fHelp);
extern Value getcontractconfirmdata(const Array& params, bool fHelp);
extern Value saveblocktofile(const Array& params, bool fHelp);
extern Value getcontractdbsize(const Array& params, bool fHelp);
extern Value getalltxinfo(const Array& params, bool fHelp);

extern Value getcontractaccountinfo(const Array& params, bool fHelp);
extern Value gethash(const Array& params, bool fHelp);
extern Value getcontractkeyvalue(const Array& params, bool fHelp);
extern Value gencheckpoint(const Array& params, bool fHelp);
extern Value setcheckpoint(const Array& params, bool fHelp);
extern Value validateaddress(const Array& params, bool fHelp);
extern Object TxToJSON(CBaseTransaction *pTx);
extern Value gettotalcoins(const Array& params, bool fHelp);
extern Value gettotalassets(const Array& params, bool fHelp);
extern Value gettxhashbyaddress(const Array& params, bool fHelp);

extern Value gettransaction(const Array& params, bool fHelp);
extern Value getsignature(const Array& params, bool fHelp);

// extern Value listauthor(const Array& params, bool fHelp);
extern Value listaddr(const Array& params, bool fHelp);
extern Value listtx(const Array& params, bool fHelp);
extern Value listcontractasset(const Array& params, bool fHelp);
extern Value listunconfirmedtx(const Array& params, bool fHelp);
extern Value listcontracts(const Array& params, bool fHelp);
extern Value listtxcache(const Array& params, bool fHelp);
extern Value listdelegates(const Array& params, bool fHelp);
extern Value listcontracttx(const Array& params, bool fHelp);
extern Value listtransactions(const Array& params, bool fHelp);
extern Value listtransactionsv2(const Array& params, bool fHelp);

#endif /* RPCTX_H_ */
