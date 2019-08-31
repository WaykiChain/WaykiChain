#pragma once
#include <chrono>

namespace wasm { 

using std::chrono::microseconds;

static auto max_serialization_time = microseconds(15*1000);
static uint16_t max_inline_action_depth = 4;




}  // wasm

