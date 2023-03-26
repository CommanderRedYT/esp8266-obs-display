#pragma once

// system includes
#include <string>
#include <vector>
#include <optional>

namespace obs {

typedef uint8_t percent_t; // 0-100

typedef struct
{
    std::string name;
    percent_t volume;
    bool muted;
} obs_audio_source_t;

enum class capture_state_t : uint8_t
{
    UNKNOWN = 0,
    IDLE,
    RECORDING,
    STREAMING,
    RECORDING_AND_STREAMING,
    RECORDING_PAUSED
};
 
#define REGISTER_STATUS_MEMBER(TYPE, NAME) \
    struct { \
        TYPE value; \
        const char * const name = #NAME; \
    } NAME;

typedef struct
{
    REGISTER_STATUS_MEMBER(std::string, currentScene);
    REGISTER_STATUS_MEMBER(std::vector<obs_audio_source_t>, audioSources);
    REGISTER_STATUS_MEMBER(bool, is_muted);
    REGISTER_STATUS_MEMBER(capture_state_t, captureState);
    REGISTER_STATUS_MEMBER(std::optional<std::string>, recording_time);
    REGISTER_STATUS_MEMBER(std::optional<std::string>, streaming_time);
} obs_status_t;

extern obs_status_t status;

void update();
void request_status();
void handle_response(const std::string& response);
} // namespace obs