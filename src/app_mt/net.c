
#include "ch.h"
#include "hal.h"
#include "net.h"
#include "wifi/wlan.h"
#include "wifi/nvmem.h"
#include "wifi/socket.h"
#include "wifi/patch.h"
#include "xflash.h"
#include "app_hdr.h"
#include "common.h"
#include "crc/crc32.h"
#include "sxfs.h"
#include "message.h"
#include "netapp.h"
#include "app_cfg.h"

#include <string.h>
#include <stdio.h>


#define SCAN_INTERVAL 1000
#define SERVICE_NAME "brewbit-model-t"

#define PING_SEND_FAST_PERIOD S2ST(30)
#define PING_SEND_SLOW_PERIOD S2ST(1 * 60)
#define PING_RECV_TIMEOUT S2ST(2 * 60)

typedef struct {
  bool valid;
  uint32_t networks_found;
  uint32_t scan_status;
  uint32_t frame_time;
  network_t network;
} net_scan_result_t;


static void
dispatch_net_msg(msg_id_t id, void* msg_data, void* listener_data, void* sub_data);

static void
initialize_and_connect(void);

static void
dispatch_idle(void);

static void
dispatch_dhcp(netapp_dhcp_params_t* dhcp);

static void
dispatch_ping(netapp_pingreport_args_t* ping_report);

static void
dispatch_network_settings(void);

static void
save_or_update_network(network_t* network);

static void
prune_networks(void);

static network_t*
save_network(network_t* net);

static int
enable_scan(bool enable);

static long
perform_scan(void);

static long
get_scan_result(net_scan_result_t* result);

static void
test_connectivity(void);


static net_status_t net_status;
static net_state_t last_net_state;
static network_t networks[16];
static systime_t next_ping_send_time;
static systime_t ping_timeout_time;
static bool wifi_config_applied;


void
net_init()
{
  wlan_init();

  msg_listener_t* l = msg_listener_create("net", 2048, dispatch_net_msg, NULL);
  msg_listener_set_idle_timeout(l, 500);
  msg_subscribe(l, MSG_NET_NETWORK_SETTINGS, NULL);
  msg_subscribe(l, MSG_WLAN_CONNECT, NULL);
  msg_subscribe(l, MSG_WLAN_DISCONNECT, NULL);
  msg_subscribe(l, MSG_WLAN_DHCP, NULL);
  msg_subscribe(l, MSG_WLAN_PING_REPORT, NULL);
}

const net_status_t*
net_get_status()
{
  return &net_status;
}

void
net_scan_start()
{
  memset(networks, 0, sizeof(networks));
  net_status.scan_active = true;
}

void
net_scan_stop()
{
  net_status.scan_active = false;
}

static void
dispatch_net_msg(msg_id_t id, void* msg_data, void* listener_data, void* sub_data)
{
  (void)sub_data;
  (void)listener_data;

  switch (id) {
    case MSG_INIT:
      initialize_and_connect();
      break;

    case MSG_IDLE:
      dispatch_idle();
      break;

    case MSG_WLAN_CONNECT:
      net_status.net_state = NS_WAIT_DHCP;
      msg_send(MSG_NET_STATUS, &net_status);
      next_ping_send_time = chTimeNow();
      break;

    case MSG_WLAN_DISCONNECT:
      if (net_status.net_state == NS_CONNECTING)
        net_status.net_state = NS_CONNECT_FAILED;
      else
        net_status.net_state = NS_DISCONNECTED;
      net_status.dhcp_resolved = false;
      msg_send(MSG_NET_STATUS, &net_status);
      break;

    case MSG_WLAN_DHCP:
      net_status.net_state = NS_CONNECTED;
      dispatch_dhcp(msg_data);
      msg_send(MSG_NET_STATUS, &net_status);
      break;

    case MSG_WLAN_PING_REPORT:
      dispatch_ping(msg_data);
      break;

    case MSG_NET_NETWORK_SETTINGS:
      dispatch_network_settings();
      break;

    default:
      break;
  }
}

static void
dispatch_network_settings()
{
  wifi_config_applied = false;
  net_status.net_state = NS_CONNECT;
}

