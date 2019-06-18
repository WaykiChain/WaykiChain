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
#include "accounts/key.h"
#include "accounts/keystore.h"
#include "util.h"
#include "walletdb.h"
#include "main.h"
#include "commons/serialize.h"
#include "tx/bcointx.h"
#include "tx/blockrewardtx.h"
#include "tx/scointx.h"
#include "tx/fcointx.h"
#include "tx/contracttx.h"
#include "tx/delegatetx.h"
#include "tx/accountregtx.h"


enum WalletFeature
{
    FEATURE_BASE = 0, // initialize version

    FEATURE_WALLETCRYPT = 10000, // wallet encryption
};

// -paytxfee will warn if called with a higher fee than this amount (in satoshis) per KB
static const int nHighTransactionFeeWarning = 0.01 * COIN;

class CAccountingEntry;

/** A CWallet is an extension of a keystore, which also maintains a set of transactions and balances,
 * and provides the ability to create new transactions.
 */
class CWallet : public CCryptoKeyStore, public CWalletInterface{
private:
    CWallet();

    CWalletDB *pWalletDbEncryption;

    static bool StartUp(string &strWalletFile);

    int nWalletVersion;
    CBlockLocator  bestBlock;
    uint256 GetCheckSum() const;

public:
    CPubKey vchDefaultKey ;

    bool fFileBacked;
    string strWalletFile;

    map<uint256, CAccountTx> mapInBlockTx;
    map<uint256, std::shared_ptr<CBaseTx> > unconfirmedTx;
    mutable CCriticalSection cs_wallet;

    typedef std::map<unsigned int, CMasterKey> MasterKeyMap;
    MasterKeyMap mapMasterKeys;
    unsigned int nMasterKeyMaxID;
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
                if(fWrite){
                 sun = GetCheckSum();
                }
                READWRITE(sun);
                if(fRead) {
                    if(sun != GetCheckSum()) {
                        throw "wallet file Invalid";
                    }
                }
            }
    )
    virtual ~CWallet(){};
    int64_t GetFreeBcoins(bool IsConfirmed = true) const;

    bool Sign(const CKeyID &keyID,const uint256 &hash,vector<unsigned char> &signature,bool IsMiner=false) const;
    //! Adds an encrypted key to the store, and saves it to disk.
    bool AddCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    bool LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);

    //! Adds a key to the store, without saving it to disk (used by LoadWallet)
    bool LoadKeyCombi(const CKeyID & keyId, const CKeyCombi& keyCombi) { return CBasicKeyStore::AddKeyCombi(keyId, keyCombi);}
    // Adds a key to the store, and saves it to disk.
    bool AddKey(const CKey &secret, const CKey &minerKey);
    bool AddKey(const CKeyID &keyId, const CKeyCombi &store);
    bool AddKey(const CKey &key);
    bool RemoveKey(const CKey &key);

    bool CleanAll(); //just for unit test
    bool IsReadyForCoolMiner(const CAccountCache& view)const;
    bool ClearAllMainKeysForCoolMiner();

    CWallet(string strWalletFileIn);
    void SetNull() ;

    bool LoadMinVersion(int nVersion);

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
    int GetVersion() ;

    bool SetMinVersion(enum WalletFeature nVersion, CWalletDB* pWalletDbIn);

    static CWallet* GetInstance();

    std::tuple<bool,string>  CommitTx(CBaseTx *pTx);
};


typedef map<string, string> mapValue_t;

static void ReadOrderPos(int64_t& nOrderPos, mapValue_t& mapValue) {
    if (!mapValue.count("n")) {
        nOrderPos = -1; // TODO: calculate elsewhere
        return;
    }
    nOrderPos = atoi64(mapValue["n"].c_str());
}

static void WriteOrderPos(const int64_t& nOrderPos, mapValue_t& mapValue) {
    if (nOrderPos == -1)
        return;
    mapValue["n"] = i64tostr(nOrderPos);
}

