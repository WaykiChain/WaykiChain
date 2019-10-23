// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet.h"

#include "commons/base58.h"

#include <openssl/rand.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include "commons/random.h"
#include "config/configuration.h"
#include "commons/json/json_spirit_value.h"
#include "commons/json/json_spirit_writer_template.h"
#include "net.h"
#include "persistence/accountdb.h"
#include "persistence/contractdb.h"

using namespace json_spirit;
using namespace boost::assign;
using namespace std;
using namespace boost;

string CWallet::defaultFileName("");

bool CWallet::Unlock(const SecureString &strWalletPassphrase) {
    CCrypter crypter;
    CKeyingMaterial vMasterKey;

    {
        LOCK(cs_wallet);
        for (const auto &pMasterKey : mapMasterKeys) {
            if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, pMasterKey.second.vchSalt,
                                              pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                continue;  // try another master key
            if (CCryptoKeyStore::Unlock(vMasterKey))
                return true;
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
                                              pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                return false;
            if (CCryptoKeyStore::Unlock(vMasterKey)) {
                int64_t nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt,
                                             pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations =
                    pMasterKey.second.nDeriveIterations * (100 / ((double)(GetTimeMillis() - nStartTime)));

                nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt,
                                             pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations =
                    (pMasterKey.second.nDeriveIterations +
                     pMasterKey.second.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime))) /
                    2;

                if (pMasterKey.second.nDeriveIterations < 25000) pMasterKey.second.nDeriveIterations = 25000;

                LogPrint("INFO", "Wallet passphrase changed to an nDeriveIterations of %i\n",
                         pMasterKey.second.nDeriveIterations);

                if (!crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt,
                                                  pMasterKey.second.nDeriveIterations,
                                                  pMasterKey.second.nDerivationMethod))
                    return false;
                if (!crypter.Encrypt(vMasterKey, pMasterKey.second.vchCryptedKey))
                    return false;
                CWalletDB(strWalletFile).WriteMasterKey(pMasterKey.first, pMasterKey.second);
                if (fWasLocked)
                    Lock();
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

void CWallet::SyncTransaction(const uint256 &hash, CBaseTx *pTx, const CBlock *pBlock) {
    assert(pTx != nullptr || pBlock != nullptr);

    if (hash.IsNull() && pTx == nullptr) {  // this is block Sync
        uint256 blockhash         = pBlock->GetHash();
        auto GenesisBlockProgress = [&]() {};

        auto ConnectBlockProgress = [&]() {
            CAccountTx netTx(this, blockhash, pBlock->GetHeight());
            for (const auto &sptx : pBlock->vptx) {
                uint256 txid = sptx->GetHash();
                // confirm the tx is mine
                if (IsMine(sptx.get())) {
                    netTx.AddTx(txid, sptx.get());
                }
                if (unconfirmedTx.count(txid) > 0) {
                    CWalletDB(strWalletFile).EraseUnconfirmedTx(txid);
                    unconfirmedTx.erase(txid);
                }
            }
            if (netTx.GetTxSize() > 0) {          // write to disk
                mapInBlockTx[blockhash] = netTx;  // add to map
                netTx.WriteToDisk();
            }
        };

        auto DisConnectBlockProgress = [&]() {
            for (const auto &sptx : pBlock->vptx) {
                if (sptx->IsBlockRewardTx()) {
                    continue;
                }
                if (IsMine(sptx.get())) {
                    unconfirmedTx[sptx.get()->GetHash()] = sptx.get()->GetNewInstance();
                    CWalletDB(strWalletFile).WriteUnconfirmedTx(sptx.get()->GetHash(), unconfirmedTx[sptx.get()->GetHash()]);
                }
            }
            if (mapInBlockTx.count(blockhash)) {
                CWalletDB(strWalletFile).EraseBlockTx(blockhash);
                mapInBlockTx.erase(blockhash);
            }
        };

        auto IsConnect = [&]() {  // Connect or disconnect
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
    if (!fFileBacked)
        return;
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
        // Do not submit the tx if in mempool already.
        if (mempool.Exists(te.first)) {
            continue;
        }
        std::shared_ptr<CBaseTx> pBaseTx = te.second->GetNewInstance();
        auto ret                         = CommitTx(&(*pBaseTx.get()));
        if (!std::get<0>(ret)) {
            erase.push_back(te.first);
            LogPrint("CWallet", "abort invalid tx %s reason:%s\n", te.second.get()->ToString(*pCdMan->pAccountCache),
                     std::get<1>(ret));
        }
    }
    for (auto const &tee : erase) {
        CWalletDB(strWalletFile).EraseUnconfirmedTx(tee);
        unconfirmedTx.erase(tee);
    }
}

//// Call after CreateTransaction unless you want to abort
std::tuple<bool, string> CWallet::CommitTx(CBaseTx *pTx) {
    LOCK2(cs_main, cs_wallet);
    LogPrint("INFO", "CommitTx() : %s\n", pTx->ToString(*pCdMan->pAccountCache));

    {
        CValidationState state;
        if (!::AcceptToMemoryPool(mempool, state, pTx, true)) {
            // This must not fail. The transaction has already been signed and recorded.
            LogPrint("INFO", "CommitTx() : invalid transaction %s\n", state.GetRejectReason());
            return std::make_tuple(false, state.GetRejectReason());
        }
    }

    uint256 txid        = pTx->GetHash();
    unconfirmedTx[txid] = pTx->GetNewInstance();
    bool flag           = CWalletDB(strWalletFile).WriteUnconfirmedTx(txid, unconfirmedTx[txid]);
    string message      = txid.ToString();

    if (!flag) {
        message = strprintf("write unconfirmed tx failed: %s, corrupted wallet?", txid.GetHex());
    }

    ::RelayTransaction(pTx, txid);

    return std::make_tuple(flag, message);
}

DBErrors CWallet::LoadWallet(bool fFirstRunRet) {
    // fFirstRunRet = false;
    return CWalletDB(strWalletFile, "cr+").LoadWallet(this);
}

int64_t CWallet::GetFreeBcoins(bool isConfirmed) const {
    int64_t ret = 0;
    {
        LOCK2(cs_main, cs_wallet);
        set<CKeyID> setKeyId;
        GetKeys(setKeyId);
        for (auto &keyId : setKeyId) {
            if (!isConfirmed)
                ret += mempool.cw->accountCache.GetAccountFreeAmount(keyId, SYMB::WICC);
            else
                ret += pCdMan->pAccountCache->GetAccountFreeAmount(keyId, SYMB::WICC);
        }
    }
    return ret;
}

bool CWallet::EncryptWallet(const SecureString &strWalletPassphrase) {
    if (IsEncrypted())
        return false;

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
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, 25000, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = 2500000 / ((double)(GetTimeMillis() - nStartTime));

    nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations,
                                 kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations =
        (kMasterKey.nDeriveIterations + kMasterKey.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime))) /
        2;

    if (kMasterKey.nDeriveIterations < 25000) kMasterKey.nDeriveIterations = 25000;

    LogPrint("INFO", "Encrypting Wallet with an nDeriveIterations of %i\n", kMasterKey.nDeriveIterations);

    if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations,
                                      kMasterKey.nDerivationMethod))
        return false;
    if (!crypter.Encrypt(vMasterKey, kMasterKey.vchCryptedKey))
        return false;

    {
        LOCK(cs_wallet);
        mapMasterKeys[++nMasterKeyMaxID] = kMasterKey;
        if (fFileBacked) {
            assert(!pWalletDbEncryption);
            pWalletDbEncryption = new CWalletDB(strWalletFile);
            if (!pWalletDbEncryption->TxnBegin()) {
                delete pWalletDbEncryption;
                pWalletDbEncryption = nullptr;
                return false;
            }
            pWalletDbEncryption->WriteMasterKey(nMasterKeyMaxID, kMasterKey);
        }

        if (!EncryptKeys(vMasterKey)) {
            if (fFileBacked) {
                pWalletDbEncryption->TxnAbort();
                delete pWalletDbEncryption;
            }
            // We now probably have half of our keys encrypted in memory, and half not...
            // die and let the user reload their unencrypted wallet.
            assert(false);
        }

        // Encryption was introduced in version 0.4.0
        SetMinVersion(FEATURE_WALLETCRYPT, pWalletDbEncryption);

        if (fFileBacked) {
            if (!pWalletDbEncryption->TxnCommit()) {
                delete pWalletDbEncryption;
                // We now have keys encrypted in memory, but not on disk...
                // die to avoid confusion and let the user reload their unencrypted wallet.
                assert(false);
            }

            delete pWalletDbEncryption;
            pWalletDbEncryption = nullptr;
        }

        Lock();
        Unlock(strWalletPassphrase);
        Lock();

        // Need to completely rewrite the wallet file; if we don't, bdb might keep
        // bits of the unencrypted private key in slack space in the database file.
        CDB::Rewrite(strWalletFile);
    }

    return true;
}

