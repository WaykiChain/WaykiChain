// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "config/txbase.h"
#include "config/const.h"
#include "entities/asset.h"
#include "entities/dexorder.h"
#include "entities/proposal.h"
#include "persistence/cachewrapper.h"
#include "main.h"
#include "tx/proposaltx.h"

#include <algorithm>
#include <set>

extern bool CheckIsGovernor(CRegID account, ProposalType proposalType, CCacheWrapper& cw );
extern uint8_t GetGovernorApprovalMinCount(ProposalType proposalType, CCacheWrapper& cw );

const uint32_t SYS_PARAM_LIST_SIZE_MAX = 50;

bool CGovSysParamProposal::CheckProposal(CTxExecuteContext& context, CBaseTx& tx) {
    CCacheWrapper &cw = *context.pCw;
    CValidationState &state = *context.pState;

    if (param_values.size() == 0 || param_values.size() > SYS_PARAM_LIST_SIZE_MAX)
            return state.DoS(100, ERRORMSG("params size=%u exceed the range (0, %u]", param_values.size(), SYS_PARAM_LIST_SIZE_MAX),
                    REJECT_INVALID, "invalid-params-size");
    for (auto pa: param_values){
        auto paramType = SysParamType(pa.first);
        auto paramValue = pa.second.get();

        if(kSysParamTable.count(paramType) == 0){
             return state.DoS(100, ERRORMSG("parameter name (%s) is not in sys params list ", pa.first),
                       REJECT_INVALID, "params-error");
        }
        auto version = GetFeatureForkVersion(context.height);
        if (version < MAJOR_VER_R3_5 && (paramType == VOTING_CONTRACT_REGID || paramType == BLOCK_INFLATED_REWARD_AMOUNT)) {
             return state.DoS(100, ERRORMSG("unsupport sys param %s at height=%d", GetSysParamName(paramType), context.height),
                       REJECT_INVALID, "unsupport-sys-param");
        }

        string errorInfo = CheckSysParamValue(paramType, paramValue);
        if (errorInfo != EMPTY_STRING)
            return state.DoS(100, ERRORMSG("CheckSysParamValue failed: %s ", errorInfo),
                     REJECT_INVALID, "params-range-error");
        if (paramType == AXC_SWAP_GATEWAY_REGID || paramType == DEX_MATCH_SVC_REGID || paramType == VOTING_CONTRACT_REGID) {
            auto regid = CRegID(paramValue);
            if (regid.IsEmpty() || tx.GetAccount(cw, regid) == nullptr)  {
                return state.DoS(100, ERRORMSG("account of %s not exist, regid_value=%llu",
                        GetSysParamName(paramType), paramValue),
                        REJECT_INVALID, "account-not-exist");
            }
        }
     }

    return true;
}

bool CGovSysParamProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    CCacheWrapper &cw = *context.pCw;

    for( auto pa: param_values){
        auto paramType = SysParamType(pa.first);

        if (!cw.sysParamCache.SetParam(paramType, pa.second.get()))
            return false;

        if (paramType == SysParamType::BP_DELEGATE_VOTE_MIN &&
                !cw.delegateCache.SetLastVoteHeight(context.height)) {
            return false;
        }

    }

    return true;
}

bool CGovBpMcListProposal::CheckProposal(CTxExecuteContext& context, CBaseTx& tx){
    IMPLEMENT_DEFINE_CW_STATE

    if (op_type != ProposalOperateType::ENABLE && op_type != ProposalOperateType::DISABLE)
        return state.DoS(100, ERRORMSG("operate type is illegal!"), REJECT_INVALID,
                         "operate_type-illegal");

    if (!gov_bp_regid.IsMature(context.height)) {
        return state.DoS(100, ERRORMSG("regid (%s) is not matured!", gov_bp_regid.ToString()), REJECT_INVALID,
                         "regid-not-matured");
    }

    auto spGovBpAccount = tx.GetAccount(context, gov_bp_regid, "gov_bp");
    if (!spGovBpAccount)
        return state.DoS(100, ERRORMSG("the account of regid=%s deos not exist", gov_bp_regid.ToString()), REJECT_INVALID,
                         "regid-not-found");

    if (op_type == ProposalOperateType::DISABLE && !cw.sysGovernCache.CheckIsGovernor(gov_bp_regid))
        return state.DoS(100, ERRORMSG("regid(%s) is not a governor!", gov_bp_regid.ToString()), REJECT_INVALID,
                         "regid-not-governor");

    if (op_type == ProposalOperateType::ENABLE && cw.sysGovernCache.CheckIsGovernor(gov_bp_regid))
        return state.DoS(100, ERRORMSG("regid(%s) is a governor already!", gov_bp_regid.ToString()), REJECT_INVALID,
                         "regid-is-governor");

    return true;
}

bool CGovBpMcListProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    CCacheWrapper &cw       = *context.pCw;

    switch (op_type) {
        case ProposalOperateType::DISABLE : return cw.sysGovernCache.EraseGovernor(gov_bp_regid);
        case ProposalOperateType::ENABLE :  return cw.sysGovernCache.AddGovernor(gov_bp_regid);
        default: break;
    }

    return false;
}

bool CGovBpSizeProposal:: CheckProposal(CTxExecuteContext& context, CBaseTx& tx) {
    CValidationState &state = *context.pState;;

    if (total_bps_size == 0) //total_bps_size > BP_MAX_COUNT: always false
        return state.DoS(100, ERRORMSG("total_bps_size must be between 1 and 255"),
                         REJECT_INVALID,"bad-bp-count");


    if (tx.nTxType == TxType::PROPOSAL_REQUEST_TX) {

        uint32_t effectiveBlockCount = BPSSIZE_EFFECTIVE_AFTER_BLOCK_COUNT;
        if(SysCfg().NetworkID() == NET_TYPE::REGTEST_NET)
            effectiveBlockCount = 50;

        if (effective_height < (uint32_t) context.height + effectiveBlockCount)
            return state.DoS(100, ERRORMSG("effective_height must be >= current height + %d", effectiveBlockCount),
                             REJECT_INVALID,"bad-effective-height");
    }


    return true;
}

