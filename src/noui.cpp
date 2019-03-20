// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The WaykiChain developers
// Copyright (c) 2016 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "noui.h"

#include "ui_interface.h"
#include "util.h"
#include <stdint.h>
#include <string>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"
#include "main.h"
#include "rpc/rpctx.h"
#include "wallet/wallet.h"
#include "init.h"
#include "util.h"
#include "miner.h"
using namespace json_spirit;
#include "cuiserver.h"
#include "net.h"

CCriticalSection cs_Sendbuffer;
deque<string> g_dSendBuffer;


void ThreadSendMessageToUI() {
	RenameThread("send-message-to-ui");
//	int64_t nCurTime = GetTime();
	while(true) {
		{
			LOCK(cs_Sendbuffer);
			if(!g_dSendBuffer.empty()) {
				if (CUIServer::HasConnection()) {
//					nCurTime = GetTime();
					string message = g_dSendBuffer.front();
					g_dSendBuffer.pop_front();
					CUIServer::Send(message);
				}
//				else {
//					if(GetTime() - nCurTime > 120) {   //2 minutes no connection exit ThreadSendMessageToUI thread
//						g_dSendBuffer.clear();
//						break;
//					}
//				}
			}
		}
		MilliSleep(10);
	}
}

void AddMessageToDeque(const std::string &strMessage)
{
    if (SysCfg().GetBoolArg("-ui", false)) {
        LOCK(cs_Sendbuffer);
        g_dSendBuffer.push_back(strMessage);
        LogPrint("msgdeque", "AddMessageToDeque %s\n", strMessage.c_str());
    } else {
        LogPrint("msgdeque", "AddMessageToDeque %s\n", strMessage.c_str());
    }
}

static bool noui_ThreadSafeMessageBox(const std::string& message, const std::string& caption, unsigned int style)
{
	Object obj;
	obj.push_back(Pair("type",     "MessageBox"));
	obj.push_back(Pair("BoxType",     message));

    std::string strCaption;
    // Check for usage of predefined caption
    switch (style) {
    case CClientUIInterface::MSG_ERROR:
      	obj.push_back(Pair("BoxType",     "Error"));
        break;
    case CClientUIInterface::MSG_WARNING:
       	obj.push_back(Pair("BoxType",     "Warning"));
        break;
    case CClientUIInterface::MSG_INFORMATION:
    	obj.push_back(Pair("BoxType",     "Information"));
        break;
    default:
    	obj.push_back(Pair("BoxType",     "unKown"));
        // strCaption += caption;  // Use supplied caption (can be empty)
    }

    AddMessageToDeque(write_string(Value(obj), true));

    fprintf(stderr, "%s: %s\n", strCaption.c_str(), message.c_str());
    return false;
}

static bool noui_SyncTx()
{
	Array arrayObj;
	int nTipHeight = chainActive.Tip()->nHeight;
	int nSyncTxDeep = SysCfg().GetArg("-synctxdeep", 100);
	if(nSyncTxDeep >= nTipHeight) {
		nSyncTxDeep = nTipHeight;
	}
	CBlockIndex *pStartBlockIndex = chainActive[nTipHeight-nSyncTxDeep];

	Object objStartHeight;
	objStartHeight.push_back(Pair("syncheight", pStartBlockIndex->nHeight));
	Object objMsg;
	objMsg.push_back(Pair("type",     "SyncTxHight"));
	objMsg.push_back(Pair("msg",  objStartHeight));
	AddMessageToDeque(write_string(Value(std::move(objMsg)),true));

	while(pStartBlockIndex != NULL) {
		if((pwalletMain->mapInBlockTx).count(pStartBlockIndex->GetBlockHash()) > 0)
		{

			Object objTx;
			CAccountTx acctTx= pwalletMain->mapInBlockTx[pStartBlockIndex->GetBlockHash()];
			map<uint256, std::shared_ptr<CBaseTx> >::iterator iterTx = acctTx.mapAccountTx.begin();
			for(;iterTx != acctTx.mapAccountTx.end(); ++iterTx) {
				objTx = iterTx->second->ToJson(*pAccountViewTip);
				objTx.push_back(Pair("blockhash", pStartBlockIndex->GetBlockHash().ToString()));
				objTx.push_back(Pair("confirmedheight", pStartBlockIndex->nHeight));
				objTx.push_back(Pair("confirmedtime", (int)pStartBlockIndex->nTime));
				Object obj;
				obj.push_back(Pair("type", "SyncTx"));
				obj.push_back(Pair("msg", objTx));
				AddMessageToDeque(write_string(Value(obj), true));
			}
		}
		pStartBlockIndex = chainActive.Next(pStartBlockIndex);
	}
	/*
	map<uint256, CAccountTx>::iterator iterAccountTx = pwalletMain->mapInBlockTx.begin();
	for(; iterAccountTx != pwalletMain->mapInBlockTx.end(); ++iterAccountTx)
	{
		Object objTx;
		map<uint256, std::shared_ptr<CBaseTx> >::iterator iterTx = iterAccountTx->second.mapAccountTx.begin();
		for(;iterTx != iterAccountTx->second.mapAccountTx.end(); ++iterTx) {
			objTx = iterTx->second.get()->ToJson(*pAccountViewTip);
			objTx.push_back(Pair("blockhash", iterAccountTx->first.GetHex()));
			if(mapBlockIndex.count(iterAccountTx->first) && chainActive.Contains(mapBlockIndex[iterAccountTx->first])) {
				objTx.push_back(Pair("confirmedheight", mapBlockIndex[iterAccountTx->first]->nHeight));
				objTx.push_back(Pair("confirmedtime", (int)mapBlockIndex[iterAccountTx->first]->nTime));
			}
			else {
				LogPrint("NOUI", "block hash=%s in wallet map invalid\n", iterAccountTx->first.GetHex());
				continue;
			}
			Object obj;
			obj.push_back(Pair("type",     "SyncTx"));
			obj.push_back(Pair("msg",  objTx));// write_string(Value(arrayObj),true)));
			AddMessageToDeque(write_string(Value(obj), true));
		}
	}
	*/
	map<uint256, std::shared_ptr<CBaseTx> >::iterator iterTx =  pwalletMain->UnConfirmTx.begin();
	for(; iterTx != pwalletMain->UnConfirmTx.end(); ++iterTx)
	{
		Object objTx = iterTx->second.get()->ToJson(*pAccountViewTip);
		arrayObj.push_back(objTx);

		Object obj;
		obj.push_back(Pair("type", "SyncTx"));
		obj.push_back(Pair("msg", objTx));
		AddMessageToDeque(write_string(Value(obj), true));
	}
	return true;
}

