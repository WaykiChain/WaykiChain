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

// TODO: move to const header
static const uint32_t NAME_SIZE_MAX = 32;
static const uint32_t MEMO_SIZE_MAX = 256;

static const uint8_t WICC_PRECISION = 8;

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

    shared_ptr<CAccount> sp_risk_riserve_account = make_shared<CAccount>();
    WASM_ASSERT(context.database.accountCache.GetFcoinGenesisAccount(*sp_risk_riserve_account), wasm_assert_exception,
        "%s(), risk riserve account does not existed", __func__);

    wasm_account::add_free_balance(*sp_risk_riserve_account, symbol, risk_fee, ERROR_TITLE("risk fee"));
    wasm_account::save(context.database, *sp_risk_riserve_account, ERROR_TITLE("risk riserve"));

    if (action == ASSET_ACTION_ISSUE)
        context.receipts.emplace_back(registrant_account.nickid, sp_risk_riserve_account->regid, symbol,
                              risk_fee, ReceiptCode::DEX_OPERATOR_REG_FEE_TO_RISERVE);
    else
        context.receipts.emplace_back(registrant_account.nickid, sp_risk_riserve_account->regid, symbol,
            risk_fee, ReceiptCode::DEX_OPERATOR_UPDATED_FEE_TO_RISERVE);

    VoteDelegateVector delegates;
    WASM_ASSERT(context.database.delegateCache.GetActiveDelegates(delegates), wasm_assert_exception,
        "%s(), GetActiveDelegates failed", __func__);
    assert(delegates.size() != 0 && delegates.size() == IniCfg().GetTotalDelegateNum());

    for (size_t i = 0; i < delegates.size(); i++) {
        const CRegID &delegate_regid = delegates[i].regid;
        shared_ptr<CAccount> sp_delegate_account = wasm_account::get_account(cw, delegate_regid, ERROR_TITLE("delegate"));

        uint64_t delegate_fee = total_delegate_fee / delegates.size();
        if (i == 0) delegate_fee += total_delegate_fee % delegates.size(); // give the dust amount to topmost miner

        wasm_account::add_free_balance(*sp_delegate_account, symbol, delegate_fee, ERROR_TITLE("delegate fee"));
        wasm_account::save(cw, *sp_delegate_account, ERROR_TITLE("delegate"));

        if (action == ASSET_ACTION_ISSUE)
            context.receipts.emplace_back(registrant_account.nickid, delegate_regid, symbol, delegate_fee,
                                  ReceiptCode::DEX_OPERATOR_REG_FEE_TO_MINER);
        else
            context.receipts.emplace_back(registrant_account.nickid, delegate_regid, symbol, delegate_fee,
                                  ReceiptCode::DEX_OPERATOR_UPDATED_FEE_TO_MINER);
    }
}

void dex::dex_operator_register(wasm_context &context) {

    WASM_ASSERT(context._receiver == wasmio_dex, wasm_assert_exception,
                "%s(), Except contract wasmio.dex, But get %s", __func__, wasm::name(context._receiver).to_string().c_str());

    auto params = wasm::unpack<std::tuple<uint64_t, wasm::asset, uint64_t, uint64_t, string, string, string>>(context.trx.data);

    auto registrant = std::get<0>(params);
    auto fee        = std::get<1>(params);
    auto owner      = std::get<2>(params);
    auto matcher    = std::get<3>(params);
    auto name       = std::get<4>(params);
    auto portal_url = std::get<5>(params);
    auto memo       = std::get<6>(params);

    context.require_auth(registrant); //registrant auth

    WASM_ASSERT(name.size() > NAME_SIZE_MAX, wasm_assert_exception, "name.size=%d is more than %d!",
        name.size(), NAME_SIZE_MAX);
    check_dex_operator_fee(context.database, DEX_OPERATOR_REGISTER_FEE, fee);
    WASM_ASSERT(check_url(portal_url), wasm_assert_exception, "portal_url invalid");
    WASM_ASSERT(memo.size() <= MEMO_SIZE_MAX, wasm_assert_exception, "memo.size=%d is more than %d",
                memo.size(), MEMO_SIZE_MAX);

    nick_name registrant_name(wasm::name(registrant).to_string());
    shared_ptr<CAccount> sp_registrant_account = wasm_account::get_account(context.database,
        registrant_name, ERROR_TITLE("registrant"));

    nick_name owner_name(wasm::name(owner).to_string());
    WASM_ASSERT(!owner_name.IsEmpty(), wasm_assert_exception, "owner is empty!");
    shared_ptr<CAccount> sp_owner_account = make_shared<CAccount>();
    if (sp_registrant_account->IsMyUid(owner_name)) {
        sp_owner_account = sp_registrant_account;
    } else {
        sp_owner_account = wasm_account::get_account(context.database, registrant_name, ERROR_TITLE("owner"));
    }
    WASM_ASSERT(!context.database.dexCache.HaveDexOperatorByOwner(owner_name), wasm_assert_exception,
        "the owner already has a dex operator! owner=%s", owner_name.ToString());

    nick_name matcher_name(wasm::name(matcher).to_string());
    WASM_ASSERT(!matcher_name.IsEmpty(), wasm_assert_exception, "matcher is empty!");
    if (!sp_registrant_account->IsMyUid(matcher_name) && !sp_owner_account->IsMyUid(matcher_name))
        wasm_account::check_account_existed(context.database, matcher_name, ERROR_TITLE("matcher"));

    vector<CReceipt> receipts; // TODO: receipts in wasm context
    process_dex_operator_fee(context, fee, ASSET_ACTION_ISSUE, *sp_registrant_account);
    DexOperatorDetail detail(owner_name, name, matcher_name, portal_url, memo);
    DexOperatorID new_id;
    WASM_ASSERT(context.database.dexCache.IncDexOperatorId(new_id), wasm_assert_exception, "increase dex operator id error");
    WASM_ASSERT(context.database.dexCache.CreateDexOperator(new_id, detail), wasm_assert_exception, "save new dex operator error");
}

