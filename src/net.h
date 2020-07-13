// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_NET_H
#define COIN_NET_H

#include "commons/uint256.h"
#include "commons/util/util.h"
#include "crypto/hash.h"
#include "sync.h"
#include "netbase.h"


#include <stdint.h>
#include <deque>

#ifndef WIN32
#include <arpa/inet.h>
#endif

#include <openssl/rand.h>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>

class CAddrMan;
class CBlockIndex;
class CNode;
class LocalServiceInfo;
class CInv;

//p2p_xiaoyu_20191126
/** Time between pings automatically sent out for latency probing and keepalive (in seconds). */
static const int PING_INTERVAL = 2 * 60;
/** Time after which to disconnect, after waiting for a ping response (or inactivity). */
static const int TIMEOUT_INTERVAL = 20 * 60;

/** -peertimeout default */
static const int64_t DEFAULT_PEER_CONNECT_TIMEOUT = 60;

inline uint32_t ReceiveFloodSize() { return 1000 * SysCfg().GetArg("-maxreceivebuffer", 5 * 1000); }
void AddOneShot(string strDest);
bool RecvLine(SOCKET hSocket, string& strLine);
bool GetMyPublicIP(CNetAddr& ipRet);
void AddressCurrentlyConnected(const CService& addr);
CNode* FindNode(const CNetAddr& ip);
CNode* FindNode(const CService& ip);
CNode* ConnectNode(CAddress addrConnect, const char* strDest = nullptr);
void MapPort(bool fUseUPnP);
uint16_t GetListenPort();
bool BindListenPort(const CService& bindAddr, string& strError = REF(string()));
void StartNode(boost::thread_group& threadGroup);
bool StopNode();

enum {
    LOCAL_NONE,    // unknown
    LOCAL_IF,      // address a local interface listens on
    LOCAL_BIND,    // address explicit bound to
    LOCAL_UPNP,    // address reported by UPnP
    LOCAL_HTTP,    // address reported by whatismyip.com and similar
    LOCAL_MANUAL,  // address explicitly specified (-externalip=)

    LOCAL_MAX
};

void SetLimited(enum Network net, bool fLimited = true);
bool IsLimited(enum Network net);
bool IsLimited(const CNetAddr& addr);
bool AddLocal(const CService& addr, int32_t nScore = LOCAL_NONE);
bool AddLocal(const CNetAddr& addr, int32_t nScore = LOCAL_NONE);
bool SeenLocal(const CService& addr);
bool IsLocal(const CService& addr);

bool IsReachable(const CNetAddr& addr);
void SetReachable(enum Network net, bool fFlag = true);

extern bool fDiscover;
extern uint64_t nLocalHostNonce;
extern CAddrMan addrman;
extern int32_t nMaxConnections;
extern vector<CNode*> vNodes;
extern CCriticalSection cs_vNodes;
extern map<CInv, CDataStream> mapRelay;
extern deque<pair<int64_t, CInv> > vRelayExpiration;
extern CCriticalSection cs_mapRelay;
extern vector<string> vAddedNodes;
extern CCriticalSection cs_vAddedNodes;
extern map<CNetAddr, LocalServiceInfo> mapLocalHost;

void RelayTransaction(CBaseTx* pBaseTx, const uint256& hash);
void RelayTransaction(CBaseTx* pBaseTx, const uint256& hash, const CDataStream& ss);

/** Access to the (IP) address database (peers.dat) */
class CAddrDB {
private:
    boost::filesystem::path pathAddr;

public:
    CAddrDB();
    bool Write(const CAddrMan& addr);
    bool Read(CAddrMan& addr);
};

#endif
