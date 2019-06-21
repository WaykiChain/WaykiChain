// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2017-2018 WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "commons/base58.h"
#include "rpcserver.h"
#include "init.h"
#include "net.h"
#include "netbase.h"
#include "util.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#include "persistence/blockdb.h"
#include "persistence/txdb.h"
#include "configuration.h"
#include "miner/miner.h"
#include "main.h"
#include "vm/script.h"
#include "vm/vmrunenv.h"
#include <stdint.h>

#include <boost/assign/list_of.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_reader.h"

#include "boost/tuple/tuple.hpp"
#define revert(height) ((height<<24) | (height << 8 & 0xff0000) |  (height>>8 & 0xff00) | (height >> 24))

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

const int MAX_RPC_SIG_STR_LEN = 65 * 1024; // 65K

string RegIDToAddress(CUserID &userId) {
    CKeyID keyId;
    if (pCdMan->pAccountCache->GetKeyId(userId, keyId))
        return keyId.ToAddress();

    return "cannot get address from given RegId";
}

static bool GetKeyId(string const &addr, CKeyID &KeyId) {
    if (!CRegID::GetKeyId(addr, KeyId)) {
        KeyId = CKeyID(addr);
        if (KeyId.IsEmpty())
            return false;
    }
    return true;
}


Object GetTxDetailJSON(const uint256& txhash) {
    Object obj;
    std::shared_ptr<CBaseTx> pBaseTx;
    {
        LOCK(cs_main);
        CBlock genesisblock;
        CBlockIndex* pgenesisblockindex = mapBlockIndex[SysCfg().GetGenesisBlockHash()];
        ReadBlockFromDisk(pgenesisblockindex, genesisblock);
        assert(genesisblock.GetMerkleRootHash() == genesisblock.BuildMerkleTree());
        for (unsigned int i = 0; i < genesisblock.vptx.size(); ++i) {
            if (txhash == genesisblock.GetTxHash(i)) {
                obj = genesisblock.vptx[i]->ToJson(*pCdMan->pAccountCache);
                obj.push_back(Pair("block_hash", SysCfg().GetGenesisBlockHash().GetHex()));
                obj.push_back(Pair("confirmed_height", (int) 0));
                obj.push_back(Pair("confirmed_time", (int) genesisblock.GetTime()));
                CDataStream ds(SER_DISK, CLIENT_VERSION);
                ds << genesisblock.vptx[i];
                obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
                return obj;
            }
        }

        if (SysCfg().IsTxIndex()) {
            CDiskTxPos postx;
            if (pCdMan->pContractCache->ReadTxIndex(txhash, postx)) {
                CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
                CBlockHeader header;
                try {
                    file >> header;
                    fseek(file, postx.nTxOffset, SEEK_CUR);
                    file >> pBaseTx;
                    obj = pBaseTx->ToJson(*pCdMan->pAccountCache);
                    obj.push_back(Pair("confirmedheight", (int) header.GetHeight()));
                    obj.push_back(Pair("confirmedtime", (int) header.GetTime()));
                    obj.push_back(Pair("blockhash", header.GetHash().GetHex()));

                    if (pBaseTx->nTxType == CONTRACT_INVOKE_TX) {
                        vector<CVmOperate> vOutput;
                        pCdMan->pContractCache->ReadTxOutPut(pBaseTx->GetHash(), vOutput);
                        Array outputArray;
                        for (auto& item : vOutput) {
                            outputArray.push_back(item.ToJson());
                        }
                        obj.push_back(Pair("listOutput", outputArray));
                    }
                    CDataStream ds(SER_DISK, CLIENT_VERSION);
                    ds << pBaseTx;
                    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
                } catch (std::exception &e) {
                    throw runtime_error(tfm::format("%s : Deserialize or I/O error - %s", __func__, e.what()).c_str());
                }
                return obj;
            }
        }
        {
            pBaseTx = mempool.Lookup(txhash);
            if (pBaseTx.get()) {
                obj = pBaseTx->ToJson(*pCdMan->pAccountCache);
                CDataStream ds(SER_DISK, CLIENT_VERSION);
                ds << pBaseTx;
                obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
                return obj;
            }
        }
    }
    return obj;
}

Array GetTxAddressDetail(std::shared_ptr<CBaseTx> pBaseTx) {
    Array arrayDetail;
    Object obj;
    std::set<CKeyID> vKeyIdSet;
    auto spCW = std::make_shared<CCacheWrapper>();
    spCW->accountCache.SetBaseView(pCdMan->pAccountCache);
    spCW->contractCache.SetBaseView(pCdMan->pContractCache);

    switch (pBaseTx->nTxType) {
        case BLOCK_REWARD_TX: {
            if (!pBaseTx->GetInvolvedKeyIds(*spCW, vKeyIdSet))
                return arrayDetail;

            obj.push_back(Pair("address", vKeyIdSet.begin()->ToAddress()));
            obj.push_back(Pair("category", "receive"));
            double dAmount = static_cast<double>(pBaseTx->GetValue()) / COIN;
            obj.push_back(Pair("amount", dAmount));
            obj.push_back(Pair("tx_type", "BLOCK_REWARD_TX"));
            arrayDetail.push_back(obj);

            break;
        }
        case ACCOUNT_REGISTER_TX: {
            if (!pBaseTx->GetInvolvedKeyIds(*spCW, vKeyIdSet))
                return arrayDetail;

            obj.push_back(Pair("address", vKeyIdSet.begin()->ToAddress()));
            obj.push_back(Pair("category", "send"));
            double dAmount = static_cast<double>(pBaseTx->GetValue()) / COIN;
            obj.push_back(Pair("amount", -dAmount));
            obj.push_back(Pair("tx_type", "ACCOUNT_REGISTER_TX"));
            arrayDetail.push_back(obj);

            break;
        }
        case BCOIN_TRANSFER_TX: {
            CBaseCoinTransferTx* ptx = (CBaseCoinTransferTx*)pBaseTx.get();
            CKeyID sendKeyID;
            if (ptx->txUid.type() == typeid(CPubKey)) {
                sendKeyID = ptx->txUid.get<CPubKey>().GetKeyId();
            } else if (ptx->txUid.type() == typeid(CRegID)) {
                sendKeyID = ptx->txUid.get<CRegID>().GetKeyId(*pCdMan->pAccountCache);
            }

            CKeyID recvKeyId;
            if (ptx->toUid.type() == typeid(CKeyID)) {
                recvKeyId = ptx->toUid.get<CKeyID>();
            } else if (ptx->toUid.type() == typeid(CRegID)) {
                CRegID desRegID = ptx->toUid.get<CRegID>();
                recvKeyId       = desRegID.GetKeyId(*pCdMan->pAccountCache);
            }

            obj.push_back(Pair("tx_type", "BCOIN_TRANSFER_TX"));
            obj.push_back(Pair("memo", HexStr(ptx->memo)));
            obj.push_back(Pair("address", sendKeyID.ToAddress()));
            obj.push_back(Pair("category", "send"));
            double dAmount = static_cast<double>(pBaseTx->GetValue()) / COIN;
            obj.push_back(Pair("amount", -dAmount));
            arrayDetail.push_back(obj);
            Object objRec;
            objRec.push_back(Pair("tx_type", "BCOIN_TRANSFER_TX"));
            objRec.push_back(Pair("memo", HexStr(ptx->memo)));
            objRec.push_back(Pair("address", recvKeyId.ToAddress()));
            objRec.push_back(Pair("category", "receive"));
            objRec.push_back(Pair("amount", dAmount));
            arrayDetail.push_back(objRec);

            break;
        }
        case CONTRACT_INVOKE_TX: {
            CContractInvokeTx* ptx = (CContractInvokeTx*)pBaseTx.get();
            CKeyID sendKeyID;
            if (ptx->txUid.type() == typeid(CPubKey)) {
                sendKeyID = ptx->txUid.get<CPubKey>().GetKeyId();
            } else if (ptx->txUid.type() == typeid(CRegID)) {
                sendKeyID = ptx->txUid.get<CRegID>().GetKeyId(*pCdMan->pAccountCache);
            }

            CKeyID recvKeyId;
            if (ptx->appUid.type() == typeid(CRegID)) {
                CRegID appUid = ptx->appUid.get<CRegID>();
                recvKeyId     = appUid.GetKeyId(*pCdMan->pAccountCache);
            }

            obj.push_back(Pair("tx_type", "CONTRACT_INVOKE_TX"));
            obj.push_back(Pair("arguments", HexStr(ptx->arguments)));
            obj.push_back(Pair("address", sendKeyID.ToAddress()));
            obj.push_back(Pair("category", "send"));
            double dAmount = static_cast<double>(pBaseTx->GetValue()) / COIN;
            obj.push_back(Pair("amount", -dAmount));
            arrayDetail.push_back(obj);
            Object objRec;
            objRec.push_back(Pair("tx_type", "CONTRACT_INVOKE_TX"));
            objRec.push_back(Pair("arguments", HexStr(ptx->arguments)));
            objRec.push_back(Pair("address", recvKeyId.ToAddress()));
            objRec.push_back(Pair("category", "receive"));
            objRec.push_back(Pair("amount", dAmount));
            arrayDetail.push_back(objRec);

            vector<CVmOperate> vOutput;
            pCdMan->pContractCache->ReadTxOutPut(pBaseTx->GetHash(), vOutput);
            Array outputArray;
            for (auto& item : vOutput) {
                Object objOutPut;
                string address;
                if (item.accountType == ACCOUNT_TYPE::regid) {
                    vector<unsigned char> vRegId(item.accountId, item.accountId + 6);
                    CRegID regId(vRegId);
                    CUserID userId(regId);
                    address = RegIDToAddress(userId);
                } else if (item.accountType == base58addr) {
                    address.assign(item.accountId[0], sizeof(item.accountId));
                }

                objOutPut.push_back(Pair("address", address));

                uint64_t amount;
                memcpy(&amount, item.money, sizeof(item.money));
                double dAmount = amount / COIN;

                if (item.opType == ADD_BCOIN) {
                    objOutPut.push_back(Pair("category", "receive"));
                    objOutPut.push_back(Pair("amount", dAmount));
                } else if (item.opType == MINUS_BCOIN) {
                    objOutPut.push_back(Pair("category", "send"));
                    objOutPut.push_back(Pair("amount", -dAmount));
                }

                if (item.timeoutHeight > 0)
                    objOutPut.push_back(Pair("freezeheight", (int)item.timeoutHeight));

                arrayDetail.push_back(objOutPut);
            }

            break;
        }
        case CONTRACT_DEPLOY_TX:
        case DELEGATE_VOTE_TX: {

            if (!pBaseTx->GetInvolvedKeyIds(*spCW, vKeyIdSet))
                return arrayDetail;

            double dAmount = static_cast<double>(pBaseTx->GetValue()) / COIN;

            obj.push_back(Pair("address", vKeyIdSet.begin()->ToAddress()));
            obj.push_back(Pair("category", "send"));
            obj.push_back(Pair("amount", -dAmount));

            if (pBaseTx->nTxType == CONTRACT_DEPLOY_TX)
                obj.push_back(Pair("tx_type", "CONTRACT_DEPLOY_TX"));
            else if (pBaseTx->nTxType == DELEGATE_VOTE_TX)
                obj.push_back(Pair("tx_type", "DELEGATE_VOTE_TX"));

            arrayDetail.push_back(obj);

            break;
        }
        case COMMON_MTX: {
            CMulsigTx* ptx = (CMulsigTx*)pBaseTx.get();

            CAccount account;
            set<CPubKey> pubKeys;
            for (const auto& item : ptx->signaturePairs) {
                if (!pCdMan->pAccountCache->GetAccount(item.regId, account))
                    return arrayDetail;

                pubKeys.insert(account.pubKey);
            }

            CMulsigScript script;
            script.SetMultisig(ptx->required, pubKeys);
            CKeyID sendKeyId = script.GetID();

            CKeyID recvKeyId;
            if (ptx->desUserId.type() == typeid(CKeyID)) {
                recvKeyId = ptx->desUserId.get<CKeyID>();
            } else if (ptx->desUserId.type() == typeid(CRegID)) {
                CRegID desRegID = ptx->desUserId.get<CRegID>();
                recvKeyId       = desRegID.GetKeyId(*pCdMan->pAccountCache);
            }

            obj.push_back(Pair("tx_type", "COMMON_MTX"));
            obj.push_back(Pair("memo", HexStr(ptx->memo)));
            obj.push_back(Pair("address", sendKeyId.ToAddress()));
            obj.push_back(Pair("category", "send"));
            double dAmount = static_cast<double>(pBaseTx->GetValue()) / COIN;
            obj.push_back(Pair("amount", -dAmount));
            arrayDetail.push_back(obj);
            Object objRec;
            objRec.push_back(Pair("tx_type", "COMMON_MTX"));
            objRec.push_back(Pair("memo", HexStr(ptx->memo)));
            objRec.push_back(Pair("address", recvKeyId.ToAddress()));
            objRec.push_back(Pair("category", "receive"));
            objRec.push_back(Pair("amount", dAmount));
            arrayDetail.push_back(objRec);

            break;
        }
        default:
            break;
    }
    return arrayDetail;
}

