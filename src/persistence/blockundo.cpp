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
// class CTxUndo
string CTxUndo::ToString() const {
    string str;
    str += "txid:" + txid.GetHex() + "\n";
    str += "db_oplog_map:" + dbOpLogMap.ToString();
    return str;
}

// uint256 CTxUndo::CalcStateHash(){
//     CHashWriter hasher(SER_GETHASH, PROTOCOL_VERSION);
//     hasher << dbOpLogMap.ToString();
//     return hasher.GetHash();
// }

////////////////////////////////////////////////////////////////////////////////
// class CBlockUndo


// uint256 CBlockUndo::CalcStateHash(uint256 preHash){
//     CHashWriter hasher(SER_GETHASH, PROTOCOL_VERSION);
//     hasher << preHash;
//     for(auto undo:vtxundo){
//         hasher << undo.CalcStateHash();
//     }
//     return hasher.GetHash();
// }

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
        return ERRORMSG("Deserialize or I/O error - %s", e.what());
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


////////////////////////////////////////////////////////////////////////////////
// class CBlockUndoExecutor

bool CBlockUndoExecutor::Execute() {
    // undoFuncMap
    // RegisterUndoFunc();
    const UndoDataFuncMap &undoDataFuncMap = cw.GetUndoDataFuncMap();

    for (auto it = block_undo.vtxundo.rbegin(); it != block_undo.vtxundo.rend(); it++) {
        for (const auto &opLogPair : it->dbOpLogMap.GetMap()) {
            dbk::PrefixType prefixType = dbk::ParseKeyPrefixType(opLogPair.first);
            if (prefixType == dbk::EMPTY)
                return ERRORMSG("%s(), unkown prefix! prefix_type=%s", __FUNCTION__,
                                opLogPair.first);
            auto funcMapIt = undoDataFuncMap.find(prefixType);
            if (funcMapIt == undoDataFuncMap.end()) {
                return ERRORMSG("%s(), unfound prefix in db! prefix_type=%s", __FUNCTION__,
                                opLogPair.first);
            }
            funcMapIt->second(opLogPair.second);
        }
    }
    return true;
}