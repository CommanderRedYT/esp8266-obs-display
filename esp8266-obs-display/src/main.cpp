// Framework
#include <Arduino.h>
#include <ESP8266WiFi.h>

// local includes
#include "com.h"
#include "globals.h"
#include "lcd.h"
#include "obs.h"
#include "pins.h"

LCD lcd(0x27, 16, 2);

void setup()
{
  // Init
  Serial.begin(9600);
  lcd.init();

  // Pins
  pinMode(pins::PIN_LED, OUTPUT);
  digitalWrite(pins::PIN_LED, LOW);

  // Display
  lcd.backlight();
  lcd.setCursor(0, 0);

  // WiFi
  lcd.print("Connecting...");
  lcd.setCursor(0, 1);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("Connecting to %s\n", WIFI_SSID);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    lcd.print(".");
  }

  // Display IP
  lcd.setCursor(0, 0);
  lcd.printcln("Connected! IP:");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());

  delay(100);
  lcd.clear();

  // Start websocket
  ws::start(WEBSOCKET_HOST, WEBSOCKET_PORT);
}

void loop()
{
  ws::update();
  obs::update();

  if (!globals::connected_to_server)
  {
    display_reconnect_message();
  }
  else
  {
    digitalWrite(pins::PIN_LED, !obs::status.is_muted.value);
    switch (obs::status.captureState.value)
    {
    case obs::capture_state_t::UNKNOWN:
    {
      lcd.setCursor(0, 0);
      lcd.printcln("Unknown");
      break;
    }
    case obs::capture_state_t::IDLE:
    {
      lcd.setCursor(0, 0);
      lcd.printcln("Not recording");
      lcd.setCursor(0, 1);
      lcd.printcln(obs::status.currentScene.value);
    }
    }
  }
}