#include "globals.h"

namespace globals {
bool connected_to_server{false};
bool handle_connected{false};
bool obs_running{false};
uint32_t reconnecting_time{0};
} // namespace globals