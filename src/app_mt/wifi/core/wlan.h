/*****************************************************************************
*
*  wlan.h  - CC3000 Host Driver Implementation.
*  Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*****************************************************************************/
#ifndef __C_WLAN_H__
#define __C_WLAN_H__

#include "cc3000_common.h"
#include "hci_msg.h"

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
  WLAN_SEC_UNSEC,
  WLAN_SEC_WEP,
  WLAN_SEC_WPA,
  WLAN_SEC_WPA2
} wlan_security_t;

typedef enum {
  PATCH_LOAD_DEFAULT   = 0, // Load patches from internal EEPROM
  PATCH_LOAD_FROM_HOST = 1, // Request patches from the host
  PATCH_LOAD_NONE      = 2  // Do not load patches
} patch_load_command_t;

//*****************************************************************************
//
//! \addtogroup wlan_api
//! @{
//
//*****************************************************************************


//*****************************************************************************
//
//!  wlan_start
//!
//!  @param   usPatchesAvailableAtHost -  flag to indicate if patches available
//!                                    from host or from EEPROM. Due to the
//!                                    fact the patches are burn to the EEPROM
//!                                    using the patch programmer utility, the
//!                                    patches will be available from the EEPROM
//!                                    and not from the host.
//!
//!  @return        none
//!
//!  @brief        Start WLAN device. This function asserts the enable pin of
//!                the device (WLAN_EN), starting the HW initialization process.
//!                The function blocked until device Initialization is completed.
//!                Function also configure patches (FW, driver or bootloader)
//!                and calls appropriate device callbacks.
//!
//!  @Note          Prior calling the function wlan_init shall be called.
//!  @Warning       This function must be called after wlan_init and before any
//!                 other wlan API
//!  @sa            wlan_init , wlan_stop
//!
//
//*****************************************************************************
void
c_wlan_start(patch_load_command_t patch_load_cmd);

//*****************************************************************************
//
//!  wlan_stop
//!
//!  @param         none
//!
//!  @return        none
//!
//!  @brief         Stop WLAN device by putting it into reset state.
//!
//!  @sa            wlan_start
//
//*****************************************************************************
extern void c_wlan_stop(void);

//*****************************************************************************
//
//!  wlan_connect
//!
//!  @param    sec_type   security options:
//!               WLAN_SEC_UNSEC,
//!               WLAN_SEC_WEP (ASCII support only),
//!               WLAN_SEC_WPA or WLAN_SEC_WPA2
//!  @param    ssid       up to 32 bytes and is ASCII SSID of the AP
//!  @param    ssid_len   length of the SSID
//!  @param    bssid      6 bytes specified the AP bssid
//!  @param    key        up to 16 bytes specified the AP security key
//!  @param    key_len    key length
//!
//!  @return     On success, zero is returned. On error, negative is returned.
//!              Note that even though a zero is returned on success to trigger
//!              connection operation, it does not mean that CCC3000 is already
//!              connected. An asynchronous "Connected" event is generated when
//!              actual association process finishes and CC3000 is connected to
//!              the AP. If DHCP is set, An asynchronous "DHCP" event is
//!              generated when DHCP process is finish.
//!
//!
//!  @brief      Connect to AP
//!  @warning    Please Note that when connection to AP configured with security
//!              type WEP, please confirm that the key is set as ASCII and not
//!              as HEX.
//!  @sa         wlan_disconnect
//
//*****************************************************************************
extern long c_wlan_connect(
    wlan_security_t ulSecType,
    const char *ssid,
    long ssid_len,
    const uint8_t *bssid,
    const uint8_t *key,
    long key_len);


//*****************************************************************************
//
//!  wlan_disconnect
//!
//!  @return    0 disconnected done, other CC3000 already disconnected
//!
//!  @brief      Disconnect connection from AP.
//!
//!  @sa         wlan_connect
//
//*****************************************************************************
extern long c_wlan_disconnect(void);

