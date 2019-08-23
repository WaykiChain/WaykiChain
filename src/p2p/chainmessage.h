// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The WaykiChain developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CHAINMESSAGE_H
#define CHAINMESSAGE_H

#include "alert.h"
#include "commons/uint256.h"
#include "commons/util.h"
#include "main.h"
#include "net.h"

#include <string>
#include <vector>

using namespace std ;

class CNode ;
class CDataStream ;
class CInv ;
class COrphanBlock ;

extern CChain chainActive ;
extern uint256 GetOrphanRoot(const uint256 &hash);


extern map<uint256, COrphanBlock *> mapOrphanBlocks;
extern map<uint256, std::shared_ptr<CBaseTx> > mapOrphanTransactions;


// Blocks that are in flight, and that are in the queue to be downloaded.
// Protected by cs_main.
struct QueuedBlock {
    uint256 hash;
    int64_t nTime;      // Time of "getdata" request in microseconds.
    int32_t nQueuedBefore;  // Number of blocks in flight at the time of request.
};
namespace {
    map<uint256, pair<NodeId, list<QueuedBlock>::iterator> > mapBlocksInFlight;
    map<uint256, pair<NodeId, list<uint256>::iterator> > mapBlocksToDownload;  //存放待下载到的块，下载后执行erase

    // Sources of received blocks, to be able to send them reject messages or ban
    // them, if processing happens afterwards. Protected by cs_main.
    map<uint256, NodeId> mapBlockSource;  // Remember who we got this block from.

    struct CBlockReject {
        uint8_t chRejectCode;
        string strRejectReason;
        uint256 blockHash;
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
        int32_t nBlocksInFlight;              //每个节点,单独能下载的最大块数量   MAX_BLOCKS_IN_TRANSIT_PER_PEER
        list<uint256> vBlocksToDownload;  //待下载的块
        int32_t nBlocksToDownload;            //待下载的块个数
        int64_t nLastBlockReceive;        //上一次收到块的时间
        int64_t nLastBlockProcess;        //收到块，处理消息时的时间

        CNodeState() {
            nMisbehavior      = 0;
            fShouldBan        = false;
            nBlocksToDownload = 0;
            nBlocksInFlight   = 0;
            nLastBlockReceive = 0;
            nLastBlockProcess = 0;
        }
    };

    // Map maintaining per-node state. Requires cs_main.
    map<NodeId, CNodeState> mapNodeState;

    // Requires cs_main.
    CNodeState *State(NodeId pNode) {
        map<NodeId, CNodeState>::iterator it = mapNodeState.find(pNode);
        if (it == mapNodeState.end())
            return nullptr;
        return &it->second;
    }




    // Requires cs_main.
    void MarkBlockAsReceived(const uint256 &hash, NodeId nodeFrom = -1) {
        map<uint256, pair<NodeId, list<uint256>::iterator> >::iterator itToDownload = mapBlocksToDownload.find(hash);
        if (itToDownload != mapBlocksToDownload.end()) {
            CNodeState *state = State(itToDownload->second.first);
            state->vBlocksToDownload.erase(itToDownload->second.second);
            state->nBlocksToDownload--;
            mapBlocksToDownload.erase(itToDownload);
        }

        map<uint256, pair<NodeId, list<QueuedBlock>::iterator> >::iterator itInFlight = mapBlocksInFlight.find(hash);
        if (itInFlight != mapBlocksInFlight.end()) {
            CNodeState *state = State(itInFlight->second.first);
            state->vBlocksInFlight.erase(itInFlight->second.second);
            state->nBlocksInFlight--;
            if (itInFlight->second.first == nodeFrom)
                state->nLastBlockReceive = GetTimeMicros();
            mapBlocksInFlight.erase(itInFlight);
        }
    }

}  // namespace

struct COrphanBlock {
    uint256 blockHash;
    uint256 prevBlockHash;
    int32_t height;
    vector<uint8_t> vchBlock;
};


