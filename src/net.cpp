// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/coin-config.h"
#endif

#include "addrman.h"
#include "config/chainparams.h"
#include "net.h"
#include "nodeinfo.h"
#include "tx/tx.h"

#ifdef WIN32
#include <string.h>
#else
#include <fcntl.h>
#endif

#ifdef USE_UPNP
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/miniwget.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>
#endif

#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

#include <fstream>
#include <sstream>
#include <string>
#include <thread>

#include <boost/filesystem.hpp>

// Dump addresses to peers.dat every 15 minutes (900s)
#define DUMP_ADDRESSES_INTERVAL 900

#if !defined(HAVE_MSG_NOSIGNAL) && !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif

using namespace std;
using namespace boost;

static const int32_t MAX_OUTBOUND_CONNECTIONS = 8;

bool OpenNetworkConnection(const CAddress& addrConnect, CSemaphoreGrant* grantOutbound = nullptr,
                           const char* strDest = nullptr, bool fOneShot = false);

//
// Global state variables
//
bool fDiscover          = true;
uint64_t nLocalServices = NODE_NETWORK;
CCriticalSection cs_mapLocalHost;
map<CNetAddr, LocalServiceInfo> mapLocalHost;
static bool vfReachable[NET_MAX] = {};
static bool vfLimited[NET_MAX]   = {};
static CNode* pnodeLocalHost     = nullptr;
static CNode* pnodeSync          = nullptr;
uint64_t nLocalHostNonce         = 0;
static vector<SOCKET> vhListenSocket;
CAddrMan addrman;
int32_t nMaxConnections = 125;
string ipHost = "";

vector<CNode*> vNodes;
CCriticalSection cs_vNodes;
map<CInv, CDataStream> mapRelay;
deque<pair<int64_t, CInv> > vRelayExpiration;
CCriticalSection cs_mapRelay;
limitedmap<CInv, int64_t> mapAlreadyAskedFor(MAX_INV_SZ);

static deque<string> vOneShots;
CCriticalSection cs_vOneShots;

set<CNetAddr> setservAddNodeAddresses;
CCriticalSection cs_setservAddNodeAddresses;

vector<string> vAddedNodes;
CCriticalSection cs_vAddedNodes;

NodeId nLastNodeId = 0;
CCriticalSection cs_nLastNodeId;

static CSemaphore* semOutbound = nullptr;

// Signals for message handling
static CNodeSignals g_node_signals;
CNodeSignals& GetNodeSignals() { return g_node_signals; }

void AddOneShot(string strDest) {
    LOCK(cs_vOneShots);
    vOneShots.push_back(strDest);
}

uint16_t GetListenPort() { return (uint16_t)(SysCfg().GetArg("-port", SysCfg().GetDefaultPort())); }

// find 'best' local address for a particular peer
bool GetLocal(CService& addr, const CNetAddr* paddrPeer) {
    if (fNoListen)
        return false;

    int32_t nBestScore        = -1;
    int32_t nBestReachability = -1;
    {
        LOCK(cs_mapLocalHost);
        for (map<CNetAddr, LocalServiceInfo>::iterator it = mapLocalHost.begin(); it != mapLocalHost.end(); it++) {
            int32_t nScore        = (*it).second.nScore;
            int32_t nReachability = (*it).first.GetReachabilityFrom(paddrPeer);
            if (nReachability > nBestReachability || (nReachability == nBestReachability && nScore > nBestScore)) {
                addr              = CService((*it).first, (*it).second.nPort);
                nBestReachability = nReachability;
                nBestScore        = nScore;
            }
        }
    }
    return nBestScore >= 0;
}

// get best local address for a particular peer as a CAddress
CAddress GetLocalAddress(const CNetAddr* paddrPeer) {
    CAddress ret(CService("0.0.0.0", 0), 0);
    CService addr;
    if (GetLocal(addr, paddrPeer)) {
        ret           = CAddress(addr);
        ret.nServices = nLocalServices;
        ret.nTime     = GetAdjustedTime();
    }
    return ret;
}

bool RecvLine(SOCKET hSocket, string& strLine) {
    strLine = "";
    while (true) {
        char c;
        int32_t nBytes = recv(hSocket, &c, 1, 0);
        if (nBytes > 0) {
            if (c == '\n')
                continue;
            if (c == '\r')
                return true;
            strLine += c;
            if (strLine.size() >= 9000)
                return true;
        } else if (nBytes <= 0) {
            boost::this_thread::interruption_point();
            if (nBytes < 0) {
                int32_t nErr = WSAGetLastError();
                if (nErr == WSAEMSGSIZE)
                    continue;
                if (nErr == WSAEWOULDBLOCK || nErr == WSAEINTR || nErr == WSAEINPROGRESS) {
                    MilliSleep(10);
                    continue;
                }
            }
            if (!strLine.empty())
                return true;
            if (nBytes == 0) {
                // socket closed
                LogPrint("net", "socket closed\n");
                return false;
            } else {
                // socket error
                int32_t nErr = WSAGetLastError();
                LogPrint("net", "recv failed: %s\n", NetworkErrorString(nErr));
                return false;
            }
        }
    }
}

// used when scores of local addresses may have changed
// pushes better local address to peers
void static AdvertizeLocal() {
    LOCK(cs_vNodes);
    for (auto pNode : vNodes) {
        if (pNode->fSuccessfullyConnected) {
            CAddress addrLocal = GetLocalAddress(&pNode->addr);
            if (addrLocal.IsRoutable() && (CService)addrLocal != (CService)pNode->addrLocal) {
                pNode->PushAddress(addrLocal);
                pNode->addrLocal = addrLocal;
            }
        }
    }
}

void SetReachable(enum Network net, bool fFlag) {
    LOCK(cs_mapLocalHost);
    vfReachable[net] = fFlag;
    if (net == NET_IPV6 && fFlag)
        vfReachable[NET_IPV4] = true;
}

// learn a new local address
bool AddLocal(const CService& addr, int32_t nScore) {
    if (!addr.IsRoutable())
        return false;

    if (!fDiscover && nScore < LOCAL_MANUAL)
        return false;

    if (IsLimited(addr))
        return false;

    LogPrint("INFO", "AddLocal(%s,%i)\n", addr.ToString(), nScore);

    {
        LOCK(cs_mapLocalHost);
        bool fAlready          = mapLocalHost.count(addr) > 0;
        LocalServiceInfo& info = mapLocalHost[addr];
        if (!fAlready || nScore >= info.nScore) {
            info.nScore = nScore + (fAlready ? 1 : 0);
            info.nPort  = addr.GetPort();
        }
        SetReachable(addr.GetNetwork());
    }

    AdvertizeLocal();

    return true;
}

bool AddLocal(const CNetAddr& addr, int32_t nScore) { return AddLocal(CService(addr, GetListenPort()), nScore); }

/** Make a particular network entirely off-limits (no automatic connects to it) */
void SetLimited(enum Network net, bool fLimited) {
    if (net == NET_UNROUTABLE)
        return;
    LOCK(cs_mapLocalHost);
    vfLimited[net] = fLimited;
}

bool IsLimited(enum Network net) {
    LOCK(cs_mapLocalHost);
    return vfLimited[net];
}

bool IsLimited(const CNetAddr& addr) { return IsLimited(addr.GetNetwork()); }

/** vote for a local address */
bool SeenLocal(const CService& addr) {
    {
        LOCK(cs_mapLocalHost);
        if (mapLocalHost.count(addr) == 0)
            return false;
        mapLocalHost[addr].nScore++;
    }

    AdvertizeLocal();

    return true;
}

/** check whether a given address is potentially local */
bool IsLocal(const CService& addr) {
    LOCK(cs_mapLocalHost);
    return mapLocalHost.count(addr) > 0;
}

/** check whether a given address is in a network we can probably connect to */
bool IsReachable(const CNetAddr& addr) {
    LOCK(cs_mapLocalHost);
    enum Network net = addr.GetNetwork();
    return vfReachable[net] && !vfLimited[net];
}

