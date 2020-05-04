// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef P2P_NODE_H
#define P2P_NODE_H

#include <boost/signals2/signal.hpp>
#include "commons/serialize.h"
#include "sync.h"
#include "commons/compat/compat.h"
#include "p2p/protocol.h"
#include "commons/limitedmap.h"
#include "commons/bloom.h"
#include "commons/mruset.h"
#include "commons/random.h"
#include "p2p/netmessage.h"

class CNode;
struct CNodeSignals;
struct CNodeState;

typedef int32_t NodeId;
extern CCriticalSection cs_nLastNodeId;
extern NodeId nLastNodeId;
extern uint64_t nLocalServices;
extern CCriticalSection cs_mapLocalHost;

extern map<NodeId, CNodeState> mapNodeState;
extern CCriticalSection cs_mapNodeState;
extern CNodeSignals& GetNodeSignals();

/** The maximum number of entries in an 'inv' protocol message */
static const uint32_t MAX_INV_SZ = 50000;
/** The maximum number of entries in mapAskFor */
static const size_t MAPASKFOR_MAX_SZ = MAX_INV_SZ;
/** The maximum number of new addresses to accumulate before announcing. */
static const uint32_t MAX_ADDR_TO_SEND = 1000;

extern limitedmap<CInv, int64_t> mapAlreadyAskedFor;

struct LocalServiceInfo {
    int32_t nScore;
    int32_t nPort;
};

// Signals for message handling
struct CNodeSignals {
    boost::signals2::signal<int32_t()> GetHeight;
    boost::signals2::signal<bool(CNode*)> ProcessMessages;
    boost::signals2::signal<bool(CNode*, bool)> SendMessages;
    boost::signals2::signal<void(NodeId, const CNode*)> InitializeNode;
    boost::signals2::signal<void(NodeId)> FinalizeNode;
};

inline uint32_t SendBufferSize() { return 1000 * SysCfg().GetArg("-maxsendbuffer", 1 * 1000); }



CAddress GetLocalAddress(const CNetAddr* paddrPeer = nullptr);
bool GetLocal(CService& addr, const CNetAddr* paddrPeer = nullptr);

class CNodeStats {
public:
    NodeId nodeid;
    uint64_t nServices;
    int64_t nLastSend;
    int64_t nLastRecv;
    int64_t nTimeConnected;
    string addrName;
    int32_t nVersion;
    string cleanSubVer;
    bool fInbound;
    int32_t nStartingHeight;
    uint64_t nSendBytes;
    uint64_t nRecvBytes;
    bool fSyncNode;
    double dPingTime;
    double dPingWait;
    string addrLocal;
};

struct CBlockReject {
    uint8_t chRejectCode;
    string strRejectReason;
    uint256 blockHash;
};

// Blocks that are in flight, and that are in the queue to be downloaded.
// Protected by cs_main.
struct QueuedBlock {
    uint256 hash;
    int64_t nTime;          // Time of "getdata" request in microseconds.
    int32_t nQueuedBefore;  // Number of blocks in flight at the time of request.
};

// Maintain validation-specific state about nodes, protected by cs_main, instead
// by CNode's own locks. This simplifies asynchronous operation, where
// processing of incoming data is done after the ProcessMessage call returns,
// and we're no longer holding the node's locks.
struct CNodeState {
    // Accumulated misbehaviour score for this peer.
    int32_t nMisbehavior;
    // Whether this peer should be disconnected and banned.
    bool fShouldBan;
    // String name of this peer (debugging/logging purposes).
    string name;
    // List of asynchronously-determined block rejections to notify this peer about.
    vector<CBlockReject> rejects;
    list<QueuedBlock> vBlocksInFlight;
    int32_t nBlocksInFlight;          // maximun blocks downloading at the same time
    list<uint256> vBlocksToDownload;  // blocks to be downloaded
    int32_t nBlocksToDownload;        // blocks number to be downloaded
    int64_t nLastBlockReceive;        // the latest receiving blocks time
    int64_t nLastBlockProcess;        // the latest processing blocks time

