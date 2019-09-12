#pragma once

#include <map>
#include <string>
#include <functional>
#include <utility>
#include <chrono>

#include "json/json_spirit.h"
#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_writer.h"
#include "wasm/abi_def.hpp"
#include "wasm/wasm_variant.hpp"
#include "wasm/datastream.hpp"
#include "wasm/types/types.hpp"
#include "wasm/exceptions.hpp"

namespace wasm {

    using namespace json_spirit;
    using namespace wasm;

    using std::map;
    using std::string;
    using std::function;
    using std::pair;

    using std::chrono::microseconds;
    using std::chrono::system_clock;

    struct abi_traverse_context;

/**
 *  Describes the binary representation message and table contents so that it can
 *  be converted to and from JSON.
 */
    struct abi_serializer {
        abi_serializer() { configure_built_in_types(); }

        abi_serializer( const abi_def &abi, const microseconds &max_serialization_time );
        void set_abi( const abi_def &abi, const microseconds &max_serialization_time );
        type_name resolve_type( const type_name &t ) const;
        bool is_array( const type_name &type ) const;
        bool is_optional( const type_name &type ) const;
        bool is_type( const type_name &type, const microseconds &max_serialization_time ) const;
        bool is_builtin_type( const type_name &type ) const;
        bool is_integer( const type_name &type ) const;
        int get_integer_size( const type_name &type ) const;
        bool is_struct( const type_name &type ) const;
        type_name fundamental_type( const type_name &type ) const;
        const struct_def &get_struct( const type_name &type ) const;
        type_name get_action_type( type_name action ) const;
        type_name get_table_type( type_name action ) const;
        void check_struct_in_recursion(const struct_def& s, vector<type_name>& types_seen, wasm::abi_traverse_context &ctx) const;

        json_spirit::Value
        binary_to_variant( const type_name &type, const bytes &binary, microseconds max_serialization_time ) const;
        bytes variant_to_binary( const type_name &type, const json_spirit::Value &var,
                                 microseconds max_serialization_time ) const;
        void variant_to_binary( const type_name &type, const json_spirit::Value &var, wasm::datastream<char *> &ds,
                                microseconds max_serialization_time ) const;
        typedef std::function<json_spirit::Value(wasm::datastream<const char *> & , bool, bool)> unpack_function;
        typedef std::function<void( const json_spirit::Value &, wasm::datastream<char *> &, bool, bool )> pack_function;

        static std::vector<char>
        pack( const string &abi, const string &action, const string &params, microseconds max_serialization_time ) {

            vector<char> data;
            try {
                json_spirit::Value abi_v;
                json_spirit::read_string(abi, abi_v);

                wasm::abi_def def;
                wasm::from_variant(abi_v, def);
                wasm::abi_serializer abis(def, max_serialization_time);

                json_spirit::Value data_v;
                json_spirit::read_string(params, data_v);
                std::cout << json_spirit::write(data_v) << std::endl;
                data = abis.variant_to_binary(action, data_v, max_serialization_time);

            }
            WASM_CAPTURE_AND_RETHROW("abi_serializer pack error in params %s", params.c_str())

            return data;

        }

        static json_spirit::Value
        unpack( const string &abi, const string &name, const bytes &data, microseconds max_serialization_time ) {

            json_spirit::Value data_v;
            try {
                json_spirit::Value abi_v;
                json_spirit::read_string(abi, abi_v);

                wasm::abi_def def;
                wasm::from_variant(abi_v, def);
                wasm::abi_serializer abis(def, max_serialization_time);

                data_v = abis.binary_to_variant(name, data, max_serialization_time);

            }
            WASM_CAPTURE_AND_RETHROW("abi_serializer unpack error in params %s", name.c_str())

            return data_v;
        }

        static json_spirit::Value
        unpack( const string &abi, const uint64_t &table, const bytes &data, microseconds max_serialization_time ) {

            //std::cout << "unpack " << " table:" << table << std::endl;
            json_spirit::Value data_v;
            type_name name;
            try {
                json_spirit::Value abi_v;
                json_spirit::read_string(abi, abi_v);

                wasm::abi_def def;
                wasm::from_variant(abi_v, def);
                wasm::abi_serializer abis(def, max_serialization_time);

                string t = wasm::name(table).to_string();
                name = abis.get_table_type(t);

                // std::cout << "unpack " << " name:" << name << std::endl;
                WASM_ASSERT(name.size() > 0, abi_parse_exception, "can not get table %s's type from abi",
                            t.data());

                data_v = abis.binary_to_variant(name, data, max_serialization_time);
            }
            WASM_CAPTURE_AND_RETHROW("abi_serializer unpack error in table %s", name.c_str())

            return data_v;
        }

    private:

        map <type_name, type_name> typedefs;
        map <type_name, struct_def> structs;
        map <type_name, type_name> actions;
        map <type_name, type_name> tables;
        map <uint64_t, string> error_messages;
        //map<type_name, variant_def>   variants;

        map <type_name, pair<unpack_function, pack_function>> built_in_types;


        void configure_built_in_types();


        json_spirit::Value _binary_to_variant( const type_name &type, wasm::datastream<const char *> &ds,
                                               wasm::abi_traverse_context &ctx ) const;


        bytes
        _variant_to_binary( const type_name &type, const json_spirit::Value &var,
                            wasm::abi_traverse_context &ctx ) const;
        void _variant_to_binary( const type_name &type, const json_spirit::Value &var, wasm::datastream<char *> &ds,
                                 wasm::abi_traverse_context &ctx ) const;
        static type_name _remove_bin_extension( const type_name &type );
        bool _is_type( const type_name &type, wasm::abi_traverse_context &ctx ) const;
        void validate( wasm::abi_traverse_context &ctx ) const;

    };

    struct abi_traverse_context {
        abi_traverse_context( std::chrono::microseconds max_serialization_time )
                : max_serialization_time(max_serialization_time),
                  deadline(system_clock::now()), // init to now, updated below
                  recursion_depth(0) {
            if (max_serialization_time > microseconds::max() - deadline.time_since_epoch()) {
                deadline = std::chrono::time_point<std::chrono::system_clock>::max();
            } else {
                deadline += max_serialization_time;
            }
        }

        abi_traverse_context( std::chrono::microseconds max_serialization_time, system_clock::time_point deadline )
                : max_serialization_time(max_serialization_time), deadline(deadline), recursion_depth(0) {}


        void check_deadline() const;

        //fc::scoped_exit<std::function<void()>> enter_scope();
    public:
        std::chrono::microseconds max_serialization_time;
        std::chrono::system_clock::time_point deadline;
        uint32_t recursion_depth;
    };


}  // wasm