static string GetSystemInfo() {
    struct sysinfo info;
    sysinfo(&info);

    struct statvfs fsinfo;
    statvfs("/", &fsinfo);

    struct utsname utsName;
    uname(&utsName);

    struct NodeInfo nodeinfo;
    getnodeinfo(&nodeinfo);

    string vcpus    = std::to_string(std::thread::hardware_concurrency());
    string memory   = std::to_string(info.totalram * info.mem_unit / 1024 / 1024);      // Unit: MB
    string totalHDD = std::to_string(fsinfo.f_frsize * fsinfo.f_blocks / 1024 / 1024);  // Unit: MB
    string freeHDD  = std::to_string(fsinfo.f_bsize * fsinfo.f_bfree / 1024 / 1024);    // Unit: MB
    string osType   = string(utsName.sysname);
    string osVer    = string(utsName.release);
    string nv       = nodeinfo.nv;
    string nfp      = nodeinfo.nfp;
    string synh     = std::to_string(nodeinfo.synh);
    string tiph     = std::to_string(nodeinfo.tiph);
    string finh     = std::to_string(nodeinfo.finh);

    string json;

    json += "{";
    json += "\"vcpus\":"    + vcpus     + ",";
    json += "\"mem\":"      + memory    + ",";
    json += "\"diskt\":"    + totalHDD  + ",";
    json += "\"diskf\":"    + freeHDD   + ",";
    json += "\"ost\":\""    + osType    + "\",";
    json += "\"osv\":\""    + osVer     + "\",";
    json += "\"nv\":\""     + nv        + "\",";
    json += "\"nfp\":\""    + nfp       + "\",";
    json += "\"synh\":"     + synh      + ",";
    json += "\"tiph\":"     + tiph      + ",";
    json += "\"finh\":"     + finh      ; //finalized height
    json += "}";

    return json;
}

bool GetMyExternalIP(CNetAddr& ipRet) {
    ipHost = SysCfg().GetArg("-ipserver", "");
    if (ipHost == "") {
        if (SysCfg().NetworkID() == MAIN_NET)
            ipHost = "wiccip.me";
        else if (SysCfg().NetworkID() == TEST_NET)
            ipHost = "wiccip.com";
        else
            return true; // no need for RegTest network
    }

    if (ipHost.find("/") != std::string::npos) {
        string host = ipHost;
        ipHost = "";
        return ERRORMSG("GetMyExternalIP() : ipserver (%s) contains /", host);
    }

    CService addrConnect(ipHost, 80, true);
    if (!addrConnect.IsValid())
        return ERRORMSG("GetMyExternalIP() : service is unavalable: %s\n", ipHost);

    stringstream stream;
    stream << "GET" << " " << "/ip" << " " << "HTTP/1.1\r\n";
    stream << "Host: " << ipHost << "\r\n";
    stream << "Connection: close\r\n\r\n";
    stream << "";
    string request = stream.str();

    SOCKET hSocket;
    if (!ConnectSocket(addrConnect, hSocket))
        return ERRORMSG("GetMyExternalIP() : failed to connect IP server: %s", addrConnect.ToString());

    send(hSocket, request.c_str(), request.length(), MSG_NOSIGNAL);

    char buffer[4 * 1024] = {'\0'};
    int32_t offset            = 0;
    int32_t rc                = 0;
    while (offset <= 3 * 1024 && (rc = recv(hSocket, buffer + offset, 1024, MSG_NOSIGNAL))) {
        offset += rc;
    }

    closesocket(hSocket);

    if (strlen(buffer) == 0)
        return ERRORMSG("GetMyExternalIP() : failed to receive data from server: %s", addrConnect.ToString());

    static const char* const key = "\"ipAddress\":\"";
    char* from                   = strstr(buffer, key);
    if (from == nullptr) {
        return ERRORMSG("GetMyExternalIP() : invalid message");
    }
    from += strlen(key);
    char* to   = strstr(from, "\"");
    string ip(from, to);
    externalIp = ip;
    CService ipAddr(externalIp, 0, true);
    if (!ipAddr.IsValid() /* || !ipAddr.IsRoutable() */)
        return ERRORMSG("GetMyExternalIP() : invalid external ip address: %s", externalIp);

    ipRet.SetIP(ipAddr);

    LogPrint("INFO", "GetMyExternalIP() : My External IP is: %s\n", externalIp);

    return true;
}

void ThreadGetMyExternalIP() {
    CNetAddr addrLocalHost;
    if (GetMyExternalIP(addrLocalHost)) {
        AddLocal(addrLocalHost, LOCAL_HTTP);
    }
}

bool PostNodeInfo() {
     if (ipHost == "")
        return ERRORMSG("PostNodeInfo() : ipserver uninitialized");

    string content  = GetSystemInfo();

    CService addrConnect(ipHost, 80, true);
    if (!addrConnect.IsValid())
        return ERRORMSG("PostNodeInfo() : service is unavalable: %s\n", ipHost);

    stringstream stream;
    stream << "POST" << " " << "/info" << " " << "HTTP/1.1\r\n";
    stream << "Host: " << ipHost << "\r\n";
    stream << "Content-Type: application/json\r\n";
    stream << "Content-Length: " << content.length() << "\r\n";
    stream << "Connection: close\r\n\r\n";
    stream << content;
    string request = stream.str();

    SOCKET hSocket;
    if (!ConnectSocket(addrConnect, hSocket))
        return ERRORMSG("PostNodeInfo() : failed to connect to server: %s", addrConnect.ToString());

    send(hSocket, request.c_str(), request.length(), MSG_NOSIGNAL);
    closesocket(hSocket);

    return true;
}

void ThreadPostNodeInfo() {
    int64_t start = GetTime();

    int64_t interval_minutes = SysCfg().GetArg("-nodeinfopostinterval", 60L); //default is one hour
    while (true) {
        boost::this_thread::interruption_point();

        while (GetTime() - start < interval_minutes * 60) {
            boost::this_thread::interruption_point();
            MilliSleep(1000); //sleep for 1 sec to check again.
        }
        start = GetTime();
        PostNodeInfo();
    }
}

void AddressCurrentlyConnected(const CService& addr) { addrman.Connected(addr); }

uint64_t CNode::nTotalBytesRecv = 0;
uint64_t CNode::nTotalBytesSent = 0;
CCriticalSection CNode::cs_totalBytesRecv;
CCriticalSection CNode::cs_totalBytesSent;

CNode* FindNode(const CNetAddr& ip) {
    LOCK(cs_vNodes);
    for (auto pNode : vNodes)
        if ((CNetAddr)pNode->addr == ip)
            return (pNode);
    return nullptr;
}

CNode* FindNode(string addrName) {
    LOCK(cs_vNodes);
    for (auto pNode : vNodes)
        if (pNode->addrName == addrName)
            return (pNode);
    return nullptr;
}

CNode* FindNode(const CService& addr) {
    LOCK(cs_vNodes);
    for (auto pNode : vNodes)
        if ((CService)pNode->addr == addr)
            return (pNode);
    return nullptr;
}

CNode* ConnectNode(CAddress addrConnect, const char* pszDest) {
    if (pszDest == nullptr) {
        if (IsLocal(addrConnect))
            return nullptr;

        // Look for an existing connection
        CNode* pNode = FindNode((CService)addrConnect);
        if (pNode) {
            pNode->AddRef();
            return pNode;
        }
    }

    LogPrint("net", "trying connection %s lastseen=%.1fhrs\n", pszDest ? pszDest : addrConnect.ToString(),
             pszDest ? 0 : (double)(GetAdjustedTime() - addrConnect.nTime) / 3600.0);

    // Connect
    SOCKET hSocket;
    if (pszDest ? ConnectSocketByName(addrConnect, hSocket, pszDest, SysCfg().GetDefaultPort())
                : ConnectSocket(addrConnect, hSocket)) {
        addrman.Attempt(addrConnect);

        LogPrint("net", "connected %s\n", pszDest ? pszDest : addrConnect.ToString());

        // Set to non-blocking
#ifdef WIN32
        u_long nOne = 1;
        if (ioctlsocket(hSocket, FIONBIO, &nOne) == SOCKET_ERROR)
            LogPrint("INFO", "ConnectSocket() : ioctlsocket non-blocking setting failed, error %s\n",
                     NetworkErrorString(WSAGetLastError()));
#else
        if (fcntl(hSocket, F_SETFL, O_NONBLOCK) == SOCKET_ERROR)
            LogPrint("INFO", "ConnectSocket() : fcntl non-blocking setting failed, error %s\n",
                     NetworkErrorString(errno));
#endif

        // Add node
        CNode* pNode = new CNode(hSocket, addrConnect, pszDest ? pszDest : "", false);
        pNode->AddRef();

        {
            LOCK(cs_vNodes);
            vNodes.push_back(pNode);
        }

        pNode->nTimeConnected = GetTime();
        return pNode;
    } else {
        return nullptr;
    }
}

