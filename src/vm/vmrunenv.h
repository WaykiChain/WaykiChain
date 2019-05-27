// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2019- The WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef VMRUNENV_H_
#define VMRUNENV_H_

#include "vmlua.h"
#include "appaccount.h"
#include "commons/serialize.h"
#include "script.h"
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
	 * run the script
	 */
	CVmScript vmScript;
	/**
	 * the block height
	 */
	unsigned int runTimeHeight;
	/**
	 * vm before the app account state
	 */
	vector<std::shared_ptr<CAppUserAccount>> rawAppUserAccount;
	/**
	 * vm operate the app account state
	 */
	vector<std::shared_ptr<CAppUserAccount>> newAppUserAccount;

	CContractCache *pContractCache;
	CAccountCache *pAccountCache;

	vector<CVmOperate> vmOperateOutput;   //保存操作结果
    bool  isCheckAccount;  //校验账户平衡开关

    map<vector<unsigned char>, vector<CAppFundOperate>> mapAppFundOperate;  // vector<unsigned char > 存的是accountId
    std::shared_ptr<vector<CContractDBOperLog>> pScriptDBOperLog;

private:
    /**
     * @brief The initialization function
     * @param Tx: run the tx's contact
     * @param view: Cache holds account
     *  @param nheight: run the Environment the block's height
     * @return : check the the tx and account is Legal true is legal false is unlegal
     */
    bool Initialize(std::shared_ptr<CBaseTx>& tx, CAccountCache& view, int nHeight);
    /**
     *@brief check aciton
     * @param listoperate: run the script return the code,check the code
     * @return : true check success
     */
    bool CheckOperate(const vector<CVmOperate>& listoperate);
    /**
     *
     * @param listoperate: through the vm return code ,The accounts plus money and less money
     * @param view:
     * @return true operate account success
     */
    bool OpeatorAccount(const vector<CVmOperate>& listoperate, CAccountCache& view,
                        const int nCurHeight);
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
    vector_unsigned_char GetAccountID(CVmOperate value);
    //	bool IsSignatureAccount(CRegID account);
    bool OpeatorAppAccount(const map<vector<unsigned char>, vector<CAppFundOperate>> opMap,
                           CContractCache& view);

    std::shared_ptr<CAppUserAccount> GetAppAccount(std::shared_ptr<CAppUserAccount>& AppAccount);

public:
    /**
     * A constructor.
     */
    CVmRunEnv();
    /**
     *@brief get be operate the account
     * @return the variable rawAccount
     */
    vector<std::shared_ptr<CAccount>>& GetRawAccont();
    /**
     *@brief get after operate the account
     * @return :the variable newAccount
     */
    vector<std::shared_ptr<CAccount>>& GetNewAccount();
    vector<std::shared_ptr<CAppUserAccount>>& GetRawAppUserAccount();
    vector<std::shared_ptr<CAppUserAccount>>& GetNewAppUserAccount();

    /**
     * @brief  start to run the script
     * @param Tx: run the tx
     * @param view: the second argument
     * @param nheight: block height
     * @param nBurnFactor: Executing a step script to spending
     * @return: tuple<bool,uint64_t,string>  bool represent the script run success
     * uint64_t if the script run sucess Run the script calls the money ,string represent run the
     * failed's  Reason
     */
    std::tuple<bool, uint64_t, string> ExecuteContract(std::shared_ptr<CBaseTx>& Tx, int nheight, CCacheWrapper &cw,
                                                  uint64_t nBurnFactor, uint64_t& uRunStep);
    /**
     * @brief just for test
     * @return:
     */
    //	shared_ptr<vector<CVmOperate> > GetOperate() const;
    const CRegID& GetScriptRegID();
    const CRegID& GetTxAccount();
    uint64_t GetValue() const;
    const vector<unsigned char>& GetTxContract();
    CContractCache* GetScriptDB();
    CAccountCache* GetCatchView();
    int GetConfirmHeight();
    // Get burn version for fuel burning
    int GetBurnVersion();
    uint256 GetCurTxHash();
    bool InsertOutputData(const vector<CVmOperate>& source);
    void InsertOutAPPOperte(const vector<unsigned char>& userId, const CAppFundOperate& source);
    std::shared_ptr<vector<CContractDBOperLog>> GetDbLog();

    bool GetAppUserAccount(const vector<unsigned char>& id, std::shared_ptr<CAppUserAccount>& sptrAcc);
    bool CheckAppAcctOperate(CContractInvokeTx* tx);
    void SetCheckAccount(bool bCheckAccount);
    virtual ~CVmRunEnv();
};

enum ACCOUNT_TYPE {
    // account type
    regid      = 0x01,  //!< Registration accountId
    base58addr = 0x02,  //!< pulickey
};
/**
 * @brief after run the script,the script output the code
 */
class CVmOperate{
public:
	unsigned char accountType;      //regid or base58addr
	unsigned char accountId[34];	//!< accountId: address
	unsigned char opType;		    //!OperType
	unsigned int  timeoutHeight;    //!< the transacion Timeout height
	unsigned char money[8];			//!<The transfer amount

	IMPLEMENT_SERIALIZE
	(
		READWRITE(accountType);
		for (int i = 0;i < 34;i++)
			READWRITE(accountId[i]);
		READWRITE(opType);
		READWRITE(timeoutHeight);
		for (int i = 0;i < 8;i++)
			READWRITE(money[i]);
	)

	CVmOperate() {
		accountType = regid;
		memset(accountId, 0, 34);
		opType = NULL_OP;
		timeoutHeight = 0;
		memset(money, 0, 8);
	}

	Object ToJson();

};

#endif /* VMRUNENV_H_ */
