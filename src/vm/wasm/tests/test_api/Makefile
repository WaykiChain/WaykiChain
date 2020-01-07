
OPT = -DNDEBUG -DEOS_VM_USE_EXTERNAL_OUTCOME -DHAVE_WORKING_BOOST_SLEEP_FOR

PLATFORM_LDFLAGS  = -pthread
PLATFORM_CCFLAGS  = -fno-builtin-memcmp -pthread -DOS_LINUX
PLATFORM_CXXFLAGS = -fno-builtin-memcmp -pthread -DOS_LINUX


WASM_DIR      = ../..
VM_DIR        = ../../..
WAYKI_SRC_DIR = ../../../..
BERKELEY_SOFTFLOAT = ../../../../external/softfloat

WASM_TYPES        = $(WASM_DIR)/types
EOSVM_INCLUDE_DIR = $(WASM_DIR)/eos-vm/include
EOSVM_OUTCOME_DIR = ../../../../external/outcome/single-header
EXCEPTION_DIR     = $(WASM_DIR)/exception

WASM_INTERFACE = $(WASM_DIR)/wasm_interface.cpp
WASM_RUNTIME   = $(WASM_DIR)/wasm_runtime.cpp
UINT128_SRC    = $(WASM_TYPES)/uint128.cpp

EXCEPTION_SRC = \
${EXCEPTION_DIR}/log_message.cpp \
${EXCEPTION_DIR}/exception.cpp 

COMPILER_BUILTINS = $(WASM_DIR)/compiler_builtins
COMPILER_BUILTINS_SRC = \
$(COMPILER_BUILTINS)/fixdfti.c \
$(COMPILER_BUILTINS)/fixsfti.c \
$(COMPILER_BUILTINS)/fixtfti.c \
$(COMPILER_BUILTINS)/fixunsdfti.c \
$(COMPILER_BUILTINS)/fixunssfti.c \
$(COMPILER_BUILTINS)/fixunstfti.c \
$(COMPILER_BUILTINS)/floattidf.c \
$(COMPILER_BUILTINS)/floatuntidf.c 


