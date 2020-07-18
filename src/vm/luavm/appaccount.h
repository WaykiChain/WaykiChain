// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef VM_APP_ACCOUT_H
#define VM_APP_ACCOUT_H

#include "commons/serialize.h"
#include "commons/uint256.h"
#include "entities/receipt.h"

#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"

#include <vector>

using namespace std;

class CAppFundOperate;

class CAppCFund {
public:
    static const int32_t MAX_TAG_SIZE = 40;

    CAppCFund();
    CAppCFund(const CAppFundOperate &operate);
    CAppCFund(const vector<uint8_t> &tag, uint64_t val, int32_t height);

    bool MergeCFund(const CAppCFund &fund);
    json_spirit::Object ToJson() const;
    string ToString() const;

    void SetHeight(int32_t height) { timeoutHeight = height; }
    int32_t GetHeight() const { return timeoutHeight; }

    void SetValue(uint64_t value) { this->value = value; }
    uint64_t GetValue() const { return value; }

    void SetTag(const vector<uint8_t> &tag) { vTag = tag; }
    const vector<uint8_t> GetTag() const { return vTag; }

public:
    IMPLEMENT_SERIALIZE(
		READWRITE(VARINT(value));
		READWRITE(VARINT(timeoutHeight));
		READWRITE(vTag);
	)

private:
    uint64_t value;         //!< amount of money
    int32_t timeoutHeight;  //!< time-out height
    vector<uint8_t> vTag;   //!< vTag of the tx which create the fund
};

enum APP_OP_TYPE {
    ADD_FREE_OP = 1,
    SUB_FREE_OP,
    ADD_TAG_OP,
    SUB_TAG_OP
} __attribute__((aligned(1)));

class CAppFundOperate {
public:
    CAppFundOperate();

    uint8_t opType = 0;          //!< OperType
    uint32_t timeoutHeight = 0;  //!< the transacion Timeout height
    int64_t mMoney = 0;          //!< The transfer amount
    uint8_t appuserIDlen = 0;
    uint8_t vAppuser[CAppCFund::MAX_TAG_SIZE] = {0};  //!< accountId
    uint8_t fundTagLen = 0;
    uint8_t vFundTag[CAppCFund::MAX_TAG_SIZE] = {0};  //!< accountId

    CAppFundOperate(const vector<uint8_t> &appUserIn, const vector<uint8_t> &fundTagIn, const APP_OP_TYPE opTypeIn,
                    const int32_t timeoutHeightIn, const int64_t moneyIn) {
        assert(sizeof(vAppuser) >= appUserIn.size());
        assert(sizeof(vFundTag) >= fundTagIn.size());
        assert((opTypeIn >= ADD_FREE_OP) && (opTypeIn <= SUB_TAG_OP));

        appuserIDlen = appUserIn.size();
        fundTagLen   = fundTagIn.size();
        memcpy(&vAppuser[0], &appUserIn[0], appUserIn.size());
        memcpy(&vFundTag[0], &fundTagIn[0], fundTagIn.size());
        mMoney        = moneyIn;
        timeoutHeight = timeoutHeightIn;
        opType        = opTypeIn;
    }

    IMPLEMENT_SERIALIZE(
		READWRITE(opType);
		READWRITE(timeoutHeight);
		READWRITE(mMoney);
		READWRITE(appuserIDlen);
        for (uint32_t i = 0; i < sizeof(vAppuser); ++i)
			READWRITE(vAppuser[i]);
        READWRITE(fundTagLen);
        for (uint32_t i = 0; i < sizeof(vFundTag); ++i)
			READWRITE(vFundTag[i]);
		)

    json_spirit::Object ToJson() const;
    string ToString() const;

    uint64_t GetUint64Value() const { return mMoney; }

    vector<uint8_t> GetFundTagV() const {
        assert(sizeof(vFundTag) >= fundTagLen);
        return vector<uint8_t>(&vFundTag[0], &vFundTag[0] + fundTagLen);
    }

    string GetFundTag() const {
        return string(&vFundTag[0], &vFundTag[0] + fundTagLen);
    }

    vector<uint8_t> GetAppUserV() const {
        assert(sizeof(vAppuser) >= appuserIDlen && appuserIDlen > 0);
        return vector<uint8_t>(&vAppuser[0], &vAppuser[0] + appuserIDlen);
    }

    CUserID GetUserID() const {
        if (appuserIDlen == 6) {  // AccountType::REGID
            return CRegID(vector<uint8_t>(&vAppuser[0], &vAppuser[0] + appuserIDlen));
        } else if (appuserIDlen == 34) {  // AccountType::BASE58ADDR
            return CKeyID(string(&vAppuser[0], &vAppuser[0] + appuserIDlen));
        }

        return CUserID();
    }
};

class CAppUserAccount {
public:
    CAppUserAccount();
    CAppUserAccount(const string &userId);
    virtual ~CAppUserAccount();

    bool Operate(const vector<CAppFundOperate> &operate, vector<CReceipt> &receipts);
    bool GetAppCFund(CAppCFund &outFound, const vector<uint8_t> &tag, int32_t height);
    bool AutoMergeFreezeToFree(int32_t height);

    json_spirit::Object ToJson() const;
    string ToString() const;

    uint64_t GetBcoins() const { return bcoins; }
    void SetBcoins(uint64_t bcoins) { this->bcoins = bcoins; }

    const string &GetAccUserId() const { return user_id; }
    CUserID GetUserId() const;
    vector<CAppCFund> &GetFrozenFunds() { return frozen_funds; }
    uint64_t GetAllFreezedValues();

    IMPLEMENT_SERIALIZE(
		READWRITE(VARINT(bcoins));
		READWRITE(user_id);
		READWRITE(frozen_funds);
	)

    bool MinusAppCFund(const vector<uint8_t> &tag, uint64_t val, int32_t height);
    bool AddAppCFund(const vector<uint8_t> &tag, uint64_t val, int32_t height);
    bool MinusAppCFund(const CAppCFund &appFund);
    bool AddAppCFund(const CAppCFund &appFund);
    bool ChangeAppCFund(const CAppCFund &appFund);
    bool Operate(const CAppFundOperate &operate, vector<CReceipt> &receipts);

    bool IsEmpty() const { return user_id.empty(); }
    void SetEmpty() { user_id.clear(); }

private:
    uint64_t bcoins;  // 自由金额
    string user_id;
    vector<CAppCFund> frozen_funds;
};

class CAssetOperate {
public:
    CAssetOperate() {
        fundTagLen    = 0;
        timeoutHeight = 0;
        mMoney        = 0;
    }

    uint64_t GetUint64Value() const { return mMoney; }

    int32_t GetHeight() const { return timeoutHeight; }

    const vector<uint8_t> GetFundTagV() const {
        assert(sizeof(vFundTag) >= fundTagLen);
        vector<uint8_t> tag(&vFundTag[0], &vFundTag[fundTagLen]);
        return (tag);
    }

    IMPLEMENT_SERIALIZE(
		READWRITE(timeoutHeight);
		READWRITE(mMoney);
		READWRITE(fundTagLen);
        for (uint32_t i = 0; i < sizeof(vFundTag); ++i)
			READWRITE(vFundTag[i]);
	)

public:
    uint32_t timeoutHeight = 0;  //!< the transacion Timeout height
    uint64_t mMoney = 0;         //!< The transfer amount
    uint8_t fundTagLen = 0;
    uint8_t vFundTag[CAppCFund::MAX_TAG_SIZE] = {0};  //!< accountId
};

#endif  // VM_APP_ACCOUT_H
