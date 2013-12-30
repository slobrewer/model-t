
#include <ch.h>
#include <hal.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <snacka/websocket.h>

#include <pb_encode.h>
#include <pb_decode.h>

#include "web_api.h"
#include "bbmt.pb.h"
#include "message.h"
#include "net.h"
#include "sensor.h"
#include "app_cfg.h"

#ifndef WEB_API_HOST
#define WEB_API_HOST_STR "brewbit.herokuapp.com"
#else
#define xstr(s) str(s)
#define str(s) #s
#define WEB_API_HOST_STR xstr(WEB_API_HOST)
#endif

#ifndef WEB_API_PORT
#define WEB_API_PORT 80
#endif


typedef struct {
  snWebsocket* ws;
  api_state_t state;
  char activation_token[64];
} web_api_t;


static void
set_state(web_api_t* api, api_state_t state);

static void
web_api_dispatch(msg_id_t id, void* msg_data, void* listener_data, void* sub_data);

static void
web_api_idle(web_api_t* api);

static void
websocket_message_rx(void* userData, snOpcode opcode, const char* data, int numBytes);

static void
send_api_msg(snWebsocket* ws, ApiMessage* msg);

static void
dispatch_api_msg(web_api_t* api, ApiMessage* msg);

static void
dispatch_sensor_sample(web_api_t* api, sensor_msg_t* sample);

static void
dispatch_sensor_timeout(web_api_t* api, sensor_timeout_msg_t* timeout);

static void
request_activation_token(web_api_t* api);

static void
request_auth(web_api_t* api);

static void
check_for_update(web_api_t* api);

static void
start_update(web_api_t* api, const char* ver);


static web_api_t* api;


void
web_api_init()
{
  api = calloc(1, sizeof(web_api_t));
  api->state = AS_AWAITING_NET_CONNECTION;

  api->ws = snWebsocket_create(
        NULL, // open callback
        websocket_message_rx,
        NULL, // closed callback
        NULL, // error callback
        api);

  msg_listener_t* l = msg_listener_create("web_api", 2048, web_api_dispatch, api);
  msg_listener_set_idle_timeout(l, 250);

  msg_subscribe(l, MSG_NET_STATUS, NULL);
  msg_subscribe(l, MSG_API_FW_UPDATE_CHECK, NULL);
  msg_subscribe(l, MSG_API_FW_DNLD_START, NULL);
  msg_subscribe(l, MSG_SENSOR_SAMPLE, NULL);
  msg_subscribe(l, MSG_SENSOR_TIMEOUT, NULL);
}

api_state_t
web_api_get_status()
{
  return api->state;
}

static void
web_api_dispatch(msg_id_t id, void* msg_data, void* listener_data, void* sub_data)
{
  (void)sub_data;
  (void)msg_data;

  web_api_t* api = listener_data;

  switch (id) {
    case MSG_NET_STATUS:
    {
      net_status_t* ns = msg_data;
      if (ns->net_state == NS_CONNECTED)
        set_state(api, AS_CONNECTING);
      else
        set_state(api, AS_AWAITING_NET_CONNECTION);
      break;
    }

    case MSG_IDLE:
      web_api_idle(api);
      break;

    default:
      break;
  }

  // Only process the following if the API connection has been established
  if (api->state == AS_CONNECTED) {
    switch (id) {
      case MSG_SENSOR_SAMPLE:
        dispatch_sensor_sample(api, msg_data);
        break;

      case MSG_SENSOR_TIMEOUT:
        dispatch_sensor_timeout(api, msg_data);
        break;

      case MSG_API_FW_UPDATE_CHECK:
        check_for_update(api);
        break;

      case MSG_API_FW_DNLD_START:
        start_update(api, msg_data);
        break;

      default:
        break;
    }
  }
}

static void
set_state(web_api_t* api, api_state_t state)
{
  if (api->state != state) {
    api->state = state;

    api_status_t status_msg = {
        .state = state
    };
    msg_send(MSG_API_STATUS, &status_msg);
  }
}

static void
web_api_idle(web_api_t* api)
{
  switch (snWebsocket_getState(api->ws)) {
    case SN_STATE_CLOSED:
      snWebsocket_connect(api->ws, WEB_API_HOST_STR, NULL, NULL, WEB_API_PORT);
      break;

    case SN_STATE_OPEN:
      if (api->state == AS_CONNECTING) {
        const char* auth_token = app_cfg_get_auth_token();
        if (strlen(auth_token) > 0) {
          request_auth(api);
          set_state(api, AS_REQUESTING_AUTH);
        }
        else {
          request_activation_token(api);
          set_state(api, AS_REQUESTING_ACTIVATION_TOKEN);
        }
      }
      break;

    default:
      break;
  }

  snWebsocket_poll(api->ws);
}

