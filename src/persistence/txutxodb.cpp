// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txutxodb.h"
#include "config/chainparams.h"

bool CTxUTXODBCache::SetUtxoTx(const pair<TxID, uint16_t> &utxoIndex) {
    return txUtxoCache.SetData(utxoIndex, 1);
}

bool CTxUTXODBCache::GetUtxoTx(const pair<TxID, uint16_t> &utxoIndex) {
    uint8_t data;
    bool result = txUtxoCache.GetData(utxoIndex, data);
    if (!result)
        return false;
    
    return true;
}

bool CTxUTXODBCache::DelUtoxTx(const pair<TxID, uint16_t> &utxoIndex) {
    return txUtxoCache.EraseData(utxoIndex);
}

void CTxUTXODBCache::Flush() { txUtxoCache.Flush(); }
