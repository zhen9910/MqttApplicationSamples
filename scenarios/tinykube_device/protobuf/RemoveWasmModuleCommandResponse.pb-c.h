/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: RemoveWasmModuleCommandResponse.proto */

#ifndef PROTOBUF_C_RemoveWasmModuleCommandResponse_2eproto__INCLUDED
#define PROTOBUF_C_RemoveWasmModuleCommandResponse_2eproto__INCLUDED

#include <protobuf-c/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1000000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1003003 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif


typedef struct _DtmiProtobufTestTinykubePrototype1__RemoveWasmModuleCommandResponse DtmiProtobufTestTinykubePrototype1__RemoveWasmModuleCommandResponse;


/* --- enums --- */


/* --- messages --- */

struct  _DtmiProtobufTestTinykubePrototype1__RemoveWasmModuleCommandResponse
{
  ProtobufCMessage base;
  protobuf_c_boolean has_result;
  int32_t result;
};
#define DTMI_PROTOBUF_TEST__TINYKUBE_PROTOTYPE__1__REMOVE_WASM_MODULE_COMMAND_RESPONSE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&dtmi_protobuf_test__tinykube_prototype__1__remove_wasm_module_command_response__descriptor) \
    , 0, 0 }


/* DtmiProtobufTestTinykubePrototype1__RemoveWasmModuleCommandResponse methods */
void   dtmi_protobuf_test__tinykube_prototype__1__remove_wasm_module_command_response__init
                     (DtmiProtobufTestTinykubePrototype1__RemoveWasmModuleCommandResponse         *message);
size_t dtmi_protobuf_test__tinykube_prototype__1__remove_wasm_module_command_response__get_packed_size
                     (const DtmiProtobufTestTinykubePrototype1__RemoveWasmModuleCommandResponse   *message);
size_t dtmi_protobuf_test__tinykube_prototype__1__remove_wasm_module_command_response__pack
                     (const DtmiProtobufTestTinykubePrototype1__RemoveWasmModuleCommandResponse   *message,
                      uint8_t             *out);
size_t dtmi_protobuf_test__tinykube_prototype__1__remove_wasm_module_command_response__pack_to_buffer
                     (const DtmiProtobufTestTinykubePrototype1__RemoveWasmModuleCommandResponse   *message,
                      ProtobufCBuffer     *buffer);
DtmiProtobufTestTinykubePrototype1__RemoveWasmModuleCommandResponse *
       dtmi_protobuf_test__tinykube_prototype__1__remove_wasm_module_command_response__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   dtmi_protobuf_test__tinykube_prototype__1__remove_wasm_module_command_response__free_unpacked
                     (DtmiProtobufTestTinykubePrototype1__RemoveWasmModuleCommandResponse *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*DtmiProtobufTestTinykubePrototype1__RemoveWasmModuleCommandResponse_Closure)
                 (const DtmiProtobufTestTinykubePrototype1__RemoveWasmModuleCommandResponse *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor dtmi_protobuf_test__tinykube_prototype__1__remove_wasm_module_command_response__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_RemoveWasmModuleCommandResponse_2eproto__INCLUDED */