bool CWallet::SetMinVersion(enum WalletFeature nVersion, CWalletDB *pWalletDbIn) {
    LOCK(cs_wallet);  // nWalletVersion
    if (nWalletVersion >= nVersion) return true;

    nWalletVersion = nVersion;
    if (fFileBacked) {
        CWalletDB *pWalletDb = pWalletDbIn ? pWalletDbIn : new CWalletDB(strWalletFile);
        pWalletDb->WriteMinVersion(nWalletVersion);
        if (!pWalletDbIn) delete pWalletDb;
    }

    return true;
}

bool CWallet::StartUp(string &strWalletFile) {
    auto InitError = [](const string &str) {
        LogPrint("ERROR", "%s\n", str);
        return true;
    };

    auto InitWarning = [](const string &str) {
        LogPrint("ERROR", "%s\n", str);
        return true;
    };

    defaultFileName   = SysCfg().GetArg("-wallet", "wallet.dat");
    string strDataDir = GetDataDir().string();

    // Wallet file must be a plain filename without a directory
    if (defaultFileName != boost::filesystem::basename(defaultFileName) + boost::filesystem::extension(defaultFileName))
        return InitError(strprintf(("Wallet %s resides outside data directory %s"), defaultFileName, strDataDir));

    if (strWalletFile == "") {
        strWalletFile = defaultFileName;
    }
    LogPrint("INFO", "Using wallet %s\n", strWalletFile);

    if (!bitdb.Open(GetDataDir())) {
        // try moving the database env out of the way
        boost::filesystem::path pathDatabase    = GetDataDir() / "database";
        boost::filesystem::path pathDatabaseBak = GetDataDir() / strprintf("database.%d.bak", GetTime());
        try {
            boost::filesystem::rename(pathDatabase, pathDatabaseBak);
            LogPrint("INFO", "Moved old %s to %s. Retrying.\n", pathDatabase.string(), pathDatabaseBak.string());
        } catch (boost::filesystem::filesystem_error &error) {
            // failure is ok (well, not really, but it's not worse than what we started with)
        }

        // try again
        if (!bitdb.Open(GetDataDir())) {
            // if it still fails, it probably means we can't even create the database env
            string msg = strprintf(_("Error initializing wallet database environment %s!"), strDataDir);
            return InitError(msg);
        }
    }

    if (SysCfg().GetBoolArg("-salvagewallet", false)) {
        // Recover readable keypairs:
        if (!CWalletDB::Recover(bitdb, strWalletFile, true))
            return false;
    }

    if (filesystem::exists(GetDataDir() / strWalletFile)) {
        CDBEnv::VerifyResult r = bitdb.Verify(strWalletFile, CWalletDB::Recover);
        if (r == CDBEnv::RECOVER_OK) {
            string msg = strprintf(_("Warning: wallet.dat corrupt, data salvaged!"
                                     " Original wallet.dat saved as wallet.{timestamp}.bak in %s; if"
                                     " your balance or transactions are incorrect you should"
                                     " restore from a backup."),
                                   strDataDir);
            InitWarning(msg);
        }
        if (r == CDBEnv::RECOVER_FAIL)
            return InitError(_("wallet.dat corrupt, salvage failed"));
    }

    return true;
}

