#pragma once

#include <map>
#include <string>
#include <functional>
#include <utility>
#include <chrono>

#include "commons/json/json_spirit.h"
#include "commons/json/json_spirit_reader_template.h"
#include "commons/json/json_spirit_writer.h"
#include "wasm/abi_def.hpp"
#include "wasm/wasm_variant.hpp"
#include "wasm/datastream.hpp"
#include "wasm/types/types.hpp"
#include "wasm/exception/exceptions.hpp"

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
    struct dag;

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
        void check_struct_in_recursion( const struct_def &s, shared_ptr <dag> &parent,
                                        wasm::abi_traverse_context &ctx ) const;

        json_spirit::Value
        binary_to_variant( const type_name &type, const bytes &binary, microseconds max_serialization_time ) const;
        bytes variant_to_binary( const type_name &type, const json_spirit::Value &var,
                                 microseconds max_serialization_time ) const;
        void variant_to_binary( const type_name &type, const json_spirit::Value &var, wasm::datastream<char *> &ds,
                                microseconds max_serialization_time ) const;
        
        typedef std::function<json_spirit::Value(wasm::datastream<const char *> & , bool, bool)> unpack_function;
        typedef std::function<void( const json_spirit::Value &, wasm::datastream<char *> &, bool, bool )> pack_function;
        void add_specialized_unpack_pack( const string &name,
                                          std::pair <abi_serializer::unpack_function, abi_serializer::pack_function> unpack_pack );

        json_spirit::Value get_field_variant( const type_name &s, const json_spirit::Value &v, field_name field, bool is_optional ) const;
        json_spirit::Value get_field_variant( const type_name &s, const json_spirit::Value &v, uint32_t index ) const;

        static std::vector<char>
        pack( const std::vector<char> &abi, const string &action, const string &params, microseconds max_serialization_time ) {

            vector<char> data;
            try {

                wasm::abi_def def = wasm::unpack<wasm::abi_def>(abi);
                wasm::abi_serializer abis(def, max_serialization_time);

                json_spirit::Value data_v;
                json_spirit::read_string(params, data_v);

                string action_type = abis.get_action_type(action);
                if(action_type == string()){
                    action_type = action;
                }
                data = abis.variant_to_binary(action_type, data_v, max_serialization_time);

            }
            CHAIN_CAPTURE_AND_RETHROW("abi_serializer pack error in action '%s' from params '%s'", action, params)

            return data;

        }

        static std::vector<char>
        pack( const std::vector<char> &abi, const string &action, const json_spirit::Value &params, microseconds max_serialization_time ) {

           vector<char> data;
           try {

                wasm::abi_def def = wasm::unpack<wasm::abi_def>(abi);
                wasm::abi_serializer abis(def, max_serialization_time);
                string action_type = abis.get_action_type(action);
                if(action_type == string()){
                    action_type = action;
                }
                data = abis.variant_to_binary(action_type, params, max_serialization_time);

            }
            CHAIN_CAPTURE_AND_RETHROW("abi_serializer pack error in action '%s'", action)

            return data;
        }

        static json_spirit::Value
        unpack( const std::vector<char> &abi, const string &action, const bytes &data, microseconds max_serialization_time ) {

            json_spirit::Value data_v;
            try {
                wasm::abi_def def = wasm::unpack<wasm::abi_def>(abi);
                wasm::abi_serializer abis(def, max_serialization_time);

                string action_type = abis.get_action_type(action);
                if(action_type == string()){
                    action_type = action;
                }
                data_v = abis.binary_to_variant(action_type, data, max_serialization_time);

            }
            CHAIN_CAPTURE_AND_RETHROW("abi_serializer unpack error in action '%s' params '%s'", action, to_hex(data))

            return data_v;
        }

        static json_spirit::Value
        unpack( const std::vector<char> &abi, const uint64_t &table, const bytes &data, microseconds max_serialization_time ) {

            json_spirit::Value data_v;
            type_name name;
            try {

                wasm::abi_def def = wasm::unpack<wasm::abi_def>(abi);
                wasm::abi_serializer abis(def, max_serialization_time);

                string t = wasm::name(table).to_string();
                name = abis.get_table_type(t);

                CHAIN_ASSERT(name.size() > 0, wasm_chain::abi_parse_exception, "can not get table %s's type from abi", t.data());

                data_v = abis.binary_to_variant(name, data, max_serialization_time);
            }
            CHAIN_CAPTURE_AND_RETHROW("abi_serializer unpack error in table %s from '%s'", name, to_hex(data))

            return data_v;
        }

    private:
        map <type_name, type_name> typedefs;
        map <type_name, struct_def> structs;
        map <type_name, type_name> actions;
        map <type_name, type_name> tables;
        map <uint64_t, string> error_messages;
        map <type_name, pair<unpack_function, pack_function>> built_in_types;

        void configure_built_in_types();
        json_spirit::Value _binary_to_variant( const type_name &type, wasm::datastream<const char *> &ds,
                                               wasm::abi_traverse_context &ctx ) const;

        bytes _variant_to_binary( const type_name &type, const json_spirit::Value &var,
                            wasm::abi_traverse_context &ctx ) const;
        void _variant_to_binary( const type_name &type, const json_spirit::Value &var, wasm::datastream<char *> &ds,
                                 wasm::abi_traverse_context &ctx ) const;
        static type_name _remove_bin_extension( const type_name &type );
        bool _is_type( const type_name &type, wasm::abi_traverse_context &ctx ) const;
        void validate( wasm::abi_traverse_context &ctx ) const;

    };

    struct abi_traverse_context {
        abi_traverse_context( std::chrono::microseconds max_serialization_time )
                : max_serialization_time_us(max_serialization_time),
                  deadline(system_clock::now()), // init to now, updated below
                  recursion_depth(0) {
            if (max_serialization_time_us > wasm::max_serialization_time) {
                deadline += wasm::max_serialization_time;
            } else {
                deadline += max_serialization_time_us;
            }
        }

        abi_traverse_context( std::chrono::microseconds max_serialization_time, system_clock::time_point deadline )
                : max_serialization_time_us(max_serialization_time), deadline(deadline), recursion_depth(0) {}


        void check_deadline() const;

    public:
        std::chrono::microseconds max_serialization_time_us;
        std::chrono::system_clock::time_point deadline;
        uint32_t recursion_depth;
    };

    struct dag {
        string name;
        shared_ptr <dag> root;
        vector <shared_ptr<dag>> parents;
        vector <shared_ptr<dag>> childs;

        bool has_circle( string n, wasm::abi_traverse_context &ctx ) {
            if (name == n) return true;
            for (auto p: parents) {
                ctx.check_deadline();
                if (p->has_circle(n, ctx)) return true;
            }
            return false;
        }

        string to_string() {
            std::stringstream ss;
            ss << "{name:" << name;
            ss << " parents:";
            for (auto p :parents) {
                ss << p->name;
                ss << " ";
            }

            ss << " childs:";
            for (auto c :childs) {
                ss << c->name;
                ss << " ";
            }

            ss << "}";
            return ss.str();
        }

        static tuple<bool, shared_ptr<dag>> add( shared_ptr <dag> t, string n, wasm::abi_traverse_context &ctx ) {

            CHAIN_ASSERT( !t->has_circle(n, ctx),
                          wasm_chain::abi_circular_def_exception,
                          "Circular reference in struct %s", n);

            for (auto child: t->childs) {
                if (child->name == n)
                    return tuple(false, child);
            }

            {
                auto child = wasm::dag::find(t->root, n, ctx);
                if (child != nullptr) {
                    auto itr = std::find(child->parents.begin(), child->parents.end(), t);
                    if (itr == child->parents.end())
                        child->parents.push_back(t);

                    return tuple(false, child);
                }
            }

            {//new child
                auto child = make_shared<dag>(
                        wasm::dag{n, t->root, vector < shared_ptr < dag >> {t}, vector < shared_ptr < dag >> {}});
                t->childs.push_back(child);
            }

            return tuple(true, t->childs.back());
        }

        static shared_ptr <dag> find( shared_ptr <dag> t, string n, wasm::abi_traverse_context &ctx ) {
            if (t->name == n) return t;
            for (auto child: t->childs) {
                ctx.check_deadline();
                auto d = wasm::dag::find(child, n, ctx);
                if (d != nullptr)
                    return d;
            }
            return nullptr;
        }
    };


}  // wasm

