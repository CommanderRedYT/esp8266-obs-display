#pragma once

// 3rdparty lib includes
#include <WebSocketsClient.h>

namespace ws {
extern bool ws_connected;

namespace {
WebSocketsClient client;
} // namespace

void start(const char* host, uint16_t port);
void stop();
void update();

void sendMessage(const char* message);
void wsprintf(const char* format, ...);
void _onEvent(WStype_t type, uint8_t *payload, size_t length);
void onReceive(const char* message);
void onConnect();
void onDisconnect();
} // namespace ws
