/*
 * ScriptCheck.h
 *
 *  Created on: Sep 2, 2014
 *      Author: ranger.shi
 */

#ifndef SCRIPTCHECK_H_
#define SCRIPTCHECK_H_

#include <memory>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"
#include "main.h"
#include "script.h"
#include "serialize.h"
#include "txdb.h"
#include "vmlua.h"

using namespace std;
class CVmOperate;
class CVmRunEvn {
    /**
     * Run the script object
     */

    shared_ptr<CVmlua> pLua;  //执行lua脚本
    /**
     * vm before the account state
     */
    vector<shared_ptr<CAccount>> RawAccont;
    /**
     * vm operate the account  state
     */
    vector<shared_ptr<CAccount>> NewAccont;
    /**
     * current run the tx
     */
    shared_ptr<CBaseTransaction> listTx;
    /**
     * run the script
     */
    CVmScript vmScript;
    /**
     * the block height
     */
    unsigned int RunTimeHeight;
    /**
     * vm before the app account state
     */
    vector<shared_ptr<CAppUserAccount>> RawAppUserAccout;
    /**
     * vm operate the app account  state
     */
    vector<shared_ptr<CAppUserAccount>> NewAppUserAccout;
    CScriptDBViewCache* m_ScriptDBTip;
    CAccountViewCache* m_view;
    vector<CVmOperate> m_output;  //保存操作结果
    bool isCheckAccount;          //校验账户平衡开关

    map<vector<unsigned char>, vector<CAppFundOperate>> MapAppOperate;  //vector<unsigned char > 存的是accountId
    shared_ptr<vector<CScriptDBOperLog>> m_dblog;

   private:
    /**
     * @brief The initialization function
     * @param Tx: run the tx's contact
     * @param view: Cache holds account
     *  @param nheight: run the Environment the block's height
     * @return : check the the tx and account is Legal true is legal false is unlegal
     */
    bool intial(shared_ptr<CBaseTransaction>& Tx, CAccountViewCache& view, int nheight);
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
    bool OpeatorAccount(const vector<CVmOperate>& listoperate, CAccountViewCache& view, const int nCurHeight);
    /**
     * @brief find the vOldAccount from NewAccont if find success remove it from NewAccont
     * @param vOldAccount: the argument
     * @return:Return the object
     */
    std::shared_ptr<CAccount> GetNewAccount(shared_ptr<CAccount>& vOldAccount);
    /**
     * @brief find the Account from NewAccont
     * @param Account: argument
     * @return:Return the object
     */
    std::shared_ptr<CAccount> GetAccount(shared_ptr<CAccount>& Account);
    /**
     * @brief get the account id
     * @param value: argument
     * @return:Return account id
     */
    vector_unsigned_char GetAccountID(CVmOperate value);
    //  bool IsSignatureAccount(CRegID account);
    bool OpeatorAppAccount(const map<vector<unsigned char>, vector<CAppFundOperate>> opMap, CScriptDBViewCache& view);

    std::shared_ptr<CAppUserAccount> GetAppAccount(shared_ptr<CAppUserAccount>& AppAccount);

   public:
    /**
     * A constructor.
     */
    CVmRunEvn();
    /**
     *@brief get be operate the account
     * @return the variable RawAccont
     */
    vector<shared_ptr<CAccount>>& GetRawAccont();
    /**
     *@brief get after operate the account
     * @return :the variable NewAccont
     */
    vector<shared_ptr<CAccount>>& GetNewAccont();
    vector<shared_ptr<CAppUserAccount>>& GetRawAppUserAccount();

    vector<shared_ptr<CAppUserAccount>>& GetNewAppUserAccount();
    /**
     * @brief  start to run the script
     * @param Tx: run the tx
     * @param view: the second argument
     * @param nheight: block height
     * @param nBurnFactor: Executing a step script to spending
     * @return: tuple<bool,uint64_t,string>  bool represent the script run success
     * uint64_t if the script run sucess Run the script calls the money ,string represent run the failed's  Reason
     */
    tuple<bool, uint64_t, string> run(shared_ptr<CBaseTransaction>& Tx, CAccountViewCache& view, CScriptDBViewCache& VmDB,
                                      int nheight, uint64_t nBurnFactor, uint64_t& uRunStep);
    /**
     * @brief just for test
     * @return:
     */
    //  shared_ptr<vector<CVmOperate> > GetOperate() const;
    const CRegID& GetScriptRegID();
    const CRegID& GetTxAccount();
    uint64_t GetValue() const;
    const vector<unsigned char>& GetTxContact();
    CScriptDBViewCache* GetScriptDB();
    CAccountViewCache* GetCatchView();
    int GetComfirHeight();
    uint256 GetCurTxHash();
    bool InsertOutputData(const vector<CVmOperate>& source);
    void InsertOutAPPOperte(const vector<unsigned char>& userId, const CAppFundOperate& source);
    shared_ptr<vector<CScriptDBOperLog>> GetDbLog();

    bool GetAppUserAccout(const vector<unsigned char>& id, shared_ptr<CAppUserAccount>& sptrAcc);
    bool CheckAppAcctOperate(CTransaction* tx);
    void SetCheckAccount(bool bCheckAccount);
    virtual ~CVmRunEvn();
};

enum ACCOUNT_TYPE {
    // account type
    regid      = 0x01,  //!< Registration accountid
    base58addr = 0x02,  //!< pulickey
};
/**
 * @brief after run the script,the script output the code
 */
class CVmOperate {
   public:
    unsigned char nacctype;       //regid or base58addr
    unsigned char accountid[34];  //!< accountid
    unsigned char opeatortype;    //!OperType
    unsigned int outheight;       //!< the transacion Timeout height
    unsigned char money[8];       //!<The transfer amount
    IMPLEMENT_SERIALIZE(
        READWRITE(nacctype);
        for (int i = 0; i < 34; i++)
            READWRITE(accountid[i]);
        READWRITE(opeatortype);
        READWRITE(outheight);
        for (int i = 0; i < 8; i++)
            READWRITE(money[i]);)
    CVmOperate() {
        nacctype = regid;
        memset(accountid, 0, 34);
        opeatortype = ADD_FREE;
        outheight   = 0;
        memset(money, 0, 8);
    }
    Object ToJson();
};

//extern CVmRunEvn *pVmRunEvn; //提供给lmylib.cpp库使用
#endif /* SCRIPTCHECK_H_ */
