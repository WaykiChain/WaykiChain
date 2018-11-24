// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The WaykiChain developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txdb.h"
#include "wallet.h"

#include "base58.h"

#include "configuration.h"
#include "random.h"
#include "net.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"
#include <boost/algorithm/string/replace.hpp>
#include <openssl/rand.h>
using namespace json_spirit;
using namespace boost::assign;

using namespace std;
using namespace boost;




string CWallet:: defaultFilename("");
//////////////////////////////////////////////////////////////////////////////
//
// mapWallet
//

//struct CompareValueOnly {
//	bool operator()(const pair<int64_t, pair<const CWalletTx*, unsigned int> >& t1,
//			const pair<int64_t, pair<const CWalletTx*, unsigned int> >& t2) const {
//		return t1.first < t2.first;
//	}
//};

//const CAccountTx* CWallet::GetAccountTx(const uint256& hash) const {
//	LOCK(cs_wallet);
//	map<uint256, CAccountTx>::const_iterator it = mapWalletTx.find(hash);
//	if (it == mapWalletTx.end())
//		return NULL;
//	return &(it->second);
//}
//
//bool CWallet::GetTx(const uint256& hash,shared_ptr<CBaseTransaction> &tx) const{
//	LOCK(cs_wallet);
//	for(auto &wtx: mapWalletTx) {
//		const CAccountTx &acctx = wtx.second;
//		for(auto &item: acctx.mapAccountTx) {
//			if (item.first == hash) {
//				tx = item.second;
//				return true;
//			}
//		}
//	}
//	return false;
//}
//bool CWallet::AddPubKey(const CPubKey& pk)
//{
//	AssertLockHeld(cs_wallet);
//	CKeyCombi tem(pk);
//	if (mKeyPool.count(tem.GetCKeyID()) > 0) {
//		LogPrint("CWallet", "this key is in the CWallet");
//		return false;
//	}
//	mKeyPool[tem.GetCKeyID()] = tem;
//	return CWalletDB(strWalletFile).WriteKeyStoreValue(tem.GetCKeyID(),tem);
//}


//bool CWallet::AddKey(const CKey& secret) {
//	AssertLockHeld(cs_wallet);
//
//	CKeyStoreValue tem(secret);
//	CKeyID kid = tem.GetCKeyID();
//	if (mKeyPool.count(kid) > 0) {
//		CKey dumy;
//		if (mKeyPool[kid].getCKey(dumy, false) == true) {
//			LogPrint("CWallet", "this key is in the CWallet");
//			return false;
//		}
//	}
//
//	if (!fFileBacked)
//	    return true;
//
//	if (!IsCrypted()) {
//		mKeyPool[tem.GetCKeyID()] = tem;
//		return CWalletDB(strWalletFile).WriteKeyStoreValue(tem.GetCKeyID(),tem);
//	}
//	else
//	{
//		assert(0);//to add code
//	}
//	return true;
//}
//bool CWallet::AddKey(const CKeyCombi& keyCombi) {
//	CPubKey Pk;
//	if (!keyCombi.GetPubKey(Pk)) {
//		return false;
//	}
//	if (!fFileBacked)
//	    return true;
//	if (!IsCrypted()) {
//		return CWalletDB(strWalletFile).WriteKeyStoreValue(Pk.GetKeyID(),keyCombi);
//	} else {
//		assert(0 && "fix me");
//	}
//	return false;
//}

//bool CWallet::AddKey(const CKey& secret,const CKey& minerKey) {
//	AssertLockHeld(cs_wallet);
//
//	CKeyCombi tem(secret,minerKey);
//	if(mKeyPool.count(tem.GetCKeyID()) > 0)	{
//		  LogPrint("CWallet","this key is in the CWallet");
//		 return false;
//		}
//
//	if (!IsCrypted()) {
//		mKeyPool[tem.GetCKeyID()] = tem;
//		return CWalletDB(strWalletFile).WriteKeyStoreValue(tem.GetCKeyID(),tem);
//	}
//	else
//	{
//		assert(0);//to add code
//	}
//	return true;
//}