CWallet *CWallet::GetInstance() {
    string strWalletFile("");
    if (StartUp(strWalletFile)) {
        return new CWallet(strWalletFile);
    }

    return nullptr;
}

Object CAccountTx::ToJsonObj(CKeyID const &key) const {
    Object obj;

    Array txsArr;
    for (auto const &item : mapAccountTx) {
        txsArr.push_back(item.second.get()->ToString(*pCdMan->pAccountCache));
    }

    obj.push_back(Pair("block_hash",    blockHash.ToString()));
    obj.push_back(Pair("block_height",  blockHeight));
    obj.push_back(Pair("tx",            txsArr));

    return obj;
}

uint256 CWallet::GetCheckSum() const {
    CHashWriter ss(SER_GETHASH, CLIENT_VERSION);
    ss << nWalletVersion << bestBlock << mapMasterKeys << mapInBlockTx << unconfirmedTx;
    return ss.GetHash();
}

bool CWallet::IsMine(CBaseTx *pTx) const {
    auto spCW = std::make_shared<CCacheWrapper>(pCdMan);

    set<CKeyID> keyIds;
    if (!pTx->GetInvolvedKeyIds(*spCW, keyIds)) {
        return false;
    }

    for (auto &keyid : keyIds) {
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
             [&](std::map<uint256, CAccountTx>::reference a) { CWalletDB(strWalletFile).EraseUnconfirmedTx(a.first); });
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

bool CWallet::Sign(const CKeyID &keyId, const uint256 &hash, vector<uint8_t> &signature, bool isMiner) const {
    CKey key;
    if (GetKey(keyId, key, isMiner)) {
        return (key.Sign(hash, signature));
    }

    return false;
}

bool CWallet::AddCryptedKey(const CPubKey &vchPubKey, const std::vector<uint8_t> &vchCryptedSecret) {
    if (!CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret))
        return false;

    if (!fFileBacked)
        return true;

    {
        LOCK(cs_wallet);
        if (pWalletDbEncryption)
            return pWalletDbEncryption->WriteCryptedKey(vchPubKey, vchCryptedSecret);
        else
            return CWalletDB(strWalletFile).WriteCryptedKey(vchPubKey, vchCryptedSecret);
    }
    return false;
}

