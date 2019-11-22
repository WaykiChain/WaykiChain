// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "disk.h"
#include "logging.h"
#include "commons/util/util.h"
#include "boost/filesystem.hpp"

////////////////////////////////////////////////////////////////////////////////
// class CBlockFileInfo

string CBlockFileInfo::ToString() const {
    return strprintf("CBlockFileInfo(blocks=%u, size=%u, heights=%u...%u, time=%s...%s)", nBlocks,
                     nSize, nHeightFirst, nHeightLast,
                     DateTimeStrFormat("%Y-%m-%d", nTimeFirst).c_str(),
                     DateTimeStrFormat("%Y-%m-%d", nTimeLast).c_str());
}

void CBlockFileInfo::AddBlock(uint32_t nHeightIn, uint64_t nTimeIn) {
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

////////////////////////////////////////////////////////////////////////////////
// global functions

FILE *OpenDiskFile(const CDiskBlockPos &pos, const char *prefix, bool fReadOnly) {
    if (pos.IsNull())
        return nullptr;
    boost::filesystem::path path = GetDataDir() / "blocks" / strprintf("%s%05u.dat", prefix, pos.nFile);
    boost::filesystem::create_directories(path.parent_path());
    FILE *file = fopen(path.string().c_str(), "rb+");
    if (!file && !fReadOnly)
        file = fopen(path.string().c_str(), "wb+");
    if (!file) {
        LogPrint(BCLog::ERROR, "Unable to open file %s\n", path.string());
        return nullptr;
    }

    if (pos.nPos) {
        if (fseek(file, pos.nPos, SEEK_SET)) {
            LogPrint(BCLog::ERROR, "Unable to seek to position %u of %s\n", pos.nPos, path.string());
            fclose(file);
            return nullptr;
        }
    }
    return file;
}

FILE *OpenBlockFile(const CDiskBlockPos &pos, bool fReadOnly) {
    return OpenDiskFile(pos, "blk", fReadOnly);
}
