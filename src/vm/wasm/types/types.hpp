#pragma once

#include <wasm/types/name.hpp>

#include <memory>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <optional>
#include <unordered_map>
#include <cstdint>
#include <string>


namespace wasm {
    using std::map;
    using std::vector;
    using std::unordered_map;
    using std::string;
    using std::deque;
    using std::shared_ptr;
    using std::weak_ptr;
    using std::unique_ptr;
    using std::set;
    using std::pair;
    using std::optional;
    using std::make_pair;
    using std::enable_shared_from_this;
    using std::tie;
    using std::move;
    using std::forward;
    using std::to_string;
    // using std::all_of;

    using action_name      = name;
    using scope_name       = name;
    using account_name     = name;
    using table_name       = name;

    using int128_t         = __int128;
    using uint128_t        = unsigned __int128;
    using bytes            = vector<char>;


   struct __attribute__((aligned (16))) checksum160_type { uint8_t hash[20]; };
   struct __attribute__((aligned (16))) checksum256_type { uint8_t hash[32]; };
   struct __attribute__((aligned (16))) checksum512_type { uint8_t hash[64]; };

}  // wasm