bool CWallet::LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<uint8_t> &vchCryptedSecret) {
    return CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret);
}

bool CWallet::AddKey(const CKey &key, const CKey &minerKey) {
    if ((!key.IsValid()) || (!minerKey.IsValid()))
        return false;

    CKeyCombi keyCombi(key, minerKey, nWalletVersion);
    return AddKey(key.GetPubKey().GetKeyId(), keyCombi);
}

bool CWallet::AddKey(const CKeyID &KeyId, const CKeyCombi &keyCombi) {
    if (!fFileBacked)
        return true;

    if (keyCombi.HaveMainKey()) {
        if (KeyId != keyCombi.GetCKeyID())
            return false;
    }

    if (!CWalletDB(strWalletFile).WriteKeyStoreValue(KeyId, keyCombi, nWalletVersion))
        return false;

    return CCryptoKeyStore::AddKeyCombi(KeyId, keyCombi);
}

bool CWallet::AddKey(const CKey &key) {
    if (!key.IsValid())
        return false;

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

bool CWallet::IsReadyForCoolMiner(const CAccountDBCache &accountView) const {
    CRegID regId;
    for (auto const &item : mapKeys) {
        if (item.second.HaveMinerKey() && accountView.GetRegId(item.first, regId)) {
            return true;
        }
    }

    return false;
}

bool CWallet::ClearAllMainKeysForCoolMiner() {
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
    pWalletDbEncryption = nullptr;
}

bool CWallet::LoadMinVersion(int32_t nVersion) {
    AssertLockHeld(cs_wallet);
    nWalletVersion = nVersion;

    return true;
}

int32_t CWallet::GetVersion() {
    LOCK(cs_wallet);
    return nWalletVersion;
}