    CNodeState() {
        nMisbehavior      = 0;
        fShouldBan        = false;
        nBlocksToDownload = 0;
        nBlocksInFlight   = 0;
        nLastBlockReceive = 0;
        nLastBlockProcess = 0;
    }
};

// Requires cs_mapNodeState.
CNodeState *State(NodeId pNode);

/** Information about a peer */
class CNode {
public:
    // socket
    uint64_t nServices;
    SOCKET hSocket;
    CDataStream ssSend;
    size_t nSendSize;    // total size of all vSendMsg entries
    size_t nSendOffset;  // offset inside the first vSendMsg already sent
    uint64_t nSendBytes;
    deque<CSerializeData> vSendMsg;
    CCriticalSection cs_vSend;

    deque<CInv> vRecvGetData;  // strCommand == "getdata 保存的inv
    deque<CNetMessage> vRecvMsg;
    CCriticalSection cs_vRecvMsg;
    uint64_t nRecvBytes;
    int32_t nRecvVersion;

    int64_t nLastSend;
    int64_t nLastRecv;
    int64_t nLastSendEmpty;
    int64_t nTimeConnected;
    CAddress addr;
    string addrName;
    CService addrLocal;
    int32_t nVersion; //protocol version
    // strSubVer is whatever byte array we read from the wire. However, this field is intended
    // to be printed out, displayed to humans in various forms and so on. So we sanitize it and
    // store the sanitized version in cleanSubVer. The original should be used when dealing with
    // the network or wire types and the cleaned string used when displayed or logged.
    string strSubVer, cleanSubVer;
    bool fOneShot;
    bool fClient;
    bool fInbound;
    bool fNetworkNode;
    bool fSuccessfullyConnected;
    bool fDisconnect;
    // We use fRelayTxes for two purposes -
    // a) it allows us to not relay tx invs before receiving the peer's version message
    // b) the peer may tell us in their version message that we should not relay tx invs
    //    until they have initialized their bloom filter.
    bool fRelayTxes;
    CSemaphoreGrant grantOutbound;
    CCriticalSection cs_filter;
    CBloomFilter* pFilter;
    int32_t nRefCount;
    NodeId id;

protected:
    // Denial-of-service detection/prevention
    // Key is IP address, value is banned-until-time
    static map<CNetAddr, int64_t> setBanned;
    static CCriticalSection cs_setBanned;

    // Basic fuzz-testing
    void Fuzz(int32_t nChance);  // modifies ssSend

public:
    uint256 hashContinue;                   // getblocks the next batch of inventory下一次 盘点的块
    CBlockIndex* pIndexLastGetBlocksBegin;  //上次开始的块  本地节点有的块chainActive.Tip()
    uint256 hashLastGetBlocksEnd;           // 本地节点保存的孤儿块的根块 hash GetOrphanRoot(hash)
    int32_t nStartingHeight;                // Start block sync, current height
    bool fStartSync;

    // flood relay
    vector<CAddress> vAddrToSend;
    mruset<CAddress> setAddrKnown;
    bool fGetAddr;
    set<uint256> setKnown;  // alertHash

    // inventory based relay
    mruset<CInv> setInventoryKnown;  //存放已收到的inv
    vector<CInv> vInventoryToSend;   //待发送的inv
    std::set<CInv> setForceToSend;   //强制发送的inv

    CCriticalSection cs_inventory;
    multimap<int64_t, CInv> mapAskFor;  //向网络请求交易的时间, a priority queue


    mruset<CBlockConfirmMessage> setBlockConfirmMsgKnown;
    CCriticalSection cs_blockConfirm;

    mruset<CBlockFinalityMessage> setBlockFinalityMsgKnown;
    CCriticalSection cs_blockFinality;

    // Ping time measurement
    uint64_t nPingNonceSent;
    int64_t nPingUsecStart;
    int64_t nPingUsecTime;
    bool fPingQueued;