void CNode::CloseSocketDisconnect() {
    fDisconnect = true;
    if (hSocket != INVALID_SOCKET) {
        LogPrint("net", "disconnecting node %s\n", addrName);
        closesocket(hSocket);
        hSocket = INVALID_SOCKET;
    }

    // in case this fails, we'll empty the recv buffer when the CNode is deleted
    TRY_LOCK(cs_vRecvMsg, lockRecv);
    if (lockRecv) vRecvMsg.clear();

    // if this was the sync node, we'll need a new one
    if (this == pnodeSync)
        pnodeSync = nullptr;
}

void CNode::Cleanup() {}

void CNode::PushVersion() {
    int32_t nBestHeight = g_node_signals.GetHeight().get_value_or(0);

#ifdef WIN32
    string os("windows");
#else
    string os("linux");
#endif
    vector<string> comments;
    comments.push_back(os);
    /// when NTP implemented, change to just nTime = GetAdjustedTime()
    int64_t nTime    = (fInbound ? GetAdjustedTime() : GetTime());
    CAddress addrYou = (addr.IsRoutable() && !IsProxy(addr) ? addr : CAddress(CService("0.0.0.0", 0)));
    CAddress addrMe  = GetLocalAddress(&addr);
    RAND_bytes((uint8_t*)&nLocalHostNonce, sizeof(nLocalHostNonce));
    LogPrint("net", "send version message: version %d, blocks=%d, us=%s, them=%s, peer=%s\n", PROTOCOL_VERSION,
             nBestHeight, addrMe.ToString(), addrYou.ToString(), addr.ToString());

    PushMessage("version", PROTOCOL_VERSION, nLocalServices, nTime, addrYou, addrMe, nLocalHostNonce,
                FormatSubVersion(CLIENT_NAME, CLIENT_VERSION, comments), nBestHeight, true);
}

map<CNetAddr, int64_t> CNode::setBanned;
CCriticalSection CNode::cs_setBanned;

void CNode::ClearBanned() { setBanned.clear(); }

bool CNode::IsBanned(CNetAddr ip) {
    bool fResult = false;
    {
        LOCK(cs_setBanned);
        map<CNetAddr, int64_t>::iterator i = setBanned.find(ip);
        if (i != setBanned.end()) {
            int64_t t = (*i).second;
            if (GetTime() < t)
                fResult = true;
        }
    }
    return fResult;
}

bool CNode::Ban(const CNetAddr& addr) {
    int64_t banTime = GetTime() + SysCfg().GetArg("-bantime", 60 * 60 * 24);  // Default 24-hour ban
    {
        LOCK(cs_setBanned);
        if (setBanned[addr] < banTime)
            setBanned[addr] = banTime;
    }
    return true;
}

#undef X
#define X(name) stats.name = name
void CNode::copyStats(CNodeStats& stats) {
    stats.nodeid = this->GetId();
    X(nServices);
    X(nLastSend);
    X(nLastRecv);
    X(nTimeConnected);
    X(addrName);
    X(nVersion);
    X(cleanSubVer);
    X(fInbound);
    X(nStartingHeight);
    X(nSendBytes);
    X(nRecvBytes);
    stats.fSyncNode = (this == pnodeSync);

    // It is common for nodes with good ping times to suddenly become lagged,
    // due to a new block arriving or other large transfer.
    // Merely reporting pingtime might fool the caller into thinking the node was still responsive,
    // since pingtime does not update until the ping is complete, which might take a while.
    // So, if a ping is taking an unusually long time in flight,
    // the caller can immediately detect that this is happening.
    int64_t nPingUsecWait = 0;
    if ((0 != nPingNonceSent) && (0 != nPingUsecStart)) {
        nPingUsecWait = GetTimeMicros() - nPingUsecStart;
    }

    // Raw ping time is in microseconds, but show it to user as whole seconds (Coin users should be well used to small
    // numbers with many decimal places by now :)
    stats.dPingTime = (((double)nPingUsecTime) / 1e6);
    stats.dPingWait = (((double)nPingUsecWait) / 1e6);

    // Leave string empty if addrLocal invalid (not filled in yet)
    stats.addrLocal = addrLocal.IsValid() ? addrLocal.ToString() : "";
}
#undef X

// requires LOCK(cs_vRecvMsg)
bool CNode::ReceiveMsgBytes(const char* pch, uint32_t nBytes) {
    while (nBytes > 0) {
        // get current incomplete message, or create a new one
        if (vRecvMsg.empty() || vRecvMsg.back().complete()) vRecvMsg.push_back(CNetMessage(SER_NETWORK, nRecvVersion));

        CNetMessage& msg = vRecvMsg.back();

        // absorb network data
        int32_t handled;
        if (!msg.in_data)
            handled = msg.readHeader(pch, nBytes);
        else
            handled = msg.readData(pch, nBytes);

        if (handled < 0)
            return false;

        pch += handled;
        nBytes -= handled;
    }

    return true;
}

int32_t CNetMessage::readHeader(const char* pch, uint32_t nBytes) {
    // copy data to temporary parsing buffer
    uint32_t nRemaining = 24 - nHdrPos;
    uint32_t nCopy      = min(nRemaining, nBytes);

    memcpy(&hdrbuf[nHdrPos], pch, nCopy);
    nHdrPos += nCopy;

    // if header incomplete, exit
    if (nHdrPos < 24)
        return nCopy;

    // deserialize to CMessageHeader
    try {
        hdrbuf >> hdr;
    } catch (std::exception& e) {
        return -1;
    }

    // reject messages larger than MAX_SIZE
    if (hdr.nMessageSize > MAX_SIZE)
        return -1;

    // switch state to reading message data
    in_data = true;
    vRecv.resize(hdr.nMessageSize);

    return nCopy;
}

int32_t CNetMessage::readData(const char* pch, uint32_t nBytes) {
    uint32_t nRemaining = hdr.nMessageSize - nDataPos;
    uint32_t nCopy      = min(nRemaining, nBytes);

    memcpy(&vRecv[nDataPos], pch, nCopy);
    nDataPos += nCopy;

    return nCopy;
}

// requires LOCK(cs_vSend)
void SocketSendData(CNode* pNode) {
    deque<CSerializeData>::iterator it = pNode->vSendMsg.begin();

    while (it != pNode->vSendMsg.end()) {
        const CSerializeData& data = *it;
        assert(data.size() > pNode->nSendOffset);
        int32_t nBytes = send(pNode->hSocket, &data[pNode->nSendOffset], data.size() - pNode->nSendOffset,
                          MSG_NOSIGNAL | MSG_DONTWAIT);
        if (nBytes > 0) {
            pNode->nLastSend = GetTime();
            pNode->nSendBytes += nBytes;
            pNode->nSendOffset += nBytes;
            pNode->RecordBytesSent(nBytes);
            if (pNode->nSendOffset == data.size()) {
                pNode->nSendOffset = 0;
                pNode->nSendSize -= data.size();
                it++;
            } else {
                // could not send full message; stop sending more
                break;
            }
        } else {
            if (nBytes < 0) {
                // error
                int32_t nErr = WSAGetLastError();
                if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS) {
                    LogPrint("INFO", "socket send error %s\n", NetworkErrorString(nErr));
                    pNode->CloseSocketDisconnect();
                }
            }
            // couldn't send anything at all
            break;
        }
    }

    if (it == pNode->vSendMsg.end()) {
        assert(pNode->nSendOffset == 0);
        assert(pNode->nSendSize == 0);
    }
    pNode->vSendMsg.erase(pNode->vSendMsg.begin(), it);
}

static list<CNode*> vNodesDisconnected;

