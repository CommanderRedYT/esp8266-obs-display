#include "lcd.h"

// Framework
#include <Arduino.h>

// local includes
#include "helpers.h"
#include "pins.h"

size_t LCD::printcln(const char *str)
{
    const auto print_str = expand_to_n_chars(str, 16);
    return this->print(print_str.c_str());
}

size_t LCD::printcln(const std::string& str)
{
    return this->printcln(str.c_str());
}

extern LCD lcd;

void display_reconnect_message()
{
    static uint32_t last_time = 0;
    const uint32_t now = millis();
    // make pin_led blink 500ms
    if (now - last_time > 500)
    {
      last_time = now;
      digitalWrite(pins::PIN_LED, !digitalRead(pins::PIN_LED));

      // Display "reconnecting"
      static uint8_t i = 0;

      lcd.setCursor(0, 0);
      lcd.print("Reconnecting");

      if (!i) lcd.print("   ");

      for (uint8_t j = 0; j < i; j++)
        lcd.print(".");
      
      i = (i + 1) % 4;
    }
}