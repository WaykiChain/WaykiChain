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
#include "entities/account.h"
#include "commons/json/json_spirit_value.h"
#include "commons/json/json_spirit_writer_template.h"
#include "commons/json/json_spirit_utils.h"
#include "commons/serialize.h"
#include "crypto/hash.h"
#include "commons/util/util.h"
#include "main.h"
#include "vm/luavm/luavmrunenv.h"
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

bool GetTxMinFee(const TxType nTxType, int height, const TokenSymbol &symbol, uint64_t &feeOut) {
    const auto &iter = kTxFeeTable.find(nTxType);
    if (iter != kTxFeeTable.end()) {
        FeatureForkVersionEnum version = GetFeatureForkVersion(height);
        if (symbol == SYMB::WICC) {
            if (version >= MAJOR_VER_R2) {
                feeOut = std::get<2>(iter->second);
                return true;
            } else { //MAJOR_VER_R1  //Prior-stablecoin Release
                feeOut = std::get<1>(iter->second);
                return true;
            }
        } else if (symbol == SYMB::WUSD) {
            if (version >= MAJOR_VER_R2){
                feeOut = std::get<4>(iter->second);
                return true;
            } { //MAJOR_VER_R1 //Prior-stablecoin Release
                feeOut = std::get<3>(iter->second);
                return true;
            }
        }
    }
    return false;

}

bool CBaseTx::IsValidHeight(int32_t nCurrHeight, int32_t nTxCacheHeight) const {
    if (BLOCK_REWARD_TX == nTxType || UCOIN_BLOCK_REWARD_TX == nTxType || PRICE_MEDIAN_TX == nTxType)
        return true;

    if (valid_height > nCurrHeight + nTxCacheHeight / 2)
        return false;

    if (valid_height < nCurrHeight - nTxCacheHeight / 2)
        return false;

    return true;
}

bool CBaseTx::GenerateRegID(CTxExecuteContext &context, CAccount &account) {
    if (txUid.is<CPubKey>()) {
        account.owner_pubkey = txUid.get<CPubKey>();

        CRegID regId;
        if (context.pCw->accountCache.GetRegId(txUid, regId)) {
            // account has regid already, return
            return true;
        }

        // generate a new regid for the account
        account.regid = CRegID(context.height, context.index);
        if (!context.pCw->accountCache.SaveAccount(account))
            return context.pState->DoS(100, ERRORMSG("CBaseTx::GenerateRegID, save account info error"), WRITE_ACCOUNT_FAIL,
                             "bad-write-accountdb");
    }

    return true;
}

uint64_t CBaseTx::GetFuel(int32_t height, uint32_t fuelRate) {
    return (nRunStep == 0 || fuelRate == 0) ? 0 : std::ceil(nRunStep / 100.0f) * fuelRate;
}

Object CBaseTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;
    CKeyID srcKeyId;
    accountCache.GetKeyId(txUid, srcKeyId);
    result.push_back(Pair("txid",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("tx_uid",         txUid.ToString()));
    result.push_back(Pair("from_addr",      srcKeyId.ToAddress()));
    result.push_back(Pair("fee_symbol",     fee_symbol));
    result.push_back(Pair("fees",           llFees));
    result.push_back(Pair("valid_height",   valid_height));
    result.push_back(Pair("signature",      HexStr(signature)));
    return result;
}

bool CBaseTx::CheckTxFeeSufficient(const TokenSymbol &feeSymbol, const uint64_t llFees, const int32_t height) const {
    uint64_t minFee;
    if (!GetTxMinFee(nTxType, height, feeSymbol, minFee)) {
        assert(false && "Get tx min fee for WICC or WUSD");
        return false;
    }
    return llFees >= minFee;
}

// Transactions should check the signature size before verifying signature
bool CBaseTx::CheckSignatureSize(const vector<unsigned char> &signature) const {
    return signature.size() > 0 && signature.size() < MAX_SIGNATURE_SIZE;
}

string CBaseTx::ToString(CAccountDBCache &accountCache) {
    return strprintf("txType=%s, hash=%s, ver=%d, pubkey=%s, llFees=%llu, keyid=%s, valid_height=%d",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.get<CPubKey>().ToString(), llFees,
                     txUid.get<CPubKey>().GetKeyId().ToAddress(), valid_height);
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

bool CBaseTx::CheckCoinRange(const TokenSymbol &symbol, const int64_t amount) const {
    if (symbol == SYMB::WICC) {
        return CheckBaseCoinRange(amount);
    } else if (symbol == SYMB::WGRT) {
        return CheckFundCoinRange(amount);
    } else if (symbol == SYMB::WUSD) {
        return CheckStableCoinRange(amount);
    } else {
        // TODO: need to check other token range
        return amount >= 0;
    }
}


/**################################ Universal Coin Transfer ########################################**/

string SingleTransfer::ToString(const CAccountDBCache &accountCache) const {
    return strprintf("to_uid=%s, coin_symbol=%s, coin_amount=%llu", to_uid.ToDebugString(), coin_symbol, coin_amount);
}

Object SingleTransfer::ToJson(const CAccountDBCache &accountCache) const {
    Object result;

    CKeyID desKeyId;
    accountCache.GetKeyId(to_uid, desKeyId);
    result.push_back(Pair("to_uid",      to_uid.ToString()));
    result.push_back(Pair("to_addr",     desKeyId.ToAddress()));
    result.push_back(Pair("coin_symbol", coin_symbol));
    result.push_back(Pair("coin_amount", coin_amount));

    return result;
}