Value gettransaction(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "gettransaction \"txhash\"\n"
            "\nget the transaction detail by given transaction hash.\n"
            "\nArguments:\n"
            "1.txhash   (string, required) The hast of transaction.\n"
            "\nResult a object about the transaction detail\n"
            "\nResult:\n"
            "\n\"txhash\"\n"
            "\nExamples:\n" +
            HelpExampleCli("gettransaction",
                           "c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\n") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("gettransaction",
                           "c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\n"));

    uint256 txhash(uint256S(params[0].get_str()));
    std::shared_ptr<CBaseTx> pBaseTx;
    Object obj;
    LOCK(cs_main);
    CBlock genesisblock;
    CBlockIndex* pgenesisblockindex = mapBlockIndex[SysCfg().GetGenesisBlockHash()];
    ReadBlockFromDisk(pgenesisblockindex, genesisblock);
    assert(genesisblock.GetMerkleRootHash() == genesisblock.BuildMerkleTree());
    for (unsigned int i = 0; i < genesisblock.vptx.size(); ++i) {
        if (txhash == genesisblock.GetTxHash(i)) {
            double dAmount = static_cast<double>(genesisblock.vptx.at(i)->GetValue()) / COIN;
            obj.push_back(Pair("amount", dAmount));
            obj.push_back(Pair("confirmations", chainActive.Tip()->nHeight));
            obj.push_back(Pair("block_hash", genesisblock.GetHash().GetHex()));
            obj.push_back(Pair("block_index", (int)i));
            obj.push_back(Pair("block_time", (int)genesisblock.GetTime()));
            obj.push_back(Pair("txid", genesisblock.vptx.at(i)->GetHash().GetHex()));
            obj.push_back(Pair("details", GetTxAddressDetail(genesisblock.vptx.at(i))));
            CDataStream ds(SER_DISK, CLIENT_VERSION);
            ds << genesisblock.vptx[i];
            obj.push_back(Pair("hex", HexStr(ds.begin(), ds.end())));
            return obj;
        }
    }
    bool findTx(false);
    if (SysCfg().IsTxIndex()) {
        CDiskTxPos postx;
        if (pCdMan->pContractCache->ReadTxIndex(txhash, postx)) {
            findTx = true;
            CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
            CBlockHeader header;
            try {
                file >> header;
                fseek(file, postx.nTxOffset, SEEK_CUR);
                file >> pBaseTx;
                double dAmount = static_cast<double>(pBaseTx->GetValue()) / COIN;
                obj.push_back(Pair("amount", dAmount));
                obj.push_back(
                    Pair("confirmations", chainActive.Tip()->nHeight - (int)header.GetHeight()));
                obj.push_back(Pair("blockhash", header.GetHash().GetHex()));
                obj.push_back(Pair("blocktime", (int)header.GetTime()));
                obj.push_back(Pair("txid", pBaseTx->GetHash().GetHex()));
                obj.push_back(Pair("details", GetTxAddressDetail(pBaseTx)));
                CDataStream ds(SER_DISK, CLIENT_VERSION);
                ds << pBaseTx;
                obj.push_back(Pair("hex", HexStr(ds.begin(), ds.end())));
            } catch (std::exception& e) {
                throw runtime_error(
                    tfm::format("%s : Deserialize or I/O error - %s", __func__, e.what()).c_str());
            }
            return obj;
        }
    }

    if (!findTx) {
        pBaseTx = mempool.Lookup(txhash);
        if (pBaseTx == nullptr) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid txhash");
        }
        double dAmount = static_cast<double>(pBaseTx->GetValue()) / COIN;
        obj.push_back(Pair("amount", dAmount));
        obj.push_back(Pair("confirmations", 0));
        obj.push_back(Pair("txid", pBaseTx->GetHash().GetHex()));
        obj.push_back(Pair("details", GetTxAddressDetail(pBaseTx)));
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        ds << pBaseTx;
        obj.push_back(Pair("hex", HexStr(ds.begin(), ds.end())));
    }

    return obj;
}

Value gettxdetail(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "gettxdetail \"txhash\"\n"
            "\nget the transaction detail by given transaction hash.\n"
            "\nArguments:\n"
            "1.txhash   (string,required) The hash of transaction.\n"
            "\nResult an object of the transaction detail\n"
            "\nResult:\n"
            "\n\"txhash\"\n"
            "\nExamples:\n"
            + HelpExampleCli("gettxdetail","c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("gettxdetail","c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\n"));

    uint256 txhash(uint256S(params[0].get_str()));
    return GetTxDetailJSON(txhash);
}

//create a register account tx
Value registeraccounttx(const Array& params, bool fHelp) {
    if (fHelp || params.size() == 0)
        throw runtime_error("registeraccounttx \"addr\" (\"fee\")\n"
            "\nregister local account public key to get its RegId\n"
            "\nArguments:\n"
            "1.addr: (string, required)\n"
            "2.fee: (numeric, optional) pay tx fees to miner\n"
            "\nResult:\n"
            "\"txhash\": (string)\n"
            "\nExamples:\n"
            + HelpExampleCli("registeraccounttx", "n2dha9w3bz2HPVQzoGKda3Cgt5p5Tgv6oj 100000 ")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("registeraccounttx", "n2dha9w3bz2HPVQzoGKda3Cgt5p5Tgv6oj 100000 "));

    string addr = params[0].get_str();
    uint64_t fee = 0;
    uint64_t nDefaultFee = SysCfg().GetTxFee();
    if (params.size() > 1) {
        fee = params[1].get_uint64();
        if (fee < nDefaultFee) {
            throw JSONRPCError(RPC_INSUFFICIENT_FEE,
                               strprintf("Input fee smaller than mintxfee: %ld sawi", nDefaultFee));
        }
    } else {
        fee = nDefaultFee;
    }

    CKeyID keyId;
    if (!GetKeyId(addr, keyId))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Address invalid");

    CAccountRegisterTx rtx;
    assert(pWalletMain != NULL);
    {
        EnsureWalletIsUnlocked();

        CAccountCache view(*pCdMan->pAccountCache);
        CAccount account;
        CUserID userId = keyId;
        if (!pCdMan->pAccountCache->GetAccount(userId, account))
            throw JSONRPCError(RPC_WALLET_ERROR, "Account does not exist");


        if (account.IsRegistered())
            throw JSONRPCError(RPC_WALLET_ERROR, "Account was already registered");

        uint64_t balance = account.GetFreeBcoins();
        if (balance < fee) {
            LogPrint("ERROR", "balance=%d, vs fee=%d", balance, fee);
            throw JSONRPCError(RPC_WALLET_ERROR, "Account balance is insufficient");
        }

        CPubKey pubkey;
        if (!pWalletMain->GetPubKey(keyId, pubkey))
            throw JSONRPCError(RPC_WALLET_ERROR, "Key not found in local wallet");

        CPubKey minerPubKey;
        if (pWalletMain->GetPubKey(keyId, minerPubKey, true)) {
            rtx.minerUid = minerPubKey;
        } else {
            CNullID nullId;
            rtx.minerUid = nullId;
        }
        rtx.txUid        = pubkey;
        rtx.llFees       = fee;
        rtx.nValidHeight = chainActive.Tip()->nHeight;

        if (!pWalletMain->Sign(keyId, rtx.ComputeSignatureHash(), rtx.signature))
            throw JSONRPCError(RPC_WALLET_ERROR, "in registeraccounttx Error: Sign failed.");
    }

    std::tuple<bool, string> ret;
    ret = pWalletMain->CommitTx((CBaseTx *) &rtx);
    if (!std::get<0>(ret))
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));

    Object obj;
    obj.push_back(Pair("hash", std::get<1>(ret)));
    return obj;
}

Value callcontracttx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 5 || params.size() > 6) {
        throw runtime_error(
            "callcontracttx \"sender addr\" \"app regid\" \"arguments\" \"amount\" \"fee\" (\"height\")\n"
            "\ncreate contract invocation transaction\n"
            "\nArguments:\n"
            "1.\"sender addr\": (string, required)  tx sender's base58 addr\n"
            "2.\"app regid\":(string, required)     contract RegId\n"
            "3.\"arguments\": (string, required)    contract arguments (Hex encode required)\n"
            "4.\"amount\":(numeric, required)       amount of WICC to be sent to the contract account\n"
            "5.\"fee\": (numeric, required)         pay to miner\n"
            "6.\"height\": (numeric, optional)      create height,If not provide use the tip block height in chainActive\n"
            "\nResult:\n"
            "\"txhash\": (string)\n"
            "\nExamples:\n" +
            HelpExampleCli("callcontracttx",
                           "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\" \"411994-1\" \"01020304\" 10000 10000 1") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("callcontracttx",
                           "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\", \"411994-1\", \"01020304\", 10000, 10000, 1"));
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type)(int_type)(int_type)(int_type));

    EnsureWalletIsUnlocked();

    CKeyID sendKeyId, recvKeyId;
    if (!GetKeyId(params[0].get_str(), sendKeyId))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid sendaddress");

    if (!GetKeyId(params[1].get_str(), recvKeyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid app regid");
    }

    vector<unsigned char> arguments = ParseHex(params[2].get_str());
    if (arguments.size() >= kContractArgumentMaxSize) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Arguments's size out of range");
    }

    int64_t amount = AmountToRawValue(params[3]);;
    int64_t fee = AmountToRawValue(params[4]);

    int height = chainActive.Tip()->nHeight;
    if (params.size() > 5)
        height = params[5].get_int();

    CPubKey sendPubKey;
    if (!pWalletMain->GetPubKey(sendKeyId, sendPubKey)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Key not found in the local wallet.");
    }

    CUserID sendUserId;
    CRegID sendRegId;
    sendUserId = (pCdMan->pAccountCache->GetRegId(CUserID(sendKeyId), sendRegId) && pCdMan->pAccountCache->RegIDIsMature(sendRegId))
                     ? CUserID(sendRegId)
                     : CUserID(sendPubKey);

    CRegID recvRegId;
    if (!pCdMan->pAccountCache->GetRegId(CUserID(recvKeyId), recvRegId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid app regid");
    }

    if (!pCdMan->pContractCache->HaveScript(recvRegId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to get contract");
    }

    CContractInvokeTx tx;
    tx.nTxType      = CONTRACT_INVOKE_TX;
    tx.txUid        = sendUserId;
    tx.appUid       = recvRegId;
    tx.bcoins       = amount;
    tx.llFees       = fee;
    tx.arguments    = arguments;
    tx.nValidHeight = height;

    if (!pWalletMain->Sign(sendKeyId, tx.ComputeSignatureHash(), tx.signature)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
    }

    std::tuple<bool, string> ret = pWalletMain->CommitTx((CBaseTx*)&tx);
    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));
    }

    Object obj;
    obj.push_back(Pair("hash", std::get<1>(ret)));
    return obj;
}

// register a contract app tx
Value registercontracttx(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 5) {
        throw runtime_error("registercontracttx \"addr\" \"filepath\"\"fee\" (\"height\") (\"appdesc\")\n"
            "\ncreate a transaction of registering a contract app\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) contract owner address from this wallet\n"
            "2.\"filepath\": (string required), the file path of the app script\n"
            "3.\"fee\": (numeric required) pay to miner (the larger the size of script, the bigger fees are required)\n"
            "4.\"height\": (numeric optional) valid height, when not specified, the tip block hegiht in chainActive will be used\n"
            "5.\"appdesc\": (string optional) new app description\n"
            "\nResult:\n"
            "\"txhash\": (string)\n"
            "\nExamples:\n"
            + HelpExampleCli("registercontracttx",
                "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"myapp.lua\" 1000000 (10000) (\"appdesc\")") +
                "\nAs json rpc call\n"
            + HelpExampleRpc("registercontracttx",
                "WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH \"myapp.lua\" 1000000 (10000) (\"appdesc\")"));
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(int_type)(int_type)(str_type));

    string luaScriptFilePath = GetAbsolutePath(params[1].get_str()).string();
    if (luaScriptFilePath.empty())
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_NOT_EXIST, "Lua Script file not exist!");

    if (luaScriptFilePath.compare(0, kContractScriptPathPrefix.size(), kContractScriptPathPrefix.c_str()) != 0)
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_INVALID, "Lua Script file not inside /tmp/lua dir or its subdir!");

    std::tuple<bool, string> result = CVmlua::CheckScriptSyntax(luaScriptFilePath.c_str());
    bool bOK = std::get<0>(result);
    if (!bOK)
        throw JSONRPCError(RPC_INVALID_PARAMETER, std::get<1>(result));

    FILE* file = fopen(luaScriptFilePath.c_str(), "rb+");
    if (!file)
        throw runtime_error("registercontracttx open script file (" + luaScriptFilePath + ") error");

    long lSize;
    fseek(file, 0, SEEK_END);
    lSize = ftell(file);
    rewind(file);

    if (lSize <= 0 || lSize > kContractScriptMaxSize) { // contract script file size must be <= 64 KB)
        fclose(file);
        throw JSONRPCError(
            RPC_INVALID_PARAMETER,
            (lSize == -1) ? "File size is unknown"
                          : ((lSize == 0) ? "File is empty" : "File size exceeds 64 KB limit"));
    }

    // allocate memory to contain the whole file:
    char *buffer = (char*) malloc(sizeof(char) * lSize);
    if (buffer == NULL) {
        fclose(file);
        throw runtime_error("allocate memory failed");
    }
    if (fread(buffer, 1, lSize, file) != (size_t) lSize) {
        free(buffer);
        fclose(file);
        throw runtime_error("read script file error");
    } else {
        fclose(file);
    }

    CVmScript vmScript;
    vmScript.GetRom().insert(vmScript.GetRom().end(), buffer, buffer + lSize);

    if (buffer)
        free(buffer);

    if (params.size() > 4) {
        string memo = params[4].get_str();
        if (memo.size() > kContractMemoMaxSize) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "App desc is too large");
        }
        vmScript.GetMemo().insert(vmScript.GetMemo().end(), memo.begin(), memo.end());
    }

    string contractScript;
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << vmScript;
    contractScript.assign(ds.begin(), ds.end());

    uint64_t fee = params[2].get_uint64();
    int height(0);
    if (params.size() > 3)
        height = params[3].get_int();

    if (fee > 0 && fee < CBaseTx::nMinTxFee) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Fee is smaller than nMinTxFee");
    }
    CKeyID keyId;
    if (!GetKeyId(params[0].get_str(), keyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid send address");
    }

    assert(pWalletMain != NULL);
    CContractDeployTx tx;
    {
        EnsureWalletIsUnlocked();
        CAccountCache view(*pCdMan->pAccountCache);
        CAccount account;

        uint64_t balance = 0;
        CUserID userId   = keyId;
        if (pCdMan->pAccountCache->GetAccount(userId, account)) {
            balance = account.GetFreeBcoins();
        }

        if (!account.IsRegistered()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account is unregistered");
        }
        if (!pWalletMain->HaveKey(keyId)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Send address is not in wallet");
        }
        if (balance < fee) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account balance is insufficient");
        }

        CRegID regId;
        pCdMan->pAccountCache->GetRegId(keyId, regId);

        tx.txUid          = regId;
        tx.contractScript = contractScript;
        tx.llFees         = fee;
        tx.nRunStep       = contractScript.size();
        if (0 == height) {
            height = chainActive.Tip()->nHeight;
        }
        tx.nValidHeight = height;

        if (!pWalletMain->Sign(keyId, tx.ComputeSignatureHash(), tx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }
    }

    std::tuple<bool, string> ret;
    ret = pWalletMain->CommitTx((CBaseTx *) &tx);
    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));
    }
    Object obj;
    obj.push_back(Pair("hash", std::get<1>(ret)));
    return obj;
}