    CNode(SOCKET hSocketIn, CAddress addrIn, string addrNameIn = "", bool fInboundIn = false)
            : ssSend(SER_NETWORK, INIT_PROTO_VERSION), setAddrKnown(5000) {
        nServices                = 0;
        hSocket                  = hSocketIn;
        nRecvVersion             = INIT_PROTO_VERSION;
        nLastSend                = 0;
        nLastRecv                = 0;
        nSendBytes               = 0;
        nRecvBytes               = 0;
        nLastSendEmpty           = GetTime();
        nTimeConnected           = GetTime();
        addr                     = addrIn;
        addrName                 = addrNameIn == "" ? addr.ToStringIPPort() : addrNameIn;
        nVersion                 = 0;
        strSubVer                = "";
        fOneShot                 = false;
        fClient                  = false;  // set by version message
        fInbound                 = fInboundIn;
        fNetworkNode             = false;
        fSuccessfullyConnected   = false;
        fDisconnect              = false;
        nRefCount                = 0;
        nSendSize                = 0;
        nSendOffset              = 0;
        hashContinue             = uint256();
        pIndexLastGetBlocksBegin = 0;
        hashLastGetBlocksEnd     = uint256();
        nStartingHeight          = -1;
        fStartSync               = false;
        fGetAddr                 = false;
        fRelayTxes               = false;
        setInventoryKnown.max_size(SendBufferSize() / 1000);
        setBlockConfirmMsgKnown.max_size(200);
        pFilter        = new CBloomFilter();
        nPingNonceSent = 0;
        nPingUsecStart = 0;
        nPingUsecTime  = 0;
        fPingQueued    = false;

        {
            LOCK(cs_nLastNodeId);
            id = nLastNodeId++;
        }

        // Be shy and don't send version until we hear
        if (hSocket != INVALID_SOCKET && !fInbound)
            PushVersion();

        GetNodeSignals().InitializeNode(GetId(), this);
    }

    ~CNode() {
        if (hSocket != INVALID_SOCKET) {
            closesocket(hSocket);
            hSocket = INVALID_SOCKET;
        }
        if (pFilter)
            delete pFilter;
        GetNodeSignals().FinalizeNode(GetId());
    }

private:
    // Network usage totals
    static CCriticalSection cs_totalBytesRecv;
    static CCriticalSection cs_totalBytesSent;
    static uint64_t nTotalBytesRecv;
    static uint64_t nTotalBytesSent;

    CNode(const CNode&);
    void operator=(const CNode&);

public:
    NodeId GetId() const { return id; }




    // for now, use a very simple selection metric: the node from which we received
    // most recently
    int64_t NodeSyncScore() { return nLastRecv; }

    int32_t GetRefCount() {
        assert(nRefCount >= 0);
        return nRefCount;
    }

    // requires LOCK(cs_vRecvMsg)
    uint32_t GetTotalRecvSize() {
        uint32_t total = 0;
        for (const auto& msg : vRecvMsg)
            total += msg.vRecv.size() + 24;
        return total;
    }

    // requires LOCK(cs_vRecvMsg)
    bool ReceiveMsgBytes(const char* pch, uint32_t nBytes);

    // requires LOCK(cs_vRecvMsg)
    void SetRecvVersion(int32_t nVersionIn) {
        nRecvVersion = nVersionIn;
        for (auto& msg : vRecvMsg)
            msg.SetVersion(nVersionIn);
    }

    CNode* AddRef() {
        nRefCount++;
        return this;
    }

    void Release() { nRefCount--; }

    void AddAddressKnown(const CAddress& addr) { setAddrKnown.insert(addr); }

    void AddBlockConfirmMessageKnown(const CBlockConfirmMessage msg){ setBlockConfirmMsgKnown.insert(msg); }
    void AddBlockFinalityMessageKnown(const CBlockFinalityMessage msg){ setBlockFinalityMsgKnown.insert(msg); }

    void PushAddress(const CAddress& addr) {
        // Known checking here is only to save space from duplicates.
        // SendMessages will filter it again for knowns that were added
        // after addresses were pushed.
        if (addr.IsValid() && !setAddrKnown.count(addr)) {
            if (vAddrToSend.size() >= MAX_ADDR_TO_SEND) {
                vAddrToSend[insecure_rand() % vAddrToSend.size()] = addr;
            } else {
                vAddrToSend.push_back(addr);
            }
        }
    }

    void AddInventoryKnown(const CInv& inv) {
        LOCK(cs_inventory);
        setInventoryKnown.insert(inv);
    }