bool CGovBpSizeProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    IMPLEMENT_DEFINE_CW_STATE;

    auto currentTotalBpsSize = cw.sysParamCache.GetTotalBpsSize(context.height);
    if (!cw.sysParamCache.SetCurrentTotalBpsSize(currentTotalBpsSize)) {
        return state.DoS(100, ERRORMSG("save current bp count failed!"),
                REJECT_INVALID, "save-currtotalbpssize-failed");
    }

    if (!cw.sysParamCache.SetNewTotalBpsSize(total_bps_size, effective_height)) {
        return state.DoS(100, ERRORMSG("save new bp count failed!"),
                REJECT_INVALID, "save-newtotalbpssize-failed");
    }

    return true;

}

bool CGovMinerFeeProposal:: CheckProposal(CTxExecuteContext& context, CBaseTx& tx) {
    CValidationState& state = *context.pState;

    if (!kFeeSymbolSet.count(fee_symbol)) {
        return state.DoS(100, ERRORMSG("fee symbol(%s) is invalid!", fee_symbol),
                        REJECT_INVALID,
                        "feesymbol-error");
    }

    auto itr = kTxTypeInfoTable.find(tx_type);
    if (itr == kTxTypeInfoTable.end()){
        return state.DoS(100, ERRORMSG("the tx type (%d) is invalid!", tx_type),
                        REJECT_INVALID,
                        "txtype-error");
    }

    if (!std::get<5>(itr->second)){
        return state.DoS(100, ERRORMSG("the tx type (%d) miner fee can't be updated!", tx_type),
                        REJECT_INVALID,
                        "can-not-update");
    }

    if (fee_sawi_amount == 0 ){
        return state.DoS(100, ERRORMSG("the tx type (%d) miner fee can't be zero", tx_type),
                        REJECT_INVALID,
                        "can-not-be-zero");
    }
    return true;
}

bool CGovMinerFeeProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    CCacheWrapper &cw       = *context.pCw;
    return cw.sysParamCache.SetMinerFee(tx_type, fee_symbol, fee_sawi_amount);
}

bool CGovCoinTransferProposal::CheckProposal(CTxExecuteContext& context, CBaseTx& tx) {
   IMPLEMENT_DEFINE_CW_STATE

    if(tx.nTxType == TxType::PROPOSAL_REQUEST_TX && tx.sp_tx_account->IsSelfUid(from_uid)) {
        return state.DoS(100,
                         ERRORMSG("can not create this proposal that from_uid is same as txUid"),
                         REJECT_DUST, "tx_uid-can't-be-from_uid");
    }

    if (amount < DUST_AMOUNT_THRESHOLD)
        return state.DoS(100, ERRORMSG("dust amount, %llu < %llu", amount,
                                       DUST_AMOUNT_THRESHOLD), REJECT_DUST, "invalid-coin-amount");

    auto spSrcAccount = tx.GetAccount(context, from_uid, "from");
    if (!spSrcAccount) return false;


    auto spDestAccount = tx.GetAccount(cw, to_uid);
    if (!spDestAccount) {
        if (to_uid.is<CKeyID>()) {
            spDestAccount = tx.NewAccount(cw, to_uid.get<CKeyID>());
        } else if (to_uid.is<CPubKey>()) {
            const auto& pubkey = to_uid.get<CPubKey>();
            spDestAccount = tx.NewAccount(cw, pubkey.GetKeyId());
        } else {
            return state.DoS(100, ERRORMSG("to account of transfer not exist, uid=%s",
                                           to_uid.ToString()),
                             READ_ACCOUNT_FAIL, "account-not-exist");
        }
    }

    if(!spDestAccount) return false;


    return true;
}

bool CGovCoinTransferProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    IMPLEMENT_DEFINE_CW_STATE;

    auto spSrcAccount = tx.GetAccount(context, from_uid, "from");
    if (!spSrcAccount) return false;

    auto spDestAccount = tx.GetAccount(cw, to_uid);
    if (!spDestAccount) {
        if (to_uid.is<CKeyID>()) {
            spDestAccount = tx.NewAccount(cw, to_uid.get<CKeyID>());
        } else if (to_uid.is<CPubKey>()) {
            const auto& pubkey = to_uid.get<CPubKey>();
            spDestAccount = tx.NewAccount(cw, pubkey.GetKeyId());
        } else {
            return state.DoS(100, ERRORMSG("to account of transfer not exist, uid=%s",
                             to_uid.ToString()),
                             READ_ACCOUNT_FAIL, "account-not-exist");
        }
    }


    if (!spSrcAccount->OperateBalance(token, BalanceOpType::SUB_FREE, amount, ReceiptType::TRANSFER_PROPOSAL, tx.receipts, spDestAccount.get()))
        return state.DoS(100, ERRORMSG("account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");

    return true;
}

bool CGovAccountPermProposal::CheckProposal(CTxExecuteContext& context, CBaseTx& tx) {
    CValidationState &state = *context.pState;

    if (account_uid.IsEmpty())
        return state.DoS(100, ERRORMSG("target account_uid is empty"),
                        REJECT_INVALID, "account-uid-empty");

    if (proposed_perms_sum == 0 || proposed_perms_sum > kAccountAllPerms)
        return state.DoS(100, ERRORMSG("proposed perms is invalid: %llu",
                        proposed_perms_sum), REJECT_INVALID, "account-uid-empty");
    return true;

}

bool CGovAccountPermProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {

    auto spAccount = tx.GetAccount(context, account_uid, "uid");
    if (!spAccount) return false;
    spAccount->perms_sum = proposed_perms_sum;
    return true;
}