//*****************************************************************************
//
//!  wlan_add_profile
//!
//!  @param    ulSecType  WLAN_SEC_UNSEC,WLAN_SEC_WEP,WLAN_SEC_WPA,WLAN_SEC_WPA2
//!  @param    ucSsid    ssid  SSID up to 32 bytes
//!  @param    ulSsidLen ssid length
//!  @param    ucBssid   bssid  6 bytes
//!  @param    ulPriority ulPriority profile priority. Lowest priority:0.
//!  @param    ulPairwiseCipher_Or_TxKeyLen  key length for WEP security
//!  @param    ulGroupCipher_TxKeyIndex  key index
//!  @param    ulKeyMgmt        KEY management
//!  @param    ucPf_OrKey       security key
//!  @param    ulPassPhraseLen  security key length for WPA\WPA2
//!
//!  @return    On success, zero is returned. On error, -1 is returned
//!
//!  @brief     When auto start is enabled, the device connects to
//!             station from the profiles table. Up to 7 profiles are supported.
//!             If several profiles configured the device choose the highest
//!             priority profile, within each priority group, device will choose
//!             profile based on security policy, signal strength, etc
//!             parameters. All the profiles are stored in CC3000 NVMEM.
//!
//!  @sa        wlan_ioctl_del_profile
//
//*****************************************************************************
extern long c_wlan_add_profile(
    wlan_security_t ulSecType,
    uint8_t* ucSsid,
    uint32_t ulSsidLen,
    uint8_t *ucBssid,
    uint32_t ulPriority,
    uint32_t ulPairwiseCipher_Or_Key,
    uint32_t ulGroupCipher_TxKeyLen,
    uint32_t ulKeyMgmt,
    uint8_t* ucPf_OrKey,
    uint32_t ulPassPhraseLen);



//*****************************************************************************
//
//!  wlan_ioctl_del_profile
//!
//!  @param    index   number of profile to delete
//!
//!  @return    On success, zero is returned. On error, -1 is returned
//!
//!  @brief     Delete WLAN profile
//!
//!  @Note      In order to delete all stored profile, set index to 255.
//!
//!  @sa        wlan_add_profile
//
//*****************************************************************************
extern long c_wlan_ioctl_del_profile(uint32_t ulIndex);

//*****************************************************************************
//
//!  wlan_set_event_mask
//!
//!  @param    mask   mask option:
//!       HCI_EVNT_WLAN_UNSOL_CONNECT connect event
//!       HCI_EVNT_WLAN_UNSOL_DISCONNECT disconnect event
//!       HCI_EVNT_WLAN_ASYNC_SIMPLE_CONFIG_DONE  smart config done
//!       HCI_EVNT_WLAN_UNSOL_INIT init done
//!       HCI_EVNT_WLAN_UNSOL_DHCP dhcp event report
//!       HCI_EVNT_WLAN_ASYNC_PING_REPORT ping report
//!       HCI_EVNT_WLAN_KEEPALIVE keepalive
//!       HCI_EVNT_WLAN_TX_COMPLETE - disable information on end of transmission
//!       Saved: no.
//!
//!  @return    On success, zero is returned. On error, -1 is returned
//!
//!  @brief    Mask event according to bit mask. In case that event is
//!            masked (1), the device will not send the masked event to host.
//
//*****************************************************************************
extern long c_wlan_set_event_mask(uint32_t ulMask);

//*****************************************************************************
//
//!  wlan_ioctl_statusget
//!
//!  @param none
//!
//!  @return    WLAN_STATUS_DISCONNECTED, WLAN_STATUS_SCANING,
//!             STATUS_CONNECTING or WLAN_STATUS_CONNECTED
//!
//!  @brief    get wlan status: disconnected, scanning, connecting or connected
//
//*****************************************************************************
extern long c_wlan_ioctl_statusget(void);