static void
dispatch_ping(netapp_pingreport_args_t* ping_report)
{
  printf("ping report %u %u %u %u %u\r\n",
      ping_report->packets_sent,
      ping_report->packets_received,
      ping_report->min_round_time,
      ping_report->avg_round_time,
      ping_report->max_round_time);

  if ((ping_report->packets_sent > 0) &&
      (ping_report->packets_received > 0)) {
    systime_t now = chTimeNow();
    ping_timeout_time = now + PING_RECV_TIMEOUT;

    /* ping was successful, we can slow down our poll rate */
    next_ping_send_time = now + PING_SEND_SLOW_PERIOD;
  }
}

static void
dispatch_dhcp(netapp_dhcp_params_t* dhcp)
{
  net_status.dhcp_resolved = (dhcp->status == 0);
  sprintf(net_status.ip_addr, "%d.%d.%d.%d",
      dhcp->ip_addr[3],
      dhcp->ip_addr[2],
      dhcp->ip_addr[1],
      dhcp->ip_addr[0]);
  sprintf(net_status.subnet_mask, "%d.%d.%d.%d",
      dhcp->subnet_mask[3],
      dhcp->subnet_mask[2],
      dhcp->subnet_mask[1],
      dhcp->subnet_mask[0]);
  sprintf(net_status.default_gateway, "%d.%d.%d.%d",
      dhcp->default_gateway[3],
      dhcp->default_gateway[2],
      dhcp->default_gateway[1],
      dhcp->default_gateway[0]);
  sprintf(net_status.dhcp_server, "%d.%d.%d.%d",
      dhcp->dhcp_server[3],
      dhcp->dhcp_server[2],
      dhcp->dhcp_server[1],
      dhcp->dhcp_server[0]);
  sprintf(net_status.dns_server, "%d.%d.%d.%d",
      dhcp->dns_server[3],
      dhcp->dns_server[2],
      dhcp->dns_server[1],
      dhcp->dns_server[0]);
}

static int
enable_scan(bool enable)
{
  static const uint32_t channel_interval_list[16] = {
      2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000,
      2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000
  };

  uint32_t interval = enable ? SCAN_INTERVAL : 0;
  return wlan_ioctl_set_scan_params(interval, 100, 100, 5, 0x1FFF, -80, 0, 205, channel_interval_list);
}

static long
perform_scan()
{
  // enable scanning
  long ret = enable_scan(true);
  if (ret != 0)
    return ret;

  // wait for scan to complete
  chThdSleepMilliseconds(SCAN_INTERVAL);

  // disable scanning
  return enable_scan(false);
}

static long
get_scan_result(net_scan_result_t* result)
{
  wlan_scan_results_t results;

  wlan_ioctl_get_scan_results(0, &results);

  result->networks_found = results.result_count;
  result->scan_status = results.scan_status;
  result->valid = results.valid;
  result->network.rssi = results.rssi;
  result->network.security_mode = results.security_mode;
  int ssid_len = results.ssid_len;
  memcpy(result->network.ssid, results.ssid, ssid_len);
  result->network.ssid[ssid_len] = 0;

  memcpy(result->network.bssid, results.bssid, 6);

  result->network.last_seen = chTimeNow();

  return 0;
}

static network_t*
find_network(char* ssid)
{
  int i;
  for (i = 0; i < 16; ++i) {
    if (strcmp(networks[i].ssid, ssid) == 0)
      return &networks[i];
  }

  return NULL;
}

static void
save_or_update_network(network_t* network)
{
  network_t* saved_network = find_network(network->ssid);

  if (saved_network == NULL) {
    saved_network = save_network(network);
    if (saved_network != NULL)
      msg_send(MSG_NET_NEW_NETWORK, saved_network);
  }
  else {
    *saved_network = *network;
    msg_send(MSG_NET_NETWORK_UPDATED, saved_network);
  }
}

static network_t*
save_network(network_t* net)
{
  int i;
  for (i = 0; i < 16; ++i) {
    if (strcmp(networks[i].ssid, "") == 0) {
      networks[i] = *net;
      return &networks[i];
    }
  }

  return NULL;
}

#define NETWORK_TIMEOUT S2ST(60)

