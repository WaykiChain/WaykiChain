// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The WaykiChain developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet.h"
#include "tx/txdb.h"

#include "commons/base58.h"

#include <openssl/rand.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include "configuration.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"
#include "net.h"
#include "commons/random.h"
using namespace json_spirit;
using namespace boost::assign;

using namespace std;
using namespace boost;

string CWallet::defaultFilename("");

bool CWallet::Unlock(const SecureString &strWalletPassphrase) {
    CCrypter crypter;
    CKeyingMaterial vMasterKey;

    {
        LOCK(cs_wallet);
        for (const auto &pMasterKey : mapMasterKeys) {
            if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, pMasterKey.second.vchSalt,
                                              pMasterKey.second.nDeriveIterations,
                                              pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                continue;  // try another master key
            if (CCryptoKeyStore::Unlock(vMasterKey)) return true;
        }
    }
    return false;
}

bool CWallet::ChangeWalletPassphrase(const SecureString &strOldWalletPassphrase,
                                     const SecureString &strNewWalletPassphrase) {
    bool fWasLocked = IsLocked();

    {
        LOCK(cs_wallet);
        Lock();

        CCrypter crypter;
        CKeyingMaterial vMasterKey;
        for (auto &pMasterKey : mapMasterKeys) {
            if (!crypter.SetKeyFromPassphrase(strOldWalletPassphrase, pMasterKey.second.vchSalt,
                                              pMasterKey.second.nDeriveIterations,
                                              pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey)) return false;
            if (CCryptoKeyStore::Unlock(vMasterKey)) {
                int64_t nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt,
                                             pMasterKey.second.nDeriveIterations,
                                             pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations =
                    pMasterKey.second.nDeriveIterations *
                    (100 / ((double)(GetTimeMillis() - nStartTime)));

                nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt,
                                             pMasterKey.second.nDeriveIterations,
                                             pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations =
                    (pMasterKey.second.nDeriveIterations +
                     pMasterKey.second.nDeriveIterations * 100 /
                         ((double)(GetTimeMillis() - nStartTime))) /
                    2;

                if (pMasterKey.second.nDeriveIterations < 25000)
                    pMasterKey.second.nDeriveIterations = 25000;

                LogPrint("INFO", "Wallet passphrase changed to an nDeriveIterations of %i\n",
                         pMasterKey.second.nDeriveIterations);

                if (!crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt,
                                                  pMasterKey.second.nDeriveIterations,
                                                  pMasterKey.second.nDerivationMethod))
                    return false;
                if (!crypter.Encrypt(vMasterKey, pMasterKey.second.vchCryptedKey)) return false;
                CWalletDB(strWalletFile).WriteMasterKey(pMasterKey.first, pMasterKey.second);
                if (fWasLocked) Lock();
                return true;
            }
        }
    }

    return false;
}

void CWallet::SetBestChain(const CBlockLocator &loc) {
    AssertLockHeld(cs_wallet);
    bestBlock = loc;
}

void CWallet::SyncTransaction(const uint256 &hash, CBaseTx *pTx, const CBlock *pblock) {
    assert(pTx != NULL || pblock != NULL);

    if (hash.IsNull() && pTx == NULL) {  // this is block Sync
        uint256 blockhash         = pblock->GetHash();
        auto GenesisBlockProgress = [&]() {};

        auto ConnectBlockProgress = [&]() {
            CAccountTx newtx(this, blockhash, pblock->GetHeight());
            for (const auto &sptx : pblock->vptx) {
                uint256 hashtx = sptx->GetHash();
                // confirm the tx is mine
                if (IsMine(sptx.get())) {
                    newtx.AddTx(hashtx, sptx.get());
                }
                if (unconfirmedTx.count(hashtx) > 0) {
                    CWalletDB(strWalletFile).EraseUnconfirmedTx(hashtx);
                    unconfirmedTx.erase(hashtx);
                }
            }
            if (newtx.GetTxSize() > 0) {          // write to disk
                mapInBlockTx[blockhash] = newtx;  // add to map
                newtx.WriteToDisk();
            }
        };

        auto DisConnectBlockProgress = [&]() {
            for (const auto &sptx : pblock->vptx) {
                if (sptx->IsCoinBase()) {
                    continue;
                }
                if (IsMine(sptx.get())) {
                    unconfirmedTx[sptx.get()->GetHash()] = sptx.get()->GetNewInstance();
                    CWalletDB(strWalletFile)
                        .WriteUnconfirmedTx(sptx.get()->GetHash(),
                                            unconfirmedTx[sptx.get()->GetHash()]);
                }
            }
            if (mapInBlockTx.count(blockhash)) {
                CWalletDB(strWalletFile).EraseBlockTx(blockhash);
                mapInBlockTx.erase(blockhash);
            }
        };

        auto IsConnect = [&]() {  // test is connect or disconct
            return mapBlockIndex.count(blockhash) && chainActive.Contains(mapBlockIndex[blockhash]);
        };

        {
            LOCK2(cs_main, cs_wallet);
            // GenesisBlock progress
            if (SysCfg().GetGenesisBlockHash() == blockhash) {
                GenesisBlockProgress();
            } else if (IsConnect()) {
                // connect block
                ConnectBlockProgress();
            } else {
                // disconnect block
                DisConnectBlockProgress();
            }
        }
    }
}