bool CGovAssetPermProposal::CheckProposal(CTxExecuteContext& context, CBaseTx& tx) {
    CValidationState &state = *context.pState;
    CCacheWrapper &cw       = *context.pCw;

    CAsset asset;
    if (!cw.assetCache.GetAsset(asset_symbol, asset))
        return state.DoS(100, ERRORMSG("asset symbol not found"),
                         REJECT_INVALID, "asset-symbol-invalid");

    if(kCoinTypeSet.count(asset.asset_symbol) != 0)
        return state.DoS(100, ERRORMSG("WICC,WGRT,WICC perm can't be modified"),
                         REJECT_INVALID, "asset-type-error");

    if (proposed_perms_sum == 0)
        return state.DoS(100, ERRORMSG("proposed perms is invalid: %llu",
                                       proposed_perms_sum), REJECT_INVALID, "asset-perms-invalid");

    auto oldHasCdpBcoinPerm = asset.HasPerms(AssetPermType::PERM_CDP_BCOIN);
    auto newHasCdpBcoinPerm = AssetHasPerms(proposed_perms_sum, AssetPermType::PERM_CDP_BCOIN);

    if(oldHasCdpBcoinPerm != newHasCdpBcoinPerm) {
        if (asset_symbol == SYMB::WGRT || kCdpScoinSymbolSet.count(asset_symbol) > 0 || kCdpBcoinSymbolSet.count(asset_symbol) > 0)
            return state.DoS(100, ERRORMSG("asset=%s is a scoin, can not change bcoin perm", asset_symbol),
                             REJECT_INVALID, "change-bcoin-perm-error");
    }

    return true;

}

bool CGovAssetPermProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    CValidationState &state = *context.pState;
    CCacheWrapper &cw       = *context.pCw;

    CAsset asset;
    if (!cw.assetCache.GetAsset(asset_symbol, asset))
        return state.DoS(100, ERRORMSG("Asset not found! symbol=%s", asset_symbol), REJECT_INVALID, "asset-not-found");
    // process cdp bcoin perm
    bool oldCdpBcoinPerm = asset.HasPerms(AssetPermType::PERM_CDP_BCOIN);

    asset.perms_sum = proposed_perms_sum;
    if (!cw.assetCache.SetAsset(asset))
        return state.DoS(100, ERRORMSG("Save asset failed! symbol=%s", asset_symbol), REJECT_INVALID, "save-asset-failed");

    bool newCdpBcoinPerm = asset.HasPerms(AssetPermType::PERM_CDP_BCOIN);
    if (newCdpBcoinPerm != oldCdpBcoinPerm) {

        CdpBcoinStatus status = newCdpBcoinPerm ? CdpBcoinStatus::STAKE_ON : CdpBcoinStatus::STAKE_OFF;
        if (!cw.cdpCache.SetCdpBcoin(asset_symbol, {status, context.GetTxCord()}))
            return state.DoS(100, ERRORMSG("Save bcoin status failed! symbol=%s", asset_symbol),
                REJECT_INVALID, "save-bcoin-status-failed");

    }
    return true;

}

bool CGovCdpParamProposal::CheckProposal(CTxExecuteContext& context, CBaseTx& tx) {

    IMPLEMENT_DEFINE_CW_STATE

    if (param_values.size() == 0 || param_values.size() > 50)
        return state.DoS(100, ERRORMSG("params list is empty or size >50"), REJECT_INVALID,
                         "params-empty");

    for (auto pa: param_values) {
        auto value = pa.second.get();
        if (kCdpParamTable.count(CdpParamType(pa.first)) == 0) {
            return state.DoS(100, ERRORMSG("parameter name (%s) is not in sys params list ",
                            pa.first), REJECT_INVALID, "params-error");
        }

        string errMsg;
        if (!CheckCdpParamValue(CdpParamType(pa.first), value, errMsg))
            return state.DoS(100, ERRORMSG("CheckCdpParamValue failed: %s ", errMsg),
                             REJECT_INVALID, "params-range-error");

        if(pa.first == CdpParamType::CDP_START_COLLATERAL_RATIO
           || pa.first == CdpParamType::CDP_START_LIQUIDATE_RATIO
           || pa.first == CdpParamType::CDP_FORCE_LIQUIDATE_RATIO
           || pa.first == CdpParamType::CDP_LIQUIDATE_DISCOUNT_RATIO) {
            uint64_t startCollateralRatio;
            uint64_t startLiquidateRatio;
            uint64_t forceLiquidateRatio;
            uint64_t liquidateDiscountRatio;
            if (!cw.sysParamCache.GetCdpParam(coin_pair,CDP_START_COLLATERAL_RATIO, startCollateralRatio) ||
                !cw.sysParamCache.GetCdpParam(coin_pair,CDP_START_LIQUIDATE_RATIO, startLiquidateRatio) ||
                !cw.sysParamCache.GetCdpParam(coin_pair,CDP_FORCE_LIQUIDATE_RATIO, forceLiquidateRatio) ||
                !cw.sysParamCache.GetCdpParam(coin_pair,CDP_LIQUIDATE_DISCOUNT_RATIO, liquidateDiscountRatio)) {
                return state.DoS(100, ERRORMSG("get CDP_START_COLLATERAL_RATIO CDP_START_LIQUIDATE_RATIO CDP_FORCE_LIQUIDATE_RATIO error"
                ), REJECT_INVALID, "params-relation-error");
            }

            switch (pa.first) {
                case CDP_LIQUIDATE_DISCOUNT_RATIO:
                    liquidateDiscountRatio = value;
                    break;
                case CDP_START_LIQUIDATE_RATIO:
                    startLiquidateRatio = value;
                    break;
                case CDP_START_COLLATERAL_RATIO:
                    startCollateralRatio = value;
                    break;
                case CDP_FORCE_LIQUIDATE_RATIO:
                    forceLiquidateRatio = value;
                    break;
                default:
                    break;
            }

            if(startCollateralRatio <= startLiquidateRatio
               || startLiquidateRatio <= forceLiquidateRatio
               || startCollateralRatio <= forceLiquidateRatio) {
                return state.DoS(100, ERRORMSG("check CDP_START_COLLATERAL_RATIO CDP_START_LIQUIDATE_RATIO"
                                               " CDP_FORCE_LIQUIDATE_RATIO relationship error"), REJECT_INVALID, "params-relation-error");
            }

            if(forceLiquidateRatio * liquidateDiscountRatio < RATIO_BOOST * RATIO_BOOST) {
                return state.DoS(100, ERRORMSG("check CDP_LIQUIDATE_DISCOUNT_RATIO * "
                                               "CDP_FORCE_LIQUIDATE_RATIO  must >= RATIO_BOOST*RATIO_BOOST"), REJECT_INVALID, "params-check-error");
            }
        }


    }

    return true;
}

