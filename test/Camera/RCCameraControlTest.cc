#include "RCCameraControlTest.h"

#include <array>
#include <vector>

#include "RCCameraControl.h"

namespace {
std::array<int, 6> channels(int video = 1500, int photo = 1100)
{
    return {1500, 1500, 1100, 1500, video, photo};
}

bool noAction(const RCCameraControl::Actions& actions)
{
    return !actions.toggleVideo && !actions.autoFocus && !actions.takePhoto;
}
}  // namespace

void RCCameraControlTest::_videoOnlyOnPress()
{
    RCCameraControl control;
    QVERIFY(noAction(control.update(channels(), 0)));
    auto action = control.update(channels(1900), 100);
    QVERIFY(action.toggleVideo);
    QVERIFY(!action.autoFocus && !action.takePhoto);
    QVERIFY(noAction(control.update(channels(1900), 200)));
    QVERIFY(noAction(control.update(channels(), 300)));
    QVERIFY(control.update(channels(1900), 400).toggleVideo);
}

void RCCameraControlTest::_shortPressFocusesOnRelease()
{
    RCCameraControl control;
    control.update(channels(), 0);
    QVERIFY(noAction(control.update(channels(1500, 1500), 100)));
    QVERIFY(noAction(control.update(channels(1500, 1500), 200)));
    const auto action = control.update(channels(), 300);
    QVERIFY(action.autoFocus);
    QVERIFY(!action.toggleVideo && !action.takePhoto);
    QVERIFY(noAction(control.update(channels(), 400)));
}

void RCCameraControlTest::_longPressOnlyTakesPhoto()
{
    RCCameraControl control;
    control.update(channels(), 0);
    QVERIFY(noAction(control.update(channels(1500, 1500), 100)));
    QVERIFY(noAction(control.update(channels(1500, 1900), 200)));
    QVERIFY(noAction(control.update(channels(1500, 1900), 300)));
    QVERIFY(noAction(control.update(channels(1500, 1500), 400)));
    const auto action = control.update(channels(), 500);
    QVERIFY(action.takePhoto);
    QVERIFY(!action.toggleVideo && !action.autoFocus);
    QVERIFY(noAction(control.update(channels(), 600)));
    control.update(channels(1500, 1900), 700);
    QVERIFY(control.update(channels(), 800).takePhoto);
}

void RCCameraControlTest::_heldAtStartupIsIgnored()
{
    RCCameraControl control;
    QVERIFY(noAction(control.update(channels(1900, 1900), 0)));
    QVERIFY(noAction(control.update(channels(1900, 1500), 100)));
    QVERIFY(noAction(control.update(channels(), 200)));
    QVERIFY(control.update(channels(1900, 1900), 300).toggleVideo);
    QVERIFY(control.update(channels(), 400).takePhoto);
}

void RCCameraControlTest::_invalidChannelsDiscardPress()
{
    for (const auto& invalid :
         {std::vector<int>{}, std::vector<int>{1500, 1500, 1100, 1500, 1500},
          std::vector<int>{1500, 1500, 1100, 1500, 65535, 65535}, std::vector<int>{1500, 1500, 1100, 1500, 0, 0},
          std::vector<int>{1500, 1500, 1100, 1500, 1700, 1300}}) {
        RCCameraControl control;
        control.update(channels(), 0);
        control.update(channels(1900, 1900), 100);
        QVERIFY(noAction(control.update(invalid, 200)));
        QVERIFY(noAction(control.update(channels(1900, 1500), 300)));
        QVERIFY(noAction(control.update(channels(), 400)));
        QVERIFY(control.update(channels(1900, 1900), 500).toggleVideo);
        QVERIFY(control.update(channels(), 600).takePhoto);
    }
}

void RCCameraControlTest::_timeoutDiscardsPress()
{
    RCCameraControl control;
    control.update(channels(), 0);
    control.update(channels(1500, 1900), 100);
    QVERIFY(noAction(control.update(channels(1900, 1500), 2201)));
    QVERIFY(noAction(control.update(channels(), 2300)));
    QVERIFY(control.update(channels(1900, 1500), 2400).toggleVideo);
    QVERIFY(control.update(channels(), 2500).autoFocus);
}

void RCCameraControlTest::_resetRequiresRelease()
{
    RCCameraControl control;
    control.update(channels(), 0);
    control.update(channels(1900, 1900), 100);
    control.reset();
    QVERIFY(noAction(control.update(channels(1900, 1500), 200)));
    QVERIFY(noAction(control.update(channels(), 300)));
    QVERIFY(control.update(channels(1900, 1500), 400).toggleVideo);
    QVERIFY(control.update(channels(), 500).autoFocus);
}

void RCCameraControlTest::_channelTolerance()
{
    RCCameraControl control;
    control.update(channels(1490, 1090), 0);
    QVERIFY(control.update(channels(1890, 1490), 100).toggleVideo);
    QVERIFY(control.update(channels(1510, 1110), 200).autoFocus);
    control.update(channels(1510, 1910), 300);
    QVERIFY(control.update(channels(1510, 1110), 400).takePhoto);
}

UT_REGISTER_TEST(RCCameraControlTest, TestLabel::Unit)
