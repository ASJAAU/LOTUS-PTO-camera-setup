#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include "pwm_controller.h"
#include "wifi_manager.h"
#include "resource_provider.h"

class MessageHandler {
public:
  MessageHandler();
  void addProvider(ResourceProvider* provider);
  void handle(const String& payload, WiFiClient& client, WiFiManager& wifi);

private:
  std::vector<ResourceProvider*> _providers;
};

#endif // MESSAGE_HANDLER_H
