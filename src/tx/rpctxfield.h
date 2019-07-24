//
// Created by yehuan on 2019-07-24.
//

#ifndef WAYKICHAIN_RPCTXFIELD_H
#define WAYKICHAIN_RPCTXFIELD_H

#include <string>


namespace txfield {
    const string TXID  = "txid" ;
    const string TXTYPE = "tx_type" ;
    const string RAWTX = "rawtx" ;
    const string TRIGGER_UID = "tx_uid" ;
    const string TRIGGER_ADDR = "addr" ;
    const string DEST_UID = "to_uid" ;
    const string DEST_ADDR = "to_addr"
    const string BLOCK_HASH = "block_hash" ;
    const string FEES_AMOUNT = "fees" ;
    const string FEES_COIN_TYPE = "fees_coin_type" ;
    const string VERSION = "ver" ;
    const string VALID_HEIGHT = "valid_height" ;
    const string CONFIRMED_HEIGHT = "confirmed_height" ;
}

#endif //WAYKICHAIN_RPCTXFIELD_H
