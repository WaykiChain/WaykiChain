// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "pricefeedtx.h"
#include "commons/serialize.h"
#include "tx.h"
#include "crypto/hash.h"
#include "util.h"
#include "main.h"
#include "vm/vmrunenv.h"
#include "miner/miner.h"
#include "version.h"

bool CPriceFeedTx::CheckTx(CCacheWrapper &cw, CValidationState &state) {

    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (pricePoints.size() == 0 || pricePoints.size() > 3) { //FIXME: hardcode here
        return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, tx price points number not within 1..3"),
            REJECT_INVALID, "bad-tx-pricepoint-size-error");
    }
    CAccount account;
    if (!cw.pAccountCache->GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, read txUid %s account info error",
                        txUid.ToString()), PRICE_FEED_FAIL, "bad-read-accountdb");

    if (account.stakedFcoins < kDefaultPriceFeedStakedFcoinsMin) // check if account has sufficient staked fcoins to be a price feeder
        return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, Not stake ready for txUid %s account error",
                        txUid.ToString()), PRICE_FEED_FAIL, "account-stake-not-ready");

    IMPLEMENT_CHECK_TX_SIGNATURE(txUid.get<CPubKey>());
    return true;
}

bool CPriceFeedTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {

    CAccount account;
    if (!cw.pAccountCache->GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CPriceFeedTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), PRICE_FEED_FAIL, "bad-read-accountdb");



    // check if the account is among the top 22 accounts list

    // update the price state accordingly


    return true;
}

bool CPriceFeedTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    //TODO
    return true;

}

Object CPriceFeedTx::ToString(const CAccountCache &AccountView) const {
  //TODO
  return Object();
}

Object CPriceFeedTx::ToJson(const CAccountCache &AccountView) const {
  //TODO
  return Object();
}

bool CPriceFeedTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    //TODO
    return true;
}

bool CPriceFeedTx::GetTopPriceFeederList(CCacheWrapper &cw, vector<CAccount> &priceFeederAccts) {
    LOCK(cs_main);

    return true;
}

//############################################################################################################

bool CheckTx(CValidationState &state, CAccountCache &view, CContractCache &scriptDB) {
    return true;
}

bool CBlockPriceMedianTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    return true;
}

bool CBlockPriceMedianTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    return true;
}

string CBlockPriceMedianTx::ToString(CAccountCache &view) const {
    //TODO
    return "";
}

Object CBlockPriceMedianTx::ToJson(const CAccountCache &AccountView) const {
    //TODO
    return Object();
}

bool CBlockPriceMedianTx::GetInvolvedKeyIds(set<CKeyID> &vAddr, CAccountCache &view, CContractCache &scriptDB) {
    //TODO
    return true;
}

inline uint64_t GetMedianPriceByType(const CoinType coinType, const PriceType priceType) {
    return mapMediaPricePoints[make_tuple<CoinType, PriceType>(coinType, priceType)];
}