# ##################################################################################################
# Utilities for setting up various builds.
# ##################################################################################################
include(CMakeDependentOption)

# ##################################################################################################
# Check if trying to build with a supported compiler.
# ##################################################################################################
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
   message(FATAL "Compiler currently not supported.")
endif()

# ##################################################################################################
# Setup CCACHE
# ##################################################################################################
option(ENABLE_CCACHE "enable building with CCACHE or SCCACHE if they are present" ON)

if(ENABLE_CCACHE)
   find_program(CCACHE_PROGRAM ccache)
   if(CCACHE_PROGRAM)
      message(STATUS "eos-vm using ccache")
      set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
      set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CCACHE_PROGRAM}")
   endif()
endif()

# ##################################################################################################
# Setup profile builds
# ##################################################################################################
option(ENABLE_PROFILE "enable profile build" OFF)
option(ENABLE_GPERFTOOLS "enable gperftools" OFF)

if(ENABLE_PROFILE)
   message(STATUS "Building with profiling information.")
   add_compile_options("-pg")
endif()

# ##################################################################################################
# Setup santized builds
# ##################################################################################################
cmake_dependent_option(ENABLE_ADDRESS_SANITIZER "build with address sanitization" OFF
                       "NOT ENABLE_PROFILE;NOT ENABLE_GPERFTOOLS" OFF)
cmake_dependent_option(ENABLE_UNDEFINED_BEHAVIOR_SANITIZER
		       "build with undefined behavior sanitization" OFF
                       "NOT ENABLE_PROFILE;NOT ENABLE_GPERFTOOLS" OFF)

if(ENABLE_ADDRESS_SANITIZER)
   message(STATUS "Building with address sanitization.")
   add_compile_options("-fsanitize=address")
   set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()

if(ENABLE_UNDEFINED_BEHAVIOR_SANITIZER)
   message(STATUS "Building with undefined behavior sanitization.")
   add_compile_options("-fsanitize=undefined")
   set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined")
endif()
