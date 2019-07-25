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
                        const uint64_t &total_owed_scoins, uint64_t &interestOut);

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
    CCDPStakeTx(const CUserID &txUidIn, uint64_t feesIn, int32_t validHeightIn,
                uint64_t bcoinsToStake, uint64_t scoinsToMint):
                CBaseTx(CDP_STAKE_TX, txUidIn, validHeightIn, feesIn) {
        bcoins_to_stake   = bcoinsToStake;
        scoins_to_mint    = scoinsToMint;
    }
    /** Stake an existing CDP */
    CCDPStakeTx(const CUserID &txUidIn, uint64_t feesIn, int32_t validHeightIn, uint256 cdpTxId,
                uint64_t bcoinsToStake, uint64_t scoinsToMint)
        : CBaseTx(CDP_STAKE_TX, txUidIn, validHeightIn, feesIn) {
        if (txUidIn.type() == typeid(CRegID)) {
            assert(!txUidIn.get<CRegID>().IsEmpty());
        }

        cdp_txid        = cdpTxId;
        bcoins_to_stake = bcoinsToStake;
        scoins_to_mint  = scoinsToMint;
    }

    ~CCDPStakeTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(VARINT(llFees));
        READWRITE(cdp_txid);
        READWRITE(VARINT(bcoins_to_stake));
        READWRITE(VARINT(scoins_to_mint));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << VARINT(llFees)
                << cdp_txid << VARINT(bcoins_to_stake) << VARINT(scoins_to_mint);
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual map<TokenSymbol, uint64_t> GetValues() const { return map<TokenSymbol, uint64_t>{{SYMB::WICC, bcoins_to_stake}}; }
    virtual TxID GetHash() const { return ComputeSignatureHash(); }
    // virtual uint64_t GetFees() const { return llFees; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CCDPStakeTx>(this); }

    virtual string ToString(CAccountDBCache &view);
    virtual Object ToJson(const CAccountDBCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
private:
    bool SellInterestForFcoins(const uint64_t scoinsInterestToRepay, CCacheWrapper &cw, CValidationState &state);

private:
    TxID        cdp_txid;            //optional: only required for staking existing CDPs
    TokenSymbol bcoin_symbol;        //optional: only required for 1st-time CDP staking
    TokenSymbol scoin_symbol;        //ditto
    uint64_t    bcoins_to_stake;      // base coins amount to stake or collateralize
    uint64_t    scoins_to_mint;       // initial collateral ratio must be >= 190 (%), boosted by 10000
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

    CCDPRedeemTx(const CUserID &txUidIn, uint64_t feesIn, int32_t validHeightIn,
                uint256 cdpTxIdIn, uint64_t scoinsToRepay, uint64_t bcoinsToRedeem):
                CBaseTx(CDP_REDEEM_TX, txUidIn, validHeightIn, feesIn) {
        if (txUidIn.type() == typeid(CRegID)) {
            assert(!txUidIn.get<CRegID>().IsEmpty());
        }
        cdp_txid         = cdpTxId;
        scoins_to_repay  = scoinsToRepay;
        bcoins_to_redeem = bcoinsToRedeem;
    }

    ~CCDPRedeemTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(VARINT(llFees));

        READWRITE(cdp_txid);
        READWRITE(VARINT(scoins_to_repay));
        READWRITE(VARINT(bcoins_to_redeem));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << VARINT(llFees)
                << cdp_txid << VARINT(scoins_to_repay) << VARINT(bcoins_to_redeem);
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual map<TokenSymbol, uint64_t> GetValues() const { return map<TokenSymbol, uint64_t>{{SYMB::WUSD, bcoins_to_redeem}}; }
    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    // virtual uint64_t GetFees() const { return llFees; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CCDPRedeemTx>(this); }

    virtual string ToString(CAccountDBCache &view);
    virtual Object ToJson(const CAccountDBCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
private:
    bool SellInterestForFcoins(const int32_t height, const CUserCDP &cdp, CCacheWrapper &cw, CValidationState &state);

private:
    uint256 cdp_txid;          // CDP cdpTxId
    uint64_t scoins_to_repay;   // stableCoins amount to redeem or burn, including interest
    uint64_t bcoins_to_redeem;
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

    CCDPLiquidateTx(const CUserID &txUidIn, uint64_t feesIn, int32_t validHeightIn,
                    uint256 cdpTxId, uint64_t scoinsToLiquidate):
                CBaseTx(CDP_LIQUIDATE_TX, txUidIn, validHeightIn, feesIn) {

        if (txUidIn.type() == typeid(CRegID)) {
            assert(!txUidIn.get<CRegID>().IsEmpty());
        }

        cdp_txid            = cdpTxId;
        scoins_to_liquidate = scoinsToLiquidate;
    }

    ~CCDPLiquidateTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(VARINT(llFees));

        READWRITE(cdp_txid);
        READWRITE(scoins_to_liquidate);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << VARINT(llFees)
                << cdp_txid << VARINT(scoins_to_liquidate);
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual map<TokenSymbol, uint64_t> GetValues() const {
        return map<TokenSymbol, uint64_t>{{SYMB::WUSD, scoins_to_liquidate}};
    }
    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    // virtual uint64_t GetFees() const { return llFees; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CCDPLiquidateTx>(this); }

    virtual string ToString(CAccountDBCache &view);
    virtual Object ToJson(const CAccountDBCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);

private:
    bool SellPenaltyForFcoins(uint64_t scoinPenaltyFees, const int32_t height,
                            const CUserCDP &cdp, CCacheWrapper &cw, CValidationState &state);

private:
    uint256     cdp_txid;            // target CDP to liquidate
    uint64_t    scoins_to_liquidate;  // partial liquidation is allowed, must include penalty fees in
};

#endif //TX_CDP_H