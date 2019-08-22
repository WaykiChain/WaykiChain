// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_ASSET_H
#define TX_ASSET_H

#include "tx.h"

/**
 * Issue a new asset onto Chain
 */
class CAssetIssueTx: public CBaseTx {
public:
    CAsset      asset;          // asset
public:
    CAssetIssueTx() : CBaseTx(ASSET_ISSUE_TX) {};

    CAssetIssueTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbol,
                  uint64_t fees, const CAsset &assetIn)
        : CBaseTx(ASSET_ISSUE_TX, txUidIn, validHeightIn, feeSymbol, fees),
          asset(assetIn){}

    ~CAssetIssueTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(asset);
        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << fee_symbol << VARINT(llFees)
               << asset;
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual TxID GetHash() const { return ComputeSignatureHash(); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CAssetIssueTx>(*this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
};


/**
 * Update an existing asset from Chain
 */
class CAssetUpdateTx: public CBaseTx {
public:
    TokenSymbol asset_symbol;       // symbol of asset that needs to be updated
    CUserID owner_uid;           // new owner userid of the asset
    TokenName asset_name;           // new asset long name, E.g WaykiChain coin
    uint64_t mint_amount;           // mint amount, boosted by 1e8
public:
    CAssetUpdateTx() : CBaseTx(ASSET_UPDATE_TX) {};

    CAssetUpdateTx(const CUserID &txUidIn, int32_t validHeightIn, const TokenSymbol &feeSymbolIn,
                   uint64_t feesIn, TokenSymbol assetSymbolIn, CUserID ownerUseridIn,
                   TokenName assetNameIn, uint64_t mintAmountIn)
        : CBaseTx(ASSET_UPDATE_TX, txUidIn, validHeightIn, feeSymbolIn, feesIn),
          asset_symbol(assetSymbolIn),
          owner_uid(ownerUseridIn),
          asset_name(assetNameIn),
          mint_amount(mintAmountIn) {}

    ~CAssetUpdateTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(asset_symbol);
        READWRITE(owner_uid);
        READWRITE(asset_name);
        READWRITE(VARINT(mint_amount));
        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << fee_symbol << VARINT(llFees)
               << asset_symbol << owner_uid << asset_name << VARINT(mint_amount);
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual TxID GetHash() const { return ComputeSignatureHash(); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CAssetUpdateTx>(*this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);

};

#endif //TX_ASSET_H