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
    CRegID      owner_regid;    // owner RegID, can be transferred though
    TokenSymbol asset_symbol;   // asset symbol, E.g WICC | WUSD            len <= 12 chars
    TokenName   asset_name;     // asset long name, E.g WaykiChain coin,    len <= 32 chars
    uint64_t    total_supply;   // boosted by 1e8 for the decimal part, max is 90 billion.
    bool        mintable;       // whether this token can be minted in the future.

public:
    CAssetIssueTx() : CBaseTx(ASSET_ISSUE_TX) {};

  ~CAssetIssueTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(owner_regid);
        READWRITE(asset_symbol);
        READWRITE(asset_name);
        READWRITE(VARINT(total_supply));
        READWRITE(mintable);

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << fee_symbol << VARINT(llFees)
               << owner_regid << asset_symbol << asset_name << VARINT(total_supply) << mintable;
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual map<TokenSymbol, uint64_t> GetValues() const { return map<TokenSymbol, uint64_t>{{asset_symbol, total_supply}}; }
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
    CAssetUpdateTx() : CBaseTx(ASSET_UPDATE_TX) {};

    ~CAssetUpdateTx() {}

};

#endif //TX_ASSET_H