/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "logging.h"
#include "mosquitto.h"
#include "mqtt_callbacks.h"
#include "mqtt_protocol.h"
#include "mqtt_setup.h"
#include "tinykube_device.h"

#include "CreateWamrRuntimeCommandRequest.pb-c.h"
#include "CreateWamrRuntimeCommandResponse.pb-c.h"
#include "DestroyWamrRuntimeCommandResponse.pb-c.h"
#include "AddWasmModuleCommandRequest.pb-c.h"
#include "AddWasmModuleCommandResponse.pb-c.h"
#include "RemoveWasmModuleCommandRequest.pb-c.h"
#include "RemoveWasmModuleCommandResponse.pb-c.h"
#include "StartWasmModuleCommandRequest.pb-c.h"
#include "StartWasmModuleCommandResponse.pb-c.h"
#include "StopWasmModuleCommandRequest.pb-c.h"
#include "StopWasmModuleCommandResponse.pb-c.h"

#define ADDRESS     "tcp://broker.hivemq.com"
#define CLIENTID    "f9e57487-616a-4725-aa9f-ee88b611228a"

#define TOPIC_TINYKUBE_CMD_REQ "tinykube/+/command/+"

#define CMD_CREATR_WAMR_RUNTIME   "createWamrRuntime"
#define CMD_DESTROY_WAMR_RUNTIME  "destroyWamrRuntime"
#define CMD_ADD_WASM_MODULE       "addWasmModule"
#define CMD_REMOVE_WASM_MODULE    "removeWasmModule"
#define CMD_START_WASM_MODULE     "startWasmModule"
#define CMD_STOP_WASM_MODULE      "stopWasmModule"

#define QOS_LEVEL 1
#define MQTT_VERSION MQTT_PROTOCOL_V5

#define COMMAND_CONTENT_TYPE "application/protobuf"

#define RETURN_IF_FALSE(rc)                                                    \
  do                                                                           \
  {                                                                            \
    if (rc != true)                                                \
    {                                                                          \
      LOG_ERROR("%s(Line %d): Failure while sending response", __func__, __LINE__); \
      if (response_topic)                                                      \
      {                                                                        \
        free(response_topic);                                                  \
        response_topic = NULL;                                                 \
      }                                                                        \
      if (correlation_data)                                                    \
      {                                                                        \
        free(correlation_data);                                                \
        correlation_data = NULL;                                               \
      }                                                                        \
      mosquitto_property_free_all(&response_props);                            \
      response_props = NULL;                                                   \
      return -1;                                                                  \
    }                                                                          \
  } while (0)

int prepare_and_publish_response(
  struct mosquitto* mosq,
  void* proto_payload_buf,
  unsigned proto_payload_len,
  const mosquitto_property* props)
{
  mosquitto_property* response_props = NULL;
  char* response_topic = NULL;
  void* correlation_data = NULL;
  uint16_t correlation_data_len;

  // read response topic (0x08)
  RETURN_IF_FALSE((mosquitto_property_read_string(props, MQTT_PROP_RESPONSE_TOPIC, &response_topic, false)
      != NULL));
  printf("   response_topic: %s\n", response_topic);

  // add content type (0x03)
  RETURN_IF_FALSE((mosquitto_property_add_string(&response_props, MQTT_PROP_CONTENT_TYPE, COMMAND_CONTENT_TYPE)
      == MOSQ_ERR_SUCCESS));

  // read correlation data (0x09)
  RETURN_IF_FALSE((mosquitto_property_read_binary(
          props, MQTT_PROP_CORRELATION_DATA, &correlation_data, &correlation_data_len, false)
      != NULL));

  // add correlation data (0x09)
  RETURN_IF_FALSE((mosquitto_property_add_binary(
      &response_props, MQTT_PROP_CORRELATION_DATA, correlation_data, correlation_data_len) == MOSQ_ERR_SUCCESS));

  // add topic alias (0x23)
  RETURN_IF_FALSE((mosquitto_property_add_int16(&response_props, MQTT_PROP_TOPIC_ALIAS, 1) == MOSQ_ERR_SUCCESS));

  // add user properties (0x26) with key "Status" and value "200"
  RETURN_IF_FALSE((mosquitto_property_add_string_pair(&response_props, 0x26, "Status", "200") == MOSQ_ERR_SUCCESS));

  // publish response
  RETURN_IF_FALSE((mosquitto_publish_v5(
      mosq,
      NULL,
      response_topic,
      proto_payload_len,
      proto_payload_buf,
      QOS_LEVEL,
      false,
      response_props) == MOSQ_ERR_SUCCESS));
  return 0;
}