void ThreadSocketHandler() {
    uint32_t nPrevNodeCount = 0;
    while (true) {
        //
        // Disconnect nodes
        //
        {
            LOCK(cs_vNodes);
            // Disconnect unused nodes
            vector<CNode*> vNodesCopy = vNodes;
            for (auto pNode : vNodesCopy) {
                if (pNode->fDisconnect || (pNode->GetRefCount() <= 0 && pNode->vRecvMsg.empty() &&
                                           pNode->nSendSize == 0 && pNode->ssSend.empty())) {
                    // remove from vNodes
                    vNodes.erase(remove(vNodes.begin(), vNodes.end(), pNode), vNodes.end());

                    // release outbound grant (if any)
                    pNode->grantOutbound.Release();

                    // close socket and cleanup
                    pNode->CloseSocketDisconnect();
                    pNode->Cleanup();

                    // hold in disconnected pool until all refs are released
                    if (pNode->fNetworkNode || pNode->fInbound)
                        pNode->Release();
                    vNodesDisconnected.push_back(pNode);
                }
            }
        }
        {
            // Delete disconnected nodes
            list<CNode*> vNodesDisconnectedCopy = vNodesDisconnected;
            for (auto pNode : vNodesDisconnectedCopy) {
                // wait until threads are done using it
                if (pNode->GetRefCount() <= 0) {
                    bool fDelete = false;
                    {
                        TRY_LOCK(pNode->cs_vSend, lockSend);
                        if (lockSend) {
                            TRY_LOCK(pNode->cs_vRecvMsg, lockRecv);
                            if (lockRecv) {
                                TRY_LOCK(pNode->cs_inventory, lockInv);
                                if (lockInv)
                                    fDelete = true;
                            }
                        }
                    }
                    if (fDelete) {
                        vNodesDisconnected.remove(pNode);
                        delete pNode;
                    }
                }
            }
        }
        if (vNodes.size() != nPrevNodeCount) {
            nPrevNodeCount = vNodes.size();

            LogPrint("INFO", "Connections number changed, %d -> %d\n", nPrevNodeCount, vNodes.size());
        }

        //
        // Find which sockets have data to receive
        //
        struct timeval timeout;
        timeout.tv_sec  = 0;
        timeout.tv_usec = 50000;  // frequency to poll pNode->vSend

        fd_set fdsetRecv;
        fd_set fdsetSend;
        fd_set fdsetError;
        FD_ZERO(&fdsetRecv);
        FD_ZERO(&fdsetSend);
        FD_ZERO(&fdsetError);
        SOCKET hSocketMax = 0;
        bool have_fds     = false;

        for (auto hListenSocket : vhListenSocket) {
            FD_SET(hListenSocket, &fdsetRecv);
            hSocketMax = max(hSocketMax, hListenSocket);
            have_fds   = true;
        }

        {
            LOCK(cs_vNodes);
            for (auto pNode : vNodes) {
                if (pNode->hSocket == INVALID_SOCKET)
                    continue;

                FD_SET(pNode->hSocket, &fdsetError);
                hSocketMax = max(hSocketMax, pNode->hSocket);
                have_fds   = true;

                // Implement the following logic:
                // * If there is data to send, select() for sending data. As this only
                //   happens when optimistic write failed, we choose to first drain the
                //   write buffer in this case before receiving more. This avoids
                //   needlessly queueing received data, if the remote peer is not themselves
                //   receiving data. This means properly utilizing TCP flow control signalling.
                // * Otherwise, if there is no (complete) message in the receive buffer,
                //   or there is space left in the buffer, select() for receiving data.
                // * (if neither of the above applies, there is certainly one message
                //   in the receiver buffer ready to be processed).
                // Together, that means that at least one of the following is always possible,
                // so we don't deadlock:
                // * We send some data.
                // * We wait for data to be received (and disconnect after timeout).
                // * We process a message in the buffer (message handler thread).
                {
                    TRY_LOCK(pNode->cs_vSend, lockSend);
                    if (lockSend && !pNode->vSendMsg.empty()) {
                        FD_SET(pNode->hSocket, &fdsetSend);
                        continue;
                    }
                }
                {
                    TRY_LOCK(pNode->cs_vRecvMsg, lockRecv);
                    if (lockRecv && (pNode->vRecvMsg.empty() || !pNode->vRecvMsg.front().complete() ||
                                     pNode->GetTotalRecvSize() <= ReceiveFloodSize()))
                        FD_SET(pNode->hSocket, &fdsetRecv);
                }
            }
        }

        int32_t nSelect = select(have_fds ? hSocketMax + 1 : 0, &fdsetRecv, &fdsetSend, &fdsetError, &timeout);
        boost::this_thread::interruption_point();

        if (nSelect == SOCKET_ERROR) {
            if (have_fds) {
                int32_t nErr = WSAGetLastError();
                LogPrint("INFO", "socket select error %s\n", NetworkErrorString(nErr));
                for (uint32_t i = 0; i <= hSocketMax; i++)
                    FD_SET(i, &fdsetRecv);
            }
            FD_ZERO(&fdsetSend);
            FD_ZERO(&fdsetError);
            MilliSleep(timeout.tv_usec / 1000);
        }

        //
        // Accept new connections
        //
        for (auto hListenSocket : vhListenSocket)
            if (hListenSocket != INVALID_SOCKET && FD_ISSET(hListenSocket, &fdsetRecv)) {
                struct sockaddr_storage sockaddr;
                socklen_t len  = sizeof(sockaddr);
                SOCKET hSocket = accept(hListenSocket, (struct sockaddr*)&sockaddr, &len);
                CAddress addr;
                int32_t nInbound = 0;

                if (hSocket != INVALID_SOCKET)
                    if (!addr.SetSockAddr((const struct sockaddr*)&sockaddr))
                        LogPrint("INFO", "Warning: Unknown socket family\n");

                {
                    LOCK(cs_vNodes);
                    for (auto pNode : vNodes)
                        if (pNode->fInbound)
                            nInbound++;
                }

                if (hSocket == INVALID_SOCKET) {
                    int32_t nErr = WSAGetLastError();
                    if (nErr != WSAEWOULDBLOCK)
                        LogPrint("INFO", "socket[%s] error accept failed: %s\n", addr.ToString(), NetworkErrorString(nErr));
                } else if (nInbound >= nMaxConnections - MAX_OUTBOUND_CONNECTIONS) {
                    closesocket(hSocket);
                } else if (CNode::IsBanned(addr)) {
                    LogPrint("INFO", "connection from %s dropped (banned)\n", addr.ToString());
                    closesocket(hSocket);
                } else {
                    LogPrint("net", "accepted connection %s\n", addr.ToString());
                    CNode* pNode = new CNode(hSocket, addr, "", true);
                    pNode->AddRef();
                    {
                        LOCK(cs_vNodes);
                        vNodes.push_back(pNode);
                    }
                }
            }

        //
        // Service each socket
        //
        vector<CNode*> vNodesCopy;
        {
            LOCK(cs_vNodes);
            vNodesCopy = vNodes;
            for (auto pNode : vNodesCopy)
                pNode->AddRef();
        }
        for (auto pNode : vNodesCopy) {
            boost::this_thread::interruption_point();

            //
            // Receive
            //
            if (pNode->hSocket == INVALID_SOCKET)
                continue;
            if (FD_ISSET(pNode->hSocket, &fdsetRecv) || FD_ISSET(pNode->hSocket, &fdsetError)) {
                TRY_LOCK(pNode->cs_vRecvMsg, lockRecv);
                if (lockRecv) {
                    {
                        // typical socket buffer is 8K-64K
                        char pchBuf[0x10000];
                        int32_t nBytes = recv(pNode->hSocket, pchBuf, sizeof(pchBuf), MSG_DONTWAIT);
                        if (nBytes > 0) {
                            if (!pNode->ReceiveMsgBytes(pchBuf, nBytes))
                                pNode->CloseSocketDisconnect();
                            pNode->nLastRecv = GetTime();
                            pNode->nRecvBytes += nBytes;
                            pNode->RecordBytesRecv(nBytes);
                        } else if (nBytes == 0) {
                            // socket closed gracefully
                            if (!pNode->fDisconnect)
                                LogPrint("net", "socket[%s] closed\n", pNode->addr.ToString());
                            pNode->CloseSocketDisconnect();
                        } else if (nBytes < 0) {
                            // error
                            int32_t nErr = WSAGetLastError();
                            if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR &&
                                nErr != WSAEINPROGRESS) {
                                if (!pNode->fDisconnect)
                                    LogPrint("INFO", "socket[%s] recv error %s\n", pNode->addr.ToString(), NetworkErrorString(nErr));
                                pNode->CloseSocketDisconnect();
                            }
                        }
                    }
                }
            }

            //
            // Send
            //
            if (pNode->hSocket == INVALID_SOCKET)
                continue;
            if (FD_ISSET(pNode->hSocket, &fdsetSend)) {
                TRY_LOCK(pNode->cs_vSend, lockSend);
                if (lockSend)
                    SocketSendData(pNode);
            }

            //
            // Inactivity checking
            //
            if (pNode->vSendMsg.empty())
                pNode->nLastSendEmpty = GetTime();

            if (GetTime() - pNode->nTimeConnected > 60) {
                if (pNode->nLastRecv == 0 || pNode->nLastSend == 0) {
                    LogPrint("net", "socket no message in first 60 seconds, %d %d\n", pNode->nLastRecv != 0,
                             pNode->nLastSend != 0);
                    pNode->fDisconnect = true;
                } else if (GetTime() - pNode->nLastSend > 90 * 60 && GetTime() - pNode->nLastSendEmpty > 90 * 60) {
                    LogPrint("INFO", "socket not sending\n");
                    pNode->fDisconnect = true;
                } else if (GetTime() - pNode->nLastRecv > 90 * 60) {
                    LogPrint("INFO", "socket inactivity timeout\n");
                    pNode->fDisconnect = true;
                }
            }
        }

        {
            LOCK(cs_vNodes);
            for (auto pNode : vNodesCopy)
                pNode->Release();
        }
    }
}