SOFTFLOAT_SRC = \
${BERKELEY_SOFTFLOAT}/source/s_eq128.c \
${BERKELEY_SOFTFLOAT}/source/s_le128.c \
${BERKELEY_SOFTFLOAT}/source/s_lt128.c \
${BERKELEY_SOFTFLOAT}/source/s_shortShiftLeft128.c \
${BERKELEY_SOFTFLOAT}/source/s_shortShiftRight128.c \
${BERKELEY_SOFTFLOAT}/source/s_shortShiftRightJam64.c \
${BERKELEY_SOFTFLOAT}/source/s_shortShiftRightJam64Extra.c \
${BERKELEY_SOFTFLOAT}/source/s_shortShiftRightJam128.c \
${BERKELEY_SOFTFLOAT}/source/s_shortShiftRightJam128Extra.c \
${BERKELEY_SOFTFLOAT}/source/s_shiftRightJam32.c \
${BERKELEY_SOFTFLOAT}/source/s_shiftRightJam64.c \
${BERKELEY_SOFTFLOAT}/source/s_shiftRightJam64Extra.c \
${BERKELEY_SOFTFLOAT}/source/s_shiftRightJam128.c \
${BERKELEY_SOFTFLOAT}/source/s_shiftRightJam128Extra.c \
${BERKELEY_SOFTFLOAT}/source/s_shiftRightJam256M.c \
${BERKELEY_SOFTFLOAT}/source/s_countLeadingZeros8.c \
${BERKELEY_SOFTFLOAT}/source/s_countLeadingZeros16.c \
${BERKELEY_SOFTFLOAT}/source/s_countLeadingZeros32.c \
${BERKELEY_SOFTFLOAT}/source/s_countLeadingZeros64.c \
${BERKELEY_SOFTFLOAT}/source/s_add128.c \
${BERKELEY_SOFTFLOAT}/source/s_add256M.c \
${BERKELEY_SOFTFLOAT}/source/s_sub128.c \
${BERKELEY_SOFTFLOAT}/source/s_sub256M.c \
${BERKELEY_SOFTFLOAT}/source/s_mul64ByShifted32To128.c \
${BERKELEY_SOFTFLOAT}/source/s_mul64To128.c \
${BERKELEY_SOFTFLOAT}/source/s_mul128By32.c \
${BERKELEY_SOFTFLOAT}/source/s_mul128To256M.c \
${BERKELEY_SOFTFLOAT}/source/s_approxRecip_1Ks.c \
${BERKELEY_SOFTFLOAT}/source/s_approxRecip32_1.c \
${BERKELEY_SOFTFLOAT}/source/s_approxRecipSqrt_1Ks.c \
${BERKELEY_SOFTFLOAT}/source/s_approxRecipSqrt32_1.c \
${BERKELEY_SOFTFLOAT}/source/8086-SSE/softfloat_raiseFlags.c \
${BERKELEY_SOFTFLOAT}/source/8086-SSE/s_f16UIToCommonNaN.c \
${BERKELEY_SOFTFLOAT}/source/8086-SSE/s_commonNaNToF16UI.c \
${BERKELEY_SOFTFLOAT}/source/8086-SSE/s_propagateNaNF16UI.c \
${BERKELEY_SOFTFLOAT}/source/8086-SSE/s_f32UIToCommonNaN.c \
${BERKELEY_SOFTFLOAT}/source/8086-SSE/s_commonNaNToF32UI.c \
${BERKELEY_SOFTFLOAT}/source/8086-SSE/s_propagateNaNF32UI.c \
${BERKELEY_SOFTFLOAT}/source/8086-SSE/s_f64UIToCommonNaN.c \
${BERKELEY_SOFTFLOAT}/source/8086-SSE/s_commonNaNToF64UI.c \
${BERKELEY_SOFTFLOAT}/source/8086-SSE/s_propagateNaNF64UI.c \
${BERKELEY_SOFTFLOAT}/source/8086-SSE/extF80M_isSignalingNaN.c \
${BERKELEY_SOFTFLOAT}/source/8086-SSE/s_extF80UIToCommonNaN.c \
${BERKELEY_SOFTFLOAT}/source/8086-SSE/s_commonNaNToExtF80UI.c \
${BERKELEY_SOFTFLOAT}/source/8086-SSE/s_propagateNaNExtF80UI.c \
${BERKELEY_SOFTFLOAT}/source/8086-SSE/f128M_isSignalingNaN.c \
${BERKELEY_SOFTFLOAT}/source/8086-SSE/s_f128UIToCommonNaN.c \
${BERKELEY_SOFTFLOAT}/source/8086-SSE/s_commonNaNToF128UI.c \
${BERKELEY_SOFTFLOAT}/source/8086-SSE/s_propagateNaNF128UI.c \
${BERKELEY_SOFTFLOAT}/source/s_roundToUI32.c \
${BERKELEY_SOFTFLOAT}/source/s_roundToUI64.c \
${BERKELEY_SOFTFLOAT}/source/s_roundToI32.c \
${BERKELEY_SOFTFLOAT}/source/s_roundToI64.c \
${BERKELEY_SOFTFLOAT}/source/s_normSubnormalF16Sig.c \
${BERKELEY_SOFTFLOAT}/source/s_roundPackToF16.c \
${BERKELEY_SOFTFLOAT}/source/s_normRoundPackToF16.c \
${BERKELEY_SOFTFLOAT}/source/s_addMagsF16.c \
${BERKELEY_SOFTFLOAT}/source/s_subMagsF16.c \
${BERKELEY_SOFTFLOAT}/source/s_mulAddF16.c \
${BERKELEY_SOFTFLOAT}/source/s_normSubnormalF32Sig.c \
${BERKELEY_SOFTFLOAT}/source/s_roundPackToF32.c \
${BERKELEY_SOFTFLOAT}/source/s_normRoundPackToF32.c \
${BERKELEY_SOFTFLOAT}/source/s_addMagsF32.c \
${BERKELEY_SOFTFLOAT}/source/s_subMagsF32.c \
${BERKELEY_SOFTFLOAT}/source/s_mulAddF32.c \
${BERKELEY_SOFTFLOAT}/source/s_normSubnormalF64Sig.c \
${BERKELEY_SOFTFLOAT}/source/s_roundPackToF64.c \
${BERKELEY_SOFTFLOAT}/source/s_normRoundPackToF64.c \
${BERKELEY_SOFTFLOAT}/source/s_addMagsF64.c \
${BERKELEY_SOFTFLOAT}/source/s_subMagsF64.c \
${BERKELEY_SOFTFLOAT}/source/s_mulAddF64.c \
${BERKELEY_SOFTFLOAT}/source/s_normSubnormalExtF80Sig.c \
${BERKELEY_SOFTFLOAT}/source/s_roundPackToExtF80.c \
${BERKELEY_SOFTFLOAT}/source/s_normRoundPackToExtF80.c \
${BERKELEY_SOFTFLOAT}/source/s_addMagsExtF80.c \
${BERKELEY_SOFTFLOAT}/source/s_subMagsExtF80.c \
${BERKELEY_SOFTFLOAT}/source/s_normSubnormalF128Sig.c \
${BERKELEY_SOFTFLOAT}/source/s_roundPackToF128.c \
${BERKELEY_SOFTFLOAT}/source/s_normRoundPackToF128.c \
${BERKELEY_SOFTFLOAT}/source/s_addMagsF128.c \
${BERKELEY_SOFTFLOAT}/source/s_subMagsF128.c \
${BERKELEY_SOFTFLOAT}/source/s_mulAddF128.c \
${BERKELEY_SOFTFLOAT}/source/softfloat_state.c \
${BERKELEY_SOFTFLOAT}/source/ui32_to_f16.c \
${BERKELEY_SOFTFLOAT}/source/ui32_to_f32.c \
${BERKELEY_SOFTFLOAT}/source/ui32_to_f64.c \
${BERKELEY_SOFTFLOAT}/source/ui32_to_extF80.c \
${BERKELEY_SOFTFLOAT}/source/ui32_to_extF80M.c \
${BERKELEY_SOFTFLOAT}/source/ui32_to_f128.c \
${BERKELEY_SOFTFLOAT}/source/ui32_to_f128M.c \
${BERKELEY_SOFTFLOAT}/source/ui64_to_f16.c \
${BERKELEY_SOFTFLOAT}/source/ui64_to_f32.c \
${BERKELEY_SOFTFLOAT}/source/ui64_to_f64.c \
${BERKELEY_SOFTFLOAT}/source/ui64_to_extF80.c \
${BERKELEY_SOFTFLOAT}/source/ui64_to_extF80M.c \
${BERKELEY_SOFTFLOAT}/source/ui64_to_f128.c \
${BERKELEY_SOFTFLOAT}/source/ui64_to_f128M.c \
${BERKELEY_SOFTFLOAT}/source/i32_to_f16.c \
${BERKELEY_SOFTFLOAT}/source/i32_to_f32.c \
${BERKELEY_SOFTFLOAT}/source/i32_to_f64.c \
${BERKELEY_SOFTFLOAT}/source/i32_to_extF80.c \
${BERKELEY_SOFTFLOAT}/source/i32_to_extF80M.c \
${BERKELEY_SOFTFLOAT}/source/i32_to_f128.c \
${BERKELEY_SOFTFLOAT}/source/i32_to_f128M.c \
${BERKELEY_SOFTFLOAT}/source/i64_to_f16.c \
${BERKELEY_SOFTFLOAT}/source/i64_to_f32.c \
${BERKELEY_SOFTFLOAT}/source/i64_to_f64.c \
${BERKELEY_SOFTFLOAT}/source/i64_to_extF80.c \
${BERKELEY_SOFTFLOAT}/source/i64_to_extF80M.c \
${BERKELEY_SOFTFLOAT}/source/i64_to_f128.c \
${BERKELEY_SOFTFLOAT}/source/i64_to_f128M.c \
${BERKELEY_SOFTFLOAT}/source/f16_to_ui32.c \
${BERKELEY_SOFTFLOAT}/source/f16_to_ui64.c \
${BERKELEY_SOFTFLOAT}/source/f16_to_i32.c \
${BERKELEY_SOFTFLOAT}/source/f16_to_i64.c \
${BERKELEY_SOFTFLOAT}/source/f16_to_ui32_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/f16_to_ui64_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/f16_to_i32_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/f16_to_i64_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/f16_to_f32.c \
${BERKELEY_SOFTFLOAT}/source/f16_to_f64.c \
${BERKELEY_SOFTFLOAT}/source/f16_to_extF80.c \
${BERKELEY_SOFTFLOAT}/source/f16_to_extF80M.c \
${BERKELEY_SOFTFLOAT}/source/f16_to_f128.c \
${BERKELEY_SOFTFLOAT}/source/f16_to_f128M.c \
${BERKELEY_SOFTFLOAT}/source/f16_roundToInt.c \
${BERKELEY_SOFTFLOAT}/source/f16_add.c \
${BERKELEY_SOFTFLOAT}/source/f16_sub.c \
${BERKELEY_SOFTFLOAT}/source/f16_mul.c \
${BERKELEY_SOFTFLOAT}/source/f16_mulAdd.c \
${BERKELEY_SOFTFLOAT}/source/f16_div.c \
${BERKELEY_SOFTFLOAT}/source/f16_rem.c \
${BERKELEY_SOFTFLOAT}/source/f16_sqrt.c \
${BERKELEY_SOFTFLOAT}/source/f16_eq.c \
${BERKELEY_SOFTFLOAT}/source/f16_le.c \
${BERKELEY_SOFTFLOAT}/source/f16_lt.c \
${BERKELEY_SOFTFLOAT}/source/f16_eq_signaling.c \
${BERKELEY_SOFTFLOAT}/source/f16_le_quiet.c \
${BERKELEY_SOFTFLOAT}/source/f16_lt_quiet.c \
${BERKELEY_SOFTFLOAT}/source/f16_isSignalingNaN.c \
${BERKELEY_SOFTFLOAT}/source/f32_to_ui32.c \
${BERKELEY_SOFTFLOAT}/source/f32_to_ui64.c \
${BERKELEY_SOFTFLOAT}/source/f32_to_i32.c \
${BERKELEY_SOFTFLOAT}/source/f32_to_i64.c \
${BERKELEY_SOFTFLOAT}/source/f32_to_ui32_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/f32_to_ui64_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/f32_to_i32_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/f32_to_i64_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/f32_to_f16.c \
${BERKELEY_SOFTFLOAT}/source/f32_to_f64.c \
${BERKELEY_SOFTFLOAT}/source/f32_to_extF80.c \
${BERKELEY_SOFTFLOAT}/source/f32_to_extF80M.c \
${BERKELEY_SOFTFLOAT}/source/f32_to_f128.c \
${BERKELEY_SOFTFLOAT}/source/f32_to_f128M.c \
${BERKELEY_SOFTFLOAT}/source/f32_roundToInt.c \
${BERKELEY_SOFTFLOAT}/source/f32_add.c \
${BERKELEY_SOFTFLOAT}/source/f32_sub.c \
${BERKELEY_SOFTFLOAT}/source/f32_mul.c \
${BERKELEY_SOFTFLOAT}/source/f32_mulAdd.c \
${BERKELEY_SOFTFLOAT}/source/f32_div.c \
${BERKELEY_SOFTFLOAT}/source/f32_rem.c \
${BERKELEY_SOFTFLOAT}/source/f32_sqrt.c \
${BERKELEY_SOFTFLOAT}/source/f32_eq.c \
${BERKELEY_SOFTFLOAT}/source/f32_le.c \
${BERKELEY_SOFTFLOAT}/source/f32_lt.c \
${BERKELEY_SOFTFLOAT}/source/f32_eq_signaling.c \
${BERKELEY_SOFTFLOAT}/source/f32_le_quiet.c \
${BERKELEY_SOFTFLOAT}/source/f32_lt_quiet.c \
${BERKELEY_SOFTFLOAT}/source/f32_isSignalingNaN.c \
${BERKELEY_SOFTFLOAT}/source/f64_to_ui32.c \
${BERKELEY_SOFTFLOAT}/source/f64_to_ui64.c \
${BERKELEY_SOFTFLOAT}/source/f64_to_i32.c \
${BERKELEY_SOFTFLOAT}/source/f64_to_i64.c \
${BERKELEY_SOFTFLOAT}/source/f64_to_ui32_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/f64_to_ui64_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/f64_to_i32_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/f64_to_i64_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/f64_to_f16.c \
${BERKELEY_SOFTFLOAT}/source/f64_to_f32.c \
${BERKELEY_SOFTFLOAT}/source/f64_to_extF80.c \
${BERKELEY_SOFTFLOAT}/source/f64_to_extF80M.c \
${BERKELEY_SOFTFLOAT}/source/f64_to_f128.c \
${BERKELEY_SOFTFLOAT}/source/f64_to_f128M.c \
${BERKELEY_SOFTFLOAT}/source/f64_roundToInt.c \
${BERKELEY_SOFTFLOAT}/source/f64_add.c \
${BERKELEY_SOFTFLOAT}/source/f64_sub.c \
${BERKELEY_SOFTFLOAT}/source/f64_mul.c \
${BERKELEY_SOFTFLOAT}/source/f64_mulAdd.c \
${BERKELEY_SOFTFLOAT}/source/f64_div.c \
${BERKELEY_SOFTFLOAT}/source/f64_rem.c \
${BERKELEY_SOFTFLOAT}/source/f64_sqrt.c \
${BERKELEY_SOFTFLOAT}/source/f64_eq.c \
${BERKELEY_SOFTFLOAT}/source/f64_le.c \
${BERKELEY_SOFTFLOAT}/source/f64_lt.c \
${BERKELEY_SOFTFLOAT}/source/f64_eq_signaling.c \
${BERKELEY_SOFTFLOAT}/source/f64_le_quiet.c \
${BERKELEY_SOFTFLOAT}/source/f64_lt_quiet.c \
${BERKELEY_SOFTFLOAT}/source/f64_isSignalingNaN.c \
${BERKELEY_SOFTFLOAT}/source/extF80_to_ui32.c \
${BERKELEY_SOFTFLOAT}/source/extF80_to_ui64.c \
${BERKELEY_SOFTFLOAT}/source/extF80_to_i32.c \
${BERKELEY_SOFTFLOAT}/source/extF80_to_i64.c \
${BERKELEY_SOFTFLOAT}/source/extF80_to_ui32_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/extF80_to_ui64_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/extF80_to_i32_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/extF80_to_i64_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/extF80_to_f16.c \
${BERKELEY_SOFTFLOAT}/source/extF80_to_f32.c \
${BERKELEY_SOFTFLOAT}/source/extF80_to_f64.c \
${BERKELEY_SOFTFLOAT}/source/extF80_to_f128.c \
${BERKELEY_SOFTFLOAT}/source/extF80_roundToInt.c \
${BERKELEY_SOFTFLOAT}/source/extF80_add.c \
${BERKELEY_SOFTFLOAT}/source/extF80_sub.c \
${BERKELEY_SOFTFLOAT}/source/extF80_mul.c \
${BERKELEY_SOFTFLOAT}/source/extF80_div.c \
${BERKELEY_SOFTFLOAT}/source/extF80_rem.c \
${BERKELEY_SOFTFLOAT}/source/extF80_sqrt.c \
${BERKELEY_SOFTFLOAT}/source/extF80_eq.c \
${BERKELEY_SOFTFLOAT}/source/extF80_le.c \
${BERKELEY_SOFTFLOAT}/source/extF80_lt.c \
${BERKELEY_SOFTFLOAT}/source/extF80_eq_signaling.c \
${BERKELEY_SOFTFLOAT}/source/extF80_le_quiet.c \
${BERKELEY_SOFTFLOAT}/source/extF80_lt_quiet.c \
${BERKELEY_SOFTFLOAT}/source/extF80_isSignalingNaN.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_to_ui32.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_to_ui64.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_to_i32.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_to_i64.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_to_ui32_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_to_ui64_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_to_i32_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_to_i64_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_to_f16.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_to_f32.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_to_f64.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_to_f128M.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_roundToInt.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_add.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_sub.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_mul.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_div.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_rem.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_sqrt.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_eq.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_le.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_lt.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_eq_signaling.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_le_quiet.c \
${BERKELEY_SOFTFLOAT}/source/extF80M_lt_quiet.c \
${BERKELEY_SOFTFLOAT}/source/f128_to_ui32.c \
${BERKELEY_SOFTFLOAT}/source/f128_to_ui64.c \
${BERKELEY_SOFTFLOAT}/source/f128_to_i32.c \
${BERKELEY_SOFTFLOAT}/source/f128_to_i64.c \
${BERKELEY_SOFTFLOAT}/source/f128_to_ui32_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/f128_to_ui64_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/f128_to_i32_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/f128_to_i64_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/f128_to_f16.c \
${BERKELEY_SOFTFLOAT}/source/f128_to_f32.c \
${BERKELEY_SOFTFLOAT}/source/f128_to_extF80.c \
${BERKELEY_SOFTFLOAT}/source/f128_to_f64.c \
${BERKELEY_SOFTFLOAT}/source/f128_roundToInt.c \
${BERKELEY_SOFTFLOAT}/source/f128_add.c \
${BERKELEY_SOFTFLOAT}/source/f128_sub.c \
${BERKELEY_SOFTFLOAT}/source/f128_mul.c \
${BERKELEY_SOFTFLOAT}/source/f128_mulAdd.c \
${BERKELEY_SOFTFLOAT}/source/f128_div.c \
${BERKELEY_SOFTFLOAT}/source/f128_rem.c \
${BERKELEY_SOFTFLOAT}/source/f128_sqrt.c \
${BERKELEY_SOFTFLOAT}/source/f128_eq.c \
${BERKELEY_SOFTFLOAT}/source/f128_le.c \
${BERKELEY_SOFTFLOAT}/source/f128_lt.c \
${BERKELEY_SOFTFLOAT}/source/f128_eq_signaling.c \
${BERKELEY_SOFTFLOAT}/source/f128_le_quiet.c \
${BERKELEY_SOFTFLOAT}/source/f128_lt_quiet.c \
${BERKELEY_SOFTFLOAT}/source/f128_isSignalingNaN.c \
${BERKELEY_SOFTFLOAT}/source/f128M_to_ui32.c \
${BERKELEY_SOFTFLOAT}/source/f128M_to_ui64.c \
${BERKELEY_SOFTFLOAT}/source/f128M_to_i32.c \
${BERKELEY_SOFTFLOAT}/source/f128M_to_i64.c \
${BERKELEY_SOFTFLOAT}/source/f128M_to_ui32_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/f128M_to_ui64_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/f128M_to_i32_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/f128M_to_i64_r_minMag.c \
${BERKELEY_SOFTFLOAT}/source/f128M_to_f16.c \
${BERKELEY_SOFTFLOAT}/source/f128M_to_f32.c \
${BERKELEY_SOFTFLOAT}/source/f128M_to_extF80M.c \
${BERKELEY_SOFTFLOAT}/source/f128M_to_f64.c \
${BERKELEY_SOFTFLOAT}/source/f128M_roundToInt.c \
${BERKELEY_SOFTFLOAT}/source/f128M_add.c \
${BERKELEY_SOFTFLOAT}/source/f128M_sub.c \
${BERKELEY_SOFTFLOAT}/source/f128M_mul.c \
${BERKELEY_SOFTFLOAT}/source/f128M_mulAdd.c \
${BERKELEY_SOFTFLOAT}/source/f128M_div.c \
${BERKELEY_SOFTFLOAT}/source/f128M_rem.c \
${BERKELEY_SOFTFLOAT}/source/f128M_sqrt.c \
${BERKELEY_SOFTFLOAT}/source/f128M_eq.c \
${BERKELEY_SOFTFLOAT}/source/f128M_le.c \
${BERKELEY_SOFTFLOAT}/source/f128M_lt.c \
${BERKELEY_SOFTFLOAT}/source/f128M_eq_signaling.c \
${BERKELEY_SOFTFLOAT}/source/f128M_le_quiet.c \
${BERKELEY_SOFTFLOAT}/source/f128M_lt_quiet.c 


