/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: AddWasmModuleCommandRequest.proto */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C__NO_DEPRECATED
#define PROTOBUF_C__NO_DEPRECATED
#endif

#include "AddWasmModuleCommandRequest.pb-c.h"
void   dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_request__init
                     (DtmiProtobufTestTinykubePrototype1__AddWasmModuleCommandRequest         *message)
{
  static const DtmiProtobufTestTinykubePrototype1__AddWasmModuleCommandRequest init_value = DTMI_PROTOBUF_TEST__TINYKUBE_PROTOTYPE__1__ADD_WASM_MODULE_COMMAND_REQUEST__INIT;
  *message = init_value;
}
size_t dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_request__get_packed_size
                     (const DtmiProtobufTestTinykubePrototype1__AddWasmModuleCommandRequest *message)
{
  assert(message->base.descriptor == &dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_request__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_request__pack
                     (const DtmiProtobufTestTinykubePrototype1__AddWasmModuleCommandRequest *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_request__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_request__pack_to_buffer
                     (const DtmiProtobufTestTinykubePrototype1__AddWasmModuleCommandRequest *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_request__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
DtmiProtobufTestTinykubePrototype1__AddWasmModuleCommandRequest *
       dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_request__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (DtmiProtobufTestTinykubePrototype1__AddWasmModuleCommandRequest *)
     protobuf_c_message_unpack (&dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_request__descriptor,
                                allocator, len, data);
}
void   dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_request__free_unpacked
                     (DtmiProtobufTestTinykubePrototype1__AddWasmModuleCommandRequest *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_request__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCFieldDescriptor dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_request__field_descriptors[1] =
{
  {
    "wasmModule",
    1,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_MESSAGE,
    0,   /* quantifier_offset */
    offsetof(DtmiProtobufTestTinykubePrototype1__AddWasmModuleCommandRequest, wasmmodule),
    &dtmi_protobuf_test__tinykube_prototype__1__object__add_wasm_module__request__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_request__field_indices_by_name[] = {
  0,   /* field[0] = wasmModule */
};
static const ProtobufCIntRange dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_request__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 1 }
};
const ProtobufCMessageDescriptor dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_request__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "dtmi_protobufTest_TinykubePrototype__1.AddWasmModuleCommandRequest",
  "AddWasmModuleCommandRequest",
  "DtmiProtobufTestTinykubePrototype1__AddWasmModuleCommandRequest",
  "dtmi_protobufTest_TinykubePrototype__1",
  sizeof(DtmiProtobufTestTinykubePrototype1__AddWasmModuleCommandRequest),
  1,
  dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_request__field_descriptors,
  dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_request__field_indices_by_name,
  1,  dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_request__number_ranges,
  (ProtobufCMessageInit) dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_request__init,
  NULL,NULL,NULL    /* reserved[123] */
};
