#include "RCCameraControl.h"

#include <algorithm>

namespace {
constexpr int64_t kReceiveTimeoutMs = 2000;

bool inRange(int value, int nominal)
{
    return value >= nominal - 100 && value <= nominal + 100;
}
}  // namespace

RCCameraControl::Actions RCCameraControl::update(std::span<const int> channels, int64_t nowMs)
{
    if (_lastUpdateMs >= 0 && (nowMs < _lastUpdateMs || nowMs - _lastUpdateMs > kReceiveTimeoutMs)) {
        reset();
    }
    if (channels.size() < 6) {
        reset();
        return {};
    }
    _lastUpdateMs = nowMs;

    Actions actions;
    if (inRange(channels[4], 1500)) {
        _videoReady = true;
    } else {
        actions.toggleVideo = _videoReady && inRange(channels[4], 1900);
        _videoReady = false;
    }

    const int photoValue = channels[5];
    if (inRange(photoValue, 1100)) {
        actions.autoFocus = _photoReady && _photoPress == 1;
        actions.takePhoto = _photoReady && _photoPress == 2;
        _photoPress = 0;
        _photoReady = true;
    } else if (inRange(photoValue, 1500) || inRange(photoValue, 1900)) {
        if (_photoReady) {
            _photoPress = std::max(_photoPress, inRange(photoValue, 1900) ? 2 : 1);
        }
    } else {
        _photoReady = false;
        _photoPress = 0;
    }
    return actions;
}

void RCCameraControl::reset()
{
    _lastUpdateMs = -1;
    _videoReady = false;
    _photoReady = false;
    _photoPress = 0;
}