C_DEFINES = -DSOFTFLOAT_FAST_INT64 -DSOFTFLOAT_ROUND_EVEN -DINLINE_LEVEL=5 -DSOFTFLOAT_FAST_DIV32TO16 -DSOFTFLOAT_FAST_DIV64TO32 -DLITTLEENDIAN
C_FLAGS =  -Wall -Werror-implicit-function-declaration -Wno-conversion ${C_DEFINES}
C_FLAGS += -fPIC ${PLATFORM_CCFLAGS} -I. -I${BERKELEY_SOFTFLOAT} \
                     -I${BERKELEY_SOFTFLOAT}/source/include \
                     -I${BERKELEY_SOFTFLOAT}/build/Linux-x86_64-GCC \
                     -I${BERKELEY_SOFTFLOAT}/source/8086-SSE 
CXXFLAGS += -std=gnu++1z -fPIC ${PLATFORM_CXXFLAGS} -I. -I$(EOSVM_INCLUDE_DIR) \
                                                      -I$(EOSVM_OUTCOME_DIR) \
                                                      -I$(COMPILER_BUILTINS) \
                                                  	  -I$(BERKELEY_SOFTFLOAT) \
								                      -I$(BERKELEY_SOFTFLOAT)/source/include \
								                      -I$(BERKELEY_SOFTFLOAT)/build/Linux-x86_64-GCC \
								                      -I$(BERKELEY_SOFTFLOAT)/source/8086-SSE \
								                      -I$(WASM_TYPES) \
								                      -I$(VM_DIR) \
								                      -I$(WAYKI_SRC_DIR)