void CWallet::EraseTransaction(const uint256 &hash) {
    if (!fFileBacked) return;
    {
        LOCK(cs_wallet);
        if (unconfirmedTx.count(hash)) {
            unconfirmedTx.erase(hash);
            CWalletDB(strWalletFile).EraseUnconfirmedTx(hash);
        }
    }
    return;
}

void CWallet::ResendWalletTransactions() {
    vector<uint256> erase;
    for (auto &te : unconfirmedTx) {
        // Do not sumit the tx if in mempool already.
        if (mempool.Exists(te.first)) {
            continue;
        }
        std::shared_ptr<CBaseTx> pBaseTx = te.second->GetNewInstance();
        auto ret                         = CommitTransaction(&(*pBaseTx.get()));
        if (!std::get<0>(ret)) {
            erase.push_back(te.first);
            LogPrint("CWallet", "abort invalid tx %s reason:%s\n",
                     te.second.get()->ToString(*pAccountViewTip), std::get<1>(ret));
        }
    }
    for (auto const &tee : erase) {
        CWalletDB(strWalletFile).EraseUnconfirmedTx(tee);
        unconfirmedTx.erase(tee);
    }
}

//// Call after CreateTransaction unless you want to abort
std::tuple<bool, string> CWallet::CommitTransaction(CBaseTx *pTx) {
    LOCK2(cs_main, cs_wallet);
    LogPrint("INFO", "CommitTransaction() : %s", pTx->ToString(*pAccountViewTip));

    {
        CValidationState state;
        if (!::AcceptToMemoryPool(mempool, state, pTx, true)) {
            // This must not fail. The transaction has already been signed and recorded.
            LogPrint("INFO", "CommitTransaction() : Error: Transaction not valid %s\n",
                     state.GetRejectReason());
            return std::make_tuple(false, state.GetRejectReason());
        }
    }

    uint256 txhash      = pTx->GetHash();
    unconfirmedTx[txhash] = pTx->GetNewInstance();
    bool flag           = CWalletDB(strWalletFile).WriteUnconfirmedTx(txhash, unconfirmedTx[txhash]);
    ::RelayTransaction(pTx, txhash);

    return std::make_tuple(flag, txhash.ToString());
}

DBErrors CWallet::LoadWallet(bool fFirstRunRet) {
    //    fFirstRunRet = false;
    return CWalletDB(strWalletFile, "cr+").LoadWallet(this);
}

int64_t CWallet::GetRawBalance(bool IsConfirmed) const {
    int64_t ret = 0;
    {
        LOCK2(cs_main, cs_wallet);
        set<CKeyID> setKeyId;
        GetKeys(setKeyId);
        for (auto &keyId : setKeyId) {
            if (!IsConfirmed)
                ret += mempool.pAccountViewCache->GetRawBalance(keyId);
            else
                ret += pAccountViewTip->GetRawBalance(keyId);
        }
    }
    return ret;
}

