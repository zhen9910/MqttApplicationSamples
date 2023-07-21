# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(CMAKE_CACHEFILE_DIR ${CMAKE_CURRENT_LIST_DIR}/build)
include_directories( ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/protobuf)

link_libraries(
    uuid
    protobuf-c
)

# tinykube_device
add_executable (tinykube_device
  ${MOSQUITTO_CLIENT_EXTENSIONS}
  ${CMAKE_CURRENT_LIST_DIR}/main.c
  ${CMAKE_CURRENT_LIST_DIR}/protobuf/CreateWamrRuntimeCommandRequest.pb-c.c
  ${CMAKE_CURRENT_LIST_DIR}/protobuf/CreateWamrRuntimeCommandResponse.pb-c.c

  # ${CMAKE_CURRENT_LIST_DIR}/protobuf/StartWasmModuleCommandRequest.pb-c.c
  # ${CMAKE_CURRENT_LIST_DIR}/protobuf/StartWasmModuleCommandResponse.pb-c.c
  # ${CMAKE_CURRENT_LIST_DIR}/protobuf/StopWasmModuleCommandRequest.pb-c.c
  # ${CMAKE_CURRENT_LIST_DIR}/protobuf/StopWasmModuleCommandResponse.pb-c.c
)
