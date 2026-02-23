#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

class WiFiManager {
public:
  void begin();
  void loop();
  String getSSID() const;
  String localIP() const;
  int rssi() const;
  bool connected() const;
};

#endif // WIFI_MANAGER_H
