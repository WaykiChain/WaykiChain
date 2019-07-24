// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php


#ifndef VM_APP_ACCOUT_H
#define VM_APP_ACCOUT_H

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "commons/serialize.h"
#include "commons/uint256.h"

#include <vector>

class CAppFundOperate;

class CAppCFund {
public:
	static const int MAX_TAG_SIZE  = 40;
	CAppCFund();
	CAppCFund(const CAppFundOperate &Op);
	CAppCFund(const CAppCFund &fund);
	CAppCFund(const vector<unsigned char> &vtag,uint64_t val,int nhight);
	bool MergeCFund( const CAppCFund &fund);
	json_spirit::Object ToJson()const;
	string ToString()const;

	void SetHeight(int height) { timeoutHeight = height; }
	int GetHeight() const { return timeoutHeight; }

	void SetValue(uint64_t value) { this->value = value; }
	uint64_t GetValue() const { return value; }

	void SetTag(const vector<unsigned char>& tag) { vTag = tag; }
	const vector<unsigned char> GetTag() const { return vTag; }

public:
	IMPLEMENT_SERIALIZE
	(
		READWRITE(VARINT(value));
		READWRITE(VARINT(timeoutHeight));
		READWRITE(vTag);
	)

private:
	uint64_t value;					//!< amount of money
	int timeoutHeight;				//!< time-out height
	vector<unsigned char> vTag;		//!< vTag of the tx which create the fund

};

enum APP_OP_TYPE{
	ADD_FREE_OP = 1,
	SUB_FREE_OP,
	ADD_TAG_OP,
	SUB_TAG_OP
}__attribute__((aligned(1)));


class CAppFundOperate {
public:
	CAppFundOperate();

	unsigned char opType;			//!OperType
	unsigned int timeoutHeight;		//!< the transacion Timeout height
	int64_t mMoney;			        //!<The transfer amount
	unsigned char appuserIDlen;
	unsigned char vAppuser[CAppCFund::MAX_TAG_SIZE];				//!< accountId
	unsigned char fundTagLen;
	unsigned char vFundTag[CAppCFund::MAX_TAG_SIZE];				//!< accountId

	CAppFundOperate(const vector<unsigned char> &AppTag,
		const vector<unsigned char> &FundTag,
		APP_OP_TYPE optype,
		int timeoutHeightIn,
		int64_t money)
	{
		assert(sizeof(vAppuser) >= AppTag.size());
		assert(sizeof(vFundTag) >= FundTag.size());
		assert((optype >= ADD_FREE_OP) && (optype <= SUB_TAG_OP));

		appuserIDlen = AppTag.size();
		fundTagLen = FundTag.size();
		memcpy(&vAppuser[0],&AppTag[0],AppTag.size());
		memcpy(&vFundTag[0],&FundTag[0],FundTag.size());
		mMoney = money;
		timeoutHeight = timeoutHeightIn;
		opType = optype;
	}

	IMPLEMENT_SERIALIZE
	(
		READWRITE(opType);
		READWRITE(timeoutHeight);
		READWRITE(mMoney);
		READWRITE(appuserIDlen);
		for (unsigned int i = 0;i < sizeof(vAppuser);++i)
		READWRITE(vAppuser[i]);
		READWRITE(fundTagLen);
		for (unsigned int i = 0;i < sizeof(vFundTag);++i)
		READWRITE(vFundTag[i]);
	)

	json_spirit::Object ToJson()const;
	string ToString()const;

	uint64_t GetUint64Value() const { return mMoney; }

	const vector<unsigned char> GetFundTagV() const {
		assert(sizeof(vFundTag) >= fundTagLen );
		vector<unsigned char> tag(&vFundTag[0], &vFundTag[fundTagLen]);
		return (tag);
	}

