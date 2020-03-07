//
// Created by yehuan on 2020-02-12.
//

#include "rpcgenrawtx.h"

#include "commons/base58.h"
#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"
#include "commons/util/util.h"
#include "init.h"
#include "miner/miner.h"
#include "net.h"
#include "netbase.h"
#include "wallet/wallet.h"

#include "persistence/contractdb.h"
#include "rpc/core/rpccommons.h"
#include "rpc/core/rpcserver.h"
#include "vm/luavm/appaccount.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#include "tx/mulsigtx.h"

#include <stdint.h>
#include <boost/assign/list_of.hpp>


using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;


Value gensendtxraw(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 6 && params.size() != 7))
        throw runtime_error(
                "submitsendtx \"from\" \"to\" \"symbol:coin:unit\" \"symbol:fee:unit\" [\"memo\"]\n"
                "\nSend coins to a given address.\n" +
                HelpRequiringPassphrase() +
                "\nArguments:\n"
                "1.\"txAddress\":           (string, required) the address use to create signature"
                "2.\"submit_height\":       (numberic,required) height when submit"
                "3.\"from\":                (string, required) The address where coins are sent from\n"
                "4.\"to\":                  (string, required) The address where coins are received\n"
                "5.\"symbol:coin:unit\":    (symbol:amount:unit, required) transferred coins\n"
                "6.\"symbol:fee:unit\":     (symbol:amount:unit, required) fee paid to miner, default is WICC:10000:sawi\n"
                "7.\"memo\":                (string, optional)\n"
                "\nResult:\n"
                "\"txid\"                   (string) The transaction id.\n"
                "\nExamples:\n" +
                HelpExampleCli("submitsendtx",
                               "\"wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4\" \"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\" "
                               "\"WICC:1000000:sawi\" \"WICC:10000:sawi\" \"Hello, WaykiChain!\"") +
                "\nAs json rpc call\n" +
                HelpExampleRpc("submitsendtx",
                               "\"wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4\", \"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\", "
                               "\"WICC:1000000:sawi\", \"WICC:10000:sawi\", \"Hello, WaykiChain!\""));

    EnsureWalletIsUnlocked();

    CKeyID keyID       = CKeyID(params[0].get_str()) ;
    int32_t height     = params[1].get_int() ;
    CUserID sendUserId = RPC_PARAM::GetUserId(params[2], true);
    CUserID recvUserId = RPC_PARAM::GetUserId(params[3]);
    ComboMoney cmCoin  = RPC_PARAM::GetComboMoney(params[4], SYMB::WICC);
    ComboMoney cmFee   = RPC_PARAM::GetFee(params, 5, UCOIN_TRANSFER_TX);

    if (cmCoin.amount == 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Coins is zero!");
    string memo    = params.size() == 7 ? params[6].get_str() : "";

    std::shared_ptr<CCoinTransferTx> tx;

    tx = std::make_shared<CCoinTransferTx>(sendUserId, recvUserId, height, cmCoin.symbol,
                                                cmCoin.GetAmountInSawi(), cmFee.symbol, cmFee.GetAmountInSawi(), memo);


    if (!pWalletMain->Sign(keyID, tx->GetHash(), tx->signature)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,  "Sign failed");
    }

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << tx;
    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
    return obj;
}