static CMedianFilter<int32_t> cPeerBlockCounts(8, 0);

inline void ProcessGetData(CNode *pFrom) {
    deque<CInv>::iterator it = pFrom->vRecvGetData.begin();

    vector<CInv> vNotFound;

    LOCK(cs_main);

    while (it != pFrom->vRecvGetData.end()) {
        // Don't bother if send buffer is too full to respond anyway
        if (pFrom->nSendSize >= SendBufferSize())
            break;

        const CInv &inv = *it;
        {
            boost::this_thread::interruption_point();
            it++;

            if (inv.type == MSG_BLOCK || inv.type == MSG_FILTERED_BLOCK) {
                bool send                                = false;
                map<uint256, CBlockIndex *>::iterator mi = mapBlockIndex.find(inv.hash);
                if (mi != mapBlockIndex.end()) {
                    send = true;
                }
                if (send) {
                    // Send block from disk
                    CBlock block;
                    ReadBlockFromDisk((*mi).second, block);
                    if (inv.type == MSG_BLOCK)
                        pFrom->PushMessage("block", block);
                    else  // MSG_FILTERED_BLOCK)
                    {
                        LOCK(pFrom->cs_filter);
                        if (pFrom->pfilter) {
                            CMerkleBlock merkleBlock(block, *pFrom->pfilter);
                            pFrom->PushMessage("merkleblock", merkleBlock);
                            // CMerkleBlock just contains hashes, so also push any transactions in the block the client did not see
                            // This avoids hurting performance by pointlessly requiring a round-trip
                            // Note that there is currently no way for a node to request any single transactions we didnt send here -
                            // they must either disconnect and retry or request the full block.
                            // Thus, the protocol spec specified allows for us to provide duplicate txn here,
                            // however we MUST always provide at least what the remote peer needs
                            for (auto &pair : merkleBlock.vMatchedTxn)
                                if (!pFrom->setInventoryKnown.count(CInv(MSG_TX, pair.second)))
                                    pFrom->PushMessage("tx", block.vptx[pair.first]);
                        }
                        // else
                        // no response
                    }

                    // Trigger them to send a getblocks request for the next batch of inventory
                    if (inv.hash == pFrom->hashContinue) {
                        // Bypass PushInventory, this must send even if redundant,
                        // and we want it right after the last block so they don't
                        // wait for other stuff first.
                        vector<CInv> vInv;
                        vInv.push_back(CInv(MSG_BLOCK, chainActive.Tip()->GetBlockHash()));
                        pFrom->PushMessage("inv", vInv);
                        pFrom->hashContinue.SetNull();
                        LogPrint("net", "reset node hashcontinue\n");
                    }
                }
            } else if (inv.IsKnownType()) {
                // Send stream from relay memory
                bool pushed = false;
                {
                    LOCK(cs_mapRelay);
                    map<CInv, CDataStream>::iterator mi = mapRelay.find(inv);
                    if (mi != mapRelay.end()) {
                        pFrom->PushMessage(inv.GetCommand(), (*mi).second);
                        pushed = true;
                    }
                }
                if (!pushed && inv.type == MSG_TX) {
                    std::shared_ptr<CBaseTx> pBaseTx = mempool.Lookup(inv.hash);
                    if (pBaseTx.get() && !pBaseTx->IsBlockRewardTx() && !pBaseTx->IsMedianPriceTx()) {
                        CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
                        ss.reserve(1000);
                        ss << pBaseTx;
                        pFrom->PushMessage("tx", ss);
                        pushed = true;
                    }
                }
                if (!pushed) {
                    vNotFound.push_back(inv);
                }
            }

            // Track requests for our stuff.
            // g_signals.Inventory(inv.hash);

            if (inv.type == MSG_BLOCK || inv.type == MSG_FILTERED_BLOCK)
                break;
        }
    }

    pFrom->vRecvGetData.erase(pFrom->vRecvGetData.begin(), it);

    if (!vNotFound.empty()) {
        // Let the peer know that we didn't find what it asked for, so it doesn't
        // have to wait around forever. Currently only SPV clients actually care
        // about this message: it's needed when they are recursively walking the
        // dependencies of relevant unconfirmed transactions. SPV clients want to
        // do that because they want to know about (and store and rebroadcast and
        // risk analyze) the dependencies of transactions relevant to them, without
        // having to download the entire memory pool.
        pFrom->PushMessage("notfound", vNotFound);
    }
}