int process_create_wamr_runtime_request(
  struct mosquitto* mosq,
  const struct mosquitto_message* message,
  const mosquitto_property* props)
{
  // process the request
  DtmiProtobufTestTinykubePrototype1__CreateWamrRuntimeCommandRequest
    *createWamrRuntimeCommandRequest = 
      dtmi_protobuf_test__tinykube_prototype__1__create_wamr_runtime_command_request__unpack(NULL, message->payloadlen, message->payload);

  if (createWamrRuntimeCommandRequest == NULL)
  {
      fprintf(stderr, "error unpacking protobuf\n");
      return -1;
  }

  printf("   message: heapsize = %d\n", createWamrRuntimeCommandRequest->heapsize);
  LOG_INFO(SERVER_LOG_TAG, "createWamrRuntimeCommandRequest successfully received");
  dtmi_protobuf_test__tinykube_prototype__1__create_wamr_runtime_command_request__free_unpacked(createWamrRuntimeCommandRequest, NULL);

  int req_rst = create_wamr_runtime(createWamrRuntimeCommandRequest->heapsize);

  DtmiProtobufTestTinykubePrototype1__CreateWamrRuntimeCommandResponse
    createWamrRuntimeCommandResponse = DTMI_PROTOBUF_TEST__TINYKUBE_PROTOTYPE__1__CREATE_WAMR_RUNTIME_COMMAND_RESPONSE__INIT;  // Default value
  createWamrRuntimeCommandResponse.has_result = 1;
  createWamrRuntimeCommandResponse.result = req_rst ? -1 : 0;

  // prepare protobuf payload for response
  void* proto_payload_buf;
  unsigned proto_payload_len;

  proto_payload_len = dtmi_protobuf_test__tinykube_prototype__1__create_wamr_runtime_command_response__get_packed_size(&createWamrRuntimeCommandResponse);
  proto_payload_buf = malloc(proto_payload_len);
  if (proto_payload_buf == NULL)
  {
    LOG_ERROR("Failed to allocate memory for payload buffer.");
    return -1;
  }

  int rst = 0;
  size_t pack_size = dtmi_protobuf_test__tinykube_prototype__1__create_wamr_runtime_command_response__pack(&createWamrRuntimeCommandResponse, proto_payload_buf);
  if (pack_size != proto_payload_len)
  {
    LOG_ERROR("Failure serializing payload.");
    rst = -1;
    goto exit;
  }

  // prepare and publish response with associated properties
  rst = prepare_and_publish_response(mosq, proto_payload_buf, proto_payload_len, props);
  if (rst != 0)
  {
    LOG_ERROR("Failure preparing response properties.");
    goto exit;
  }

exit:
  free(proto_payload_buf);
  proto_payload_buf = NULL;
  return rst;
}

int process_destroy_wamr_runtime_request(
  struct mosquitto* mosq,
  const struct mosquitto_message* message,
  const mosquitto_property* props)
{
  int req_rst = destroy_wamr_runtime();
  DtmiProtobufTestTinykubePrototype1__DestroyWamrRuntimeCommandResponse
    destroyWamrRuntimeCommandResponse = DTMI_PROTOBUF_TEST__TINYKUBE_PROTOTYPE__1__DESTROY_WAMR_RUNTIME_COMMAND_RESPONSE__INIT;  // Default value
  destroyWamrRuntimeCommandResponse.has_result = 1;
  destroyWamrRuntimeCommandResponse.result = req_rst ? -1 : 0;

  // prepare protobuf payload for response
  void* proto_payload_buf;
  unsigned proto_payload_len;

  proto_payload_len = dtmi_protobuf_test__tinykube_prototype__1__destroy_wamr_runtime_command_response__get_packed_size(&destroyWamrRuntimeCommandResponse);
  proto_payload_buf = malloc(proto_payload_len);
  if (proto_payload_buf == NULL)
  {
    LOG_ERROR("Failed to allocate memory for payload buffer.");
    return -1;
  }

  int rst = 0;
  size_t pack_size = dtmi_protobuf_test__tinykube_prototype__1__destroy_wamr_runtime_command_response__pack(&destroyWamrRuntimeCommandResponse, proto_payload_buf);
  if (pack_size != proto_payload_len)
  {
    LOG_ERROR("Failure serializing payload.");
    rst = -1;
    goto exit;
  }

  // prepare and publish response with associated properties
  rst = prepare_and_publish_response(mosq, proto_payload_buf, proto_payload_len, props);
  if (rst != 0)
  {
    LOG_ERROR("Failure preparing response properties.");
    goto exit;
  }

exit:
  free(proto_payload_buf);
  proto_payload_buf = NULL;
  return rst;

}

