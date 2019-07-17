// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assetdb.h"

#include "commons/uint256.h"
#include "commons/util.h"

#include <stdint.h>

using namespace std;

bool CAssetDBCache::GetAsset(const TokenSymbol &tokenSymbol, CAsset &asset) {
    return assetCache.GetData(tokenSymbol, asset);
}

bool CAssetDBCache::SaveAsset(const CAsset &asset) {
    return assetCache.SetData(asset.symbol, asset);
}

bool CAssetDBCache::ExistAssetSymbol(const TokenSymbol &tokenSymbol) {
    return assetCache.HaveData(tokenSymbol);
}