#ifdef USE_UPNP
void ThreadMapPort() {
    string port               = strprintf("%u", GetListenPort());
    const char* multicastif   = 0;
    const char* minissdpdpath = 0;
    struct UPNPDev* devlist   = 0;
    char lanaddr[64];

#ifndef UPNPDISCOVER_SUCCESS
    /* miniupnpc 1.5 */
    devlist = upnpDiscover(2000, multicastif, minissdpdpath, 0);
#else
    /* miniupnpc 1.6 */
    int32_t error = 0;
#ifdef MAC_OSX
    devlist   = upnpDiscover(2000, multicastif, minissdpdpath, 0, 0, 0, &error);
#else
    devlist = upnpDiscover(2000, multicastif, minissdpdpath, 0, 0, &error);
#endif
#endif

    struct UPNPUrls urls;
    struct IGDdatas data;
    int32_t r;

    r = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr));
    if (r == 1) {
        if (fDiscover) {
            char externalIPAddress[40];
            r = UPNP_GetExternalIPAddress(urls.controlURL, data.first.servicetype, externalIPAddress);
            if (r != UPNPCOMMAND_SUCCESS) {
                LogPrint("INFO", "UPnP: GetExternalIPAddress() returned %d\n", r);
            } else {
                if (externalIPAddress[0]) {
                    LogPrint("INFO", "UPnP: ExternalIPAddress = %s\n", externalIPAddress);
                    AddLocal(CNetAddr(externalIPAddress), LOCAL_UPNP);
                } else {
                    LogPrint("INFO", "UPnP: GetExternalIPAddress failed.\n");
                }
            }
        }

        string strDesc = "Coin " + FormatFullVersion();

        try {
            while (true) {
#ifndef UPNPDISCOVER_SUCCESS
                /* miniupnpc 1.5 */
                r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype, port.c_str(), port.c_str(), lanaddr,
                                        strDesc.c_str(), "TCP", 0);
#else
                /* miniupnpc 1.6 */
                r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype, port.c_str(), port.c_str(), lanaddr,
                                        strDesc.c_str(), "TCP", 0, "0");
#endif

                if (r != UPNPCOMMAND_SUCCESS) {
                    LogPrint("INFO", "AddPortMapping(%s, %s, %s) failed with code %d (%s)\n", port, port, lanaddr, r,
                             strupnperror(r));
                } else {
                    LogPrint("INFO", "UPnP Port Mapping successful.\n");
                }

                MilliSleep(20 * 60 * 1000);  // Refresh every 20 minutes
            }
        } catch (boost::thread_interrupted) {
            r = UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, port.c_str(), "TCP", 0);
            LogPrint("INFO", "UPNP_DeletePortMapping() returned : %d\n", r);
            freeUPNPDevlist(devlist);
            devlist = 0;
            FreeUPNPUrls(&urls);
            throw;
        }
    } else {
        LogPrint("INFO", "No valid UPnP IGDs found\n");
        freeUPNPDevlist(devlist);
        devlist = 0;
        if (r != 0)
            FreeUPNPUrls(&urls);
    }
}

void MapPort(bool fUseUPnP) {
    static boost::thread* upnp_thread = nullptr;

    if (fUseUPnP) {
        if (upnp_thread) {
            upnp_thread->interrupt();
            upnp_thread->join();
            delete upnp_thread;
        }
        upnp_thread = new boost::thread(boost::bind(&TraceThread<void (*)()>, "upnp", &ThreadMapPort));
    } else if (upnp_thread) {
        upnp_thread->interrupt();
        upnp_thread->join();
        delete upnp_thread;
        upnp_thread = nullptr;
    }
}

#else
void MapPort(bool) {
    // Intentionally left blank.
}
#endif

void ThreadDNSAddressSeed() {
    // goal: only query DNS seeds if address need is acute
    if ((addrman.size() > 0) && (!SysCfg().GetBoolArg("-forcednsseed", false))) {
        MilliSleep(11 * 1000);

        LOCK(cs_vNodes);
        if (vNodes.size() >= 2) {
            LogPrint("INFO", "P2P peers available. Skipped DNS seeding.\n");
            return;
        }
    }

    const vector<CDNSSeedData>& vSeeds = SysCfg().DNSSeeds();
    int32_t found                          = 0;

    LogPrint("INFO", "Loading addresses from DNS seeds (could take a while)\n");

    for (const auto& seed : vSeeds) {
        if (HaveNameProxy()) {
            AddOneShot(seed.host);
        } else {
            vector<CNetAddr> vIPs;
            vector<CAddress> vAdd;
            if (LookupHost(seed.host.c_str(), vIPs)) {
                for (auto& ip : vIPs) {
                    int32_t nOneDay   = 24 * 3600;
                    CAddress addr = CAddress(CService(ip, SysCfg().GetDefaultPort()));
                    addr.nTime =
                        GetTime() - 3 * nOneDay - GetRand(4 * nOneDay);  // use a random age between 3 and 7 days old
                    vAdd.push_back(addr);
                    found++;
                }
            }
            addrman.Add(vAdd, CNetAddr(seed.name, true));
        }
    }

    LogPrint("INFO", "%d addresses found from DNS seeds\n", found);
}

void DumpAddresses() {
    int64_t nStart = GetTimeMillis();

    CAddrDB adb;
    adb.Write(addrman);

    LogPrint("net", "Flushed %d addresses to peers.dat  %dms\n", addrman.size(), GetTimeMillis() - nStart);
}

void static ProcessOneShot() {
    string strDest;
    {
        LOCK(cs_vOneShots);
        if (vOneShots.empty())
            return;
        strDest = vOneShots.front();
        vOneShots.pop_front();
    }
    CAddress addr;
    CSemaphoreGrant grant(*semOutbound, true);
    if (grant) {
        if (!OpenNetworkConnection(addr, &grant, strDest.c_str(), true))
            AddOneShot(strDest);
    }
}

