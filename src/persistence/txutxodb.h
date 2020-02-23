// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_TXUTXODB_H
#define PERSIST_TXUTXODB_H

#include <map>
#include <set>
#include <vector>

#include "commons/serialize.h"
#include "dbaccess.h"
#include "dbconf.h"
#include "tx/coinutxotx.h"

using namespace std;

class CTxUTXODBCache {
public:
    CTxUTXODBCache() {};
    CTxUTXODBCache(CDBAccess *pDbAccess) : txUtxoCache(pDbAccess) {};
    CTxUTXODBCache(CTxUTXODBCache* pBaseIn): txUtxoCache(pBaseIn->txUtxoCache) {} ;

public:
    bool SetUtxoTx(const TxID &txid, const uint16_t voutIndex);
    bool GetUtxoTx(const TxID &txid, const uint16_t voutIndex);
    bool DelUtoxTx(const TxID &txid, const uint16_t voutIndex);

    void Flush();

    uint32_t GetCacheSize() const { return txUtxoCache.GetCacheSize(); }

    void SetBaseViewPtr(CTxUTXODBCache *pBaseIn) { txUtxoCache.SetBase(&pBaseIn->txUtxoCache); }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) { txUtxoCache.SetDbOpLogMap(pDbOpLogMapIn); }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        txUtxoCache.RegisterUndoFunc(undoDataFuncMap);
    }
public:
/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// UTXO DB
    // $txid$vout_index -> 1
    CCompositeKVCache<   dbk::TX_UTXO,            pair<TxID, uint16_t>,      uint8_t >             txUtxoCache;
};

#endif // PERSIST_TXUTXODB_H