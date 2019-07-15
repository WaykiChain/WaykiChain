// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <algorithm>

#include "tx.h"
#include "persistence/accountdb.h"
#include "persistence/contractdb.h"
#include "persistence/txdb.h"
#include "accounts/account.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_utils.h"
#include "commons/serialize.h"
#include "crypto/hash.h"
#include "commons/util.h"
#include "main.h"
#include "vm/luavm/vmrunenv.h"
#include "miner/miner.h"
#include "config/version.h"

using namespace json_spirit;

string GetTxType(const TxType txType) {
    auto it = kTxFeeTable.find(txType);
    if (it != kTxFeeTable.end())
        return std::get<0>(it->second);
    else
        return "";
}

uint64_t GetTxMinFee(const TxType nTxType, int nHeight) {
    const auto &iter = kTxFeeTable.find(nTxType);
    switch (GetFeatureForkVersion(nHeight)) {
        case MAJOR_VER_R1: // Prior-stablecoin Release
            return iter != kTxFeeTable.end() ? std::get<1>(iter->second) : 0;

        case MAJOR_VER_R2:  // StableCoin Release
            return iter != kTxFeeTable.end() ? std::get<2>(iter->second) : 0;

        default:
            return 10000; //10^-8 WICC
    }
}

bool CBaseTx::IsValidHeight(int32_t nCurrHeight, int32_t nTxCacheHeight) const {
    if (BLOCK_REWARD_TX == nTxType || BLOCK_PRICE_MEDIAN_TX == nTxType)
        return true;

    if (nValidHeight > nCurrHeight + nTxCacheHeight / 2)
        return false;

    if (nValidHeight < nCurrHeight - nTxCacheHeight / 2)
        return false;

    return true;
}

uint64_t CBaseTx::GetFuel(int32_t nFuelRate) {
    uint64_t llFuel = ceil(nRunStep/100.0f) * nFuelRate;
    if (CONTRACT_DEPLOY_TX == nTxType) {
        if (llFuel < 1 * COIN) {
            llFuel = 1 * COIN;
        }
    }
    return llFuel;
}

int32_t CBaseTx::GetFuelRate(CContractDBCache &scriptDB) {
    if (nFuelRate > 0)
        return nFuelRate;

    CDiskTxPos txPos;
    if (scriptDB.ReadTxIndex(GetHash(), txPos)) {
        CAutoFile file(OpenBlockFile(txPos, true), SER_DISK, CLIENT_VERSION);
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

bool CBaseTx::CheckTxFeeSufficient(const uint64_t llFees, const int32_t nHeight) const {
    const auto &iter = kTxFeeTable.find(nTxType);

    switch (GetFeatureForkVersion(nHeight) ) {

        case MAJOR_VER_R1: // Prior-stablecoin Release
            return iter != kTxFeeTable.end() ? (llFees >= std::get<1>(iter->second)) : true;

        case MAJOR_VER_R2:  // StableCoin Release
            return iter != kTxFeeTable.end() ? (llFees >= std::get<2>(iter->second)) : true;

        default:
            return true;
    }
}

// Transactions should check the signature size before verifying signature
bool CBaseTx::CheckSignatureSize(const vector<unsigned char> &signature) const {
    return signature.size() > 0 && signature.size() < MAX_BLOCK_SIGNATURE_SIZE;
}

string CBaseTx::ToString(CAccountDBCache &view) {
    string str = strprintf("txType=%s, hash=%s, ver=%d, pubkey=%s, llFees=%ld, keyid=%s, nValidHeight=%d\n",
                            GetTxType(nTxType), GetHash().ToString(), nVersion,
                            txUid.get<CPubKey>().ToString(),
                            llFees, txUid.get<CPubKey>().GetKeyId().ToAddress(), nValidHeight);

    return str;
}

bool CBaseTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    return AddInvolvedKeyIds({txUid}, cw, keyIds);
}

bool CBaseTx::AddInvolvedKeyIds(vector<CUserID> uids, CCacheWrapper &cw, set<CKeyID> &keyIds) {
    for (auto uid : uids) {
        CKeyID keyId;
        if (!cw.accountCache.GetKeyId(uid, keyId))
            return false;

        keyIds.insert(keyId);
    }
    return true;
}

bool CBaseTx::SaveTxAddresses(uint32_t height, uint32_t index, CCacheWrapper &cw,
                              CValidationState &state, const vector<CUserID> &userIds) {
    if (SysCfg().GetAddressToTxFlag()) {
        for (auto userId : userIds) {
            if (userId.type() != typeid(CNullID)) {
                CKeyID keyId;
                if (!cw.accountCache.GetKeyId(userId, keyId))
                    return state.DoS(100, ERRORMSG("CBaseTx::SaveTxAddresses, get keyid by uid error"),
                                    READ_ACCOUNT_FAIL, "bad-get-keyid-uid");

                if (!cw.contractCache.SetTxHashByAddress(keyId, height, index + 1,
                                                         cw.txUndo.txid, cw.txUndo.dbOpLogMap))
                    return state.DoS(100, ERRORMSG("CBaseTx::SaveTxAddresses, SetTxHashByAddress to db cache failed!"),
                                    READ_ACCOUNT_FAIL, "bad-set-txHashByAddress");
            }
        }
    }
    return true;
}

bool CBaseTx::UndoTxAddresses(CCacheWrapper &cw, CValidationState &state) {
    if (!cw.contractCache.UndoTxHashByAddress(cw.txUndo.dbOpLogMap))
        return state.DoS(100, ERRORMSG("CBaseTx::UndoTxAddresses failed!"), READ_ACCOUNT_FAIL, "undo-data-failed");

    return true;
}
