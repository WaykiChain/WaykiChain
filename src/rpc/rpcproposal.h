// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RPC_RPCPROPOSAL_H
#define RPC_RPCPROPOSAL_H

#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"

using namespace std;
using namespace json_spirit;


extern Value submitparamgovernproposal(const Array& params, bool fHelp) ;
extern Value submitgovernerupdateproposal(const Array& params, bool fHelp) ;
extern Value submitdexswitchproposal(const Array& params, bool fHelp) ;
extern Value submitminerfeeproposal(const Array& params, bool fHelp) ;
extern Value submitproposalassenttx(const Array& params, bool fHelp) ;

extern Value getsysparam(const Array& params, bool fHelp) ;
extern Value getproposal(const Array& params, bool fHelp) ;


#endif //RPC_RPCPROPOSAL_H
