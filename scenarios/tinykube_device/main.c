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

#include "CreateWamrRuntimeCommandRequest.pb-c.h"
#include "CreateWamrRuntimeCommandResponse.pb-c.h"

#define ADDRESS     "tcp://broker.hivemq.com"
#define CLIENTID    "f9e57487-616a-4725-aa9f-ee88b611228a"

//#define TOPIC_CREATE_WAMR_RUNTIME_REQ "tinykube/f9e57487-616a-4725-aa9f-ee88b611228a/command/createWamrRuntime/"
#define TOPIC_CREATE_WAMR_RUNTIME_REQ "tinykube/+/command/createWamrRuntime"
#define TOPIC_CREATE_WAMR_RUNTIME_RSP "tinykube/f9e57487-616a-4725-aa9f-ee88b611228a/command/createWamrRuntime/__for_30657190-2dea-42dd-a438-0b77483446ae"
// #define TOPIC_CREATE_WAMR_RUNTIME_RSP "tinykube/+/command/createWamrRuntime/__for_30657190-2dea-42dd-a438-0b77483446ae"

#define QOS_LEVEL 1
#define MQTT_VERSION MQTT_PROTOCOL_V5

#define COMMAND_CONTENT_TYPE "application/protobuf"

#define RETURN_IF_ERROR(rc)                                                    \
  do                                                                           \
  {                                                                            \
    if (rc != MOSQ_ERR_SUCCESS)                                                \
    {                                                                          \
      LOG_ERROR("Failure while sending response: %s", mosquitto_strerror(rc)); \
      free(response_topic);                                                    \
      response_topic = NULL;                                                   \
      free(correlation_data);                                                  \
      correlation_data = NULL;                                                 \
      mosquitto_property_free_all(&response_props);                            \
      response_props = NULL;                                                   \
      free(payload_buf);                                                       \
      payload_buf = NULL;                                                      \
      return;                                                                  \
    }                                                                          \
  } while (0)

// Function to execute unlock request. For this sample, it just prints the request information.
bool process_create_wamr_runtime_command_request(char* payload, int payload_length)
{

  DtmiProtobufTestTinykubePrototype1__CreateWamrRuntimeCommandRequest
    *createWamrRuntimeCommandRequest = 
      dtmi_protobuf_test__tinykube_prototype__1__create_wamr_runtime_command_request__unpack(NULL, payload_length, payload);

  if (createWamrRuntimeCommandRequest == NULL)
  {
      fprintf(stderr, "error unpacking protobuf\n");
      return false;
  }

  printf("   message: heapsize = %d\n", createWamrRuntimeCommandRequest->heapsize);
  LOG_INFO(SERVER_LOG_TAG, "createWamrRuntimeCommandRequest successfully received");
  dtmi_protobuf_test__tinykube_prototype__1__create_wamr_runtime_command_request__free_unpacked(createWamrRuntimeCommandRequest, NULL);
  return true;
}

// Custom callback for when a message is received.
// Executes vehicle unlock and sends the response.
void handle_message(
    struct mosquitto* mosq,
    const struct mosquitto_message* message,
    const mosquitto_property* props)
{
  char* response_topic;
  void* correlation_data;
  uint16_t correlation_data_len;
  mosquitto_property* response_props = NULL;

  printf("Received message on topic %s\n", message->topic);
  bool command_succeed = process_create_wamr_runtime_command_request(message->payload, message->payloadlen);

  DtmiProtobufTestTinykubePrototype1__CreateWamrRuntimeCommandResponse
    createWamrRuntimeCommandResponse = DTMI_PROTOBUF_TEST__TINYKUBE_PROTOTYPE__1__CREATE_WAMR_RUNTIME_COMMAND_RESPONSE__INIT;  // Default value
  createWamrRuntimeCommandResponse.has_result = 1;
  // createWamrRuntimeCommandResponse.result = 1000;
  createWamrRuntimeCommandResponse.result = 0;

  void* payload_buf;
  unsigned proto_payload_len;
  if (command_succeed == false)
  {
    createWamrRuntimeCommandResponse.result = 1001;
  }

  proto_payload_len = dtmi_protobuf_test__tinykube_prototype__1__create_wamr_runtime_command_response__get_packed_size(&createWamrRuntimeCommandResponse);
  payload_buf = malloc(proto_payload_len);
  if (payload_buf == NULL)
  {
    LOG_ERROR("Failed to allocate memory for payload buffer.");
    return;
  }

  size_t pack_size = dtmi_protobuf_test__tinykube_prototype__1__create_wamr_runtime_command_response__pack(&createWamrRuntimeCommandResponse, payload_buf);
  if (pack_size != proto_payload_len)
  {
    LOG_ERROR("Failure serializing payload.");
    free(payload_buf);
    payload_buf = NULL;
    return;
  }

  if (mosquitto_property_read_string(props, MQTT_PROP_RESPONSE_TOPIC, &response_topic, false)
      == NULL)
  {
    LOG_ERROR("Message does not have a response topic property");
    return;
  }
  printf("   response_topic: %s\n", response_topic);

  // add content type (0x03)
  RETURN_IF_ERROR(
      mosquitto_property_add_string(&response_props, MQTT_PROP_CONTENT_TYPE, COMMAND_CONTENT_TYPE));

  if (mosquitto_property_read_binary(
          props, MQTT_PROP_CORRELATION_DATA, &correlation_data, &correlation_data_len, false)
      == NULL)
  {
    LOG_ERROR("Message does not have a correlation data property");
    return;
  }

  // add correlation data (0x09)
  RETURN_IF_ERROR(mosquitto_property_add_binary(
      &response_props, MQTT_PROP_CORRELATION_DATA, correlation_data, correlation_data_len));

  // add topic alias (0x23)
  RETURN_IF_ERROR(mosquitto_property_add_int16(&response_props, MQTT_PROP_TOPIC_ALIAS, 1));

  // add user properties (0x26) with key "Status" and value "200"
  RETURN_IF_ERROR(mosquitto_property_add_string_pair(&response_props, 0x26, "Status", "200"));
  

  // LOG_INFO(
  //     SERVER_LOG_TAG,
  //     "Sending response (on topic %s):\n\tSucceed: %s",
  //     response_topic,
  //     proto_unlock_response.succeed ? "True" : "False");
  // if (command_succeed == false)
  // {
  //   printf("\tError: %s\n", proto_unlock_response.errordetail);
  // }

  RETURN_IF_ERROR(mosquitto_publish_v5(
      mosq,
      NULL,
      response_topic,
      proto_payload_len,
      payload_buf,
      QOS_LEVEL,
      false,
      response_props));

  // free(response_topic);
  response_topic = NULL;
  free(correlation_data);
  correlation_data = NULL;
  mosquitto_property_free_all(&response_props);
  response_props = NULL;
  free(payload_buf);
  payload_buf = NULL;
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
  mqtt_client_obj* client_obj = (mqtt_client_obj*)obj;
  // char sub_topic[strlen(client_obj->client_id) + 33];
  // char sub_topic[strlen(client_obj->client_id) + 33];
  // sprintf(sub_topic, "vehicles/%s/command/unlock/request", client_obj->client_id);
  // sprintf(sub_topic, TOPIC_CREATE_WAMR_RUNTIME_REQ);

  /* Making subscriptions in the on_connect() callback means that if the
   * connection drops and is automatically resumed by the client, then the
   * subscriptions will be recreated when the client reconnects. */
  if (keep_running
      && (result = mosquitto_subscribe_v5(mosq, NULL, TOPIC_CREATE_WAMR_RUNTIME_REQ, QOS_LEVEL, 0, NULL))
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
