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

//         ReceiptType                   CodeValue         memo
//       -----------------              ----------  ----------------------------
#define RECEIPT_TYPE_LIST(DEFINE) \
    DEFINE(NULL_RECEIPT_TYPE,                     0,  "null type") \
    /**** reward */ \
    DEFINE(BLOCK_REWARD_TO_MINER,               101, "block reward to miner") \
    DEFINE(COIN_MINT_ONCHAIN,                   102, "coin minted onchain") \
    DEFINE(COIN_BLOCK_INFLATE,                  103, "inflate bcoins to block miner") \
    DEFINE(COIN_STAKE,                          104, "stake user's coins") \
    /**** transfer */ \
    DEFINE(TRANSFER_FEE_TO_RESERVE,             201, "transfer fees to risk reserve") \
    DEFINE(TRANSFER_ACTUAL_COINS,               202, "transfer coins with account") \
    DEFINE(LUAVM_TRANSFER_ACTUAL_COINS,         203, "transfer coins in Lua contract") \
    DEFINE(WASM_TRANSFER_ACTUAL_COINS,          204, "transfer coins in WASM contract") \
    DEFINE(WASM_MINT_COINS,                     205, "mint coins in WASM contract") \
    DEFINE(WASM_BURN_COINS,                     206, "burn coins in WASM contract") \
    DEFINE(TRANSFER_UTXO_COINS,                 207, "transfer coins in a UTXO trx") \
    DEFINE(TRANSFER_PROPOSAL,                   208, "transfer coins in a proposal") \
    /**** delegate */ \
    DEFINE(DELEGATE_ADD_VOTE,                   301, "add votes to delegate(s)") \
    DEFINE(DELEGATE_SUB_VOTE,                   302, "sub votes to delegate(s)") \
    DEFINE(DELEGATE_VOTE_INTEREST,              303, "receive interest thru delegate voting") \
    /**** CDP stake*/ \
    DEFINE(CDP_STAKED_ASSET_FROM_OWNER,         401, "staked assets from cdp owner") \
    DEFINE(CDP_MINTED_SCOIN_TO_OWNER,           402, "minted scoins to cdp owner") \
    DEFINE(CDP_INTEREST_BUY_DEFLATE_FCOINS,     403, "cdp interest scoins to buy fcoins for deflating") \
    DEFINE(CDP_REPAY_INTEREST,                  404, "repay cdp interest scoins to owner") \
    DEFINE(CDP_REPAY_INTEREST_TO_FUND,          405, "repay cdp interest scoins to fund") \
    /**** CDP redeem*/ \
    DEFINE(CDP_REPAID_SCOIN_FROM_OWNER,         420, "actual repaid scoins from cdp owner") \
    DEFINE(CDP_REDEEMED_ASSET_TO_OWNER,         421, "redeemed assets to cdp owner") \
    /**** CDP user liquidate*/ \
    DEFINE(CDP_SCOIN_FROM_LIQUIDATOR,           440, "cdp scoins from liquidator") \
    DEFINE(CDP_ASSET_TO_LIQUIDATOR,             441, "cdp assets to liquidator") \
    DEFINE(CDP_LIQUIDATED_ASSET_TO_OWNER,       442, "cdp liquidated assets to owner") \
    DEFINE(CDP_LIQUIDATED_CLOSEOUT_SCOIN,       443, "cdp liquidated closeout scoins") \
    DEFINE(CDP_PENALTY_TO_RESERVE,              444, "cdp half penalty scoins to risk reserve directly") \
    DEFINE(CDP_PENALTY_BUY_DEFLATE_FCOINS,      445, "cdp half penalty scoins to buy fcoins for deflating") \
    /**** CDP forced liquidate*/ \
    DEFINE(CDP_TOTAL_CLOSEOUT_SCOIN_FROM_RESERVE, 460, "total closeout scoins from risk reserve in forced-liquidation") \
    DEFINE(CDP_TOTAL_INFLATE_FCOIN_TO_RESERVE,    461, "total inflate fcoins to risk reserve in forced-liquidation") \
    DEFINE(CDP_TOTAL_ASSET_TO_RESERVE,            462, "total assets to reserve in forced-liquidation") \
    /**** DEX */ \
    DEFINE(DEX_ASSET_FEE_TO_SETTLER,            501, "dex deal asset fee from buyer to settler") \
    DEFINE(DEX_COIN_FEE_TO_SETTLER,             502, "dex deal coin fee from seller to settler") \
    DEFINE(DEX_ASSET_TO_BUYER,                  503, "dex dealed: transfer assets to buyer") \
    DEFINE(DEX_COIN_TO_SELLER,                  504, "dex dealed: transfer coins to seller") \
    DEFINE(DEX_UNFREEZE_COIN_TO_BUYER,          505, "dex unfreeze buyer's coins") \
    DEFINE(DEX_UNFREEZE_ASSET_TO_SELLER,        506, "dex unfreeze seller's assets") \
    DEFINE(DEX_OPERATOR_REG_FEE_TO_RESERVE,     520, "dex operator registered fee to risk reserve") \
    DEFINE(DEX_OPERATOR_UPDATED_FEE_TO_RESERVE, 521, "dex operator updated fee to risk reserve") \
    DEFINE(DEX_OPERATOR_REG_FEE_TO_MINER,       522, "dex operator registered fee to miner") \
    DEFINE(DEX_OPERATOR_UPDATED_FEE_TO_MINER,   523, "dex operator updated fee to miner") \
    /**** contract */ \
    DEFINE(CONTRACT_FUEL_TO_RISK_RESERVE,       601, "contract fuel to risk reserve") \
    DEFINE(CONTRACT_TOKEN_OPERATE_ADD,          602, "add operate on contract-managed tokens") \
    DEFINE(CONTRACT_TOKEN_OPERATE_SUB,          603, "sub operate on contract-managed tokens") \
    DEFINE(CONTRACT_TOKEN_OPERATE_TAG_ADD,      604, "add operate on contract-managed tag tokens") \
    DEFINE(CONTRACT_TOKEN_OPERATE_TAG_SUB,      605, "sub operate on contract-managed tag tokens") \
    DEFINE(CONTRACT_ACCOUNT_OPERATE_ADD,        606, "add operate on account in contract") \
    DEFINE(CONTRACT_ACCOUNT_OPERATE_SUB,        607, "sub operate on account in contract") \
    DEFINE(CONTRACT_ACCOUNT_TRANSFER_ASSET,     608, "transfer account asset in contract") \
    /**** asset */ \
    DEFINE(ASSET_ISSUED_FEE_TO_RESERVE,         701, "asset issued fee to risk reserve") \
    DEFINE(ASSET_UPDATED_FEE_TO_RESERVE,        702, "asset updated fee to risk reserve") \
    DEFINE(ASSET_ISSUED_FEE_TO_MINER,           703, "asset issued fee to miner") \
    DEFINE(ASSET_UPDATED_FEE_TO_MINER,          704, "asset updated fee to miner") \
    DEFINE(ASSET_MINT_NEW_AMOUNT,               705, "asset minted with new amount") \
    /*** cross-chain */ \
    DEFINE(AXC_MINT_COINS,                      801, "cross-chain asset minted onchain") \
    DEFINE(AXC_BURN_COINS,                      802, "cross-chain asset burned onchain") \
    DEFINE(AXC_REWARD_FEE_TO_GOVERNOR,          803, "cross-chain reward fees to GOVERNOR") \
    DEFINE(AXC_REWARD_FEE_TO_GW,                804, "cross-chain reward fees to GW") \