bool CGovCdpParamProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {

    IMPLEMENT_DEFINE_CW_STATE
    for (auto pa: param_values){
        auto value = pa.second.get();
        auto itr = kCdpParamTable.find(CdpParamType(pa.first));
        if (itr == kCdpParamTable.end())
            return state.DoS(100, ERRORMSG(" cdpparam type error"), REJECT_INVALID, "bad-cdptype");

        if (!cw.sysParamCache.SetCdpParam(coin_pair,CdpParamType(pa.first), value))
            return state.DoS(100, ERRORMSG("save cdpparam  error"), REJECT_INVALID, "setparam-error");

        if (pa.first == CdpParamType ::CDP_INTEREST_PARAM_A || pa.first == CdpParamType::CDP_INTEREST_PARAM_B) {
            if (!cw.sysParamCache.SetCdpInterestParam(coin_pair, CdpParamType(pa.first), context.height, value))
                return state.DoS(100, ERRORMSG("SetCdpInterestParam  error"), REJECT_INVALID, "setcdpinterestparam-error");
        }
    }

    return true;
}

bool CGovDexOpProposal::CheckProposal(CTxExecuteContext& context, CBaseTx& tx) {
    IMPLEMENT_DEFINE_CW_STATE

    if (dexid == 0)
        return state.DoS(100,ERRORMSG("the No.0 dex operator can't be disable"),
                REJECT_INVALID, "operator0-can't-be-switched");

    if (operate_type != ProposalOperateType::ENABLE && operate_type != ProposalOperateType::DISABLE)
        return state.DoS(100, ERRORMSG("operate type error!"), REJECT_INVALID,
                         "operate-type-error");

    DexOperatorDetail dexOperator;
    if (!cw.dexCache.GetDexOperator(dexid, dexOperator))
        return state.DoS(100, ERRORMSG("dexoperator(%d) is not a governor!", dexid), REJECT_INVALID,
                         "dexoperator-not-exist");

    if ((dexOperator.activated && operate_type == ProposalOperateType::ENABLE)||
        (!dexOperator.activated && operate_type == ProposalOperateType::DISABLE))
        return state.DoS(100, ERRORMSG("dexoperator(%d) is activated or not activated already !", dexid), REJECT_INVALID,
                         "need-not-update");

    return true;
}

bool CGovDexOpProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    IMPLEMENT_DEFINE_CW_STATE

    DexOperatorDetail dexOperator;
    if (!cw.dexCache.GetDexOperator(dexid, dexOperator))
        return state.DoS(100, ERRORMSG("dexoperator(%d) is not a governor!", dexid), REJECT_INVALID,
                         "dexoperator-not-exist");

    if ((dexOperator.activated && operate_type == ProposalOperateType::ENABLE)||
       (!dexOperator.activated && operate_type == ProposalOperateType::DISABLE))
        return state.DoS(100, ERRORMSG("dexoperator(%d) is activated or not activated already !", dexid), REJECT_INVALID,
                         "need-not-update");

    DexOperatorDetail newOperator = dexOperator;
    newOperator.activated = (operate_type == ProposalOperateType::ENABLE);

    if (!cw.dexCache.UpdateDexOperator(dexid, dexOperator, newOperator))
        return state.DoS(100, ERRORMSG("Save updated DEX operator error! dex_id=%u", dexid),
                         UPDATE_ACCOUNT_FAIL, "save-updated-operator-error");

    return true;
}

bool CGovFeedCoinPairProposal::CheckProposal(CTxExecuteContext& context, CBaseTx& tx) {
    IMPLEMENT_DEFINE_CW_STATE;

    if (op_type == ProposalOperateType::NULL_PROPOSAL_OP)
        return state.DoS(100, ERRORMSG("op_type is null"), REJECT_INVALID, "bad-op-type");

    if (base_symbol == quote_symbol)
        return state.DoS(100, ERRORMSG("base_symbol==quote_symbol"), REJECT_INVALID, "same-base-quote-symbol");

    if (!cw.assetCache.CheckPriceFeedQuoteSymbol(quote_symbol))
        return state.DoS(100, ERRORMSG("Unsupported quote_symbol=%s", quote_symbol), REJECT_INVALID, "unsupported-quote-symbol");

    if (!cw.assetCache.CheckAsset(base_symbol, AssetPermType::PERM_PRICE_FEED))
        return state.DoS(100, ERRORMSG("Unsupported base_symbol=%s", base_symbol), REJECT_INVALID, "unsupported-base-symbol");

    PriceCoinPair coinPair(base_symbol, quote_symbol);
    if (kPriceFeedCoinPairSet.count(coinPair) > 0) {
        return state.DoS(100, ERRORMSG("The hard code price_coin_pair={%s:%s} can not be governed", quote_symbol),
                        REJECT_INVALID, "hard-code-coin-pair");
    }

    bool hasCoin = cw.priceFeedCache.HasFeedCoinPair(coinPair);
    if (hasCoin && op_type == ProposalOperateType ::ENABLE) {
        return state.DoS(100, ERRORMSG("base_symbol(%s),quote_symbol(%s)"
                                       "is dex quote coin symbol already", base_symbol, quote_symbol),
                         REJECT_INVALID, "symbol-exist");
    }

    if (!hasCoin && op_type == ProposalOperateType ::DISABLE) {
        return state.DoS(100, ERRORMSG("base_symbol(%s),quote_symbol(%s) "
                                       "is not a dex quote coin symbol ",base_symbol, quote_symbol),
                         REJECT_INVALID, "symbol-not-exist");
    }

    return true;
}

