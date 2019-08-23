// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2017-2018 WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RPC_RPCTX_H
#define RPC_RPCTX_H

#include <boost/assign/list_of.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace std;
using namespace json_spirit;

class CBaseTx;

extern Value registeraccounttx(const Array& params, bool fHelp);

extern Value callcontracttx(const Array& params, bool fHelp);
extern Value signcontracttx(const Array& params, bool fHelp);
extern Value createfreezetx(const Array& params, bool fHelp);
extern Value deploycontracttx(const Array& params, bool fHelp);
extern Value votedelegatetx(const Array& params, bool fHelp);

extern Value gensendtoaddressraw(const Array& params, bool fHelp);
extern Value genregisteraccountraw(const Array& params, bool fHelp);
extern Value genregistercontractraw(const Array& params, bool fHelp);
extern Value gencallcontractraw(const Array& params, bool fHelp);
extern Value genvotedelegateraw(const Array& params, bool fHelp);

extern Value getaddrinfo(const Array& params, bool fHelp);

extern Value gettxdetail(const Array& params, bool fHelp);
// extern Value listregid(const Array& params, bool fHelp);
extern Value sign(const Array& params, bool fHelp);
extern Value getaccountinfo(const Array& params, bool fHelp);
extern Value disconnectblock(const Array& params, bool fHelp);

extern Value generateblock(const Array& params, bool fHelp);
//extern Value getpublickey(const Array& params, bool fHelp);

extern Value reloadtxcache(const Array& params, bool fHelp);

extern Value getcontractinfo(const Array& params, bool fHelp);
extern Value getcontractdata(const Array& params, bool fHelp);
extern Value getcontractaccountinfo(const Array& params, bool fHelp);

extern Value saveblocktofile(const Array& params, bool fHelp);
extern Value getalltxinfo(const Array& params, bool fHelp);

extern Value gethash(const Array& params, bool fHelp);
extern Value validateaddr(const Array& params, bool fHelp);
extern Object TxToJSON(CBaseTx *pTx);
extern Value gettotalcoins(const Array& params, bool fHelp);
extern Value gettotalassets(const Array& params, bool fHelp);

extern Value getsignature(const Array& params, bool fHelp);

extern Value listaddr(const Array& params, bool fHelp);
extern Value listtx(const Array& params, bool fHelp);
extern Value listcontractassets(const Array& params, bool fHelp);
extern Value listunconfirmedtx(const Array& params, bool fHelp);
extern Value listcontracts(const Array& params, bool fHelp);
extern Value listtxcache(const Array& params, bool fHelp);
extern Value listdelegates(const Array& params, bool fHelp);

#endif  // RPC_RPCTX_H
