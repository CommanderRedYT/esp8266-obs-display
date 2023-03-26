#include "obs.h"

// 3rdparty lib includes
#include <ArduinoJson.h>

// local includes
#include "com.h"
#include "globals.h"

namespace obs {
obs_status_t status;

void update()
{
    static uint32_t last_time = 0;
    const uint32_t now = millis();
    if (now - last_time > 10000)
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

    const auto& type = doc["type"].as<std::string>();

    if (type == "fullStatus")
    {
        const auto& is_muted = doc["is_muted"].isNull() ? false : doc["is_muted"].as<bool>();
        const auto& current_scene = doc["currentScene"].isNull() ? "" : doc["currentScene"].as<std::string>();
        const auto& recording_time = doc["recording_time"].isNull() ? "" : doc["recording_time"].as<std::string>();
        const auto& streaming_time = doc["streaming_time"].isNull() ? "" : doc["streaming_time"].as<std::string>();
        const auto& audio_sources = doc["audioSources"].as<JsonArray>();
        const auto& capture_state = doc["captureState"].as<uint8_t>();
        globals::obs_running = doc["connected"].as<bool>();

        status.is_muted.value = is_muted;
        status.currentScene.value = current_scene;
        status.recording_time.value = recording_time;
        status.streaming_time.value = streaming_time;

        status.audioSources.value.clear();
        status.audioSources.value.reserve(audio_sources.size());
        for (const auto& audio_source : audio_sources)
        {
            const auto& name = audio_source["name"].as<std::string>();
            const auto& volume = audio_source["volume"].as<percent_t>();
            const auto& muted = audio_source["muted"].as<bool>();

            status.audioSources.value.push_back({name, volume, muted});
        }

        status.captureState.value = static_cast<capture_state_t>(capture_state);

        // Serial.println("Full status updated");
    } 
    else if (type == "singleStatus")
    {
        // only one variable to update
        const std::string key = doc["key"].as<std::string>();
        const auto& value = doc["value"];

        Serial.printf("Updating %s\n", key.c_str());

        if (key == status.is_muted.name)
        {
            status.is_muted.value = value.isNull() ? false : value.as<bool>();
        }
        else if (key == status.currentScene.name)
        {
            status.currentScene.value = value.isNull() ? "" : value.as<std::string>();
        }
        else if (key == status.recording_time.name)
        {
            status.recording_time.value = value.isNull() ? "" : value.as<std::string>();
        }
        else if (key == status.streaming_time.name)
        {
            status.streaming_time.value = value.isNull() ? "" : value.as<std::string>();
        }
        else if (key == status.captureState.name)
        {
            status.captureState.value = static_cast<capture_state_t>(value.as<uint8_t>());
        }
        else if (key == status.audioSources.name)
        {
            status.audioSources.value.clear();
            status.audioSources.value.reserve(value.size());
            for (const auto& audio_source : value.as<JsonArray>())
            {
                const auto& name = audio_source["name"].as<std::string>();
                const auto& volume = audio_source["volume"].as<percent_t>();
                const auto& muted = audio_source["muted"].as<bool>();

                status.audioSources.value.push_back({name, volume, muted});
            }
        }
        else
        {
            Serial.printf("Unsupported key: %s\n", key.c_str());
        }
    }
}
} // namespace obs