static void
prune_networks()
{
  int i;
  for (i = 0; i < 16; ++i) {
    if ((strcmp(networks[i].ssid, "") != 0) &&
        (chTimeNow() - networks[i].last_seen) > NETWORK_TIMEOUT) {
      msg_send(MSG_NET_NETWORK_TIMEOUT, &networks[i]);
      memset(&networks[i], 0, sizeof(networks[i]));
    }
  }
}

static void
test_connectivity()
{
  systime_t now = chTimeNow();
  if (now > next_ping_send_time) {
    // Assume that the ping will fail and we will have to try again soon
    next_ping_send_time = now + PING_SEND_FAST_PERIOD;

    printf("sending ping\r\n");
    // Ping google's public DNS server
    uint32_t ip = 0x08080808;
    long ret = netapp_ping_send(&ip, 4, 16, 1000);
    if (ret != 0)
      printf("ping failed!\r\n");
  }

  if (now > ping_timeout_time) {
    printf("net connection timed out\r\n");
    initialize_and_connect();
  }
}

static void
initialize_and_connect()
{
  const net_settings_t* ns = app_cfg_get_net_settings();

  net_status.net_state = NS_DISCONNECTED;
  msg_send(MSG_NET_STATUS, &net_status);

  systime_t now = chTimeNow();
  ping_timeout_time = now + PING_RECV_TIMEOUT;
  next_ping_send_time = now + PING_SEND_FAST_PERIOD;

  wlan_stop();

  wlan_start(PATCH_LOAD_DEFAULT);

  {
    nvmem_sp_version_t sp_version;
    nvmem_read_sp_version(&sp_version);
    sprintf(net_status.sp_ver, "%d.%d", sp_version.package_id, sp_version.package_build);
    printf("CC3000 Service Pack Version: %s\r\n", net_status.sp_ver);

    if (sp_version.package_id != 1 || sp_version.package_build != 24) {
      printf("  Not up to date. Applying patch.\r\n");
      wlan_apply_patch();
      printf("  Update complete\r\n");

      nvmem_read_sp_version(&sp_version);
      sprintf(net_status.sp_ver, "%d.%d", sp_version.package_id, sp_version.package_build);
      printf("Updated CC3000 Service Pack Version: %s\r\n", net_status.sp_ver);
    }
  }

  if (!wifi_config_applied) {
    wlan_ioctl_set_connection_policy(0, 0, 0);

    uint32_t dhcp_timeout = 14400;
    uint32_t arp_timeout = 3600;
    uint32_t keepalive = 10;
    uint32_t inactivity_timeout = 0;
    netapp_timeout_values(&dhcp_timeout, &arp_timeout, &keepalive, &inactivity_timeout);

    netapp_dhcp(&ns->ip, &ns->subnet_mask, &ns->gateway, &ns->dns_server);

    wlan_stop();

    wlan_start(PATCH_LOAD_DEFAULT);

    wifi_config_applied = true;
  }

  {
    uint8_t mac[6];
    nvmem_get_mac_address(mac);
    sprintf(net_status.mac_addr, "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  }

  if (strlen(ns->ssid) > 0) {
    net_status.net_state = NS_CONNECTING;
    msg_send(MSG_NET_STATUS, &net_status);

    wlan_disconnect();

    chThdSleepMilliseconds(100);

    wlan_connect(ns->security_mode,
        ns->ssid,
        strlen(ns->ssid),
        NULL,
        (const uint8_t*)ns->passphrase,
        strlen(ns->passphrase));
  }
}

static void
dispatch_idle()
{
  if (net_status.scan_active) {
    if (perform_scan() == 0) {
      net_scan_result_t result;
      do {
        if (get_scan_result(&result) != 0)
          break;

        if (result.scan_status == 1 && result.valid)
          save_or_update_network(&result.network);
      } while (result.networks_found > 1);

      prune_networks();
    }
  }
  else {
    switch (net_status.net_state) {
      case NS_CONNECT_FAILED:
      case NS_DISCONNECTED:
      case NS_CONNECT:
        initialize_and_connect();
        break;

      case NS_WAIT_DHCP:
      case NS_CONNECTED:
      case NS_CONNECTING:
      default:
        break;
    }

    if (net_status.net_state != last_net_state) {
      msg_send(MSG_NET_STATUS, &net_status);
      last_net_state = net_status.net_state;
    }
  }

  test_connectivity();
}
