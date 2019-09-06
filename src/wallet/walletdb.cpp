// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_NO_CXX11_SCOPED_ENUMS

#include "walletdb.h"

#include "commons/base58.h"
#include "commons/serialize.h"
#include "protocol.h"
#include "sync.h"
#include "tx/tx.h"
#include "wallet.h"

#include <algorithm>
#include <boost/filesystem.hpp>
#include <fstream>
#include <iterator>

using namespace boost;

//
// CWalletDB
//

bool ReadKeyValue(CWallet* pWallet, CDataStream& ssKey, CDataStream& ssValue, string& strType, string& strErr,
                  int32_t MinVersion) {
    try {
        // Unserialize
        // Taking advantage of the fact that pair serialization
        // is just the two items serialized one after the other
        ssKey >> strType;

        if (strType == "tx") {
            uint256 hash;
            ssKey >> hash;
            std::shared_ptr<CBaseTx> pBaseTx;  //= make_shared<CLuaContractInvokeTx>();
            ssValue >> pBaseTx;
            if (pBaseTx->GetHash() == hash) {
                if (pWallet != nullptr)
                    pWallet->unconfirmedTx[hash] = pBaseTx->GetNewInstance();
            } else {
                strErr = "Error reading wallet database: tx corrupt";
                return false;
            }

        } else if (strType == "keystore") {
            if (-1 != MinVersion) {
                ssKey.SetVersion(MinVersion);
                ssValue.SetVersion(MinVersion);
            }
            CKeyCombi keyCombi;
            CKeyID cKeyid;
            ssKey >> cKeyid;
            ssValue >> keyCombi;
            if (keyCombi.HaveMainKey()) {
                if (cKeyid != keyCombi.GetCKeyID()) {
                    strErr = "Error reading wallet database: keystore corrupt";
                    return false;
                }
            }
            if (pWallet != nullptr)
                pWallet->LoadKeyCombi(cKeyid, keyCombi);

        } else if (strType == "ckey") {
            CPubKey pubKey;
            std::vector<unsigned char> vchCryptedSecret;
            ssKey >> pubKey;
            ssValue >> vchCryptedSecret;
            if (pWallet != nullptr)
                pWallet->LoadCryptedKey(pubKey, vchCryptedSecret);

        } else if (strType == "mkey") {
            uint32_t ID;
            CMasterKey kMasterKey;
            ssKey >> ID;
            ssValue >> kMasterKey;
            if (pWallet != nullptr) {
                pWallet->nMasterKeyMaxID = ID;
                pWallet->mapMasterKeys.insert(make_pair(ID, kMasterKey));
            }

        } else if (strType == "blocktx") {
            uint256 hash;
            CAccountTx atx;
            ssKey >> hash;
            ssValue >> atx;
            if (pWallet != nullptr)
                pWallet->mapInBlockTx[hash] = atx;
        } else if (strType == "defaultkey") {
            if (pWallet != nullptr)
                ssValue >> pWallet->vchDefaultKey;

        } else if (strType != "version" && "minversion" != strType) {
            ERRORMSG("load wallet error! read invalid key type:%s\n", strType);
        }
    } catch (...) {
        return false;
    }
    return true;
}

