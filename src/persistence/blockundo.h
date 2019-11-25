// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_BLOCK_UNDO_H
#define PERSIST_BLOCK_UNDO_H

#include "commons/serialize.h"
#include "commons/uint256.h"
#include "cachewrapper.h"
#include "leveldbwrapper.h"
#include "disk.h"

#include <stdint.h>
#include <memory>

class CTxUndo {
public:
    uint256 txid;
    CDBOpLogMap dbOpLogMap; // dbPrefix -> dbOpLogs

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

    string ToString() const;
};

/** Undo information for a CBlock */
class CBlockUndo {
public:
    vector<CTxUndo> vtxundo;

    IMPLEMENT_SERIALIZE(
        READWRITE(vtxundo);
    )

    bool WriteToDisk(CDiskBlockPos &pos, const uint256 &blockHash);

    bool ReadFromDisk(const CDiskBlockPos &pos, const uint256 &blockHash);

    string ToString() const;
};

class CTxUndoOpLogger {
public:
    CCacheWrapper &cw;
    CBlockUndo &block_undo;
    CTxUndo tx_undo;

    CTxUndoOpLogger(CCacheWrapper& cwIn, const TxID& txidIn, CBlockUndo& blockUndoIn)
        : cw(cwIn), block_undo(blockUndoIn) {

        tx_undo.SetTxID(txidIn);
        cw.SetDbOpLogMap(&tx_undo.dbOpLogMap);
    }
    ~CTxUndoOpLogger() {
        block_undo.vtxundo.push_back(tx_undo);
        cw.SetDbOpLogMap(nullptr);
    }
};

class CBlockUndoExecutor {
public:
    CCacheWrapper &cw;
    CBlockUndo &block_undo;

    CBlockUndoExecutor(CCacheWrapper &cwIn, CBlockUndo &blockUndoIn)
        : cw(cwIn), block_undo(blockUndoIn) {}
    bool Execute();
};

/** Open an undo file (rev?????.dat) */
FILE *OpenUndoFile(const CDiskBlockPos &pos, bool fReadOnly = false);

#endif  // PERSIST_BLOCK_UNDO_H