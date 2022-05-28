#pragma once

// Framework
#include <Arduino.h>

namespace pins {
// Display
constexpr auto PIN_DISPLAY_SDA = D2; // blue wire
constexpr auto PIN_DISPLAY_SCL = D1; // white wire

// LED
constexpr auto PIN_LED = D8; // red wire
} // namespace pins