bool CWallet::EncryptWallet(const SecureString &strWalletPassphrase) {
    if (IsEncrypted()) return false;

    CKeyingMaterial vMasterKey;
    RandAddSeedPerfmon();

    vMasterKey.resize(WALLET_CRYPTO_KEY_SIZE);
    GetRandBytes(&vMasterKey[0], WALLET_CRYPTO_KEY_SIZE);

    CMasterKey kMasterKey;
    RandAddSeedPerfmon();

    kMasterKey.vchSalt.resize(WALLET_CRYPTO_SALT_SIZE);
    GetRandBytes(&kMasterKey.vchSalt[0], WALLET_CRYPTO_SALT_SIZE);

    CCrypter crypter;
    int64_t nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, 25000,
                                 kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = 2500000 / ((double)(GetTimeMillis() - nStartTime));

    nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt,
                                 kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations =
        (kMasterKey.nDeriveIterations +
         kMasterKey.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime))) /
        2;

    if (kMasterKey.nDeriveIterations < 25000) kMasterKey.nDeriveIterations = 25000;

    LogPrint("INFO", "Encrypting Wallet with an nDeriveIterations of %i\n",
             kMasterKey.nDeriveIterations);

    if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt,
                                      kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod))
        return false;
    if (!crypter.Encrypt(vMasterKey, kMasterKey.vchCryptedKey)) return false;

    {
        LOCK(cs_wallet);
        mapMasterKeys[++nMasterKeyMaxID] = kMasterKey;
        if (fFileBacked) {
            assert(!pwalletdbEncryption);
            pwalletdbEncryption = new CWalletDB(strWalletFile);
            if (!pwalletdbEncryption->TxnBegin()) {
                delete pwalletdbEncryption;
                pwalletdbEncryption = NULL;
                return false;
            }
            pwalletdbEncryption->WriteMasterKey(nMasterKeyMaxID, kMasterKey);
        }

        if (!EncryptKeys(vMasterKey)) {
            if (fFileBacked) {
                pwalletdbEncryption->TxnAbort();
                delete pwalletdbEncryption;
            }
            // We now probably have half of our keys encrypted in memory, and half not...
            // die and let the user reload their unencrypted wallet.
            assert(false);
        }

        // Encryption was introduced in version 0.4.0
        SetMinVersion(FEATURE_WALLETCRYPT, pwalletdbEncryption);

        if (fFileBacked) {
            if (!pwalletdbEncryption->TxnCommit()) {
                delete pwalletdbEncryption;
                // We now have keys encrypted in memory, but not on disk...
                // die to avoid confusion and let the user reload their unencrypted wallet.
                assert(false);
            }

            delete pwalletdbEncryption;
            pwalletdbEncryption = NULL;
        }

        Lock();
        Unlock(strWalletPassphrase);
        Lock();

        // Need to completely rewrite the wallet file; if we don't, bdb might keep
        // bits of the unencrypted private key in slack space in the database file.
        CDB::Rewrite(strWalletFile);
    }
    NotifyStatusChanged(this);

    return true;
}

bool CWallet::SetMinVersion(enum WalletFeature nVersion, CWalletDB *pwalletdbIn) {
    LOCK(cs_wallet);  // nWalletVersion
    if (nWalletVersion >= nVersion) return true;

    nWalletVersion = nVersion;
    if (fFileBacked) {
        CWalletDB *pwalletdb = pwalletdbIn ? pwalletdbIn : new CWalletDB(strWalletFile);
        pwalletdb->WriteMinVersion(nWalletVersion);
        if (!pwalletdbIn) delete pwalletdb;
    }

    return true;
}

void CWallet::UpdatedTransaction(const uint256 &hashTx) {
    {
        LOCK(cs_wallet);
        // Only notify UI if this transaction is in this wallet
        NotifyTransactionChanged(this, hashTx, CT_UPDATED);
    }
}