int process_add_wasm_module_request(
  struct mosquitto* mosq,
  const struct mosquitto_message* message,
  const mosquitto_property* props)
{
  int result = 0;
  // process the request
  DtmiProtobufTestTinykubePrototype1__AddWasmModuleCommandRequest
    *addWasmModuleCommandRequest = 
      dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_request__unpack(NULL, message->payloadlen, message->payload);
  
  if (addWasmModuleCommandRequest == NULL)
  {
      fprintf(stderr, "error unpacking protobuf\n");
      result = -1;
      goto pack_resp;
  }

  char *wasm_module_name = addWasmModuleCommandRequest->wasmmodule->wasmmodulename;
  int wasm_module_size = addWasmModuleCommandRequest->wasmmodule->wasmmodulesize;
  DtmiProtobufTestTinykubePrototype1__ArrayAddWasmModuleRequestWasmModuleContent
    *wasm_module_content = addWasmModuleCommandRequest->wasmmodule->wasmmodulecontent;
  printf("   AddWasmModule: wasmModuleName = %s, wasmModuleSize = %d\n", wasm_module_name, wasm_module_size);

  if (wasm_module_content == NULL) {
    fprintf(stderr, "AddWasmModule: ModuleContent is null");
    result = -1;
    goto pack_resp;
  }

  if (wasm_module_size != (int)wasm_module_content->n_value) {
    fprintf(stderr, "AddWasmModule: wasm_module_size = %d, wasmModuleContent->n_value = %d\n",
        wasm_module_size, (int)wasm_module_content->n_value);
    result = -1;
    goto pack_resp;
  }

  result = add_wasm_module(wasm_module_name, wasm_module_content->value, wasm_module_content->n_value);
  if (result != 0) {
    fprintf(stderr, "AddWasmModule: Failed to save wasm module");
    goto pack_resp;
  }

pack_resp:
  // prepare protobuf payload for response
  void* proto_payload_buf;
  unsigned proto_payload_len;

  DtmiProtobufTestTinykubePrototype1__AddWasmModuleCommandResponse
    addWasmModuleCommandResponse = DTMI_PROTOBUF_TEST__TINYKUBE_PROTOTYPE__1__ADD_WASM_MODULE_COMMAND_RESPONSE__INIT;  // Default value
  addWasmModuleCommandResponse.has_result = 1;
  addWasmModuleCommandResponse.result = result;

  proto_payload_len = dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_response__get_packed_size(&addWasmModuleCommandResponse);
  proto_payload_buf = malloc(proto_payload_len);
  if (proto_payload_buf == NULL)
  {
    LOG_ERROR("Failed to allocate memory for payload buffer.");
    return -1;
  }

  int rst = 0;
  size_t pack_size = dtmi_protobuf_test__tinykube_prototype__1__add_wasm_module_command_response__pack(&addWasmModuleCommandResponse, proto_payload_buf);
  if (pack_size != proto_payload_len)
  {
    LOG_ERROR("Failure serializing payload.");
    rst = -1;
    goto exit;
  }

  // prepare and publish response with associated properties
  rst = prepare_and_publish_response(mosq, proto_payload_buf, proto_payload_len, props);
  if (rst != 0)
  {
    LOG_ERROR("Failure preparing response properties.");
    goto exit;
  }

exit:
  free(proto_payload_buf);
  proto_payload_buf = NULL;
  return rst;
}

