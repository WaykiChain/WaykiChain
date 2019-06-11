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
class CdpStakeTx: public CBaseTx {
public:
    uint64_t bcoinsToStake;         // base coins amount to stake or collateralize
    uint64_t collateralRatio;       // must be >= 200 (%)
    uint64_t scoinsInterest;        // Interest will be deducted from bcoinsToStake when 0
                                    // For the first-time staking, no interest shall be paid though
    uint64_t fcoinsInterest;

public:
    CdpStakeTx() : CBaseTx(CDP_STAKE_TX) {}

    CdpStakeTx(const CBaseTx *pBaseTx): CBaseTx(CDP_STAKE_TX) {
        *this = *(CdpStakeTx *)pBaseTx;
    }

    CdpStakeTx(const CUserID &txUidIn, uint64_t feeIn, int validHeightIn,
                uint64_t bcoinsToStakeIn, uint64_t collateralRatioIn, uint64_t fcoinsInterestIn):
                CBaseTx(CDP_STAKE_TX, txUidIn, validHeightIn, feeIn) {
        if (txUidIn.type() == typeid(CRegID)) {
            assert(!txUidIn.get<CRegID>().IsEmpty());
        }

        bcoinsToStake = bcoinsToStakeIn;
        collateralRatio = collateralRatioIn;
        fcoinsInterest = fcoinsInterestIn;
    }

    ~CdpStakeTx() {}

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
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid << appUid
               << VARINT(llFees) << VARINT(bcoinsToStake) << VARINT(collateralRatio) << VARINT(fcoinsInterest);
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual uint64_t GetValue() const { return bcoinsToStake; }
    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual uint64_t GetFee() const { return llFees; }
    virtual double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CdpTx>(this); }

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
class CdpRedeemTx: public CBaseTx {
public:
    uint64_t scoinsToRedeem;    // stableCoins amount to redeem or burn
    uint64_t fcoinsInterest;    // Interest will be deducted from scoinsToRedeem when 0
                                // For the first-time staking, no interest shall be paid though

public:
    CdpRedeemTx() : CBaseTx(CDP_REDEEMP_TX) {}

    CdpRedeemTx(const CBaseTx *pBaseTx): CBaseTx(CDP_REDEEMP_TX) {
        *this = *(CdpRedeemTx *)pBaseTx;
    }

    CdpRedeemTx(const CUserID &txUidIn, uint64_t feeIn, int validHeightIn,
                uint64_t scoinsToRedeemIn, uint64_t fcoinsInterestIn):
                CBaseTx(CDP_REDEEMP_TX, txUidIn, validHeightIn, feeIn) {
        if (txUidIn.type() == typeid(CRegID)) {
            assert(!txUidIn.get<CRegID>().IsEmpty());
        }

        scoinsToRedeem = scoinsToRedeemIn;
        fcoinsInterest = fcoinsInterestIn;
    }

    ~CdpRedeemTx() {}

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
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid << appUid
               << VARINT(llFees) << VARINT(scoinsToRedeem) << VARINT(fcoinsInterest);
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual uint64_t GetValue() const { return bcoinsToStake; }
    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual uint64_t GetFee() const { return llFees; }
    virtual double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CdpTx>(this); }

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
class CdpLiquidateTx: public CBaseTx {
public:
    uint64_t scoinsToRedeem;    // stable coins to redeem base coins
    uint64_t fcoinsInterest;    // Interest will be deducted from scoinsToRedeem when 0
                                // For the first-time staking, no interest shall be paid though
public:
    CdpLiquidateTx() : CBaseTx(CDP_LIQUIDATE_TX) {}

    CdpLiquidateTx(const CBaseTx *pBaseTx): CBaseTx(CDP_LIQUIDATE_TX) {
        *this = *(CdpLiquidateTx *)pBaseTx;
    }

    CdpLiquidateTx(const CUserID &txUidIn, uint64_t feeIn, int validHeightIn,
                uint64_t scoinsToRedeemIn, uint64_t fcoinsInterestIn):
                CBaseTx(CDP_LIQUIDATE_TX, txUidIn, validHeightIn, feeIn) {
        if (txUidIn.type() == typeid(CRegID)) {
            assert(!txUidIn.get<CRegID>().IsEmpty());
        }

        scoinsToRedeem = scoinsToRedeemIn;
        fcoinsInterest = fcoinsInterestIn;
    }

    ~CdpLiquidateTx() {}

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
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CdpTx>(this); }

    virtual string ToString(CAccountCache &view);
    virtual Object ToJson(const CAccountCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
};

#endif //TX_CDP_H