static void noui_InitMessage(const std::string &message)
{
	if(message =="initialize end")
	{
		CUIServer::IsInitalEnd = true;
	}
	if("Sync Tx" == message)
	{
		 noui_SyncTx();
		 return;
	}
	Object obj;
	obj.push_back(Pair("type",     "init"));
	obj.push_back(Pair("msg",     message));
	AddMessageToDeque(write_string(Value(obj), true));
}

static void noui_BlockChanged(int64_t time,int64_t high,const uint256 &hash) {

	Object obj;
	obj.push_back(Pair("type",     "blockchanged"));
	obj.push_back(Pair("tips",     nSyncTipHeight));
	obj.push_back(Pair("high",     (int)high));
	obj.push_back(Pair("time",     (int)time));
	obj.push_back(Pair("hash",     hash.ToString()));
    obj.push_back(Pair("connections",   (int)vNodes.size()));
    obj.push_back(Pair("fuelrate", GetElementForBurn(chainActive.Tip())));
    AddMessageToDeque(write_string(Value(obj), true));
}

extern Object GetTxDetailJSON(const uint256& txhash);

static bool noui_RevTransaction(const uint256 &hash){
	Object obj;
	obj.push_back(Pair("type", "revtransaction"));
	obj.push_back(Pair("transaction",     GetTxDetailJSON(hash)));
	AddMessageToDeque(write_string(Value(obj), true));
	return true;
}

static bool noui_RevAppTransaction(const CBlock *pBlock ,int nIndex){
	Object obj;
	obj.push_back(Pair("type", "rev_app_transaction"));
	Object objTx = pBlock->vptx[nIndex].get()->ToJson(*pAccountViewTip);
	objTx.push_back(Pair("blockhash", pBlock->GetHash().GetHex()));
	objTx.push_back(Pair("confirmedheight", (int) pBlock->GetHeight()));
	objTx.push_back(Pair("confirmedtime", (int) pBlock->GetTime()));
	obj.push_back(Pair("transaction", objTx));
	AddMessageToDeque(write_string(Value(obj), true));
	return true;
}

static void noui_NotifyMessage(const std::string &message)
{
	Object obj;
	obj.push_back(Pair("type","notify"));
	obj.push_back(Pair("msg",message));
	AddMessageToDeque(write_string(Value(obj), true));
}

static bool noui_ReleaseTransaction(const uint256 &hash){
	Object obj;
	obj.push_back(Pair("type",     "releasetx"));
	obj.push_back(Pair("hash",   hash.ToString()));
	AddMessageToDeque(write_string(Value(obj), true));
	return true;
}

static bool noui_RemoveTransaction(const uint256 &hash) {
	Object obj;
	obj.push_back(Pair("type",     "rmtx"));
	obj.push_back(Pair("hash",   hash.ToString()));
	AddMessageToDeque(write_string(Value(obj), true));
	return true;
}

void noui_connect()
{
    // Connect Coin signal handlers
	uiInterface.RevTransaction.connect(noui_RevTransaction);
	uiInterface.RevAppTransaction.connect(noui_RevAppTransaction);
    uiInterface.ThreadSafeMessageBox.connect(noui_ThreadSafeMessageBox);
    uiInterface.InitMessage.connect(noui_InitMessage);
    uiInterface.NotifyBlocksChanged.connect(noui_BlockChanged);
    uiInterface.NotifyMessage.connect(noui_NotifyMessage);
    uiInterface.ReleaseTransaction.connect(noui_ReleaseTransaction);
    uiInterface.RemoveTransaction.connect(noui_RemoveTransaction);
}