DBErrors CWalletDB::LoadWallet(CWallet* pWallet) {
    pWallet->vchDefaultKey  = CPubKey();
    bool fNoncriticalErrors = false;
    DBErrors result         = DB_LOAD_OK;

    try {
        LOCK(pWallet->cs_wallet);
        int32_t nMinVersion = 0;
        if (Read((string) "minversion", nMinVersion)) {
            if (nMinVersion > CLIENT_VERSION)
                return DB_TOO_NEW;
            pWallet->LoadMinVersion(nMinVersion);
        }

        // Get cursor
        Dbc* pCursor = GetCursor();
        if (!pCursor) {
            LogPrint("INFO", "Error getting wallet database cursor\n");
            return DB_CORRUPT;
        }

        while (true) {
            // Read next record
            CDataStream ssKey(SER_DISK, CLIENT_VERSION);
            CDataStream ssValue(SER_DISK, CLIENT_VERSION);
            int32_t ret = ReadAtCursor(pCursor, ssKey, ssValue);
            if (ret == DB_NOTFOUND)
                break;
            else if (ret != 0) {
                LogPrint("INFO", "Error reading next record from wallet database\n");
                return DB_CORRUPT;
            }

            // Try to be tolerant of single corrupt records:
            string strType, strErr;
            if (!ReadKeyValue(pWallet, ssKey, ssValue, strType, strErr, GetMinVersion())) {
                // losing keys is considered a catastrophic error, anything else
                // we assume the user can live with:
                if (strType == "keystore") {
                    return DB_CORRUPT;
                } else {
                    // Leave other errors alone, if we try to fix them we might make things worse.
                    fNoncriticalErrors = true;  // ... but do warn the user there is something wrong.
                }
            }
            if (!strErr.empty())
                LogPrint("INFO", "%s\n", strErr);
        }

        pCursor->close();

    } catch (boost::thread_interrupted) {
        throw;
    } catch (...) {
        result = DB_CORRUPT;
    }

    if (fNoncriticalErrors && result == DB_LOAD_OK)
        result = DB_NONCRITICAL_ERROR;

    // Any wallet corruption at all: skip any rewriting or
    // upgrading, we don't want to make it worse.
    if (result != DB_LOAD_OK)
        return result;

    LogPrint("INFO", "nFileVersion = %d\n", GetMinVersion());

    // // nTimeFirstKey is only reliable if all keys have metadata
    // if ((wss.nKeys + wss.nCKeys) != wss.nKeyMeta)
    //     pWallet->nTimeFirstKey = 1;  // 0 would be considered 'no value'

    // for (auto hash : wss.vWalletUpgrade)
    //     WriteAccountTx(hash, pWallet->mapWalletTx[hash]);

    // // Rewrite encrypted wallets of versions 0.4.0 and 0.5.0rc:
    // if (wss.fIsEncrypted && (wss.nFileVersion == 40000 || wss.nFileVersion == 50000))
    //     return DB_NEED_REWRITE;

    if (GetMinVersion() < CLIENT_VERSION)  // Update
        WriteVersion(GetMinVersion());

    if (pWallet->IsEmpty()) {
        CKey mCkey;
        mCkey.MakeNewKey();
        if (!pWallet->AddKey(mCkey)) {
            throw runtime_error("add key failed ");
        }
    }

    return result;
}

//
// Try to (very carefully!) recover wallet.dat if there is a problem.
//
bool CWalletDB::Recover(CDBEnv& dbenv, string filename, bool fOnlyKeys) {
    // Recovery procedure:
    // move wallet.dat to wallet.timestamp.bak
    // Call Salvage with fAggressive=true to
    // get as much data as possible.
    // Rewrite salvaged data to wallet.dat
    // Set -rescan so any missing transactions will be
    // found.
    int64_t now        = GetTime();
    string newFilename = strprintf("wallet.%d.bak", now);

    int32_t result = dbenv.dbenv->dbrename(nullptr, filename.c_str(), nullptr, newFilename.c_str(), DB_AUTO_COMMIT);
    if (result == 0) {
        LogPrint("INFO", "Renamed %s to %s\n", filename, newFilename);
    } else {
        LogPrint("INFO", "Failed to rename %s to %s\n", filename, newFilename);
        return false;
    }

    vector<CDBEnv::KeyValPair> salvagedData;
    bool allOK = dbenv.Salvage(newFilename, true, salvagedData);
    if (salvagedData.empty()) {
        LogPrint("INFO", "Salvage(aggressive) found no records in %s.\n", newFilename);
        return false;
    }
    LogPrint("INFO", "Salvage(aggressive) found %u records\n", salvagedData.size());

    bool fSuccess = allOK;
    boost::scoped_ptr<Db> pdbCopy(new Db(dbenv.dbenv, 0));
    int32_t ret = pdbCopy->open(nullptr,           // Txn pointer
                                filename.c_str(),  // Filename
                                "main",            // Logical db name
                                DB_BTREE,          // Database type
                                DB_CREATE,         // Flags
                                0);
    if (ret > 0) {
        LogPrint("INFO", "Cannot create database file %s\n", filename);
        return false;
    }
    DbTxn* ptxn = dbenv.TxnBegin();
    for (auto& row : salvagedData) {
        if (fOnlyKeys) {
            CDataStream ssKey(row.first, SER_DISK, CLIENT_VERSION);
            CDataStream ssValue(row.second, SER_DISK, CLIENT_VERSION);
            string strType, strErr;
            bool fReadOK = ReadKeyValue(nullptr, ssKey, ssValue, strType, strErr, -1);
            if (strType != "keystore")
                continue;
            if (!fReadOK) {
                LogPrint("INFO", "WARNING: CWalletDB::Recover skipping %s: %s\n", strType, strErr);
                continue;
            }
        }
        Dbt datKey(&row.first[0], row.first.size());
        Dbt datValue(&row.second[0], row.second.size());
        int32_t ret2 = pdbCopy->put(ptxn, &datKey, &datValue, DB_NOOVERWRITE);
        if (ret2 > 0)
            fSuccess = false;
    }
    ptxn->commit(0);
    pdbCopy->close(0);

    return fSuccess;
}

