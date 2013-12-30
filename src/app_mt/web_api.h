
#ifndef WEB_API_H
#define WEB_API_H

typedef enum {
  AS_AWAITING_NET_CONNECTION,
  AS_CONNECTING,
  AS_REQUESTING_AUTH,
  AS_REQUESTING_ACTIVATION_TOKEN,
  AS_AWAITING_ACTIVATION,
  AS_CONNECTED
} api_state_t;

typedef struct {
  api_state_t state;
} api_status_t;


void
web_api_init(void);

api_state_t
web_api_get_status(void);

#endif
