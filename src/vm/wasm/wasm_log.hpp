#pragma once

#include "logging.h"

#define WASM_TRACE( ... ) LogPrint(BCLog::WASM, __VA_ARGS__);