bool CWalletDB::Recover(CDBEnv& dbenv, string filename) { return CWalletDB::Recover(dbenv, filename, false); }

bool CWalletDB::WriteBlockTx(const uint256& hash, const CAccountTx& atx) {
    nWalletDBUpdated++;
    return Write(make_pair(string("blocktx"), hash), atx);
}

bool CWalletDB::EraseBlockTx(const uint256& hash) {
    nWalletDBUpdated++;
    return Erase(make_pair(string("blocktx"), hash));
}

bool CWalletDB::WriteKeyStoreValue(const CKeyID& keyId, const CKeyCombi& KeyCombi, int32_t nVersion) {
    nWalletDBUpdated++;
    //	cout << "keystore:" << "Keyid=" << keyId.ToAddress() << " KeyCombi:" << KeyCombi.ToString() << endl;
    return Write(make_pair(string("keystore"), keyId), KeyCombi, true, nVersion);
}

bool CWalletDB::EraseKeyStoreValue(const CKeyID& keyId) {
    nWalletDBUpdated++;
    return Erase(make_pair(string("keystore"), keyId));
}

bool CWalletDB::WriteCryptedKey(const CPubKey& pubkey, const std::vector<unsigned char>& vchCryptedSecret) {
    nWalletDBUpdated++;
    if (!Write(std::make_pair(std::string("ckey"), pubkey), vchCryptedSecret, true))
        return false;
    CKeyCombi keyCombi;
    CKeyID keyId = pubkey.GetKeyId();
    int32_t nVersion(0);
    Read(string("minversion"), nVersion);
    if (Read(make_pair(string("keystore"), keyId), keyCombi, nVersion)) {
        keyCombi.CleanMainKey();
        if (!Write(make_pair(string("keystore"), keyId), keyCombi, true)) {
            return false;
        }
    }
    return true;
}

bool CWalletDB::WriteUnconfirmedTx(const uint256& hash, const std::shared_ptr<CBaseTx>& tx) {
    nWalletDBUpdated++;
    return Write(make_pair(string("tx"), hash), tx);
}

bool CWalletDB::EraseUnconfirmedTx(const uint256& hash) {
    nWalletDBUpdated++;
    return Erase(make_pair(string("tx"), hash));
}

bool CWalletDB::WriteVersion(const int32_t version) {
    nWalletDBUpdated++;
    return Write(string("version"), version);
}
int32_t CWalletDB::GetVersion(void) {
    int32_t verion;
    return Read(string("version"), verion);
}
bool CWalletDB::WriteMinVersion(const int32_t version) {
    nWalletDBUpdated++;
    return Write(string("minversion"), version);
}
int32_t CWalletDB::GetMinVersion(void) {
    int32_t verion;
    return Read(string("minversion"), verion);
}

bool CWalletDB::WriteMasterKey(uint32_t nID, const CMasterKey& kMasterKey) {
    nWalletDBUpdated++;
    return Write(std::make_pair(std::string("mkey"), nID), kMasterKey, true);
}

bool CWalletDB::EraseMasterKey(uint32_t nID) {
    nWalletDBUpdated++;
    return Erase(std::make_pair(std::string("mkey"), nID));
}

uint32_t CWalletDB::nWalletDBUpdated = 0;

