// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_CDP_H
#define TX_CDP_H

#include "tx.h"

/**
 * Stake or ReStake bcoins into a CDP
 */
class CCDPStakeTx: public CBaseTx {

public:
    CCDPStakeTx() : CBaseTx(CDP_STAKE_TX) {}

    CCDPStakeTx(const CBaseTx *pBaseTx): CBaseTx(CDP_STAKE_TX) {
        *this = *(CCDPStakeTx *)pBaseTx;
    }

    CCDPStakeTx(const CUserID &txUidIn, uint64_t feesIn, int validHeightIn,
                CTxCord cdpTxCordIn, uint64_t bcoinsToStakeIn, uint64_t collateralRatioIn,
                uint64_t fcoinsInterestIn, uint64_t scoinsInterestIn):
                CBaseTx(CDP_STAKE_TX, txUidIn, validHeightIn, feesIn) {
        if (txUidIn.type() == typeid(CRegID)) {
            assert(!txUidIn.get<CRegID>().IsEmpty());
        }

        cdpTxCord       = cdpTxCordIn;
        bcoinsToStake   = bcoinsToStakeIn;
        collateralRatio = collateralRatioIn;
        fcoinsInterest  = fcoinsInterestIn;
        scoinsInterest  = scoinsInterestIn;
    }

    ~CCDPStakeTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(VARINT(llFees));
        READWRITE(cdpTxCord);
        READWRITE(VARINT(bcoinsToStake));
        READWRITE(VARINT(collateralRatio));
        READWRITE(VARINT(fcoinsInterest));
        READWRITE(VARINT(scoinsInterest));

        READWRITE(signature);
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << VARINT(llFees)
                << cdpTxCord << VARINT(bcoinsToStake) << VARINT(collateralRatio)
                << VARINT(fcoinsInterest) << VARINT(scoinsInterest);
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual uint64_t GetValue() const { return bcoinsToStake; }
    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual uint64_t GetFee() const { return llFees; }
    virtual double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CCDPStakeTx>(this); }

    virtual string ToString(CAccountDBCache &view);
    virtual Object ToJson(const CAccountDBCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);

private:
    bool PayInterest(int nHeight, CCacheWrapper &cw, CValidationState &state);

private:
    CTxCord cdpTxCord;              // .IsEmpty() will be true if the target CDP does not exist
    uint64_t bcoinsToStake;         // base coins amount to stake or collateralize
    uint64_t collateralRatio;       // initial value must be >= 200 (%)
    uint64_t fcoinsInterest;        // preferred, will be burned immediately
    uint64_t scoinsInterest;        // 3% increase compared to fcoins value, to place buy order of WGRTs to burn

};

/**
 * Redeem scoins into a CDP fully or partially
 * Need to pay interest or stability fees
 */
class CCDPRedeemTx: public CBaseTx {
public:
    CCDPRedeemTx() : CBaseTx(CDP_REDEEMP_TX) {}

    CCDPRedeemTx(const CBaseTx *pBaseTx): CBaseTx(CDP_REDEEMP_TX) {
        *this = *(CCDPRedeemTx *)pBaseTx;
    }

    CCDPRedeemTx(const CUserID &txUidIn, uint64_t feesIn, int validHeightIn,
                CTxCord cdpTxCordIn, uint64_t scoinsToRedeemIn, uint64_t fcoinsInterestIn):
                CBaseTx(CDP_REDEEMP_TX, txUidIn, validHeightIn, feesIn) {
        if (txUidIn.type() == typeid(CRegID)) {
            assert(!txUidIn.get<CRegID>().IsEmpty());
        }
        cdpTxCord = cdpTxCordIn;
        scoinsToRedeem = scoinsToRedeemIn;
        fcoinsInterest = fcoinsInterestIn;
    }

    ~CCDPRedeemTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(VARINT(llFees));
        READWRITE(cdpTxCord);
        READWRITE(VARINT(scoinsToRedeem));
        READWRITE(VARINT(collateralRatio));
        READWRITE(VARINT(fcoinsInterest));

        READWRITE(signature);
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << VARINT(llFees)
                << cdpTxCord << VARINT(scoinsToRedeem) << VARINT(collateralRatio) << VARINT(fcoinsInterest);
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual uint64_t GetValue() const { return scoinsToRedeem; }
    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual uint64_t GetFee() const { return llFees; }
    virtual double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CCDPRedeemTx>(this); }

    virtual string ToString(CAccountDBCache &view);
    virtual Object ToJson(const CAccountDBCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);

private:
    bool PayInterest(int nHeight, CCacheWrapper &cw, CValidationState &state);

private:
    CTxCord cdpTxCord;          // CDP TxCord
    uint64_t scoinsToRedeem;    // stableCoins amount to redeem or burn
    uint64_t collateralRatio;   // must be >= 150 (%)
    uint64_t fcoinsInterest;    // Interest will be deducted from scoinsToRedeem when 0
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
                CTxCord cdpTxCordIn, uint64_t scoinsToLiquidateIn, uint64_t fcoinsInterestIn):
                CBaseTx(CDP_LIQUIDATE_TX, txUidIn, validHeightIn, feesIn) {
        if (txUidIn.type() == typeid(CRegID)) {
            assert(!txUidIn.get<CRegID>().IsEmpty());
        }

        cdpTxCord = cdpTxCordIn;
        scoinsToLiquidate = scoinsToLiquidateIn;
        fcoinsPenalty = fcoinsPenaltyIn;
        scoinsPenalty = scoinsPenaltyIn;
    }

    ~CCDPLiquidateTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(VARINT(llFees));
        READWRITE(cdpTxCord);
        READWRITE(scoinsToLiquidate);
        READWRITE(VARINT(fcoinsPenalty));
        READWRITE(VARINT(scoinsPenalty)));

        READWRITE(signature);
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << VARINT(llFees)
                << VARINT(cdpTxCord) << VARINT(scoinsToLiquidate)
                << VARINT(fcoinsPenalty) << VARINT(scoinsPenalty);
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual uint64_t GetValue() const { return bcoinsToStake; }
    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual uint64_t GetFee() const { return llFees; }
    virtual double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CCDPLiquidateTx>(this); }

    virtual string ToString(CAccountDBCache &view);
    virtual Object ToJson(const CAccountDBCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);

private:
    CTxCord cdpTxCord;          // target CDP to liquidate
    uint64_t scoinsToLiquidate; // partial liqudiation is allowed
    uint64_t fcoinsPenalty;
    uint64_t scoinsPenalty;
};

#endif //TX_CDP_H