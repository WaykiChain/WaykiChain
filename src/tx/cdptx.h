// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_CDP_H
#define TX_CDP_H

#include "entities/receipt.h"
#include "tx.h"

class CUserCDP;

// CDPStakeAssetMap: symbol -> amount
// support to stake multi token
typedef std::map<TokenSymbol, CVarIntValue<uint64_t> > CDPStakeAssetMap;

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
        : CBaseTx(CDP_STAKE_TX, txUidIn, validHeightIn, cmFeeIn.symbol, cmFeeIn.GetAmountInSawi()),
          cdp_txid(cdpTxId),
          assets_to_stake({ {cmBcoinsToStake.symbol, cmBcoinsToStake.GetAmountInSawi()} }),
          scoin_symbol(cmScoinsToMint.symbol),
          scoins_to_mint(cmScoinsToMint.GetAmountInSawi()) {}

    ~CCDPStakeTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(cdp_txid);
        READWRITE(assets_to_stake);
        READWRITE(scoin_symbol);
        READWRITE(VARINT(scoins_to_mint));

        READWRITE(signature);
    )

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid
                   << fee_symbol << VARINT(llFees) << cdp_txid << assets_to_stake << scoin_symbol
                   << VARINT(scoins_to_mint);
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CCDPStakeTx>(*this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);

private:
    bool SellInterestForFcoins(const CTxCord &txCord, const CUserCDP &cdp, const uint64_t scoinsInterestToRepay,
        CCacheWrapper &cw, CValidationState &state, vector<CReceipt> &receipts);

private:
    TxID cdp_txid;                      // optional: only required for staking existing CDPs
    CDPStakeAssetMap assets_to_stake;   // asset map to stake, support to stake multi token
    TokenSymbol scoin_symbol;           // ditto
    uint64_t scoins_to_mint;            // initial collateral ratio must be >= 190 (%), boosted by 10^4
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
        : CBaseTx(CDP_REDEEM_TX, txUidIn, validHeightIn, cmFeeIn.symbol, cmFeeIn.GetAmountInSawi()),
          cdp_txid(cdpTxId),
          scoins_to_repay(scoinsToRepay),
          assets_to_redeem( { {SYMB::WICC, bcoinsToRedeem} }) {}

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
        READWRITE(assets_to_redeem);

        READWRITE(signature);
    )

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid
                   << fee_symbol << VARINT(llFees) << cdp_txid << VARINT(scoins_to_repay)
                   << assets_to_redeem;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CCDPRedeemTx>(*this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
private:
    bool SellInterestForFcoins(const CTxCord &txCord, const CUserCDP &cdp, const uint64_t scoinsInterestToRepay,
        CCacheWrapper &cw, CValidationState &state, vector<CReceipt> &receipts);

private:
    uint256 cdp_txid;           // CDP cdpTxId
    uint64_t scoins_to_repay;   // stablecoin amount to redeem or burn, including interest
    CDPStakeAssetMap assets_to_redeem;   // asset map to redeem, support to redeem multi token
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
                  cmFeeIn.GetAmountInSawi()),
          cdp_txid(cdpTxId),
          liquidate_asset_symbol(),
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
        READWRITE(liquidate_asset_symbol);
        READWRITE(VARINT(scoins_to_liquidate));

        READWRITE(signature);
    )

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << fee_symbol
           << VARINT(llFees) << cdp_txid << liquidate_asset_symbol << VARINT(scoins_to_liquidate);
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CCDPLiquidateTx>(*this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);

private:
    bool ProcessPenaltyFees(CTxExecuteContext &context, const CUserCDP &cdp, uint64_t scoinPenaltyFees,
        vector<CReceipt> &receipts);

private:
    uint256     cdp_txid;            // target CDP to liquidate
    TokenSymbol liquidate_asset_symbol;   // can be empty. Even when specified, it can also liquidate more than one asset.
    uint64_t    scoins_to_liquidate;  // partial liquidation is allowed, must include penalty fees in
};

/**
 * settle interest of CDPs
 */
class CCDPSettleInterestTx: public CBaseTx {
public:
    vector<uint256> cdp_list; // cdp list
public:
    CCDPSettleInterestTx() : CBaseTx(CDP_SETTLE_INTEREST_TX) {}

    CCDPSettleInterestTx(const CUserID &txUidIn, const ComboMoney &cmFeeIn, int32_t validHeightIn)
        : CBaseTx(CDP_SETTLE_INTEREST_TX, txUidIn, validHeightIn, cmFeeIn.symbol,
                  cmFeeIn.GetAmountInSawi()) {}

    ~CCDPSettleInterestTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid); // reserve, should be empty

        READWRITE(cdp_list);

        READWRITE(signature); // reserve, should be empty
    )

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << cdp_list;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CCDPSettleInterestTx>(*this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};

bool GetSettledInterestCdps(CCacheWrapper &cw, HeightType height, const CCdpCoinPair &cdpCoinPair,
                            vector<uint256> &cdpList, uint32_t maxCount);

#endif //TX_CDP_H