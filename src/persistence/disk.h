// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DISK_H
#define PERSIST_DISK_H

#include "commons/util/util.h"
#include "commons/serialize.h"
#include "entities/id.h"

struct CDiskBlockPos {
    int32_t nFile;
    uint32_t nPos;

    IMPLEMENT_SERIALIZE(
        READWRITE(nFile); /* TODO: write with var signed int format */
        READWRITE(VARINT(nPos));
    )

    CDiskBlockPos() {
        SetNull();
    }

    CDiskBlockPos(uint32_t nFileIn, uint32_t nPosIn) {
        nFile = nFileIn;
        nPos  = nPosIn;
    }

    friend bool operator==(const CDiskBlockPos &a, const CDiskBlockPos &b) {
        return (a.nFile == b.nFile && a.nPos == b.nPos);
    }

    friend bool operator!=(const CDiskBlockPos &a, const CDiskBlockPos &b) {
        return !(a == b);
    }

    void SetNull() {
        nFile = -1;
        nPos  = 0;
    }
    bool IsNull() const { return (nFile == -1); }

    bool IsEmpty() const { return IsNull(); }

    string ToString() const {
        return strprintf("file=%d", nFile) + ", " +
        strprintf("pos=%d", nPos);
    }
};

struct CDiskTxPos : public CDiskBlockPos {
    uint32_t nTxOffset;  // after header
    CTxCord  tx_cord;

    IMPLEMENT_SERIALIZE(
        READWRITE(*(CDiskBlockPos *)this);
        READWRITE(VARINT(nTxOffset));
        READWRITE(tx_cord);
    )

    CDiskTxPos(const CDiskBlockPos &blockIn, uint32_t nTxOffsetIn, const CTxCord &txCordIn) :
        CDiskBlockPos(blockIn.nFile, blockIn.nPos), nTxOffset(nTxOffsetIn), tx_cord(txCordIn) {
    }

    CDiskTxPos() {
        SetNull();
    }

    void SetNull() {
        CDiskBlockPos::SetNull();
        nTxOffset = 0;
    }

    void SetEmpty() { SetNull(); }

    string ToString() const {
        return strprintf("%s, tx_offset=%d", CDiskBlockPos::ToString(), nTxOffset);
    }
};

class CBlockFileInfo {
public:
    uint32_t nBlocks;       // number of blocks stored in file
    uint32_t nSize;         // number of used bytes of block file
    uint32_t nUndoSize;     // number of used bytes in the undo file
    uint32_t nHeightFirst;  // lowest height of block in file
    uint32_t nHeightLast;   // highest height of block in file
    uint64_t nTimeFirst;    // earliest time of block in file
    uint64_t nTimeLast;     // latest time of block in file

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(nBlocks));
        READWRITE(VARINT(nSize));
        READWRITE(VARINT(nUndoSize));
        READWRITE(VARINT(nHeightFirst));
        READWRITE(VARINT(nHeightLast));
        READWRITE(VARINT(nTimeFirst));
        READWRITE(VARINT(nTimeLast));)

    void SetNull() {
        nBlocks      = 0;
        nSize        = 0;
        nUndoSize    = 0;
        nHeightFirst = 0;
        nHeightLast  = 0;
        nTimeFirst   = 0;
        nTimeLast    = 0;
    }

    bool IsEmpty() { return nBlocks == 0 && nSize == 0; }
    void SetEmpty() { SetNull(); }

    CBlockFileInfo() {
        SetNull();
    }

    string ToString() const;

    // update statistics (does not update nSize)
    void AddBlock(uint32_t nHeightIn, uint64_t nTimeIn);
};

FILE *OpenDiskFile(const CDiskBlockPos &pos, const char *prefix, bool fReadOnly);

/** Open a block file (blk?????.dat) */
FILE *OpenBlockFile(const CDiskBlockPos &pos, bool fReadOnly = false);

#endif //PERSIST_DISK_H