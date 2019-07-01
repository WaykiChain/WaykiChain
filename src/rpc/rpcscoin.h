// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RPC_SCOIN_H_
#define RPC_SCOIN_H_

#include <boost/assign/list_of.hpp>

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace std;
using namespace json_spirit;

class CBaseTx;

extern Value submitpricefeedtx(const Array& params, bool fHelp);
extern Value submitstakefcointx(const Array& params, bool fHelp);

extern Value submitdexbuyordertx(const Array& params, bool fHelp);
extern Value submitdexsellordertx(const Array& params, bool fHelp);
extern Value submitdexcancelordertx(const Array& params, bool fHelp);
extern Value submitdexsettletx(const Array& params, bool fHelp);

extern Value submitstakecdptx(const Array& params, bool fHelp);
extern Value submitredeemcdptx(const Array& params, bool fHelp);
extern Value submitliquidatecdptx(const Array& params, bool fHelp);

extern Value getmedianprice(const Array& params, bool fHelp);

extern Value listcdps(const Array& params, bool fHelp);
extern Value listcdpstoliquidate(const Array& params, bool fHelp);

#endif /* RPC_SCOIN_H_ */
