project(test_api)

set(WASM_WASM_OLD_BEHAVIOR "Off")
find_package(wasm.cdt)

add_contract( test_api test_api test_api.cpp test_print.cpp test_types.cpp test_datastream.cpp test_action.cpp)
target_include_directories( test_api PUBLIC ${CMAKE_SOURCE_DIR}/../include )
target_ricardian_directory( test_api ${CMAKE_SOURCE_DIR}/../ricardian )