void ThreadOpenConnections() {
    // Connect to specific addresses
    if (SysCfg().IsArgCount("-connect") && SysCfg().GetMultiArgs("-connect").size() > 0) {
        for (int64_t nLoop = 0;; nLoop++) {
            ProcessOneShot();
            vector<string> tmp = SysCfg().GetMultiArgs("-connect");
            for (auto strAddr : tmp) {
                CAddress addr;
                OpenNetworkConnection(addr, nullptr, strAddr.c_str());
                for (int32_t i = 0; i < 10 && i < nLoop; i++) {
                    MilliSleep(500);
                }
            }
            MilliSleep(500);
        }
    }

    // Initiate network connections
    int64_t nStart = GetTime();
    while (true) {
        ProcessOneShot();

        MilliSleep(500);

        CSemaphoreGrant grant(*semOutbound);
        boost::this_thread::interruption_point();

        // Add seed nodes if DNS seeds are all down (an infrastructure attack?).
        if (addrman.size() == 0 && (GetTime() - nStart > 60)) {
            static bool done = false;
            if (!done) {
                LogPrint("INFO", "Adding fixed seed nodes as DNS doesn't seem to be available.\n");
                addrman.Add(SysCfg().FixedSeeds(), CNetAddr("127.0.0.1"));
                done = true;
            }
        }

        //
        // Choose an address to connect to based on most recently seen
        //
        CAddress addrConnect;

        // Only connect out to one peer per network group (/16 for IPv4).
        // Do this here so we don't have to critsect vNodes inside mapAddresses critsect.
        int32_t nOutbound = 0;
        set<vector<uint8_t> > setConnected;
        {
            LOCK(cs_vNodes);
            for (auto pNode : vNodes) {
                if (!pNode->fInbound) {
                    setConnected.insert(pNode->addr.GetGroup());
                    nOutbound++;
                }
            }
        }

        int64_t nANow = GetAdjustedTime();

        int32_t nTries = 0;
        while (true) {
            // use an nUnkBias between 10 (no outgoing connections) and 90 (8 outgoing connections)
            CAddress addr = addrman.Select(10 + min(nOutbound, 8) * 10);

            // if we selected an invalid address, restart
            if (!addr.IsValid() || (setConnected.count(addr.GetGroup()) && !SysCfg().IsInFixedSeeds(addr)) ||
                IsLocal(addr))
                break;

            // If we didn't find an appropriate destination after trying 100 addresses fetched from addrman,
            // stop this loop, and let the outer loop run again (which sleeps, adds seed nodes, recalculates
            // already-connected network ranges, ...) before trying new addrman addresses.
            nTries++;
            if (nTries > 100)
                break;

            if (IsLimited(addr))
                continue;

            // only consider very recently tried nodes after 30 failed attempts
            if (nANow - addr.nLastTry < 600 && nTries < 30)
                continue;

            // do not allow non-default ports, unless after 50 invalid addresses selected already
            if (addr.GetPort() != SysCfg().GetDefaultPort() && nTries < 50)
                continue;

            addrConnect = addr;
            break;
        }

        if (addrConnect.IsValid())
            OpenNetworkConnection(addrConnect, &grant);
    }
}

void ThreadOpenAddedConnections() {
    {
        LOCK(cs_vAddedNodes);
        vAddedNodes = SysCfg().GetMultiArgs("-addnode");
    }

    if (HaveNameProxy()) {
        while (true) {
            list<string> lAddresses(0);
            {
                LOCK(cs_vAddedNodes);
                for (auto& strAddNode : vAddedNodes)
                    lAddresses.push_back(strAddNode);
            }
            for (auto& strAddNode : lAddresses) {
                CAddress addr;
                CSemaphoreGrant grant(*semOutbound);
                OpenNetworkConnection(addr, &grant, strAddNode.c_str());
                MilliSleep(500);
            }
            MilliSleep(120000);  // Retry every 2 minutes
        }
    }

    for (uint32_t i = 0; true; i++) {
        list<string> lAddresses(0);
        {
            LOCK(cs_vAddedNodes);
            for (auto& strAddNode : vAddedNodes)
                lAddresses.push_back(strAddNode);
        }

        list<vector<CService> > lservAddressesToAdd(0);
        for (auto& strAddNode : lAddresses) {
            vector<CService> vservNode(0);
            if (Lookup(strAddNode.c_str(), vservNode, SysCfg().GetDefaultPort(), fNameLookup, 0)) {
                lservAddressesToAdd.push_back(vservNode);
                {
                    LOCK(cs_setservAddNodeAddresses);
                    for (auto& serv : vservNode)
                        setservAddNodeAddresses.insert(serv);
                }
            }
        }
        // Attempt to connect to each IP for each addnode entry until at least one is successful per addnode entry
        // (keeping in mind that addnode entries can have many IPs if fNameLookup)
        {
            LOCK(cs_vNodes);
            for (auto pNode : vNodes)
                for (list<vector<CService> >::iterator it = lservAddressesToAdd.begin();
                     it != lservAddressesToAdd.end(); it++)
                    for (auto& addrNode : *(it))
                        if (pNode->addr == addrNode) {
                            it = lservAddressesToAdd.erase(it);
                            it--;
                            break;
                        }
        }
        for (auto& vserv : lservAddressesToAdd) {
            CSemaphoreGrant grant(*semOutbound);
            OpenNetworkConnection(CAddress(vserv[i % vserv.size()]), &grant);
            MilliSleep(500);
        }
        MilliSleep(120000);  // Retry every 2 minutes
    }
}

// if successful, this moves the passed grant to the constructed node
bool OpenNetworkConnection(const CAddress& addrConnect, CSemaphoreGrant* grantOutbound, const char* strDest,
                           bool fOneShot) {
    //
    // Initiate outbound network connection
    //
    boost::this_thread::interruption_point();
    if (!strDest)
        if (IsLocal(addrConnect) || FindNode((CNetAddr)addrConnect) || CNode::IsBanned(addrConnect) ||
            FindNode(addrConnect.ToStringIPPort().c_str()))
            return false;
    if (strDest && FindNode(strDest))
        return false;

    CNode* pNode = ConnectNode(addrConnect, strDest);
    boost::this_thread::interruption_point();

    if (!pNode)
        return false;
    if (grantOutbound)
        grantOutbound->MoveTo(pNode->grantOutbound);
    pNode->fNetworkNode = true;
    if (fOneShot)
        pNode->fOneShot = true;

    return true;
}

// for now, use a very simple selection metric: the node from which we received
// most recently
static int64_t NodeSyncScore(const CNode* pNode) { return pNode->nLastRecv; }

void static StartSync(const vector<CNode*>& vNodes) {
    CNode* pnodeNewSync = nullptr;
    int64_t nBestScore  = 0;

    int32_t nBestHeight = g_node_signals.GetHeight().get_value_or(0);

    // Iterate over all nodes
    for (auto pNode : vNodes) {
        // check preconditions for allowing a sync
        if (!pNode->fClient && !pNode->fOneShot && !pNode->fDisconnect && pNode->fSuccessfullyConnected &&
            (pNode->nStartingHeight >
             (nBestHeight -
              144)) /*&& (pNode->nVersion < NOBLKS_VERSION_START || pNode->nVersion >= NOBLKS_VERSION_END)*/
        ) {
            // if ok, compare node's score with the best so far
            int64_t nScore = NodeSyncScore(pNode);
            if (pnodeNewSync == nullptr || nScore > nBestScore) {
                pnodeNewSync = pNode;
                nBestScore   = nScore;
            }
        }
    }
    // if a new sync candidate was found, start sync!
    if (pnodeNewSync) {
        pnodeNewSync->fStartSync = true;
        pnodeSync                = pnodeNewSync;
    }
}

void ThreadMessageHandler() {
    SetThreadPriority(THREAD_PRIORITY_BELOW_NORMAL);
    while (true) {
        bool fHaveSyncNode = false;

        vector<CNode*> vNodesCopy;
        {
            LOCK(cs_vNodes);
            vNodesCopy = vNodes;
            for (auto pNode : vNodesCopy) {
                pNode->AddRef();
                if (pNode == pnodeSync)
                    fHaveSyncNode = true;
            }
        }

        if (!fHaveSyncNode)
            StartSync(vNodesCopy);

        // Poll the connected nodes for messages
        CNode* pnodeTrickle = nullptr;
        if (!vNodesCopy.empty())
            pnodeTrickle = vNodesCopy[GetRand(vNodesCopy.size())];

        bool fSleep = true;

        for (auto pNode : vNodesCopy) {
            if (pNode->fDisconnect)
                continue;

            // Receive messages
            {
                TRY_LOCK(pNode->cs_vRecvMsg, lockRecv);
                if (lockRecv) {
                    if (!g_node_signals.ProcessMessages(pNode))
                        pNode->CloseSocketDisconnect();

                    if (pNode->nSendSize < SendBufferSize()) {
                        if (!pNode->vRecvGetData.empty() ||
                            (!pNode->vRecvMsg.empty() && pNode->vRecvMsg[0].complete())) {
                            fSleep = false;
                        }
                    }
                }
            }
            boost::this_thread::interruption_point();

            // Send messages
            {
                TRY_LOCK(pNode->cs_vSend, lockSend);
                if (lockSend)
                    g_node_signals.SendMessages(pNode, pNode == pnodeTrickle);
            }

            boost::this_thread::interruption_point();
        }

        {
            LOCK(cs_vNodes);
            for (auto pNode : vNodesCopy)
                pNode->Release();
        }

        if (fSleep)
            MilliSleep(100);
    }
}

