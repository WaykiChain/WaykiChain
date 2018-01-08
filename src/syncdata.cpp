/*
 * CSyncData.cpp
 *
 *  Created on: Jun 14, 2014
 *      Author: ranger.shi
 */

#include "syncdata.h"

namespace SyncData {

CSyncData::CSyncData()
{
}

CSyncData::~CSyncData()
{
}

bool CSyncData::CheckSignature(const std::string& pubKey)
{
	CPubKey key(ParseHex(pubKey));
	LogPrint("CHECKPOINT", "CheckSignature PubKey:%s\n", pubKey.c_str());
	LogPrint("CHECKPOINT", "CheckSignature Msg:%s\n", HexStr(m_vchMsg).c_str());
	LogPrint("CHECKPOINT", "CheckSignature Sig:%s\n", HexStr(m_vchSig).c_str());
    if (!key.Verify(Hash(m_vchMsg.begin(), m_vchMsg.end()), m_vchSig))
    {
        return ERRORMSG("CSyncCheckpoint::CheckSignature : verify signature failed");
    }
    return true;
}

bool CSyncData::Sign(const std::vector<unsigned char>& priKey, const std::vector<unsigned char>& syncData)
{
    CKey key;
	m_vchMsg.assign(syncData.begin(), syncData.end());
    key.Set(priKey.begin(), priKey.end(), true);
    if (!key.Sign(Hash(m_vchMsg.begin(), m_vchMsg.end()), m_vchSig))
    {
    	return ERRORMSG("CSyncCheckpoint::Sign: Unable to sign checkpoint, check private key?");
    }
	return true;
}

bool CSyncData::Sign(const CKey& priKey, const std::vector<unsigned char>& syncData)
{
	m_vchMsg.assign(syncData.begin(), syncData.end());
    if (!priKey.Sign(Hash(m_vchMsg.begin(), m_vchMsg.end()), m_vchSig))
    {
    	return ERRORMSG("CSyncCheckpoint::Sign: Unable to sign checkpoint, check private key?");
    }
	return true;
}

json_spirit::Object CSyncData::ToJsonObj()
{
	Object obj;
	obj.push_back(Pair("msg", HexStr(m_vchMsg)));
	obj.push_back(Pair("sig", HexStr(m_vchSig)));
	return obj;
}

} /* namespace SyncData */
