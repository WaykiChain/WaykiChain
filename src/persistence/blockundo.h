// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_BLOCK_UNDO_H
#define PERSIST_BLOCK_UNDO_H

#include "commons/serialize.h"
#include "commons/uint256.h"
#include "disk.h"
#include "persistence/txdb.h"

#include <stdint.h>
#include <memory>

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

/** Open an undo file (rev?????.dat) */
FILE *OpenUndoFile(const CDiskBlockPos &pos, bool fReadOnly = false);

#endif  // PERSIST_BLOCK_UNDO_H