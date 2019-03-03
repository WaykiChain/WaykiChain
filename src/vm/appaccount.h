/*
 * appuseraccout.h
 *
 *  Created on: 2015年3月30日
 *      Author: ranger.shi
 */

#ifndef APPUSERACCOUT_H_
#define APPUSERACCOUT_H_

#include "tx.h"

class CAppFundOperate;

class CAppCFund {
public:
	static const int MAX_TAG_SIZE  = 40;
	CAppCFund();
	CAppCFund(const CAppFundOperate &Op);
	CAppCFund(const CAppCFund &fund);
	CAppCFund(const vector<unsigned char> &vtag,uint64_t val,int nhight);
	bool MergeCFund( const CAppCFund &fund);
	Object ToJSON()const;
	string ToString()const;

	void SetHeight(int height) { nHeight = height; }
	int GetHeight() const { return nHeight; }

	void SetValue(uint64_t value) { this->value = value; }
	uint64_t GetValue() const { return value; }
	
	void SetTag(const vector<unsigned char>& tag) { vTag = tag; }
	const vector<unsigned char> GetTag() const { return vTag; }

public:
	IMPLEMENT_SERIALIZE
	(
		READWRITE(VARINT(value));
		READWRITE(VARINT(nHeight));
		READWRITE(vTag);
	)

private:
	uint64_t value;					//!< amount of money
	int nHeight;					//!< time-out height
	vector<unsigned char> vTag;	//!< vTag of the tx which create the fund

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

	unsigned char opType;		//!OperType
	unsigned int outHeight;		    //!< the transacion Timeout height
	int64_t mMoney;			        //!<The transfer amount
	unsigned char appuserIDlen;
	unsigned char vAppuser[CAppCFund::MAX_TAG_SIZE ];				//!< accountid
	unsigned char fundTagLen;
	unsigned char vFundTag[CAppCFund::MAX_TAG_SIZE ];				//!< accountid

	CAppFundOperate(const vector<unsigned char> &AppTag,
		const vector<unsigned char> &FundTag,
		APP_OP_TYPE optype,
		int timeout,
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
		outHeight = timeout;
		opType = optype;
	}

	IMPLEMENT_SERIALIZE
	(
		READWRITE(opType);
		READWRITE(outHeight);
		READWRITE(mMoney);
		READWRITE(appuserIDlen);
		for (unsigned int i = 0;i < sizeof(vAppuser);++i)
		READWRITE(vAppuser[i]);
		READWRITE(fundTagLen);
		for (unsigned int i = 0;i < sizeof(vFundTag);++i)
		READWRITE(vFundTag[i]);
	)

	Object ToJSON()const;
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
		return outHeight;
	}

	void SetOutHeight(unsigned int outHeight) {
		this->outHeight = outHeight;
	}
};


class CAppUserAccount {
public:
	CAppUserAccount();
	CAppUserAccount(const vector<unsigned char> &userId);
	bool Operate(const vector<CAppFundOperate> &Op);
	bool GetAppCFund(CAppCFund &outFound,const vector<unsigned char> &vtag,int nheight);

	bool AutoMergeFreezeToFree(int hight);

	virtual ~CAppUserAccount();

	Object ToJSON()const;

	string ToString()const;

	uint64_t GetLlValues() const { return llValues; }
	void SetLlValues(uint64_t llValues) { this->llValues = llValues; }

	const vector<unsigned char>& GetAccUserId() const { return mAccUserID; }

	// void SetAccUserId(const vector<unsigned char>& accUserId) {
	// 	mAccUserID = accUserId;
	// }

	vector<CAppCFund>& GetFrozenFunds() { return vFrozenFunds; }

	// void SetFrozenFund(const vector<CAppCFund>& vtmp)
	// {
	// 	vFrozenFunds.clear();
	// 	for (int i = 0; i < (int)vtmp.size(); i++)
	// 		vFrozenFunds.push_back(vtmp[i]);
	// }

	uint64_t GetAllFreezedValues();

	IMPLEMENT_SERIALIZE
	(
		READWRITE(VARINT(llValues));
		READWRITE(mAccUserID);
		READWRITE(vFrozenFunds);
	)

	bool MinusAppCFund(const vector<unsigned char> &vtag,uint64_t val,int nhight);
	bool AddAppCFund(const vector<unsigned char>& vtag, uint64_t val, int nhight);
	bool MinusAppCFund(const CAppCFund &inFound);
	bool AddAppCFund(const CAppCFund &inFound);
	bool ChangeAppCFund(const CAppCFund &inFound);
	bool Operate(const CAppFundOperate &Op);

private:
	uint64_t llValues;       //自由金额
	vector<unsigned char>  mAccUserID;
	vector<CAppCFund> vFrozenFunds;
};

class CAssetOperate
{
public:
	CAssetOperate() {
		fundTagLen = 0;
		outHeight = 0;
		mMoney = 0;
	}

	uint64_t GetUint64Value() const { return mMoney; }

	int GetHeight() const { return outHeight; }

	const vector<unsigned char> GetFundTagV() const {
		assert(sizeof(vFundTag) >= fundTagLen );
		vector<unsigned char> tag(&vFundTag[0], &vFundTag[ fundTagLen ]);
		return (tag);
	}

	IMPLEMENT_SERIALIZE
	(
		READWRITE(outHeight);
		READWRITE(mMoney);
		READWRITE(fundTagLen);
		for(unsigned int i = 0;i < sizeof(vFundTag);++i)
		READWRITE(vFundTag[i]);
	)

public:
	unsigned int outHeight;		    //!< the transacion Timeout height
	uint64_t mMoney;			        //!<The transfer amount
	unsigned char fundTagLen;
	unsigned char vFundTag[ CAppCFund::MAX_TAG_SIZE ];				//!< accountid
};

#endif /* APPUSERACCOUT_H_ */
