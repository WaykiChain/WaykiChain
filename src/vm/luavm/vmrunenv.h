// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef VMRUNENV_H
#define VMRUNENV_H

#include "vmlua.h"
#include "appaccount.h"
#include "commons/serialize.h"
#include "entities/account.h"
#include "persistence/leveldbwrapper.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"

#include <memory>

using namespace std;
class CVmOperate;

class CVmRunEnv {
private:
	/**
	 * Run the script object
	 */
	std::shared_ptr<CVmlua> pLua;
	/**
	 * vm before the account state
	 */
	vector<std::shared_ptr<CAccount> > rawAccount;
	/**
	 * vm operate the account state
	 */
	vector<std::shared_ptr<CAccount> > newAccount;
	/**
	 * current run the tx
	 */
	std::shared_ptr<CBaseTx> pBaseTx;
	/**
	 * the block height
	 */
	uint32_t runtimeHeight;
	/**
	 * vm before the app account state
	 */
	vector<std::shared_ptr<CAppUserAccount>> rawAppUserAccount;
	/**
	 * vm operate the app account state
	 */
	vector<std::shared_ptr<CAppUserAccount>> newAppUserAccount;

	CContractDBCache *pContractCache;
	CAccountDBCache *pAccountCache;

	vector<CVmOperate> vmOperateOutput;   //保存操作结果
    bool isCheckAccount;  //校验账户平衡开关

    map<vector<unsigned char>, vector<CAppFundOperate>> mapAppFundOperate;  // vector<unsigned char > 存的是accountId
private:
    /**
     * @brief The initialization function
     * @param Tx: run the tx's contact
     * @param accountView: Cache holds account
     * @param nheight: run the Environment the block's height
     * @return : check the the tx and account is Legal true is legal false is unlegal
     */
    bool Initialize(std::shared_ptr<CBaseTx>& tx, CAccountDBCache& accountView, int32_t height);
    /**
     * @brief check aciton
     * @param listoperate: run the script return the code,check the code
     * @return : true check success
     */
    bool CheckOperate(const vector<CVmOperate>& listoperate);
    /**
     *
     * @param listoperate: through the vm return code ,The accounts plus money and less money
     * @param accountView:
     * @return true operate account success
     */
    bool OperateAccount(const vector<CVmOperate>& listoperate, CAccountDBCache& accountView,
                        const int32_t nCurHeight);
    /**
     * @brief find the vOldAccount from newAccount if find success remove it from newAccount
     * @param vOldAccount: the argument
     * @return:Return the object
     */
    std::shared_ptr<CAccount> GetNewAccount(std::shared_ptr<CAccount>& vOldAccount);
    /**
     * @brief find the Account from newAccount
     * @param Account: argument
     * @return:Return the object
     */
    std::shared_ptr<CAccount> GetAccount(std::shared_ptr<CAccount>& Account);
    /**
     * @brief get the account id
     * @param value: argument
     * @return:Return account id
     */
    UnsignedCharArray GetAccountID(CVmOperate value);
    //	bool IsSignatureAccount(CRegID account);
    bool OperateAppAccount(const map<vector<unsigned char>, vector<CAppFundOperate>> opMap,
                           CContractDBCache& accountView);

    std::shared_ptr<CAppUserAccount> GetAppAccount(std::shared_ptr<CAppUserAccount>& AppAccount);

public:
    /**
     * A constructor.
     */
    CVmRunEnv();
    /**
     * @brief get be operate the account
     * @return the variable rawAccount
     */
    vector<std::shared_ptr<CAccount>>& GetRawAccont();
    /**
     * @brief get after operate the account
     * @return :the variable newAccount
     */
    vector<std::shared_ptr<CAccount>>& GetNewAccount();
    vector<std::shared_ptr<CAppUserAccount>>& GetRawAppUserAccount();
    vector<std::shared_ptr<CAppUserAccount>>& GetNewAppUserAccount();

    /**
     * @brief  start to run the script
     * @param Tx: run the tx
     * @param accountView: the second argument
     * @param nheight: block height
     * @param nBurnFactor: Executing a step script to spending
     * @return: tuple<bool,uint64_t,string>  bool represent the script run success
     * uint64_t if the script run sucess Run the script calls the money ,string represent run the
     * failed's  Reason
     */
    std::tuple<bool, uint64_t, string> ExecuteContract(std::shared_ptr<CBaseTx>& Tx, int32_t nheight, CCacheWrapper &cw,
                                                  uint64_t nBurnFactor, uint64_t& uRunStep);

    /**
     * @brief just for test
     * @return:
     */
    //	shared_ptr<vector<CVmOperate> > GetOperate() const;
    const CRegID& GetScriptRegID();
    const CRegID& GetTxAccount();
    uint64_t GetValue() const;
    const string& GetTxContract();
    CContractDBCache* GetScriptDB();
    CAccountDBCache* GetCatchView();
    int32_t GetConfirmHeight();
    // Get burn version for fuel burning
    int32_t GetBurnVersion();
    uint256 GetCurTxHash();
    bool InsertOutputData(const vector<CVmOperate>& source);
    void InsertOutAPPOperte(const vector<unsigned char>& userId, const CAppFundOperate& source);

    bool GetAppUserAccount(const vector<unsigned char>& id, std::shared_ptr<CAppUserAccount>& sptrAcc);
    bool CheckAppAcctOperate(CLuaContractInvokeTx* tx);
    void SetCheckAccount(bool bCheckAccount);
    virtual ~CVmRunEnv();
};

#endif  // VMRUNENV_H
