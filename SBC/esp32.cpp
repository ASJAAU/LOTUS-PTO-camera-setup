#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoJson.h>

// Define network configuration
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 50);
EthernetServer server(5000);
EthernetClient client;

// Define GPIO pins
const int pwm_frequency = 10000; // Frequency is specified in Hz
const int pwm_resolution = 8; // PWM bit resolution
const int pwm01 = 5;
const int pwm02 = 10;
const int pwm03 = 11;

// Define log_level
constant int log_level = 0; // 0: None, 1: Error, 2: Warning, 3: Info, 4: Debug/Verbose

void setup() {
  // Setup ethernet connection
  Serial.begin(115200);
  Ethernet.begin(mac, ip);
  server.begin();

  // Setup pins
  /// PWM
  ledcAttach(pwm01, pwm_frequency, pwn_resolution);
  ledcAttach(pwm02, pwm_frequency, pwm_resolution);
  ledcAttach(pwm03, pwm_frequency, pwm_resolution);

  // initialize pin values
  /// Set initial pwm duty cycle to off (i.e. 0)
  ledcWrite(pwm01, 0);
  ledcWrite(pwm02, 0);
  ledcWrite(pwm03, 0);
}

void loop() {
  if (!client || !client.connected()) {
    client = server.available();
    return;
  }

  if (client.available() >= 4) {
    uint32_t length = readLength();
    String jsonString = readPayload(length);

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, jsonString);

    if (!error) {
      handleMessage(doc);
    }
    else {
      // Handle JSON deserialization error
      StaticJsonDocument<256> reply;
      reply["type"] = "response";
      reply["success"] = false;
      reply["error"] = "invalid JSON payload";
      sendJson(reply);
    }
  }
}

uint32_t readLength() {
  uint8_t header[4];
  client.read(header, 4);
  return (header[0] << 24) |
         (header[1] << 16) |
         (header[2] << 8)  |
         (header[3]);
}

String readPayload(uint32_t length) {
  String payload = "";
  while (payload.length() < length) {
    if (client.available()) {
      payload += (char)client.read();
    }
  }
  return payload;
}

void sendJson(JsonDocument& doc) {
  String output;
  serializeJson(doc, output);

  uint32_t length = output.length();
  uint8_t header[4] = {
    (length >> 24) & 0xFF,
    (length >> 16) & 0xFF,
    (length >> 8) & 0xFF,
    length & 0xFF
  };

  client.write(header, 4);
  client.print(output);
}

void handleMessage(JsonDocument& doc) {
  const char* type = doc["type"] | "";

  // common reply object
  StaticJsonDocument<256> reply;

  // heartbeat message (ping/pong for connectivity check)
  if (strcmp(type, "ping") == 0) {
    reply["type"] = "pong";
    reply["success"] = true;
    sendJson(reply);
    return;
  }

  // Setter message (set named values) 
  if (strcmp(type, "set") == 0) {
    reply["type"] = "response";
    JsonVariant setVar = doc["set"];

    if (!setVar.is<JsonObject>()) {
      reply["success"] = false;
      reply["error"] = "malformed set payload; expected object";
      sendJson(reply);
      return;
    }

    // nested json object containing key-value pairs to set
    JsonObject setObj = setVar.as<JsonObject>();

    // Iterate over each key-value pair in the "set" object and handle accordingly
    for (JsonPair kv : setObj) {
      const char* key = kv.key().c_str();
      JsonVariant value = kv.value();

      // handle pwm.channel key to address specific channel
      if (strncmp(key, "pwm.", 4) == 0) {
        int ch = atoi(key + 4);
        if (ch >= 1 && ch <= 3 && value.is<int>()) {
          int v = value.as<int>();
          if (v < 0) v = 0;
          if (v > 255) v = 255;
          setPWM((uint8_t)ch, (uint8_t)v);
          reply["success"] = true;
          // include which channel was set
          reply["result"] = String("pwm.") + String(ch) + " set";
        } else {
          reply["success"] = false;
          reply["error"] = "invalid pwm channel or value";
          reply["value"] = key;
        }
      }
      // If no PWM channel specified default to all channels
      else if (strcmp(key, "pwm") == 0 && value.is<int>()) {
        // Bound value
        int v = value.as<int>();
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        // Set all channels
        setPWM(1, (uint8_t)v);
        setPWM(2, (uint8_t)v);
        setPWM(3, (uint8_t)v);
        reply["success"] = true;
        reply["result"] = "all pwm channels set";
      }
      else {
        reply["success"] = false;
        reply["error"] = "unknown set key";
        reply["value"] = key;
      }
    }

    sendJson(reply);
    return;
  }

  // Getter message (retrieve named values)
  if (strcmp(type, "get") == 0) {
    reply["type"] = "response";
    const char* key = doc["get"] | "";
    
    // handle empty key case
    if (strlen(key) == 0) {
      reply["success"] = false;
      reply["error"] = "malformed get payload; expected string key";
      sendJson(reply);
      return;
    }

    // handle known keys; support "pwm.1" to get channel 1
    if (strncmp(key, "pwm.", 4) == 0) {
      int ch = atoi(key + 4); // Move pointer past "pwm." and convert to int ("pwm.1" -> 1)
      if (ch == 1) {
          ledcRead(pwm01);
          reply["success"] = true;
          reply["value"] = ledcRead(pwm01);
}
      else if (ch == 2) {
          ledcRead(pwm02);
          reply["success"] = true;
          reply["value"] = ledcRead(pwm02);
      }
      else if (ch == 3) {
          ledcRead(pwm03);
          reply["success"] = true;
          reply["value"] = ledcRead(pwm03);
      } 
      else {
        reply["success"] = false;
        reply["error"] = "unknown get key";
        reply["value"] = key;
      }
    }
    else if (strcmp(key, "pwm") == 0) {
      // return aggregate or default PWM (return channel 1 for legacy behavior)
      reply["success"] = true;
      reply["value"] = pwm_state[1];
    }
    else {
      reply["success"] = false;
      reply["error"] = "unknown get key";
      reply["value"] = key;
    }

    sendJson(reply);
    return;
  }

  // command message (execute a named command)
  if (strcmp(type, "cmd") == 0) {
    reply["type"] = "response";
    const char* cmd = doc["cmd"] | "";

    // handle known commands
    if (strcmp(cmd, "pwmTest") == 0) {
      pwmTest();
      reply["success"] = true;
      reply["result"] = "pwmTest started";
    }
    else if (strcmp(cmd, "pwmOff") == 0) {
      pwmOff();
      reply["success"] = true;
      reply["result"] = "all pwm off";
    } 
    else {
      reply["success"] = false;
      reply["error"] = "unknown cmd";
      reply["value"] = cmd;
    }

    sendJson(reply);
    return;
  }

  // Unknown type
  reply["type"] = "response";
  reply["success"] = false;
  reply["error"] = "unknown message type";
  reply["value"] = type;
  sendJson(reply);

}

void setPWM(uint8_t pin, uint8_t dutyCycle) {
  if (pin == 1) {ledcWrite(pwm01, dutyCycle);}
  else if (pin == 2) {ledcWrite(pwm02, dutyCycle);}
  else if (pin == 3) {ledcWrite(pwm03, dutyCycle);}
}

void pwmOff() {
  ledcWrite(pwm01, 0);
  ledcWrite(pwm02, 0);
  ledcWrite(pwm03, 0);
}

void pwmTest() {
  for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle+10) {
    setPWM(1, dutyCycle);
    setPWM(2, dutyCycle);
    setPWM(3, dutyCycle);
    delay(10);
  }
}