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
class CCdpStakeTx: public CBaseTx {
public:
    uint64_t bcoinsToStake;         // base coins amount to stake or collateralize
    uint64_t collateralRatio;       // must be >= 200 (%)
    uint64_t fcoinsInterest;        // Interest will be deducted from bcoinsToStake when 0
                                    // For the first-time staking, no interest shall be paid though

public:
    CCdpStakeTx() : CBaseTx(CDP_STAKE_TX) {}

    CCdpStakeTx(const CBaseTx *pBaseTx): CBaseTx(CDP_STAKE_TX) {
        *this = *(CCdpStakeTx *)pBaseTx;
    }

    CCdpStakeTx(const CUserID &txUidIn, uint64_t feesIn, int validHeightIn,
                uint64_t bcoinsToStakeIn, uint64_t collateralRatioIn, uint64_t fcoinsInterestIn):
                CBaseTx(CDP_STAKE_TX, txUidIn, validHeightIn, feesIn) {
        if (txUidIn.type() == typeid(CRegID)) {
            assert(!txUidIn.get<CRegID>().IsEmpty());
        }

        bcoinsToStake = bcoinsToStakeIn;
        collateralRatio = collateralRatioIn;
        fcoinsInterest = fcoinsInterestIn;
    }

    ~CCdpStakeTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(VARINT(llFees));
        READWRITE(VARINT(bcoinsToStake));
        READWRITE(VARINT(collateralRatio));
        READWRITE(VARINT(fcoinsInterest));

        READWRITE(signature);
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid
               << VARINT(llFees) << VARINT(bcoinsToStake) << VARINT(collateralRatio) << VARINT(fcoinsInterest);
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual uint64_t GetValue() const { return bcoinsToStake; }
    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual uint64_t GetFee() const { return llFees; }
    virtual double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CCdpStakeTx>(this); }

    virtual string ToString(CAccountCache &view);
    virtual Object ToJson(const CAccountCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);

};

/**
 * Redeem scoins into a CDP fully or partially
 * Need to pay interest or stability fees
 */
class CCdpRedeemTx: public CBaseTx {
public:
    uint64_t scoinsToRedeem;    // stableCoins amount to redeem or burn
    uint64_t fcoinsInterest;    // Interest will be deducted from scoinsToRedeem when 0
                                // For the first-time staking, no interest shall be paid though

public:
    CCdpRedeemTx() : CBaseTx(CDP_REDEEMP_TX) {}

    CCdpRedeemTx(const CBaseTx *pBaseTx): CBaseTx(CDP_REDEEMP_TX) {
        *this = *(CCdpRedeemTx *)pBaseTx;
    }

    CCdpRedeemTx(const CUserID &txUidIn, uint64_t feesIn, int validHeightIn,
                uint64_t scoinsToRedeemIn, uint64_t fcoinsInterestIn):
                CBaseTx(CDP_REDEEMP_TX, txUidIn, validHeightIn, feesIn) {
        if (txUidIn.type() == typeid(CRegID)) {
            assert(!txUidIn.get<CRegID>().IsEmpty());
        }

        scoinsToRedeem = scoinsToRedeemIn;
        fcoinsInterest = fcoinsInterestIn;
    }

    ~CCdpRedeemTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(VARINT(llFees));
        READWRITE(VARINT(scoinsToRedeem));
        READWRITE(VARINT(fcoinsInterest));

        READWRITE(signature);
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid
               << VARINT(llFees) << VARINT(scoinsToRedeem) << VARINT(fcoinsInterest);
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual uint64_t GetValue() const { return scoinsToRedeem; }
    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual uint64_t GetFee() const { return llFees; }
    virtual double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CCdpRedeemTx>(this); }

    virtual string ToString(CAccountCache &view);
    virtual Object ToJson(const CAccountCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
};

/**
 * Liquidate a CDP
 */
class CCdpLiquidateTx: public CBaseTx {
public:
    uint64_t scoinsToRedeem;    // stable coins to redeem base coins
    uint64_t fcoinsInterest;    // Interest will be deducted from scoinsToRedeem when 0
                                // For the first-time staking, no interest shall be paid though
public:
    CCdpLiquidateTx() : CBaseTx(CDP_LIQUIDATE_TX) {}

    CCdpLiquidateTx(const CBaseTx *pBaseTx): CBaseTx(CDP_LIQUIDATE_TX) {
        *this = *(CCdpLiquidateTx *)pBaseTx;
    }

    CCdpLiquidateTx(const CUserID &txUidIn, uint64_t feesIn, int validHeightIn,
                uint64_t scoinsToRedeemIn, uint64_t fcoinsInterestIn):
                CBaseTx(CDP_LIQUIDATE_TX, txUidIn, validHeightIn, feesIn) {
        if (txUidIn.type() == typeid(CRegID)) {
            assert(!txUidIn.get<CRegID>().IsEmpty());
        }

        scoinsToRedeem = scoinsToRedeemIn;
        fcoinsInterest = fcoinsInterestIn;
    }

    ~CCdpLiquidateTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(VARINT(llFees));
        READWRITE(VARINT(bcoinsToStake));
        READWRITE(VARINT(fcoinsInterest));

        READWRITE(signature);
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid << appUid
               << VARINT(llFees) << VARINT(bcoinsToStake);
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual uint64_t GetValue() const { return bcoinsToStake; }
    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual uint64_t GetFee() const { return llFees; }
    virtual double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CCdpLiquidateTx>(this); }

    virtual string ToString(CAccountCache &view);
    virtual Object ToJson(const CAccountCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
};

#endif //TX_CDP_H