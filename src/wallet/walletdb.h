// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_WALLETDB_H
#define COIN_WALLETDB_H

#include "entities/key.h"

#include <stdint.h>
#include <list>
#include <string>
#include <utility>
#include <vector>
#include "db.h"
#include "stdio.h"

class CAccount;
struct CBlockLocator;
class CKeyPool;
class CMasterKey;
class CWallet;
class uint160;
class uint256;
class CRegID;
class CAccountTx;
class CKeyCombi;
class CBaseTx;

/** Error statuses for the wallet database */
enum DBErrors {
    DB_LOAD_OK,
    DB_CORRUPT,
    DB_NONCRITICAL_ERROR,
    DB_TOO_NEW,
    DB_LOAD_FAIL,
    DB_NEED_REWRITE
};

/** Access to the wallet database (wallet.dat) */
class CWalletDB : public CDB {
public:
    CWalletDB(const std::string& strFilename, const char* pszMode = "r+", bool fFlushOnClose = true)
        : CDB(strFilename, pszMode, fFlushOnClose) {}

private:
    CWalletDB(const CWalletDB&);
    void operator=(const CWalletDB&);

public:
    bool WriteCryptedKey(const CPubKey& pubkey, const std::vector<unsigned char>& vchCryptedSecret);
    bool WriteKeyStoreValue(const CKeyID& keyId, const CKeyCombi& KeyStoreValue, int32_t nVersion);
    bool EraseKeyStoreValue(const CKeyID& keyId);
    bool WriteBlockTx(const uint256& hash, const CAccountTx& atx);
    bool EraseBlockTx(const uint256& hash);
    bool WriteUnconfirmedTx(const uint256& hash, const std::shared_ptr<CBaseTx>& tx);
    bool EraseUnconfirmedTx(const uint256& hash);
    bool WriteMasterKey(uint32_t nID, const CMasterKey& kMasterKey);
    bool EraseMasterKey(uint32_t nID);
    bool WriteVersion(const int32_t version);
    bool WriteMinVersion(const int32_t version);
    int32_t GetMinVersion(void);
    int32_t GetVersion(void);

    DBErrors LoadWallet(CWallet* pwallet);
    static uint32_t nWalletDBUpdated;
    static bool Recover(CDBEnv& dbenv, string filename, bool fOnlyKeys);
    static bool Recover(CDBEnv& dbenv, string filename);
};

bool BackupWallet(const CWallet& wallet, const string& strDest);

extern void ThreadFlushWalletDB(const string& strFile);

extern void ThreadRelayTx(CWallet* pWallet);
#endif  // COIN_WALLETDB_H