//vote a delegate transaction
Value votedelegatetx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 3 || params.size() > 4) {
        throw runtime_error(
            "votedelegatetx \"sendaddr\" \"votes\" \"fee\" (\"height\") \n"
            "\ncreate a delegate vote transaction\n"
            "\nArguments:\n"
            "1.\"sendaddr\": (string required) The address from which votes are sent to other "
            "delegate addresses\n"
            "2. \"votes\"    (string, required) A json array of votes to delegate candidates\n"
            " [\n"
            "   {\n"
            "      \"delegate\":\"address\", (string, required) The delegate address where votes "
            "are received\n"
            "      \"votes\": n (numeric, required) votes, increase votes when positive or reduce "
            "votes when negative\n"
            "   }\n"
            "       ,...\n"
            " ]\n"
            "3.\"fee\": (numeric required) pay fee to miner\n"
            "4.\"height\": (numeric optional) valid height. When not supplied, the tip block "
            "height in chainActive will be used.\n"
            "\nResult:\n"
            "\"txhash\": (string)\n"
            "\nExamples:\n" +
            HelpExampleCli("votedelegatetx",
                           "\"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\" "
                           "\"[{\\\"delegate\\\":\\\"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\\\", "
                           "\\\"votes\\\":100000000}]\" 10000 ") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("votedelegatetx",
                           " \"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\", "
                           "[{\"delegate\":\"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\", "
                           "\"votes\":100000000}], 10000 "));
    }
    RPCTypeCheck(params, list_of(str_type)(array_type)(int_type)(int_type));

    string sendAddr = params[0].get_str();
    uint64_t fee    = params[2].get_uint64();  // real type
    int nHeight     = 0;
    if (params.size() > 3) {
        nHeight = params[3].get_int();
    }
    Array arrVotes = params[1].get_array();

    CKeyID keyId;
    if (!GetKeyId(sendAddr, keyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid send address");
    }
    CDelegateVoteTx delegateVoteTx;
    assert(pWalletMain != NULL);
    {
        EnsureWalletIsUnlocked();
        CAccountCache view(*pCdMan->pAccountCache);
        CAccount account;

        CUserID userId = keyId;
        if (!pCdMan->pAccountCache->GetAccount(userId, account)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account does not exist");
        }

        if (!account.IsRegistered()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account is unregistered");
        }

        uint64_t balance = account.GetFreeBcoins();
        if (balance < fee) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account balance is insufficient");
        }

        if (!pWalletMain->HaveKey(keyId)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Send address is not in wallet");
        }

        delegateVoteTx.llFees = fee;
        if (0 != nHeight) {
            delegateVoteTx.nValidHeight = nHeight;
        } else {
            delegateVoteTx.nValidHeight = chainActive.Tip()->nHeight;
        }
        delegateVoteTx.txUid = account.regID;

        for (auto objVote : arrVotes) {

            const Value& delegateAddr = find_value(objVote.get_obj(), "delegate");
            const Value& delegateVotes = find_value(objVote.get_obj(), "votes");
            if (delegateAddr.type() == null_type || delegateVotes == null_type) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Vote fund address error or fund value error");
            }
            CKeyID delegateKeyId;
            if (!GetKeyId(delegateAddr.get_str(), delegateKeyId)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Delegate address error");
            }
            CAccount delegateAcct;
            if (!pCdMan->pAccountCache->GetAccount(CUserID(delegateKeyId), delegateAcct)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Delegate address is not exist");
            }
            if (!delegateAcct.IsRegistered()) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Delegate address is unregistered");
            }

            VoteType voteType = (delegateVotes.get_int64() > 0) ? VoteType::ADD_BCOIN : VoteType::MINUS_BCOIN;
            CUserID candidateUid = CUserID(delegateAcct.keyID);
            uint64_t bcoins = (uint64_t)abs(delegateVotes.get_int64());
            CCandidateVote candidateVote(voteType, candidateUid, bcoins);

            delegateVoteTx.candidateVotes.push_back(candidateVote);
        }

        if (!pWalletMain->Sign(keyId, delegateVoteTx.ComputeSignatureHash(), delegateVoteTx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }
    }

    std::tuple<bool, string> ret;
    ret = pWalletMain->CommitTx((CBaseTx*)&delegateVoteTx);
    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));
    }

    Object objRet;
    objRet.push_back(Pair("hash", std::get<1>(ret)));
    return objRet;
}

//create a vote delegate raw transaction
Value genvotedelegateraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() <  3  || params.size() > 4) {
        throw runtime_error(
            "genvotedelegateraw \"addr\" \"opervotes\" \"fee\" \"height\"\n"
            "\nget a vote delegate transaction raw transaction\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) from address that votes delegate(s)\n"
            "2. \"opervotes\"    (string, required) A json array of json oper vote to delegates\n"
            " [\n"
            " {\n"
            "    \"delegate\":\"address\", (string, required) The transaction id\n"
            "    \"votes\":n  (numeric, required) votes\n"
            " }\n"
            "       ,...\n"
            " ]\n"
            "3.\"fee\": (numeric required) pay to miner\n"
            "4.\"height\": (numeric optional) valid height, If not provide, use the tip block hegiht "
            "in chainActive\n"
            "\nResult:\n"
            "\"txhash\": (string)\n"
            "\nExamples:\n" +
            HelpExampleCli("genvotedelegateraw",
                           "\"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\" "
                           "\"[{\\\"delegate\\\":\\\"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\\\", "
                           "\\\"votes\\\":100000000}]\" 1000") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("genvotedelegateraw",
                           " \"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\", "
                           "[{\"delegate\":\"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\", "
                           "\"votes\":100000000}], 1000"));
    }
    RPCTypeCheck(params, list_of(str_type)(array_type)(int_type)(int_type));

    string sendAddr = params[0].get_str();
    uint64_t fee    = params[2].get_uint64();  // real type
    int nHeight     = chainActive.Tip()->nHeight;
    if (params.size() > 3) {
        nHeight = params[3].get_int();
        if (nHeight <= 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid height");
        }
    }
    Array arrVotes = params[1].get_array();

    CKeyID keyId;
    if (!GetKeyId(sendAddr, keyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid send address");
    }
    CDelegateVoteTx delegateVoteTx;
    assert(pWalletMain != NULL);
    {
        EnsureWalletIsUnlocked();
        CAccountCache view(*pCdMan->pAccountCache);
        CAccount account;

        CUserID userId = keyId;
        if (!pCdMan->pAccountCache->GetAccount(userId, account)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account does not exist");
        }

        if (!account.IsRegistered()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account is unregistered");
        }

        uint64_t balance = account.GetFreeBcoins();
        if (balance < fee) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account balance is insufficient");
        }

        if (!pWalletMain->HaveKey(keyId)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Send address is not in wallet");
        }

        delegateVoteTx.llFees       = fee;
        delegateVoteTx.nValidHeight = nHeight;
        delegateVoteTx.txUid        = account.regID;

        for (auto objVote : arrVotes) {

            const Value& delegateAddress = find_value(objVote.get_obj(), "delegate");
            const Value& delegateVotes   = find_value(objVote.get_obj(), "votes");
            if (delegateAddress.type() == null_type || delegateVotes == null_type) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,
                                   "Voted delegator's address type "
                                   "error or vote value error");
            }
            CKeyID delegateKeyId;
            if (!GetKeyId(delegateAddress.get_str(), delegateKeyId)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Voted delegator's address error");
            }
            CAccount delegateAcct;
            if (!pCdMan->pAccountCache->GetAccount(CUserID(delegateKeyId), delegateAcct)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Voted delegator's address is unregistered");
            }

            VoteType voteType = (delegateVotes.get_int64() > 0) ? VoteType::ADD_BCOIN: VoteType::MINUS_BCOIN;
            CUserID candidateUid = CUserID(delegateAcct.keyID);
            uint64_t bcoins = (uint64_t)abs(delegateVotes.get_int64());
            CCandidateVote candidateVote(voteType, candidateUid, bcoins);

            delegateVoteTx.candidateVotes.push_back(candidateVote);
        }

        if (!pWalletMain->Sign(keyId, delegateVoteTx.ComputeSignatureHash(), delegateVoteTx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }
    }

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx = delegateVoteTx.GetNewInstance();
    ds << pBaseTx;
    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
    return obj;
}

Value listaddr(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error(
            "listaddr\n"
            "\nreturn Array containing address,balance,haveminerkey,regid information.\n"
            "\nArguments:\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("listaddr", "")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("listaddr", ""));
    }

    Array retArray;
    assert(pWalletMain != NULL);
    {
        set<CKeyID> setKeyId;
        pWalletMain->GetKeys(setKeyId);
        if (setKeyId.size() == 0) {
            return retArray;
        }
        CAccountCache accView(*pCdMan->pAccountCache);

        for (const auto &keyId : setKeyId) {
            CUserID userId(keyId);
            CAccount acctInfo;
            pCdMan->pAccountCache->GetAccount(userId, acctInfo);
            CKeyCombi keyCombi;
            pWalletMain->GetKeyCombi(keyId, keyCombi);

            Object obj;
            obj.push_back(Pair("addr", keyId.ToAddress()));
            obj.push_back(Pair("balance", (double)acctInfo.GetFreeBcoins()/ (double) COIN));
            obj.push_back(Pair("hasminerkey", keyCombi.HaveMinerKey()));
            obj.push_back(Pair("regid",acctInfo.regID.ToString()));
            retArray.push_back(obj);
        }
    }

    return retArray;
}


Value listtransactions(const Array& params, bool fHelp) {
    if (fHelp || params.size() > 3)
            throw runtime_error(
                "listtransactions ( \"account\" count from includeWatchonly)\n"
                "\nReturns up to 'count' most recent transactions skipping the first 'from' transactions for account 'account'.\n"
                "\nArguments:\n"
                "1. \"address\"    (string, optional) DEPRECATED. The account name. Should be \"*\"\n"
                "2. count          (numeric, optional, default=10) The number of transactions to return\n"
                "3. from           (numeric, optional, default=0) The number of transactions to skip\n"    "\nExamples:\n"
                "\nList the most recent 10 transactions in the systems\n"
                   + HelpExampleCli("listtransactions", "") +
                   "\nList transactions 100 to 120\n"
                   + HelpExampleCli("listtransactions", "\"*\" 20 100") +
                   "\nAs a json rpc call\n"
                   + HelpExampleRpc("listtransactions", "\"*\", 20, 100")
              );
    assert(pWalletMain != NULL);
    string strAddress = "*";
    if (params.size() > 0)
        strAddress = params[0].get_str();
    if ("" == strAddress) {
        strAddress = "*";
    }

    Array arrayData;
    int nCount = -1;
    int nFrom = 0;
    if(params.size() > 1) {
        nCount =  params[1].get_int();
    }
    if(params.size() > 2) {
        nFrom = params[2].get_int();
    }

    LOCK2(cs_main, pWalletMain->cs_wallet);

    map<int, uint256, std::greater<int> > blockInfoMap;
    for (auto const &wtx : pWalletMain->mapInBlockTx) {
        CBlockIndex *pIndex = mapBlockIndex[wtx.first];
        if (pIndex != NULL)
            blockInfoMap.insert(make_pair(pIndex->nHeight, wtx.first));
    }

    int txnCount(0);
    int nIndex(0);
    for (auto const &wtx : blockInfoMap) {
        CAccountTx accountTx = pWalletMain->mapInBlockTx[wtx.second];
        for (auto const & item : accountTx.mapAccountTx) {
            if (item.second->nTxType == BCOIN_TRANSFER_TX) {
                CBaseCoinTransferTx* ptx = (CBaseCoinTransferTx*)item.second.get();
                CKeyID sendKeyID;
                if (ptx->txUid.type() == typeid(CPubKey)) {
                    sendKeyID = ptx->txUid.get<CPubKey>().GetKeyId();
                } else if (ptx->txUid.type() == typeid(CRegID)) {
                    sendKeyID = ptx->txUid.get<CRegID>().GetKeyId(*pCdMan->pAccountCache);
                }

                CKeyID recvKeyId;
                if (ptx->toUid.type() == typeid(CKeyID)) {
                    recvKeyId = ptx->toUid.get<CKeyID>();
                } else if (ptx->toUid.type() == typeid(CRegID)) {
                    CRegID desRegID = ptx->toUid.get<CRegID>();
                    recvKeyId       = desRegID.GetKeyId(*pCdMan->pAccountCache);
                }

                bool bSend = true;
                if ("*" != strAddress && sendKeyID.ToAddress() != strAddress) {
                    bSend = false;
                }

                bool bRecv = true;
                if ("*" != strAddress && recvKeyId.ToAddress() != strAddress) {
                    bRecv = false;
                }

                if(nFrom > 0 && nIndex++ < nFrom) {
                    continue;
                }

                if(!(bSend || bRecv)) {
                    continue;
                }

                if(nCount > 0 && txnCount > nCount) {
                    return arrayData;
                }

                if (bSend) {
                    if (pWalletMain->HaveKey(sendKeyID)) {
                        Object obj;
                        obj.push_back(Pair("address", recvKeyId.ToAddress()));
                        obj.push_back(Pair("category", "send"));
                        double dAmount = static_cast<double>(item.second->GetValue()) / COIN;
                        obj.push_back(Pair("amount", -dAmount));
                        obj.push_back(Pair("confirmations", chainActive.Tip()->nHeight - accountTx.blockHeight));
                        obj.push_back(Pair("blockhash", (chainActive[accountTx.blockHeight]->GetBlockHash().GetHex())));
                        obj.push_back(Pair("blocktime", (int64_t)(chainActive[accountTx.blockHeight]->nTime)));
                        obj.push_back(Pair("txid", item.second->GetHash().GetHex()));
                        obj.push_back(Pair("tx_type", "BCOIN_TRANSFER_TX"));
                        obj.push_back(Pair("memo", HexStr(ptx->memo)));
                        arrayData.push_back(obj);

                        txnCount++;
                    }
                }

                if (bRecv) {
                    if (pWalletMain->HaveKey(recvKeyId)) {
                        Object obj;
                        obj.push_back(Pair("srcaddr", sendKeyID.ToAddress()));
                        obj.push_back(Pair("address", recvKeyId.ToAddress()));
                        obj.push_back(Pair("category", "receive"));
                        double dAmount = static_cast<double>(item.second->GetValue()) / COIN;
                        obj.push_back(Pair("amount", dAmount));
                        obj.push_back(Pair("confirmations", chainActive.Tip()->nHeight - accountTx.blockHeight));
                        obj.push_back(Pair("blockhash", (chainActive[accountTx.blockHeight]->GetBlockHash().GetHex())));
                        obj.push_back(Pair("blocktime", (int64_t)(chainActive[accountTx.blockHeight]->nTime)));
                        obj.push_back(Pair("txid", item.second->GetHash().GetHex()));
                        obj.push_back(Pair("tx_type", "BCOIN_TRANSFER_TX"));
                        obj.push_back(Pair("memo", HexStr(ptx->memo)));

                        arrayData.push_back(obj);

                        txnCount++;
                    }
                }
            } else if (item.second->nTxType == CONTRACT_INVOKE_TX) {
                CContractInvokeTx* ptx = (CContractInvokeTx*)item.second.get();
                CKeyID sendKeyID;
                if (ptx->txUid.type() == typeid(CPubKey)) {
                    sendKeyID = ptx->txUid.get<CPubKey>().GetKeyId();
                } else if (ptx->txUid.type() == typeid(CRegID)) {
                    sendKeyID = ptx->txUid.get<CRegID>().GetKeyId(*pCdMan->pAccountCache);
                }

                CKeyID recvKeyId;
                if (ptx->appUid.type() == typeid(CRegID)) {
                    CRegID appUid = ptx->appUid.get<CRegID>();
                    recvKeyId     = appUid.GetKeyId(*pCdMan->pAccountCache);
                }

                bool bSend = true;
                if ("*" != strAddress && sendKeyID.ToAddress() != strAddress) {
                    bSend = false;
                }

                bool bRecv = true;
                if ("*" != strAddress && recvKeyId.ToAddress() != strAddress) {
                    bRecv = false;
                }

                if(nFrom > 0 && nIndex++ < nFrom) {
                    continue;
                }

                if(!(bSend || bRecv)) {
                    continue;
                }

                if(nCount > 0 && txnCount > nCount) {
                    return arrayData;
                }

                if (bSend) {
                    if (pWalletMain->HaveKey(sendKeyID)) {
                        Object obj;
                        obj.push_back(Pair("address", recvKeyId.ToAddress()));
                        obj.push_back(Pair("category", "send"));
                        double dAmount = static_cast<double>(item.second->GetValue()) / COIN;
                        obj.push_back(Pair("amount", -dAmount));
                        obj.push_back(Pair("confirmations", chainActive.Tip()->nHeight - accountTx.blockHeight));
                        obj.push_back(Pair("blockhash", (chainActive[accountTx.blockHeight]->GetBlockHash().GetHex())));
                        obj.push_back(Pair("blocktime", (int64_t)(chainActive[accountTx.blockHeight]->nTime)));
                        obj.push_back(Pair("txid", item.second->GetHash().GetHex()));
                        obj.push_back(Pair("tx_type", "CONTRACT_INVOKE_TX"));
                        obj.push_back(Pair("arguments", HexStr(ptx->arguments)));

                        arrayData.push_back(obj);

                        txnCount++;
                    }
                }

                if (bRecv) {
                    if (pWalletMain->HaveKey(recvKeyId)) {
                        Object obj;
                        obj.push_back(Pair("srcaddr", sendKeyID.ToAddress()));
                        obj.push_back(Pair("address", recvKeyId.ToAddress()));
                        obj.push_back(Pair("category", "receive"));
                        double dAmount = static_cast<double>(item.second->GetValue()) / COIN;
                        obj.push_back(Pair("amount", dAmount));
                        obj.push_back(Pair("confirmations", chainActive.Tip()->nHeight - accountTx.blockHeight));
                        obj.push_back(Pair("blockhash", (chainActive[accountTx.blockHeight]->GetBlockHash().GetHex())));
                        obj.push_back(Pair("blocktime", (int64_t)(chainActive[accountTx.blockHeight]->nTime)));
                        obj.push_back(Pair("txid", item.second->GetHash().GetHex()));
                        obj.push_back(Pair("tx_type", "CONTRACT_INVOKE_TX"));
                        obj.push_back(Pair("arguments", HexStr(ptx->arguments)));

                        arrayData.push_back(obj);

                        txnCount++;
                    }
                }
            }
            // TODO: COMMON_MTX
        }
    }
    return arrayData;
}