int process_remove_wasm_module_request(
  struct mosquitto* mosq,
  const struct mosquitto_message* message,
  const mosquitto_property* props)
{
  int result = 0;
  // process the request
  char *message_payload = (char *)message->payload;
  DtmiProtobufTestTinykubePrototype1__RemoveWasmModuleCommandRequest
    *removeWasmModuleCommandRequest = 
      dtmi_protobuf_test__tinykube_prototype__1__remove_wasm_module_command_request__unpack(NULL, message->payloadlen, message_payload);
  
  if (removeWasmModuleCommandRequest == NULL)
  {
      fprintf(stderr, "error unpacking protobuf\n");
      result = -1;
      goto pack_resp;
  }

  char *wasm_module_name = removeWasmModuleCommandRequest->wasmmodulename;
  printf("   RemoveWasmModule: wasmModuleName = %s\n", wasm_module_name);

  result = remove_wasm_module(wasm_module_name);
  if (result != 0) {
    fprintf(stderr, "RemoveWasmModule: Failed to remove wasm module");
    goto pack_resp;
  }

pack_resp:
  // prepare protobuf payload for response
  void* proto_payload_buf;
  unsigned proto_payload_len;

  DtmiProtobufTestTinykubePrototype1__RemoveWasmModuleCommandResponse
    removeWasmModuleCommandResponse = DTMI_PROTOBUF_TEST__TINYKUBE_PROTOTYPE__1__REMOVE_WASM_MODULE_COMMAND_RESPONSE__INIT;  // Default value

  proto_payload_len = dtmi_protobuf_test__tinykube_prototype__1__remove_wasm_module_command_response__get_packed_size(&removeWasmModuleCommandResponse);
  proto_payload_buf = malloc(proto_payload_len);
  if (proto_payload_buf == NULL)
  {
    LOG_ERROR("Failed to allocate memory for payload buffer.");
    return -1;
  }

  int rst = 0;
  size_t pack_size = dtmi_protobuf_test__tinykube_prototype__1__remove_wasm_module_command_response__pack(&removeWasmModuleCommandResponse, proto_payload_buf);
  if (pack_size != proto_payload_len)
  {
    LOG_ERROR("Failure serializing payload.");
    rst = -1;
    goto exit;
  }

  // prepare and publish response with associated properties
  rst = prepare_and_publish_response(mosq, proto_payload_buf, proto_payload_len, props);
  if (rst != 0)
  {
    LOG_ERROR("Failure preparing response properties.");
    goto exit;
  }

exit:
  free(proto_payload_buf);
  proto_payload_buf = NULL;
  return 0;
}

int process_start_wasm_module_request(
  struct mosquitto* mosq,
  const struct mosquitto_message* message,
  const mosquitto_property* props)
{
  int result = 0;
  // process the request
  char *message_payload = (char *)message->payload;
  DtmiProtobufTestTinykubePrototype1__StartWasmModuleCommandRequest
    *startWasmModuleCommandRequest = 
             dtmi_protobuf_test__tinykube_prototype__1__start_wasm_module_command_request__unpack(NULL, message->payloadlen, message_payload);
  
  if (startWasmModuleCommandRequest == NULL)
  {
      fprintf(stderr, "error unpacking protobuf\n");
      result = -1;
      goto pack_resp;
  }

  char *wasm_module_name = startWasmModuleCommandRequest->wasmmodulename;
  printf("   StartWasmModule: wasmModuleName = %s\n", wasm_module_name);

  result = start_wasm_module_v2(wasm_module_name);
  if (result != 0) {
    fprintf(stderr, "StartWasmModule: Failed to start wasm module");
    goto pack_resp;
  }

pack_resp:
  // prepare protobuf payload for response
  void* proto_payload_buf;
  unsigned proto_payload_len;

  DtmiProtobufTestTinykubePrototype1__StartWasmModuleCommandResponse
    startWasmModuleCommandResponse = DTMI_PROTOBUF_TEST__TINYKUBE_PROTOTYPE__1__START_WASM_MODULE_COMMAND_RESPONSE__INIT;  // Default value
  
  proto_payload_len = dtmi_protobuf_test__tinykube_prototype__1__start_wasm_module_command_response__get_packed_size(&startWasmModuleCommandResponse);
  proto_payload_buf = malloc(proto_payload_len);
  if (proto_payload_buf == NULL)
  {
    LOG_ERROR("Failed to allocate memory for payload buffer.");
    return -1;
  }

  int rst = 0;
  size_t pack_size = dtmi_protobuf_test__tinykube_prototype__1__start_wasm_module_command_response__pack(&startWasmModuleCommandResponse, proto_payload_buf);
  if (pack_size != proto_payload_len)
  {
    LOG_ERROR("Failure serializing payload.");
    rst = -1;
    goto exit;
  }

  // prepare and publish response with associated properties
  rst = prepare_and_publish_response(mosq, proto_payload_buf, proto_payload_len, props);
  if (rst != 0)
  {
    LOG_ERROR("Failure preparing response properties.");
    goto exit;
  }

exit:
  free(proto_payload_buf);
  proto_payload_buf = NULL;
  return 0;
}

