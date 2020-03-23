// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assetdb.h"

#include "commons/uint256.h"
#include "commons/util/util.h"

#include <stdint.h>

using namespace std;

bool CAssetDbCache::GetAsset(const TokenSymbol &tokenSymbol, CAsset &asset) {
    return assetCache.GetData(tokenSymbol, asset);
}

bool CAssetDbCache::SetAsset(const CAsset &asset) {
    return assetCache.SetData(asset.asset_symbol, asset);
}

bool CAssetDbCache::HasAsset(const TokenSymbol &tokenSymbol) {
    return assetCache.HasData(tokenSymbol);
}

bool CAssetDbCache::CheckAsset(const TokenSymbol &symbol, uint64_t permsSum) {
    if (symbol.size() < 3 || symbol.size() > MAX_TOKEN_SYMBOL_LEN) {
        LogPrint(BCLog::INFO, "[WARN] Invalid format of symbol=%s\n", symbol);
        return false;
    }

    if (kCoinTypeSet.count(symbol)) // the hard code symbol has all perms
        return true;

    CAsset asset;
    if (!GetAsset(symbol, asset)) {
        LogPrint(BCLog::INFO, "[WARN] Asset of symbol=%s does not exist\n", symbol);
        return false;
    }

    return asset.HasPerms(permsSum);
}

bool CAssetDbCache::SetAssetPerms(const CAsset &oldAsset, const CAsset &newAsset) {
    if (oldAsset.perms_sum != newAsset.perms_sum) {
        for (const auto &item : kAssetPermTitleMap) {
            uint64_t perm = item.first;
            bool oldPermValue = oldAsset.HasPerms(perm);
            bool newPermValue = newAsset.HasPerms(perm);
            if (oldPermValue != newPermValue) {
                AssetPermStatus status = newPermValue ? AssetPermStatus::ENABLED : AssetPermStatus::DISABLED;
                if (!perm_assets_cache.SetData(make_pair(CFixedUInt64(perm), newAsset.asset_symbol), (uint8_t)status))
                    return false;
            }
        }
    }
    return true;
}

void CAssetDbCache::GetDexQuoteSymbolSet(set<TokenSymbol> &symbolSet) {
    symbolSet.insert(kDexQuoteSymbolSet.begin(), kDexQuoteSymbolSet.end());
    CPermAssetsIterator it(perm_assets_cache, CFixedUInt64(AssetPermType::PERM_DEX_BASE));
    for (it.First(); it.IsValid(); it.Next()) {
        if (it.GetValue() == (uint8_t)AssetPermStatus::ENABLED)
            symbolSet.insert(it.GetKey().second);
    }
}


