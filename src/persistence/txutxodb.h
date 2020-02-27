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
    CTxUTXODBCache(CDBAccess *pDbAccess) : txUtxoCache(pDbAccess), txUtxoPasswordProofCache(pDbAccess) {};
    CTxUTXODBCache(CTxUTXODBCache* pBaseIn): txUtxoCache(pBaseIn->txUtxoCache), 
        txUtxoPasswordProofCache(pBaseIn->txUtxoPasswordProofCache) {} ;

public:
    bool SetUtxoTx(const pair<TxID, uint16_t> &utxoKey);
    bool GetUtxoTx(const pair<TxID, uint16_t> &utxoKey);
    bool DelUtoxTx(const pair<TxID, uint16_t> &utxoKey);

    bool SetUtxoPasswordProof(const tuple<TxID, uint16_t, CRegIDKey> &proofKey, uint256 &proof);
    bool GetUtxoPasswordProof(const tuple<TxID, uint16_t, CRegIDKey> &proofKey, uint256 &proof);
    bool DelUtoxPasswordProof(const tuple<TxID, uint16_t, CRegIDKey> &proofKey);

    void Flush() { 
        txUtxoCache.Flush(); 
        txUtxoPasswordProofCache.Flush(); 
    }

    uint32_t GetCacheSize() const { 
        return txUtxoCache.GetCacheSize() + txUtxoPasswordProofCache.GetCacheSize(); 
    }

    void SetBaseViewPtr(CTxUTXODBCache *pBaseIn) { 
        txUtxoCache.SetBase(&pBaseIn->txUtxoCache);
        txUtxoPasswordProofCache.SetBase(&pBaseIn->txUtxoPasswordProofCache); 
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) { 
        txUtxoCache.SetDbOpLogMap(pDbOpLogMapIn); 
        txUtxoPasswordProofCache.SetDbOpLogMap(pDbOpLogMapIn); 
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        txUtxoCache.RegisterUndoFunc(undoDataFuncMap);
        txUtxoPasswordProofCache.RegisterUndoFunc(undoDataFuncMap);
    }

public:
/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// UTXO DB
    // $txid$vout_index -> 1
    CCompositeKVCache<   dbk::TX_UTXO,            pair<TxID, uint16_t>,      uint8_t >             txUtxoCache;

    // $txid$vout_index$userID -> (hash)
    CCompositeKVCache<   dbk::UTXO_PWSDPRF,       tuple<TxID, uint16_t, CRegIDKey>, uint256 >       txUtxoPasswordProofCache;
};

#endif // PERSIST_TXUTXODB_H