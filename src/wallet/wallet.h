// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The WaykiChain developers
// Copyright (c) 2009-2013 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef COIN_WALLET_H
#define COIN_WALLET_H

#include "core.h"
#include "crypter.h"
#include "key.h"
#include "keystore.h"
#include "main.h"
#include "ui_interface.h"
#include "util.h"
#include "walletdb.h"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

#include <memory>


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

    CWalletDB *pwalletdbEncryption;

    static bool StartUp(string &strWalletFile);

//  CMasterKey MasterKey;
    int nWalletVersion;
    CBlockLocator  bestBlock;
    uint256 GetCheckSum()const;
public:
    CPubKey vchDefaultKey ;

    bool fFileBacked;         //初始化钱包文件名，为true
    string strWalletFile;     //钱包文件名

    map<uint256, CAccountTx> mapInBlockTx;
    map<uint256, std::shared_ptr<CBaseTx> > UnConfirmTx;
    mutable CCriticalSection cs_wallet;
    //map<CKeyID, CKeyCombi> GetKeyPool() const;

    typedef std::map<unsigned int, CMasterKey> MasterKeyMap;
    MasterKeyMap mapMasterKeys;
    unsigned int nMasterKeyMaxID;

    static string defaultFilename;    //默认钱包文件名  wallet.dat
public:

    IMPLEMENT_SERIALIZE
    (
            LOCK(cs_wallet);
            {
                READWRITE(nWalletVersion);
                READWRITE(bestBlock);
                READWRITE(mapMasterKeys);
                READWRITE(mapInBlockTx);
                READWRITE(UnConfirmTx);
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
    int64_t GetRawBalance(bool IsConfirmed=true)const;

    bool Sign(const CKeyID &keyID,const uint256 &hash,vector<unsigned char> &signature,bool IsMiner=false) const;
    //! Adds an encrypted key to the store, and saves it to disk.
    bool AddCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    bool LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);

    //! Adds a key to the store, without saving it to disk (used by LoadWallet)
    bool LoadKeyCombi(const CKeyID & keyId, const CKeyCombi& keycombi) { return CBasicKeyStore::AddKeyCombi(keyId, keycombi);}
    // Adds a key to the store, and saves it to disk.
    bool AddKey(const CKey& secret,const CKey& minerKey);
    bool AddKey(const CKeyID &keyId, const CKeyCombi& store);
    bool AddKey(const CKey& key);

    bool CleanAll(); //just for unit test
    bool IsReadyForCoolMiner(const CAccountViewCache& view)const;
    bool ClearAllCkeyForCoolMiner();

    CWallet(string strWalletFileIn);
    void SetNull() ;

    bool LoadMinVersion(int nVersion);

    void SyncTransaction(const uint256 &hash, CBaseTx *pTx, const CBlock* pblock);
    void EraseFromWallet(const uint256 &hash);
    int ScanForWalletTransactions(CBlockIndex* pindexStart, bool fUpdate = false);
//  void ReacceptWalletTransactions();
    void ResendWalletTransactions();

    bool IsMine(CBaseTx*pTx)const;

    void SetBestChain(const CBlockLocator& loc);

    DBErrors LoadWallet(bool fFirstRunRet);

    void UpdatedTransaction(const uint256 &hashTx);

    bool EncryptWallet(const SecureString& strWalletPassphrase);

    bool Unlock(const SecureString& strWalletPassphrase);

    bool ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase);

    // get the current wallet format (the oldest client version guaranteed to understand this wallet)
    int GetVersion() ;

    bool SetMinVersion(enum WalletFeature nVersion, CWalletDB* pwalletdbIn);


    static CWallet* getinstance();

    std::tuple<bool,string>  CommitTransaction(CBaseTx *pTx);

//  std::tuple<bool,string>  SendMoney(const CRegID &send,const CUserID &rsv, int64_t nValue, int64_t nFee=0);

    /** Address book entry changed.
     * @note called with lock cs_wallet held.
     */
    boost::signals2::signal<
            void(CWallet *wallet, const CTxDestination &address, const string &label, bool isMine,
                    const string &purpose, ChangeType status)> NotifyAddressBookChanged;

    /** Wallet transaction added, removed or updated.
     * @note called with lock cs_wallet held.
     */
    boost::signals2::signal<void(CWallet *wallet, const uint256 &hashTx, ChangeType status)> NotifyTransactionChanged;

    /** Show progress e.g. for rescan */
    boost::signals2::signal<void(const string &title, int nProgress)> ShowProgress;
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
    CAccountTx(CWallet* pwallet = NULL, uint256 hash = uint256(), int height = 0) {
        pWallet = pwallet;
        blockHash = hash;
        mapAccountTx.clear();
        blockHeight = height;
    }

    ~CAccountTx() { }

    void BindWallet(CWallet* pwallet) {
        if (pWallet == NULL) {
            assert(pwallet != NULL);
            pWallet = pwallet;
        }
    }

    bool AddTx(const uint256 &hash, const CBaseTx *pTx) {
        switch (pTx->nTxType) {
        case COMMON_TX:
            mapAccountTx[hash] = std::make_shared<CCommonTx>(pTx);
            break;
        case CONTRACT_TX:
            mapAccountTx[hash] = std::make_shared<CContractTransaction>(pTx);
            break;
        case REG_ACCT_TX:
            mapAccountTx[hash] = std::make_shared<CRegisterAccountTx>(pTx);
            break;
        case REWARD_TX:
            mapAccountTx[hash] = std::make_shared<CRewardTransaction>(pTx);
            break;
        case REG_CONT_TX:
            mapAccountTx[hash] = std::make_shared<CRegisterContractTx>(pTx);
            break;
        case DELEGATE_TX:
            mapAccountTx[hash] = std::make_shared<CDelegateTransaction>(pTx);
            break;
        default:
//          assert(0);
            return false;
        }
        return true;
    }

    bool HasTx(const uint256 &hash) {
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