static void
request_activation_token(web_api_t* api)
{
  ApiMessage* msg = calloc(1, sizeof(ApiMessage));
  msg->type = ApiMessage_Type_ACTIVATION_TOKEN_REQUEST;
  msg->has_activationTokenRequest = true;
  strcpy(msg->activationTokenRequest.device_id, "asdfasdf");

  send_api_msg(api->ws, msg);

  free(msg);
}

static void
request_auth(web_api_t* api)
{
  ApiMessage* msg = calloc(1, sizeof(ApiMessage));
  msg->type = ApiMessage_Type_AUTH_REQUEST;
  msg->has_authRequest = true;
  uint32_t* devid = (uint32_t*)0x1FFF7A10;
  sprintf(msg->authRequest.device_id, "%08x%08x%08x", devid[0], devid[1], devid[2]);
  sprintf(msg->authRequest.activation_token, "asdfasdf");

  send_api_msg(api->ws, msg);

  free(msg);
}

static void
dispatch_sensor_sample(web_api_t* api, sensor_msg_t* sample)
{
}

static void
dispatch_sensor_timeout(web_api_t* api, sensor_timeout_msg_t* timeout)
{
}

static void
check_for_update(web_api_t* api)
{
  printf("sending update check\r\n");
  ApiMessage* msg = calloc(1, sizeof(ApiMessage));
  msg->type = ApiMessage_Type_FIRMWARE_UPDATE_CHECK_REQUEST;
  msg->has_firmwareUpdateCheckRequest = true;
  sprintf(msg->firmwareUpdateCheckRequest.current_version, "%d.%d.%d", MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION);

  send_api_msg(api->ws, msg);

  free(msg);
}

static void
start_update(web_api_t* api, const char* ver)
{
  printf("sending update start\r\n");
  ApiMessage* msg = calloc(1, sizeof(ApiMessage));
  msg->type = ApiMessage_Type_FIRMWARE_DOWNLOAD_REQUEST;
  msg->has_firmwareDownloadRequest = true;
  strncpy(msg->firmwareDownloadRequest.requested_version, ver,
      sizeof(msg->firmwareDownloadRequest.requested_version));

  send_api_msg(api->ws, msg);

  free(msg);
}

static void
send_api_msg(snWebsocket* ws, ApiMessage* msg)
{
  uint8_t* buffer = malloc(ApiMessage_size);

  pb_ostream_t stream = pb_ostream_from_buffer(buffer, ApiMessage_size);
  bool encoded_ok = pb_encode(&stream, ApiMessage_fields, msg);

  if (encoded_ok)
    snWebsocket_sendBinaryData(ws, stream.bytes_written, (char*)buffer);

  free(buffer);
}

static void
websocket_message_rx(void* userData, snOpcode opcode, const char* data, int numBytes)
{
  web_api_t* api = userData;

  if (opcode != SN_OPCODE_BINARY)
    return;

  ApiMessage* msg = malloc(sizeof(ApiMessage));

  pb_istream_t stream = pb_istream_from_buffer((const uint8_t*)data, numBytes);
  bool status = pb_decode(&stream, ApiMessage_fields, msg);

  if (status)
    dispatch_api_msg(api, msg);

  free(msg);
}

static void
dispatch_api_msg(web_api_t* api, ApiMessage* msg)
{
  switch (msg->type) {
  case ApiMessage_Type_ACTIVATION_TOKEN_RESPONSE:
    printf("got activation token: %s\r\n", msg->activationTokenResponse.activation_token);
    strncpy(api->activation_token,
        msg->activationTokenResponse.activation_token,
        sizeof(api->activation_token));
    set_state(api, AS_AWAITING_ACTIVATION);
    break;

  case ApiMessage_Type_ACTIVATION_NOTIFICATION:
    printf("got auth token: %s\r\n", msg->activationNotification.auth_token);
    app_cfg_set_auth_token(msg->activationNotification.auth_token);
    set_state(api, AS_CONNECTED);
    break;

  case ApiMessage_Type_AUTH_RESPONSE:
    if (msg->authResponse.authenticated) {
      printf("auth succeeded\r\n");
      set_state(api, AS_CONNECTED);
    }
    else {
      printf("auth failed, restarting activation\r\n");
      app_cfg_set_auth_token("");
      request_activation_token(api);
      set_state(api, AS_REQUESTING_ACTIVATION_TOKEN);
    }
    break;

  case ApiMessage_Type_FIRMWARE_UPDATE_CHECK_RESPONSE:
    msg_send(MSG_API_FW_UPDATE_CHECK_RESPONSE, &msg->firmwareUpdateCheckResponse);
    break;

  case ApiMessage_Type_FIRMWARE_DOWNLOAD_RESPONSE:
    msg_send(MSG_API_FW_CHUNK, &msg->firmwareDownloadResponse);
    break;

  default:
    printf("Unsupported API message: %d\r\n", msg->type);
    break;
  }
}

