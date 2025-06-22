#ifndef DEAUTH_H
#define DEAUTH_H

#include <Arduino.h>
#include <esp_wifi.h>
#include <WiFi.h>

void start_deauth(int wifi_number, int attack_type, uint16_t reason);
void stop_deauth();
void deauth_all();
void setRndMac(){ //by deepseek-v3
    esp_wifi_set_max_tx_power(50); //4x
    uint8_t apmac[6];
    /* MAC Generation Process:
   * 1. First byte must have:
   *    - LSB = 0 (unicast)
   *    - Second LSB = 1 (locally administered)
   * 2. Cannot be 00:00:00:00:00:00
   * 3. Should avoid FF:FF:FF:FF:FF:FF
   */
    do {
      for(int i=0; i<6; i++){
        apmac[i] = (uint8_t)esp_random(); // Generate random bytes using hardware RNG
      }
    
      // Enforce MAC specification bits
      apmac[0] &= 0xFE; // Clear multicast bit
      apmac[0] |= 0x02;  // Set local administration bit
  } while(apmac[0] == 0x00); // Prevent all-zero edge case

  // set MAC address
  esp_wifi_set_mac(WIFI_IF_AP, apmac);
}

extern int eliminated_stations;
extern int deauth_type;
extern int deauth_iterations;

#endif