//*****************************************************************************
//
//!  wlan_ioctl_set_connection_policy
//!
//!  @param    should_connect_to_open_ap  enable(1), disable(0) connect to any
//!            available AP. This parameter corresponds to the configuration of
//!            item # 3 in the brief description.
//!  @param    should_use_fast_connect enable(1), disable(0). if enabled, tries
//!            to connect to the last connected AP. This parameter corresponds
//!            to the configuration of item # 1 in the brief description.
//!  @param    auto_start enable(1), disable(0) auto connect
//!            after reset and periodically reconnect if needed. This
//!            configuration configures option 2 in the above description.
//!
//!  @return     On success, zero is returned. On error, -1 is returned
//!
//!  @brief      When auto is enabled, the device tries to connect according
//!              the following policy:
//!              1) If fast connect is enabled and last connection is valid,
//!                 the device will try to connect to it without the scanning
//!                 procedure (fast). The last connection will be marked as
//!                 invalid, due to adding/removing profile.
//!              2) If profile exists, the device will try to connect it
//!                 (Up to seven profiles).
//!              3) If fast and profiles are not found, and open mode is
//!                 enabled, the device will try to connect to any AP.
//!              * Note that the policy settings are stored in the CC3000 NVMEM.
//!
//!  @sa         wlan_add_profile , wlan_ioctl_del_profile
//
//*****************************************************************************
extern long c_wlan_ioctl_set_connection_policy(uint32_t should_connect_to_open_ap,
    uint32_t should_use_fast_connect,
    uint32_t ulUseProfiles);

//*****************************************************************************
//
//!  wlan_ioctl_get_scan_results
//!
//!  @param[in]    scan_timeout   parameter not supported
//!  @param[out]   ucResults  scan result (_wlan_full_scan_results_args_t)
//!
//!  @return    On success, zero is returned. On error, -1 is returned
//!
//!  @brief    Gets entry from scan result table.
//!            The scan results are returned one by one, and each entry
//!            represents a single AP found in the area. The following is a
//!            format of the scan result:
//!          - 4 Bytes: number of networks found
//!          - 4 Bytes: The status of the scan: 0 - aged results,
//!                     1 - results valid, 2 - no results
//!          - 42 bytes: Result entry, where the bytes are arranged as  follows:
//!
//!                         - 1 bit isValid - is result valid or not
//!                         - 7 bits rssi - RSSI value;
//!                 - 2 bits: securityMode - security mode of the AP:
//!                           0 - Open, 1 - WEP, 2 WPA, 3 WPA2
//!                         - 6 bits: SSID name length
//!                         - 2 bytes: the time at which the entry has entered into
//!                            scans result table
//!                         - 32 bytes: SSID name
//!                 - 6 bytes:  BSSID
//!
//!  @Note      scan_timeout, is not supported on this version.
//!
//!  @sa        wlan_ioctl_set_scan_params
//
//*****************************************************************************
void
c_wlan_ioctl_get_scan_results(
    uint32_t ulScanTimeout,
    wlan_scan_results_t* results);

//*****************************************************************************
//
//!  wlan_ioctl_set_scan_params
//!
//!  @param    uiEnable - start/stop application scan:
//!            1 = start scan with default interval value of 10 min.
//!            in order to set a different scan interval value apply the value
//!            in milliseconds. minimum 1 second. 0=stop). Wlan reset
//!           (wlan_stop() wlan_start()) is needed when changing scan interval
//!            value. Saved: No
//!  @param   uiMinDwellTime   minimum dwell time value to be used for each
//!           channel, in milliseconds. Saved: yes
//!           Recommended Value: 100 (Default: 20)
//!  @param   uiMaxDwellTime    maximum dwell time value to be used for each
//!           channel, in milliseconds. Saved: yes
//!           Recommended Value: 100 (Default: 30)
//!  @param   uiNumOfProbeRequests  max probe request between dwell time.
//!           Saved: yes. Recommended Value: 5 (Default:2)
//!  @param   uiChannelMask  bitwise, up to 13 channels (0x1fff).
//!           Saved: yes. Default: 0x7ff
//!  @param   uiRSSIThreshold   RSSI threshold. Saved: yes (Default: -80)
//!  @param   uiSNRThreshold    NSR threshold. Saved: yes (Default: 0)
//!  @param   uiDefaultTxPower  probe Tx power. Saved: yes (Default: 205)
//!  @param   aiIntervalList    pointer to array with 16 entries (16 channels)
//!           each entry (uint32_t) holds timeout between periodic scan
//!           (connection scan) - in milliseconds. Saved: yes. Default 2000ms.
//!
//!  @return    On success, zero is returned. On error, -1 is returned
//!
//!  @brief    start and stop scan procedure. Set scan parameters.
//!
//!  @Note     uiDefaultTxPower, is not supported on this version.
//!
//!  @sa        wlan_ioctl_get_scan_results
//
//*****************************************************************************
extern long c_wlan_ioctl_set_scan_params(uint32_t uiEnable, uint32_t uiMinDwellTime,
    uint32_t uiMaxDwellTime, uint32_t uiNumOfProbeRequests,
    uint32_t uiChannelMask, long iRSSIThreshold,
    uint32_t uiSNRThreshold, uint32_t uiDefaultTxPower,
    const uint32_t *aiIntervalList);


