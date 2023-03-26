// Framework
#include <Arduino.h>
#include <ESP8266WiFi.h>

// system includes
#include <optional>

// local includes
#include "com.h"
#include "globals.h"
#include "lcd.h"
#include "obs.h"
#include "pins.h"
#include "led.h"

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
    delay(250);
    lcd.print(".");
  }

  Serial.printf("Connected to %s with IP %s\n", WIFI_SSID, WiFi.localIP().toString().c_str());

  // Display IP
  lcd.setCursor(0, 0);
  lcd.printcln("Connected! IP:");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());

  delay(250);
  lcd.clear();

  // Start websocket
  ws::start(WEBSOCKET_HOST, WEBSOCKET_PORT);
}

void loop()
{
  static std::optional<bool> lastObsRunning = std::nullopt;
  ws::update();
  obs::update();

  if (WiFi.status() != WL_CONNECTED)
  {
    lcd.setCursor(0, 0);
    lcd.printcln("WiFi disconnected");
    return;
  }

  //Serial.printf("OBS running: %d\n", globals::obs_running);

  if (!lastObsRunning || (lastObsRunning && globals::obs_running != *lastObsRunning))
  {
    lastObsRunning = globals::obs_running;
    if (globals::obs_running)
    {
      // turn on led and backlight
      Serial.println("backlight on");
      lcd.backlight();
    }
    else
    {
      // turn off led and backlight
      Serial.println("backlight off");
      lcd.noBacklight();
      lcd.clear();
      digitalWrite(pins::PIN_LED, LOW);
      return;
    }
  }

  if (!globals::connected_to_server)
  {
    display_reconnect_message();
  }
  else if (globals::obs_running)
  {
    led::handleAnimation();
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
      break;
    }
    case obs::capture_state_t::RECORDING:
    {
      lcd.setCursor(0, 0);
      lcd.printcln("Recording");
      lcd.setCursor(0, 1);
      if (obs::status.recording_time.value)
      {
        lcd.printcln(obs::status.recording_time.value.value());
      }
      break;
    }
    case obs::capture_state_t::RECORDING_PAUSED:
    {
      lcd.setCursor(0, 0);
      lcd.printcln("Paused");
      lcd.setCursor(0, 1);
      if (obs::status.recording_time.value)
      {
        lcd.printcln(obs::status.recording_time.value.value());
      }
      break;
    }
    case obs::capture_state_t::STREAMING:
    case obs::capture_state_t::RECORDING_AND_STREAMING:
    {
      lcd.setCursor(0, 0);
      lcd.printcln("Streaming");
      lcd.setCursor(0, 1);
      if (obs::status.streaming_time.value)
      {
        lcd.printcln(obs::status.streaming_time.value.value());
      }
      break;
    }
    }
  }
}