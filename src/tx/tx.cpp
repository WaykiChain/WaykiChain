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
#include "util.h"
#include "main.h"
#include "vm/vmrunenv.h"
#include "miner/miner.h"
#include "version.h"

using namespace json_spirit;

string GetTxType(const unsigned char txType) {
    auto it = kTxTypeMap.find(txType);
    if (it != kTxTypeMap.end())
        return it->second;
    else
        return "";
}

bool CBaseTx::IsValidHeight(int nCurrHeight, int nTxCacheHeight) const {
    if (BLOCK_REWARD_TX == nTxType || BLOCK_PRICE_MEDIAN_TX == nTxType)
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

int CBaseTx::GetFuelRate(CContractCache &scriptDB) {
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
bool CBaseTx::CheckMinTxFee(const uint64_t llFees, const int nHeight) const {
    if (GetFeatureForkVersion(nHeight) == MAJOR_VER_R2 )
        return llFees >= nMinTxFee;

    return true;
}

// transactions should check the signature size before verifying signature
bool CBaseTx::CheckSignatureSize(const vector<unsigned char> &signature) const {
    return signature.size() > 0 && signature.size() < MAX_BLOCK_SIGNATURE_SIZE;
}

string CBaseTx::ToString(CAccountCache &view) {
    string str = strprintf("txType=%s, hash=%s, ver=%d, pubkey=%s, llFees=%ld, keyid=%s, nValidHeight=%d\n",
                            GetTxType(nTxType), GetHash().ToString().c_str(), nVersion,
                            txUid.get<CPubKey>().ToString(),
                            llFees, txUid.get<CPubKey>().GetKeyId().ToAddress(), nValidHeight);

    return str;
}

bool CBaseTx::SaveTxAddresses(int nHeight, int nIndex, CCacheWrapper &cw,
                              const vector<CUserID> &userIds) {
    if (SysCfg().GetAddressToTxFlag()) {
        CDbOpLogs &opLogs = cw.txUndo.mapDbOpLogs[DB_OP_CONTRACT];
        CDbOpLog operAddressToTxLog;
        for (auto userId : userIds) {
            if (userId.type() != typeid(CNullID)) {
                CKeyID keyId;
                if (!cw.accountCache.GetKeyId(userId, keyId))
                    return ERRORMSG("SaveTxAddresses, get keyid by uid error!");

                if (!cw.contractCache.SetTxHashByAddress(keyId, nHeight, nIndex + 1,
                                                         cw.txUndo.txHash, operAddressToTxLog))
                    return ERRORMSG("SaveTxAddresses, SetTxHashByAddress to db cache failed!");

                opLogs.push_back(operAddressToTxLog);
            }
        }
    }
    return true;
}