bool CGovFeedCoinPairProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    CCacheWrapper& cw = *context.pCw;
    PriceCoinPair coinPair(base_symbol, quote_symbol);
    if (ProposalOperateType::ENABLE == op_type)
        return cw.priceFeedCache.AddFeedCoinPair(coinPair);
    else
        return cw.priceFeedCache.EraseFeedCoinPair(coinPair);

}

bool CGovAxcInProposal::CheckProposal(CTxExecuteContext& context, CBaseTx& tx) {
    IMPLEMENT_DEFINE_CW_STATE;

    CAxcSwapPairStore swapPair;
    if(!cw.assetCache.GetAxcCoinPairByPeerSymbol(peer_chain_token_symbol, swapPair)){
        return state.DoS(
            100, ERRORMSG("axc swap pair peer_chain_token_symbol=%s does not exist", peer_chain_token_symbol),
            REJECT_INVALID, "axc-swap-pair-not-exist");
    }
    if(swapPair.status != ProposalOperateType::ENABLE){
        return state.DoS(100,
                         ERRORMSG("axc swap pair peer_chain_token_symbol=%s is DISABLE", peer_chain_token_symbol),
                         REJECT_INVALID, "axc-swap-pair-disable");
    }

    TokenSymbol self_chain_token_symbol = swapPair.GetSelfSymbol();
    ChainType  peer_chain_type = swapPair.peer_chain_type;

    if ((peer_chain_type == ChainType::BITCOIN && (peer_chain_addr.size() < 26 || peer_chain_addr.size() > 35)) ||
        (peer_chain_type == ChainType::ETHEREUM && (peer_chain_addr.size() > 42)))
        return state.DoS(100, ERRORMSG("peer_chain_addr=%s invalid",
                                        peer_chain_addr), REJECT_INVALID, "peer_chain_addr-invalid");

    if ( (peer_chain_type == ChainType::BITCOIN && (peer_chain_txid.size() != 66)) ||
         (peer_chain_type == ChainType::ETHEREUM && (peer_chain_txid.size() != 66)) )
        return state.DoS(100, ERRORMSG("peer_chain_txid=%s invalid",
                                        peer_chain_txid), REJECT_INVALID, "peer_chain_txid-invalid");
    if (self_chain_uid.IsEmpty())
        return state.DoS(100, ERRORMSG("self_chain_uid empty"),
                                        REJECT_INVALID, "self_chain_uid-empty");

    auto spAccount = tx.GetAccount(context, self_chain_uid, "self_chain");
    if (!spAccount) return false;

    if (swap_amount < DUST_AMOUNT_THRESHOLD)
        return state.DoS(100, ERRORMSG("swap_amount=%llu too small than %llu",
                                        swap_amount, DUST_AMOUNT_THRESHOLD), REJECT_INVALID, "swap_amount-dust");
    if (swap_amount > MAX_ASSET_TOTAL_SUPPLY)
        return state.DoS(100, ERRORMSG("swap_amount=%llu too large than %llu",
                                        swap_amount, MAX_ASSET_TOTAL_SUPPLY), REJECT_INVALID, "swap-amount-too-large");

    uint64_t mintAmount = 0;
    if (cw.axcCache.GetSwapInMintRecord(peer_chain_type, peer_chain_txid, mintAmount))
        return state.DoS(100, ERRORMSG("GetSwapInMintRecord existing err  %s", peer_chain_txid),
                        REJECT_INVALID, "get_swapin_mint-record-err");
    CAsset asset;
    if(!cw.assetCache.GetAsset(self_chain_token_symbol, asset)) {
        return state.DoS(100, ERRORMSG("don't find axc asset %s", self_chain_token_symbol),
                                       REJECT_INVALID, "get-axc-dia-asset-err");
    }

    uint128_t totalSupply = asset.total_supply + swap_amount;
    if (totalSupply > MAX_ASSET_TOTAL_SUPPLY)
        return state.DoS(100, ERRORMSG("asset total_supply=%llu + swap_amount=%llu too large than %llu",
                                        asset.total_supply, swap_amount, MAX_ASSET_TOTAL_SUPPLY),
                                        REJECT_INVALID, "total-supply-too-large");
    return true;
}

bool ProcessAxcInFee(CTxExecuteContext& context, CBaseTx& tx, TokenSymbol& selfChainTokenSymbol, uint64_t& swapFees) {
    IMPLEMENT_DEFINE_CW_STATE

    vector<CRegID> govBpRegIds;
    TxID proposalId = ((CProposalApprovalTx &) tx).proposal_id;
    if (!cw.sysGovernCache.GetApprovalList(proposalId, govBpRegIds) || govBpRegIds.size() == 0)
        return state.DoS(100, ERRORMSG("failed to get BP Governors"),
                         REJECT_INVALID, "bad-get-bp-governors");

    uint64_t swapFeesPerBp = swapFees / (govBpRegIds.size() + 3);
    for (const auto &bpRegID : govBpRegIds) {
        auto spBpAccount = tx.GetAccount(context, bpRegID, "self_chain");
        if (!spBpAccount) return false;

        if (!spBpAccount->OperateBalance(selfChainTokenSymbol, BalanceOpType::ADD_FREE, swapFeesPerBp,
                                   ReceiptType::AXC_REWARD_FEE_TO_GOVERNOR, tx.receipts))
            return state.DoS(100,
                             ERRORMSG("opreate balance failed, swapFeesPerBp=%llu",
                                      swapFeesPerBp), REJECT_INVALID, "bad-operate-balance");
    }

    uint64_t swapFeeForGw = swapFees - swapFeesPerBp * govBpRegIds.size();

    CRegID axcgwId;
    if(!cw.sysParamCache.GetAxcSwapGwRegId(axcgwId)) {
        return state.DoS(100, ERRORMSG("failed to get GW regid (%s)",
                                       axcgwId.ToString()),
                         REJECT_INVALID, "bad-get-gw-account");
    }

    auto spAxcgwAccount = tx.GetAccount(context, axcgwId, "axcgwId");
    if (!spAxcgwAccount) return false;

    if (!spAxcgwAccount->OperateBalance(selfChainTokenSymbol, BalanceOpType::ADD_FREE, swapFeeForGw,
                                     ReceiptType::AXC_REWARD_FEE_TO_GW, tx.receipts))
        return state.DoS(100, ERRORMSG("opreate balance failed, swapFeesPerBp=%llu",
                                       swapFeesPerBp), REJECT_INVALID, "bad-operate-balance");
    return true;
}