Value listtransactionsv2(const Array& params, bool fHelp) {
    if (fHelp || params.size() > 3)
            throw runtime_error(
                "listtransactionsv2 ( \"account\" count from includeWatchonly)\n"
                "\nReturns up to 'count' most recent transactions skipping the first 'from' transactions for account 'account'.\n"
                "\nArguments:\n"
                "1. \"address\"    (string, optional) DEPRECATED. The account name. Should be \"*\".\n"
                "2. count          (numeric, optional, default=10) The number of transactions to return\n"
                "3. from           (numeric, optional, default=0) The number of transactions to skip\n"    "\nExamples:\n"
                "\nList the most recent 10 transactions in the systems\n"
                   + HelpExampleCli("listtransactionsv2", "") +
                   "\nList transactions 100 to 120\n"
                   + HelpExampleCli("listtransactionsv2", "\"*\" 20 100") +
                   "\nAs a json rpc call\n"
                   + HelpExampleRpc("listtransactionsv2", "\"*\", 20, 100")
              );

    assert(pWalletMain != NULL);
    string strAddress = "*";
    if (params.size() > 0)
        strAddress = params[0].get_str();
    if("" == strAddress) {
        strAddress = "*";
    }

    Array arrayData;
    int nCount = -1;
    int nFrom = 0;
    if(params.size() > 1) {
        nCount =  params[1].get_int();
    }
    if(params.size() > 2) {
        nFrom = params[2].get_int();
    }

    LOCK2(cs_main, pWalletMain->cs_wallet);

    int txnCount(0);
    int nIndex(0);
    CAccountCache accView(*pCdMan->pAccountCache);
    for (auto const &wtx : pWalletMain->mapInBlockTx) {
        for (auto const & item : wtx.second.mapAccountTx) {
            Object obj;
            CKeyID keyId;
            if (item.second.get() && item.second->nTxType == BCOIN_TRANSFER_TX) {
                CBaseCoinTransferTx* ptx = (CBaseCoinTransferTx*)item.second.get();

                if (!pCdMan->pAccountCache->GetKeyId(ptx->txUid, keyId)) {
                    continue;
                }
                string srcAddr = keyId.ToAddress();
                if (!pCdMan->pAccountCache->GetKeyId(ptx->toUid, keyId)) {
                    continue;
                }
                string desAddr = keyId.ToAddress();
                if ("*" != strAddress && desAddr != strAddress) {
                    continue;
                }
                if (nFrom > 0 && nIndex++ < nFrom) {
                    continue;
                }
                if (nCount > 0 && txnCount++ > nCount) {
                    return arrayData;
                }
                obj.push_back(Pair("srcaddr", srcAddr));
                obj.push_back(Pair("desaddr", desAddr));
                double dAmount = static_cast<double>(item.second->GetValue()) / COIN;
                obj.push_back(Pair("amount", dAmount));
                obj.push_back(Pair("confirmations", chainActive.Tip()->nHeight - wtx.second.blockHeight));
                obj.push_back(Pair("blockhash", (chainActive[wtx.second.blockHeight]->GetBlockHash().GetHex())));
                obj.push_back(Pair("blocktime", (int64_t)(chainActive[wtx.second.blockHeight]->nTime)));
                obj.push_back(Pair("txid", item.second->GetHash().GetHex()));
                arrayData.push_back(obj);
            }
        }
    }
    return arrayData;

}

Value listcontracttx(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 3 || params.size() < 1)
            throw runtime_error("listcontracttx ( \"account\" count from )\n"
                "\nReturns up to 'count' most recent transactions skipping the first 'from' transactions for account 'account'.\n"
                "\nArguments:\n"
                "1. \"account\"    (string). The contract RegId. \n"
                "2. count          (numeric, optional, default=10) The number of transactions to return\n"
                "3. from           (numeric, optional, default=0) The number of transactions to skip\n"    "\nExamples:\n"
                "\nList the most recent 10 transactions in the systems\n"
                   + HelpExampleCli("listcontracttx", "") +
                   "\nList transactions 100 to 120\n"
                   + HelpExampleCli("listcontracttx", "\"*\" 20 100") +
                   "\nAs a json rpc call\n"
                   + HelpExampleRpc("listcontracttx", "\"*\", 20, 100")
              );
    assert(pWalletMain != NULL);

    string strRegId = params[0].get_str();
    CRegID regId(strRegId);
    if (regId.IsEmpty() == true) {
        throw runtime_error("in listcontracttx: contractRegId size error!\n");
    }

    if (!pCdMan->pContractCache->HaveScript(regId)) {
        throw runtime_error("in listcontracttx: contractRegId does not exist!\n");
    }

    Array arrayData;
    int nCount = -1;
    int nFrom = 0;
    if (params.size() > 1) {
        nCount = params[1].get_int();
    }
    if (params.size() > 2) {
        nFrom = params[2].get_int();
    }

    auto getregidstring = [&](CUserID const &userId) {
        if(userId.type() == typeid(CRegID))
            return userId.get<CRegID>().ToString();
        return string(" ");
    };

    LOCK2(cs_main, pWalletMain->cs_wallet);

    map<int, uint256, std::greater<int> > blockInfoMap;
    for (auto const &wtx : pWalletMain->mapInBlockTx) {
        CBlockIndex *pIndex = mapBlockIndex[wtx.first];
        if (pIndex != NULL)
            blockInfoMap.insert(make_pair(pIndex->nHeight, wtx.first));
    }

    int txnCount(0);
    int nIndex(0);
    for (auto const &wtx : blockInfoMap) {
        CAccountTx accountTx = pWalletMain->mapInBlockTx[wtx.second];
        for (auto const & item : accountTx.mapAccountTx) {
            if (item.second.get() && item.second->nTxType == CONTRACT_INVOKE_TX) {
                if (nFrom > 0 && nIndex++ < nFrom) {
                    continue;
                }
                if (nCount > 0 && txnCount > nCount) {
                    return arrayData;
                }

                CContractInvokeTx* ptx = (CContractInvokeTx*) item.second.get();
                if (strRegId != getregidstring(ptx->appUid)) {
                    continue;
                }

                CKeyID keyId;
                Object obj;

                CAccountCache accView(*pCdMan->pAccountCache);
                obj.push_back(Pair("hash", ptx->GetHash().GetHex()));
                obj.push_back(Pair("regid",  getregidstring(ptx->txUid)));
                pCdMan->pAccountCache->GetKeyId(ptx->txUid, keyId);
                obj.push_back(Pair("addr",  keyId.ToAddress()));
                obj.push_back(Pair("dest_regid", getregidstring(ptx->appUid)));
                pCdMan->pAccountCache->GetKeyId(ptx->txUid, keyId);
                obj.push_back(Pair("dest_addr", keyId.ToAddress()));
                obj.push_back(Pair("money", ptx->bcoins));
                obj.push_back(Pair("fees", ptx->llFees));
                obj.push_back(Pair("valid_height", ptx->nValidHeight));
                obj.push_back(Pair("arguments", HexStr(ptx->arguments)));
                arrayData.push_back(obj);

                txnCount++;
            }
        }
    }
    return arrayData;
}

Value listtx(const Array& params, bool fHelp) {
if (fHelp || params.size() > 2) {
        throw runtime_error("listtx\n"
                "\nget all confirmed transactions and all unconfirmed transactions from wallet.\n"
                "\nArguments:\n"
                "1. count          (numeric, optional, default=10) The number of transactions to return\n"
                "2. from           (numeric, optional, default=0) The number of transactions to skip\n"
                "\nExamples:\n"
                "\nResult:\n"
                "\nExamples:\n"
                "\nList the most recent 10 transactions in the system\n"
                + HelpExampleCli("listtx", "") +
                "\nList transactions 100 to 120\n"
                + HelpExampleCli("listtx", "20 100")
            );
    }

    Object retObj;
    int nDefCount = 10;
    int nFrom = 0;
    if(params.size() > 0) {
        nDefCount = params[0].get_int();
    }
    if(params.size() > 1) {
        nFrom = params[1].get_int();
    }
    assert(pWalletMain != NULL);

    //Object Inblockobj;
    Array ConfirmTxArry;
    int nCount = 0;
    map<int, uint256, std::greater<int> > blockInfoMap;
    for (auto const &wtx : pWalletMain->mapInBlockTx) {
        CBlockIndex *pIndex = mapBlockIndex[wtx.first];
        if (pIndex != NULL)
            blockInfoMap.insert(make_pair(pIndex->nHeight, wtx.first));
    }
    bool bUpLimited = false;
    for (auto const &blockInfo : blockInfoMap) {
        CAccountTx accountTx = pWalletMain->mapInBlockTx[blockInfo.second];
        for (auto const & item : accountTx.mapAccountTx) {
            if (nFrom-- > 0)
                continue;
            if (++nCount > nDefCount) {
                bUpLimited = true;
                break;
            }
            //Inblockobj.push_back(Pair("tx", item.first.GetHex()));
            ConfirmTxArry.push_back(item.first.GetHex());
        }
        if (bUpLimited) {
            break;
        }
    }
    retObj.push_back(Pair("ConfirmTx", ConfirmTxArry));
    //CAccountCache view(*pCdMan->pAccountCache);
    Array UnConfirmTxArry;
    for (auto const &wtx : pWalletMain->unconfirmedTx) {
        UnConfirmTxArry.push_back(wtx.first.GetHex());
    }
    retObj.push_back(Pair("unconfirmedTx", UnConfirmTxArry));
    return retObj;
}

Value getaccountinfo(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "getaccountinfo \"addr\"\n"
            "\nget account information\n"
            "\nArguments:\n"
            "1.\"addr\": (string, required) account base58 address"
            "Returns account details.\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("getaccountinfo", "\"WT52jPi8DhHUC85MPYK8y8Ajs8J7CshgaB\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("getaccountinfo", "\"WT52jPi8DhHUC85MPYK8y8Ajs8J7CshgaB\""));
    }
    RPCTypeCheck(params, list_of(str_type));
    CKeyID keyId;
    CUserID userId;
    string addr = params[0].get_str();
    if (!GetKeyId(addr, keyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    userId = keyId;
    Object obj;
    {
        CAccount account;
        if (pCdMan->pAccountCache->GetAccount(userId, account)) {
            if (!account.pubKey.IsValid()) {
                CPubKey pk;
                CPubKey minerpk;
                if (pWalletMain->GetPubKey(keyId, pk)) {
                    pWalletMain->GetPubKey(keyId, minerpk, true);
                    account.pubKey = pk;
                    account.keyID  = pk.GetKeyId();
                    if (pk != minerpk && !account.minerPubKey.IsValid()) {
                        account.minerPubKey = minerpk;
                    }
                }
            }
            obj = account.ToJsonObj(true);
            obj.push_back(Pair("position", "inblock"));
        } else {  // unregistered keyId
            CPubKey pk;
            CPubKey minerpk;
            if (pWalletMain->GetPubKey(keyId, pk)) {
                pWalletMain->GetPubKey(keyId, minerpk, true);
                account.pubKey = pk;
                account.keyID  = pk.GetKeyId();
                if (minerpk != pk) {
                    account.minerPubKey = minerpk;
                }
                obj = account.ToJsonObj(true);
                obj.push_back(Pair("position", "inwallet"));
            }
        }
    }
    return obj;
}

//list unconfirmed transaction of mine
Value listunconfirmedtx(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
         throw runtime_error("listunconfirmedtx \n"
                "\nget the list  of unconfirmedtx.\n"
                "\nArguments:\n"
                "\nResult a object about the unconfirm transaction\n"
                "\nResult:\n"
                "\nExamples:\n"
                + HelpExampleCli("listunconfirmedtx", "")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("listunconfirmedtx", ""));
    }

    Object retObj;
    CAccountCache view(*pCdMan->pAccountCache);
    Array UnConfirmTxArry;
    for (auto const &wtx : pWalletMain->unconfirmedTx) {
        UnConfirmTxArry.push_back(wtx.second.get()->ToString(view));
    }
    retObj.push_back(Pair("unconfirmedTx", UnConfirmTxArry));
    return retObj;
}