    void PushInventory(const CInv& inv, bool forced = false) {
        LOCK(cs_inventory);

        if (forced)
            setForceToSend.insert(inv);

        if (forced || !setInventoryKnown.count(inv))
            vInventoryToSend.push_back(inv);
    }

    void PushBlockConfirmMessage(const CBlockConfirmMessage& msg) {
        LOCK(cs_blockConfirm);
        if(!setBlockConfirmMsgKnown.count(msg)){
            PushMessage(NetMsgType::CONFIRMBLOCK, msg);
            setBlockConfirmMsgKnown.insert(msg);
        }
    }

    void PushBlockFinalityMessage(const CBlockFinalityMessage& msg) {
        LOCK(cs_blockFinality);
        if(!setBlockFinalityMsgKnown.count(msg)){
            PushMessage(NetMsgType::FINALITYBLOCK, msg);
            setBlockFinalityMsgKnown.insert(msg);
        }
    }

    void AskFor(const CInv& inv) {
        if (mapAskFor.size() > MAPASKFOR_MAX_SZ) {
            return;
        }

        // We're using mapAskFor as a priority queue,
        // the key is the earliest time the request can be sent
        int64_t nRequestTime;
        limitedmap<CInv, int64_t>::const_iterator it = mapAlreadyAskedFor.find(inv);
        if (it != mapAlreadyAskedFor.end())
            nRequestTime = it->second;
        else
            nRequestTime = 0;
        LogPrint(BCLog::NET, "ask for %s %d (%s)\n", inv.ToString().c_str(), nRequestTime,
                 DateTimeStrFormat("%H:%M:%S", nRequestTime / 1000000).c_str());

        // Make sure not to reuse time indexes to keep things in the same order
        int64_t nNow = GetTimeMicros() - 1000000;
        static int64_t nLastTime;
        ++nLastTime;
        nNow      = max(nNow, nLastTime);
        nLastTime = nNow;

        // Each retry is 2 minutes after the last
        nRequestTime = max(nRequestTime + 2 * 60 * 1000000, nNow);
        if (it != mapAlreadyAskedFor.end())
            mapAlreadyAskedFor.update(it, nRequestTime);
        else
            mapAlreadyAskedFor.insert(make_pair(inv, nRequestTime));
        mapAskFor.insert(make_pair(nRequestTime, inv));
    }

    // TODO: Document the postcondition of this function.  Is cs_vSend locked?
    void BeginMessage(const char* pszCommand) EXCLUSIVE_LOCK_FUNCTION(cs_vSend) {
            ENTER_CRITICAL_SECTION(cs_vSend);
            assert(ssSend.size() == 0);
            ssSend << CMessageHeader(pszCommand, 0);
            LogPrint(BCLog::NET, "sending: %s\n", pszCommand);
    }

    // TODO: Document the precondition of this function.  Is cs_vSend locked?
    void AbortMessage() UNLOCK_FUNCTION(cs_vSend) {
            ssSend.clear();

            LEAVE_CRITICAL_SECTION(cs_vSend);

            LogPrint(BCLog::NET, "(aborted)\n");
    }

    // TODO: Document the precondition of this function.  Is cs_vSend locked?
    void EndMessage() UNLOCK_FUNCTION(cs_vSend) {
            // The -*messagestest options are intentionally not documented in the help message,
            // since they are only used during development to debug the networking code and are
            // not intended for end-users.
            if (SysCfg().IsArgCount("-dropmessagestest") && GetRand(SysCfg().GetArg("-dropmessagestest", 2)) == 0) {
                LogPrint(BCLog::NET, "dropmessages DROPPING SEND MESSAGE\n");
                AbortMessage();
                return;
            }
            if (SysCfg().IsArgCount("-fuzzmessagestest"))
            Fuzz(SysCfg().GetArg("-fuzzmessagestest", 10));

            if (ssSend.size() == 0)
            return;

            // Set the size
            uint32_t nSize = ssSend.size() - CMessageHeader::HEADER_SIZE;
            memcpy((char*)&ssSend[CMessageHeader::MESSAGE_SIZE_OFFSET], &nSize, sizeof(nSize));

            // Set the checksum
            uint256 hash           = Hash(ssSend.begin() + CMessageHeader::HEADER_SIZE, ssSend.end());
            uint32_t nChecksum = 0;
            memcpy(&nChecksum, &hash, sizeof(nChecksum));
            assert(ssSend.size() >= CMessageHeader::CHECKSUM_OFFSET + sizeof(nChecksum));
            memcpy((char*)&ssSend[CMessageHeader::CHECKSUM_OFFSET], &nChecksum, sizeof(nChecksum));

            LogPrint(BCLog::NET, "(%d bytes)\n", nSize);

            deque<CSerializeData>::iterator it = vSendMsg.insert(vSendMsg.end(), CSerializeData());
            ssSend.GetAndClear(*it);
            nSendSize += (*it).size();

            // If write queue empty, attempt "optimistic write"
            if (it == vSendMsg.begin()) SocketSendData();

            LEAVE_CRITICAL_SECTION(cs_vSend);
    }

