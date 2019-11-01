// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_RECEIPT_H
#define ENTITIES_RECEIPT_H

#include "config/txbase.h"
#include "crypto/hash.h"
#include "entities/asset.h"
#include "entities/id.h"
#include "commons/json/json_spirit_utils.h"

static CUserID nullId;

//         ReceiptCode                   CodeValue         memo
//       -----------------              ----------  ----------------------------
#define RECEIPT_CODE_LIST(DEFINE) \
    /**** reward */ \
    DEFINE(BLOCK_REWORD_TO_MINER,               101, "block reward to miner") \
    DEFINE(COIN_BLOCK_REWORD_TO_MINER,          102, "coin block reward to miner") \
    DEFINE(COIN_BLOCK_INFLATE,                  103, "inflate coins to user of tx") \
    /**** transfer */ \
    DEFINE(TRANSFER_FEE_TO_RISERVE,             201, "transferred fee to risk riserve") \
    DEFINE(TRANSFER_ACTUAL_COINS,               202, "actual transferred coins") \
    /**** delegate */ \
    DEFINE(DELEGATE_ADD_VOTE,                   301, "delegate add votes") \
    DEFINE(DELEGATE_SUB_VOTE,                   302, "delegate sub votes") \
    DEFINE(DELEGATE_VOTE_INTEREST,              303, "delegate vote interest") \
    /**** CDP stake*/ \
    DEFINE(CDP_STAKED_ASSET_FROM_OWNER,         401, "staked assets from cdp owner") \
    DEFINE(CDP_MINTED_SCOIN_TO_OWNER,           402, "minted scoins to cdp owner") \
    DEFINE(CDP_INTEREST_BUY_DEFLATE_FCOINS,     403, "cdp interest scoins to buy fcoins for deflating") \
    /**** CDP redeem*/ \
    DEFINE(CDP_REPAID_SCOIN_FROM_OWNER,         420, "actual repaid scoins from cdp owner") \
    DEFINE(CDP_REDEEMED_ASSET_TO_OWNER,         421, "redeemed assets to cdp owner") \
    /**** CDP user liquidate*/ \
    DEFINE(CDP_SCOIN_FROM_LIQUIDATOR,           440, "cdp scoins from liquidator") \
    DEFINE(CDP_ASSET_TO_LIQUIDATOR,             441, "cdp assets to liquidator") \
    DEFINE(CDP_LIQUIDATED_ASSET_TO_OWNER,       442, "cdp liquidated assets to owner") \
    DEFINE(CDP_LIQUIDATED_CLOSEOUT_SCOIN,       443, "cdp liquidated closeout scoins") \
    DEFINE(CDP_PENALTY_TO_RISERVE,              444, "cdp half penalty scoins to risk riserve directly") \
    DEFINE(CDP_PENALTY_BUY_DEFLATE_FCOINS,      445, "cdp half penalty scoins to buy fcoins for deflating") \
    /**** CDP forced liquidate*/ \
    DEFINE(CDP_TOTAL_CLOSEOUT_SCOIN_FROM_RESERVE, 460, "total closeout scoins from risk reserve in forced-liquidation") \
    DEFINE(CDP_TOTAL_INFLATE_FCOIN_TO_RESERVE,    461, "total inflate fcoins to risk reserve in forced-liquidation") \
    DEFINE(CDP_TOTAL_ASSET_TO_RESERVE,            462, "total assets to reserve in forced-liquidation") \
    /**** DEX */ \
    DEFINE(DEX_ASSET_FEE_TO_SETTLER,            501, "dex deal asset fee from buyer to settler") \
    DEFINE(DEX_COIN_FEE_TO_SETTLER,             502, "dex deal coin fee from seller to settler") \
    DEFINE(DEX_ASSET_TO_BUYER,                  503, "dex deal deal assets to buyer") \
    DEFINE(DEX_COIN_TO_SELLER,                  504, "dex deal deal coins to seller") \
    DEFINE(DEX_UNFREEZE_COIN_TO_BUYER,          505, "dex unfreeze coins to buyer for canceling order") \
    DEFINE(DEX_UNFREEZE_ASSET_TO_SELLER,        506, "dex unfreeze asset to seller for canceling order") \
    /**** contract */ \
    DEFINE(CONTRACT_FUEL_TO_RISK_RISERVE,       601, "contract fuel to risk riserve") \
    DEFINE(CONTRACT_TOKEN_OPERATE_ADD,          602, "operate add token of contract user account") \
    DEFINE(CONTRACT_TOKEN_OPERATE_SUB,          603, "operate sub token of contract user account") \
    DEFINE(CONTRACT_TOKEN_OPERATE_TAG_ADD,      604, "operate add token tag of contract user account") \
    DEFINE(CONTRACT_TOKEN_OPERATE_TAG_SUB,      605, "operate sub token tag of contract user account") \
    DEFINE(CONTRACT_ACCOUNT_OPERATE_ADD,        606, "operate add bcoins of account by contract") \
    DEFINE(CONTRACT_ACCOUNT_OPERATE_SUB,        607, "operate sub bcoins of account by contract") \
    DEFINE(CONTRACT_ACCOUNT_TRANSFER_ASSET,     608, "transfer account asset by contract") \
    /**** asset */ \
    DEFINE(ASSET_ISSUED_FEE_TO_RISERVE,         701, "asset issued fee to risk riserve") \
    DEFINE(ASSET_UPDATED_FEE_TO_RISERVE,        702, "asset updated fee to risk riserve") \
    DEFINE(ASSET_ISSUED_FEE_TO_MINER,           703, "asset issued fee to miner") \
    DEFINE(ASSET_UPDATED_FEE_TO_MINER,          704, "asset updated fee to miner")

#define DEFINE_RECEIPT_CODE_TYPE(enumType, code, enumName) enumType = code,
enum ReceiptCode: uint16_t {
    RECEIPT_CODE_LIST(DEFINE_RECEIPT_CODE_TYPE)
};

#define DEFINE_RECEIPT_CODE_NAMES(enumType, code, enumName) { ReceiptCode::enumType, enumName },
static const EnumTypeMap<ReceiptCode, string> RECEIPT_CODE_NAMES = {
    RECEIPT_CODE_LIST(DEFINE_RECEIPT_CODE_NAMES)
};

inline const string& GetReceiptCodeName(ReceiptCode code) {
    const auto it = RECEIPT_CODE_NAMES.find(code);
    if (it != RECEIPT_CODE_NAMES.end())
        return it->second;
    return EMPTY_STRING;
}

class CReceipt {
public:
    CUserID     from_uid;
    CUserID     to_uid;
    TokenSymbol coin_symbol;
    uint64_t    coin_amount;
    ReceiptCode code;

public:
    CReceipt() {}

    CReceipt(const CUserID &fromUid, const CUserID &toUid, const TokenSymbol &coinSymbol, const uint64_t coinAmount,
             ReceiptCode codeIn)
        : from_uid(fromUid), to_uid(toUid), coin_symbol(coinSymbol), coin_amount(coinAmount), code(codeIn) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(from_uid);
        READWRITE(to_uid);
        READWRITE(coin_symbol);
        READWRITE(VARINT(coin_amount));
        READWRITE_CONVERT(uint16_t, code);
    )
};

#endif  // ENTITIES_RECEIPT_H