bool BindListenPort(const CService& addrBind, string& strError) {
    strError = "";
    int32_t nOne = 1;

    // Create socket for listening for incoming connections
    struct sockaddr_storage sockaddr;
    socklen_t len = sizeof(sockaddr);
    if (!addrBind.GetSockAddr((struct sockaddr*)&sockaddr, &len)) {
        strError = strprintf("Error: bind address family for %s not supported", addrBind.ToString());
        LogPrint("INFO", "%s\n", strError);
        return false;
    }

    SOCKET hListenSocket = socket(((struct sockaddr*)&sockaddr)->sa_family, SOCK_STREAM, IPPROTO_TCP);
    if (hListenSocket == INVALID_SOCKET) {
        strError = strprintf("Error: Couldn't open socket for incoming connections (socket returned error %s)",
                             NetworkErrorString(WSAGetLastError()));
        LogPrint("INFO", "%s\n", strError);
        return false;
    }

#ifdef SO_NOSIGPIPE
    // Different way of disabling SIGPIPE on BSD
    setsockopt(hListenSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&nOne, sizeof(int32_t));
#endif

#ifndef WIN32
    // Allow binding if the port is still in TIME_WAIT state after
    // the program was closed and restarted.  Not an issue on windows.
    setsockopt(hListenSocket, SOL_SOCKET, SO_REUSEADDR, (void*)&nOne, sizeof(int32_t));
#endif

#ifdef WIN32
    // Set to non-blocking, incoming connections will also inherit this
    if (ioctlsocket(hListenSocket, FIONBIO, (u_long*)&nOne) == SOCKET_ERROR)
#else
    if (fcntl(hListenSocket, F_SETFL, O_NONBLOCK) == SOCKET_ERROR)
#endif
    {
        strError = strprintf("Error: Couldn't set properties on socket for incoming connections (error %s)",
                             NetworkErrorString(WSAGetLastError()));
        LogPrint("INFO", "%s\n", strError);
        return false;
    }

    // some systems don't have IPV6_V6ONLY but are always v6only; others do have the option
    // and enable it by default or not. Try to enable it, if possible.
    if (addrBind.IsIPv6()) {
#ifdef IPV6_V6ONLY
#ifdef WIN32
        setsockopt(hListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&nOne, sizeof(int32_t));
#else
        setsockopt(hListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&nOne, sizeof(int32_t));
#endif
#endif
#ifdef WIN32
        int32_t nProtLevel   = 10 /* PROTECTION_LEVEL_UNRESTRICTED */;
        int32_t nParameterId = 23 /* IPV6_PROTECTION_LEVEl */;
        // this call is allowed to fail
        setsockopt(hListenSocket, IPPROTO_IPV6, nParameterId, (const char*)&nProtLevel, sizeof(int32_t));
#endif
    }

    if (::bind(hListenSocket, (struct sockaddr*)&sockaddr, len) == SOCKET_ERROR) {
        int32_t nErr = WSAGetLastError();
        if (nErr == WSAEADDRINUSE)
            strError = strprintf(_("Unable to bind to %s on this computer. Coin Core is probably already running."),
                                 addrBind.ToString());
        else
            strError = strprintf(_("Unable to bind to %s on this computer (bind returned error %s)"),
                                 addrBind.ToString(), NetworkErrorString(nErr));
        LogPrint("INFO", "%s\n", strError);
        return false;
    }
    LogPrint("INFO", "Bound to %s\n", addrBind.ToString());

    // Listen for incoming connections
    if (listen(hListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        strError = strprintf(_("Error: Listening for incoming connections failed (listen returned error %s)"),
                             NetworkErrorString(WSAGetLastError()));
        LogPrint("INFO", "%s\n", strError);
        return false;
    }

    vhListenSocket.push_back(hListenSocket);

    if (addrBind.IsRoutable() && fDiscover)
        AddLocal(addrBind, LOCAL_BIND);

    return true;
}

void static Discover(boost::thread_group& threadGroup) {
    if (!fDiscover)
        return;

#ifdef WIN32
    // Get local host IP
    char pszHostName[1000] = "";
    if (gethostname(pszHostName, sizeof(pszHostName)) != SOCKET_ERROR) {
        vector<CNetAddr> vaddr;
        if (LookupHost(pszHostName, vaddr)) {
            for (const auto& addr : vaddr) {
                AddLocal(addr, LOCAL_IF);
            }
        }
    }
#else
    // Get local host ip
    struct ifaddrs* myaddrs;
    if (getifaddrs(&myaddrs) == 0) {
        for (struct ifaddrs* ifa = myaddrs; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == nullptr)
                continue;
            if ((ifa->ifa_flags & IFF_UP) == 0)
                continue;
            if (strcmp(ifa->ifa_name, "lo") == 0)
                continue;
            if (strcmp(ifa->ifa_name, "lo0") == 0)
                continue;
            if (ifa->ifa_addr->sa_family == AF_INET) {
                struct sockaddr_in* s4 = (struct sockaddr_in*)(ifa->ifa_addr);
                CNetAddr addr(s4->sin_addr);
                if (AddLocal(addr, LOCAL_IF))
                    LogPrint("INFO", "IPv4 %s: %s\n", ifa->ifa_name, addr.ToString());
            } else if (ifa->ifa_addr->sa_family == AF_INET6) {
                struct sockaddr_in6* s6 = (struct sockaddr_in6*)(ifa->ifa_addr);
                CNetAddr addr(s6->sin6_addr);
                if (AddLocal(addr, LOCAL_IF))
                    LogPrint("INFO", "IPv6 %s: %s\n", ifa->ifa_name, addr.ToString());
            }
        }
        freeifaddrs(myaddrs);
    }
#endif

    // Don't use external IPv4 discovery, when -onlynet="IPv6"
    if (!IsLimited(NET_IPV4))
        threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "ext-ip", &ThreadGetMyExternalIP));
}

void StartNode(boost::thread_group& threadGroup) {
    if (semOutbound == nullptr) {
        // initialize semaphore
        int32_t nMaxOutbound = min(MAX_OUTBOUND_CONNECTIONS, nMaxConnections);
        semOutbound      = new CSemaphore(nMaxOutbound);
    }

    if (pnodeLocalHost == nullptr)
        pnodeLocalHost = new CNode(INVALID_SOCKET, CAddress(CService("127.0.0.1", 0), nLocalServices));

    Discover(threadGroup);

    //
    // Start threads
    //

    if (!SysCfg().GetBoolArg("-dnsseed", true)) {
        LogPrint("INFO", "DNS seeding disabled\n");
    } else {
        threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "dnsseed", &ThreadDNSAddressSeed));
    }

#ifdef USE_UPNP
    // Map ports with UPnP
    MapPort(SysCfg().GetBoolArg("-upnp", USE_UPNP));
#endif

    // Send and receive from sockets, accept connections
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "net", &ThreadSocketHandler));

    // Initiate outbound connections from -addnode
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "addcon", &ThreadOpenAddedConnections));

    // Initiate outbound connections
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "opencon", &ThreadOpenConnections));

    // Process messages
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "msghand", &ThreadMessageHandler));

    // Dump network addresses
    threadGroup.create_thread(boost::bind(&LoopForever<void (*)()>, "dumpaddr", &DumpAddresses, DUMP_ADDRESSES_INTERVAL * 1000));

    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "post-ip", &ThreadPostNodeInfo));
}

