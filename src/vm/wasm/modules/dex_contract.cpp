// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dex_contract.hpp"
#include "wasm/datastream.hpp"
#include "wasm/types/types.hpp"
#include <string>
#include <tuple>

using namespace std;

static bool is_url_char(const char &c) {
    return  (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            (c == '/' || c == '.' || c == '-' || c == '_' || c == '@');
}

static bool check_url(const string &url) {
    for (const auto &c : url) {
        if (!is_url_char(c))
            return false;
    }
    return true;
}

static const string ASSET_ACTION_ISSUE = "register";
static const string ASSET_ACTION_UPDATE = "update";

namespace wasm_account {

    string to_string(const CUserID &uid) {
        return uid.ToDebugString();
    }

    static shared_ptr<CAccount> get_account(CCacheWrapper &cw, const CUserID &uid, const string &err_title) {
        shared_ptr<CAccount> account_sp = make_shared<CAccount>();
        WASM_ASSERT(cw.accountCache.GetAccount(uid, *account_sp), account_operation_exception,
                    "%s, account does not exist, uid=%s", err_title, to_string(uid))
        return account_sp;
    }

    static void check_account_existed(CCacheWrapper &cw, const CUserID &uid, const string &err_title) {
        WASM_ASSERT(cw.accountCache.HaveAccount(uid), account_operation_exception,
                    "%s, account does not exist, uid=%s", err_title, to_string(uid))
    }

    static void sub_free_balance(CAccount &account, const TokenSymbol &symbol, uint64_t amount,
                                const string &err_title) {
        WASM_ASSERT(account.OperateBalance(symbol, BalanceOpType::SUB_FREE, amount),
                    account_operation_exception,
                    "%s, subtract fee amount from account failed, name=%s, addr=%s, money=%s:%d",
                    err_title, account.nickid.ToString(), account.keyid.ToAddress(), symbol,
                    amount);
    }

    static void add_free_balance(CAccount &account, const TokenSymbol &symbol, uint64_t amount,
                                 const string &err_title) {
        WASM_ASSERT(account.OperateBalance(symbol, BalanceOpType::ADD_FREE, amount),
                    account_operation_exception,
                    "%s, add free amount to account failed, name=%s, addr=%s, money=%s:%d",
                    err_title, account.nickid.ToString(), account.keyid.ToAddress(), symbol,
                    amount);
    }

    static void save(CCacheWrapper &cw, CAccount &account, const string &err_title) {
        WASM_ASSERT(cw.accountCache.SetAccount(account.keyid, account), account_operation_exception,
                "%s save account error, name=%s, addr=%s", err_title, account.nickid.ToString(), account.keyid.ToAddress())
    }

}

#define ERROR_TITLE(msg) (std::string(__FUNCTION__) + "(), " + msg)

static uint64_t get_sys_param(CCacheWrapper &cw, const SysParamType &param_type) {
    uint64_t value = 0;
    WASM_ASSERT(cw.sysParamCache.GetParam(param_type, value), wasm_assert_exception,
        "%s(), read sys param error! type=%d", __func__, param_type);
    return value;
}

//DEX_OPERATOR_REGISTER_FEE
static void check_dex_operator_fee(CCacheWrapper &cw, const SysParamType &param_type, const wasm::asset &fee) {
    WASM_ASSERT(fee.sym.code().to_string() == SYMB::WICC, wasm_assert_exception, "fee.symbol must be WICC");
    WASM_ASSERT(fee.sym.precision() == WICC_PRECISION, wasm_assert_exception, "fee.precision must be %d", WICC_PRECISION);
    uint64_t default_fee = get_sys_param(cw, param_type);
    WASM_ASSERT(fee.amount == default_fee, wasm_assert_exception, "fee.amount must be %d", default_fee);
}

static void process_dex_operator_fee(wasm_context &context, const wasm::asset &fee, const string &action,
    CAccount &registrant_account) {

    const TokenSymbol &symbol = fee.sym.code().to_string();
    uint64_t amount = fee.amount;

    wasm_account::sub_free_balance(registrant_account, symbol, amount, ERROR_TITLE("registered fee"));
    wasm_account::save(context.database, registrant_account, ERROR_TITLE("registrant"));

    uint64_t risk_fee       = amount * DEX_OPERATOR_RISK_FEE_RATIO / RATIO_BOOST;
    uint64_t total_delegate_fee = amount - risk_fee;

    shared_ptr<CAccount> sp_risk_reserve_account = make_shared<CAccount>();
    WASM_ASSERT(context.database.accountCache.GetFcoinGenesisAccount(*sp_risk_reserve_account), wasm_assert_exception,
        "%s(), risk reserve account does not existed", __func__);

    wasm_account::add_free_balance(*sp_risk_reserve_account, symbol, risk_fee, ERROR_TITLE("risk fee"));
    wasm_account::save(context.database, *sp_risk_reserve_account, ERROR_TITLE("risk reserve"));

    if (action == ASSET_ACTION_ISSUE)
        context.receipts.emplace_back(registrant_account.nickid, sp_risk_reserve_account->regid, symbol,
                              risk_fee, ReceiptCode::DEX_OPERATOR_REG_FEE_TO_RESERVE);
    else
        context.receipts.emplace_back(registrant_account.nickid, sp_risk_reserve_account->regid, symbol,
            risk_fee, ReceiptCode::DEX_OPERATOR_UPDATED_FEE_TO_RESERVE);

    VoteDelegateVector delegates;
    WASM_ASSERT(context.database.delegateCache.GetActiveDelegates(delegates), wasm_assert_exception,
        "%s(), GetActiveDelegates failed", __func__);
   // auto delegateNum = context.database.sysParamCache.GetBpCount()
   //TODO replace IniCfg().GetTotalDelegateNum
   assert(delegates.size() != 0 && delegates.size() == IniCfg().GetTotalDelegateNum());

    for (size_t i = 0; i < delegates.size(); i++) {
        const CRegID &delegate_regid = delegates[i].regid;
        shared_ptr<CAccount> sp_delegate_account = wasm_account::get_account(context.database, delegate_regid, ERROR_TITLE("delegate"));

        uint64_t delegate_fee = total_delegate_fee / delegates.size();
        if (i == 0) delegate_fee += total_delegate_fee % delegates.size(); // give the dust amount to topmost miner

        wasm_account::add_free_balance(*sp_delegate_account, symbol, delegate_fee, ERROR_TITLE("delegate fee"));
        wasm_account::save(context.database, *sp_delegate_account, ERROR_TITLE("delegate"));

        if (action == ASSET_ACTION_ISSUE)
            context.receipts.emplace_back(registrant_account.nickid, delegate_regid, symbol, delegate_fee,
                                  ReceiptCode::DEX_OPERATOR_REG_FEE_TO_MINER);
        else
            context.receipts.emplace_back(registrant_account.nickid, delegate_regid, symbol, delegate_fee,
                                  ReceiptCode::DEX_OPERATOR_UPDATED_FEE_TO_MINER);
    }
}

void dex::dex_operator_register(wasm_context &context) {

    WASM_ASSERT(context._receiver == dex_operator, wasm_assert_exception,
                "%s(), Except contract wasmio.dex, But get %s", __func__, wasm::name(context._receiver).to_string().c_str());

    operator_register_t args;
    args.unpack_from_bin(context.trx.data);

    context.require_auth(args.registrant()); //registrant auth

    WASM_ASSERT(args.name().size() <= NAME_SIZE_MAX, wasm_assert_exception, "name.size=%d is more than %d!",
        args.name().size(), NAME_SIZE_MAX);
    check_dex_operator_fee(context.database, DEX_OPERATOR_REGISTER_FEE, args.fee());
    WASM_ASSERT(check_url(args.portal_url()), wasm_assert_exception, "portal_url invalid");
    WASM_ASSERT(args.memo().size() <= MEMO_SIZE_MAX, wasm_assert_exception, "memo.size=%d is more than %d",
                args.memo().size(), MEMO_SIZE_MAX);

    nick_name registrant_name(wasm::name(args.registrant()).to_string());
    shared_ptr<CAccount> sp_registrant_account = wasm_account::get_account(context.database,
        registrant_name, ERROR_TITLE("registrant"));

    nick_name owner_name(wasm::name(args.owner()).to_string());
    WASM_ASSERT(!owner_name.IsEmpty(), wasm_assert_exception, "owner is empty!");
    shared_ptr<CAccount> sp_owner_account = make_shared<CAccount>();
    if (sp_registrant_account->IsMyUid(owner_name)) {
        sp_owner_account = sp_registrant_account;
    } else {
        sp_owner_account = wasm_account::get_account(context.database, registrant_name, ERROR_TITLE("owner"));
    }
    WASM_ASSERT(!context.database.dexCache.HaveDexOperatorByOwner(owner_name), wasm_assert_exception,
        "the owner already has a dex operator! owner=%s", owner_name.ToString());

    nick_name matcher_name(wasm::name(args.matcher()).to_string());
    WASM_ASSERT(!matcher_name.IsEmpty(), wasm_assert_exception, "matcher is empty!");
    if (!sp_registrant_account->IsMyUid(matcher_name) && !sp_owner_account->IsMyUid(matcher_name))
        wasm_account::check_account_existed(context.database, matcher_name, ERROR_TITLE("matcher"));

    vector<CReceipt> receipts; // TODO: receipts in wasm context
    process_dex_operator_fee(context, args.fee(), ASSET_ACTION_ISSUE, *sp_registrant_account);
    DexOperatorDetail detail(owner_name, matcher_name, args.name(), args.portal_url(), args.memo());
    DexID new_id;
    WASM_ASSERT(context.database.dexCache.IncDexID(new_id), wasm_assert_exception, "increase dex operator id error");
    WASM_ASSERT(context.database.dexCache.CreateDexOperator(new_id, detail), wasm_assert_exception, "save new dex operator error");
    context.require_recipient(args.registrant());
    context.require_recipient(args.owner());
    context.require_recipient(args.matcher());
}

void dex::dex_operator_update(wasm_context &context) {

    WASM_ASSERT(context._receiver == dex_operator, wasm_assert_exception,
                "%s(), Except contract wasmio.dex, But get %s", __func__, wasm::name(context._receiver).to_string().c_str());

    operator_update_t args;
    args.unpack_from_bin(context.trx.data);

    context.require_auth(args.registrant()); //registrant auth

    check_dex_operator_fee(context.database, DEX_OPERATOR_UPDATE_FEE, args.fee());

    nick_name registrant_name(wasm::name(args.registrant()).to_string());
    shared_ptr<CAccount> sp_registrant_account = wasm_account::get_account(context.database,
        registrant_name, ERROR_TITLE("registrant"));

    DexOperatorDetail operator_detail;
    WASM_ASSERT(context.database.dexCache.GetDexOperator(args.id(), operator_detail), wasm_assert_exception,
        "the dex operator does not exist! exid=%u", args.id());

    WASM_ASSERT(registrant_name == operator_detail.owner, wasm_assert_exception,
        "the registrant is not owner of dex operator! registrant=%u", registrant_name.ToString())

    DexOperatorDetail old_operator_detail = operator_detail;

    if (args.name() && operator_detail.name != args.name().value()) {
        WASM_ASSERT(args.name().value().size() <= NAME_SIZE_MAX, wasm_assert_exception, "name.size=%d is more than %d!",
            args.name().value().size(), NAME_SIZE_MAX);
        operator_detail.name = args.name().value();
    }
    if (args.portal_url() && operator_detail.portal_url != args.portal_url().value()) {
        WASM_ASSERT(check_url(args.portal_url().value()), wasm_assert_exception, "portal_url invalid");
        operator_detail.portal_url = args.portal_url().value();
    }
    if (args.memo() && operator_detail.memo != args.memo().value()) {
        WASM_ASSERT(args.memo().value().size() <= MEMO_SIZE_MAX, wasm_assert_exception, "memo.size=%d is more than %s",
                args.memo().value().size());
        operator_detail.memo = args.memo().value();
    }
    uint64_t old_owner = NAME(old_operator_detail.owner.ToString().c_str());
    shared_ptr<CAccount> sp_owner_account;
    if (args.owner() && args.owner().value() != old_owner) {
        nick_name owner_name(wasm::name(args.owner().value()).to_string());
        WASM_ASSERT(!owner_name.IsEmpty(), wasm_assert_exception, "owner is empty!");
        sp_owner_account = make_shared<CAccount>();
        if (sp_registrant_account->IsMyUid(owner_name)) {
            sp_owner_account = sp_registrant_account;
        } else {
            sp_owner_account = wasm_account::get_account(context.database, registrant_name, ERROR_TITLE("owner"));
        }
        WASM_ASSERT(!context.database.dexCache.HaveDexOperatorByOwner(owner_name), wasm_assert_exception,
            "the owner already has a dex operator! owner=%s", owner_name.ToString());
        operator_detail.owner = owner_name;
    }

    uint64_t old_matcher = NAME(old_operator_detail.matcher.ToString().c_str());
    if (args.matcher() && args.matcher().value() != old_matcher) {
        nick_name matcher_name(wasm::name(args.matcher().value()).to_string());
        if (operator_detail.matcher != matcher_name) {
            WASM_ASSERT(!matcher_name.IsEmpty(), wasm_assert_exception, "matcher is empty!");
            if (!sp_registrant_account->IsMyUid(matcher_name) &&
                (!sp_owner_account || !sp_owner_account->IsMyUid(matcher_name)))
                wasm_account::check_account_existed(context.database, matcher_name, ERROR_TITLE("matcher"));
            operator_detail.matcher = matcher_name;
        }
    }

    process_dex_operator_fee(context, args.fee(), ASSET_ACTION_UPDATE, *sp_registrant_account);
    WASM_ASSERT(context.database.dexCache.UpdateDexOperator(args.id(), old_operator_detail, operator_detail),
        wasm_assert_exception, "update dex operator error");
    context.require_recipient(args.registrant());
    context.require_recipient(old_owner);
    if (args.owner() && args.owner() != old_owner)
        context.require_recipient(args.owner().value());
    context.require_recipient(old_matcher);
    if (args.matcher() && args.matcher().value() != old_matcher)
        context.require_recipient(args.matcher().value());
}

void check_order_amount_range(const TokenSymbol &symbol, const int64_t amount, const string &err_title) {

    static_assert(MIN_DEX_ORDER_AMOUNT < INT64_MAX, "minimum dex order amount out of range");
    WASM_ASSERT(amount >= (int64_t)MIN_DEX_ORDER_AMOUNT, wasm_assert_exception,
                "%s, amount is too small, symbol=%s, amount=%llu, min_amount=%llu", err_title,
                symbol, amount, MIN_DEX_ORDER_AMOUNT);

    WASM_ASSERT(CheckCoinRange(symbol, amount), wasm_assert_exception,
                "%s, amount is out of range, symbol=%s, amount=%llu", err_title, symbol, amount);
}

// void check_order_symbol_pair(const TokenSymbol &asset_symbol, const TokenSymbol &coin_symbol) {
//     WASM_ASSERT(kTradingPairSet.count(make_pair(asset_symbol, coin_symbol)) != 0, wasm_assert_exception,
//         "unsupport trading pair! asset_symbol=%s, coin_symbol=%s", asset_symbol, coin_symbol);
// }

uint64_t calc_coin_amount(uint64_t asset_amount, const uint64_t price) {
    uint128_t coin_amount = asset_amount * (uint128_t)price / PRICE_BOOST;
    assert(coin_amount < ULLONG_MAX);
    WASM_ASSERT(coin_amount < ULLONG_MAX, wasm_assert_exception,
                "the calculated coin amount=%llu is out of range, asset_amount=%llu, price=%llu",
                (uint64_t)coin_amount, asset_amount, price);
    return (uint64_t)coin_amount;
}

void dex::dex_order_create(wasm_context &context) {

    WASM_ASSERT(context._receiver == dex_order, wasm_assert_exception,
                "%s(), Except contract dex.order, But get %s", __func__, wasm::name(context._receiver).to_string().c_str());

    dex::order_t args;
    args.unpack_from_bin(context.trx.data);

    context.require_auth(args.from());

    nick_name from_name(wasm::name(args.from()).to_string());
    shared_ptr<CAccount> sp_from_account = wasm_account::get_account(context.database,
        from_name, ERROR_TITLE("from"));

    WASM_ASSERT(sp_from_account->IsRegistered(), wasm_assert_exception, "from account must be registered! from=%s",
            from_name.ToString());
    // TODO: check account nonce

    WASM_ASSERT(CheckOrderType(OrderType(args.order_type())), wasm_assert_exception,
        "order_type=%d is invalid", args.order_type())
    WASM_ASSERT(kOrderSideHelper.CheckEnum(OrderSide(args.order_side())), wasm_assert_exception,
        "order_side=%d is invalid", args.order_side())

    DexOperatorDetail operator_detail;
    WASM_ASSERT(context.database.dexCache.GetDexOperator(args.exid(), operator_detail), wasm_assert_exception,
        "the dex operator does not exist! exid=%u", args.exid());

    WASM_ASSERT(args.fee_rate() <= DEX_ORDER_FEE_RATE_MAX, wasm_assert_exception,
        "fee_rate=%d is too large", args.fee_rate())

    TokenSymbol asset_sym = args.asset().sym.code().to_string();
    TokenSymbol coin_sym = args.coin().sym.code().to_string();
    // check_order_symbol_pair(asset_sym, coin_sym);

    OrderType order_type = OrderType(args.order_type());
    OrderSide order_side = OrderSide(args.order_side());
    uint64_t asset_amount = args.asset().amount;
    uint64_t coin_amount;
    if (order_type == ORDER_MARKET_PRICE && order_side == ORDER_BUY) {
        WASM_ASSERT(asset_amount == 0, wasm_assert_exception,
                    "asset.amount=%llu must be 0 when order_type=%s, order_side=%s",
                    asset_amount, kOrderTypeHelper.GetName(order_type),
                    kOrderSideHelper.GetName(order_side));
        coin_amount = args.coin().amount;
        check_order_amount_range(coin_sym, coin_amount, ERROR_TITLE("order.coin"));
    } else {
        check_order_amount_range(asset_sym, asset_amount, ERROR_TITLE("order.asset"));
        WASM_ASSERT(args.coin().amount == 0, wasm_assert_exception,
                    "coin.amount of sell order must be 0 when order_type=%s, order_side=%s",
                    args.coin().amount, kOrderTypeHelper.GetName(order_type),
                    kOrderSideHelper.GetName(order_side));
        coin_amount = calc_coin_amount(asset_amount, args.price());
    }

    if (order_side == ORDER_BUY) {
        WASM_ASSERT(sp_from_account->OperateBalance(coin_sym, FREEZE, coin_amount), wasm_assert_exception,
            "account has insufficient funds to freeze coin.amount! symbol=%s, amount=(%llu vs %llu)", coin_sym, coin_amount,
                sp_from_account->GetBalance(coin_sym, BalanceType::FREE_VALUE));
    } else {
        WASM_ASSERT(sp_from_account->OperateBalance(asset_sym, FREEZE, asset_amount), wasm_assert_exception,
        "account has insufficient funds to freeze asset.amount! symbol=%s, amount=(%llu vs %llu)", coin_sym,
            asset_amount, sp_from_account->GetBalance(coin_sym, BalanceType::FREE_VALUE));
    }

    WASM_ASSERT(args.price() > 0 && args.price() <= DEX_PRICE_MAX, wasm_assert_exception,
        "asset.price=%d is 0 or too large! max=%llu", args.price(), DEX_PRICE_MAX);

    WASM_ASSERT(args.memo().size() <= MEMO_SIZE_MAX, wasm_assert_exception, "memo.size=%d is more than %s",
            args.memo().size());

    uint256 order_id = context.control_trx.GetHash();

    auto sp_sys_order        = make_shared<CDEXOrderDetail>();
    sp_sys_order->generate_type = USER_GEN_ORDER;
    sp_sys_order->order_type    = OrderType(args.order_type());
    sp_sys_order->order_side    = OrderSide(args.order_side());
    sp_sys_order->coin_symbol   = args.coin().sym.code().to_string();
    sp_sys_order->asset_symbol  = args.asset().sym.code().to_string();
    sp_sys_order->coin_amount   = coin_amount;
    sp_sys_order->asset_amount  = asset_amount;
    sp_sys_order->price         = args.price();
    // TODO:...
//    sp_sys_order->tx_cord       = txCord;
    sp_sys_order->user_regid    = sp_from_account->regid;
    WASM_ASSERT(context.database.dexCache.CreateActiveOrder(order_id, *sp_sys_order),
        wasm_assert_exception, "save active order error");
    wasm_account::save(context.database, *sp_from_account, ERROR_TITLE("from"));
    context.require_recipient(args.from());
    uint64_t owner = NAME(operator_detail.owner.ToString().c_str());
    context.require_recipient(owner);
}

static uint256 checksum_to_uint256(checksum256_type &checksum) {
    static_assert(sizeof(checksum.hash) == uint256::WIDTH, "");
    uint256 ret;
    ret.SetReverse( &checksum.hash[0], &checksum.hash[sizeof(checksum.hash)] );
    return ret;
}

void dex::dex_order_cancel(wasm_context &context) {

    WASM_ASSERT(context._receiver == dex_order, wasm_assert_exception,
                "%s(), Except contract dex.order, But get %s", __func__, wasm::name(context._receiver).to_string().c_str());

    dex::order_cancel_t args;
    args.unpack_from_bin(context.trx.data);

    context.require_auth(args.from());

    nick_name from_name(wasm::name(args.from()).to_string());
    shared_ptr<CAccount> sp_from_account = wasm_account::get_account(context.database,
        from_name, ERROR_TITLE("from"));

    WASM_ASSERT(sp_from_account->IsRegistered(), wasm_assert_exception, "from account must be registered! from=%s",
            from_name.ToString());

    DexOperatorDetail operator_detail;
    WASM_ASSERT(context.database.dexCache.GetDexOperator(args.exid(), operator_detail), wasm_assert_exception,
        "the dex operator does not exist! exid=%u", args.exid());

    uint256 order_id = checksum_to_uint256(args.order_id());
    CDEXOrderDetail active_order;
    WASM_ASSERT(context.database.dexCache.GetActiveOrder(order_id, active_order), wasm_assert_exception,
        "the order is inactive or not existed! order_id=%s", order_id.ToString());

    WASM_ASSERT(active_order.generate_type == USER_GEN_ORDER, wasm_assert_exception,
                "the order is not generate by tx of user, order_id=%s", order_id.ToString());

    WASM_ASSERT(
        !sp_from_account->regid.IsEmpty() && sp_from_account->regid == active_order.user_regid,
        wasm_assert_exception, "can not cancel other user's order tx! order_id=%s, order_regid=%s",
        order_id.ToString(), active_order.user_regid.ToString());

    // get frozen money
    vector<CReceipt> receipts;
    TokenSymbol frozen_symbol;
    uint64_t frozen_amount = 0;
    if (active_order.order_side == ORDER_BUY) {
        frozen_symbol = active_order.coin_symbol;
        WASM_ASSERT(active_order.coin_amount >= active_order.total_deal_coin_amount,
                    wasm_assert_exception,
                    "invalid total_deal_coin_amount=%llu, order_coin_amount=%llu",
                    active_order.total_deal_coin_amount, active_order.coin_amount);
        frozen_amount = active_order.coin_amount - active_order.total_deal_coin_amount;

        receipts.emplace_back(CUserID::NULL_ID, active_order.user_regid, frozen_symbol, frozen_amount,
                              ReceiptCode::DEX_UNFREEZE_COIN_TO_BUYER);
    } else if(active_order.order_side == ORDER_SELL) {
        frozen_symbol = active_order.asset_symbol;
        frozen_amount = active_order.asset_amount - active_order.total_deal_asset_amount;

        receipts.emplace_back(CUserID::NULL_ID, active_order.user_regid, frozen_symbol, frozen_amount,
                              ReceiptCode::DEX_UNFREEZE_ASSET_TO_SELLER);
    } else {
        WASM_ASSERT(false,  wasm_assert_exception, "Order side must be ORDER_BUY|ORDER_SELL");
    }

    WASM_ASSERT(sp_from_account->OperateBalance(frozen_symbol, UNFREEZE, frozen_amount), wasm_assert_exception,
        "from account has insufficient frozen amount to unfreeze, from=%s", from_name.ToString());

    wasm_account::save(context.database, *sp_from_account, ERROR_TITLE("from"));
    WASM_ASSERT(context.database.dexCache.EraseActiveOrder(order_id, active_order), wasm_assert_exception,
        "erase active order failed! order_id=%s", order_id.ToString());
    context.require_recipient(args.from());
    uint64_t owner = NAME(operator_detail.owner.ToString().c_str());
    context.require_recipient(owner);
}
