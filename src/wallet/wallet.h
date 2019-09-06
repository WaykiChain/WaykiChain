// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_WALLET_H
#define COIN_WALLET_H

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>
#include <memory>

#include "crypter.h"
#include "entities/key.h"
#include "entities/keystore.h"
#include "commons/util.h"
#include "walletdb.h"
#include "main.h"
#include "commons/serialize.h"
#include "tx/cointransfertx.h"
#include "tx/blockrewardtx.h"
#include "tx/contracttx.h"
#include "tx/delegatetx.h"
#include "tx/accountregtx.h"


enum WalletFeature
{
    FEATURE_BASE = 0, // initialize version

    FEATURE_WALLETCRYPT = 10000, // wallet encryption
};

// -paytxfee will warn if called with a higher fee than this amount (in satoshis) per KB
static const int32_t nHighTransactionFeeWarning = 0.01 * COIN;

/** A CWallet is an extension of a keystore, which also maintains a set of transactions and balances,
 * and provides the ability to create new transactions.
 */
class CWallet : public CCryptoKeyStore, public CWalletInterface{
private:
    CWallet();

    CWalletDB *pWalletDbEncryption;

    static bool StartUp(string &strWalletFile);

    int32_t nWalletVersion;
    CBlockLocator  bestBlock;
    uint256 GetCheckSum() const;

public:
    CPubKey vchDefaultKey ;

    bool fFileBacked;
    string strWalletFile;

    map<uint256, CAccountTx> mapInBlockTx;
    map<uint256, std::shared_ptr<CBaseTx> > unconfirmedTx;
    mutable CCriticalSection cs_wallet;

    typedef std::map<uint32_t, CMasterKey> MasterKeyMap;
    MasterKeyMap mapMasterKeys;
    uint32_t nMasterKeyMaxID;
    static string defaultFileName;  // default to wallet.dat

    IMPLEMENT_SERIALIZE
    (
        LOCK(cs_wallet);
        {
            READWRITE(nWalletVersion);
            READWRITE(bestBlock);
            READWRITE(mapMasterKeys);
            READWRITE(mapInBlockTx);
            READWRITE(unconfirmedTx);
            uint256 sun;
            if (fWrite){
                sun = GetCheckSum();
            }
            READWRITE(sun);
            if (fRead) {
                if (sun != GetCheckSum()) {
                    throw "wallet file Invalid";
                }
            }
        }
    )
    virtual ~CWallet(){};
    int64_t GetFreeBcoins(bool isConfirmed = true) const;

    bool Sign(const CKeyID &keyId, const uint256 &hash, vector<unsigned char> &signature, bool isMiner = false) const;
    //! Adds an encrypted key to the store, and saves it to disk.
    bool AddCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    bool LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);

    //! Adds a key to the store, without saving it to disk (used by LoadWallet)
    bool LoadKeyCombi(const CKeyID &keyId, const CKeyCombi &keyCombi) {
        return CBasicKeyStore::AddKeyCombi(keyId, keyCombi);
    }
    // Adds a key to the store, and saves it to disk.
    bool AddKey(const CKey &secret, const CKey &minerKey);
    bool AddKey(const CKeyID &keyId, const CKeyCombi &store);
    bool AddKey(const CKey &key);
    bool RemoveKey(const CKey &key);

    bool CleanAll(); //just for unit test
    bool IsReadyForCoolMiner(const CAccountDBCache& accountView)const;
    bool ClearAllMainKeysForCoolMiner();

    CWallet(string strWalletFileIn);
    void SetNull() ;

    bool LoadMinVersion(int32_t nVersion);

    void SyncTransaction(const uint256 &hash, CBaseTx *pTx, const CBlock* pblock);
    void EraseTransaction(const uint256 &hash);
    void ResendWalletTransactions();

    bool IsMine(CBaseTx*pTx)const;

    void SetBestChain(const CBlockLocator& loc);

    DBErrors LoadWallet(bool fFirstRunRet);

    bool EncryptWallet(const SecureString& strWalletPassphrase);

    bool Unlock(const SecureString& strWalletPassphrase);

    bool ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase);

    // get the current wallet format (the oldest client version guaranteed to understand this wallet)
    int32_t GetVersion() ;

    bool SetMinVersion(enum WalletFeature nVersion, CWalletDB* pWalletDbIn);

    static CWallet* GetInstance();

    std::tuple<bool,string>  CommitTx(CBaseTx *pTx);
};

/** Private key that includes an expiration date in case it never gets used. */
class CWalletKey {
public:
    CPrivKey vchPrivKey;
    int64_t nTimeCreated;
    int64_t nTimeExpires;
    string strComment;

    CWalletKey(int64_t nExpires = 0) {
        nTimeCreated = (nExpires ? GetTime() : 0);
        nTimeExpires = nExpires;
    }

    IMPLEMENT_SERIALIZE
    (
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
            READWRITE(vchPrivKey);
            READWRITE(nTimeCreated);
            READWRITE(nTimeExpires);
            READWRITE(strComment);
    )
};

class CAccountTx {
private:
    CWallet* pWallet;

public:
    uint256 blockHash;
    int32_t blockHeight;
    map<uint256, std::shared_ptr<CBaseTx> > mapAccountTx;
public:
    CAccountTx(CWallet* pWalletIn = NULL, uint256 hash = uint256(), int32_t height = 0) {
        pWallet = pWalletIn;
        blockHash = hash;
        mapAccountTx.clear();
        blockHeight = height;
    }

    ~CAccountTx() { }

    void BindWallet(CWallet* pWalletIn) {
        if (pWallet == NULL) {
            assert(pWalletIn != NULL);
            pWallet = pWalletIn;
        }
    }

    bool AddTx(const uint256 &hash, const CBaseTx *pTx) {
        mapAccountTx[hash] = pTx->GetNewInstance();
        return true;
    }

    bool HaveTx(const uint256 &hash) {
        if (mapAccountTx.end() != mapAccountTx.find(hash)) {
            return true;
        }
        return false;
    }

    bool DelTx(const uint256 &hash) {
        return mapAccountTx.erase(hash);
    }

    size_t GetTxSize() {
        return mapAccountTx.size();
    }

    bool WriteToDisk() {
        return CWalletDB(pWallet->strWalletFile).WriteBlockTx(blockHash, *this);
    }

    Object ToJsonObj(CKeyID const &key = CKeyID()) const;

    IMPLEMENT_SERIALIZE
    (
        READWRITE(blockHash);
        READWRITE(blockHeight);
        READWRITE(mapAccountTx);
    )
};

#endif