LDFLAGS += $(PLATFORM_LDFLAGS) -lboost_unit_test_framework -lboost_system -lssl -lcrypto -g

WASM_EMPTY = 
COMPILER_BUILTINS_PATH_SOURCE = $(COMPILER_BUILTINS)/
COMPILER_BUILTINS_OBJECTS = $(subst $(COMPILER_BUILTINS_PATH_SOURCE), $(WASM_EMPTY), $(COMPILER_BUILTINS_SRC:.c=.o))
#COMPILER_BUILTINS_OBJECTS = fixdfti.o fixsfti.o fixtfti.o fixunsdfti.o fixunssfti.o fixunstfti.o floattidf.o floatuntidf.o

BERKELEY_SOFTFLOAT_PATH_SOURCE = $(BERKELEY_SOFTFLOAT)/source/
BERKELEY_SOFTFLOAT_PATH_SSE = $(BERKELEY_SOFTFLOAT)/source/8086-SSE/
BERKELEY_SOFTFLOAT_OBJECTS= $(subst $(BERKELEY_SOFTFLOAT_PATH_SOURCE), $(WASM_EMPTY), $(subst $(BERKELEY_SOFTFLOAT_PATH_SSE), $(WASM_EMPTY), $(SOFTFLOAT_SRC:.c=.o)))