int process_stop_wasm_module_request(
  struct mosquitto* mosq,
  const struct mosquitto_message* message,
  const mosquitto_property* props)
{
  int result = 0;
  // process the request
  char *message_payload = (char *)message->payload;
  DtmiProtobufTestTinykubePrototype1__StopWasmModuleCommandRequest
    *stopWasmModuleCommandRequest = 
      dtmi_protobuf_test__tinykube_prototype__1__stop_wasm_module_command_request__unpack(NULL, message->payloadlen, message_payload);
  
  if (stopWasmModuleCommandRequest == NULL)
  {
      fprintf(stderr, "error unpacking protobuf\n");
      result = -1;
      goto pack_resp;
  }

  char *wasm_module_name = stopWasmModuleCommandRequest->wasmmodulename;
  printf("   StopWasmModule: wasmModuleName = %s\n", wasm_module_name);

  result = stop_wasm_module_v2(wasm_module_name);
  if (result != 0) {
    fprintf(stderr, "StopWasmModule: Failed to stop wasm module\n");
    goto pack_resp;
  }

pack_resp:
  // prepare protobuf payload for response
  void* proto_payload_buf;
  unsigned proto_payload_len;

  DtmiProtobufTestTinykubePrototype1__StopWasmModuleCommandResponse
    stopWasmModuleCommandResponse = DTMI_PROTOBUF_TEST__TINYKUBE_PROTOTYPE__1__STOP_WASM_MODULE_COMMAND_RESPONSE__INIT;  // Default value
  
  proto_payload_len = dtmi_protobuf_test__tinykube_prototype__1__stop_wasm_module_command_response__get_packed_size(&stopWasmModuleCommandResponse);
  proto_payload_buf = malloc(proto_payload_len);
  if (proto_payload_buf == NULL)
  {
    LOG_ERROR("Failed to allocate memory for payload buffer.");
    return -1;
  }

  int rst = 0;
  size_t pack_size = dtmi_protobuf_test__tinykube_prototype__1__stop_wasm_module_command_response__pack(&stopWasmModuleCommandResponse, proto_payload_buf);
  if (pack_size != proto_payload_len)
  {
    LOG_ERROR("Failure serializing payload.");
    rst = -1;
    goto exit;
  }
  
  // prepare and publish response with associated properties
  rst = prepare_and_publish_response(mosq, proto_payload_buf, proto_payload_len, props);
  if (rst != 0)
  {
    LOG_ERROR("Failure preparing response properties.");
    goto exit;
  }

exit:
  free(proto_payload_buf);
  proto_payload_buf = NULL;
  return 0;
}