bool AlreadyHave(const CInv &inv) {
    switch (inv.type) {
        case MSG_TX: {
            bool txInMap = false;
            txInMap      = mempool.Exists(inv.hash);
            return txInMap || mapOrphanTransactions.count(inv.hash);
        }
        case MSG_BLOCK:
            return mapBlockIndex.count(inv.hash) ||
                   mapOrphanBlocks.count(inv.hash);
    }
    // Don't know what it is, just say we already got one
    return true;
}

// Requires cs_main.
inline bool AddBlockToQueue(NodeId nodeid, const uint256 &hash) {

    if (mapBlocksToDownload.count(hash) || mapBlocksInFlight.count(hash)) {
        return false;
    }

    CNodeState *state = State(nodeid);
    if (state == nullptr) {
        return false;
    }

    list<uint256>::iterator it = state->vBlocksToDownload.insert(state->vBlocksToDownload.end(), hash);
    state->nBlocksToDownload++;
    if (state->nBlocksToDownload > 5000) {
        LogPrint("INFO", "Misbehaving: AddBlockToQueue download too many times, nMisbehavior add 10\n");
        Misbehaving(nodeid, 10);
    }
    mapBlocksToDownload[hash] = make_pair(nodeid, it);
    return true;
}

inline int ProcessVersionMessage(CNode *pFrom, string strCommand, CDataStream &vRecv){
    // Each connection can only send one version message
    if (pFrom->nVersion != 0) {
        pFrom->PushMessage("reject", strCommand, REJECT_DUPLICATE, string("Duplicate version message"));
        LogPrint("INFO", "Misbehaving: Duplicated version message, nMisbehavior add 1\n");
        Misbehaving(pFrom->GetId(), 1);
        return false;
    }

    int64_t nTime;
    CAddress addrMe;
    CAddress addrFrom;
    uint64_t nNonce = 1;
    vRecv >> pFrom->nVersion >> pFrom->nServices >> nTime >> addrMe;
    if (pFrom->nVersion < MIN_PEER_PROTO_VERSION) {
        // Disconnect from peers older than this proto version
        LogPrint("INFO", "partner %s using obsolete version %i; disconnecting\n", pFrom->addr.ToString(), pFrom->nVersion);
        pFrom->PushMessage("reject", strCommand, REJECT_OBSOLETE,
                           strprintf("Version must be %d or greater", MIN_PEER_PROTO_VERSION));
        pFrom->fDisconnect = true;
        return 0;
    }

    if (pFrom->nVersion == 10300)
        pFrom->nVersion = 300;

    if (!vRecv.empty())
        vRecv >> addrFrom >> nNonce;

    if (!vRecv.empty()) {
        vRecv >> pFrom->strSubVer;
        pFrom->cleanSubVer = SanitizeString(pFrom->strSubVer);
    }

    if (!vRecv.empty())
        vRecv >> pFrom->nStartingHeight;

    if (!vRecv.empty())
        vRecv >> pFrom->fRelayTxes;  // set to true after we get the first filter* message
    else
        pFrom->fRelayTxes = true;

    if (pFrom->fInbound && addrMe.IsRoutable()) {
        pFrom->addrLocal = addrMe;
        SeenLocal(addrMe);
    }

    // Disconnect if we connected to ourself
    if (nNonce == nLocalHostNonce && nNonce > 1) {
        LogPrint("INFO", "connected to self at %s, disconnecting\n", pFrom->addr.ToString());
        pFrom->fDisconnect = true;
        return 1;
    }

    // Be shy and don't send version until we hear
    if (pFrom->fInbound)
        pFrom->PushVersion();

    pFrom->fClient = !(pFrom->nServices & NODE_NETWORK);

    // Change version
    pFrom->PushMessage("verack");
    pFrom->ssSend.SetVersion(min(pFrom->nVersion, PROTOCOL_VERSION));

    if (!pFrom->fInbound) {
        // Advertise our address
        if (!fNoListen && !IsInitialBlockDownload()) {
            CAddress addr = GetLocalAddress(&pFrom->addr);
            if (addr.IsRoutable())
                pFrom->PushAddress(addr);
        }

        // Get recent addresses
        if (pFrom->fOneShot || /*pFrom->nVersion >= CADDR_TIME_VERSION || */ addrman.size() < 1000) {
            pFrom->PushMessage("getaddr");
            pFrom->fGetAddr = true;
        }
        addrman.Good(pFrom->addr);
    } else {
        if (((CNetAddr)pFrom->addr) == (CNetAddr)addrFrom) {
            addrman.Add(addrFrom, addrFrom);
            addrman.Good(addrFrom);
        }
    }

    // Relay alerts
    {
        LOCK(cs_mapAlerts);
        for (const auto &item : mapAlerts)
            item.second.RelayTo(pFrom);
    }

    pFrom->fSuccessfullyConnected = true;

    LogPrint("INFO", "receive version message: %s: version %d, blocks=%d, us=%s, them=%s, peer=%s\n", pFrom->cleanSubVer, pFrom->nVersion, pFrom->nStartingHeight, addrMe.ToString(), addrFrom.ToString(), pFrom->addr.ToString());

    AddTimeData(pFrom->addr, nTime);

    {
        LOCK(cs_main);
        cPeerBlockCounts.input(pFrom->nStartingHeight);
    }

    return -1 ;
}