#define DEFINE_RECEIPT_CODE_TYPE(enumType, code, enumName) enumType = code,
enum ReceiptType: uint16_t {
    RECEIPT_TYPE_LIST(DEFINE_RECEIPT_CODE_TYPE)
};

#define DEFINE_RECEIPT_CODE_NAMES(enumType, code, enumName) { ReceiptType::enumType, enumName },
static const EnumTypeMap<ReceiptType, string> RECEIPT_CODE_NAMES = {
    RECEIPT_TYPE_LIST(DEFINE_RECEIPT_CODE_NAMES)
};

inline const string& GetReceiptTypeName(ReceiptType code) {
    const auto it = RECEIPT_CODE_NAMES.find(code);
    if (it != RECEIPT_CODE_NAMES.end())
        return it->second;
    return EMPTY_STRING;
}

class CReceipt {
public:
    ReceiptType     receipt_type;
    BalanceOpType   op_type;
    CUserID         from_uid;
    CUserID         to_uid;
    TokenSymbol     coin_symbol;
    uint64_t        coin_amount;

public:
    CReceipt() {}

    CReceipt(const ReceiptType receiptType, const BalanceOpType opType) : receipt_type(receiptType), op_type(opType) {}
    CReceipt(const ReceiptType receiptType, const BalanceOpType opType,
            const CUserID &fromUid, const CUserID &toUid,
            const TokenSymbol &coinSymbol, const uint64_t coinAmount) :
            receipt_type(receiptType), op_type(opType), from_uid(fromUid), to_uid(toUid),
            coin_symbol(coinSymbol), coin_amount(coinAmount) {}

    IMPLEMENT_SERIALIZE(
        READWRITE_CONVERT(uint16_t, receipt_type);
        READWRITE_CONVERT(uint8_t, op_type);
        READWRITE(from_uid);
        READWRITE(to_uid);
        READWRITE(coin_symbol);
        READWRITE(VARINT(coin_amount));

    )

    void SetInfo(const CUserID &fromUid, const CUserID &toUid, const TokenSymbol &coinSymbol, const uint64_t coinAmount) {
        from_uid    = fromUid;
        to_uid      = toUid;
        coin_symbol = coinSymbol;
        coin_amount = coinAmount;
    }

    string ToString() const {
        return strprintf("from_uid=%s", from_uid.ToString()) + ", " +
        strprintf("to_uid=%s", to_uid.ToString()) + ", " +
        strprintf("coin_symbol=%s", coin_symbol) + ", " +
        strprintf("coin_amount=%f", ValueFromAmount(coin_amount)) + ", " +
        strprintf("receipt_type=%s", GetReceiptTypeName(receipt_type));
    }
};

typedef vector<CReceipt> ReceiptList;

#endif  // ENTITIES_RECEIPT_H