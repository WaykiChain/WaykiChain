// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DISK_H
#define PERSIST_DISK_H

struct CDiskBlockPos {
    int nFile;
    unsigned int nPos;

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(nFile));
        READWRITE(VARINT(nPos));)

    CDiskBlockPos() {
        SetNull();
    }

    CDiskBlockPos(int nFileIn, unsigned int nPosIn) {
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
};

struct CDiskTxPos : public CDiskBlockPos {
    unsigned int nTxOffset;  // after header

    IMPLEMENT_SERIALIZE(
        READWRITE(*(CDiskBlockPos *)this);
        READWRITE(VARINT(nTxOffset));)

    CDiskTxPos(const CDiskBlockPos &blockIn, unsigned int nTxOffsetIn) :
        CDiskBlockPos(blockIn.nFile, blockIn.nPos), nTxOffset(nTxOffsetIn) {
    }

    CDiskTxPos() {
        SetNull();
    }

    void SetNull() {
        CDiskBlockPos::SetNull();
        nTxOffset = 0;
    }

    void SetEmpty() { SetNull(); }
};

class CBlockFileInfo {
public:
    unsigned int nBlocks;       // number of blocks stored in file
    unsigned int nSize;         // number of used bytes of block file
    unsigned int nUndoSize;     // number of used bytes in the undo file
    unsigned int nHeightFirst;  // lowest height of block in file
    unsigned int nHeightLast;   // highest height of block in file
    uint64_t nTimeFirst;        // earliest time of block in file
    uint64_t nTimeLast;         // latest time of block in file

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

    CBlockFileInfo() {
        SetNull();
    }

    string ToString() const {
        return strprintf("CBlockFileInfo(blocks=%u, size=%u, heights=%u...%u, time=%s...%s)", nBlocks, nSize, nHeightFirst, nHeightLast, DateTimeStrFormat("%Y-%m-%d", nTimeFirst).c_str(), DateTimeStrFormat("%Y-%m-%d", nTimeLast).c_str());
    }

    // update statistics (does not update nSize)
    void AddBlock(unsigned int nHeightIn, uint64_t nTimeIn) {
        if (nBlocks == 0 || nHeightFirst > nHeightIn)
            nHeightFirst = nHeightIn;
        if (nBlocks == 0 || nTimeFirst > nTimeIn)
            nTimeFirst = nTimeIn;
        nBlocks++;
        if (nHeightIn > nHeightLast)
            nHeightLast = nHeightIn;
        if (nTimeIn > nTimeLast)
            nTimeLast = nTimeIn;
    }
};

#endif //PERSIST_DISK_H