inline void ProcessPongMessage(CNode *pFrom, CDataStream &vRecv){
    int64_t pingUsecEnd = GetTimeMicros();
    uint64_t nonce      = 0;
    size_t nAvail       = vRecv.in_avail();
    bool bPingFinished  = false;
    string sProblem;

    if (nAvail >= sizeof(nonce)) {
        vRecv >> nonce;

        // Only process pong message if there is an outstanding ping (old ping without nonce should never pong)
        if (pFrom->nPingNonceSent != 0) {
            if (nonce == pFrom->nPingNonceSent) {
                // Matching pong received, this ping is no longer outstanding
                bPingFinished        = true;
                int64_t pingUsecTime = pingUsecEnd - pFrom->nPingUsecStart;
                if (pingUsecTime > 0) {
                    // Successful ping time measurement, replace previous
                    pFrom->nPingUsecTime = pingUsecTime;
                } else {
                    // This should never happen
                    sProblem = "Timing mishap";
                }
            } else {
                // Nonce mismatches are normal when pings are overlapping
                sProblem = "Nonce mismatch";
                if (nonce == 0) {
                    // This is most likely a bug in another implementation somewhere, cancel this ping
                    bPingFinished = true;
                    sProblem      = "Nonce zero";
                }
            }
        } else {
            sProblem = "Unsolicited pong without ping";
        }
    } else {
        // This is most likely a bug in another implementation somewhere, cancel this ping
        bPingFinished = true;
        sProblem      = "Short payload";
    }

    if (!(sProblem.empty())) {
        LogPrint("net", "pong %s %s: %s, %x expected, %x received, %u bytes\n",
                 pFrom->addr.ToString(),
                 pFrom->cleanSubVer,
                 sProblem,
                 pFrom->nPingNonceSent,
                 nonce,
                 nAvail);
    }
    if (bPingFinished) {
        pFrom->nPingNonceSent = 0;
    }
}

