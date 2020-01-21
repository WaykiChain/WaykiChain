
WASM_DIR       = ../..
VM_DIR         = ../../..
WAYKI_SRC_DIR  = ../../../..
WAYKI_COMMONS  = ${WAYKI_SRC_DIR}/commons
WASM_TYPES_DIR = $(WASM_DIR)/types
EXCEPTION_DIR  = $(WASM_DIR)/exception

# C_DEFINES = -DSOFTFLOAT_FAST_INT64 -DSOFTFLOAT_ROUND_EVEN -DINLINE_LEVEL=5 -DSOFTFLOAT_FAST_DIV32TO16 -DSOFTFLOAT_FAST_DIV64TO32 -DLITTLEENDIAN
JSON_SPIRIT_SRC = \
${WAYKI_COMMONS}/json/json_spirit_reader.cpp \
${WAYKI_COMMONS}/json/json_spirit_value.cpp \
${WAYKI_COMMONS}/json/json_spirit_writer.cpp 

WASM_EMPTY = 
JSON_SPIRIT_INCLUDE = ${WAYKI_COMMONS}/json
JSON_SPIRIT_OBJECTS = $(subst $(JSON_SPIRIT_INCLUDE)/, $(WASM_EMPTY), $(JSON_SPIRIT_SRC:.cpp=.o))


ABI_SERIALIZER_SRC = \
${WASM_DIR}/abi_serializer.cpp 

ABI_SERIALIZER_OBJECT = abi_serializer.o

WASM_UINT128_SRC = \
${WASM_TYPES_DIR}/uint128.cpp 

WASM_UINT128_OBJECT = uint128.o

EXCEPTION_SRC = \
${EXCEPTION_DIR}/log_message.cpp \
${EXCEPTION_DIR}/exception.cpp 

EXCEPTION_OBJECT = exception.o log_message.o


CXXFLAGS = -std=c++17 -I. -lboost_system -pthread -lboost_unit_test_framework -I${WASM_DIR} -I${VM_DIR} -I${WAYKI_SRC_DIR} -I${WASM_TYPES_DIR} -I${WAYKI_COMMONS} -I${EXCEPTION_DIR}
JSON_CXXFLAGS = -std=c++17 -I. -pthread -I${WASM_DIR} -I${VM_DIR}  -I${WAYKI_SRC_DIR} -I${WASM_TYPES_DIR} 

test:json_spirit.a test_abi.cpp
	@echo "building tests"
	@${CXX} -c $(CXXFLAGS) $(ABI_SERIALIZER_SRC)
	@${CXX} -c $(CXXFLAGS) $(WASM_UINT128_SRC)
	@${CXX} -c $(CXXFLAGS) $(EXCEPTION_SRC)
	@${AR} rcs abi_serializer.a  $(JSON_SPIRIT_OBJECTS) $(ABI_SERIALIZER_OBJECT) $(WASM_UINT128_OBJECT) $(EXCEPTION_OBJECT)
	@${CXX} -o test test_abi.cpp abi_serializer.a $(CXXFLAGS)

json_spirit.a:
	@${CXX} -c $(JSON_CXXFLAGS) $(JSON_SPIRIT_SRC)
	@${AR} rcs json_spirit.a $(JSON_SPIRIT_OBJECTS)


clean:
	rm -rf *.o *.a


