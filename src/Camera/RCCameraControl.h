#pragma once

#include <cstdint>
#include <span>

/// Decodes the custom remote's CH5 video and CH6 focus/photo buttons.
class RCCameraControl
{
public:
    struct Actions
    {
        bool toggleVideo = false;
        bool autoFocus = false;
        bool takePhoto = false;
    };

    Actions update(std::span<const int> channels, int64_t nowMs);
    void reset();

private:
    int64_t _lastUpdateMs = -1;
    bool _videoReady = false;
    bool _photoReady = false;
    int _photoPress = 0;
};
