// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The WaykiChain developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef COIN_WALLETDB_H
#define COIN_WALLETDB_H


#include "key.h"

#include <list>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>
#include "stdio.h"
#include "db.h"

class CAccountInfo;
class CAccountingEntry;
struct CBlockLocator;
class CKeyPool;
class CMasterKey;
class CWallet;
class uint160;
class uint256;
class CRegID;
class CAccountTx;
class CKeyCombi;

/** Error statuses for the wallet database */
enum DBErrors
{
    DB_LOAD_OK,
    DB_CORRUPT,
    DB_NONCRITICAL_ERROR,
    DB_TOO_NEW,
    DB_LOAD_FAIL,
    DB_NEED_REWRITE
};

/*class CKeyMetadata
{
public:
    static const int CURRENT_VERSION=1;
    int nVersion;
    int64_t nCreateTime; // 0 means unknown

    CKeyMetadata()
    {
        SetNull();
    }
    CKeyMetadata(int64_t nCreateTime_)
    {
        nVersion = CKeyMetadata::CURRENT_VERSION;
        nCreateTime = nCreateTime_;
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(nCreateTime);
    )

    void SetNull()
    {
        nVersion = CKeyMetadata::CURRENT_VERSION;
        nCreateTime = 0;
    }
};*/


/** Access to the wallet database (wallet.dat) */
class CWalletDB : public CDB
{

public:
	CWalletDB(const std::string& strFilename, const char* pszMode = "r+", bool fFlushOnClose = true) : CDB(strFilename, pszMode, fFlushOnClose)
	{
	}
private:
    CWalletDB(const CWalletDB&);
    void operator=(const CWalletDB&);
public:
    bool WriteCryptedKey(const CPubKey& pubkey, const std::vector<unsigned char>& vchCryptedSecret);
    bool WriteKeyStoreValue(const CKeyID &keyId, const CKeyCombi& KeyStoreValue, int nVersion);
    bool EraseKeyStoreValue(const CKeyID &keyId);
    bool WriteBlockTx(const uint256 &hash, const CAccountTx& atx);
    bool EraseBlockTx(const uint256& hash);
    bool WriteUnComFirmedTx(const uint256 &hash, const std::shared_ptr<CBaseTx> &tx);
    bool EraseUnComFirmedTx(const uint256& hash);
    bool WriteMasterKey(unsigned int nID, const CMasterKey& kMasterKey);
    bool EraseMasterKey(unsigned int nID);
    bool WriteVersion(const int version) ;
    bool WriteMinVersion(const int version) ;
    int GetMinVersion(void);
    int GetVersion(void);

    DBErrors LoadWallet(CWallet* pwallet);
    static unsigned int nWalletDBUpdated ;
    static bool Recover(CDBEnv& dbenv, string filename, bool fOnlyKeys);
    static bool Recover(CDBEnv& dbenv, string filename);
};

bool BackupWallet(const CWallet& wallet, const string& strDest);

extern void ThreadFlushWalletDB(const string& strFile);

extern void ThreadRelayTx(CWallet* pWallet);
#endif // COIN_WALLETDB_H