inline bool ProcessAddrMessage(CNode* pFrom, CDataStream &vRecv){
    vector<CAddress> vAddr;
    vRecv >> vAddr;

    // Don't want addr from older versions unless seeding
    // if (pFrom->nVersion < CADDR_TIME_VERSION && addrman.size() > 1000)
    //     return true;
    if (vAddr.size() > 1000) {
        Misbehaving(pFrom->GetId(), 20);
        return ERRORMSG("message addr size() = %u", vAddr.size());
    }

    // Store the new addresses
    vector<CAddress> vAddrOk;
    int64_t nNow   = GetAdjustedTime();
    int64_t nSince = nNow - 10 * 60;
    for (auto &addr : vAddr) {
        boost::this_thread::interruption_point();

        if (addr.nTime <= 100000000 || addr.nTime > nNow + 10 * 60)
            addr.nTime = nNow - 5 * 24 * 60 * 60;

        pFrom->AddAddressKnown(addr);
        bool fReachable = IsReachable(addr);
        if (addr.nTime > nSince && !pFrom->fGetAddr && vAddr.size() <= 10 && addr.IsRoutable()) {
            // Relay to a limited number of other nodes
            {
                LOCK(cs_vNodes);
                // Use deterministic randomness to send to the same nodes for 24 hours
                // at a time so the setAddrKnowns of the chosen nodes prevent repeats
                static uint256 hashSalt;
                if (hashSalt.IsNull())
                    hashSalt = GetRandHash();
                uint64_t hashAddr = addr.GetHash();
                uint256 hashRand  = ArithToUint256(UintToArith256(hashSalt) ^ (hashAddr << 32) ^ ((GetTime() + hashAddr) / (24 * 60 * 60)));
                hashRand          = Hash(BEGIN(hashRand), END(hashRand));
                multimap<uint256, CNode *> mapMix;
                for (auto pNode : vNodes) {
                    //                        if (pNode->nVersion < CADDR_TIME_VERSION)
                    //                            continue;
                    uint32_t nPointer;
                    memcpy(&nPointer, &pNode, sizeof(nPointer));
                    uint256 hashKey = ArithToUint256(UintToArith256(hashRand) ^ nPointer);
                    hashKey         = Hash(BEGIN(hashKey), END(hashKey));
                    mapMix.insert(make_pair(hashKey, pNode));
                }
                int32_t nRelayNodes = fReachable ? 2 : 1;  // limited relaying of addresses outside our network(s)
                for (multimap<uint256, CNode *>::iterator mi = mapMix.begin(); mi != mapMix.end() && nRelayNodes-- > 0; ++mi)
                    ((*mi).second)->PushAddress(addr);
            }
        }
        // Do not store addresses outside our network
        if (fReachable)
            vAddrOk.push_back(addr);
    }
    addrman.Add(vAddrOk, pFrom->addr, 2 * 60 * 60);
    if (vAddr.size() < 1000)
        pFrom->fGetAddr = false;
    if (pFrom->fOneShot)
        pFrom->fDisconnect = true;

    return true;
}

