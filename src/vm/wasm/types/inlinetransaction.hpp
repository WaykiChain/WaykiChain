#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <map>
// #include <datastream.hpp>

using namespace std;

namespace wasm
{

class CInlineTransaction {

public:
	CInlineTransaction(){};
	~CInlineTransaction(){};

public:
	uint64_t contract;
	uint64_t action;
	std::vector<char> data;

      /**
      *  Serialize a asset into a stream
      *
      *  @brief Serialize a asset
      *  @param ds - The stream to write
      *  @param sym - The value to serialize
      *  @tparam DataStream - Type of datastream buffer
      *  @return DataStream& - Reference to the datastream
      */
      template<typename DataStream>
      friend inline DataStream& operator<<(DataStream& ds, const CInlineTransaction& trx) {

        ds.write( (const char*)&trx.contract, sizeof(trx.contract));
        ds.write( (const char*)&trx.action, sizeof(trx.action));
        ds << trx.data;       
        return ds;
      }

      /**
      *  Deserialize a asset from a stream
      *
      *  @brief Deserialize a asset
      *  @param ds - The stream to read
      *  @param symbol - The destination for deserialized value
      *  @tparam DataStream - Type of datastream buffer
      *  @return DataStream& - Reference to the datastream
      */
      template<typename DataStream>
      friend inline DataStream& operator>>(DataStream& ds, CInlineTransaction& trx) {

         ds.read((char*)&trx.contract, sizeof(trx.contract));
         ds.read((char*)&trx.action, sizeof(trx.action));
         ds >> trx.data;      

         return ds;
      } 

};

}//wasm