bool CGovAxcInProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    IMPLEMENT_DEFINE_CW_STATE;

    CAxcSwapPairStore swapPair;
    if (!cw.assetCache.GetAxcCoinPairByPeerSymbol(peer_chain_token_symbol, swapPair)) {
        return state.DoS(100, ERRORMSG("swap pair is not exist"),
                         REJECT_INVALID, "find-swapcoinpair-error");
    }
    TokenSymbol self_chain_token_symbol = swapPair.GetSelfSymbol();
    ChainType peer_chain_type = swapPair.peer_chain_type;


    uint64_t swap_fee_ratio;
    if (!cw.sysParamCache.GetParam(AXC_SWAP_FEE_RATIO, swap_fee_ratio) || swap_fee_ratio * 1.0 / RATIO_BOOST > 1)
        return state.DoS(100, ERRORMSG("get sysparam: axc_swap_fee_ratio failed"),
                         REJECT_INVALID, "bad-get-swap_fee_ratio");

    uint64_t swap_fees = swap_fee_ratio * (swap_amount * 1.0 / RATIO_BOOST);
    uint64_t swap_amount_after_fees = swap_amount - swap_fees;

    if (!ProcessAxcInFee(context,tx, self_chain_token_symbol, swap_fees)) {
        return state.DoS(100, ERRORMSG("process swap in fee error"), REJECT_INVALID,
                         "bad-process-swapfee");
    }

    auto spSelfChainAccount = tx.GetAccount(context, self_chain_uid, "self_chain");
    if (!spSelfChainAccount) return false;

    // mint the new mirro-coin (self_chain_token_symbol) out of thin air
    if (!spSelfChainAccount->OperateBalance(self_chain_token_symbol, BalanceOpType::ADD_FREE, swap_amount_after_fees,
                                ReceiptType::AXC_MINT_COINS, tx.receipts))
        return state.DoS(100, ERRORMSG("opreate balance failed, swap_amount_after_fees=%llu",
                                        swap_amount_after_fees), REJECT_INVALID, "bad-operate-balance");

    uint64_t mintAmount = 0;
    if (cw.axcCache.GetSwapInMintRecord(peer_chain_type, peer_chain_txid, mintAmount))
        return state.DoS(100, ERRORMSG("GetSwapInMintRecord existing err %s",
                        REJECT_INVALID, "get_swapin_mint_record-err"));

    if (!cw.axcCache.SetSwapInMintRecord(peer_chain_type, peer_chain_txid, swap_amount_after_fees))
        return state.DoS(100, ERRORMSG("SetSwapInMintRecord existing err %s",
                        REJECT_INVALID, "get_swapin_mint_record-err"));

    //add dia total supply
    CAsset asset;
    if(!cw.assetCache.GetAsset(self_chain_token_symbol, asset)) {
        return state.DoS(100, ERRORMSG("don't find axc asset %s", self_chain_token_symbol),
                                       REJECT_INVALID, "get-axc-dia-asset-err");
    }

    uint128_t totalSupply = asset.total_supply + swap_amount;
    if (totalSupply > MAX_ASSET_TOTAL_SUPPLY)
        return state.DoS(100, ERRORMSG("asset total_supply=%llu + swap_amount=%llu too large than %llu",
                                        asset.total_supply, swap_amount, MAX_ASSET_TOTAL_SUPPLY),
                                        REJECT_INVALID, "total-supply-too-large");
    asset.OperateToTalSupply(swap_amount, TotalSupplyOpType::ADD);
    if(!cw.assetCache.SetAsset( asset)) {
        return state.DoS(100, ERRORMSG("save axc asset error %s", asset.asset_symbol ),
                                       REJECT_INVALID, "save-axc-dia-asset-err");
    }

    return true;
}

bool CGovAxcOutProposal::CheckProposal(CTxExecuteContext& context, CBaseTx& tx) {
    IMPLEMENT_DEFINE_CW_STATE;

    CAxcSwapPairStore swapPair;
    if(!cw.assetCache.GetAxcCoinPairBySelfSymbol(self_chain_token_symbol, swapPair)){
        return state.DoS(
            100, ERRORMSG("self_chain_token_symbol=%s does not exist", self_chain_token_symbol),
            REJECT_INVALID, "self-chain-token-symbol-not-exist");
    }
    if(swapPair.status != ProposalOperateType::ENABLE){
        return state.DoS(100,
                         ERRORMSG("self_chain_token_symbol=%s is DISABLE", self_chain_token_symbol),
                         REJECT_INVALID, "self-chain-token-symbol-disable");
    }

    ChainType  peer_chain_type = swapPair.peer_chain_type;
    if ((peer_chain_type == ChainType::BITCOIN && (peer_chain_addr.size() < 26 || peer_chain_addr.size() > 35)) ||
        (peer_chain_type == ChainType::ETHEREUM && (peer_chain_addr.size() > 42)))
        return state.DoS(100, ERRORMSG("peer_chain_addr=%s invalid",
                                        peer_chain_addr), REJECT_INVALID, "peer_chain_addr-invalid");
    CUserID uid;
    if(tx.nTxType == TxType::PROPOSAL_REQUEST_TX)
        uid = tx.txUid;
    else
        uid = self_chain_uid;

    auto spSelfChainAccount = tx.GetAccount(context, uid, "self_chain");
    if (!spSelfChainAccount) return false;

    if (!spSelfChainAccount->CheckBalance(self_chain_token_symbol, BalanceType::FREE_VALUE, swap_amount))
        return state.DoS(100, ERRORMSG("Account does not have enough %s",
                                   self_chain_token_symbol), REJECT_INVALID, "balance-not-enough");

    if (swap_amount < DUST_AMOUNT_THRESHOLD)
        return state.DoS(100, ERRORMSG("swap_amount=%llu too small",
                                        swap_amount), REJECT_INVALID, "swap_amount-dust");

    if (swap_amount > MAX_ASSET_TOTAL_SUPPLY)
        return state.DoS(100, ERRORMSG("swap_amount=%llu too large than %llu",
                                        swap_amount, MAX_ASSET_TOTAL_SUPPLY), REJECT_INVALID, "swap-amount-too-large");
    return true;
}