static Value AccountLogToJson(const CAccountLog &accoutLog) {
    Object obj;
    obj.push_back(Pair("keyId", accoutLog.keyID.ToString()));
    obj.push_back(Pair("bcoins", accoutLog.bcoins));
    // Array array;
    // for (auto const& te : accoutLog.vRewardFund) {
    //     Object obj2;
    //     obj2.push_back(Pair("value", te.value));
    //     obj2.push_back(Pair("nHeight", te.nHeight));
    //     array.push_back(obj2);
    // }
    // obj.push_back(Pair("vRewardFund", array));
    return obj;
}

Value gettxoperationlog(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("gettxoperationlog \"txhash\"\n"
                    "\nget transaction operation log\n"
                    "\nArguments:\n"
                    "1.\"txhash\": (string required) \n"
                    "\nResult:\n"
                    "\"vOperFund\": (string)\n"
                    "\"authorLog\": (string)\n"
                    "\nExamples:\n"
                    + HelpExampleCli("gettxoperationlog",
                            "\"0001a87352387b5b4d6d01299c0dc178ff044f42e016970b0dc7ea9c72c08e2e494a01020304100000\"")
                    + "\nAs json rpc call\n"
                    + HelpExampleRpc("gettxoperationlog",
                            "\"0001a87352387b5b4d6d01299c0dc178ff044f42e016970b0dc7ea9c72c08e2e494a01020304100000\""));
    }
    RPCTypeCheck(params, list_of(str_type));
    uint256 txHash(uint256S(params[0].get_str()));
    vector<CAccountLog> vLog;
    Object retobj;
    retobj.push_back(Pair("hash", txHash.GetHex()));
    if (!GetTxOperLog(txHash, vLog))
        throw JSONRPCError(RPC_INVALID_PARAMETER, "error hash");
    {
        Array arrayvLog;
        for (auto const &te : vLog) {
            Object obj;
            obj.push_back(Pair("addr", te.keyID.ToAddress()));
            Array array;
            array.push_back(AccountLogToJson(te));
            arrayvLog.push_back(obj);
        }
        retobj.push_back(Pair("AccountOperLog", arrayvLog));

    }
    return retobj;

}

static Value TestDisconnectBlock(int number) {
    CBlock block;
    Object obj;

    CValidationState state;
    if ((chainActive.Tip()->nHeight - number) < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "restclient Error: number");
    }
    if (number > 0) {
        do {
            CBlockIndex * pTipIndex = chainActive.Tip();
            LogPrint("DEBUG", "current height:%d\n", pTipIndex->nHeight);
            if (!DisconnectBlockFromTip(state))
                return false;
            chainMostWork.SetTip(pTipIndex->pprev);
            if (!EraseBlockIndexFromSet(pTipIndex))
                return false;
            if (!pCdMan->pBlockTreeDb->EraseBlockIndex(pTipIndex->GetBlockHash()))
                return false;
            mapBlockIndex.erase(pTipIndex->GetBlockHash());
        } while (--number);
    }

    obj.push_back(Pair("tip", strprintf("hash:%s hight:%s",chainActive.Tip()->GetBlockHash().ToString(),chainActive.Tip()->nHeight)));
    return obj;
}

Value disconnectblock(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("disconnectblock \"numbers\"\n"
                "\ndisconnect block\n"
                "\nArguments:\n"
                "1. \"numbers \"  (numeric, required) the block numbers.\n"
                "\nResult:\n"
                "\"disconnect result\"  (bool) \n"
                "\nExamples:\n"
                + HelpExampleCli("disconnectblock", "\"1\"")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("disconnectblock", "\"1\""));
    }
    int number = params[0].get_int();

    Value te = TestDisconnectBlock(number);

    return te;
}

Value resetclient(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error("resetclient\n"
            "\nreset the client such that its blocks and wallet data is purged to none and needs to sync from network again.\n"
            "\nArguments:\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("resetclient", "")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("resetclient", ""));
    }

    Value ret = TestDisconnectBlock(chainActive.Tip()->nHeight);
    if (chainActive.Tip()->nHeight == 0) {
        pWalletMain->CleanAll();
        CBlockIndex* te = chainActive.Tip();
        uint256 hash = te->GetBlockHash();
        // auto ret        = remove_if(mapBlockIndex.begin(), mapBlockIndex.end(),
        //                      [&](std::map<uint256, CBlockIndex*>::reference a) { return (a.first == hash); });
        // mapBlockIndex.erase(ret, mapBlockIndex.end());
        for (auto it = mapBlockIndex.begin(), ite = mapBlockIndex.end(); it != ite;) {
            if (it->first != hash)
                it = mapBlockIndex.erase(it);
            else
                ++it;
        }
        pCdMan->pAccountCache->Flush();
        pCdMan->pContractCache->Flush();

/* TODO:...
        assert(pCdMan->pAccountDb->GetDbCount() == 43);
        assert(pCdMan->pContractDb->GetDbCount() == 0 || pCdMan->pContractDb->GetDbCount() == 1);
        assert(pCdMan->pTxCache->GetSize() == 0);
*/
        CBlock firs = SysCfg().GenesisBlock();
        pWalletMain->SyncTransaction(uint256(), NULL, &firs);
        mempool.Clear();
    } else {
        throw JSONRPCError(RPC_WALLET_ERROR, "restclient Error: Reset failed.");
    }
    return ret;
}

Value listcontracts(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("listcontracts \"showDetail\"\n"
                "\nget the list of all registered contracts\n"
                "\nArguments:\n"
                "1. showDetail  (boolean, required) true to show scriptContent, otherwise to not show it.\n"
                "\nReturn an object contain many script data\n"
                "\nResult:\n"
                "\nExamples:\n"
                + HelpExampleCli("listcontracts", "true")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("listcontracts", "true"));
    }
    bool showDetail = false;
    showDetail = params[0].get_bool();
    Object obj;
    Array arrayScript;

    if (pCdMan->pContractCache != NULL) {
        int nCount(0);
        if (!pCdMan->pContractCache->GetScriptCount(nCount))
            throw JSONRPCError(RPC_DATABASE_ERROR, "get contract error: cannot get registered contract number.");
        CRegID regId;
        string contractScript;
        Object script;
        if (!pCdMan->pContractCache->GetScript(0, regId, contractScript))
            throw JSONRPCError(RPC_DATABASE_ERROR, "get contract error: cannot get registered contract.");
        script.push_back(Pair("contract_regid", regId.ToString()));
        CDataStream ds(contractScript, SER_DISK, CLIENT_VERSION);
        CVmScript vmScript;
        ds >> vmScript;
        // string strDes(vmScript.GetMemo().begin(), vmScript.GetMemo()->end());
        script.push_back(Pair("memo", HexStr(vmScript.GetMemo())));

        if (showDetail)
            script.push_back(Pair("contract", HexStr(vmScript.GetRom().begin(), vmScript.GetRom().end())));

        arrayScript.push_back(script);
        while (pCdMan->pContractCache->GetScript(1, regId, contractScript)) {
            Object obj;
            obj.push_back(Pair("contract_regid", regId.ToString()));
            CDataStream ds(contractScript, SER_DISK, CLIENT_VERSION);
            CVmScript vmScript;
            ds >> vmScript;
            // string strDes(vmScript.GetMemo().begin(), vmScript.GetMemo().end());
            obj.push_back(Pair("memo", HexStr(vmScript.GetMemo())));
            if (showDetail)
                obj.push_back(Pair("contract", HexStr(vmScript.GetRom().begin(), vmScript.GetRom().end())));

            arrayScript.push_back(obj);
        }
    }

    obj.push_back(Pair("contracts", arrayScript));
    return obj;
}

Value getcontractinfo(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
            throw runtime_error(
                "getcontractinfo ( \"contractRegId\" )\n"
                "\nget app information.\n"
                "\nArguments:\n"
                "1. \"contractRegId\"    (string, required) the script ID. \n"
                "\nget app information in the systems\n"
                "\nExamples:\n"
                + HelpExampleCli("getcontractinfo", "123-1")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("getcontractinfo", "123-1"));

    string strRegId = params[0].get_str();
    CRegID regId(strRegId);
    if (regId.IsEmpty() == true) {
        throw runtime_error("in getcontractinfo: contract regid size invalid!\n");
    }

    if (!pCdMan->pContractCache->HaveScript(regId)) {
        throw runtime_error("in getcontractinfo: contract regid not exist!\n");
    }

    string contractScript;
    if (!pCdMan->pContractCache->GetScript(regId, contractScript)) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "get script error: cannot get registered script.");
    }

    Object obj;
    obj.push_back(Pair("contract_regid", regId.ToString()));
    CDataStream ds(contractScript, SER_DISK, CLIENT_VERSION);
    CVmScript vmScript;
    ds >> vmScript;
    obj.push_back(Pair("contract_memo", HexStr(vmScript.GetMemo())));
    obj.push_back(Pair("contract_content", HexStr(vmScript.GetRom().begin(), vmScript.GetRom().end())));
    return obj;
}

Value getaddrbalance(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        string msg = "getaddrbalance nrequired [\"key\",...] ( \"account\" )\n"
                "\nAdd a nrequired-to-sign multisignature address to the wallet.\n"
                "Each key is a  address or hex-encoded public key.\n" + HelpExampleCli("getaddrbalance", "")
                + "\nAs json rpc call\n" + HelpExampleRpc("getaddrbalance", "");
        throw runtime_error(msg);
    }

    assert(pWalletMain != NULL);

    CKeyID keyId;
    if (!GetKeyId(params[0].get_str(), keyId))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid  address");

    double dbalance = 0.0;
    {
        LOCK(cs_main);
        CAccountCache accView(*pCdMan->pAccountCache);
        CAccount secureAcc;
        CUserID userId = keyId;
        if (pCdMan->pAccountCache->GetAccount(userId, secureAcc)) {
            dbalance = (double) secureAcc.GetFreeBcoins() / (double) COIN;
        }
    }
    return dbalance;
}

Value generateblock(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("generateblock \"addr\"\n"
            "\ncreate a block with the appointed address\n"
            "\nArguments:\n"
            "1.\"addr\": (string, required)\n"
            "\nResult:\n"
            "\nblockhash\n"
            "\nExamples:\n" +
            HelpExampleCli("generateblock", "\"5Vp1xpLT8D2FQg3kaaCcjqxfdFNRhxm4oy7GXyBga9\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("generateblock", "\"5Vp1xpLT8D2FQg3kaaCcjqxfdFNRhxm4oy7GXyBga9\""));
    }
    //get keyId
    CKeyID keyId;

    if (!GetKeyId(params[0].get_str(), keyId))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "in generateblock :address err");

//  uint256 hash = CreateBlockWithAppointedAddr(keyId);
//  if (hash.IsNull()) {
//      throw runtime_error("in generateblock :cannot generate block\n");
//  }
    Object obj;
//  obj.push_back(Pair("blockhash", hash.GetHex()));
    return obj;
}

Value listtxcache(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error("listtxcache\n"
                "\nget all transactions in cahce\n"
                "\nArguments:\n"
                "\nResult:\n"
                "\"txcache\"  (string) \n"
                "\nExamples:\n" + HelpExampleCli("listtxcache", "")+ HelpExampleRpc("listtxcache", ""));
    }
    const map<uint256, UnorderedHashSet> &mapBlockTxHashSet = pCdMan->pTxCache->GetTxHashCache();

    Array retTxHashArray;
    for (auto &item : mapBlockTxHashSet) {
        Object blockObj;
        Array txHashArray;
        blockObj.push_back(Pair("blockhash", item.first.GetHex()));
        for (auto &txHash : item.second)
            txHashArray.push_back(txHash.GetHex());
        blockObj.push_back(Pair("txcache", txHashArray));
        retTxHashArray.push_back(blockObj);
    }

    return retTxHashArray;
}

Value reloadtxcache(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error("reloadtxcache \n"
            "\nreload transactions catch\n"
            "\nArguments:\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("reloadtxcache", "")
            + HelpExampleRpc("reloadtxcache", ""));
    }
    pCdMan->pTxCache->Clear();
    CBlockIndex *pIndex = chainActive.Tip();
    if ((chainActive.Tip()->nHeight - SysCfg().GetTxCacheHeight()) >= 0) {
        pIndex = chainActive[(chainActive.Tip()->nHeight - SysCfg().GetTxCacheHeight())];
    } else {
        pIndex = chainActive.Genesis();
    }
    CBlock block;
    do {
        if (!ReadBlockFromDisk(pIndex, block))
            return ERRORMSG("reloadtxcache() : *** ReadBlockFromDisk failed at %d, hash=%s",
                pIndex->nHeight, pIndex->GetBlockHash().ToString());

        pCdMan->pTxCache->AddBlockToCache(block);
        pIndex = chainActive.Next(pIndex);
    } while (NULL != pIndex);

    Object obj;
    obj.push_back(Pair("info", "reload tx cache succeed"));
    return obj;
}

static int GetDataFromAppDb(CContractCache &cache, const CRegID &regId, int pagesize, int index,
        vector<std::tuple<string, string > > &ret) {
    int dbsize = 0;
    int height = chainActive.Height();
    cache.GetContractItemCount(regId, dbsize);
    if (0 == dbsize)
        throw runtime_error("GetDataFromAppDb : the app has NO data!\n");

    string value;
    string scriptKey;

    if (!cache.GetContractData(height, regId, 0, scriptKey, value))
        throw runtime_error("GetContractData : the app data retrieval failed!\n");

    if (index == 1)
        ret.push_back(std::make_tuple(scriptKey, value));

    int readCount(1);
    while (--dbsize) {
        if (cache.GetContractData(height, regId, 1, scriptKey, value)) {
            ++readCount;
            if (readCount > pagesize * (index - 1)) {
                ret.push_back(std::make_tuple(scriptKey, value));
            }
        }
        if (readCount >= pagesize * index) {
            return ret.size();
        }
    }
    return ret.size();
}

