// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CHAINMESSAGE_H
#define CHAINMESSAGE_H

#include "alert.h"
#include "commons/uint256.h"
#include "commons/util/util.h"
#include "main.h"
#include "net.h"
#include "miner/pbftcontext.h"
#include "miner/pbftmanager.h"

#include <string>
#include <tuple>
#include <vector>

using namespace std;

static const int64_t MINER_NODE_BLOCKS_TO_DOWNLOAD_TIMEOUT   = 2;   // 2 seconds
static const int64_t MINER_NODE_BLOCKS_IN_FLIGHT_TIMEOUT     = 1;   // 1 seconds
static const int64_t WITNESS_NODE_BLOCKS_TO_DOWNLOAD_TIMEOUT = 20;  // 20 seconds
static const int64_t WITNESS_NODE_BLOCKS_IN_FLIGHT_TIMEOUT   = 10;  // 10 seconds

class CNode;
class CDataStream;
class CInv;
class COrphanBlock;
class CBlockConfirmMessage;

extern CPBFTContext pbftContext;
extern CPBFTMan pbftMan;
extern CChain chainActive;
extern uint256 GetOrphanRoot(const uint256 &hash);
extern map<uint256, COrphanBlock *> mapOrphanBlocks;

namespace {
map<uint256, tuple<NodeId, list<QueuedBlock>::iterator, int64_t>> mapBlocksInFlight;  // downloading blocks
map<uint256, tuple<NodeId, list<uint256>::iterator, int64_t>> mapBlocksToDownload;    // blocks to be downloaded

// Sources of received blocks, to be able to send them reject messages or ban
// them, if processing happens afterwards. Protected by cs_main.
map<uint256, NodeId> mapBlockSource;  // Remember who we got this block from.


// Requires cs_mapNodeState.
void MarkBlockAsReceived(const uint256 &hash, NodeId nodeFrom = -1) {
    AssertLockHeld(cs_mapNodeState);
    auto itToDownload = mapBlocksToDownload.find(hash);
    if (itToDownload != mapBlocksToDownload.end()) {
        CNodeState *state = State(std::get<0>(itToDownload->second));
        state->vBlocksToDownload.erase(std::get<1>(itToDownload->second));
        state->nBlocksToDownload--;

        mapBlocksToDownload.erase(itToDownload);
    }

    auto itInFlight = mapBlocksInFlight.find(hash);
    if (itInFlight != mapBlocksInFlight.end()) {
        CNodeState *state = State(std::get<0>(itInFlight->second));
        state->vBlocksInFlight.erase(std::get<1>(itInFlight->second));
        state->nBlocksInFlight--;
        if (std::get<0>(itInFlight->second) == nodeFrom)
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
        if (pFrom->nSendSize >= SendBufferSize()) {
            LogPrint(BCLog::NET, "send buffer size: %d full for peer: %s\n", pFrom->nSendSize, pFrom->addr.ToString());
            break;
        }

        const CInv &inv = *it;
        {
            boost::this_thread::interruption_point();
            it++;

            if (inv.type == MSG_BLOCK || inv.type == MSG_FILTERED_BLOCK) {
                auto mi = mapBlockIndex.find(inv.hash);
                if (mi == mapBlockIndex.end()) {
                    LogPrint(BCLog::NET, "block %s not found\n", inv.hash.GetHex());

                } else { // Load block from disk and send it
                    CBlock block;
                    ReadBlockFromDisk((*mi).second, block);
                    if (inv.type == MSG_BLOCK) {
                        LogPrint(BCLog::NET, "send block[%u]: %s to peer %s\n", block.GetHeight(), block.GetHash().GetHex(),
                                 pFrom->addr.ToString());

                        pFrom->PushMessage(NetMsgType::BLOCK, block);

                    } else  {// MSG_FILTERED_BLOCK)
                        LOCK(pFrom->cs_filter);
                        if (pFrom->pFilter) {
                            CMerkleBlock merkleBlock(block, *pFrom->pFilter);
                            pFrom->PushMessage("merkleblock", merkleBlock);
                            // CMerkleBlock just contains hashes, so also push any transactions in the block the client
                            // did not see This avoids hurting performance by pointlessly requiring a round-trip Note
                            // that there is currently no way for a node to request any single transactions we didnt
                            // send here - they must either disconnect and retry or request the full block. Thus, the
                            // protocol spec specified allows for us to provide duplicate txn here, however we MUST
                            // always provide at least what the remote peer needs
                            for (auto &pair : merkleBlock.vMatchedTxn)
                                if (!pFrom->setInventoryKnown.count(CInv(MSG_TX, pair.second)))
                                    pFrom->PushMessage(NetMsgType::TX, block.vptx[pair.first]);
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
                        pFrom->PushMessage(NetMsgType::INV, vInv);
                        pFrom->hashContinue.SetNull();
                        LogPrint(BCLog::NET, "reset node hashcontinue\n");
                    }
                }
            } else if (inv.IsKnownType()) {
                // Send stream from relay memory
                bool pushed = false;
                {
                    LOCK(cs_mapRelay);
                    auto mi = mapRelay.find(inv);
                    if (mi != mapRelay.end()) {
                        pFrom->PushMessage(inv.GetCommand(), (*mi).second);
                        pushed = true;
                    }
                }
                if (!pushed && inv.type == MSG_TX) {
                    std::shared_ptr<CBaseTx> pBaseTx = mempool.Lookup(inv.hash);
                    if (pBaseTx && !pBaseTx->IsRelayForbidden()) {
                        CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
                        ss.reserve(1000); //FIXME: hardcoding here
                        ss << pBaseTx;
                        pFrom->PushMessage(NetMsgType::TX, ss);
                        pushed = true;
                    }
                }
                if (!pushed) {
                    vNotFound.push_back(inv);
                }
            }

            // Track requests for our stuff.
            // g_signals.Inventory(inv.hash);
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
            return mempool.Exists(inv.hash);
        }

        case MSG_BLOCK: {
            return mapBlockIndex.count(inv.hash) || mapOrphanBlocks.count(inv.hash);
        }
    }

    // Don't know what it is, just say we already got one
    return true;
}

// Requires cs_main.
inline bool AddBlockToQueue(const uint256 &hash, NodeId nodeId) {
    int64_t now  = GetTimeMicros();
    bool isMiner = SysCfg().GetBoolArg("-genblock", false);

    int64_t blocksToDownloadTimeout = isMiner ? MINER_NODE_BLOCKS_TO_DOWNLOAD_TIMEOUT : WITNESS_NODE_BLOCKS_TO_DOWNLOAD_TIMEOUT;
    int64_t blockInFlightTimeout    = isMiner ? MINER_NODE_BLOCKS_IN_FLIGHT_TIMEOUT : WITNESS_NODE_BLOCKS_IN_FLIGHT_TIMEOUT;

    if ((mapBlocksToDownload.count(hash) &&
         (now - std::get<2>(mapBlocksToDownload[hash]) < blocksToDownloadTimeout * 1000000)) ||
        (mapBlocksInFlight.count(hash) &&
         (now - std::get<2>(mapBlocksInFlight[hash]) < blockInFlightTimeout * 1000000))) {
        LogPrint(BCLog::NET, "block (%s) being downloaded from another peer, ignore! ts=%lld\n", hash.GetHex(), GetTimeMillis());

        return false;
    }

    LOCK(cs_mapNodeState);
    CNodeState *state = State(nodeId);
    if (state == nullptr) {
        LogPrint(BCLog::NET, "peer (%d) not found! ts=%lld, block(%s) \n",
            nodeId, GetTimeMillis(), hash.ToString());
        return false;
    }

    // Make sure it's not listed somewhere already.
    MarkBlockAsReceived(hash);

    list<uint256>::iterator it = state->vBlocksToDownload.insert(state->vBlocksToDownload.end(), hash);
    state->nBlocksToDownload++;
    if (state->nBlocksToDownload > 5000) {
        LogPrint(BCLog::INFO, "Misbehaving: AddBlockToQueue download too many times, nMisbehavior add 10\n");
        Misbehaving(nodeId, 10);
    }

    LogPrint(BCLog::NET, "start downloading block(%s)! time_ms=%lld, peer=%s\n", hash.ToString(),
                          GetTimeMillis(), state->name);

    mapBlocksToDownload[hash] = std::make_tuple(nodeId, it, GetTimeMicros());

    return true;
}

inline int32_t ProcessVersionMessage(CNode *pFrom, string strCommand, CDataStream &vRecv) {
    // Each connection can only send one version message
    if (pFrom->nVersion != 0) {
        pFrom->PushMessage(NetMsgType::REJECT, strCommand, REJECT_DUPLICATE, string("Duplicate version message"));
        LogPrint(BCLog::INFO, "Misbehaving: Duplicated version message, nMisbehavior add 1\n");
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
        LogPrint(BCLog::INFO, "partner %s using obsolete version %i; disconnecting\n", pFrom->addr.ToString(),
                 pFrom->nVersion);
        pFrom->PushMessage(NetMsgType::REJECT, strCommand, REJECT_OBSOLETE,
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

    if (!vRecv.empty()) vRecv >> pFrom->nStartingHeight;

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
        LogPrint(BCLog::INFO, "connected to self at %s, disconnecting\n", pFrom->addr.ToString());
        pFrom->fDisconnect = true;
        return 1;
    }

    // Be shy and don't send version until we hear
    if (pFrom->fInbound)
        pFrom->PushVersion();

    pFrom->fClient = !(pFrom->nServices & NODE_NETWORK);

    // Change version
    pFrom->PushMessage(NetMsgType::VERACK);
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
            pFrom->PushMessage(NetMsgType::GETADDR);
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
        for (const auto &item : mapAlerts) item.second.RelayTo(pFrom);
    }

    pFrom->fSuccessfullyConnected = true;

    LogPrint(BCLog::NET, "receive version msg: %s: protocol_ver %d, blocks=%d, us=%s, them=%s, peer=%s\n",
             pFrom->cleanSubVer, pFrom->nVersion, pFrom->nStartingHeight, addrMe.ToString(), addrFrom.ToString(),
             pFrom->addr.ToString());

    AddTimeData(pFrom->addr, nTime);

    {
        LOCK(cs_main);
        cPeerBlockCounts.input(pFrom->nStartingHeight);
    }

    return -1;
}

inline void ProcessPongMessage(CNode *pFrom, CDataStream &vRecv) {
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
        LogPrint(BCLog::NET, "pong %s %s: %s, %x expected, %x received, %u bytes\n", pFrom->addr.ToString(),
                 pFrom->cleanSubVer, sProblem, pFrom->nPingNonceSent, nonce, nAvail);
    }

    if (bPingFinished) {
        pFrom->nPingNonceSent = 0;
    }
}

inline bool ProcessAddrMessage(CNode *pFrom, CDataStream &vRecv) {
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
                if (hashSalt.IsNull()) hashSalt = GetRandHash();
                uint64_t hashAddr = addr.GetHash();
                uint256 hashRand  = ArithToUint256(UintToArith256(hashSalt) ^ (hashAddr << 32) ^
                                                  ((GetTime() + hashAddr) / (24 * 60 * 60)));
                hashRand          = Hash(BEGIN(hashRand), END(hashRand));
                multimap<uint256, CNode *> mapMix;
                for (auto pNode : vNodes) {
                    // if (pNode->nVersion < CADDR_TIME_VERSION)
                    //     continue;
                    uint32_t nPointer;
                    memcpy(&nPointer, &pNode, sizeof(nPointer));
                    uint256 hashKey = ArithToUint256(UintToArith256(hashRand) ^ nPointer);
                    hashKey         = Hash(BEGIN(hashKey), END(hashKey));
                    mapMix.insert(make_pair(hashKey, pNode));
                }
                int32_t nRelayNodes = fReachable ? 2 : 1;  // limited relaying of addresses outside our network(s)
                for (auto mi = mapMix.begin(); mi != mapMix.end() && nRelayNodes-- > 0; ++mi)
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

inline bool ProcessTxMessage(CNode *pFrom, string strCommand, CDataStream &vRecv) {
    std::shared_ptr<CBaseTx> pBaseTx;
    try {
        vRecv >> pBaseTx;
    } catch(runtime_error e) {
        // TODO: record the misebehaving or ban the peer node.
        return ERRORMSG("Unknown transaction type from peer %s, ignore! %s", pFrom->addr.ToString(), e.what());
    }

    if (pBaseTx->IsRelayForbidden()) {
        return ERRORMSG("Forbid transaction=%s from network from peer %s, txid=%s, raw: %s", pBaseTx->GetTxTypeName(),
                pFrom->addr.ToString(), pBaseTx->GetHash().ToString(), HexStr(vRecv.begin(), vRecv.end()));
    }

    CInv inv(MSG_TX, pBaseTx->GetHash());
    pFrom->AddInventoryKnown(inv);

    if(IsInitialBlockDownload()){
        RelayTransaction(pBaseTx.get(), inv.hash);
        return true;
    }

    LOCK(cs_main);
    CValidationState state;
    if (AcceptToMemoryPool(mempool, state, pBaseTx.get(), true)) {
        RelayTransaction(pBaseTx.get(), inv.hash);
        mapAlreadyAskedFor.erase(inv);

        LogPrint(BCLog::NET, "[%d]~ %s %s : accepted %s (poolsz %u)\n", pBaseTx->valid_height, pFrom->addr.ToString(),
                 pFrom->cleanSubVer, pBaseTx->GetHash().ToString(), mempool.memPoolTxs.size());
    }

    int32_t nDoS = 0;
    if (state.IsInvalid(nDoS)) {
        LogPrint(BCLog::NET, "[%d]~ %s from %s %s not accepted into mempool: %s\n",
                pBaseTx->GetHash().ToString(), pBaseTx->valid_height,
                pFrom->addr.ToString(), pFrom->cleanSubVer, state.GetRejectReason());

        pFrom->PushMessage(NetMsgType::REJECT, strCommand, state.GetRejectCode(), state.GetRejectReason(), inv.hash);
        // if (nDoS > 0) {
        //     LogPrint(BCLog::INFO, "Misebehaving, add to tx hash %s mempool error, Misbehavior add %d",
        //     pBaseTx->GetHash().GetHex(), nDoS); Misbehaving(pFrom->GetId(), nDoS);
        // }
    }

    return true;
}

inline bool ProcessGetHeadersMessage(CNode *pFrom, CDataStream &vRecv) {
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
        pIndex = chainActive.FindFork(mapBlockIndex, locator);
        if (pIndex)
            pIndex = chainActive.Next(pIndex);
    }

    // We must use CBlocks, as CBlockHeaders won't include the 0x00 nTx count at the end
    vector<CBlock> vHeaders;
    int32_t nLimit = 2000;
    LogPrint(BCLog::NET, "getheaders %d to %s from peer %s\n", (pIndex ? pIndex->height : -1), hashStop.ToString(),
             pFrom->addr.ToString());

    for (; pIndex; pIndex = chainActive.Next(pIndex)) {
        CBlockHeader blockHeader;
        pIndex->GetBlockHeader(blockHeader);
        vHeaders.push_back(blockHeader);

        if (--nLimit <= 0 || pIndex->GetBlockHash() == hashStop)
            break;
    }
    pFrom->PushMessage("headers", vHeaders);

    return false;
}

inline void ProcessGetBlocksMessage(CNode *pFrom, CDataStream &vRecv) {
    CBlockLocator locator;
    uint256 hashStop;
    vRecv >> locator >> hashStop;

    LOCK(cs_main);

    // Find the last block the caller has in the main chain
    auto *pStartIndex = chainActive.FindFork(mapBlockIndex, locator);

    // Send the rest of the chain
    if (pStartIndex)
        pStartIndex = chainActive.Next(pStartIndex);

    int32_t nLimit = 500;
    LogPrint(BCLog::NET, "recv getblocks msg! start_block=%s, end_block=%s, tip_block=%s, limit=%d, peer=%s\n",
        (pStartIndex ? pStartIndex->GetIndentityString() : ""), hashStop.ToString(),
        chainActive.Tip()->GetIndentityString(), nLimit, pFrom->addrName);

    CBlockIndex *pIndex = pStartIndex;
    for (; pIndex; pIndex = chainActive.Next(pIndex)) {
        if (pIndex->GetBlockHash() == hashStop) {
            LogPrint(BCLog::NET, "[%d] stoped by hash_end! end_block=%s, peer=%s\n", pIndex->height,
                     pIndex->GetIndentityString(), pFrom->addrName);
            break;
        }

        // bool forced = false;
        // if (pIndex == pStartIndex || pIndex->pprev == pStartIndex)
        //     forced = true;
        // pFrom->PushInventory(CInv(MSG_BLOCK, pIndex->GetBlockHash()), forced);
        bool force_to_send_again = false;
        if (pIndex == pStartIndex || pIndex->pprev == pStartIndex)
            force_to_send_again = true;

        pFrom->PushInventory(CInv(MSG_BLOCK, pIndex->GetBlockHash()), force_to_send_again);

        if (--nLimit <= 0) {
            // When this block is requested, we'll send an inv that'll make them
            // getblocks the next batch of inventory.
            LogPrint(BCLog::NET, "processing getblocks stopped by limit! end_block=%s, limit=%d, peer=%s\n",
                    pIndex->GetIndentityString(), 500, pFrom->addrName);

            pFrom->hashContinue = pIndex->GetBlockHash();
            break;
        }
    }
}

inline bool ProcessInvMessage(CNode *pFrom, CDataStream &vRecv) {
    vector<CInv> vInv;
    vRecv >> vInv;
    if (vInv.size() > MAX_INV_SZ) {
        Misbehaving(pFrom->GetId(), 20);
        return ERRORMSG("message inv size() = %u from peer %s", vInv.size(), pFrom->addrName);
    }

    LOCK(cs_main);

    int i = 0;
    for (CInv &inv : vInv) {
        boost::this_thread::interruption_point();
        pFrom->AddInventoryKnown(inv);

        bool fAlreadyHave = false;
        const char* msgName = "UNKNOWN";
        if (inv.type ==  MSG_TX) {
            msgName = "MSG_TX";
            if (mempool.Exists(inv.hash)) {
                LogPrint(BCLog::NET, "recv inv old data! time_ms=%lld, i=%d, msg=%s, hash=%s, peer=%s\n",
                    GetTimeMillis(), i, msgName, inv.ToString(), pFrom->addrName);
                fAlreadyHave = true;
            }
            if(chainActive.Tip()->GetBlockTime() < GetTime() - 24 * 60 * 60){
                LogPrint(BCLog::NET, "recv tx inv data when initialBlockDownload,reject it! time_ms=%lld, i=%d, msg=%s, hash=%s, peer=%s\n",
                         GetTimeMillis(), i, msgName, inv.ToString(), pFrom->addrName);
                fAlreadyHave = true;
            }
        } else if (inv.type == MSG_BLOCK) {
            msgName = "MSG_BLOCK";
            auto blockIndexIt = mapBlockIndex.find(inv.hash);
            if (blockIndexIt != mapBlockIndex.end()) {
                LogPrint(BCLog::NET, "recv inv old data! time_ms=%lld, i=%d, msg=%s, hash=%s, peer=%s, found_in=%s, height=%d\n",
                    GetTimeMillis(), i, msgName, inv.ToString(), pFrom->addrName, "BlockIndex", blockIndexIt->second->height);
                fAlreadyHave = true;
            } else {
                auto orphanBlockIt = mapOrphanBlocks.find(inv.hash);
                if (orphanBlockIt != mapOrphanBlocks.end()) {
                    LogPrint(BCLog::NET, "recv inv old data! time_ms=%lld, i=%d, msg=%s, hash=%s, peer=%s, found_in=%s, height=%d\n",
                        GetTimeMillis(), i, msgName, inv.ToString(), pFrom->addrName, "OrphanBlock", orphanBlockIt->second->height);
                    fAlreadyHave = true;

                    LogPrint(BCLog::NET, "recv orphan block and lead to getblocks! height=%d, hash=%s, "
                             "tip_height=%d, tip_hash=%s, peer=%s\n",
                             orphanBlockIt->second->height, inv.hash.GetHex(), chainActive.Height(),
                             chainActive.Tip()->GetBlockHash().GetHex(), pFrom->addrName);
                    PushGetBlocksOnCondition(pFrom, chainActive.Tip(), GetOrphanRoot(inv.hash));
                    // TODO: should get the headmost block of this fork from current peer
                }
            }
        }

        if (!fAlreadyHave) {
            LogPrint(BCLog::NET, "recv msg=%s inv=%s, ts=%lld, i=%d, peer=%s\n", msgName, inv.ToString(),
                                 GetTimeMillis(), i, pFrom->addrName);

            if (!SysCfg().IsImporting() && !SysCfg().IsReindex()) {
                if (inv.type == MSG_BLOCK)
                    AddBlockToQueue(inv.hash, pFrom->GetId());
                else
                    pFrom->AskFor(inv);  // MSG_TX
            }
        }

        if (pFrom->nSendSize > (SendBufferSize() * 2)) {
            Misbehaving(pFrom->GetId(), 50);
            return ERRORMSG("send buffer size() = %u", pFrom->nSendSize);
        }
        i++;
    }
    return true;
}

inline bool ProcessGetDataMessage(CNode *pFrom, CDataStream &vRecv) {
    vector<CInv> vInv;
    vRecv >> vInv;
    if (vInv.size() > MAX_INV_SZ) {
        Misbehaving(pFrom->GetId(), 20);
        return ERRORMSG("message getdata size() = %u from peer %s", vInv.size(), pFrom->addr.ToString());
    }

    if ((vInv.size() != 1))
        LogPrint(BCLog::NET, "received getdata (%u invsz) from peer %s\n", vInv.size(), pFrom->addr.ToString());

    if ((vInv.size() > 0) || (vInv.size() == 1))
        LogPrint(BCLog::NET, "received getdata for: %s from peer %s\n", vInv[0].ToString(), pFrom->addr.ToString());

    pFrom->vRecvGetData.insert(pFrom->vRecvGetData.end(), vInv.begin(), vInv.end());
    ProcessGetData(pFrom);

    return true;
}

inline void ProcessBlockMessage(CNode *pFrom, CDataStream &vRecv) {
    CBlock block;
    vRecv >> block;

    LogPrint(BCLog::NET, "recv block! time_ms=%lld, hash=%s, peer=%s\n", GetTimeMillis(),
        block.GetHash().ToString(), pFrom->addr.ToString());
    // block.Print();

    CInv inv(MSG_BLOCK, block.GetHash());
    pFrom->AddInventoryKnown(inv);

    {
        // Remember who we got this block from.
        LOCK(cs_mapNodeState);
        mapBlockSource[inv.hash] = pFrom->GetId();
        MarkBlockAsReceived(inv.hash, pFrom->GetId());
    }

    LOCK(cs_main);
    CValidationState state;

    std::pair<int32_t ,uint256> globalfinblock = std::make_pair(0,uint256());
    pCdMan->pBlockCache->ReadGlobalFinBlock(globalfinblock);
    if (block.GetHeight() < (uint32_t)globalfinblock.first) {
        LogPrint(BCLog::NET,"[%d] this inbound block is irrreversible (%d)",
                            block.GetHeight(), globalfinblock.first);
    } else {
        ProcessBlock(state, pFrom, &block);
    }

}

inline void ProcessMempoolMessage(CNode *pFrom, CDataStream &vRecv) {
    LOCK2(cs_main, pFrom->cs_filter);

    vector<uint256> vtxid;
    mempool.QueryHash(vtxid);
    vector<CInv> vInv;
    for (auto &hash : vtxid) {
        CInv inv(MSG_TX, hash);
        std::shared_ptr<CBaseTx> pBaseTx = mempool.Lookup(hash);
        if (pBaseTx.get())
            continue;  // another thread removed since queryHashes, maybe...

        if ((pFrom->pFilter && pFrom->pFilter->contains(hash)) ||  // other type transaction
            (!pFrom->pFilter))
            vInv.push_back(inv);

        if (vInv.size() == MAX_INV_SZ) {
            pFrom->PushMessage(NetMsgType::INV, vInv);
            vInv.clear();
        }
    }
    if (vInv.size() > 0) pFrom->PushMessage(NetMsgType::INV, vInv);
}

inline void ProcessAlertMessage(CNode *pFrom, CDataStream &vRecv) {
    CAlert alert;
    vRecv >> alert;

    uint256 alertHash = alert.GetHash();
    if (pFrom->setKnown.count(alertHash) == 0) {
        if (alert.ProcessAlert()) {
            // Relay
            pFrom->setKnown.insert(alertHash);
            {
                LOCK(cs_vNodes);
                for (auto pNode : vNodes) alert.RelayTo(pNode);
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

inline void ProcessFilterLoadMessage(CNode *pFrom, CDataStream &vRecv) {
    CBloomFilter filter;
    vRecv >> filter;

    if (!filter.IsWithinSizeConstraints()) {
        LogPrint(BCLog::INFO, "Misebehaving: filter is not within size constraints, Misbehavior add 100");
        // There is no excuse for sending a too-large filter
        Misbehaving(pFrom->GetId(), 100);
    } else {
        LOCK(pFrom->cs_filter);
        delete pFrom->pFilter;
        pFrom->pFilter = new CBloomFilter(filter);
        pFrom->pFilter->UpdateEmptyFull();
    }
    pFrom->fRelayTxes = true;
}

inline void ProcessFilterAddMessage(CNode *pFrom, CDataStream &vRecv) {
    vector<uint8_t> vData;
    vRecv >> vData;

    // Nodes must NEVER send a data item > 520 bytes (the max size for a script data object,
    // and thus, the maximum size any matched object can have) in a filteradd message
    if (vData.size() > 520)  // MAX_SCRIPT_ELEMENT_SIZE)
    {
        LogPrint(BCLog::INFO, "Misbehaving: send a data item > 520 bytes, Misbehavior add 100");
        Misbehaving(pFrom->GetId(), 100);
    } else {
        LOCK(pFrom->cs_filter);
        if (pFrom->pFilter)
            pFrom->pFilter->insert(vData);
        else {
            LogPrint(BCLog::INFO, "Misbehaving: filter error, Misbehavior add 100");
            Misbehaving(pFrom->GetId(), 100);
        }
    }
}

bool ProcessBlockConfirmMessage(CNode *pFrom, CDataStream &vRecv) {

    if(SysCfg().IsReindex()|| GetTime()-chainActive.Tip()->GetBlockTime()>600){
        LogPrint(BCLog::NET, "local tip's height is too low,drop the confirm message ");
        return false;
    }

    CPBFTMessageMan<CBlockConfirmMessage>& msgMan = pbftContext.confirmMessageMan;
    CBlockConfirmMessage message;
    vRecv >> message;

    LogPrint(BCLog::NET, "received Block confirmedMessage: blockHeight=%d, blockHash=%s, minerid =%s, signature=%s \n",
             message.height, message.blockHash.GetHex(), message.miner.ToString(), HexStr<vector<unsigned char>>(message.vSignature));

    pFrom->AddBlockConfirmMessageKnown(message);

    if(msgMan.IsKnown(message)){
        LogPrint(BCLog::NET, "duplicate confirm message,miner_id=%s, blockhash=%s \n",message.miner.ToString(), message.blockHash.GetHex());
        return false;
    }

    if(!CheckPBFTMessage(PBFTMsgType::CONFIRM_BLOCK,message)){
        LogPrint(BCLog::NET, "confirm message check failed,miner_id=%s, blockhash=%s \n",message.miner.ToString(), message.blockHash.GetHex());
        return false;
    }

    msgMan.AddMessageKnown(message);
    int messageCount = msgMan.SaveMessageByBlock(message.blockHash, message);

    bool updateFinalitySuccess = pbftMan.UpdateLocalFinBlock(message,  messageCount);


    if(CheckPBFTMessageSigner(message))
        RelayBlockConfirmMessage(message);

    if(updateFinalitySuccess){
        BroadcastBlockFinality(pbftMan.GetLocalFinIndex());
    }

    return true;
}


bool ProcessBlockFinalityMessage(CNode *pFrom, CDataStream &vRecv) {


    if(SysCfg().IsReindex()|| GetTime()-chainActive.Tip()->GetBlockTime()>600)
        return false;

    CPBFTMessageMan<CBlockFinalityMessage>& msgMan = pbftContext.finalityMessageMan;
    CBlockFinalityMessage message;
    vRecv >> message;

    LogPrint(BCLog::NET, "received Block FinalityMessage: block=%s, miner=%s, signature=%s \n",
             message.GetBlockId(), message.miner.ToString(), HexStr(message.vSignature));

    pFrom->AddBlockFinalityMessageKnown(message);

    if(msgMan.IsKnown(message)){
        LogPrint(BCLog::NET, "duplicate finality message, miner=%s, block=%s \n",
                 message.miner.ToString(), message.GetBlockId());
        return false;
    }

    if(!CheckPBFTMessage(PBFTMsgType::FINALITY_BLOCK, message)){
        LogPrint(BCLog::NET, "finality block message check failed, miner=%s, block=%s \n",
                 message.miner.ToString(), message.GetBlockId());
        return false;
    }

    msgMan.AddMessageKnown(message);
    int messageCount = msgMan.SaveMessageByBlock(message.blockHash, message);
    pbftMan.UpdateGlobalFinBlock(message, messageCount);

    if(CheckPBFTMessageSigner(message)){
        RelayBlockFinalityMessage(message);
    }


    return true;
}
inline void ProcessRejectMessage(CNode *pFrom, CDataStream &vRecv) {
    if (SysCfg().IsDebug()) {
        string message;
        uint8_t code;
        string strReason;
        vRecv >> message >> code >> strReason;

        ostringstream ss;
        ss << message << " code " << itostr(code) << ": " << strReason;

        if (message == "block" || message == "tx") {
            uint256 hash;
            vRecv >> hash;
            ss << ": hash " << hash.ToString();
        }
        // Truncate to reasonable length and sanitize before printing:
        string s = ss.str();
        if (s.size() > 111)
            s.erase(111, string::npos);

        LogPrint(BCLog::NET, "Reject %s from peer %s\n", SanitizeString(s), pFrom->addr.ToString());
    }
}

#endif  // CHAINMESSAGE_H
