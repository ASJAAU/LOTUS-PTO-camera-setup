#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <Arduino.h>
#include <vector>
#include "net/network_client.h"
#include "net/network_manager.h"
#include "resources/resource_provider.h"

class MessageHandler {
public:
  MessageHandler();
  void addProvider(ResourceProvider* provider);

  // `client`  — the abstract socket to reply on (WiFi or Ethernet)
  // `network` — provides status info (IP, SSID, RSSI) for "status" messages
  void handle(const String& payload, NetClient& client, NetworkManager& network);

private:
  std::vector<ResourceProvider*> _providers;
};

#endif // MESSAGE_HANDLER_H
