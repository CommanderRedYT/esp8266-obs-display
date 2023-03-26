// arduino includes
#include <Arduino.h>

// local includes
#include "pins.h"
#include "obs.h"

namespace led
{

    void handleAnimation()
    {
        // breathing animation for led (use analogWrite)
        static int led_value{0};
        static bool led_up{true};

        if (obs::status.is_muted.value)
        {
            if (led_value > 0)
            {
                led_value -= 10;
                if (led_value <= 0)
                {
                    led_value = 0;
                }
            }
        }
        else
        {
            if (led_up)
            {
                led_value += 5;
                if (led_value >= 255)
                {
                    led_value = 255;
                    led_up = false;
                }
            }
            else
            {
                led_value -= 5;
                if (led_value <= 0)
                {
                    led_value = 0;
                    led_up = true;
                }
            }
        }

        analogWrite(pins::PIN_LED, led_value);
        delay(1);
    }

} // namespace led