Value getcontractdataraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 2 || params.size() > 3) {
        throw runtime_error("getcontractdataraw \"contract_regid\" \"[pagesize or key]\" (\"index\")\n"
            "\nget the contract data (hexadecimal format) by a given app RegID\n"
            "\nArguments:\n"
            "1.\"contract_regid\": (string, required) App RegId\n"
            "2.[pagesize or key]: (pagesize int, required),if only two params,it is key, otherwise it is pagesize\n"
            "3.\"index\": (int optional)\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("getcontractdataraw", "\"1304166-1\" \"key\"")
            + HelpExampleRpc("getcontractdataraw", "\"1304166-1\" \"key\""));
    }

    CRegID regId(params[0].get_str());
    if (regId.IsEmpty())
        throw runtime_error("getcontractdataraw : app regid not supplied!");

    if (!pCdMan->pContractCache->HaveScript(regId))
        throw runtime_error("getcontractdataraw : app regid does NOT exist!");

    Object script;
    int height = chainActive.Height();
    CContractCache contractScriptTemp(*pCdMan->pContractCache);
    if (params.size() == 2) {
        vector<unsigned char> hex = ParseHex(params[1].get_str());
        string key(hex.begin(), hex.end());
        string value;
        if (!contractScriptTemp.GetContractData(height, regId, key, value)) {
            throw runtime_error("the key does NOT exist!");
        }
        script.push_back(Pair("regid", params[0].get_str()));
        script.push_back(Pair("key", HexStr(key)));
        script.push_back(Pair("value", HexStr(value)));
        return script;

    } else {
        int dbsize = 0;
        contractScriptTemp.GetContractItemCount(regId, dbsize);
        if (0 == dbsize) {
            throw runtime_error("the contract has NO data!");
        }
        int pagesize = params[1].get_int();
        int index = params[2].get_int();

        vector<std::tuple<string, string>> ret;
        GetDataFromAppDb(contractScriptTemp, regId, pagesize, index, ret);

        Array retArray;
        for (auto te : ret) {
            string key = std::get<0>(te);
            string value = std::get<1>(te);
            Object firt;
            firt.push_back(Pair("key", HexStr(key)));
            firt.push_back(Pair("value", HexStr(value)));
            retArray.push_back(firt);
        }
        return retArray;
    }
    return script;
}

Value getcontractdata(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 2 || params.size() > 3) {
        throw runtime_error("getcontractdata \"contract_regid\" \"[pagesize or key]\" (\"index\")\n"
            "\nget the contract data (original input format) by a given contract RegID\n"
            "\nArguments:\n"
            "1.\"contract_regid\": (string, required) Contract RegId\n"
            "2.[pagesize or key]: (pagesize int, required),if only two params,it is key, otherwise it is pagesize\n"
            "3.\"index\": (int optional)\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("getcontractdata", "\"1304166-1\" \"key\"")
            + HelpExampleRpc("getcontractdata", "\"1304166-1\" \"key\""));
    }
    int height = chainActive.Height();
    // RPCTypeCheck(params, list_of(str_type)(int_type)(int_type));
    CRegID regId(params[0].get_str());
    if (regId.IsEmpty()) {
        throw runtime_error("contract regid NOT supplied!");
    }

    if (!pCdMan->pContractCache->HaveScript(regId)) {
        throw runtime_error("contract regid NOT exist!");
    }
    Object script;

    if (params.size() == 2) {
        string strKey = params[1].get_str();
        string key = strprintf("%c", strKey.length());
        std::copy(strKey.begin(), strKey.end(), key.begin());

        string value;
        if (!pCdMan->pContractCache->GetContractData(height, regId, key, value)) {
            throw runtime_error("the key does NOT exist!");
        }
        script.push_back(Pair("regid", params[0].get_str()));
        script.push_back(Pair("key", strKey));
        script.push_back(Pair("value", value));

        return script;

    } else {
        int dbsize = 0;
        pCdMan->pContractCache->GetContractItemCount(regId, dbsize);
        if (0 == dbsize) {
            throw runtime_error("the contract has NO data!");
        }
        int pagesize = params[1].get_int();
        int index = params[2].get_int();

        vector<std::tuple<string, string>> ret;
        GetDataFromAppDb(*pCdMan->pContractCache, regId, pagesize, index, ret);

        Array retArray;
        for (auto te : ret) {
            string key   = std::get<0>(te);
            string value = std::get<1>(te);

            Object retObj;
            retObj.push_back(Pair("key", key));
            retObj.push_back(Pair("value", value));

            retArray.push_back(retObj);
        }

        return retArray;
    }

    return script;
}

Value getcontractconfirmdata(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 3 && params.size() !=4)) {
        throw runtime_error("getcontractconfirmdata \"regid\" \"pagesize\" \"index\"\n"
            "\nget script valid data\n"
            "\nArguments:\n"
            "1.\"regid\": (string, required) app RegId\n"
            "2.\"pagesize\": (int, required)\n"
            "3.\"index\": (int, required )\n"
            "4.\"minconf\":  (numeric, optional, default=1) Only include contract transactions confirmed \n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("getcontractconfirmdata", "\"1304166-1\" \"1\"  \"1\"")
            + HelpExampleRpc("getcontractconfirmdata", "\"1304166-1\" \"1\"  \"1\""));
    }
    std::shared_ptr<CContractCache> pAccountCache;
    if (4 == params.size() && 0 == params[3].get_int()) {
        pAccountCache.reset(new CContractCache(*mempool.memPoolContractCache.get()));
    } else {
        pAccountCache.reset(new CContractCache(*pCdMan->pContractCache));
    }
    int height = chainActive.Height();
    RPCTypeCheck(params, list_of(str_type)(int_type)(int_type));
    CRegID regId(params[0].get_str());
    if (regId.IsEmpty() == true)
        throw runtime_error("getcontractdata :appregid NOT found!");

    if (!pAccountCache->HaveScript(regId))
        throw runtime_error("getcontractdata :appregid does NOT exist!");

    Object obj;
    int pagesize = params[1].get_int();
    int nIndex = params[2].get_int();

    int nKey = revert(height);
    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << nKey;
    string scriptKey(ds.begin(), ds.end());
    string value;
    Array retArray;
    int nReadCount = 0;

    while (pAccountCache->GetContractData(height, regId, 1, scriptKey, value)) {
        Object item;
        ++nReadCount;
        if (nReadCount > pagesize * (nIndex - 1)) {
            item.push_back(Pair("key", HexStr(scriptKey)));
            item.push_back(Pair("value", HexStr(value)));
            retArray.push_back(item);
        }
        if (nReadCount >= pagesize * nIndex) {
            break;
        }
    }

    return retArray;
}

Value saveblocktofile(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2) {
        throw runtime_error("saveblocktofile \"blockhash\" \"filepath\"\n"
                "\n save the given block info to the given file\n"
                "\nArguments:\n"
                "1.\"blockhash\": (string, required)\n"
                "2.\"filepath\": (string, required)\n"
                "\nResult:\n"
                "\nExamples:\n"
                + HelpExampleCli("saveblocktofile", "\"12345678901211111\" \"block.log\"")
                + HelpExampleRpc("saveblocktofile", "\"12345678901211111\" \"block.log\""));
    }
    string strblockhash = params[0].get_str();
    uint256 blockHash(uint256S(params[0].get_str()));
    if(0 == mapBlockIndex.count(blockHash)) {
        throw JSONRPCError(RPC_MISC_ERROR, "block hash is not exist!");
    }
    CBlockIndex *pIndex = mapBlockIndex[blockHash];
    CBlock blockInfo;
    if (!pIndex || !ReadBlockFromDisk(pIndex, blockInfo))
        throw runtime_error(_("Failed to read block"));
    assert(strblockhash == blockInfo.GetHash().ToString());
    string file = params[1].get_str();
    try {
        FILE* fp = fopen(file.c_str(), "wb+");
        CAutoFile fileout = CAutoFile(fp, SER_DISK, CLIENT_VERSION);
        if (!fileout)
            throw JSONRPCError(RPC_MISC_ERROR, "open file:" + strblockhash + "failed!");
        if(chainActive.Contains(pIndex))
            fileout << pIndex->nHeight;
        fileout << blockInfo;
        fflush(fileout);
    } catch (std::exception &e) {
        throw JSONRPCError(RPC_MISC_ERROR, "save block to file error");
    }
    return "save succeed";
}

Value getcontractitemcount(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("getcontractitemcount \"regid\"\n"
            "\nget the total number of contract db K-V items\n"
            "\nArguments:\n"
            "1.\"regid\": (string, required) Contract RegId\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("getcontractitemcount", "\"258988-1\"")
            + HelpExampleRpc("getcontractitemcount","\"258988-1\"")
        );
    }

    CRegID regId(params[0].get_str());
    if (regId.IsEmpty()) {
        throw runtime_error("contract RegId invalid!");
    }
    if (!pCdMan->pContractCache->HaveScript(regId)) {
        throw runtime_error("contract with the given RegId does NOT exist!");
    }

    int nItemCount = 0;
    if (!pCdMan->pContractCache->GetContractItemCount(regId, nItemCount)) {
        throw runtime_error("GetContractItemCount error");
    }
    return nItemCount;
}

Value genregisteraccountraw(const Array& params, bool fHelp) {
    if (fHelp || (params.size() < 3 || params.size() > 4)) {
        throw runtime_error(
            "genregisteraccountraw \"fee\" \"height\" \"publickey\" (\"minerpublickey\") \n"
            "\ncreate a register account transaction\n"
            "\nArguments:\n"
            "1.fee: (numeric, required) pay to miner\n"
            "2.height: (numeric, required)\n"
            "3.publickey: (string, required)\n"
            "4.minerpublickey: (string, optional)\n"
            "\nResult:\n"
            "\"txhash\": (string)\n"
            "\nExamples:\n" +
            HelpExampleCli(
                "genregisteraccountraw",
                "10000  3300 "
                "\"038f679e8b63d6f9935e8ca6b7ce1de5257373ac5461874fc794004a8a00a370ae\" "
                "\"026bc0668c767ab38a937cb33151bcf76eeb4034bcb75e1632fd1249d1d0b32aa9\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc(
                "genregisteraccountraw",
                " 10000 3300 "
                "\"038f679e8b63d6f9935e8ca6b7ce1de5257373ac5461874fc794004a8a00a370ae\" "
                "\"026bc0668c767ab38a937cb33151bcf76eeb4034bcb75e1632fd1249d1d0b32aa9\""));
    }
    CUserID userId  = CNullID();
    CUserID minerId = CNullID();

    int64_t fee         = AmountToRawValue(params[0]);
    int64_t nDefaultFee = SysCfg().GetTxFee();

    if (fee < nDefaultFee) {
        throw JSONRPCError(RPC_INSUFFICIENT_FEE,
                           strprintf("Input fee smaller than mintxfee: %ld sawi", nDefaultFee));
    }

    int height = params[1].get_int();
    if (height <= 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid height");
    }

    CPubKey pubKey = CPubKey(ParseHex(params[2].get_str()));
    if (!pubKey.IsCompressed() || !pubKey.IsFullyValid()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid public key");
    }
    userId = pubKey;

    if (params.size() > 3) {
        CPubKey minerPubKey = CPubKey(ParseHex(params[3].get_str()));
        if (!minerPubKey.IsCompressed() || !minerPubKey.IsFullyValid()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid public key");
        }
        minerId = minerPubKey;
    }

    EnsureWalletIsUnlocked();
    std::shared_ptr<CAccountRegisterTx> tx =
        std::make_shared<CAccountRegisterTx>(userId, minerId, fee, height);
    if (!pWalletMain->Sign(pubKey.GetKeyId(), tx->ComputeSignatureHash(), tx->signature)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");
    }
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
    ds << pBaseTx;
    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
    return obj;
}

Value sendtxraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "sendtxraw \"transaction\" \n"
            "\nsend raw transaction\n"
            "\nArguments:\n"
            "1.\"transaction\": (string, required)\n"
            "\nExamples:\n"
            + HelpExampleCli("sendtxraw", "\"n2dha9w3bz2HPVQzoGKda3Cgt5p5Tgv6oj\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("sendtxraw", "\"n2dha9w3bz2HPVQzoGKda3Cgt5p5Tgv6oj\""));
    }
    vector<unsigned char> vch(ParseHex(params[0].get_str()));
    if (vch.size() > MAX_RPC_SIG_STR_LEN) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "The rawtx str is too long");
    }

    CDataStream stream(vch, SER_DISK, CLIENT_VERSION);

    std::shared_ptr<CBaseTx> tx;
    stream >> tx;
    std::tuple<bool, string> ret;
    ret = pWalletMain->CommitTx((CBaseTx *) tx.get());
    if (!std::get<0>(ret))
        throw JSONRPCError(RPC_WALLET_ERROR, "sendtxraw error: " + std::get<1>(ret));

    Object obj;
    obj.push_back(Pair("hash", std::get<1>(ret)));
    return obj;
}