void dex::dex_operator_update(wasm_context &context) {

    WASM_ASSERT(context._receiver == wasmio_dex, wasm_assert_exception,
                "%s(), Except contract wasmio.dex, But get %s", __func__, wasm::name(context._receiver).to_string().c_str());

    auto params = wasm::unpack<std::tuple<uint64_t, wasm::asset, uint64_t, wasm::optional<uint64_t>, wasm::optional<string>,
        wasm::optional<uint64_t>, wasm::optional<string>, wasm::optional<string>>>(context.trx.data);

    auto registrant      = std::get<0>(params);
    auto fee             = std::get<1>(params);
    auto dex_operator_id = std::get<2>(params);
    auto owner           = std::get<3>(params);
    auto name            = std::get<4>(params);
    auto matcher         = std::get<5>(params);
    auto portal_url      = std::get<6>(params);
    auto memo            = std::get<7>(params);

    context.require_auth(registrant); //registrant auth

    check_dex_operator_fee(context.database, DEX_OPERATOR_UPDATE_FEE, fee);

    nick_name registrant_name(wasm::name(registrant).to_string());
    shared_ptr<CAccount> sp_registrant_account = wasm_account::get_account(context.database,
        registrant_name, ERROR_TITLE("registrant"));

    DexOperatorDetail dexOperator;
    WASM_ASSERT(!context.database.dexCache.GetDexOperator(dex_operator_id, dexOperator), wasm_assert_exception,
        "the dex operator does not exist! owner=%ul", dex_operator_id);

    WASM_ASSERT(registrant_name == dexOperator.owner, wasm_assert_exception,
        "the registrant is not owner of dex operator ! registrant=%ul", registrant_name.ToString())

    DexOperatorDetail oldDexOperator = dexOperator;

    if (name && dexOperator.name != name.value()) {
        WASM_ASSERT(name.value().size() > NAME_SIZE_MAX, wasm_assert_exception, "name.size=%d is more than %d!",
            name.value().size(), NAME_SIZE_MAX);
        dexOperator.name = name.value();
    }
    if (portal_url && dexOperator.portal_url != portal_url.value()) {
        WASM_ASSERT(check_url(portal_url.value()), wasm_assert_exception, "portal_url invalid");
        dexOperator.portal_url = portal_url.value();
    }
    if (memo && dexOperator.memo != memo.value()) {
        WASM_ASSERT(memo.value().size() <= MEMO_SIZE_MAX, wasm_assert_exception, "memo.size=%d is more than %s",
                memo.value().size());
        dexOperator.memo = memo.value();
    }
    shared_ptr<CAccount> sp_owner_account;
    if (owner) {
        nick_name owner_name(wasm::name(owner.value()).to_string());
        if (dexOperator.owner != owner_name) {
            WASM_ASSERT(!owner_name.IsEmpty(), wasm_assert_exception, "owner is empty!");
            sp_owner_account = make_shared<CAccount>();
            if (sp_registrant_account->IsMyUid(owner_name)) {
                sp_owner_account = sp_registrant_account;
            } else {
                sp_owner_account = wasm_account::get_account(context.database, registrant_name, ERROR_TITLE("owner"));
            }
            WASM_ASSERT(!context.database.dexCache.HaveDexOperatorByOwner(owner_name), wasm_assert_exception,
                "the owner already has a dex operator! owner=%s", owner_name.ToString());
            dexOperator.owner = owner_name;
        }
    }

    if (matcher) {
        nick_name matcher_name(wasm::name(matcher.value()).to_string());
        if (dexOperator.matcher != matcher_name) {
            WASM_ASSERT(!matcher_name.IsEmpty(), wasm_assert_exception, "matcher is empty!");
            if (!sp_registrant_account->IsMyUid(matcher_name) &&
                (!sp_owner_account || !sp_owner_account->IsMyUid(matcher_name)))
                wasm_account::check_account_existed(context.database, matcher_name, ERROR_TITLE("matcher"));
            dexOperator.matcher = matcher_name;
        }
    }

    process_dex_operator_fee(context, fee, ASSET_ACTION_UPDATE, *sp_registrant_account);
    WASM_ASSERT(context.database.dexCache.UpdateDexOperator(dex_operator_id, oldDexOperator, dexOperator),
        wasm_assert_exception, "update dex operator error");
}
