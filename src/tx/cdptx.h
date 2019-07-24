// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_CDP_H
#define TX_CDP_H

#include "tx.h"

class CUserCDP;

/**
 *  Interest Ratio Formula: ( a / Log10(b + N) )
 *
 *  ==> ratio = a / Log10 (b+N)
 */
bool ComputeCdpInterest(const int32_t currBlockHeight, const uint32_t cpdLastBlockHeight, CCacheWrapper &cw,
                        const uint64_t &totalOwedScoins, uint64_t &interest);

/**
 * Stake or ReStake bcoins into a CDP
 */
class CCDPStakeTx: public CBaseTx {

public:
    CCDPStakeTx() : CBaseTx(CDP_STAKE_TX) {}

    CCDPStakeTx(const CBaseTx *pBaseTx): CBaseTx(CDP_STAKE_TX) {
        *this = *(CCDPStakeTx *)pBaseTx;
    }
    /** Newly open a CDP */
    CCDPStakeTx(const CUserID &txUidIn, uint64_t feesIn, int validHeightIn,
                uint64_t bcoinsToStakeIn, uint64_t scoinsToMintIn):
                CBaseTx(CDP_STAKE_TX, txUidIn, validHeightIn, feesIn) {
        bcoinsToStake   = bcoinsToStakeIn;
        scoinsToMint    = scoinsToMintIn;
    }
    /** Stake an existing CDP */
    CCDPStakeTx(const CUserID &txUidIn, uint64_t feesIn, int validHeightIn, uint256 cdpTxIdIn,
                uint64_t bcoinsToStakeIn, uint64_t scoinsToMintIn)
        : CBaseTx(CDP_STAKE_TX, txUidIn, validHeightIn, feesIn) {
        if (txUidIn.type() == typeid(CRegID)) {
            assert(!txUidIn.get<CRegID>().IsEmpty());
        }

        cdpTxId       = cdpTxIdIn;
        bcoinsToStake = bcoinsToStakeIn;
        scoinsToMint  = scoinsToMintIn;
    }

    ~CCDPStakeTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(VARINT(llFees));
        READWRITE(cdpTxId);
        READWRITE(VARINT(bcoinsToStake));
        READWRITE(VARINT(scoinsToMint));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << VARINT(llFees)
                << cdpTxId << VARINT(bcoinsToStake) << VARINT(scoinsToMint);
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual map<CoinType, uint64_t> GetValues() const { return map<CoinType, uint64_t>{{CoinType::WICC, bcoinsToStake}}; }
    virtual TxID GetHash() const { return ComputeSignatureHash(); }
    // virtual uint64_t GetFees() const { return llFees; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CCDPStakeTx>(this); }

    virtual string ToString(CAccountDBCache &view);
    virtual Object ToJson(const CAccountDBCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
private:
    bool SellInterestForFcoins(const uint64_t scoinsInterestToRepay, CCacheWrapper &cw, CValidationState &state);

private:
    TxID        cdpTxId;            //optional: only required for staking existing CDPs
    TokenSymbol bcoinSymbol;        //optional: only reuiqred for 1st-time CDP staking
    TokenSymbol scoinSymbol;        //ditto
    uint64_t    bcoinsToStake;      // base coins amount to stake or collateralize
    uint64_t    scoinsToMint;       // initial collateral ratio must be >= 190 (%), boosted by 10000
};

/**
 * Redeem scoins into a CDP fully or partially
 * Need to pay interest or stability fees
 */
class CCDPRedeemTx: public CBaseTx {
public:
    CCDPRedeemTx() : CBaseTx(CDP_REDEEM_TX) {}

    CCDPRedeemTx(const CBaseTx *pBaseTx): CBaseTx(CDP_REDEEM_TX) {
        *this = *(CCDPRedeemTx *)pBaseTx;
    }

    CCDPRedeemTx(const CUserID &txUidIn, uint64_t feesIn, int validHeightIn,
                uint256 cdpTxIdIn, uint64_t scoinsToRepayIn, uint64_t bcoinsToRedeemIn):
                CBaseTx(CDP_REDEEM_TX, txUidIn, validHeightIn, feesIn) {
        if (txUidIn.type() == typeid(CRegID)) {
            assert(!txUidIn.get<CRegID>().IsEmpty());
        }
        cdpTxId        = cdpTxIdIn;
        scoinsToRepay  = scoinsToRepayIn;
        bcoinsToRedeem = bcoinsToRedeemIn;
    }

    ~CCDPRedeemTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(VARINT(llFees));

        READWRITE(cdpTxId);
        READWRITE(VARINT(scoinsToRepay));
        READWRITE(VARINT(bcoinsToRedeem));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << VARINT(llFees)
                << cdpTxId << VARINT(scoinsToRepay) << VARINT(bcoinsToRedeem);
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual map<CoinType, uint64_t> GetValues() const { return map<CoinType, uint64_t>{{CoinType::WUSD, bcoinsToRedeem}}; }
    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    // virtual uint64_t GetFees() const { return llFees; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CCDPRedeemTx>(this); }

    virtual string ToString(CAccountDBCache &view);
    virtual Object ToJson(const CAccountDBCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int32_t nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
private:
    bool SellInterestForFcoins(const int nHeight, const CUserCDP &cdp, CCacheWrapper &cw, CValidationState &state);

private:
    uint256 cdpTxId;          // CDP cdpTxId
    uint64_t scoinsToRepay;   // stableCoins amount to redeem or burn, including interest
    uint64_t bcoinsToRedeem;
};

/**
 * Liquidate a CDP
 */
class CCDPLiquidateTx: public CBaseTx {

public:
    CCDPLiquidateTx() : CBaseTx(CDP_LIQUIDATE_TX) {}

    CCDPLiquidateTx(const CBaseTx *pBaseTx): CBaseTx(CDP_LIQUIDATE_TX) {
        *this = *(CCDPLiquidateTx *)pBaseTx;
    }

    CCDPLiquidateTx(const CUserID &txUidIn, uint64_t feesIn, int validHeightIn,
                    uint256 cdpTxIdIn, uint64_t scoinsToLiquidateIn):
                CBaseTx(CDP_LIQUIDATE_TX, txUidIn, validHeightIn, feesIn) {

        if (txUidIn.type() == typeid(CRegID)) {
            assert(!txUidIn.get<CRegID>().IsEmpty());
        }

        cdpTxId             = cdpTxIdIn;
        scoinsToLiquidate   = scoinsToLiquidateIn;
    }

    ~CCDPLiquidateTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(VARINT(llFees));

        READWRITE(cdpTxId);
        READWRITE(scoinsToLiquidate);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << VARINT(llFees)
                << cdpTxId << VARINT(scoinsToLiquidate);
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual map<CoinType, uint64_t> GetValues() const {
        return map<CoinType, uint64_t>{{CoinType::WUSD, scoinsToLiquidate}};
    }
    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    // virtual uint64_t GetFees() const { return llFees; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CCDPLiquidateTx>(this); }

    virtual string ToString(CAccountDBCache &view);
    virtual Object ToJson(const CAccountDBCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);

private:
    bool SellPenaltyForFcoins(uint64_t scoinPenaltyFees, const int nHeight,
                            const CUserCDP &cdp, CCacheWrapper &cw, CValidationState &state);

private:
    uint256     cdpTxId;            // target CDP to liquidate
    uint64_t    scoinsToLiquidate;  // partial liquidation is allowed, must include penalty fees in
};

#endif //TX_CDP_H