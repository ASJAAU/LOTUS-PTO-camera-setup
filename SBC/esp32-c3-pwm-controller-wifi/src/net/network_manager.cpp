#include "config.h"
#include "net/network_manager.h"

// Only compile the chosen communication method
#if defined(COMM_ETHERNET)
  #include "net/ethernet_interface.h"
#else
  #include "net/wifi_interface.h"
#endif

NetworkManager networkManager;

NetworkManager::NetworkManager() {
#if defined(COMM_ETHERNET)
  _transport = std::unique_ptr<INetworkTransport>(new EthernetTransport());
#else
  _transport = std::unique_ptr<INetworkTransport>(new WiFiTransport());
#endif
}

void NetworkManager::begin() {
  _transport->begin();
}

void NetworkManager::loop() {
  _transport->maintainConnection();
}

bool NetworkManager::connected() const {
  return _transport->connected();
}

String NetworkManager::localIP() const {
  return _transport->localIP();
}

String NetworkManager::getSSID() const {
  return _transport->networkName();
}

int NetworkManager::rssi() const {
  return _transport->rssi();
}

std::unique_ptr<NetServer> NetworkManager::createServer(uint16_t port) {
  return _transport->createServer(port);
}
