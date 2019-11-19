// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_TXDB_H
#define PERSIST_TXDB_H

#include "entities/account.h"
#include "entities/id.h"
#include "commons/serialize.h"
#include "commons/json/json_spirit_value.h"
#include "dbaccess.h"
#include "dbconf.h"


#include <map>
#include <vector>

using namespace std;
using namespace json_spirit;

class CBlock;
class CRegID;

class CTxMemCache {
public:
    CTxMemCache() : pBase(nullptr) {}
    CTxMemCache(CTxMemCache *pBaseIn) : pBase(pBaseIn) {}

public:
    bool HaveTx(const uint256 &txid);

    bool AddBlockTx(const CBlock &block);
    bool RemoveBlockTx(const CBlock &block);

    void Clear();
    void SetBaseViewPtr(CTxMemCache *pBaseIn) { pBase = pBaseIn; }
    void Flush();

    Object ToJsonObj() const;
    uint64_t GetSize();

private:
    bool HaveBlock(const uint256 &blockHash) const;
    bool HaveBlock(const CBlock &block);
    void BatchWrite(const UnorderedHashSet &mapBlockTxHashSetIn);

private:
    UnorderedHashSet txids;
    CTxMemCache *pBase;
};

class CTxUndo {
public:
    uint256 txid;
    CDBOpLogMap dbOpLogMap; // dbName -> dbOpLogs

    IMPLEMENT_SERIALIZE(
        READWRITE(txid);
        READWRITE(dbOpLogMap);
	)

public:
    CTxUndo() {}

    CTxUndo(const uint256 &txidIn): txid(txidIn) {}

    void SetTxID(const TxID &txidIn) { txid = txidIn; }

    void Clear() {
        txid = uint256();
        dbOpLogMap.Clear();
    }

    string ToString() const {
        string str;
        str += "txid:" + txid.GetHex() + "\n";
        str += "db_oplog_map:" + dbOpLogMap.ToString();
        return str;
    }
};

#endif // PERSIST_TXDB_H