bool CWallet::Unlock(const SecureString& strWalletPassphrase) {
	CCrypter crypter;
	CKeyingMaterial vMasterKey;

	{
		LOCK(cs_wallet);
		for (const auto& pMasterKey : mapMasterKeys) {
			if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, pMasterKey.second.vchSalt,
					pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
				return false;
			if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
				continue; // try another master key
			if (CCryptoKeyStore::Unlock(vMasterKey))
				return true;
		}
	}
	return false;
}

bool CWallet::ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase,
		const SecureString& strNewWalletPassphrase) {
	bool fWasLocked = IsLocked();

	{
		LOCK(cs_wallet);
		Lock();

		CCrypter crypter;
		CKeyingMaterial vMasterKey;
		for (auto& pMasterKey : mapMasterKeys) {
			if (!crypter.SetKeyFromPassphrase(strOldWalletPassphrase, pMasterKey.second.vchSalt,
					pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
				return false;
			if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
				return false;
			if (CCryptoKeyStore::Unlock(vMasterKey)) {
				int64_t nStartTime = GetTimeMillis();
				crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt,
						pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
				pMasterKey.second.nDeriveIterations = pMasterKey.second.nDeriveIterations
						* (100 / ((double) (GetTimeMillis() - nStartTime)));

				nStartTime = GetTimeMillis();
				crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt,
						pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
				pMasterKey.second.nDeriveIterations = (pMasterKey.second.nDeriveIterations
						+ pMasterKey.second.nDeriveIterations * 100 / ((double) (GetTimeMillis() - nStartTime))) / 2;

				if (pMasterKey.second.nDeriveIterations < 25000)
					pMasterKey.second.nDeriveIterations = 25000;

				LogPrint("INFO","Wallet passphrase changed to an nDeriveIterations of %i\n",
						pMasterKey.second.nDeriveIterations);

				if (!crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt,
						pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
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

void CWallet::SetBestChain(const CBlockLocator& loc) {
	AssertLockHeld(cs_wallet);
	bestBlock = loc;

}

void CWallet::SyncTransaction(const uint256 &hash, CBaseTransaction*pTx, const CBlock* pblock) {
	static std::shared_ptr<vector<string> > monitoring_appid = NULL;
	if(monitoring_appid == NULL)
	{
		monitoring_appid = SysCfg().GetMultiArgsMap("-appid");
	}

	LOCK2(cs_main, cs_wallet);

	assert(pTx != NULL || pblock != NULL);

	if(hash.IsNull() && pTx == NULL) //this is block Sync
	{
		uint256 blockhash = pblock->GetHash();
		auto GenesisBlockProgress = [&]() {
//			unsigned short i = 0;
//			for (const auto &sptx : pblock->vptx) {
//				//confirm the tx GenesisBlock
//				CRewardTransaction* prtx = (CRewardTransaction*) sptx.get();
//				CPubKey pubkey = boost::get<CPubKey>(prtx->account);
//				CAccount account;
//				if (IsMine(sptx.get())) {
//					AddPubKey(pubkey);
//				}
//				i++;
//			}
		};

		auto ConnectBlockProgress = [&]() {
			CAccountTx newtx(this, blockhash,pblock->GetHeight());
			int i=0;
			for (const auto &sptx : pblock->vptx) {
				uint256 hashtx = sptx->GetHash();
				if(sptx->nTxType == CONTRACT_TX){
					string thisapp = boost::get<CRegID>(static_cast<CTransaction const*>(sptx.get())->desUserId).ToString();
					auto it = find_if(monitoring_appid->begin(), monitoring_appid->end(), [&](const string& appid) {
						return appid ==  thisapp;});
			        if(monitoring_appid->end() != it)
					uiInterface.RevAppTransaction(pblock, i);
				}
				//confirm the tx is mine
				if (IsMine(sptx.get())) {
					if (sptx->nTxType == REG_ACCT_TX) {
						//fIsNeedUpDataRegID = true;
					} else if (sptx->nTxType == CONTRACT_TX) {
//						vector<CAccountOperLog> Log;
//						if (GetTxOperLog(hashtx, Log) == true) {
//							assert(newtx.AddOperLog(hashtx, Log));
//						} else {
//							ERRORMSG("GetTxOperLog  error %s", hashtx.GetHex());
//						}
					}
					newtx.AddTx(hashtx,sptx.get());
					uiInterface.RevTransaction(sptx.get()->GetHash());
				}
				if(UnConfirmTx.count(hashtx)> 0){
					CWalletDB(strWalletFile).EraseUnComFirmedTx(hashtx);
					UnConfirmTx.erase(hashtx);
				}
				++i;
			}
			if (newtx.GetTxSize() > 0 ) //write to disk
			{
				mapInBlockTx[blockhash] = newtx; //add to map
				newtx.WriteToDisk();
			}
		};
		auto DisConnectBlockProgress = [&]() {
			int i = 0 ;
			int index = pblock->GetHeight();
			for (const auto &sptx : pblock->vptx) {
				if(sptx->IsCoinBase()){
					continue;
				}
				CRegID regid(index, i++);
				if(IsMine(sptx.get())) {
					UnConfirmTx[sptx.get()->GetHash()] = sptx.get()->GetNewInstance();
					CWalletDB(strWalletFile).WriteUnComFirmedTx(sptx.get()->GetHash(),UnConfirmTx[sptx.get()->GetHash()]);
				}
			}
			if (mapInBlockTx.count(blockhash)) {
				CWalletDB(strWalletFile).EraseBlockTx(blockhash);
				mapInBlockTx.erase(blockhash);
			}
			};
		auto IsConnect = [&]() // test is connect or disconct
			{
				return mapBlockIndex.count(blockhash) && chainActive.Contains(mapBlockIndex[blockhash]);
			};
		// GenesisBlock progress
		if (SysCfg().HashGenesisBlock() == blockhash) {
			GenesisBlockProgress();
		} else if (IsConnect()) {
			//connect block
			ConnectBlockProgress();
		} else {
			//disconnect block
			DisConnectBlockProgress();
		}
	}
	else if (pTx != NULL)
	{
		LogPrint("todo","acept in mempool tx %s\r\n",pTx->GetHash().ToString());
    }


}

void CWallet::EraseFromWallet(const uint256 &hash) {
	if (!fFileBacked)
		return;
	{
		LOCK(cs_wallet);
		if(UnConfirmTx.count(hash)) {
			UnConfirmTx.erase(hash);
			CWalletDB(strWalletFile).EraseUnComFirmedTx(hash);
		}
	}
	return;
}
// Scan the block chain (starting in pindexStart) for transactions
// from or to us. If fUpdate is true, found transactions that already
// exist in the wallet will be updated.
int CWallet::ScanForWalletTransactions(CBlockIndex* pindexStart, bool fUpdate) {

	LogPrint("todo","todo list by ranger.shi");
//	int ret = 0;
//	int64_t nNow = GetTime();
//
//	CBlockIndex* pindex = pindexStart;
//	{
//		LOCK2(cs_main, cs_wallet);
//
//		// no need to read and scan block, if block was created before
//		// our wallet birthday (as adjusted for block time variability)
//		while (pindex && nTimeFirstKey && (pindex->nTime < (nTimeFirstKey - 7200)))
//			pindex = chainActive.Next(pindex);
//
//		ShowProgress(_("Rescanning..."), 0); // show rescan progress in GUI as dialog or on splashscreen, if -rescan on startup
//		double dProgressStart = Checkpoints::GuessVerificationProgress(pindex, false);
//		double dProgressTip = Checkpoints::GuessVerificationProgress(chainActive.Tip(), false);
//		while (pindex) {
//			if (pindex->nHeight % 100 == 0 && dProgressTip - dProgressStart > 0.0)
//				ShowProgress(_("Rescanning..."),
//						max(1,
//								min(99,
//										(int) ((Checkpoints::GuessVerificationProgress(pindex, false) - dProgressStart)
//												/ (dProgressTip - dProgressStart) * 100))));
//
//			CBlock block;
//			ReadBlockFromDisk(block, pindex);
//
//			SyncWithWallets(0, NULL, &block);
//
//			pindex = chainActive.Next(pindex);
//			if (GetTime() >= nNow + 60) {
//				nNow = GetTime();
//				LogPrint("INFO","Still rescanning. At block %d. Progress=%f\n", pindex->nHeight,
//						Checkpoints::GuessVerificationProgress(pindex));
//			}
//		}
//		ShowProgress(_("Rescanning..."), 100); // hide progress dialog in GUI
//	}
	return true;
}

//void CWallet::ReacceptWalletTransactions() {
//	LOCK2(cs_main, cs_wallet);
//
//	mapWalletTx[uint256(0)].AcceptToMemoryPool();
//}

void CWallet::ResendWalletTransactions() {
	vector<uint256> erase;
	for (auto &te : UnConfirmTx) {
		if(mempool.exists(te.first)){ //如果已经存在mempool 了那就不要再提交了
			continue;
		}
		std::shared_ptr<CBaseTransaction> pBaseTx = te.second->GetNewInstance();
		auto ret = CommitTransaction(&(*pBaseTx.get()));
		if (!std::get<0>(ret)) {
			erase.push_back(te.first);
			LogPrint("CWallet","abort inavlibal tx %s reason:%s\r\n",te.second.get()->ToString(*pAccountViewTip),std::get<1>(ret));
		}
	}
	for (auto const & tee : erase) {
		CWalletDB(strWalletFile).EraseUnComFirmedTx(tee);
		uiInterface.RemoveTransaction(tee);
		UnConfirmTx.erase(tee);

	}
}

//// Call after CreateTransaction unless you want to abort
std::tuple<bool, string> CWallet::CommitTransaction(CBaseTransaction *pTx) {
	LOCK2(cs_main, cs_wallet);
	LogPrint("INFO", "CommitTransaction:\n%s", pTx->ToString(*pAccountViewTip));
	{
		CValidationState state;
		if (!::AcceptToMemoryPool(mempool, state, pTx, true, NULL)) {
			// This must not fail. The transaction has already been signed and recorded.
			LogPrint("INFO", "CommitTransaction() : Error: Transaction not valid %s \n",state.GetRejectReason());
			return std::make_tuple (false,state.GetRejectReason());

		}
	}
	uint256 txhash = pTx->GetHash();
	UnConfirmTx[txhash] = pTx->GetNewInstance();
	bool flag =  CWalletDB(strWalletFile).WriteUnComFirmedTx(txhash,UnConfirmTx[txhash]);
	::RelayTransaction(pTx, txhash);
	return std::make_tuple (flag,txhash.ToString());

}

DBErrors CWallet::LoadWallet(bool fFirstRunRet) {
//	  fFirstRunRet = false;
	  return CWalletDB(strWalletFile, "cr+").LoadWallet(this);

}


int64_t CWallet::GetRawBalance(bool IsConfirmed) const
{
	int64_t ret = 0;
	{
		LOCK2(cs_main, cs_wallet);
		set<CKeyID> setKeyId;
		GetKeys(setKeyId);
		for(auto &keyId :setKeyId)
		{
			if(!IsConfirmed)
				ret += mempool.pAccountViewCache->GetRawBalance(keyId);
			else
				ret += pAccountViewTip->GetRawBalance(keyId);
		}
	}
	return ret;
}


//std::tuple<bool,string>  CWallet::SendMoney(const CRegID &send, const CUserID &rsv, int64_t nValue, int64_t nFee)
//{
//
//	CTransaction tx;
//	{
//		LOCK2(cs_main, cs_wallet);
//		tx.srcUserId = send;
//		tx.desUserId = rsv;
//		tx.llValues = nValue;
//		if (0 == nFee) {
//			tx.llFees = SysCfg().GetTxFee();
//		}else
//			tx.llFees = nFee;
//		tx.nValidHeight = chainActive.Tip()->nHeight;
//	}
//
//	CKeyID keID;
//	if(!pAccountViewTip->GetKeyId(send,keID)){
//		return std::make_tuple (false,"key or keID failed");
//	}
//
//	if (!Sign(keID,tx.SignatureHash(), tx.signature)) {
//		return std::make_tuple (false,"Sign failed");
//	}
//	std::tuple<bool,string> ret = CommitTransaction((CBaseTransaction *) &tx);
//	if(!std::get<0>(ret))
//		return ret;
//	return  std::make_tuple (true,tx.GetHash().GetHex());
//
//
//}

bool CWallet::EncryptWallet(const SecureString& strWalletPassphrase)
{
    if (IsCrypted())
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
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = (kMasterKey.nDeriveIterations + kMasterKey.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime))) / 2;

    if (kMasterKey.nDeriveIterations < 25000)
        kMasterKey.nDeriveIterations = 25000;

    LogPrint("INFO", "Encrypting Wallet with an nDeriveIterations of %i\n", kMasterKey.nDeriveIterations);

    if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod))
        return false;
    if (!crypter.Encrypt(vMasterKey, kMasterKey.vchCryptedKey))
        return false;

    {
        LOCK(cs_wallet);
        mapMasterKeys[++nMasterKeyMaxID] = kMasterKey;
        if (fFileBacked)
        {
            assert(!pwalletdbEncryption);
            pwalletdbEncryption = new CWalletDB(strWalletFile);
            if (!pwalletdbEncryption->TxnBegin()) {
                delete pwalletdbEncryption;
                pwalletdbEncryption = NULL;
                return false;
            }
            pwalletdbEncryption->WriteMasterKey(nMasterKeyMaxID, kMasterKey);
        }

        if (!EncryptKeys(vMasterKey))
        {
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

        if (fFileBacked)
        {
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

bool CWallet::SetMinVersion(enum WalletFeature nVersion, CWalletDB* pwalletdbIn)
{
    LOCK(cs_wallet); // nWalletVersion
    if (nWalletVersion >= nVersion)
        return true;

    nWalletVersion = nVersion;
    if (fFileBacked)
    {
        CWalletDB* pwalletdb = pwalletdbIn ? pwalletdbIn : new CWalletDB(strWalletFile);
        pwalletdb->WriteMinVersion(nWalletVersion);
        if (!pwalletdbIn)
            delete pwalletdb;
    }

    return true;
}


void CWallet::UpdatedTransaction(const uint256 &hashTx) {
	{
		LOCK(cs_wallet);
		// Only notify UI if this transaction is in this wallet
//		map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(hashTx);
//		if (mi != mapWallet.end())
		NotifyTransactionChanged(this, hashTx, CT_UPDATED);
	}
}

bool CWallet::StartUp(string &strWalletFile) {
//	[](int i) { return i+4; };
	auto InitError = [] (const string &str)
	{
		uiInterface.ThreadSafeMessageBox(str, "", CClientUIInterface::MSG_WARNING | CClientUIInterface::NOSHOWGUI);
		return true;
	};

	auto InitWarning = [](const string &str)
	{
		uiInterface.ThreadSafeMessageBox(str, "", CClientUIInterface::MSG_WARNING | CClientUIInterface::NOSHOWGUI);
		return true;
	};

	defaultFilename = SysCfg().GetArg("-wallet", "wallet.dat");
//	  bool fDisableWallet = SysCfg().GetBoolArg("-disablewallet", false);
	string strDataDir = GetDataDir().string();

	// Wallet file must be a plain filename without a directory
	if (defaultFilename != boost::filesystem::basename(defaultFilename) + boost::filesystem::extension(defaultFilename))
		return InitError(strprintf(("Wallet %s resides outside data directory %s"), defaultFilename, strDataDir));

	if(strWalletFile =="")
	{
		strWalletFile = defaultFilename;
	}
	LogPrint("INFO", "Using wallet %s\n", strWalletFile);
	uiInterface.InitMessage(_("Verifying wallet..."));

	if (!bitdb.Open(GetDataDir())) {
		// try moving the database env out of the way
		boost::filesystem::path pathDatabase = GetDataDir() / "database";
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
					" restore from a backup."), strDataDir);
			InitWarning(msg);
		}
		if (r == CDBEnv::RECOVER_FAIL)
			return InitError(_("wallet.dat corrupt, salvage failed"));
	}

	return true;
}


CWallet* CWallet::getinstance() {

	string strWalletFile("");
	if(StartUp(strWalletFile))
	{
		return new CWallet(strWalletFile);
	}
//	assert(0);
	return NULL;

}

Object CAccountTx::ToJosnObj(CKeyID const  &key) const {

	Object obj;
	obj.push_back(Pair("blockHash",  blockHash.ToString()));
	obj.push_back(Pair("blockhigh",  blockhigh));
	Array Tx;
	CAccountViewCache view(*pAccountViewTip, true);
	for (auto const &re : mapAccountTx) {
//		if (!key.IsEmpty()) {
//			auto find = mapOperLog.find(re.first);
//			if (find != mapOperLog.end()) {
//				vector<CAccountOperLog>  rep = find->second;
//				for (auto &te : rep) {
//					if (te.keyID == key) {
//						Tx.push_back(re.second.get()->ToString(view));
//					}
//				}
//			}
//		} else			//default add all tx to obj
		{
			Tx.push_back(re.second.get()->ToString(view));
		}
	}
	obj.push_back(Pair("Tx", Tx));

	return obj;
}

uint256 CWallet::GetCheckSum() const {
	CHashWriter ss(SER_GETHASH, CLIENT_VERSION);
	ss << nWalletVersion << bestBlock << mapMasterKeys  << mapInBlockTx << UnConfirmTx;
	return ss.GetHash();
}

bool CWallet::IsMine(CBaseTransaction* pTx) const{

	set<CKeyID> vaddr;
	CAccountViewCache view(*pAccountViewTip, true);
	CScriptDBViewCache scriptDB(*pScriptDBTip, true);
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

	for_each(UnConfirmTx.begin(), UnConfirmTx.end(),
			[&](std::map<uint256, std::shared_ptr<CBaseTransaction> >::reference a) {
		CWalletDB(strWalletFile).EraseUnComFirmedTx(a.first);
			});
	UnConfirmTx.clear();

	for_each(mapInBlockTx.begin(), mapInBlockTx.end(), [&](std::map<uint256, CAccountTx >::reference a) {
		CWalletDB(strWalletFile).EraseUnComFirmedTx(a.first);
	});
	mapInBlockTx.clear();

	bestBlock.SetNull();

	if(!IsCrypted()) {
		for_each(mapKeys.begin(), mapKeys.end(), [&](std::map<CKeyID, CKeyCombi> ::reference item) {
			CWalletDB(strWalletFile).EraseKeyStoreValue(item.first);
		});
		mapKeys.clear();
	}
	else {
		return ERRORMSG("wallet encrypt forbid clear data failed!");
	}
	return true;
}

bool CWallet::Sign(const CKeyID& keyId, const uint256& hash, vector<unsigned char> &signature,bool IsMiner)const {
	CKey key;
	if(GetKey(keyId, key, IsMiner)){
		if(IsMiner == true)
		{
//			cout <<"Sign miner key PubKey:"<< key.GetPubKey().ToString()<< endl;
//			cout <<"Sign miner hash:"<< hash.ToString()<< endl;
		}
		return(key.Sign(hash, signature));
	}
	return false;
}

bool CWallet::AddCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret)
{
    if (!CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret))
        return false;
    if (!fFileBacked)
        return true;
    {
        LOCK(cs_wallet);
        if (pwalletdbEncryption)
            return pwalletdbEncryption->WriteCryptedKey(vchPubKey, vchCryptedSecret);
        else
            return CWalletDB(strWalletFile).WriteCryptedKey(vchPubKey, vchCryptedSecret);
    }
    return false;
}

bool CWallet::LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret)
{
	return CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret);
}
bool CWallet::AddKey(const CKey& key,const CKey& minerKey)
{
	if((!key.IsValid()) || (!minerKey.IsValid()))
		return false;
	CKeyCombi keyCombi(key, minerKey, nWalletVersion);
	return AddKey(key.GetPubKey().GetKeyID(), keyCombi);
}
bool CWallet::AddKey(const CKeyID &KeyId, const CKeyCombi& keyCombi)
{
	if (!fFileBacked)
		return true;

	if(keyCombi.IsContainMainKey()) {
		if(KeyId != keyCombi.GetCKeyID())
			return false;
	}

	if(!CWalletDB(strWalletFile).WriteKeyStoreValue(KeyId, keyCombi, nWalletVersion)) {
		return false;
	}
	return CCryptoKeyStore::AddKeyCombi(KeyId, keyCombi);
}

bool CWallet::AddKey(const CKey& key)
{
	if(!key.IsValid())
		return false;
	CKeyCombi keyCombi(key, nWalletVersion);
	return AddKey(key.GetPubKey().GetKeyID(), keyCombi);
}


bool CWallet::IsReadyForCoolMiner(const CAccountViewCache& view) const {
	CRegID regId;
	for (auto const &item : mapKeys) {
		if (item.second.IsContainMinerKey()&&view.GetRegId(item.first,regId)) {
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

CWallet::CWallet(string strWalletFileIn)  {
	SetNull();
	strWalletFile = strWalletFileIn;
	fFileBacked = true;
}

void CWallet::SetNull() {
	nWalletVersion = 0;
	fFileBacked = false;
	nMasterKeyMaxID = 0;
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



