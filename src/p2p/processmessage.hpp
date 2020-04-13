// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PROCESSMESSAGE_HPP
#define PROCESSMESSAGE_HPP

#include "main.h"

bool static ProcessMessage(CNode *pFrom, string strCommand, CDataStream &vRecv) {
    LogPrint(BCLog::NET, "received: %s (%u bytes) from peer %s\n", strCommand, vRecv.size(), pFrom->addr.ToString());
    // RandAddSeedPerfmon();
    // if (GetRand(atoi(SysCfg().GetArg("-dropmessagestest", "0"))) == 0) {
    //     LogPrint(BCLog::INFO, "dropmessagestest DROPPING RECV MESSAGE\n");
    //     return true;
    // }

    {
        LOCK(cs_mapNodeState);
        State(pFrom->GetId())->nLastBlockProcess = GetTimeMicros();
    }

    if (pFrom->nVersion == 0) {
        // Must have a version message before anything else
        Misbehaving(pFrom->GetId(), 1);
        return false;
    }

    if (strCommand == NetMsgType::VERSION) {
        int32_t res = ProcessVersionMessage(pFrom, strCommand, vRecv);
        if (res != -1)
            return res == 1;

    } else if (strCommand == NetMsgType::VERACK) {
        pFrom->SetRecvVersion(min(pFrom->nVersion, PROTOCOL_VERSION));

    } else if (strCommand == NetMsgType::ADDR) {
       if (!ProcessAddrMessage(pFrom, vRecv))
           return false ;

    } else if (strCommand == NetMsgType::INV) {
        if (!ProcessInvMessage(pFrom, vRecv))
            return false ;
    }

    else if (strCommand == NetMsgType::GETDATA) {
        if(!ProcessGetDataMessage(pFrom, vRecv))
            return false ;
    }

    else if (strCommand == NetMsgType::GETBLOCKS) {
        ProcessGetBlocksMessage(pFrom, vRecv);
    }

    else if (strCommand == NetMsgType::GETHEADERS) {
        if (ProcessGetHeadersMessage(pFrom, vRecv))
            return true;
    }

    else if (strCommand == NetMsgType::TX) {
        if (!ProcessTxMessage(pFrom, strCommand, vRecv))
            return false;
    }

    else if (strCommand == NetMsgType::BLOCK &&
            !SysCfg().IsImporting() && !SysCfg().IsReindex())  // Ignore blocks received while importing
    {
        ProcessBlockMessage(pFrom, vRecv);
    }

    else if (strCommand == NetMsgType::GETADDR) {
        pFrom->vAddrToSend.clear();
        vector<CAddress> vAddr = addrman.GetAddr();
        for (const auto &addr : vAddr)
            pFrom->PushAddress(addr);
    }

    else if (strCommand == NetMsgType::MEMPOOL) {
        ProcessMempoolMessage(pFrom, vRecv);
    }

    else if (strCommand == NetMsgType::PING) {
        // Echo the message back with the nonce. This allows for two useful features:
        //
        // 1) A remote node can quickly check if the connection is operational
        // 2) Remote nodes can measure the latency of the network thread. If this node
        //    is overloaded it won't respond to pings quickly and the remote node can
        //    avoid sending us more work, like chain download requests.
        //
        // The nonce stops the remote getting confused between different pings: without
        // it, if the remote node sends a ping once per second and this node takes 5
        // seconds to respond to each, the 5th ping the remote sends would appear to
        // return very quickly.

        uint64_t nonce = 0;
        vRecv >> nonce;

        pFrom->PushMessage(NetMsgType::PONG, nonce);

    }

    else if (strCommand == NetMsgType::PONG) {
       ProcessPongMessage(pFrom, vRecv) ;
    }

    else if (strCommand == NetMsgType::ALERT) {
        ProcessAlertMessage(pFrom, vRecv) ;
    }

    else if (strCommand == NetMsgType::FILTERLOAD) {
        ProcessFilterLoadMessage(pFrom, vRecv);
    }

    else if (strCommand == NetMsgType::FILTERADD) {
        ProcessFilterAddMessage(pFrom, vRecv);
    }

    else if (strCommand == NetMsgType::FILTERCLEAR) {
        LOCK(pFrom->cs_filter);
        delete pFrom->pFilter;
        pFrom->pFilter    = new CBloomFilter();
        pFrom->fRelayTxes = true;
    }

    else if (strCommand == NetMsgType::REJECT) {
        ProcessRejectMessage(pFrom, vRecv);

    } else if (strCommand == NetMsgType::CONFIRMBLOCK) {
        ProcessBlockConfirmMessage(pFrom, vRecv) ;

    } else if (strCommand == NetMsgType::FINALITYBLOCK) {
        ProcessBlockFinalityMessage(pFrom, vRecv);

    } else {
        // Ignore unknown commands for extensibility
    }

    // Update the last seen time for this node's address
    if (pFrom->fNetworkNode)
        if (strCommand == NetMsgType::VERSION ||
            strCommand == NetMsgType::ADDR ||
            strCommand == NetMsgType::INV ||
            strCommand == NetMsgType::GETDATA ||
            strCommand == NetMsgType::PING)
            AddressCurrentlyConnected(pFrom->addr);

    return true;
}

