#include "com.h"

// local includes
#include "globals.h"
#include "obs.h"

namespace ws {
bool ws_connected;

void start(const char* host, uint16_t port)
{
    client.begin(host, port, "/");
    client.onEvent(_onEvent);
}

void stop()
{
    client.disconnect();
}

void update()
{
    client.loop();
}

void sendMessage(const char* message)
{
    if (!ws_connected)
        return;
    
    // Serial.printf("TX: %s\n", message);
    client.sendTXT(message);
}

void wsprintf(const char* format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    sendMessage(buffer);
}

void _onEvent(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_t::WStype_CONNECTED:
        onConnect();
        break;
    case WStype_t::WStype_DISCONNECTED:
        onDisconnect();
        break;
    case WStype_t::WStype_TEXT:
        onReceive((const char*)payload);
        break;
    case WStype_t::WStype_ERROR:
        Serial.printf("WS-Error: %.*s\n", length, payload);
        break;
    default:
        break;
    }
}

void onReceive(const char* message)
{
    // Serial.printf("RX: %s\n", message);
    obs::handle_response(std::string(message));
}

void onConnect()
{
    globals::connected_to_server = true;
    ws_connected = true;
}

void onDisconnect()
{
    globals::connected_to_server = false;
    ws_connected = false;
}
} // namespace ws