bool CWallet::StartUp(string &strWalletFile) {
    auto InitError = [](const string &str) {
        uiInterface.ThreadSafeMessageBox(
            str, "", CClientUIInterface::MSG_WARNING | CClientUIInterface::NOSHOWGUI);
        return true;
    };

    auto InitWarning = [](const string &str) {
        uiInterface.ThreadSafeMessageBox(
            str, "", CClientUIInterface::MSG_WARNING | CClientUIInterface::NOSHOWGUI);
        return true;
    };

    defaultFilename   = SysCfg().GetArg("-wallet", "wallet.dat");
    string strDataDir = GetDataDir().string();

    // Wallet file must be a plain filename without a directory
    if (defaultFilename != boost::filesystem::basename(defaultFilename) +
                               boost::filesystem::extension(defaultFilename))
        return InitError(strprintf(("Wallet %s resides outside data directory %s"), defaultFilename,
                                   strDataDir));

    if (strWalletFile == "") {
        strWalletFile = defaultFilename;
    }
    LogPrint("INFO", "Using wallet %s\n", strWalletFile);
    uiInterface.InitMessage(_("Verifying wallet..."));

    if (!bitdb.Open(GetDataDir())) {
        // try moving the database env out of the way
        boost::filesystem::path pathDatabase = GetDataDir() / "database";
        boost::filesystem::path pathDatabaseBak =
            GetDataDir() / strprintf("database.%d.bak", GetTime());
        try {
            boost::filesystem::rename(pathDatabase, pathDatabaseBak);
            LogPrint("INFO", "Moved old %s to %s. Retrying.\n", pathDatabase.string(),
                     pathDatabaseBak.string());
        } catch (boost::filesystem::filesystem_error &error) {
            // failure is ok (well, not really, but it's not worse than what we started with)
        }

        // try again
        if (!bitdb.Open(GetDataDir())) {
            // if it still fails, it probably means we can't even create the database env
            string msg =
                strprintf(_("Error initializing wallet database environment %s!"), strDataDir);
            return InitError(msg);
        }
    }

    if (SysCfg().GetBoolArg("-salvagewallet", false)) {
        // Recover readable keypairs:
        if (!CWalletDB::Recover(bitdb, strWalletFile, true)) return false;
    }

    if (filesystem::exists(GetDataDir() / strWalletFile)) {
        CDBEnv::VerifyResult r = bitdb.Verify(strWalletFile, CWalletDB::Recover);
        if (r == CDBEnv::RECOVER_OK) {
            string msg =
                strprintf(_("Warning: wallet.dat corrupt, data salvaged!"
                            " Original wallet.dat saved as wallet.{timestamp}.bak in %s; if"
                            " your balance or transactions are incorrect you should"
                            " restore from a backup."),
                          strDataDir);
            InitWarning(msg);
        }
        if (r == CDBEnv::RECOVER_FAIL) return InitError(_("wallet.dat corrupt, salvage failed"));
    }

    return true;
}

CWallet *CWallet::getinstance() {
    string strWalletFile("");
    if (StartUp(strWalletFile)) {
        return new CWallet(strWalletFile);
    }
    return NULL;
}

Object CAccountTx::ToJsonObj(CKeyID const &key) const {
    Object obj;
    obj.push_back(Pair("blockHash", blockHash.ToString()));
    obj.push_back(Pair("blockHeight", blockHeight));
    Array Tx;
    CAccountViewCache view(*pAccountViewTip);
    for (auto const &re : mapAccountTx) {
        Tx.push_back(re.second.get()->ToString(view));
    }
    obj.push_back(Pair("Tx", Tx));

    return obj;
}

uint256 CWallet::GetCheckSum() const {
    CHashWriter ss(SER_GETHASH, CLIENT_VERSION);
    ss << nWalletVersion << bestBlock << mapMasterKeys << mapInBlockTx << unconfirmedTx;
    return ss.GetHash();
}

bool CWallet::IsMine(CBaseTx *pTx) const {
    set<CKeyID> vaddr;
    CAccountViewCache view(*pAccountViewTip);
    CScriptDBViewCache scriptDB(*pScriptDBTip);
    if (!pTx->GetAddress(vaddr, view, scriptDB)) {
        return false;
    }
    for (auto &keyid : vaddr) {
        if (HaveKey(keyid) > 0) {
            return true;
        }
    }
    return false;
}

bool CWallet::CleanAll() {
    for_each(unconfirmedTx.begin(), unconfirmedTx.end(),
             [&](std::map<uint256, std::shared_ptr<CBaseTx> >::reference a) {
                 CWalletDB(strWalletFile).EraseUnconfirmedTx(a.first);
             });
    unconfirmedTx.clear();

    for_each(mapInBlockTx.begin(), mapInBlockTx.end(),
             [&](std::map<uint256, CAccountTx>::reference a) {
                 CWalletDB(strWalletFile).EraseUnconfirmedTx(a.first);
             });
    mapInBlockTx.clear();

    bestBlock.SetNull();

    if (!IsEncrypted()) {
        for_each(mapKeys.begin(), mapKeys.end(), [&](std::map<CKeyID, CKeyCombi>::reference item) {
            CWalletDB(strWalletFile).EraseKeyStoreValue(item.first);
        });
        mapKeys.clear();
    } else {
        return ERRORMSG("wallet is encrypted hence clear data forbidden!");
    }
    return true;
}