EOSVM_OBJECTS    =  wasm_interface.o wasm_runtime.o
UINT128_OBJECTS  =  uint128.o
EXCEPTION_OBJECT =  exception.o log_message.o

test:libwasm.a test.cpp wasm_context.cpp
	@echo "building tests"
	@${CXX} -o test ${LDFLAGS} $(CXXFLAGS) test.cpp wasm_context.cpp libwasm.a 
	@rm -rf *.o

libwasm.a:$(WASM_INTERFACE) 
	@echo "building libwasm"
	@${CC}  -c ${LDFLAGS} ${C_FLAGS} ${SOFTFLOAT_SRC}
	@${CC}  -c ${LDFLAGS} ${C_FLAGS} ${OPT} ${COMPILER_BUILTINS_SRC}
	@${CXX} -c ${LDFLAGS} ${CXXFLAGS} ${OPT} ${UINT128_SRC}
	@${CXX} -c ${LDFLAGS} ${CXXFLAGS} ${OPT} ${EXCEPTION_SRC}
	@${CXX} -c ${LDFLAGS} ${CXXFLAGS} ${OPT} ${WASM_INTERFACE} ${WASM_RUNTIME}
	@${AR} rcs $@  ${EOSVM_OBJECTS} ${UINT128_OBJECTS} ${BERKELEY_SOFTFLOAT_OBJECTS} ${COMPILER_BUILTINS_OBJECTS} ${EXCEPTION_OBJECT}
# 	@${AR} rcs $@ ${COMPILER_BUILTINS_OBJECTS} ${BERKELEY_SOFTFLOAT_OBJECTS} ${UINT128_OBJECTS} ${EOSVM_OBJECTS}

clean:
	rm -rf *.o *.a

