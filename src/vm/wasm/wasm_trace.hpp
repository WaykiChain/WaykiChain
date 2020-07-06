#pragma once

#include <chrono>
#include "entities/id.h"
#include "commons/uint256.h"
#include "wasm/types/inline_transaction.hpp"
#include "wasm/wasm_serialize_reflect.hpp"

namespace wasm {

    struct base_trace {
        uint256            trx_id;
        uint64_t           receiver;
        inline_transaction trx;

        //std::chrono::microseconds elapsed;
        string console;

        WASM_REFLECT( base_trace, (trx_id)(receiver)(trx) )
    };

    struct inline_transaction_trace : public base_trace {
        vector <inline_transaction_trace> inline_traces;

        WASM_REFLECT_DERIVED( inline_transaction_trace, base_trace, (inline_traces) )
    };


    struct transaction_trace {
        uint256                   trx_id;
        std::chrono::microseconds elapsed;
        uint64_t                  fuel = 0;
        uint64_t                  fuel_rate = 0;
        uint64_t                  minimum_tx_execute_fee = 0;
        vector <inline_transaction_trace> traces;

        WASM_REFLECT( transaction_trace, (trx_id)(elapsed)(fuel)(fuel_rate)(minimum_tx_execute_fee)(traces) )
    };

   /**
    *  Serialize a asset into a stream
    *
    *  @brief Serialize a asset
    *  @param ds - The stream to write
    *  @param std::chrono::microseconds - The value to serialize
    *  @tparam DataStream - Type of datastream buffer
    *  @return DataStream& - Reference to the datastream
    */
    template<typename DataStream>
    inline DataStream &operator<<( DataStream &ds, const std::chrono::microseconds &v ) {
        ds.write((const char *) &v, sizeof(std::chrono::microseconds));
        return ds;
    }

    /**
    *  Deserialize a asset from a stream
    *
    *  @brief Deserialize a asset
    *  @param ds - The stream to read
    *  @param std::chrono::microseconds - The destination for deserialized value
    *  @tparam DataStream - Type of datastream buffer
    *  @return DataStream& - Reference to the datastream
    */
    template<typename DataStream>
    inline DataStream &operator>>( DataStream &ds, std::chrono::microseconds &v ) {
        ds.read((char *) &v, sizeof(std::chrono::microseconds));
        return ds;
    }

    /**
    *  Serialize a asset into a stream
    *
    *  @brief Serialize a asset
    *  @param ds - The stream to write
    *  @param uint256 - The value to serialize
    *  @tparam DataStream - Type of datastream buffer
    *  @return DataStream& - Reference to the datastream
    */
    template<typename DataStream>
    inline DataStream &operator<<( DataStream &ds, const uint256 &v ) {
        ds.write((const char *) &v, sizeof(uint256));
        return ds;
    }

    /**
    *  Deserialize a asset from a stream
    *
    *  @brief Deserialize a asset
    *  @param ds - The stream to read
    *  @param uint256 - The destination for deserialized value
    *  @tparam DataStream - Type of datastream buffer
    *  @return DataStream& - Reference to the datastream
    */
    template<typename DataStream>
    inline DataStream &operator>>( DataStream &ds, uint256 &v ) {
        ds.read((char *) &v, sizeof(uint256));
        return ds;
    }

} //wasm