inline bool ProcessTxMessage(CNode* pFrom, string strCommand , CDataStream& vRecv){
    std::shared_ptr<CBaseTx> pBaseTx = CreateNewEmptyTransaction(vRecv[0]);

    if (pBaseTx->IsBlockRewardTx() || pBaseTx->IsMedianPriceTx()) {
        return ERRORMSG("None of BLOCK_REWARD_TX, UCOIN_BLOCK_REWARD_TX, PRICE_MEDIAN_TX from network "
                        "should be accepted, raw string: %s", HexStr(vRecv.begin(), vRecv.end()));
    }

    vRecv >> pBaseTx;

    CInv inv(MSG_TX, pBaseTx->GetHash());
    pFrom->AddInventoryKnown(inv);

    LOCK(cs_main);
    CValidationState state;
    if (AcceptToMemoryPool(mempool, state, pBaseTx.get(), true)) {
        RelayTransaction(pBaseTx.get(), inv.hash);
        mapAlreadyAskedFor.erase(inv);

        LogPrint("INFO", "AcceptToMemoryPool: %s %s : accepted %s (poolsz %u)\n",
                 pFrom->addr.ToString(), pFrom->cleanSubVer,
                 pBaseTx->GetHash().ToString(),
                 mempool.memPoolTxs.size());
    }

    int32_t nDoS = 0;
    if (state.IsInvalid(nDoS)) {
        LogPrint("INFO", "%s from %s %s was not accepted into the memory pool: %s\n",
                 pBaseTx->GetHash().ToString(),
                 pFrom->addr.ToString(),
                 pFrom->cleanSubVer,
                 state.GetRejectReason());

        pFrom->PushMessage("reject", strCommand, state.GetRejectCode(), state.GetRejectReason(), inv.hash);
        // if (nDoS > 0) {
        //     LogPrint("INFO", "Misebehaving, add to tx hash %s mempool error, Misbehavior add %d", pBaseTx->GetHash().GetHex(), nDoS);
        //     Misbehaving(pFrom->GetId(), nDoS);
        // }
    }

    return true ;
}

inline bool ProcessGetHeadersMessage(CNode *pFrom, CDataStream &vRecv){

    CBlockLocator locator;
    uint256 hashStop;
    vRecv >> locator >> hashStop;

    LOCK(cs_main);

    CBlockIndex *pIndex = nullptr;
    if (locator.IsNull()) {
        // If locator is null, return the hashStop block
        map<uint256, CBlockIndex *>::iterator mi = mapBlockIndex.find(hashStop);
        if (mi == mapBlockIndex.end())
            return true;
        pIndex = (*mi).second;
    } else {
        // Find the last block the caller has in the main chain
        pIndex = chainActive.FindFork(locator);
        if (pIndex)
            pIndex = chainActive.Next(pIndex);
    }

    // We must use CBlocks, as CBlockHeaders won't include the 0x00 nTx count at the end
    vector<CBlock> vHeaders;
    int32_t nLimit = 2000;
    LogPrint("NET", "getheaders %d to %s\n", (pIndex ? pIndex->height : -1), hashStop.ToString());
    for (; pIndex; pIndex = chainActive.Next(pIndex)) {
        vHeaders.push_back(pIndex->GetBlockHeader());
        if (--nLimit <= 0 || pIndex->GetBlockHash() == hashStop)
            break;
    }
    pFrom->PushMessage("headers", vHeaders);

    return false;
}

inline void ProcessGetBlocksMessage(CNode *pFrom, CDataStream &vRecv){

    CBlockLocator locator;
    uint256 hashStop;
    vRecv >> locator >> hashStop;

    LOCK(cs_main);

    // Find the last block the caller has in the main chain
    CBlockIndex *pIndex = chainActive.FindFork(locator);

    // Send the rest of the chain
    if (pIndex)
        pIndex = chainActive.Next(pIndex);
    int32_t nLimit = 500;
    LogPrint("net", "getblocks %d to %s limit %d\n", (pIndex ? pIndex->height : -1), hashStop.ToString(), nLimit);
    for (; pIndex; pIndex = chainActive.Next(pIndex)) {
        if (pIndex->GetBlockHash() == hashStop) {
            LogPrint("net", "getblocks stopping at %d %s\n", pIndex->height, pIndex->GetBlockHash().ToString());
            break;
        }
        pFrom->PushInventory(CInv(MSG_BLOCK, pIndex->GetBlockHash()));
        if (--nLimit <= 0) {
            // When this block is requested, we'll send an inv that'll make them
            // getblocks the next batch of inventory.
            LogPrint("net", "getblocks stopping at limit %d %s\n", pIndex->height, pIndex->GetBlockHash().ToString());
            pFrom->hashContinue = pIndex->GetBlockHash();
            break;
        }
    }

}

