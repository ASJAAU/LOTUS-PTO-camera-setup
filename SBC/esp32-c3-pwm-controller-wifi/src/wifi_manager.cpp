#include "wifi_manager.h"
#include "config.h"
#include <Arduino.h>

void WiFiManager::begin() {
  WiFi.useStaticBuffers(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(Config::WIFI_SSID, Config::WIFI_PASS);
  Serial.print("Connecting to WiFi");
  // Try to connect for a short period; if initial connect fails,
  // return and let loop() handle retries so device startup doesn't block.
  unsigned long start = millis();
  const unsigned long INIT_TIMEOUT_MS = 10000;
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < INIT_TIMEOUT_MS) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
  } else {
    Serial.println("WiFi initial connect failed; will retry in loop");
  }
}

void WiFiManager::loop() {
  static unsigned long lastAttempt = 0;
  const unsigned long RECONNECT_INTERVAL_MS = 5000;

  if (WiFi.status() == WL_CONNECTED) return;

  unsigned long now = millis();
  if (now - lastAttempt < RECONNECT_INTERVAL_MS) return;
  lastAttempt = now;

  Serial.println("WiFi disconnected — attempting reconnect");
  WiFi.begin(Config::WIFI_SSID, Config::WIFI_PASS);
}

String WiFiManager::getSSID() const {
  return WiFi.SSID();
}

String WiFiManager::localIP() const {
  return WiFi.localIP().toString();
}

int WiFiManager::rssi() const {
  return WiFi.RSSI();
}

bool WiFiManager::connected() const {
  return WiFi.status() == WL_CONNECTED;
}