Value gencallcontractraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 5 || params.size() > 6) {
        throw runtime_error(
            "gencallcontractraw \"sender addr\" \"app regid\" \"amount\" \"contract\" \"fee\" (\"height\")\n"
            "\ncreate contract invocation raw transaction\n"
            "\nArguments:\n"
            "1.\"sender addr\": (string, required)\n tx sender's base58 addr\n"
            "2.\"app regid\":(string, required) contract RegId\n"
            "3.\"arguments\": (string, required) contract arguments (Hex encode required)\n"
            "4.\"amount\":(numeric, required)\n amount of WICC to be sent to the contract account\n"
            "5.\"fee\": (numeric, required) pay to miner\n"
            "6.\"height\": (numeric, optional)create height,If not provide use the tip block height in "
            "chainActive\n"
            "\nResult:\n"
            "\"rawtx\"  (string) The raw transaction\n"
            "\nExamples:\n" +
            HelpExampleCli("gencallcontractraw",
                           "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\" \"411994-1\" \"01020304\" 10000 10000 1") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("gencallcontractraw",
                           "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\", \"411994-1\", \"01020304\", 10000, 10000, 1"));
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(int_type)(str_type)(int_type)(int_type));

    EnsureWalletIsUnlocked();

    CKeyID sendKeyId, recvKeyId;
    if (!GetKeyId(params[0].get_str(), sendKeyId))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid sendaddress");

    if (!GetKeyId(params[1].get_str(), recvKeyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid app regid");
    }

    vector<unsigned char> arguments = ParseHex(params[2].get_str());
    if (arguments.size() >= kContractArgumentMaxSize) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Arguments's size out of range");
    }

    int64_t amount = AmountToRawValue(params[3]);;
    int64_t fee = AmountToRawValue(params[4]);

    int height = chainActive.Tip()->nHeight;
    if (params.size() > 5)
        height = params[5].get_int();

    CPubKey sendPubKey;
    if (!pWalletMain->GetPubKey(sendKeyId, sendPubKey)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Key not found in the local wallet.");
    }

    CUserID sendUserId;
    CRegID sendRegId;
    sendUserId = (pCdMan->pAccountCache->GetRegId(CUserID(sendKeyId), sendRegId) && pCdMan->pAccountCache->RegIDIsMature(sendRegId))
                     ? CUserID(sendRegId)
                     : CUserID(sendPubKey);

    CRegID recvRegId;
    if (!pCdMan->pAccountCache->GetRegId(CUserID(recvKeyId), recvRegId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid app regid");
    }

    if (!pCdMan->pContractCache->HaveScript(recvRegId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to get contract");
    }

    CContractInvokeTx tx;
    tx.nTxType      = CONTRACT_INVOKE_TX;
    tx.txUid        = sendUserId;
    tx.appUid       = recvRegId;
    tx.bcoins       = amount;
    tx.llFees       = fee;
    tx.arguments    = arguments;
    tx.nValidHeight = height;

    if (!pWalletMain->Sign(sendKeyId, tx.ComputeSignatureHash(), tx.signature)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
    }

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx = tx.GetNewInstance();
    ds << pBaseTx;
    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

    return obj;
}

Value genregistercontractraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 3 || params.size() > 5) {
        throw runtime_error(
            "genregistercontractraw \"addr\" \"filepath\" \"fee\"  \"height\" (\"script description\")\n"
            "\nregister script\n"
            "\nArguments:\n"
            "1.\"addr\": (string required)\n from address that registers the contract"
            "2.\"filepath\": (string required), script's file path\n"
            "3.\"fee\": (numeric required) pay to miner\n"
            "4.\"height\": (int optional) valid height\n"
            "5.\"script description\":(string optional) new script description\n"
            "\nResult:\n"
            "\"txhash\": (string)\n"
            "\nExamples:\n"
            + HelpExampleCli("genregistercontractraw",
                    "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"/tmp/lua/hello.lua\" \"10000\" ")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("genregistercontractraw",
                    "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\", \"/tmp/lua/hello.lua\", \"10000\" "));
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(int_type)(int_type)(str_type));

    CVmScript vmScript;
    string contractScript;
    string luaScriptFilePath = GetAbsolutePath(params[1].get_str()).string();
    if (luaScriptFilePath.empty())
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_NOT_EXIST, "Lua Script file not exist!");

    if (luaScriptFilePath.compare(0, kContractScriptPathPrefix.size(), kContractScriptPathPrefix.c_str()) != 0)
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_INVALID, "Lua Script file not inside /tmp/lua dir or its subdir!");

    FILE* file = fopen(luaScriptFilePath.c_str(), "rb+");
    if (!file)
        throw runtime_error("genregistercontractraw open App Lua Script file" + luaScriptFilePath + "error");

    long lSize;
    fseek(file, 0, SEEK_END);
    lSize = ftell(file);
    rewind(file);

    // allocate memory to contain the whole file:
    char *buffer = (char*) malloc(sizeof(char) * lSize);
    if (buffer == NULL) {
        fclose(file);
        throw runtime_error("allocate memory failed");
    }

    if (fread(buffer, 1, lSize, file) != (size_t) lSize) {
        free(buffer);
        fclose(file);
        throw runtime_error("read contract script file error");
    } else {
        fclose(file);
    }
    vmScript.GetRom().insert(vmScript.GetRom().end(), buffer, buffer + lSize);
    if (buffer)
        free(buffer);

    CDataStream dsScript(SER_DISK, CLIENT_VERSION);
    dsScript << vmScript;

    contractScript.assign(dsScript.begin(), dsScript.end());

    if (params.size() > 4) {
        string memo = params[4].get_str();
        vmScript.GetMemo().insert(vmScript.GetMemo().end(), memo.begin(), memo.end());
    }

    uint64_t fee = params[2].get_uint64();
    if (fee > 0 && fee < CBaseTx::nMinTxFee) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Fee smaller than nMinTxFee");
    }
    CKeyID keyId;
    if (!GetKeyId(params[0].get_str(), keyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Recv address invalid");
    }

    CAccountCache view(*pCdMan->pAccountCache);
    CAccount account;

    CUserID userId = keyId;
    if (!pCdMan->pAccountCache->GetAccount(userId, account)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Account does not exist");
    }
    if (!account.IsRegistered()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Account is unregistered");
    }

    std::shared_ptr<CContractDeployTx> tx = std::make_shared<CContractDeployTx>();
    CRegID regId;
    pCdMan->pAccountCache->GetRegId(keyId, regId);

    tx.get()->txUid          = regId;
    tx.get()->contractScript = contractScript;
    tx.get()->llFees         = fee;

    uint32_t height = chainActive.Tip()->nHeight;
    if (params.size() > 3) {
        height =  params[3].get_int();
        if (height <= 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid height");
        }
    }
    tx.get()->nValidHeight = height;

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
    ds << pBaseTx;

    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
    return obj;
}

Value signtxraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2) {
        throw runtime_error(
            "signtxraw \"str\" \"addr\"\n"
            "\nsignature transaction\n"
            "\nArguments:\n"
            "1.\"str\": (string, required) Hex-format string, no longer than 65K in binary bytes\n"
            "2.\"addr\": (string, required) A json array of WICC addresses\n"
            "[\n"
            "  \"address\"  (string) WICC address\n"
            "  ...,\n"
            "]\n"
            "\nExamples:\n" +
            HelpExampleCli("signtxraw",
                           "\"0701ed7f0300030000010000020002000bcd10858c200200\" "
                           "\"[\\\"wKwPHfCJfUYZyjJoa6uCVdgbVJkhEnguMw\\\", "
                           "\\\"wQT2mY1onRGoERTk4bgAoAEaUjPLhLsrY4\\\", "
                           "\\\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\\\"]\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("signtxraw",
                           "\"0701ed7f0300030000010000020002000bcd10858c200200\", "
                           "\"[\\\"wKwPHfCJfUYZyjJoa6uCVdgbVJkhEnguMw\\\", "
                           "\\\"wQT2mY1onRGoERTk4bgAoAEaUjPLhLsrY4\\\", "
                           "\\\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\\\"]\""));
    }

    vector<unsigned char> vch(ParseHex(params[0].get_str()));
    if (vch.size() > MAX_RPC_SIG_STR_LEN) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "The sig str is too long");
    }

    CDataStream stream(vch, SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx;
    stream >> pBaseTx;
    if (!pBaseTx.get()) {
        return Value::null;
    }

    const Array& addresses = params[1].get_array();
    if (pBaseTx.get()->nTxType != COMMON_MTX && addresses.size() != 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "To many addresses provided");
    }

    std::set<CKeyID> keyIds;
    CKeyID keyId;
    for (unsigned int i = 0; i < addresses.size(); i++) {
        if (!GetKeyId(addresses[i].get_str(), keyId)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Failed to get keyId");
        }
        keyIds.insert(keyId);
    }

    if (keyIds.empty()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No valid address provided");
    }

    Object obj;
    switch (pBaseTx.get()->nTxType) {
        case BCOIN_TRANSFER_TX: {
            std::shared_ptr<CBaseCoinTransferTx> tx = std::make_shared<CBaseCoinTransferTx>(pBaseTx.get());
            if (!pWalletMain->Sign(*keyIds.begin(), tx.get()->ComputeSignatureHash(), tx.get()->signature))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");

            CDataStream ds(SER_DISK, CLIENT_VERSION);
            std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
            ds << pBaseTx;
            obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

            break;
        }

        case ACCOUNT_REGISTER_TX: {
            std::shared_ptr<CAccountRegisterTx> tx =
                std::make_shared<CAccountRegisterTx>(pBaseTx.get());
            if (!pWalletMain->Sign(*keyIds.begin(), tx.get()->ComputeSignatureHash(), tx.get()->signature))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");

            CDataStream ds(SER_DISK, CLIENT_VERSION);
            std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
            ds << pBaseTx;
            obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

            break;
        }

        case CONTRACT_INVOKE_TX: {
            std::shared_ptr<CContractInvokeTx> tx = std::make_shared<CContractInvokeTx>(pBaseTx.get());
            if (!pWalletMain->Sign(*keyIds.begin(), tx.get()->ComputeSignatureHash(), tx.get()->signature)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");
            }
            CDataStream ds(SER_DISK, CLIENT_VERSION);
            std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
            ds << pBaseTx;
            obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

            break;
        }

        case BLOCK_REWARD_TX: {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Block reward transation is forbidden");
        }

        case CONTRACT_DEPLOY_TX: {
            std::shared_ptr<CContractDeployTx> tx =
                std::make_shared<CContractDeployTx>(pBaseTx.get());
            if (!pWalletMain->Sign(*keyIds.begin(), tx.get()->ComputeSignatureHash(), tx.get()->signature)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");
            }
            CDataStream ds(SER_DISK, CLIENT_VERSION);
            std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
            ds << pBaseTx;
            obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

            break;
        }

        case DELEGATE_VOTE_TX: {
            std::shared_ptr<CDelegateVoteTx> tx = std::make_shared<CDelegateVoteTx>(pBaseTx.get());
            if (!pWalletMain->Sign(*keyIds.begin(), tx.get()->ComputeSignatureHash(), tx.get()->signature)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");
            }
            CDataStream ds(SER_DISK, CLIENT_VERSION);
            std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
            ds << pBaseTx;
            obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

            break;
        }

        case COMMON_MTX: {
            std::shared_ptr<CMulsigTx> tx = std::make_shared<CMulsigTx>(pBaseTx.get());

            vector<CSignaturePair>& signaturePairs = tx.get()->signaturePairs;
            for (const auto& keyIdItem : keyIds) {
                CRegID regId;
                if (!pCdMan->pAccountCache->GetRegId(CUserID(keyIdItem), regId)) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Address is unregistered");
                }

                bool valid = false;
                for (auto& signatureItem : signaturePairs) {
                    if (regId == signatureItem.regId) {
                        if (!pWalletMain->Sign(keyIdItem, tx.get()->ComputeSignatureHash(),
                                               signatureItem.signature)) {
                            throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");
                        } else {
                            valid = true;
                        }
                    }
                }

                if (!valid) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Provided address is unmatched");
                }
            }

            CDataStream ds(SER_DISK, CLIENT_VERSION);
            std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
            ds << pBaseTx;
            obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

            break;
        }

        default: {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Unsupported transaction type");
        }
    }
    return obj;
}

Value decodemulsigscript(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "decodemulsigscript \"hex\"\n"
            "\nDecode a hex-encoded script.\n"
            "\nArguments:\n"
            "1. \"hex\"     (string) the hex encoded mulsig script\n"
            "\nResult:\n"
            "{\n"
            "  \"type\":\"type\", (string) The transaction type\n"
            "  \"reqSigs\": n,    (numeric) The required signatures\n"
            "  \"addr\",\"address\" (string) mulsig script address\n"
            "  \"addresses\": [   (json array of string)\n"
            "     \"address\"     (string) bitcoin address\n"
            "     ,...\n"
            "  ]\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("decodemulsigscript", "\"hexstring\"") +
            HelpExampleRpc("decodemulsigscript", "\"hexstring\""));

    RPCTypeCheck(params, list_of(str_type));

    vector<unsigned char> multiScript = ParseHex(params[0].get_str());
    if (multiScript.empty() || multiScript.size() > KMultisigScriptMaxSize) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid script size");
    }

    CDataStream ds(multiScript, SER_DISK, CLIENT_VERSION);
    CMulsigScript script;
    try {
        ds >> script;
    } catch (std::exception& e) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid script content");
    }

    CKeyID scriptId           = script.GetID();
    int8_t required           = (int8_t)script.GetRequired();
    std::set<CPubKey> pubKeys = script.GetPubKeys();

    Array addressArray;
    for (const auto& pubKey : pubKeys) {
        addressArray.push_back(pubKey.GetKeyId().ToAddress());
    }

    Object obj;
    obj.push_back(Pair("type", "mulsig"));
    obj.push_back(Pair("req_sigs", required));
    obj.push_back(Pair("addr", scriptId.ToAddress()));
    obj.push_back(Pair("addresses", addressArray));

    return obj;
}

Value decodetxraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "decodetxraw \"hexstring\"\n"
            "\ndecode transaction\n"
            "\nArguments:\n"
            "1.\"str\": (string, required) hexstring\n"
            "\nExamples:\n" +
            HelpExampleCli("decodetxraw",
                           "\"03015f020001025a0164cd10004630440220664de5ec373f44d2756a23d5267ab25f2"
                           "2af6162d166b1cca6c76631701cbeb5022041959ff75f7c7dd39c1f9f6ef9a237a6ea46"
                           "7d02d2d2c3db62a1addaa8009ccd\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("decodetxraw",
                           "\"03015f020001025a0164cd10004630440220664de5ec373f44d2756a23d5267ab25f2"
                           "2af6162d166b1cca6c76631701cbeb5022041959ff75f7c7dd39c1f9f6ef9a237a6ea46"
                           "7d02d2d2c3db62a1addaa8009ccd\""));
    }
    Object obj;
    vector<unsigned char> vch(ParseHex(params[0].get_str()));
    CDataStream stream(vch, SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx;
    stream >> pBaseTx;
    if (!pBaseTx.get()) {
        return obj;
    }

    CAccountCache view(*pCdMan->pAccountCache);
    switch (pBaseTx.get()->nTxType) {
        case BCOIN_TRANSFER_TX: {
            std::shared_ptr<CBaseCoinTransferTx> tx = std::make_shared<CBaseCoinTransferTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(view);
            }
            break;
        }
        case ACCOUNT_REGISTER_TX: {
            std::shared_ptr<CAccountRegisterTx> tx =
                std::make_shared<CAccountRegisterTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(view);
            }
            break;
        }
        case CONTRACT_INVOKE_TX: {
            std::shared_ptr<CContractInvokeTx> tx = std::make_shared<CContractInvokeTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(view);
            }
            break;
        }
        case BLOCK_REWARD_TX: {
            std::shared_ptr<CBlockRewardTx> tx = std::make_shared<CBlockRewardTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(view);
            }
            break;
        }
        case CONTRACT_DEPLOY_TX: {
            std::shared_ptr<CContractDeployTx> tx =
                std::make_shared<CContractDeployTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(view);
            }
            break;
        }
        case DELEGATE_VOTE_TX: {
            std::shared_ptr<CDelegateVoteTx> tx = std::make_shared<CDelegateVoteTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(view);
            }
            break;
        }
        case COMMON_MTX: {
            std::shared_ptr<CMulsigTx> tx = std::make_shared<CMulsigTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(view);
            }
            break;
        }
        default:
            break;
    }
    return obj;
}

