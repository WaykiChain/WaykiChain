// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2016 The Coin developers
// Copyright (c) 2014-2019 The WaykiChain developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <algorithm>

#include "tx.h"
#include "persistence/accountview.h"
#include "persistence/contractdb.h"
#include "persistence/txdb.h"
#include "accounts/account.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_utils.h"
#include "commons/serialize.h"
#include "crypto/hash.h"
#include "util.h"
#include "main.h"
#include "vm/vmrunenv.h"
#include "miner/miner.h"
#include "version.h"

using namespace json_spirit;

const string COperVoteFund::voteOperTypeArray[3] = {"NULL_OPER", "ADD_FUND", "MINUS_FUND"};

string GetTxType(unsigned char txType) {
    auto it = kTxTypeMap.find(txType);
    if (it != kTxTypeMap.end())
        return it->second;
    else
        return "";
}

bool CBaseTx::IsValidHeight(int nCurrHeight, int nTxCacheHeight) const {
    if(BLOCK_REWARD_TX == nTxType)
        return true;

    if (nValidHeight > nCurrHeight + nTxCacheHeight / 2)
        return false;

    if (nValidHeight < nCurrHeight - nTxCacheHeight / 2)
        return false;

    return true;
}

uint64_t CBaseTx::GetFuel(int nfuelRate) {
    uint64_t llFuel = ceil(nRunStep/100.0f) * nfuelRate;
    if (CONTRACT_DEPLOY_TX == nTxType) {
        if (llFuel < 1 * COIN) {
            llFuel = 1 * COIN;
        }
    }
    return llFuel;
}

int CBaseTx::GetFuelRate(CScriptDBViewCache &scriptDB) {
    if (nFuelRate > 0)
        return nFuelRate;

    CDiskTxPos postx;
    if (scriptDB.ReadTxIndex(GetHash(), postx)) {
        CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
        CBlockHeader header;
        try {
            file >> header;
        } catch (std::exception &e) {
            return ERRORMSG("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
        nFuelRate = header.GetFuelRate();
    } else {
        nFuelRate = GetElementForBurn(chainActive.Tip());
    }

    return nFuelRate;
}

// check the fees must be more than nMinTxFee
bool CBaseTx::CheckMinTxFee(const uint64_t llFees) const {
    if (GetFeatureForkVersion(chainActive.Tip()->nHeight) == MAJOR_VER_R2 )
        return llFees >= nMinTxFee;

    return true;
}

// transactions should check the signagure size before verifying signature
bool CBaseTx::CheckSignatureSize(const vector<unsigned char> &signature) const {
    return signature.size() > 0 && signature.size() < MAX_BLOCK_SIGNATURE_SIZE;
}

string CBaseTx::ToString(CAccountViewCache &view) const {
    string str = strprintf("txType=%s, hash=%s, ver=%d, pubkey=%s, llFees=%ld, keyid=%s, nValidHeight=%d\n",
                            GetTxType(nTxType), GetHash().ToString().c_str(), nVersion,
                            txUid.get<CPubKey>().ToString(),
                            llFees, txUid.get<CPubKey>().GetKeyId().ToAddress(), nValidHeight);

    return str;
}

string CTxUndo::ToString() const {
    string str;
    string strTxHash("txHash:");
    strTxHash += txHash.GetHex();
    strTxHash += "\n";

    str += strTxHash;

    string strAccountLog("list account Log:\n");
    vector<CAccountLog>::const_iterator iterLog = vAccountLog.begin();
    for (; iterLog != vAccountLog.end(); ++iterLog) {
        strAccountLog += iterLog->ToString();
    }
    strAccountLog += "\n";

    str += strAccountLog;

    string strDBOperLog("list script db Log:\n");
    vector<CScriptDBOperLog>::const_iterator iterDbLog = vScriptOperLog.begin();
    for (; iterDbLog != vScriptOperLog.end(); ++iterDbLog) {
        strDBOperLog += iterDbLog->ToString();
    }
    strDBOperLog += "\n";

    str += strDBOperLog;

    return str;
}

bool CTxUndo::GetAccountOperLog(const CKeyID &keyId, CAccountLog &accountLog) {
    vector<CAccountLog>::iterator iterLog = vAccountLog.begin();
    for (; iterLog != vAccountLog.end(); ++iterLog) {
        if (iterLog->keyID == keyId) {
            accountLog = *iterLog;
            return true;
        }
    }
    return false;
}