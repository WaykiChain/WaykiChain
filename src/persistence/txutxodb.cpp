// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txutxodb.h"
#include "config/chainparams.h"

bool CTxUTXODBCache::SetUtxoTx(const TxID &txid, const uint16_t voutIndex) {
    return txUtxoCache.SetData(std::make_pair(txid, voutIndex), 1);
}

bool CTxUTXODBCache::GetUtxoTx(const TxID &txid, const uint16_t voutIndex) {
    uint8_t data;
    bool result = txUtxoCache.GetData(std::make_pair(txid, voutIndex), data);
    if (!result)
        return false;
    
    return true;
}

bool CTxUTXODBCache::DelUtoxTx(const TxID &txid, const uint16_t voutIndex) {
    return txUtxoCache.EraseData(std::make_pair(txid, voutIndex));
}

void CTxUTXODBCache::Flush() { txUtxoCache.Flush(); }
