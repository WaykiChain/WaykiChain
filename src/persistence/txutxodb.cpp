// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txutxodb.h"
#include "config/chainparams.h"

bool CTxUTXODBCache::SetUtxoTx(const TxID &txid, const uint64_t &blockHeight, const CCoinUTXOTx &utxo) {
    return txUtxoCache.SetData(txid, std::make_tuple(blockHeight, utxo));
}

bool CTxUTXODBCache::GetUtxoTx(const TxID &txid, uint64_t &blockHeight, CCoinUTXOTx &utxo) {
    std::tuple<uint64_t, CCoinUTXOTx> data;
    bool result = txUtxoCache.GetData(txid, data);
    if (!result)
        return false;
    
    blockHeight = get<0>(data);
    utxo = get<1>(data);

    return true;
}

bool CTxUTXODBCache::DelUtoxTx(const TxID &txid) {
    return txUtxoCache.EraseData(txid);
}

void CTxUTXODBCache::Flush() { txUtxoCache.Flush(); }
