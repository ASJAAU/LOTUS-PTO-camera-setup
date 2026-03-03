#include <Arduino.h>
#include "config.h"
#include "net/network_manager.h"
#include "net/network_server.h"
#include "net/message_handler.h"
#include "resources/pwm_controller.h"

// Import and intialize modules
// Global network manager (defined in network_manager.cpp)
static PwmController pwm;
static PwmProvider pwmProvider(pwm);
static MessageHandler msgHandler;
// NetServer is created via the NetworkManager at runtime so it can pick the
// correct transport (WiFi / Ethernet) after the refactor.
static std::unique_ptr<NetServer> server;

void setup() {
  // Begin Serial (For debugging)
  pinMode(8, OUTPUT); //assign led pin
  Serial.begin(115200);
  Serial.println("Starting device");

  // Intialize modules
  pwm.begin(); // Initializes PWM pins

  // start network transport(s)
  networkManager.begin();

  // create a server instance bound to the configured port
  server = networkManager.createServer(Config::SERVER_PORT);
  if (server) server->begin();

  // register providers so MessageHandler can dispatch messages to relevant modules
  msgHandler.addProvider(&pwmProvider);

  // start TCP server
  server->begin();
}

void loop() {
  networkManager.loop(); // maintain network state

  if (!server) {
    // attempt to recreate server if transport changed
    server = networkManager.createServer(Config::SERVER_PORT);
    if (server) server->begin();
    delay(10);
    Serial.println("Server not available yet");
    return;
  }

  // Accept an incoming client (returns nullptr if none waiting)
  std::unique_ptr<NetClient> client = server->available();
  if (!client) {
    delay(10);
    return;
  }

  // Read all available bytes into a payload string and dispatch
  if (client->available() > 0) {
    digitalWrite(Config::ONBOARD_LED_PIN, HIGH); // Turn debug light on
    String payload;
    while (client->available() > 0) {
      int c = client->read();
      if (c < 0) break;
      payload += (char)c;
    }
    Serial.println("Datapacket received");
    msgHandler.handle(payload, *client, networkManager);
  }
  
  digitalWrite(Config::ONBOARD_LED_PIN, LOW); // Turn debug light off}
    
  // Close client when done
  client->stop();

  // slow down cpu by 1 ms to reduce load
  delay(1);
}