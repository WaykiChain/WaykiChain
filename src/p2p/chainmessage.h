//
// Created by yehuan on 2019-08-13.
//

#ifndef WAYKICHAIN_CHAINMESSAGE_H
#define WAYKICHAIN_CHAINMESSAGE_H

#include <string>
#include <vector>
#include "commons/uint256.h"
#include "commons/util.h"
#include "net.h"
#include "main.h"
using namespace std ;

class CNode ;
class CDataStream ;
class CInv ;
class COrphanBlock ;

extern CChain chainActive ;
extern uint256 GetOrphanRoot(const uint256 &hash);

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

void  ProcessGetData(CNode *pFrom);
bool  AlreadyHave(const CInv &inv);
bool  AddBlockToQueue(NodeId nodeid, const uint256 &hash) ;

int   ProcessVersionMessage(CNode *pFrom, string strCommand, CDataStream &vRecv);
void  ProcessPongMessage(CNode *pFrom, CDataStream &vRecv);
bool  ProcessAddrMessage(CNode* pFrom, CDataStream &vRecv);
bool  ProcessGetDataMessage(CNode *pFrom, CDataStream &vRecv);
bool  ProcessInvMessage(CNode *pFrom, CDataStream &vRecv);
void  ProcessGetBlocksMessage(CNode *pFrom, CDataStream &vRecv);
void  ProcessBlockMessage(CNode *pFrom, CDataStream &vRecv);
void  ProcessMempoolMessage(CNode *pFrom, CDataStream &vRecv);
void  ProcessAlertMessage(CNode *pFrom, CDataStream &vRecv);
void  ProcessFilterLoadMessage(CNode *pFrom, CDataStream &vRecv);
void  ProcessFilterAddMessage(CNode *pFrom, CDataStream &vRecv);
void  ProcessRejectMessage(CNode *pFrom, CDataStream &vRecv);
bool  ProcessTxMessage(CNode* pFrom, string strCommand , CDataStream& vRecv);
bool  ProcessGetHeadersMessage(CNode *pFrom, CDataStream &vRecv);


#endif //WAYKICHAIN_CHAINMESSAGE_H