bool CGovAxcOutProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    IMPLEMENT_DEFINE_CW_STATE;

    uint64_t swap_fee_ratio;
    if (!cw.sysParamCache.GetParam(AXC_SWAP_FEE_RATIO, swap_fee_ratio))
        return state.DoS(100, ERRORMSG("get sysparam: axc_swap_fee_ratio failed"),
                        REJECT_INVALID, "bad-get-swap_fee_ratio");

    auto spSelfChainAccount = tx.GetAccount(context, self_chain_uid, "self_chain");
    if (!spSelfChainAccount) return false;

    // burn the mirroed tokens from self-chain
    if (!spSelfChainAccount->OperateBalance(self_chain_token_symbol, BalanceOpType::SUB_FREE,
                                            swap_amount, ReceiptType::AXC_BURN_COINS, tx.receipts))
        return state.DoS(100, ERRORMSG("opreate balance failed, swap_amount=%llu",
                                        swap_amount), REJECT_INVALID, "bad-operate-balance");

    //sub dia total supply
    CAsset asset;
    if(!cw.assetCache.GetAsset(self_chain_token_symbol, asset)) {
        return state.DoS(100, ERRORMSG("don't find axc asset %s", self_chain_token_symbol),
                         REJECT_INVALID, "get-axc-dia-asset-err");
    }

    if (swap_amount > asset.total_supply)
        return state.DoS(100, ERRORMSG("asset swap_amount=%llu too large than total_supply=%llu",
                                        swap_amount, asset.total_supply),
                                        REJECT_INVALID, "swap-amount-exceed-total-supply");
    asset.OperateToTalSupply(swap_amount, TotalSupplyOpType::SUB);
    if(!cw.assetCache.SetAsset( asset)) {
        return state.DoS(100, ERRORMSG("save axc asset error %s", asset.asset_symbol ),
                         REJECT_INVALID, "save-axc-dia-asset-err");
    }

    return true;
}

bool CGovAxcCoinProposal::CheckProposal(CTxExecuteContext& context, CBaseTx& tx) {
    IMPLEMENT_DEFINE_CW_STATE;

    if (op_type != ProposalOperateType::ENABLE && op_type != ProposalOperateType::DISABLE)
        return state.DoS(100, ERRORMSG("op_type is not 1 or 2"), REJECT_INVALID,
                         "op-type-error");

    const auto &selfSymbol = GenSelfChainCoinSymbol();
    CAxcSwapPairStore swapPair;
    if (!cw.assetCache.GetAxcCoinPairByPeerSymbol(peer_chain_coin_symbol, swapPair)) {
        if (op_type != ProposalOperateType::ENABLE) {
            return state.DoS(100, ERRORMSG("the axc swap coin=%s must be enabled for op_type=%s",
                    peer_chain_coin_symbol, kProposalOperateTypeHelper.GetName(op_type)),
                    REJECT_INVALID, "axc-coin-not-enabled");
        }
        if (cw.assetCache.HasAsset(selfSymbol))
            return state.DoS(100, ERRORMSG("the asset of symbol=%s is exist", selfSymbol),
                             REJECT_INVALID, "asset-exist");
    } else {
        if (swapPair.peer_chain_type != peer_chain_type)
            return state.DoS(100, ERRORMSG("the peer_chain_type=%s unmatch with the existed=%s",
                    peer_chain_type, swapPair.peer_chain_type), REJECT_INVALID, "peer-chain-type-unmatch");
        if (swapPair.status == op_type) {
            return state.DoS(100, ERRORMSG("the swap pair has been in '%s' status",
                    kProposalOperateTypeHelper.GetName(op_type)), REJECT_INVALID, "repeated-op-type");
        }
    }

    switch (peer_chain_type) {
        case ChainType ::BITCOIN:
        case ChainType ::EOS:
        case ChainType ::ETHEREUM:
            break;
        default:
            return state.DoS(100, ERRORMSG("chain type is not eos, btc, ethereum"), REJECT_INVALID,
                             "chain_type-error");
    }

    if (peer_chain_coin_symbol.size() > 6)
        return state.DoS(100, ERRORMSG("peer_chain_coin_symbol size=%u is too long than %u",
                    peer_chain_coin_symbol.size(), 6), REJECT_INVALID, "peer-coin-symbol-error");

    auto selfChainCoinSymbol = GenSelfChainCoinSymbol();
    string errMsg;
    if (!CAsset::CheckSymbol(context.height, AssetType::DIA, selfChainCoinSymbol, errMsg )) {
        return state.DoS(100, ERRORMSG("Invalid symbol=%s: %s", selfChainCoinSymbol, errMsg.c_str()),
                REJECT_INVALID, "bad-symbol");
    }
    return true;
}