inline bool ProcessInvMessage(CNode *pFrom, CDataStream &vRecv){
    vector<CInv> vInv;
    vRecv >> vInv;
    if (vInv.size() > MAX_INV_SZ) {
        Misbehaving(pFrom->GetId(), 20);
        return ERRORMSG("message inv size() = %u", vInv.size());
    }

    LOCK(cs_main);

    for (uint32_t nInv = 0; nInv < vInv.size(); nInv++) {
        const CInv &inv = vInv[nInv];

        boost::this_thread::interruption_point();
        pFrom->AddInventoryKnown(inv);
        bool fAlreadyHave = AlreadyHave(inv);

        int32_t nBlockHeight = 0;
        if (inv.type == MSG_BLOCK && mapBlockIndex.count(inv.hash))
            nBlockHeight = mapBlockIndex[inv.hash]->height;

        LogPrint("net", "got inventory[%d]: %s %s %d from peer %s\n", nInv, inv.ToString(),
                 fAlreadyHave ? "have" : "new", nBlockHeight, pFrom->addr.ToString());

        if (!fAlreadyHave) {
            if (!SysCfg().IsImporting() && !SysCfg().IsReindex()) {
                if (inv.type == MSG_BLOCK)
                    AddBlockToQueue(pFrom->GetId(), inv.hash);
                else
                    pFrom->AskFor(inv);  // MSG_TX
            }
        } else if (inv.type == MSG_BLOCK && mapOrphanBlocks.count(inv.hash)) {
            COrphanBlock *pOrphanBlock = mapOrphanBlocks[inv.hash];
            LogPrint("net", "receive orphan block inv height=%d hash=%s lead to getblocks, current height=%d\n",
                     pOrphanBlock->height, inv.hash.GetHex(), chainActive.Height());
            PushGetBlocksOnCondition(pFrom, chainActive.Tip(), GetOrphanRoot(inv.hash));
        }

        if (pFrom->nSendSize > (SendBufferSize() * 2)) {
            Misbehaving(pFrom->GetId(), 50);
            return ERRORMSG("send buffer size() = %u", pFrom->nSendSize);
        }
    }
    return true ;
}

inline bool ProcessGetDataMessage(CNode *pFrom, CDataStream &vRecv){
    vector<CInv> vInv;
    vRecv >> vInv;
    if (vInv.size() > MAX_INV_SZ) {
        Misbehaving(pFrom->GetId(), 20);
        return ERRORMSG("message getdata size() = %u", vInv.size());
    }

    if ((vInv.size() != 1))
    LogPrint("net", "received getdata (%u invsz)\n", vInv.size());

    if ((vInv.size() > 0) || (vInv.size() == 1))
    LogPrint("net", "received getdata for: %s\n", vInv[0].ToString());

    pFrom->vRecvGetData.insert(pFrom->vRecvGetData.end(), vInv.begin(), vInv.end());
    ProcessGetData(pFrom);
    return true ;
}

inline void ProcessBlockMessage(CNode *pFrom, CDataStream &vRecv){
    CBlock block;
    vRecv >> block;

    LogPrint("net", "received block %s from %s\n", block.GetHash().ToString(), pFrom->addr.ToString());
    // block.Print();

    CInv inv(MSG_BLOCK, block.GetHash());
    pFrom->AddInventoryKnown(inv);

    LOCK(cs_main);
    // Remember who we got this block from.
    mapBlockSource[inv.hash] = pFrom->GetId();
    MarkBlockAsReceived(inv.hash, pFrom->GetId());

    CValidationState state;
    ProcessBlock(state, pFrom, &block);
}

