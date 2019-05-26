// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_SYNCDATA_H
#define PERSIST_SYNCDATA_H

#include "db.h"
#include "main.h"
#include "commons/uint256.h"
#include "accounts/key.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

namespace SyncData
{

class CSyncData
{
	public:
		CSyncData() {};
		virtual ~CSyncData() {};
		bool CheckSignature(const std::string& pubKey);
		bool Sign(const std::vector<unsigned char>& priKey, const std::vector<unsigned char>& syncData);
		bool Sign(const CKey& priKey, const std::vector<unsigned char>& syncData);
		const std::vector<unsigned char>& GetMessageData() const
		{
			return m_vchMsg;
		}
		json_spirit::Object ToJsonObj();
		IMPLEMENT_SERIALIZE
	    (
	        READWRITE(m_vchMsg);
	        READWRITE(m_vchSig);
	    )
	public:
		std::vector<unsigned char> 	m_vchMsg;
		std::vector<unsigned char> 	m_vchSig;
};

}

#endif // PERSIST_SYNCDATA_H
