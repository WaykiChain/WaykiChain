// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "asset.h"


using namespace json_spirit;
using namespace std;

    // Check it when supplied from external like Tx or RPC calls
bool CAsset::CheckSymbol(const AssetType assetType, const TokenSymbol &assetSymbol, string &errMsg) {
    if (assetType == AssetType::NULL_ASSET) {
        errMsg = "null asset type";
        return false;
    }

    uint32_t symbolSizeMin = 2;
    uint32_t symbolSizeMax = 7;
    if (assetType == AssetType::UIA) {
        symbolSizeMin = 6;
        symbolSizeMax = 7;
    }

    size_t symbolSize = assetSymbol.size();
    if (symbolSize < symbolSizeMin || symbolSize > symbolSizeMax) {
        errMsg = strprintf("symbol len=%d, beyond range[%d, %d]",
                            symbolSize, symbolSizeMin, symbolSizeMax);
        return false;
    }

    bool valid = false;
    for (auto ch : assetSymbol) {
        valid = (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z');

        if (assetType == AssetType::UIA)
            valid = valid || (ch == '#' || ch == '.' || ch == '@' || ch == '_');
        if (assetType == AssetType::DIA) {
            valid = valid || (ch >= 'a' && ch <= 'z');
        }

        if (!valid) {
            errMsg = strprintf("Invalid char in symbol: %d", ch);
            return false;
        }
    }

    return true;
}
