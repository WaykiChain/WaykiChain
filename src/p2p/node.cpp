// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "node.h"
#include "netmessage.h"
#include <openssl/rand.h>

uint64_t CNode::nTotalBytesRecv = 0;
uint64_t CNode::nTotalBytesSent = 0;
CCriticalSection CNode::cs_totalBytesRecv;
CCriticalSection CNode::cs_totalBytesSent;
CCriticalSection cs_nLastNodeId;
uint64_t nLocalServices = NODE_NETWORK;
uint64_t nLocalHostNonce         = 0;
extern map<CNetAddr, LocalServiceInfo> mapLocalHost;
// Map maintaining per-node state. Requires cs_mapNodeState.
map<NodeId, CNodeState> mapNodeState;
CCriticalSection cs_mapNodeState;

NodeId nLastNodeId = 0;
limitedmap<CInv, int64_t> mapAlreadyAskedFor(MAX_INV_SZ);
CNode* pnodeSync = nullptr;


map<uint256, tuple<NodeId, list<QueuedBlock>::iterator, int64_t>> mapBlocksInFlight;  // downloading blocks
map<uint256, tuple<NodeId, list<uint256>::iterator, int64_t>> mapBlocksToDownload;    // blocks to be downloaded

void InitializeNode(NodeId nodeid, const CNode *pNode) {
    LOCK(cs_mapNodeState);
    CNodeState &state = mapNodeState.insert(make_pair(nodeid, CNodeState())).first->second;
    state.name        = pNode->addrName;
}

void FinalizeNode(NodeId nodeid) {
    LOCK(cs_mapNodeState);
    CNodeState *state = State(nodeid);

    for (const auto &entry : state->vBlocksInFlight)
        mapBlocksInFlight.erase(entry.hash);

    for (const auto &hash : state->vBlocksToDownload)
        mapBlocksToDownload.erase(hash);

    mapNodeState.erase(nodeid);
}

// Requires cs_mapNodeState.
CNodeState *State(NodeId pNode) {
    AssertLockHeld(cs_mapNodeState);
    map<NodeId, CNodeState>::iterator it = mapNodeState.find(pNode);
    if (it == mapNodeState.end())
        return nullptr;

    return &it->second;
}

// requires LOCK(cs_vSend)
void CNode::SocketSendData() {
    deque<CSerializeData>::iterator it = vSendMsg.begin();

    while (it != vSendMsg.end()) {
        const CSerializeData& data = *it;
        assert(data.size() > nSendOffset);
        int32_t nBytes = send(hSocket, &data[nSendOffset], data.size() - nSendOffset,
                              MSG_NOSIGNAL | MSG_DONTWAIT);
        if (nBytes > 0) {
            nLastSend = GetTime();
            nSendBytes += nBytes;
            nSendOffset += nBytes;
            RecordBytesSent(nBytes);
            if (nSendOffset == data.size()) {
                nSendOffset = 0;
                nSendSize -= data.size();
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
                    LogPrint(BCLog::INFO, "socket send error %s\n", NetworkErrorString(nErr));
                    CloseSocketDisconnect();
                }
            }
            // couldn't send anything at all
            break;
        }
    }

    if (it == vSendMsg.end()) {
        assert(nSendOffset == 0);
        assert(nSendSize == 0);
    }
    vSendMsg.erase(vSendMsg.begin(), it);
}



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

void CNode::CloseSocketDisconnect() {
    fDisconnect = true;
    if (hSocket != INVALID_SOCKET) {
        LogPrint(BCLog::NET, "disconnecting node %s\n", addrName);
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
    int32_t nBestHeight = GetNodeSignals().GetHeight().get_value_or(0);

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
    LogPrint(BCLog::NET, "send version message: version %d, blocks=%d, us=%s, them=%s, peer=%s\n", PROTOCOL_VERSION,
             nBestHeight, addrMe.ToString(), addrYou.ToString(), addr.ToString());

    PushMessage(NetMsgType::VERSION, PROTOCOL_VERSION, nLocalServices, nTime, addrYou, addrMe, nLocalHostNonce,
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