bool  CGovAxcCoinProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    IMPLEMENT_DEFINE_CW_STATE

    CAxcSwapPairStore swapPair;
    if (!cw.assetCache.GetAxcCoinPairByPeerSymbol(peer_chain_coin_symbol, swapPair)) {
        //Persist with Owner's RegID to save space than other User ID types
        auto selfChainCoinSymbol = GenSelfChainCoinSymbol();
        CAsset savedAsset(selfChainCoinSymbol, selfChainCoinSymbol, AssetType::DIA, kAssetDefaultPerms,
                          CRegID(), 0, false);

        if (!cw.assetCache.SetAsset(savedAsset))
            return state.DoS(100, ERRORMSG("save asset failed! peer_chain_coin_symbol=%s",
                                           peer_chain_coin_symbol), UPDATE_ACCOUNT_FAIL, "save-asset-failed");
        swapPair = {
            op_type,
            peer_chain_coin_symbol,
            peer_chain_type
        };
    } else {
        swapPair.status = op_type;
    }

    if (!cw.assetCache.SetAxcSwapPair(swapPair))
        return state.DoS(100, ERRORMSG("write db error"), REJECT_INVALID,
                            "db-error");

    return true;
}

bool CGovAssetIssueProposal::CheckProposal(CTxExecuteContext& context, CBaseTx& tx) {

    IMPLEMENT_DEFINE_CW_STATE

    string errMsg;
    if (!CAsset::CheckSymbol(context.height, AssetType::DIA, asset_symbol, errMsg )) {
        return state.DoS(100, ERRORMSG(errMsg.c_str()), REJECT_INVALID,
                         "bad-symbol");
    }
    if (asset_symbol.at(0) == 'm') {
        return state.DoS(100, ERRORMSG("asset symbol=%s can not start with 'm'", asset_symbol), REJECT_INVALID,
                         "bad-symbol");
    }

    if ( total_supply > MAX_ASSET_TOTAL_SUPPLY)
        return state.DoS(100, ERRORMSG("asset total_supply=%llu can not == 0 or > %llu",
                                       total_supply, MAX_ASSET_TOTAL_SUPPLY), REJECT_INVALID, "invalid-total-supply");

    auto spOwnerAccount = tx.GetAccount(context, owner_regid, "owner");
    if (!spOwnerAccount) return false;

    if (cw.assetCache.HasAsset(asset_symbol))
        return state.DoS(100, ERRORMSG("asset_symbol is exist"), REJECT_INVALID,
                         "asset-exist");


    return true;
}

bool CGovAssetIssueProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {

    IMPLEMENT_DEFINE_CW_STATE

    CAsset asset(asset_symbol, asset_symbol, AssetType::DIA, kAssetDefaultPerms, owner_regid, total_supply, true);

    if (!cw.assetCache.SetAsset(asset)) {
        return state.DoS(100, ERRORMSG("save asset error"), REJECT_INVALID,
                         "asset-write-error");
    }
    auto spOwnerAccount = tx.GetAccount(context, owner_regid, "owner");
    if (!spOwnerAccount) {
        return state.DoS(100, ERRORMSG("fail to find owner account, owner_id = %s",
                                       owner_regid.ToString()), UPDATE_ACCOUNT_FAIL, "account-not-found");
    }
    if (!spOwnerAccount->OperateBalance(asset.asset_symbol, BalanceOpType::ADD_FREE, asset.total_supply,
                                        ReceiptType::ASSET_MINT_NEW_AMOUNT, tx.receipts)) {
        return state.DoS(100, ERRORMSG("fail to add total_supply to issued account! total_supply=%llu, txUid=%s",
                                       asset.total_supply, owner_regid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficent-funds");
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// CGovCancelOrderProposal
bool CGovCancelOrderProposal::CheckProposal(CTxExecuteContext& context, CBaseTx& tx) {
    CValidationState &state = *context.pState;

    if (order_id.IsEmpty())
        return state.DoS(100, ERRORMSG("order_id is empty"), REJECT_INVALID,
                        "invalid-order-id");
    return true;
}

bool CGovCancelOrderProposal::ExecuteProposal(CTxExecuteContext& context, CBaseTx& tx) {
    IMPLEMENT_DEFINE_CW_STATE;
    using namespace dex;

    CDEXOrderDetail activeOrder;
    if (!cw.dexCache.GetActiveOrder(order_id, activeOrder)) {
        return state.DoS(100, ERRORMSG("the order=%s is inactive or not existed", order_id.ToString()),
                        REJECT_INVALID, "order-inactive");
    }

    if (activeOrder.generate_type != USER_GEN_ORDER) {
        return state.DoS(100, ERRORMSG("only support canceling user generated order, order_id=%s", order_id.ToString()),
                        REJECT_INVALID, "order-not-generated-by-user");
    }

    auto sp_account = tx.GetAccount(context, activeOrder.user_regid, "order");
    if (!sp_account) return false; // error has been processed

    // get frozen money
    TokenSymbol frozenSymbol;
    uint64_t frozenAmount = 0;
    ReceiptType code;
    if (activeOrder.order_side == ORDER_BUY) {
        frozenSymbol = activeOrder.coin_symbol;
        frozenAmount = activeOrder.coin_amount - activeOrder.total_deal_coin_amount;
        code = ReceiptType::DEX_UNFREEZE_COIN_TO_BUYER;

    } else if(activeOrder.order_side == ORDER_SELL) {
        frozenSymbol = activeOrder.asset_symbol;
        frozenAmount = activeOrder.asset_amount - activeOrder.total_deal_asset_amount;
        code = ReceiptType::DEX_UNFREEZE_ASSET_TO_SELLER;
    } else {
        return state.DoS(100, ERRORMSG("Order side must be ORDER_BUY|ORDER_SELL, order_id=%s", order_id.ToString()),
                        REJECT_INVALID, "invalid-order-side");
    }

    if (!sp_account->OperateBalance(frozenSymbol, UNFREEZE, frozenAmount, code, tx.receipts))
        return state.DoS(100, ERRORMSG("account=%s has (%llu) insufficient frozen amount to unfreeze(%llu)",
                order_id.ToString(), frozenAmount, sp_account->GetToken(frozenSymbol).frozen_amount),
                UPDATE_ACCOUNT_FAIL, "unfreeze-account-failed");

    if (!cw.dexCache.EraseActiveOrder(order_id, activeOrder)) {
        return state.DoS(100, ERRORMSG("erase active order failed! order_id=%s", order_id.ToString()),
                        REJECT_INVALID, "order-erase-failed");
    }

    return true;
}
