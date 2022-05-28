#include "obs.h"

// 3rdparty lib includes
#include <ArduinoJson.h>

// local includes
#include "com.h"

namespace obs {
obs_status_t status;

void update()
{
    static uint32_t last_time = 0;
    const uint32_t now = millis();
    if (now - last_time > 5000)
    {
        last_time = now;
        request_status();
    }
}

void request_status()
{
    ws::sendMessage("{\"type\":\"fullStatus\"}");
}

void handle_response(const std::string& response)
{
    StaticJsonDocument<315> doc;
    if (const auto err = deserializeJson(doc, response); err)
    {
        Serial.printf("JSON deserialization error: %s\n", err.c_str());
        return;
    }
    else
    {
        Serial.println("JSON deserialized");
    }

    const auto& type = doc["type"].as<std::string>();

    if (type == "fullStatus")
    {
        const auto& is_muted = doc["is_muted"].isNull() ? false : doc["is_muted"].as<bool>();
        const auto& current_scene = doc["current_scene"].isNull() ? "" : doc["current_scene"].as<std::string>();
        const auto& recording_time = doc["recording_time"].isNull() ? "" : doc["recording_time"].as<std::string>();
        const auto& streaming_time = doc["streaming_time"].isNull() ? "" : doc["streaming_time"].as<std::string>();

        status.is_muted.value = is_muted;
        status.currentScene.value = current_scene;
        status.recording_time.value = recording_time;
        status.streaming_time.value = streaming_time;
    } 
    else if (type == "singleStatus")
    {
        // only one variable to update
        const char *key = doc["key"].as<const char *>();
        const auto& value = doc["value"];

        if (strcmp(key, status.is_muted.name.c_str()) == 0)
        {
            status.is_muted.value = value.isNull() ? false : value.as<bool>();
        }
        else if (strcmp(key, status.currentScene.name.c_str()) == 0)
        {
            status.currentScene.value = value.isNull() ? "" : value.as<std::string>();
        }
        else
        {
            Serial.printf("Unsupported key: %s\n", key);
        }
    }
}
} // namespace obs