inline void ProcessMempoolMessage(CNode *pFrom, CDataStream &vRecv){
    LOCK2(cs_main, pFrom->cs_filter);

    vector<uint256> vtxid;
    mempool.QueryHash(vtxid);
    vector<CInv> vInv;
    for (auto &hash : vtxid) {
        CInv inv(MSG_TX, hash);
        std::shared_ptr<CBaseTx> pBaseTx = mempool.Lookup(hash);
        if (pBaseTx.get())
            continue;  // another thread removed since queryHashes, maybe...

        if ((pFrom->pfilter && pFrom->pfilter->contains(hash)) ||  //other type transaction
            (!pFrom->pfilter))
            vInv.push_back(inv);

        if (vInv.size() == MAX_INV_SZ) {
            pFrom->PushMessage("inv", vInv);
            vInv.clear();
        }
    }
    if (vInv.size() > 0)
        pFrom->PushMessage("inv", vInv);
}

inline void ProcessAlertMessage(CNode *pFrom, CDataStream &vRecv){

    CAlert alert;
    vRecv >> alert;

    uint256 alertHash = alert.GetHash();
    if (pFrom->setKnown.count(alertHash) == 0) {
        if (alert.ProcessAlert()) {
            // Relay
            pFrom->setKnown.insert(alertHash);
            {
                LOCK(cs_vNodes);
                for (auto pNode : vNodes)
                    alert.RelayTo(pNode);
            }
        } else {
            // Small DoS penalty so peers that send us lots of
            // duplicate/expired/invalid-signature/whatever alerts
            // eventually get banned.
            // This isn't a Misbehaving(100) (immediate ban) because the
            // peer might be an older or different implementation with
            // a different signature key, etc.
            Misbehaving(pFrom->GetId(), 10);
        }
    }
}

inline void ProcessFilterLoadMessage(CNode *pFrom, CDataStream &vRecv){
    CBloomFilter filter;
    vRecv >> filter;

    if (!filter.IsWithinSizeConstraints()) {
        LogPrint("INFO", "Misebehaving: filter is not within size constraints, Misbehavior add 100");
        // There is no excuse for sending a too-large filter
        Misbehaving(pFrom->GetId(), 100);
    } else {
        LOCK(pFrom->cs_filter);
        delete pFrom->pfilter;
        pFrom->pfilter = new CBloomFilter(filter);
        pFrom->pfilter->UpdateEmptyFull();
    }
    pFrom->fRelayTxes = true;
}

inline void ProcessFilterAddMessage(CNode *pFrom, CDataStream &vRecv){
    vector<uint8_t> vData;
    vRecv >> vData;

    // Nodes must NEVER send a data item > 520 bytes (the max size for a script data object,
    // and thus, the maximum size any matched object can have) in a filteradd message
    if (vData.size() > 520)  //MAX_SCRIPT_ELEMENT_SIZE)
    {
        LogPrint("INFO", "Misbehaving: send a data item > 520 bytes, Misbehavior add 100");
        Misbehaving(pFrom->GetId(), 100);
    } else {
        LOCK(pFrom->cs_filter);
        if (pFrom->pfilter)
            pFrom->pfilter->insert(vData);
        else {
            LogPrint("INFO", "Misbehaving: filter error, Misbehavior add 100");
            Misbehaving(pFrom->GetId(), 100);
        }
    }
}

inline void ProcessRejectMessage(CNode *pFrom, CDataStream &vRecv){
    if (SysCfg().IsDebug()) {
        string strMsg;
        uint8_t ccode;
        string strReason;
        vRecv >> strMsg >> ccode >> strReason;

        ostringstream ss;
        ss << strMsg << " code " << itostr(ccode) << ": " << strReason;

        if (strMsg == "block" || strMsg == "tx") {
            uint256 hash;
            vRecv >> hash;
            ss << ": hash " << hash.ToString();
        }
        // Truncate to reasonable length and sanitize before printing:
        string s = ss.str();
        if (s.size() > 111)
            s.erase(111, string::npos);

        LogPrint("net", "Reject %s\n", SanitizeString(s));
    }

}

#endif  // CHAINMESSAGE_H