    void PushVersion();

    void PushMessage(const char* pszCommand) {
        try {
            BeginMessage(pszCommand);
            EndMessage();
        } catch (...) {
            AbortMessage();
            throw;
        }
    }

    template <typename T1>
    void PushMessage(const char* pszCommand, const T1& a1) {
        try {
            BeginMessage(pszCommand);
            ssSend << a1;
            EndMessage();
        } catch (...) {
            AbortMessage();
            throw;
        }
    }

    template <typename T1, typename T2>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2) {
        try {
            BeginMessage(pszCommand);
            ssSend << a1 << a2;
            EndMessage();
        } catch (...) {
            AbortMessage();
            throw;
        }
    }

    template <typename T1, typename T2, typename T3>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3) {
        try {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3;
            EndMessage();
        } catch (...) {
            AbortMessage();
            throw;
        }
    }

    template <typename T1, typename T2, typename T3, typename T4>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4) {
        try {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3 << a4;
            EndMessage();
        } catch (...) {
            AbortMessage();
            throw;
        }
    }

    template <typename T1, typename T2, typename T3, typename T4, typename T5>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5) {
        try {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3 << a4 << a5;
            EndMessage();
        } catch (...) {
            AbortMessage();
            throw;
        }
    }

    template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5,
                     const T6& a6) {
        try {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3 << a4 << a5 << a6;
            EndMessage();
        } catch (...) {
            AbortMessage();
            throw;
        }
    }

    template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5,
                     const T6& a6, const T7& a7) {
        try {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3 << a4 << a5 << a6 << a7;
            EndMessage();
        } catch (...) {
            AbortMessage();
            throw;
        }
    }

    template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5,
                     const T6& a6, const T7& a7, const T8& a8) {
        try {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3 << a4 << a5 << a6 << a7 << a8;
            EndMessage();
        } catch (...) {
            AbortMessage();
            throw;
        }
    }

    template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8,
            typename T9>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5,
                     const T6& a6, const T7& a7, const T8& a8, const T9& a9) {
        try {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3 << a4 << a5 << a6 << a7 << a8 << a9;
            EndMessage();
        } catch (...) {
            AbortMessage();
            throw;
        }
    }

    void CloseSocketDisconnect();
    void Cleanup();
    void SocketSendData();
    // Denial-of-service detection/prevention
    // The idea is to detect peers that are behaving
    // badly and disconnect/ban them, but do it in a
    // one-coding-mistake-won't-shatter-the-entire-network
    // way.
    // IMPORTANT:  There should be nothing I can give a
    // node that it will forward on that will make that
    // node's peers drop it. If there is, an attacker
    // can isolate a node and/or try to split the network.
    // Dropping a node for sending stuff that is invalid
    // now but might be valid in a later version is also
    // dangerous, because it can cause a network split
    // between nodes running old code and nodes running
    // new code.
    static bool IsBanned(CNetAddr ip);
    static bool Ban(const CNetAddr& ip);
    void copyStats(CNodeStats& stats);

    // Network stats
    static void RecordBytesRecv(uint64_t bytes);
    static void RecordBytesSent(uint64_t bytes);

    static uint64_t GetTotalBytesRecv();
    static uint64_t GetTotalBytesSent();
};

#endif //P2P_NODE_H
