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
#include "addrman.h"
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

// Requires cs_mapNodeState.
void MarkBlockAsReceived(const uint256 &hash, NodeId nodeFrom = -1);

// Requires cs_mapNodeState.
void MarkBlockAsInFlight(const uint256 &hash, NodeId nodeId);

struct COrphanBlock {
    uint256 blockHash;
    uint256 prevBlockHash;
    int32_t height;
    vector<uint8_t> vchBlock;
};

static CMedianFilter<int32_t> cPeerBlockCounts(8, 0);

void ProcessGetData(CNode *pFrom);

bool AlreadyHave(const CInv &inv);

// Requires cs_main.
bool AddBlockToQueue(const uint256 &hash, NodeId nodeId);

int32_t ProcessVersionMessage(CNode *pFrom, string strCommand, CDataStream &vRecv);

void ProcessPongMessage(CNode *pFrom, CDataStream &vRecv);

bool ProcessAddrMessage(CNode *pFrom, CDataStream &vRecv);

bool ProcessTxMessage(CNode *pFrom, string strCommand, CDataStream &vRecv);

bool ProcessGetHeadersMessage(CNode *pFrom, CDataStream &vRecv);

void ProcessGetBlocksMessage(CNode *pFrom, CDataStream &vRecv);

bool ProcessInvMessage(CNode *pFrom, CDataStream &vRecv);

bool ProcessGetDataMessage(CNode *pFrom, CDataStream &vRecv);

void ProcessBlockMessage(CNode *pFrom, CDataStream &vRecv);

void ProcessMempoolMessage(CNode *pFrom, CDataStream &vRecv);

void ProcessAlertMessage(CNode *pFrom, CDataStream &vRecv);

void ProcessFilterLoadMessage(CNode *pFrom, CDataStream &vRecv);

void ProcessFilterAddMessage(CNode *pFrom, CDataStream &vRecv);

bool ProcessBlockConfirmMessage(CNode *pFrom, CDataStream &vRecv);


bool ProcessBlockFinalityMessage(CNode *pFrom, CDataStream &vRecv);

void ProcessRejectMessage(CNode *pFrom, CDataStream &vRecv);

#endif  // CHAINMESSAGE_H
