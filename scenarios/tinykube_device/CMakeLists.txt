# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(CMAKE_CACHEFILE_DIR ${CMAKE_CURRENT_LIST_DIR}/build)

include_directories( ${CMAKE_CURRENT_LIST_DIR}/src ${CMAKE_CURRENT_LIST_DIR}/protobuf)

# include(CheckPIESupported)

################  runtime settings  ################
string (TOLOWER ${CMAKE_HOST_SYSTEM_NAME} WAMR_BUILD_PLATFORM)
if (APPLE)
  add_definitions(-DBH_PLATFORM_DARWIN)
endif ()

# Reset default linker flags
set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# WAMR features switch

# Set WAMR_BUILD_TARGET, currently values supported:
# "X86_64", "AMD_64", "X86_32", "AARCH64[sub]", "ARM[sub]", "THUMB[sub]",
# "MIPS", "XTENSA", "RISCV64[sub]", "RISCV32[sub]"
if (NOT DEFINED WAMR_BUILD_TARGET)
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)")
    set (WAMR_BUILD_TARGET "AARCH64")
  elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "riscv64")
    set (WAMR_BUILD_TARGET "RISCV64")
  elseif (CMAKE_SIZEOF_VOID_P EQUAL 8)
    # Build as X86_64 by default in 64-bit platform
    set (WAMR_BUILD_TARGET "X86_64")
  elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
    # Build as X86_32 by default in 32-bit platform
    set (WAMR_BUILD_TARGET "X86_32")
  else ()
    message(SEND_ERROR "Unsupported build target platform!")
  endif ()
endif ()

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Release)
endif ()

set (WAMR_BUILD_INTERP 1)
set (WAMR_BUILD_AOT 1)
set (WAMR_BUILD_JIT 0)
set (WAMR_BUILD_LIBC_BUILTIN 1)

if (NOT MSVC)
  set (WAMR_BUILD_LIBC_WASI 1)
endif ()

if (NOT MSVC)
  # linker flags
  if (NOT (CMAKE_C_COMPILER MATCHES ".*clang.*" OR CMAKE_C_COMPILER_ID MATCHES ".*Clang"))
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
  endif ()
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wformat -Wformat-security")
  if (WAMR_BUILD_TARGET MATCHES "X86_.*" OR WAMR_BUILD_TARGET STREQUAL "AMD_64")
    if (NOT (CMAKE_C_COMPILER MATCHES ".*clang.*" OR CMAKE_C_COMPILER_ID MATCHES ".*Clang"))
      set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mindirect-branch-register")
    endif ()
  endif ()
endif ()

# build out vmlib
set (WAMR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../wasm-micro-runtime)
include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
# include (${SHARED_DIR}/utils/uncommon/shared_uncommon.cmake)

add_library(vmlib ${WAMR_RUNTIME_LIB_SOURCE})

################# libWasmRuntimeExecutor  ################
include_directories(${CMAKE_CURRENT_LIST_DIR}, ${CMAKE_CURRENT_LIST_DIR}/inc)
include (${SHARED_DIR}/utils/uncommon/shared_uncommon.cmake)
set (WAMR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../wasm-micro-runtime)

add_library(WasmRuntimeExecutor SHARED
  ${CMAKE_CURRENT_LIST_DIR}/libWasmRuntimeExecutor/lib.c
  # ${WAMR_RUNTIME_LIB_SOURCE}
  ${UNCOMMON_SHARED_SOURCE}
)

set_target_properties(WasmRuntimeExecutor PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
)
target_link_libraries(WasmRuntimeExecutor vmlib -lm -ldl -lpthread -lrt)

################  application related  ################
# tinykube_device
include_directories(${CMAKE_CURRENT_LIST_DIR}, ${CMAKE_CURRENT_LIST_DIR}/inc)
include (${SHARED_DIR}/utils/uncommon/shared_uncommon.cmake)

add_executable (tinykube_device
  ${MOSQUITTO_CLIENT_EXTENSIONS}
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
  ${CMAKE_CURRENT_LIST_DIR}/src/wasm_controller.c
  ${UNCOMMON_SHARED_SOURCE}
  ${CMAKE_CURRENT_LIST_DIR}/protobuf/CreateWamrRuntimeCommandRequest.pb-c.c
  ${CMAKE_CURRENT_LIST_DIR}/protobuf/CreateWamrRuntimeCommandResponse.pb-c.c
  ${CMAKE_CURRENT_LIST_DIR}/protobuf/DestroyWamrRuntimeCommandResponse.pb-c.c
  ${CMAKE_CURRENT_LIST_DIR}/protobuf/Object_AddWasmModule_Request.pb-c.c
  ${CMAKE_CURRENT_LIST_DIR}/protobuf/Array_AddWasmModule_Request_WasmModuleContent.pb-c.c
  ${CMAKE_CURRENT_LIST_DIR}/protobuf/AddWasmModuleCommandRequest.pb-c.c
  ${CMAKE_CURRENT_LIST_DIR}/protobuf/AddWasmModuleCommandResponse.pb-c.c
  ${CMAKE_CURRENT_LIST_DIR}/protobuf/RemoveWasmModuleCommandRequest.pb-c.c
  ${CMAKE_CURRENT_LIST_DIR}/protobuf/RemoveWasmModuleCommandResponse.pb-c.c
  ${CMAKE_CURRENT_LIST_DIR}/protobuf/StartWasmModuleCommandRequest.pb-c.c
  ${CMAKE_CURRENT_LIST_DIR}/protobuf/StartWasmModuleCommandResponse.pb-c.c
  ${CMAKE_CURRENT_LIST_DIR}/protobuf/StopWasmModuleCommandRequest.pb-c.c
  ${CMAKE_CURRENT_LIST_DIR}/protobuf/StopWasmModuleCommandResponse.pb-c.c
)

# check_pie_supported()

target_link_libraries (tinykube_device vmlib uuid protobuf-c -lm -ldl -lpthread -lrt)
