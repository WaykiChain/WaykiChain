// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2019- The WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef SYNC_DATA_DB_H_
#define SYNC_DATA_DB_H_

#include "sync.h"
#include "syncdata.h"
#include "net.h"
#include "persistence/leveldbwrapper.h"

#include <boost/shared_ptr.hpp>
#include <map>

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
