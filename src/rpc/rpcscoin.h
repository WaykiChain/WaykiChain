// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RPC_SCOIN_H_
#define RPC_SCOIN_H_

#include <boost/assign/list_of.hpp>

#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"

using namespace std;
using namespace json_spirit;

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

#endif /* RPC_SCOIN_H_ */