Value getalltxinfo(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 0 && params.size() != 1)) {
        throw runtime_error("getalltxinfo \n"
            "\nget all transaction info\n"
            "\nArguments:\n"
            "1.\"nlimitCount\": (numeric, optional, default=0) 0 return all tx, else return number of nlimitCount txs \n"
            "\nResult:\n"
            "\nExamples:\n" + HelpExampleCli("getalltxinfo", "") + "\nAs json rpc call\n"
            + HelpExampleRpc("getalltxinfo", ""));
    }

    Object retObj;
    int nLimitCount(0);
    if(params.size() == 1)
        nLimitCount = params[0].get_int();
    assert(pWalletMain != NULL);
    if (nLimitCount <= 0) {
        Array confirmedTx;
        for (auto const &wtx : pWalletMain->mapInBlockTx) {
            for (auto const & item : wtx.second.mapAccountTx) {
                Object objtx = GetTxDetailJSON(item.first);
                confirmedTx.push_back(objtx);
            }
        }
        retObj.push_back(Pair("confirmed", confirmedTx));

        Array unconfirmedTx;
        CAccountCache view(*pCdMan->pAccountCache);
        for (auto const &wtx : pWalletMain->unconfirmedTx) {
            Object objtx = GetTxDetailJSON(wtx.first);
            unconfirmedTx.push_back(objtx);
        }
        retObj.push_back(Pair("unconfirmed", unconfirmedTx));
    } else {
        Array confirmedTx;
        multimap<int, Object, std::greater<int> > mapTx;
        for (auto const &wtx : pWalletMain->mapInBlockTx) {
            for (auto const & item : wtx.second.mapAccountTx) {
                Object objtx = GetTxDetailJSON(item.first);
                int nConfHeight = find_value(objtx, "confirmedheight").get_int();
                mapTx.insert(pair<int, Object>(nConfHeight, objtx));
            }
        }
        int nSize(0);
        for(auto & txItem : mapTx) {
            if(++nSize > nLimitCount)
                break;
            confirmedTx.push_back(txItem.second);
        }
        retObj.push_back(Pair("Confirmed", confirmedTx));
    }

    return retObj;
}

Value printblockdbinfo(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error(
            "printblockdbinfo \n"
            "\nprint block log\n"
            "\nArguments:\n"
            "\nExamples:\n" + HelpExampleCli("printblockdbinfo", "")
            + HelpExampleRpc("printblockdbinfo", ""));
    }

    if (!pCdMan->pAccountCache->Flush())
        throw runtime_error("Failed to write to account database\n");
    if (!pCdMan->pContractCache->Flush())
        throw runtime_error("Failed to write to account database\n");
    WriteBlockLog(false, "");
    return Value::null;
}

Value getcontractaccountinfo(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 2 && params.size() != 3)) {
        throw runtime_error("getcontractaccountinfo \"contract_regid\" \"account_address | account_regid\""
            "\nget contract account info\n"
            "\nArguments:\n"
            "1.\"contract_regid\":(string, required) App RegId\n"
            "2.\"account_address or regid\": (string, required) contract account address or its regid\n"
            "3.\"minconf\"  (numeric, optional, default=1) Only include contract transactions confirmed \n"
            "\nExamples:\n"
            + HelpExampleCli("getcontractaccountinfo", "\"452974-3\" \"WUZBQZZqyWgJLvEEsHrXL5vg5qaUwgfjco\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("getcontractaccountinfo", "\"452974-3\" \"WUZBQZZqyWgJLvEEsHrXL5vg5qaUwgfjco\""));
    }

    string strAppRegId = params[0].get_str();
    if (!CRegID::IsSimpleRegIdStr(strAppRegId))
        throw runtime_error("getcontractaccountinfo: invalid contract regid: " + strAppRegId);

    CRegID appRegId(strAppRegId);

    string acctKey;
    if (CRegID::IsSimpleRegIdStr(params[1].get_str())) {
        CRegID acctRegId(params[1].get_str());
        CUserID acctUserId(acctRegId);
        acctKey = RegIDToAddress(acctUserId);
    } else { //in wicc address format
        acctKey = params[1].get_str();
    }

    std::shared_ptr<CAppUserAccount> appUserAccount = std::make_shared<CAppUserAccount>();
    if (params.size() == 3 && params[2].get_int() == 0) {
        if (!mempool.memPoolContractCache->GetScriptAcc(appRegId, acctKey, *appUserAccount.get())) {
            appUserAccount = std::make_shared<CAppUserAccount>(acctKey);
        }
    } else {
        if (!pCdMan->pContractCache->GetScriptAcc(appRegId, acctKey, *appUserAccount.get())) {
            appUserAccount = std::make_shared<CAppUserAccount>(acctKey);
        }
    }
    appUserAccount.get()->AutoMergeFreezeToFree(chainActive.Tip()->nHeight);

    return Value(appUserAccount.get()->ToJson());
}

Value listcontractassets(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("listcontractassets regid\n"
            "\nreturn Array containing address, asset information.\n"
            "\nArguments: regid: Contract RegId\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("listcontractassets", "1-1")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("listcontractassets", "1-1"));
    }

    if (!CRegID::IsSimpleRegIdStr(params[0].get_str()))
        throw runtime_error("in listcontractassets :regid is invalid!\n");

    CRegID script(params[0].get_str());

    Array retArray;
    assert(pWalletMain != NULL);
    {
        set<CKeyID> setKeyId;
        pWalletMain->GetKeys(setKeyId);
        if (setKeyId.size() == 0)
            return retArray;

        CContractCache contractScriptTemp(*pCdMan->pContractCache);

        for (const auto &keyId : setKeyId) {

            string key = keyId.ToAddress();

            std::shared_ptr<CAppUserAccount> tem = std::make_shared<CAppUserAccount>();
            if (!contractScriptTemp.GetScriptAcc(script, key, *tem.get())) {
                tem = std::make_shared<CAppUserAccount>(key);
            }
            tem.get()->AutoMergeFreezeToFree(chainActive.Tip()->nHeight);

            Object obj;
            obj.push_back(Pair("addr", key));
            obj.push_back(Pair("asset", (double) tem.get()->GetBcoins() / (double) COIN));
            retArray.push_back(obj);
        }
    }

    return retArray;
}


Value gethash(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("gethash  \"str\"\n"
            "\nget the hash of given str\n"
            "\nArguments:\n"
            "1.\"str\": (string, required) \n"
            "\nresult an object \n"
            "\nExamples:\n"
            + HelpExampleCli("gethash", "\"0000001000005zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("gethash", "\"0000001000005zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG\""));
    }

    string str = params[0].get_str();
    vector<unsigned char> vTemp;
    vTemp.assign(str.c_str(), str.c_str() + str.length());
    uint256 strhash = Hash(vTemp.begin(), vTemp.end());
    Object obj;
    obj.push_back(Pair("hash", strhash.ToString()));
    return obj;

}

Value getcontractkeyvalue(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2) {
        throw runtime_error("getcontractkeyvalue  \"regid\" \"array\""
            "\nget contract key value\n"
            "\nArguments:\n"
            "1.\"regid\": (string, required) \n"
            "2.\"array\": (string, required) \n"
            "\nExamples:\n"
            + HelpExampleCli("getcontractkeyvalue", "\"1651064-1\" \"Wgim6agki6CmntK4LVs3QnCKbeQ2fsgqWP\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("getcontractkeyvalue", "\"1651064-1\" \"Wgim6agki6CmntK4LVs3QnCKbeQ2fsgqWP\""));
    }

    CRegID contractRegId(params[0].get_str());
    Array array = params[1].get_array();

    int height = chainActive.Height();

    if (contractRegId.IsEmpty())
        throw runtime_error("in getcontractkeyvalue: contract regid size is error!\n");

    if (!pCdMan->pContractCache->HaveScript(contractRegId))
        throw runtime_error("in getcontractkeyvalue: contract regid not exist!\n");

    Array retArray;
    CContractCache contractScriptTemp(*pCdMan->pContractCache);

    for (size_t i = 0; i < array.size(); i++) {
        uint256 txhash(uint256S(array[i].get_str()));
        string key(txhash.begin(), txhash.end());
        string value;

        Object obj;
        if (!contractScriptTemp.GetContractData(height, contractRegId, key, value)) {
            obj.push_back(Pair("key", array[i].get_str()));
            obj.push_back(Pair("value", HexStr(value)));
        } else {
            obj.push_back(Pair("key", array[i].get_str()));
            obj.push_back(Pair("value", HexStr(value)));
        }

        std::shared_ptr<CBaseTx> pBaseTx;
        int time = 0;
        int height = 0;
        if (SysCfg().IsTxIndex()) {
            CDiskTxPos postx;
            if (pCdMan->pContractCache->ReadTxIndex(txhash, postx)) {
                CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
                CBlockHeader header;
                try {
                    file >> header;
                    fseek(file, postx.nTxOffset, SEEK_CUR);
                    file >> pBaseTx;
                    height = header.GetHeight();
                    time = header.GetTime();
                } catch (std::exception &e) {
                    throw runtime_error(tfm::format("%s : Deserialize or I/O error - %s", __func__, e.what()).c_str());
                }
            }
        }

        obj.push_back(Pair("confirmedheight", (int) height));
        obj.push_back(Pair("confirmedtime", (int) time));
        retArray.push_back(obj);
    }

    return retArray;
}

Value validateaddr(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "validateaddr \"wicc_address\"\n"
            "\ncheck whether address is valid or not\n"
            "\nArguments:\n"
            "1. \"wicc_address\"  (string, required) WICC address\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("validateaddr", "\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\"") +
            HelpExampleRpc("validateaddr", "\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\""));

    Object obj;
    CKeyID keyId;
    string addr = params[0].get_str();
    if (!GetKeyId(addr, keyId)) {
        obj.push_back(Pair("is_valid", false));
    } else {
        obj.push_back(Pair("is_valid", true));
    }
    return obj;
}

Value gettotalcoins(const Array& params, bool fHelp) {
    if(fHelp || params.size() != 0) {
        throw runtime_error(
            "gettotalcoins \n"
            "\nget the total number of circulating coins excluding those locked for votes\n"
            "\nand the toal number of registered addresses\n"
            "\nArguments:\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("gettotalcoins", "")
            + HelpExampleRpc("gettotalcoins", ""));
    }

    Object obj;
    {
        uint64_t totalCoins(0);
        uint64_t totalRegIds(0);
        // CAccountCache *view = pCdMan->pAccountCache;
        std::tie(totalCoins, totalRegIds) = pCdMan->pAccountCache->TraverseAccount();
        // auto [totalCoins, totalRegIds] = pCdMan->pAccountCache->TraverseAccount(); //C++17
        obj.push_back( Pair("total_coins", ValueFromAmount(totalCoins)) );
        obj.push_back( Pair("total_regids", totalRegIds) );
    }
    return obj;
}

Value gettotalassets(const Array& params, bool fHelp) {
    if(fHelp || params.size() != 1) {
        throw runtime_error("gettotalassets \n"
            "\nget all assets belonging to a contract\n"
            "\nArguments:\n"
            "1.\"contract_regid\": (string, required)\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("gettotalassets", "11-1")
            + HelpExampleRpc("gettotalassets", "11-1"));
    }
    CRegID regId(params[0].get_str());
    if (regId.IsEmpty() == true)
        throw runtime_error("contract regid invalid!\n");

    if (!pCdMan->pContractCache->HaveScript(regId))
        throw runtime_error("contract regid not exist!\n");

    Object obj;
    {
        map<string, string> mapAcc;
        bool bRet = pCdMan->pContractCache->GetAllContractAcc(regId, mapAcc);
        if (bRet) {
            uint64_t totalassets = 0;
            for (auto & it : mapAcc) {
                CAppUserAccount appAccOut;
                CDataStream ds(it.second, SER_DISK, CLIENT_VERSION);
                ds >> appAccOut;

                totalassets += appAccOut.GetBcoins();
                totalassets += appAccOut.GetAllFreezedValues();
            }

            obj.push_back(Pair("total_assets", ValueFromAmount(totalassets)));
        } else
            throw runtime_error("failed to find contract account!\n");
    }
    return obj;
}

Value listtxbyaddr(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2) {
        throw runtime_error(
            "listtxbyaddr \n"
            "\nlist all transactions by their sender/receiver addresss\n"
            "\nArguments:\n"
            "1.\"address\": (string, required)\n"
            "2.\"height\": (numeric, required)\n"
            "\nResult: address related tx hash as array\n"
            "\nExamples:\n" +
            HelpExampleCli("listtxbyaddr",
                           "\"wcoA7yUW4fc4m6a2HSk36t4VVxzKUnvq4S\" \"10000\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("listtxbyaddr",
                           "\"wcoA7yUW4fc4m6a2HSk36t4VVxzKUnvq4S\", \"10000\""));
    }

    string address = params[0].get_str();
    int height     = params[1].get_int();
    if (height < 0 || height > chainActive.Height())
        throw runtime_error("Height out of range.");

    CKeyID keyId;
    if (!GetKeyId(address, keyId))
        throw runtime_error("Address invalid.");

    map<string, string> mapTxHash;
    if (!pCdMan->pContractCache->GetTxHashByAddress(keyId, height, mapTxHash))
        throw runtime_error("Failed to fetch data.");

    Object obj;
    Array arrayObj;
    for (auto item : mapTxHash) {
        arrayObj.push_back(item.second);
    }
    obj.push_back(Pair("address", address));
    obj.push_back(Pair("height", height));
    obj.push_back(Pair("tx_array", arrayObj));

    return obj;
}

Value listdelegates(const Array& params, bool fHelp) {
    if (fHelp || params.size() > 1) {
        throw runtime_error(
                "listdelegates \n"
                "\nreturns the specified number delegates by reversed order voting number.\n"
                "\nArguments:\n"
                "1. number           (number, optional) the number of the delegates, default to all delegates.\n"
                "\nResult:\n"
                "\nExamples:\n"
                + HelpExampleCli("listdelegates", "11")
                + HelpExampleRpc("listdelegates", "11"));
    }

    int delegateNum = (params.size() == 1) ? params[0].get_int() : IniCfg().GetTotalDelegateNum();
    if (delegateNum < 1 || delegateNum > 11) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           strprintf("Delegate number not between 1 and %u", IniCfg().GetTotalDelegateNum()));
    }

    vector<CRegID> delegatesList;
    if (!pCdMan->pDelegateCache->GetTopDelegates(delegatesList)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to get delegates list");
    }

    Object obj;
    Array delegateArray;

    delegateNum = min(delegateNum, (int)delegatesList.size());
    CAccount account;
    for (const auto& delegate : delegatesList) {
        if (!pCdMan->pAccountCache->GetAccount(delegate, account)) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to get account info");
        }
        delegateArray.push_back(account.ToJsonObj());
        if (--delegateNum == 0) {
            break;
        }
    }
    obj.push_back(Pair("delegates", delegateArray));

    return obj;
}
