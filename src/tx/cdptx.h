// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_CDP_H
#define TX_CDP_H

#include "entities/receipt.h"
#include "tx.h"

class CUserCDP;

/**
 * Stake or ReStake bcoins into a CDP
 */
class CCDPStakeTx: public CBaseTx {
public:
    CCDPStakeTx() : CBaseTx(CDP_STAKE_TX) {}

    /** Newly open a CDP */
    CCDPStakeTx(const CUserID &txUidIn, int32_t validHeightIn, const ComboMoney &cmFeeIn,
                const ComboMoney &cmBcoinsToStake, const ComboMoney &cmScoinsToMint)
        : CCDPStakeTx(txUidIn, validHeightIn, uint256(), cmFeeIn, cmBcoinsToStake, cmScoinsToMint) {
    }

    /** Stake an existing CDP */
    CCDPStakeTx(const CUserID &txUidIn, int32_t validHeightIn, uint256 cdpTxId,
                const ComboMoney &cmFeeIn, const ComboMoney &cmBcoinsToStake,
                const ComboMoney &cmScoinsToMint)
        : CBaseTx(CDP_STAKE_TX, txUidIn, validHeightIn, cmFeeIn.symbol, cmFeeIn.GetSawiAmount()),
          cdp_txid(cdpTxId),
          bcoin_symbol(cmBcoinsToStake.symbol),
          scoin_symbol(cmScoinsToMint.symbol),
          bcoins_to_stake(cmBcoinsToStake.GetSawiAmount()),
          scoins_to_mint(cmScoinsToMint.GetSawiAmount()) {}

    ~CCDPStakeTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(cdp_txid);
        READWRITE(bcoin_symbol);
        READWRITE(scoin_symbol);
        READWRITE(VARINT(bcoins_to_stake));
        READWRITE(VARINT(scoins_to_mint));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
               << cdp_txid << bcoin_symbol << scoin_symbol << VARINT(bcoins_to_stake) << VARINT(scoins_to_mint);
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual TxID GetHash() const { return ComputeSignatureHash(); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CCDPStakeTx>(*this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);

private:
    bool SellInterestForFcoins(const CTxCord &txCord, const CUserCDP &cdp, const uint64_t scoinsInterestToRepay,
        CCacheWrapper &cw, CValidationState &state);

private:
    TxID cdp_txid;             // optional: only required for staking existing CDPs
    TokenSymbol bcoin_symbol;  // optional: only required for 1st-time CDP staking
    TokenSymbol scoin_symbol;  // ditto
    uint64_t bcoins_to_stake;  // base coins amount to stake or collateralize
    uint64_t scoins_to_mint;   // initial collateral ratio must be >= 190 (%), boosted by 10^4
};

/**
 * Redeem scoins into a CDP fully or partially
 * Need to pay interest or stability fees
 */
class CCDPRedeemTx: public CBaseTx {
public:
    CCDPRedeemTx() : CBaseTx(CDP_REDEEM_TX) {}

    CCDPRedeemTx(const CUserID &txUidIn, const ComboMoney &cmFeeIn, int32_t validHeightIn,
                 uint256 cdpTxId, uint64_t scoinsToRepay, uint64_t bcoinsToRedeem)
        : CBaseTx(CDP_REDEEM_TX, txUidIn, validHeightIn, cmFeeIn.symbol, cmFeeIn.GetSawiAmount()),
          cdp_txid(cdpTxId),
          scoins_to_repay(scoinsToRepay),
          bcoins_to_redeem(bcoinsToRedeem) {}

    ~CCDPRedeemTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(cdp_txid);
        READWRITE(VARINT(scoins_to_repay));
        READWRITE(VARINT(bcoins_to_redeem));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
               << cdp_txid << VARINT(scoins_to_repay) << VARINT(bcoins_to_redeem);
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CCDPRedeemTx>(*this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
private:
    bool SellInterestForFcoins(const CTxCord &txCord, const CUserCDP &cdp, const uint64_t scoinsInterestToRepay,
        CCacheWrapper &cw, CValidationState &state);

private:
    uint256 cdp_txid;           // CDP cdpTxId
    uint64_t scoins_to_repay;   // stablecoin amount to redeem or burn, including interest
    uint64_t bcoins_to_redeem;
};

/**
 * Liquidate a CDP
 */
class CCDPLiquidateTx: public CBaseTx {
public:
    CCDPLiquidateTx() : CBaseTx(CDP_LIQUIDATE_TX) {}

    CCDPLiquidateTx(const CUserID &txUidIn, const ComboMoney &cmFeeIn, int32_t validHeightIn,
                    uint256 cdpTxId, uint64_t scoinsToLiquidate)
        : CBaseTx(CDP_LIQUIDATE_TX, txUidIn, validHeightIn, cmFeeIn.symbol,
                  cmFeeIn.GetSawiAmount()),
          cdp_txid(cdpTxId),
          scoins_to_liquidate(scoinsToLiquidate) {}

    ~CCDPLiquidateTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(cdp_txid);
        READWRITE(VARINT(scoins_to_liquidate));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
               << cdp_txid << VARINT(scoins_to_liquidate);
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CCDPLiquidateTx>(*this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);

private:
    bool ProcessPenaltyFees(const CTxCord &txCord, const CUserCDP &cdp, uint64_t scoinPenaltyFees,
        CCacheWrapper &cw, CValidationState &state, vector<CReceipt> &receipts);

private:
    uint256     cdp_txid;            // target CDP to liquidate
    uint64_t    scoins_to_liquidate;  // partial liquidation is allowed, must include penalty fees in
};

#endif //TX_CDP_H