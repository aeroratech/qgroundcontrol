#pragma once

#include "UnitTest.h"

class RCCameraControlTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _videoOnlyOnPress();
    void _shortPressFocusesOnRelease();
    void _longPressOnlyTakesPhoto();
    void _heldAtStartupIsIgnored();
    void _invalidChannelsDiscardPress();
    void _timeoutDiscardsPress();
    void _resetRequiresRelease();
    void _channelTolerance();
};
