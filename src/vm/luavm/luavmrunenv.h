// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LUA_VM_RUNENV_H
#define LUA_VM_RUNENV_H

#include "luavm.h"
#include "appaccount.h"
#include "commons/serialize.h"
#include "entities/account.h"
#include "entities/receipt.h"
#include "persistence/leveldbwrapper.h"
#include "persistence/cachewrapper.h"

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"

#include <memory>

using namespace std;
class CVmOperate;
struct lua_State;

class CLuaVMContext {
public:
    CCacheWrapper *p_cw;
    uint32_t height;
	CBaseTx *p_base_tx;
    uint64_t fuel_limit;
    TokenSymbol transfer_symbol; // amount of tx user transfer to contract account
    uint64_t transfer_amount; // amount of tx user transfer to contract account
    CAccount *p_tx_user_account;
    CAccount *p_app_account;
    CUniversalContract *p_contract;
    string *p_arguments;
};

struct AssetTransfer {
    bool isContractAccount; // Is contract account or tx sender' account
    CUserID  toUid;         // to address of the transfer
    TokenSymbol tokenType;  // Token type of the transfer
    uint64_t  tokenAmount;  // Token ammount of the transfer
};

class CLuaVMRunEnv {
private:
    CLuaVMContext *p_context;
	/**
	 * Run the script object
	 */
	std::shared_ptr<CLuaVM> pLua;
	/**
	 * vm before the app account state
	 */
	vector<std::shared_ptr<CAppUserAccount>> rawAppUserAccount;
	/**
	 * vm operate the app account state
	 */
	vector<std::shared_ptr<CAppUserAccount>> newAppUserAccount;

    vector<CReceipt> receipts;

	vector<CVmOperate> vmOperateOutput;   //保存操作结果
    bool isCheckAccount;  //校验账户平衡开关

    map<vector<uint8_t>, vector<CAppFundOperate>> mapAppFundOperate;  // vector<unsigned char > 存的是accountId
private:
    bool Init();
    /**
     * @brief check action
     * @param operates: run the script return the code,check the code
     * @return : true check success
     */
    bool CheckOperate(const vector<CVmOperate>& operates);
    /**
     *
     * @param operates: through the vm return code, The accounts add/minus money
     * @param accountCache:
     * @param contractCache
     * @return true operate account success
     */
    bool OperateAccount(const vector<CVmOperate>& operates);

    /**
     * @brief get the account id
     * @param value: argument
     * @return:Return account id
     */
    UnsignedCharArray GetAccountID(const CVmOperate& value);

    bool OperateAppAccount(const map<vector<uint8_t>, vector<CAppFundOperate>> opMap);

    std::shared_ptr<CAppUserAccount> GetAppAccount(std::shared_ptr<CAppUserAccount>& appAccount);

public:
    /**
     * A constructor.
     */
    CLuaVMRunEnv();
    virtual ~CLuaVMRunEnv();

    vector<std::shared_ptr<CAppUserAccount>>& GetRawAppUserAccount();
    vector<std::shared_ptr<CAppUserAccount>>& GetNewAppUserAccount();

    vector<CReceipt> GetReceipts() const { return receipts; }

    /**
     * execute contract
     * @param pContextIn: run context
     * @param fuel: burned fuel amount
     * @return: nullptr if run success, else error string
     */
    std::shared_ptr<string> ExecuteContract(CLuaVMContext *pContextIn, uint64_t& uRunStep);

    /**
     * @brief just for test
     * @return:
     */
    //	shared_ptr<vector<CVmOperate> > GetOperate() const;
    const CRegID& GetContractRegID();
    const CRegID& GetTxUserRegid();
    uint64_t GetValue() const;
    const string& GetTxContract();
    CCacheWrapper* GetCw();
    CContractDBCache* GetScriptDB();
    CAccountDBCache* GetCatchView();
    int32_t GetConfirmHeight();
    // Get burn version for fuel burning
    int32_t GetBurnVersion();
    uint256 GetCurTxHash();
    bool InsertOutputData(const vector<CVmOperate>& source);
    /**
     * transfer account asset
     * @param transfers: transfer info vector
     * @return transfer success or not
     */
    bool TransferAccountAsset(lua_State *L, const vector<AssetTransfer> &transfers);
    void InsertOutAPPOperte(const vector<uint8_t>& userId, const CAppFundOperate& source);

    bool GetAppUserAccount(const vector<uint8_t>& id, std::shared_ptr<CAppUserAccount>& pAppUserAccount);
    bool CheckAppAcctOperate();
    void SetCheckAccount(bool bCheckAccount);
};

#endif  // LUA_VM_RUNENV_H
