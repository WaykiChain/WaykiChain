// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef TX_EINVALIDTXTYPE_H
#define TX_EINVALIDTXTYPE_H

#include <stdexcept>

using namespace std;

class EInvalidTxType: public runtime_error {
public:
    using runtime_error::runtime_error;
};


#endif //TX_EINVALIDTXTYPE_H
