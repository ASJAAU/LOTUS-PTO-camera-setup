
#include <ArduinoJson.h>
#include "net/message_handler.h"
#include "tcp/tcp_server.h"

MessageHandler::MessageHandler()
{
}

void MessageHandler::addProvider(ResourceProvider *provider)
{
    _providers.push_back(provider);
}

// Serialise `doc` to JSON and send it as a length-prefixed packet via `client`.
// Uses NetClient directly rather than duplicating TcpServer's framing logic.
static void sendJsonReply(NetClient &client, const JsonDocument &doc)
{
    String output;
    serializeJson(doc, output);

    uint32_t length = output.length();
    uint8_t header[4] = {
        (uint8_t)((length >> 24) & 0xFF),
        (uint8_t)((length >> 16) & 0xFF),
        (uint8_t)((length >>  8) & 0xFF),
        (uint8_t)( length        & 0xFF)
    };
    client.write(header, 4);
    client.print(output);
}

void MessageHandler::handle(const String &payload, NetClient &client, NetworkManager &network)
{
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    JsonDocument reply;

    // Handle corrupt json payloads
    if (err)
    {
        reply["type"]    = "response";
        reply["success"] = false;
        reply["error"]   = "invalid JSON payload";
        sendJsonReply(client, reply);
        return;
    }

    const char *type = doc["type"] | "";

    // ######### HEARTBEAT MESSAGE #########
    if (strcmp(type, "ping") == 0)
    {
        reply["type"]    = "pong";
        reply["success"] = true;
        sendJsonReply(client, reply);
        return;
    }

    // ######### STATUS MESSAGE #########
    if (strcmp(type, "status") == 0)
    {
        reply["type"]    = "status";
        reply["success"] = true;
        reply["ssid"]    = network.getSSID();   // SSID for WiFi; "Ethernet" for wired
        reply["ip"]      = network.localIP();
        reply["rssi"]    = network.rssi();       // 0 for Ethernet
        sendJsonReply(client, reply);
        return;
    }

    // ##############################
    // ##   ACTIONABLE REQUESTS    ##
    // ##############################

    // ######### Setters #########
    if (strcmp(type, "set") == 0)
    {/// Handle setter functions for all modules/sensors
        reply["type"] = "response";
        JsonVariant setVar = doc["set"];

        if (!setVar.is<JsonObject>())
        {// Return an error if the payload is improperly formatted
            reply["success"] = false;
            reply["error"]   = "malformed set payload; expected object";
            sendJsonReply(client, reply);
            return;
        }

        // Assume correct formatting and attempt to handle the key-value pairs
        JsonObject obj = setVar.as<JsonObject>();
        for (JsonPair kv : obj)
        {// Process each key-value pair
            const char *key   = kv.key().c_str();
            JsonVariant value = kv.value();

            bool handled = false;
            for (ResourceProvider *p : _providers)
            {
                if (p->matchesKey(key))
                {
                    handled = p->handleSet(key, value, reply);
                    break;
                }
            }

            if (!handled)
            {
                reply["success"] = false;
                reply["error"]   = "unknown set key";
                reply["value"]   = key;
            }
            // Returns a reply for each key-value pair provided
            sendJsonReply(client, reply);
        }
        return;
    }

    // ######### Getters #########
    if (strcmp(type, "get") == 0)
    {/// Handle getter functions for all modules/sensors
        reply["type"] = "response";
        JsonVariant getVar = doc["get"];

        if (!getVar.is<JsonArray>())
        {// Return an error if the payload is improperly formatted
            reply["success"] = false;
            reply["error"]   = "malformed get payload; expected array";
            sendJsonReply(client, reply);
            return;
        }

        // Assume correct formatting and attempt to handle the key-value pairs
        JsonArray arr = getVar.as<JsonArray>();
        for (JsonVariant v : arr)
        {// Process each key
            const char *key  = v.as<const char *>();
            bool handled = false;
            for (ResourceProvider *p : _providers)
            {
                if (p->matchesKey(key))
                {
                    handled = p->handleGet(key, reply);
                    break;
                }
            }

            if (!handled)
            {
                reply["success"] = false;
                reply["error"]   = "unknown get key";
                reply["value"]   = key;
            }
            // Returns a reply for each key provided
            sendJsonReply(client, reply);
        }
        return;
    }

    // ######### Commands #########
    if (strcmp(type, "cmd") == 0)
    {/// Handle command functions for all modules/sensors
        reply["type"]    = "response";
        const char *cmd  = doc["cmd"] | "";
        JsonVariant params = doc["params"];

        bool handled = false;
        for (ResourceProvider *p : _providers)
        {
            // No break — several modules may share a command name
            // (i.e. compounded module commands)
            if (p->handleCmd(cmd, params, reply)) { handled = true; }
        }

        if (!handled)
        {
            reply["success"] = false;
            reply["error"]   = "unknown cmd";
            reply["value"]   = cmd;
        }

        sendJsonReply(client, reply);
        return;
    }

    // Handle correctly formed but unknown message types
    reply["type"]    = "response";
    reply["success"] = false;
    reply["error"]   = "unknown message type";
    reply["value"]   = type;
    sendJsonReply(client, reply);
}