	const vector<unsigned char> GetAppUserV() const {
	//	cout<<appuserIDlen<<endl;
		assert(sizeof(vAppuser) >= appuserIDlen && appuserIDlen > 0);
		vector<unsigned char> tag(&vAppuser[0], &vAppuser[appuserIDlen]);
		return (tag);
	}

	unsigned char GetOpType() const {
		return opType;
	}

	bool SetOpType(unsigned char opType) {
		if ((opType >= ADD_FREE_OP) && (opType <= SUB_TAG_OP)) {
			this->opType = opType;
			return true;
		} else
			return false;
	}

	unsigned int GetOutHeight() const {
		return timeoutHeight;
	}

	void SetOutHeight(unsigned int timeoutHeightIn) {
		this->timeoutHeight = timeoutHeightIn;
	}
};


class CAppUserAccount {
public:
    CAppUserAccount();
    CAppUserAccount(const string &userId);
    bool Operate(const vector<CAppFundOperate> &Op);
    bool GetAppCFund(CAppCFund &outFound, const vector<unsigned char> &vTag, int height);

    bool AutoMergeFreezeToFree(int hight);

    virtual ~CAppUserAccount();

    json_spirit::Object ToJson() const;

    string ToString() const;

    uint64_t GetBcoins() const { return bcoins; }
    void SetBcoins(uint64_t bcoins) { this->bcoins = bcoins; }

    const string& GetAccUserId() const { return mAccUserID; }

    // void SetAccUserId(const vector<unsigned char>& accUserId) {
    // 	mAccUserID = accUserId;
    // }

    vector<CAppCFund> &GetFrozenFunds() { return vFrozenFunds; }

    // void SetFrozenFund(const vector<CAppCFund>& vtmp)
    // {
    // 	vFrozenFunds.clear();
    // 	for (int i = 0; i < (int)vtmp.size(); i++)
    // 		vFrozenFunds.push_back(vtmp[i]);
    // }

    uint64_t GetAllFreezedValues();

    IMPLEMENT_SERIALIZE(
		READWRITE(VARINT(bcoins));
		READWRITE(mAccUserID);
		READWRITE(vFrozenFunds);)

    bool MinusAppCFund(const vector<unsigned char> &vtag, uint64_t val, int nhight);
    bool AddAppCFund(const vector<unsigned char> &vtag, uint64_t val, int nhight);
    bool MinusAppCFund(const CAppCFund &inFound);
    bool AddAppCFund(const CAppCFund &inFound);
    bool ChangeAppCFund(const CAppCFund &inFound);
    bool Operate(const CAppFundOperate &Op);

	bool IsEmpty() const { return mAccUserID.empty(); }

	void SetEmpty() { mAccUserID.clear(); }

private:
	uint64_t bcoins;       //自由金额
	string  mAccUserID;
	vector<CAppCFund> vFrozenFunds;
};

class CAssetOperate
{
public:
	CAssetOperate() {
		fundTagLen = 0;
		timeoutHeight = 0;
		mMoney = 0;
	}

	uint64_t GetUint64Value() const { return mMoney; }

	int GetHeight() const { return timeoutHeight; }

	const vector<unsigned char> GetFundTagV() const {
		assert(sizeof(vFundTag) >= fundTagLen );
		vector<unsigned char> tag(&vFundTag[0], &vFundTag[ fundTagLen ]);
		return (tag);
	}

	IMPLEMENT_SERIALIZE
	(
		READWRITE(timeoutHeight);
		READWRITE(mMoney);
		READWRITE(fundTagLen);
		for(unsigned int i = 0;i < sizeof(vFundTag);++i)
		READWRITE(vFundTag[i]);
	)

public:
	unsigned int timeoutHeight;		    //!< the transacion Timeout height
	uint64_t mMoney;			        //!<The transfer amount
	unsigned char fundTagLen;
	unsigned char vFundTag[ CAppCFund::MAX_TAG_SIZE ];				//!< accountId
};

#endif /* VM_APP_ACCOUT_H */