// requires LOCK(cs_vRecvMsg)
bool ProcessMessages(CNode *pFrom) {
    //if (fDebug)
    //    LogPrint(BCLog::INFO,"ProcessMessages(%u messages)\n", pFrom->vRecvMsg.size());

    //
    // Message format
    //  (4) message start
    //  (12) command
    //  (4) size
    //  (4) checksum
    //  (x) data
    //
    bool fOk = true;

    if (!pFrom->vRecvGetData.empty())
        ProcessGetData(pFrom);

    // this maintains the order of responses
    if (!pFrom->vRecvGetData.empty())
        return fOk;

    deque<CNetMessage>::iterator it = pFrom->vRecvMsg.begin();
    while (!pFrom->fDisconnect && it != pFrom->vRecvMsg.end()) {
        // Don't bother if send buffer is too full to respond anyway
        if (pFrom->nSendSize >= SendBufferSize()) {
            LogPrint(BCLog::NET, "send buffer size: %d full for peer: %s\n", pFrom->nSendSize, pFrom->addr.ToString());
            break;
        }

        // get next message
        CNetMessage &msg = *it;

        //if (fDebug)
        //    LogPrint(BCLog::INFO,"ProcessMessages(message %u msgsz, %u bytes, complete:%s)\n",
        //            msg.hdr.nMessageSize, msg.vRecv.size(),
        //            msg.complete() ? "Y" : "N");

        // end, if an incomplete message is found
        if (!msg.complete())
            break;

        // at this point, any failure means we can delete the current message
        it++;

        // Scan for message start
        if (memcmp(msg.hdr.pchMessageStart, SysCfg().MessageStart(), MESSAGE_START_SIZE) != 0) {
            LogPrint(BCLog::INFO, "\n\nPROCESSMESSAGE: INVALID MESSAGESTART\n\n");
            fOk = false;
            break;
        }

        // Read header
        CMessageHeader &hdr = msg.hdr;
        if (!hdr.IsValid()) {
            LogPrint(BCLog::INFO, "\n\nPROCESSMESSAGE: ERRORS IN HEADER %s\n\n\n", hdr.GetCommand());
            continue;
        }
        string strCommand = hdr.GetCommand();

        // Message size
        uint32_t nMessageSize = hdr.nMessageSize;

        // Checksum
        CDataStream &vRecv = msg.vRecv;
        uint256 hash       = Hash(vRecv.begin(), vRecv.begin() + nMessageSize);
        uint32_t nChecksum = 0;
        memcpy(&nChecksum, &hash, sizeof(nChecksum));
        if (nChecksum != hdr.nChecksum) {
            LogPrint(BCLog::INFO, "(%s, %u bytes) : CHECKSUM ERROR nChecksum=%08x hdr.nChecksum=%08x\n",
                     strCommand, nMessageSize, nChecksum, hdr.nChecksum);
            continue;
        }

        // Process message
        bool fRet = false;
        try {
            fRet = ProcessMessage(pFrom, strCommand, vRecv);
            boost::this_thread::interruption_point();
        } catch (std::ios_base::failure &e) {
            pFrom->PushMessage(NetMsgType::REJECT, strCommand, REJECT_MALFORMED, string("error parsing message"));
            if (strstr(e.what(), "end of data")) {
                // Allow exceptions from under-length message on vRecv
                LogPrint(BCLog::INFO, "(%s, %u bytes) : Exception '%s' caught, normally caused by a message being shorter than its stated length\n", strCommand, nMessageSize, e.what());
                LogPrint(BCLog::INFO, "(%s, %u bytes) : %s\n", strCommand, nMessageSize, HexStr(vRecv.begin(), vRecv.end()).c_str());
            } else if (strstr(e.what(), "size too large")) {
                // Allow exceptions from over-long size
                LogPrint(BCLog::INFO, "(%s, %u bytes) : Exception '%s' caught\n", strCommand, nMessageSize, e.what());
            } else {
                PrintExceptionContinue(&e, "ProcessMessages()");
            }
        } catch (boost::thread_interrupted) {
            throw;
        } catch (std::exception &e) {
            PrintExceptionContinue(&e, "ProcessMessages()");
        } catch (...) {
            PrintExceptionContinue(nullptr, "ProcessMessages()");
        }

        if (!fRet)
            LogPrint(BCLog::INFO, "ProcessMessage(%s, %u bytes) FAILED\n", strCommand, nMessageSize);

        break;
    }

    // In case the connection got shut down, its receive buffer was wiped
    if (!pFrom->fDisconnect)
        pFrom->vRecvMsg.erase(pFrom->vRecvMsg.begin(), it);

    return fOk;
}

#endif