// Custom callback for when a message is received.
// Executes tinykube request and sends the response.
void handle_message(
    struct mosquitto* mosq,
    const struct mosquitto_message* message,
    const mosquitto_property* props)
{
    printf("Received message on topic %s\n", message->topic);
    printf("    mqtt_message length = %d\n", message->payloadlen);
    // extract substring from the last "/"
    char *topicName = strrchr(message->topic, '/');
    if (topicName == NULL)
    {
        printf("Invalid topic: %s\n", message->topic);
        return;
    }

    if (strcmp(topicName + 1, CMD_CREATR_WAMR_RUNTIME) == 0) {
        printf("Creating WAMR runtime.\n");
        if (process_create_wamr_runtime_request(mosq, message, props) != 0) {
            printf("Failed to create WAMR runtime.\n");
        }
    } else if (strcmp(topicName + 1, CMD_DESTROY_WAMR_RUNTIME) == 0) {
        printf("Destroying WAMR runtime.\n");
        if (process_destroy_wamr_runtime_request(mosq, message, props) != 0) {
            printf("Failed to destroy WAMR runtime.\n");
        }
    } else if (strcmp(topicName + 1, CMD_ADD_WASM_MODULE) == 0) {
        printf("Adding WASM module.\n");
        if (process_add_wasm_module_request(mosq, message, props) != 0) {
            printf("Failed to add WASM module.\n");
        }
    } else if (strcmp(topicName + 1, CMD_REMOVE_WASM_MODULE) == 0) {
        printf("Removing WASM module.\n");
        if (process_remove_wasm_module_request(mosq, message, props) != 0) {
            printf("Failed to remove WASM module.\n");
        }
    } else if (strcmp(topicName + 1, CMD_START_WASM_MODULE) == 0) {
        printf("Starting WASM module.\n");
        if (process_start_wasm_module_request(mosq, message, props) != 0) {
            printf("Failed to start WASM module.\n");
        }
    } else if (strcmp(topicName + 1, CMD_STOP_WASM_MODULE) == 0) {
        printf("Stopping WASM module.\n");
        if (process_stop_wasm_module_request(mosq, message, props) != 0) {
            printf("Failed to stop WASM module.\n");
        }
    } else {
        printf("Unknown topicName %s.\n", topicName + 1);
    }

}

/* Callback called when the client receives a CONNACK message from the broker and we want to
 * subscribe on connect. */
void on_connect_with_subscribe(
    struct mosquitto* mosq,
    void* obj,
    int reason_code,
    int flags,
    const mosquitto_property* props)
{
  on_connect(mosq, obj, reason_code, flags, props);

  int result;
  /* Making subscriptions in the on_connect() callback means that if the
   * connection drops and is automatically resumed by the client, then the
   * subscriptions will be recreated when the client reconnects. */
  if (keep_running
      && (result = mosquitto_subscribe_v5(mosq, NULL, TOPIC_TINYKUBE_CMD_REQ, QOS_LEVEL, 0, NULL))
          != MOSQ_ERR_SUCCESS)
  {
    LOG_ERROR("Failed to subscribe: %s", mosquitto_strerror(result));
    keep_running = 0;
    /* We might as well disconnect if we were unable to subscribe */
    if ((result = mosquitto_disconnect_v5(mosq, reason_code, props)) != MOSQ_ERR_SUCCESS)
    {
      LOG_ERROR("Failed to disconnect: %s", mosquitto_strerror(result));
    }
  }
}

/*
 * This sample receives commands from a client and responds.
 */
int main(int argc, char* argv[])
{
  struct mosquitto* mosq;
  int result = MOSQ_ERR_SUCCESS;

  mqtt_client_obj obj;
  obj.handle_message = handle_message;
  obj.mqtt_version = MQTT_VERSION;

  if ((mosq = mqtt_client_init(true, argv[1], on_connect_with_subscribe, &obj)) == NULL)
  {
    result = MOSQ_ERR_UNKNOWN;
  }
  else if (
      (result = mosquitto_connect_bind_v5(
           mosq, obj.hostname, obj.tcp_port, obj.keep_alive_in_seconds, NULL, NULL))
      != MOSQ_ERR_SUCCESS)
  {
    LOG_ERROR("Failed to connect: %s", mosquitto_strerror(result));
    result = MOSQ_ERR_UNKNOWN;
  }
  else if ((result = mosquitto_loop_start(mosq)) != MOSQ_ERR_SUCCESS)
  {
    LOG_ERROR("Failure starting mosquitto loop: %s", mosquitto_strerror(result));
    result = MOSQ_ERR_UNKNOWN;
  }
  else
  {
    open_runtime_lib();
    while (keep_running)
    {
    }
  }

  if (mosq != NULL)
  {
    mosquitto_disconnect_v5(mosq, result, NULL);
    mosquitto_loop_stop(mosq, false);
    mosquitto_destroy(mosq);
  }
  mosquitto_lib_cleanup();
  return result;
}
