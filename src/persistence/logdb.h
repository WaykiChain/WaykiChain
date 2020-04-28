// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_LOGDB_H
#define PERSIST_LOGDB_H

#include "entities/account.h"
#include "entities/id.h"
#include "commons/serialize.h"
#include "dbaccess.h"
#include "dbconf.h"
#include "commons/leb128.h"

#include <map>
#include <set>

#include <vector>

using namespace std;

// TODO: should erase history log?
class CLogDBCache {
public:
    CLogDBCache() {}
    CLogDBCache(CDBAccess *pDbAccess) : executeFailCache(pDbAccess) {}

public:
    bool SetExecuteFail(const int32_t blockHeight, const uint256 txid, const uint8_t errorCode,
                        const string &errorMessage);

    void Flush();

    uint32_t GetCacheSize() const { return executeFailCache.GetCacheSize(); }

    void SetBaseViewPtr(CLogDBCache *pBaseIn) { executeFailCache.SetBase(&pBaseIn->executeFailCache); }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) { executeFailCache.SetDbOpLogMap(pDbOpLogMapIn); }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        executeFailCache.RegisterUndoFunc(undoDataFuncMap);
    }
public:
/*  CCompositeKVCache    prefixType             key                 value                        variable      */
/*  -------------------- --------------------- ------------------  ---------------------------  -------------- */
    // [prefix]{height}{txid} --> {error code, error message}
    CCompositeKVCache<dbk::TX_EXECUTE_FAIL,    pair<CFixedUInt32, uint256>,    std::pair<uint8_t, string> > executeFailCache;
};

#endif // PERSIST_LOGDB_H
