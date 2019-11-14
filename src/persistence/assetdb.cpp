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

bool CAssetDBCache::HaveAsset(const TokenSymbol &tokenSymbol) {
    return assetCache.HaveData(tokenSymbol);
}

bool CAssetDBCache::SaveAsset(const CAsset &asset) {
    return assetCache.SetData(asset.symbol, asset);
}
bool CAssetDBCache::ExistAssetSymbol(const TokenSymbol &tokenSymbol) {
    return assetCache.HaveData(tokenSymbol);
}

shared_ptr<string> CAssetDBCache::CheckTransferCoinSymbol(const TokenSymbol &symbol) {
    size_t coinSymbolSize = symbol.size();
    if (coinSymbolSize == 0 || coinSymbolSize > MAX_TOKEN_SYMBOL_LEN) {
        return make_shared<string>("empty or too long");
    }

    if ((coinSymbolSize < MIN_ASSET_SYMBOL_LEN && !kCoinTypeSet.count(symbol)) ||
        (coinSymbolSize >= MIN_ASSET_SYMBOL_LEN && !HaveAsset(symbol)))
        return make_shared<string>("unsupported symbol");

    return nullptr;
}

bool CAssetDBCache::AddAssetTradingPair(const CAssetTradingPair &assetTradingPair) {
    return assetTradingPairCache.SetData(assetTradingPair, 1);
}
bool CAssetDBCache::EraseAssetTradingPair(const CAssetTradingPair &assetTradingPair) {
    return assetTradingPairCache.EraseData(assetTradingPair);
}
bool CAssetDBCache::ExistAssetTradingPair(const CAssetTradingPair &assetTradingPair) {
    return assetTradingPairCache.HaveData(assetTradingPair);
}

bool CAssetDBCache::Flush() {
    assetCache.Flush();
    assetTradingPairCache.Flush();
    return true;
}