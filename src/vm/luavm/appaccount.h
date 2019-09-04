// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef VM_APP_ACCOUT_H
#define VM_APP_ACCOUT_H

#include "commons/serialize.h"
#include "commons/uint256.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

#include <vector>

using namespace std;

class CAppFundOperate;

class CAppCFund {
public:
    static const int32_t MAX_TAG_SIZE = 40;

    CAppCFund();
    CAppCFund(const CAppFundOperate &operate);
    CAppCFund(const CAppCFund &fund);
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

    uint8_t opType;          //!< OperType
    uint32_t timeoutHeight;  //!< the transacion Timeout height
    int64_t mMoney;          //!< The transfer amount
    uint8_t appuserIDlen;
    uint8_t vAppuser[CAppCFund::MAX_TAG_SIZE];  //!< accountId
    uint8_t fundTagLen;
    uint8_t vFundTag[CAppCFund::MAX_TAG_SIZE];  //!< accountId

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

    const vector<uint8_t> GetFundTagV() const {
        assert(sizeof(vFundTag) >= fundTagLen);
        vector<uint8_t> tag(&vFundTag[0], &vFundTag[fundTagLen]);
        return (tag);
    }

    const vector<uint8_t> GetAppUserV() const {
        assert(sizeof(vAppuser) >= appuserIDlen && appuserIDlen > 0);
        vector<uint8_t> tag(&vAppuser[0], &vAppuser[appuserIDlen]);
        return (tag);
    }

    uint8_t GetOpType() const { return opType; }

    bool SetOpType(uint8_t opType) {
        if ((opType >= ADD_FREE_OP) && (opType <= SUB_TAG_OP)) {
            this->opType = opType;
            return true;
        } else
            return false;
    }

    uint32_t GetOutHeight() const { return timeoutHeight; }

    void SetOutHeight(uint32_t timeoutHeightIn) { this->timeoutHeight = timeoutHeightIn; }
};

class CAppUserAccount {
public:
    CAppUserAccount();
    CAppUserAccount(const string &userId);
    virtual ~CAppUserAccount();

    bool Operate(const vector<CAppFundOperate> &operate);
    bool GetAppCFund(CAppCFund &outFound, const vector<uint8_t> &tag, int32_t height);
    bool AutoMergeFreezeToFree(int32_t height);

    json_spirit::Object ToJson() const;
    string ToString() const;

    uint64_t GetBcoins() const { return bcoins; }
    void SetBcoins(uint64_t bcoins) { this->bcoins = bcoins; }

    const string &GetAccUserId() const { return mAccUserID; }
    vector<CAppCFund> &GetFrozenFunds() { return vFrozenFunds; }
    uint64_t GetAllFreezedValues();

    IMPLEMENT_SERIALIZE(
		READWRITE(VARINT(bcoins));
		READWRITE(mAccUserID);
		READWRITE(vFrozenFunds);
	)

    bool MinusAppCFund(const vector<uint8_t> &tag, uint64_t val, int32_t height);
    bool AddAppCFund(const vector<uint8_t> &tag, uint64_t val, int32_t height);
    bool MinusAppCFund(const CAppCFund &appFund);
    bool AddAppCFund(const CAppCFund &appFund);
    bool ChangeAppCFund(const CAppCFund &appFund);
    bool Operate(const CAppFundOperate &operate);

    bool IsEmpty() const { return mAccUserID.empty(); }

    void SetEmpty() { mAccUserID.clear(); }

private:
    uint64_t bcoins;  //自由金额
    string mAccUserID;
    vector<CAppCFund> vFrozenFunds;
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
    uint32_t timeoutHeight;  //!< the transacion Timeout height
    uint64_t mMoney;         //!< The transfer amount
    uint8_t fundTagLen;
    uint8_t vFundTag[CAppCFund::MAX_TAG_SIZE];  //!< accountId
};

#endif  // VM_APP_ACCOUT_H