/** Private key that includes an expiration date in case it never gets used. */
class CWalletKey {
public:
    CPrivKey vchPrivKey;
    int64_t nTimeCreated;
    int64_t nTimeExpires;
    string strComment;
    //// todo: add something to note what created it (user, getnewaddr, change)
    ////   maybe should have a map<string, string> property map

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

/** Account information.
 * Stored in wallet with key "acc"+string account name.
 */
class CAccountInfo {
public:
    CPubKey vchPubKey;

    CAccountInfo() {
        SetNull();
    }

    void SetNull() {
        vchPubKey = CPubKey();
    }

    IMPLEMENT_SERIALIZE
    (
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
            READWRITE(vchPubKey);
    )
};

/** Internal transfers.
 * Database key is acentry<account><counter>.
 */
class CAccountingEntry {
public:
    string strAccount;
    int64_t nCreditDebit;
    int64_t nTime;
    string strOtherAccount;
    string strComment;
    mapValue_t mapValue;
    int64_t nOrderPos;  // position in ordered transaction list
    uint64_t nEntryNo;

    CAccountingEntry() {
        SetNull();
    }

    void SetNull() {
        nCreditDebit = 0;
        nTime = 0;
        strAccount.clear();
        strOtherAccount.clear();
        strComment.clear();
        nOrderPos = -1;
    }

    IMPLEMENT_SERIALIZE
    (
        CAccountingEntry& me = *const_cast<CAccountingEntry*>(this);
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
            // Note: strAccount is serialized as part of the key, not here.
            READWRITE(nCreditDebit);
            READWRITE(nTime);
            READWRITE(strOtherAccount);

        if (!fRead) {
            WriteOrderPos(nOrderPos, me.mapValue);

            if (!(mapValue.empty() && _ssExtra.empty())) {
                CDataStream ss(nType, nVersion);
                ss.insert(ss.begin(), '\0');
                ss << mapValue;
                ss.insert(ss.end(), _ssExtra.begin(), _ssExtra.end());
                me.strComment.append(ss.str());
            }
        }

        READWRITE(strComment);

        size_t nSepPos = strComment.find("\0", 0, 1);
        if (fRead) {
            me.mapValue.clear();
            if (string::npos != nSepPos) {
                CDataStream ss(vector<char>(strComment.begin() + nSepPos + 1, strComment.end()), nType, nVersion);
                ss >> me.mapValue;
                me._ssExtra = vector<char>(ss.begin(), ss.end());
            }
            ReadOrderPos(me.nOrderPos, me.mapValue);
        }
        if (string::npos != nSepPos)
        me.strComment.erase(nSepPos);
        me.mapValue.erase("n");
    )

private:
    vector<char> _ssExtra;
};

class CAccountTx {
private:
    CWallet* pWallet;

public:
    uint256 blockHash;
    int blockHeight;
    map<uint256, std::shared_ptr<CBaseTx> > mapAccountTx;
public:
    CAccountTx(CWallet* pWalletIn = NULL, uint256 hash = uint256(), int height = 0) {
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
        switch (pTx->nTxType) {
        case BCOIN_TRANSFER_TX:
            mapAccountTx[hash] = std::make_shared<CBaseCoinTransferTx>(pTx);
            break;
        case CONTRACT_INVOKE_TX:
            mapAccountTx[hash] = std::make_shared<CContractInvokeTx>(pTx);
            break;
        case ACCOUNT_REGISTER_TX:
            mapAccountTx[hash] = std::make_shared<CAccountRegisterTx>(pTx);
            break;
        case BLOCK_REWARD_TX:
            mapAccountTx[hash] = std::make_shared<CBlockRewardTx>(pTx);
            break;
        case CONTRACT_DEPLOY_TX:
            mapAccountTx[hash] = std::make_shared<CContractDeployTx>(pTx);
            break;
        case DELEGATE_VOTE_TX:
            mapAccountTx[hash] = std::make_shared<CDelegateVoteTx>(pTx);
            break;
        case COMMON_MTX:
            mapAccountTx[hash] = std::make_shared<CMulsigTx>(pTx);
            break;
        default:
            return false;
        }
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
