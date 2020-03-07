// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assetdb.h"

#include "commons/uint256.h"
#include "commons/util/util.h"

#include <stdint.h>

using namespace std;

bool CAssetDBCache::GetAsset(const TokenSymbol &tokenSymbol, CAsset &asset) {
    return assetCache.GetData(tokenSymbol, asset);
}

bool CAssetDBCache::SetAsset(const CAsset &asset) {
    return assetCache.SetData(asset.asset_symbol, asset);
}

bool CAssetDBCache::HasAsset(const TokenSymbol &tokenSymbol) {
    return assetCache.HasData(tokenSymbol);
}

bool CAssetDBCache::CheckAssetSymbol(const TokenSymbol &symbol, uint64_t permsSum) {
    if (symbol.size() == 0 || symbol.size() > MAX_TOKEN_SYMBOL_LEN)
        return false;

    if (kCoinTypeSet.count(symbol))
        return true;

    CAsset asset;
    if (GetAsset(symbol, asset))
        return true;

     if (permsSum > asset.asset_perms_sum)
        return false;

    return (permsSum == (asset.asset_perms_sum & permsSum));
}

bool CAssetDBCache::Flush() {
    assetCache.Flush();
    return true;
}