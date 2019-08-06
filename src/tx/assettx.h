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
    TokenSymbol fee_symbol;

    CAsset      asset;          // asset
public:
    CAssetIssueTx() : CBaseTx(ASSET_ISSUE_TX) {};

    CAssetIssueTx(TxType nTxTypeIn): CBaseTx(nTxTypeIn) {};

    CAssetIssueTx(const CUserID &txUidIn, int validHeightIn, const TokenSymbol &feeSymbol,
                  uint64_t fees, const CAsset &assetIn)
        : CBaseTx(ASSET_ISSUE_TX, txUidIn, validHeightIn, fees),
          fee_symbol(feeSymbol),
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

    virtual map<TokenSymbol, uint64_t> GetValues() const {
        return map<TokenSymbol, uint64_t>{{asset.symbol, asset.total_supply}};
    }
    virtual TxID GetHash() const { return ComputeSignatureHash(); }
    // virtual uint64_t GetFees() const { return llFees; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CAssetIssueTx>(*this); }

    virtual string ToString(CAccountDBCache &view);
    virtual Object ToJson(const CAccountDBCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
};


/**
 * Update an existing asset from Chain
 */
class CAssetUpdateTx: public CAssetIssueTx {
public:
    CAssetUpdateTx() : CAssetIssueTx(ASSET_UPDATE_TX) {};

    ~CAssetUpdateTx() {}

};

#endif //TX_ASSET_H