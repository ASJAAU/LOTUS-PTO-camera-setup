#include <Arduino.h>
#include "config.h"
#include "wifi_manager.h"
#include "tcp_server.h"
#include "message_handler.h"
#include "pwm_controller.h"

// Import and intialize modules
static WiFiManager wifi;
static PwmController pwm;
static PwmProvider pwmProvider(pwm);
static TcpServer server(Config::SERVER_PORT);
static MessageHandler msgHandler;

void setup() {
  // Begin Serial (For debugging)
  pinMode(8, OUTPUT); //assign led pin
  Serial.begin(115200);
  Serial.println("Starting device");

  // Intialize modules
  pwm.begin(); // Initializes PWM pins

  // start the WIFI connection (This blocks until the device is connected)
  wifi.begin();

  // register providers so MessageHandler can dispatch messages to relevant modules
  msgHandler.addProvider(&pwmProvider);

  // start TCP server
  server.begin();
}

void loop() {
  digitalWrite(Config::ONBOARD_LED_PIN, HIGH); // Turn debug light on

  wifi.loop(); // Check connection and attempt reconnect if disconnected
  
  if (!server.acceptClient()) { // Accept/connect client if necessary
    delay(10);
    Serial.println("No client connected yet");
    return;
  }
 
  if (server.hasPacket()) { // If a framed packet is available, read and dispatch to handler
    Serial.println("Datapacket recieved");
    String payload = server.readPacket();
    WiFiClient& client = server.client();
    msgHandler.handle(payload, client, wifi);
  }

  digitalWrite(Config::ONBOARD_LED_PIN, LOW); // Turn debug light off

  // slow down cpu by 1 ms to reduce load
  delay(1);
}