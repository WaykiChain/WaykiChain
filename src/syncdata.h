/*
 * CSyncData.h
 *
 *  Created on: Jun 14, 2014
 *      Author: ranger.shi
 */

#ifndef CSYNC_DATA_H_
#define CSYNC_DATA_H_

#include "db.h"
#include "main.h"
#include "uint256.h"
#include "key.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

namespace SyncData
{

class CSyncData
{
	public:
		CSyncData();
		virtual ~CSyncData();
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

} /* namespace SyncData */

#endif /* CSYNC_DATA_H_ */