//*****************************************************************************
//
//!  wlan_smart_config_start
//!
//!  @param    algoEncryptedFlag indicates whether the information is encrypted
//!
//!  @return   On success, zero is returned. On error, -1 is returned
//!
//!  @brief   Start to acquire device profile. The device acquire its own
//!           profile, if profile message is found. The acquired AP information
//!           is stored in CC3000 EEPROM only in case AES128 encryption is used.
//!           In case AES128 encryption is not used, a profile is created by
//!           CC3000 internally.
//!
//!  @Note    An asynchronous event - Smart Config Done will be generated as soon
//!           as the process finishes successfully.
//!
//!  @sa      wlan_smart_config_set_prefix , wlan_smart_config_stop
//
//*****************************************************************************
extern long c_wlan_smart_config_start(uint32_t algoEncryptedFlag);

//*****************************************************************************
//
//!  wlan_smart_config_stop
//!
//!  @param    algoEncryptedFlag indicates whether the information is encrypted
//!
//!  @return   On success, zero is returned. On error, -1 is returned
//!
//!  @brief   Stop the acquire profile procedure
//!
//!  @sa      wlan_smart_config_start , wlan_smart_config_set_prefix
//
//*****************************************************************************
extern long c_wlan_smart_config_stop(void);

//*****************************************************************************
//
//!  wlan_smart_config_set_prefix
//!
//!  @param   newPrefix  3 bytes identify the SSID prefix for the Smart Config.
//!
//!  @return   On success, zero is returned. On error, -1 is returned
//!
//!  @brief   Configure station ssid prefix. The prefix is used internally
//!           in CC3000. It should always be TTT.
//!
//!  @Note    The prefix is stored in CC3000 NVMEM
//!
//!  @sa      wlan_smart_config_start , wlan_smart_config_stop
//
//*****************************************************************************
extern long c_wlan_smart_config_set_prefix(char* cNewPrefix);

//*****************************************************************************
//
//!  wlan_smart_config_process
//!
//!  @param   none
//!
//!  @return   On success, zero is returned. On error, -1 is returned
//!
//!  @brief   process the acquired data and store it as a profile. The acquired
//!           AP information is stored in CC3000 EEPROM encrypted.
//!           The encrypted data is decrypted and stored as a profile.
//!           behavior is as defined by connection policy.
//
//*****************************************************************************
extern long c_wlan_smart_config_process(void);

//*****************************************************************************
//
//!  mdnsAdvertiser
//!
//!  @param[in] mdnsEnabled         flag to enable/disable the mDNS feature
//!  @param[in] deviceServiceName   Service name as part of the published
//!                                 canonical domain name
//!  @param[in] deviceServiceNameLength   Length of the service name
//!
//!
//!  @return   On success, zero is returned, return SOC_ERROR if socket was not
//!            opened successfully, or if an error occurred.
//!
//!  @brief    Set CC3000 in mDNS advertiser mode in order to advertise itself.
//
//*****************************************************************************
extern int mdns_advertiser(uint16_t mdnsEnabled, char * deviceServiceName, uint16_t deviceServiceNameLength);


//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************



//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef  __cplusplus
}
#endif // __cplusplus

#endif  // __C_WLAN_H__
