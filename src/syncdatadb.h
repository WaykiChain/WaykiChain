/*
 * CSyncDataDb.h
 *
 *  Created on: Jun 14, 2014
 *      Author: ranger.shi
 */

#ifndef SYNC_DATA_DB_H_
#define SYNC_DATA_DB_H_

#include "sync.h"
#include <map>
#include "syncdata.h"
#include "net.h"
#include "leveldbwrapper.h"
#include <boost/shared_ptr.hpp>

namespace SyncData
{

class CSyncCheckPoint
{
	public:
		CSyncCheckPoint():m_version(1), m_height(0),m_hashCheckpoint(){}
		void SetData(const CSyncData& data)
		{
			CDataStream sstream(data.GetMessageData(), SER_NETWORK, PROTOCOL_VERSION);
			sstream>>*this;
		}
		IMPLEMENT_SERIALIZE
		(
			READWRITE(this->m_version);
			nVersion = this->m_version;
			READWRITE(m_height);
			READWRITE(m_hashCheckpoint);
		)
	public:
		int 		m_version;
		int			m_height;
		uint256 	m_hashCheckpoint;      // checkpoint block
};

class CSyncDataDb
{
	public:
		bool WriteCheckpoint(int height, const CSyncData& data);
		bool ReadCheckpoint(int height, CSyncData& data);
		bool ExistCheckpoint(int height);
		bool LoadCheckPoint(std::map<int, uint256>& values);

	public:
		bool InitializeSyncDataDb(const boost::filesystem::path &path);
		void CloseSyncDataDb()
		{
			if (m_dbPoint)
			{
				m_dbPoint.reset();
			}
		}
	private:
		static boost::shared_ptr<CLevelDBWrapper>	m_dbPoint;
};

} /* namespace SyncData */

#endif /* SYNC_DATA_DB_H_ */
