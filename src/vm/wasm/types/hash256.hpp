#pragma once

#include <limits>
#include <stdint.h>
#include <string>

namespace wasm {

    struct hash256 {

        public:
            constexpr hash256() : value() {}
            explicit hash256( uint8_t buffer[32] ){
                std::copy(buffer, buffer+32, value.begin());
            }

            explicit hash256( std::string_view str ){
                std::map<char, uint8_t> hex = {
                        {'0', 0x00},
                        {'1', 0x01},
                        {'2', 0x02},
                        {'3', 0x03},
                        {'4', 0x04},
                        {'5', 0x05},
                        {'6', 0x06},
                        {'7', 0x07},
                        {'8', 0x08},
                        {'9', 0x09},
                        {'a', 0x0a},
                        {'b', 0x0b},
                        {'c', 0x0c},
                        {'d', 0x0d},
                        {'e', 0x0e},
                        {'f', 0x0f}
                };

                if(str[0] == '0' && tolower(str[1]) == 'x'){
                    str = str.substr(2);
                }

                check(str.size() == sizeof(value) * 2, "Hex string for hash256 must be 64 bytes");

                for(std::string::size_type i = 0; i < sizeof(value); i++){
                    uint8_t h = hex[(char) str[str.size() - 2 * (i + 1)]];
                    uint8_t l = hex[(char) str[str.size() - 2 * (i + 1) + 1]];
                    uint8_t t = l | h << 4;
                    value[i] = t;
                }


                WASM_TRACE("%s", to_string());


            }

            std::string to_string() const {
                const std::string hex = "0123456789abcdef";
                std::string str;

                for (std::string::size_type i = 0; i < sizeof(value); ++i){
                    str = str + hex[(unsigned char) value[sizeof(value) - i - 1] >> 4];
                    str = str + hex[(unsigned char) value[sizeof(value) - i - 1] & 0xf];
                }
                return str;
            }



            // /**  Serialize a symbol_code into a stream
            // *
            // *  @brief Serialize a symbol_code
            // *  @param ds - The stream to write
            // *  @param sym - The value to serialize
            // *  @tparam DataStream - Type of datastream buffer
            // *  @return DataStream& - Reference to the datastream
            // */
            // template<typename DataStream, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
            // friend inline DataStream &operator<<( DataStream &ds, const wasm::hash256 &hash256 ) {
            //     ds.write((const char *) &hash256.value, sizeof(value));
            //     return ds;
            // }

            // /**
            // *  Deserialize a symbol_code from a stream
            // *
            // *  @brief Deserialize a symbol_code
            // *  @param ds - The stream to read
            // *  @param symbol - The destination for deserialized value
            // *  @tparam DataStream - Type of datastream buffer
            // *  @return DataStream& - Reference to the datastream
            // */
            // template<typename DataStream, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
            // friend inline DataStream &operator>>( DataStream &ds, wasm::hash256 &hash256 ) {
            //     ds.read((char *) &hash256.value, sizeof(value));
            //     return ds;
            // }


            std::array<uint8_t, 32> value;

            WASM_REFLECT( hash256, (value) )
    };
}