#pragma once

#include <chrono>
#include "commons/uint256.h"
#include "wasm/types/inline_transaction.hpp"

namespace wasm {

    struct base_trace {
        uint256 trx_id;
        uint64_t receiver;
        inline_transaction trx;

        //std::chrono::microseconds elapsed;
        string console;
    };

    struct inline_transaction_trace : public base_trace {
        vector <inline_transaction_trace> inline_traces;
    };


    struct transaction_trace {       
        uint256 trx_id;
        std::chrono::microseconds elapsed;
        vector <inline_transaction_trace> traces;
    };

   /**
    *  Serialize a asset into a stream
    *
    *  @brief Serialize a asset
    *  @param ds - The stream to write
    *  @param wasm::inline_transaction_trace - The value to serialize
    *  @tparam DataStream - Type of datastream buffer
    *  @return DataStream& - Reference to the datastream
    */
    template<typename DataStream>
    inline DataStream &operator<<( DataStream &ds, const wasm::inline_transaction_trace &v ) {
        ds << v.trx_id;
        ds << v.receiver;
        ds << v.trx;
        //ds << v.console;
        ds << v.inline_traces;
        return ds;
    }

    /**
    *  Deserialize a asset from a stream
    *
    *  @brief Deserialize a asset
    *  @param ds - The stream to read
    *  @param wasm::inline_transaction_trace - The destination for deserialized value
    *  @tparam DataStream - Type of datastream buffer
    *  @return DataStream& - Reference to the datastream
    */
    template<typename DataStream>
    inline DataStream &operator>>( DataStream &ds, wasm::inline_transaction_trace &v ) {
        ds >> v.trx_id;
        ds >> v.receiver;
        ds >> v.trx;
        //ds >> v.console;
        ds >> v.inline_traces;
        return ds;
    }

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

    /**
    *  Serialize a asset into a stream
    *
    *  @brief Serialize a asset
    *  @param ds - The stream to write
    *  @param transaction_trace - The value to serialize
    *  @tparam DataStream - Type of datastream buffer
    *  @return DataStream& - Reference to the datastream
    */
    template<typename DataStream>
    inline DataStream &operator<<( DataStream &ds, const wasm::transaction_trace &v ) {
        ds << v.trx_id;
        ds << v.elapsed;
        ds << v.traces;
        return ds;
    }

    /**
    *  Deserialize a asset from a stream
    *
    *  @brief Deserialize a asset
    *  @param ds - The stream to read
    *  @param transaction_trace - The destination for deserialized value
    *  @tparam DataStream - Type of datastream buffer
    *  @return DataStream& - Reference to the datastream
    */
    template<typename DataStream>
    inline DataStream &operator>>( DataStream &ds, wasm::transaction_trace &v ) {
        ds >> v.trx_id;
        ds >> v.elapsed;
        ds >> v.traces;
        return ds;
    }

} //wasm