bool StopNode() {
    LogPrint("INFO", "StopNode()\n");
    MapPort(false);
    if (semOutbound)
        for (int32_t i = 0; i < MAX_OUTBOUND_CONNECTIONS; i++)
            semOutbound->post();
    MilliSleep(50);
    DumpAddresses();

    return true;
}

class CNetCleanup {
public:
    CNetCleanup() {}
    ~CNetCleanup() {
        // Close sockets
        for (auto pNode : vNodes)
            if (pNode->hSocket != INVALID_SOCKET)
                closesocket(pNode->hSocket);
        for (auto hListenSocket : vhListenSocket)
            if (hListenSocket != INVALID_SOCKET)
                if (closesocket(hListenSocket) == SOCKET_ERROR)
                    LogPrint("INFO", "closesocket(hListenSocket) failed with error %s\n",
                             NetworkErrorString(WSAGetLastError()));

        // clean up some globals (to help leak detection)
        for (auto pNode : vNodes)
            delete pNode;
        for (auto pNode : vNodesDisconnected)
            delete pNode;
        vNodes.clear();
        vNodesDisconnected.clear();
        delete semOutbound;
        semOutbound = nullptr;
        delete pnodeLocalHost;
        pnodeLocalHost = nullptr;

#ifdef WIN32
        // Shutdown Windows Sockets
        WSACleanup();
#endif
    }
}

instance_of_cnetcleanup;

void RelayTransaction(CBaseTx* pBaseTx, const uint256& hash) {
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss.reserve(1000);
    auto pTx = pBaseTx->GetNewInstance();
    ss << pTx;
    RelayTransaction(pBaseTx, hash, ss);
}

void RelayTransaction(CBaseTx* pBaseTx, const uint256& hash, const CDataStream& ss) {
    CInv inv(MSG_TX, hash);
    {
        LOCK(cs_mapRelay);
        // Expire old relay messages
        while (!vRelayExpiration.empty() && vRelayExpiration.front().first < GetTime()) {
            mapRelay.erase(vRelayExpiration.front().second);
            vRelayExpiration.pop_front();
        }

        // Save original serialized message so newer versions are preserved
        mapRelay.insert(make_pair(inv, ss));
        vRelayExpiration.push_back(make_pair(GetTime() + 15 * 60, inv));
    }
    LOCK(cs_vNodes);
    for (auto pNode : vNodes) {
        if (!pNode->fRelayTxes)
            continue;
        LOCK(pNode->cs_filter);
        if (pNode->pFilter) {
            if (pNode->pFilter->IsRelevantAndUpdate(pBaseTx, hash)) {
                pNode->PushInventory(inv);
                LogPrint("sendtx", "hash:%s time:%ld\n", inv.hash.GetHex(), GetTime());
            }
        } else {
            pNode->PushInventory(inv);
            LogPrint("sendtx", "hash:%s time:%ld\n", inv.hash.GetHex(), GetTime());
        }
    }
}

void CNode::RecordBytesRecv(uint64_t bytes) {
    LOCK(cs_totalBytesRecv);
    nTotalBytesRecv += bytes;
}

void CNode::RecordBytesSent(uint64_t bytes) {
    LOCK(cs_totalBytesSent);
    nTotalBytesSent += bytes;
}

uint64_t CNode::GetTotalBytesRecv() {
    LOCK(cs_totalBytesRecv);
    return nTotalBytesRecv;
}

uint64_t CNode::GetTotalBytesSent() {
    LOCK(cs_totalBytesSent);
    return nTotalBytesSent;
}

void CNode::Fuzz(int32_t nChance) {
    if (!fSuccessfullyConnected)
        return;  // Don't fuzz initial handshake
    if (GetRand(nChance) != 0)
        return;    // Fuzz 1 of every nChance messages

    switch (GetRand(3)) {
        case 0:
            // xor a random byte with a random value:
            if (!ssSend.empty()) {
                CDataStream::size_type pos = GetRand(ssSend.size());
                ssSend[pos] ^= (uint8_t)(GetRand(256));
            }
            break;
        case 1:
            // delete a random byte:
            if (!ssSend.empty()) {
                CDataStream::size_type pos = GetRand(ssSend.size());
                ssSend.erase(ssSend.begin() + pos);
            }
            break;
        case 2:
            // insert a random byte at a random position
            {
                CDataStream::size_type pos = GetRand(ssSend.size());
                char ch                    = (char)GetRand(256);
                ssSend.insert(ssSend.begin() + pos, ch);
            }
            break;
    }
    // Chance of more than one change half the time:
    // (more changes exponentially less likely):
    Fuzz(2);
}

//
// CAddrDB
//

CAddrDB::CAddrDB() { pathAddr = GetDataDir() / "peers.dat"; }

bool CAddrDB::Write(const CAddrMan& addr) {
    // Generate random temporary filename
    uint16_t randv = 0;
    RAND_bytes((uint8_t*)&randv, sizeof(randv));
    string tmpfn = strprintf("peers.dat.%04x", randv);

    // serialize addresses, checksum data up to that point, then append csum
    CDataStream ssPeers(SER_DISK, CLIENT_VERSION);
    ssPeers << FLATDATA(SysCfg().MessageStart());
    ssPeers << addr;
    uint256 hash = Hash(ssPeers.begin(), ssPeers.end());
    ssPeers << hash;

    // open temp output file, and associate with CAutoFile
    boost::filesystem::path pathTmp = GetDataDir() / tmpfn;
    FILE* file                      = fopen(pathTmp.string().c_str(), "wb");
    CAutoFile fileout               = CAutoFile(file, SER_DISK, CLIENT_VERSION);
    if (!fileout)
        return ERRORMSG("%s : Failed to open file %s", __func__, pathTmp.string());

    // Write and commit header, data
    try {
        fileout << ssPeers;
    } catch (std::exception& e) {
        return ERRORMSG("%s : Serialize or I/O error - %s", __func__, e.what());
    }
    FileCommit(fileout);
    fileout.fclose();

    // replace existing peers.dat, if any, with new peers.dat.XXXX
    if (!RenameOver(pathTmp, pathAddr))
        return ERRORMSG("%s : Rename-into-place failed", __func__);

    return true;
}

bool CAddrDB::Read(CAddrMan& addr) {
    // open input file, and associate with CAutoFile
    FILE* file       = fopen(pathAddr.string().c_str(), "rb");
    CAutoFile filein = CAutoFile(file, SER_DISK, CLIENT_VERSION);
    if (!filein)
        return ERRORMSG("%s : Failed to open file %s", __func__, pathAddr.string());

    // use file size to size memory buffer
    int32_t fileSize = boost::filesystem::file_size(pathAddr);
    int32_t dataSize = fileSize - sizeof(uint256);
    // Don't try to resize to a negative number if file is small
    if (dataSize < 0)
        dataSize = 0;
    vector<uint8_t> vchData;
    vchData.resize(dataSize);
    uint256 hashIn;

    // read data and checksum from file
    try {
        filein.read((char*)&vchData[0], dataSize);
        filein >> hashIn;
    } catch (std::exception& e) {
        return ERRORMSG("%s : Deserialize or I/O error - %s", __func__, e.what());
    }
    filein.fclose();

    CDataStream ssPeers(vchData, SER_DISK, CLIENT_VERSION);

    // verify stored checksum matches input data
    uint256 hashTmp = Hash(ssPeers.begin(), ssPeers.end());
    if (hashIn != hashTmp)
        return ERRORMSG("%s : Checksum mismatch, data corrupted", __func__);

    uint8_t pchMsgTmp[4];
    try {
        // de-serialize file header (network specific magic number) and ..
        ssPeers >> FLATDATA(pchMsgTmp);

        // ... verify the network matches ours
        if (memcmp(pchMsgTmp, SysCfg().MessageStart(), sizeof(pchMsgTmp)))
            return ERRORMSG("%s : Invalid network magic number", __func__);

        // de-serialize address data into one CAddrMan object
        ssPeers >> addr;
    } catch (std::exception& e) {
        return ERRORMSG("%s : Deserialize or I/O error - %s", __func__, e.what());
    }

    return true;
}
