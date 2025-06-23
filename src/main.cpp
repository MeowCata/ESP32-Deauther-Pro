/**
 * @file main.cpp 
 * @brief Main program for ESP32 WiFi deauther
 *
 * Handles device initialization, button monitoring, and main operation loop
 */

#include <WiFi.h>
#include <esp_wifi.h>
#include "types.h"
#include "web_interface.h"
#include "web_interface.cpp"
#include "deauth.h"
#include "definitions.h"
#include "esp_task_wdt.h"

int currentChannel = 1;  // Tracks current WiFi channel for scanning
bool wasButtonPressed = false;  // Tracks button press state

/**
 * @brief Blink LED once with minimal delay
 */
void blinkOnce() {
  constexpr uint32_t blinkDuration = 50; // ms
  digitalWrite(ledPin, LOW);   // LED on
  delay(blinkDuration);                  
  digitalWrite(ledPin, HIGH);  // LED off
}
void stopAP() {
  WiFi.disconnect();
}

void setup() {
#ifdef SERIAL_DEBUG
  Serial.begin(115200);
#endif
#ifdef LED
  pinMode(LED, OUTPUT);
#endif

  // Initialize WiFi in AP mode with random MAC
  WiFi.mode(WIFI_MODE_AP);
  setRndMac();
  esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
  WiFi.softAP(AP_SSID, AP_PASS, 1, true, 2); // Hidden SSID, max 2 connections
  
  DEBUG_PRINTLN("[INIT] AP started");
  start_web_interface();
  delay(50); // Brief delay for web server initialization
  DEBUG_PRINTLN("[INIT] Web service started");
  DEBUG_PRINT("[INIT] AP MAC: ");
  DEBUG_PRINTLN(WiFi.softAPmacAddress());

  // Initialize hardware
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH); // LED off
  pinMode(buttonPin, INPUT_PULLUP);
  delay(50); // Debounce delay
  blinkOnce(); // Visual confirmation of boot
}

void loop() {
  bool isButtonPressed = (digitalRead(buttonPin) == LOW); // Active-low button
  
  // Detect button press edge
  if (isButtonPressed && !wasButtonPressed) {
    deauth_type = DEAUTH_TYPE_ALL;
    blinkOnce();
    DEBUG_PRINTLN("[MODE] Switching to deauth mode (WebUI disabled)");
    handle_deauth_all();
  }
  wasButtonPressed = isButtonPressed;

  if (deauth_type == DEAUTH_TYPE_ALL) {
    // Cycle through channels
    if (currentChannel > CHANNEL_MAX) currentChannel = 1;
    esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
    deauth_all();  // Scan and deauth on current channel
    currentChannel++;
    delay(10); // Minimal delay for channel switching
  } else {
    web_interface_handle_client(); // Normal web interface mode
  }
}
