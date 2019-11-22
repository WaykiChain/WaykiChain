// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockundo.h"
#include "main.h"

/** Open an undo file (rev?????.dat) */
FILE *OpenUndoFile(const CDiskBlockPos &pos, bool fReadOnly) {
    return OpenDiskFile(pos, "rev", fReadOnly);
}

////////////////////////////////////////////////////////////////////////////////
// class CBlockUndo

bool CBlockUndo::WriteToDisk(CDiskBlockPos &pos, const uint256 &blockHash) {
    // Open history file to append
    CAutoFile fileout = CAutoFile(OpenUndoFile(pos), SER_DISK, CLIENT_VERSION);
    if (!fileout)
        return ERRORMSG("CBlockUndo::WriteToDisk : OpenUndoFile failed");

    // Write index header
    uint32_t nSize = fileout.GetSerializeSize(*this);
    fileout << FLATDATA(SysCfg().MessageStart()) << nSize;

    // Write undo data
    long fileOutPos = ftell(fileout);
    if (fileOutPos < 0)
        return ERRORMSG("CBlockUndo::WriteToDisk : ftell failed");
    pos.nPos = (uint32_t)fileOutPos;
    fileout << *this;

    // calculate & write checksum
    CHashWriter hasher(SER_GETHASH, PROTOCOL_VERSION);
    hasher << blockHash;
    hasher << *this;

    fileout << hasher.GetHash();

    // Flush stdio buffers and commit to disk before returning
    fflush(fileout);
    if (!IsInitialBlockDownload())
        FileCommit(fileout);

    return true;
}

bool CBlockUndo::ReadFromDisk(const CDiskBlockPos &pos, const uint256 &blockHash) {
    // Open history file to read
    CAutoFile filein = CAutoFile(OpenUndoFile(pos, true), SER_DISK, CLIENT_VERSION);
    if (!filein)
        return ERRORMSG("CBlockUndo::ReadFromDisk : OpenBlockFile failed");

    // Read block
    uint256 hashChecksum;
    try {
        filein >> *this;
        filein >> hashChecksum;
    } catch (std::exception &e) {
        return ERRORMSG("%s : Deserialize or I/O error - %s", __func__, e.what());
    }

    // Verify checksum
    CHashWriter hasher(SER_GETHASH, PROTOCOL_VERSION);
    hasher << blockHash;
    hasher << *this;

    if (hashChecksum != hasher.GetHash())
        return ERRORMSG("CBlockUndo::ReadFromDisk : Checksum mismatch");
    return true;
}

string CBlockUndo::ToString() const {
    string str;
    vector<CTxUndo>::const_iterator iterUndo = vtxundo.begin();
    for (; iterUndo != vtxundo.end(); ++iterUndo) {
        str += iterUndo->ToString();
    }
    return str;
}