bool CWallet::Sign(const CKeyID &keyId, const uint256 &hash, vector<unsigned char> &signature,
                   bool IsMiner) const {
    CKey key;
    if (GetKey(keyId, key, IsMiner)) {
        // if(IsMiner == true) {
        //     cout <<"Sign miner key PubKey:"<< key.GetPubKey().ToString()<< endl;
        //     cout <<"Sign miner hash:"<< hash.ToString()<< endl;
        // }
        return (key.Sign(hash, signature));
    }
    return false;
}

bool CWallet::AddCryptedKey(const CPubKey &vchPubKey,
                            const std::vector<unsigned char> &vchCryptedSecret) {
    if (!CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret)) return false;

    if (!fFileBacked) return true;

    {
        LOCK(cs_wallet);
        if (pwalletdbEncryption)
            return pwalletdbEncryption->WriteCryptedKey(vchPubKey, vchCryptedSecret);
        else
            return CWalletDB(strWalletFile).WriteCryptedKey(vchPubKey, vchCryptedSecret);
    }
    return false;
}

bool CWallet::LoadCryptedKey(const CPubKey &vchPubKey,
                             const std::vector<unsigned char> &vchCryptedSecret) {
    return CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret);
}

bool CWallet::AddKey(const CKey &key, const CKey &minerKey) {
    if ((!key.IsValid()) || (!minerKey.IsValid())) return false;

    CKeyCombi keyCombi(key, minerKey, nWalletVersion);
    return AddKey(key.GetPubKey().GetKeyId(), keyCombi);
}

bool CWallet::AddKey(const CKeyID &KeyId, const CKeyCombi &keyCombi) {
    if (!fFileBacked) return true;

    if (keyCombi.HaveMainKey()) {
        if (KeyId != keyCombi.GetCKeyID()) return false;
    }

    if (!CWalletDB(strWalletFile).WriteKeyStoreValue(KeyId, keyCombi, nWalletVersion)) return false;

    return CCryptoKeyStore::AddKeyCombi(KeyId, keyCombi);
}

bool CWallet::AddKey(const CKey &key) {
    if (!key.IsValid()) return false;

    CKeyCombi keyCombi(key, nWalletVersion);
    return AddKey(key.GetPubKey().GetKeyId(), keyCombi);
}

bool CWallet::RemoveKey(const CKey &key) {
    CKeyID keyId = key.GetPubKey().GetKeyId();
    mapKeys.erase(keyId);
    if (!IsEncrypted()) {
        CWalletDB(strWalletFile).EraseKeyStoreValue(keyId);
    } else {
        return ERRORMSG("wallet is encrypted hence remove key forbidden!");
    }

    return true;
}

bool CWallet::IsReadyForCoolMiner(const CAccountViewCache &view) const {
    CRegID regId;
    for (auto const &item : mapKeys) {
        if (item.second.HaveMinerKey() && view.GetRegId(item.first, regId)) {
            return true;
        }
    }
    return false;
}

bool CWallet::ClearAllCkeyForCoolMiner() {
    for (auto &item : mapKeys) {
        if (item.second.CleanMainKey()) {
            CWalletDB(strWalletFile).WriteKeyStoreValue(item.first, item.second, nWalletVersion);
        }
    }
    return true;
}

CWallet::CWallet(string strWalletFileIn) {
    SetNull();
    strWalletFile = strWalletFileIn;
    fFileBacked   = true;
}

void CWallet::SetNull() {
    nWalletVersion      = 0;
    fFileBacked         = false;
    nMasterKeyMaxID     = 0;
    pwalletdbEncryption = NULL;
}

bool CWallet::LoadMinVersion(int nVersion) {
    AssertLockHeld(cs_wallet);
    nWalletVersion = nVersion;

    return true;
}

int CWallet::GetVersion() {
    LOCK(cs_wallet);
    return nWalletVersion;
}