// extern CDBEnv bitdb;
void ThreadFlushWalletDB(const string& strFile) {
    // Make this thread recognisable as the wallet flushing thread
    RenameThread("coin-wallet");
    static bool fOneThread;
    if (fOneThread)
        return;
    fOneThread = true;
    if (!SysCfg().GetBoolArg("-flushwallet", true))
        return;

    uint32_t nLastSeen        = CWalletDB::nWalletDBUpdated;
    uint32_t nLastFlushed     = CWalletDB::nWalletDBUpdated;
    int64_t nLastWalletUpdate = GetTime();
    while (true) {
        MilliSleep(500);

        if (nLastSeen != CWalletDB::nWalletDBUpdated) {
            nLastSeen         = CWalletDB::nWalletDBUpdated;
            nLastWalletUpdate = GetTime();
        }

        if (nLastFlushed != CWalletDB::nWalletDBUpdated && GetTime() - nLastWalletUpdate >= 2) {
            TRY_LOCK(bitdb.cs_db, lockDb);
            if (lockDb) {
                // Don't do this if any databases are in use
                int32_t nRefCount                 = 0;
                map<string, int32_t>::iterator mi = bitdb.mapFileUseCount.begin();
                while (mi != bitdb.mapFileUseCount.end()) {
                    nRefCount += (*mi).second;
                    mi++;
                }

                if (nRefCount == 0) {
                    boost::this_thread::interruption_point();
                    map<string, int32_t>::iterator mi = bitdb.mapFileUseCount.find(strFile);
                    if (mi != bitdb.mapFileUseCount.end()) {
                        LogPrint("db", "Flushing wallet.dat\n");
                        nLastFlushed   = CWalletDB::nWalletDBUpdated;
                        int64_t nStart = GetTimeMillis();

                        // Flush wallet.dat so it's self contained
                        bitdb.CloseDb(strFile);
                        bitdb.CheckpointLSN(strFile);

                        bitdb.mapFileUseCount.erase(mi++);
                        LogPrint("db", "Flushed wallet.dat %dms\n", GetTimeMillis() - nStart);
                    }
                }
            }
        }
    }
}

void ThreadRelayTx(CWallet* pWallet) {
    RenameThread("relay-tx");
    while (pWallet) {
        MilliSleep(60 * 1000);
        map<uint256, std::shared_ptr<CBaseTx> >::iterator iterTx = pWallet->unconfirmedTx.begin();
        for (; iterTx != pWallet->unconfirmedTx.end(); ++iterTx) {
            if (mempool.Exists(iterTx->first)) {
                RelayTransaction(iterTx->second.get(), iterTx->first);
                LogPrint("sendtx", "ThreadRelayTx resend tx hash:%s time:%ld\n", iterTx->first.GetHex(), GetTime());
            }
        }
    }
}

bool BackupWallet(const CWallet& wallet, const string& strDest) {
    while (true) {
        {
            LOCK(bitdb.cs_db);
            if (!bitdb.mapFileUseCount.count(wallet.strWalletFile) ||
                bitdb.mapFileUseCount[wallet.strWalletFile] == 0) {
                // Flush log data to the dat file
                bitdb.CloseDb(wallet.strWalletFile);
                bitdb.CheckpointLSN(wallet.strWalletFile);
                bitdb.mapFileUseCount.erase(wallet.strWalletFile);

                // Copy wallet.dat
                boost::filesystem::path pathSrc = GetDataDir() / wallet.strWalletFile;
                boost::filesystem::path pathDest(strDest);
                if (boost::filesystem::is_directory(pathDest))
                    pathDest /= wallet.strWalletFile;

                try {
#if BOOST_VERSION >= 104000
                    boost::filesystem::copy_file(pathSrc, pathDest,
                                                 boost::filesystem::copy_option::overwrite_if_exists);
#else
                    boost::filesystem::copy_file(pathSrc, pathDest);
#endif
                    LogPrint("INFO", "copied wallet.dat into %s\n", pathDest.string());
                    return true;
                } catch (const boost::filesystem::filesystem_error& e) {
                    LogPrint("ERROR", "error copying wallet.dat into %s - %s\n", pathDest.string(), e.what());
                    return false;
                }
            }
        }
        MilliSleep(100);
    }
    return false;
}
