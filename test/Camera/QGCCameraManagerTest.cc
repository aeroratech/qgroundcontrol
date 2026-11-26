#include "QGCCameraManagerTest.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>

#include "CameraMetaData.h"
#include "MAVLinkLib.h"
#include "MultiVehicleManager.h"
#include "QGCCameraManager.h"
#include "Vehicle.h"
#include "VehicleCameraControl.h"
#include "VehicleLinkManager.h"

void QGCCameraManagerTest::_testCameraList()
{
    const QList<CameraMetaData*> cameraList = CameraMetaData::parseCameraMetaData();
    QVERIFY(!cameraList.isEmpty());
    qDeleteAll(cameraList);
}

/// Reproduces issue #13251 (crash 1): use-after-free of QGCCameraManager::CameraStruct.
///
/// The camera info request commands are queued in the vehicle's MavCommandQueue with
/// resultHandlerData pointing at the CameraStruct. _checkForLostCameras() deletes the
/// CameraStruct when the camera goes silent, without cancelling the pending command.
/// When the command later times out, the failure handler dereferences the freed struct.
///
/// In production this is reached when a camera reappears after a silent period (which
/// re-requests camera info with infoReceived still true) and then goes silent again for
/// kSilentTimeoutMs while the request is pending. The test shortcuts the two 5 second
/// silent periods by performing the same cleanup _checkForLostCameras() does (take from
/// _cameraInfoRequest + delete) while the request is pending.
///
/// The use-after-free is only reliably detected when running under AddressSanitizer
/// (CI ASan job). Without ASan the test exercises the path but may pass silently.
void QGCCameraManagerTest::_testLostCameraCleanupWithPendingRequest()
{
    ignoreLogMessage("Vehicle.MavCommandQueue", QtWarningMsg,
                     QRegularExpression("Giving up sending command after max retries:"));

    // Enable camera manager debug logging: the failure handlers log CameraStruct
    // fields, widening the use-after-free reads for ASan to catch. Scoped so the
    // process-global filter rules are restored even on early test failure.
    const QByteArray oldLoggingRules = qgetenv("QT_LOGGING_RULES");
    QLoggingCategory::setFilterRules(QStringLiteral("Camera.QGCCameraManager.debug=true"));
    const auto restoreLoggingRules = qScopeGuard([oldLoggingRules]() {
        QLoggingCategory::setFilterRules(QString::fromUtf8(oldLoggingRules));
    });

    // Ensure MockLink never responds to the camera info request so it stays pending
    // and eventually times out.
    mockLink()->setRequestMessageNoResponse(MAVLINK_MSG_ID_CAMERA_INFORMATION);

    // Inject a camera component heartbeat. The camera manager creates a CameraStruct
    // and immediately requests CAMERA_INFORMATION with the struct as handler data.
    mavlink_message_t msg{};
    (void) mavlink_msg_heartbeat_pack_chan(vehicle()->id(),
                                           MAV_COMP_ID_CAMERA,
                                           mockLink()->mavlinkChannel(),
                                           &msg,
                                           MAV_TYPE_CAMERA,
                                           MAV_AUTOPILOT_INVALID,
                                           0,   // base_mode
                                           0,   // custom_mode
                                           MAV_STATE_ACTIVE);
    mockLink()->respondWithMavlinkMessage(msg);

    QGCCameraManager* cameraManager = vehicle()->cameraManager();
    QVERIFY(cameraManager);

    QTRY_VERIFY_WITH_TIMEOUT(cameraManager->findCameraStruct(MAV_COMP_ID_CAMERA) != nullptr, TestTimeout::longMs());
    QTRY_VERIFY_WITH_TIMEOUT(vehicle()->isMavCommandPending(MAV_COMP_ID_CAMERA, MAV_CMD_REQUEST_MESSAGE),
                             TestTimeout::longMs());

    // Mimic the lost-camera cleanup in _checkForLostCameras() while the camera info
    // request is still pending in the MavCommandQueue.
    QGCCameraManager::CameraStruct* pInfo = cameraManager->_cameraInfoRequest.take(QString::number(MAV_COMP_ID_CAMERA));
    QVERIFY(pInfo);
    delete pInfo;

    // Let the pending request retry and give up. Before the fix the give-up failure
    // handler dereferenced the freed CameraStruct (issue #13251 crash 1) — detected
    // by the CI ASan job. With the fix, the handler resolves the compId via the
    // manager-owned request context, finds the camera gone, and bails out.
    QTRY_VERIFY_WITH_TIMEOUT(!vehicle()->isMavCommandPending(MAV_COMP_ID_CAMERA, MAV_CMD_REQUEST_MESSAGE),
                             TestTimeout::longMs());
}

void QGCCameraManagerTest::_testRCCameraButtons()
{
    // MockLink currently advertises a fileCrc that fails the metadata schema.
    // Keep this unrelated fixture warning separate from camera command assertions.
    ignoreLogMessage("ComponentInformation.CompInfoGeneral", QtWarningMsg,
                     QRegularExpression("^Metadata json schema validation failed:.*fileCrc:"));
    ignoreLogMessage("Vehicle.Vehicle", QtWarningMsg, QRegularExpression("Non-contiguous RC channels detected"));
    _disconnectMockLink();
    _connectMockLink(MAV_AUTOPILOT_PX4, MockConfiguration::FailNone, MockConfiguration::OptionEnableCamera);
    QVERIFY(vehicle());
    QVERIFY(waitForInitialConnect());
    auto* manager = vehicle()->cameraManager();
    QVERIFY(manager);
    QTRY_VERIFY_WITH_TIMEOUT(manager->cameras()->count() >= 2, TestTimeout::longMs());
    // Use a focus-capable camera controller with the real MockLink camera backend.
    manager->_cameras.clear();
    manager->_cameraLabels.clear();
    mavlink_camera_information_t info{};
    info.flags = CAMERA_CAP_FLAGS_CAPTURE_IMAGE | CAMERA_CAP_FLAGS_CAPTURE_VIDEO | CAMERA_CAP_FLAGS_HAS_BASIC_FOCUS;
    info.cam_definition_version = 1;
    auto* camera = new VehicleCameraControl(&info, vehicle(), MAV_COMP_ID_CAMERA, manager);
    manager->_addCameraControlToLists(camera);
    QCOMPARE(manager->currentCameraInstance(), camera);
    mavlink_camera_capture_status_t status{};
    camera->handleCameraCaptureStatus(status);

    // Inspect real outbound MAVLink commands, including target and focus parameters.
    QSignalSpy sent(mockLink(), &MockLink::writeBytesQueuedSignal);
    auto commands = [&sent](MAV_CMD command) {
        QList<mavlink_command_long_t> result;
        mavlink_message_t buffer{}, message{};
        mavlink_status_t parser{}, output{};
        for (const auto& arguments : sent) {
            const auto bytes = arguments[0].toByteArray();
            for (const auto byte : bytes) {
                if (mavlink_frame_char_buffer(&buffer, &parser, static_cast<uint8_t>(byte), &message, &output) !=
                        MAVLINK_FRAMING_OK ||
                    message.msgid != MAVLINK_MSG_ID_COMMAND_LONG) {
                    continue;
                }
                mavlink_command_long_t decoded{};
                mavlink_msg_command_long_decode(&message, &decoded);
                if (decoded.command == command) {
                    result.append(decoded);
                }
            }
        }
        return result;
    };
    int rcReceived = 0;
    const auto rcConnection =
        connect(vehicle(), &Vehicle::mavlinkMessageReceived, this, [&rcReceived](const mavlink_message_t& message) {
            if (message.msgid == MAVLINK_MSG_ID_RC_CHANNELS) {
                ++rcReceived;
            }
        });
    const auto disconnectRC = qScopeGuard([rcConnection]() { disconnect(rcConnection); });
    auto sendChannels = [this, &rcReceived](int video, int photo, bool noncontiguous = false) {
        const int expected = rcReceived + 1;
        mavlink_message_t message{};
        mavlink_msg_rc_channels_pack_chan(
            vehicle()->id(), MAV_COMP_ID_AUTOPILOT1, mockLink()->mavlinkChannel(), &message, 0, noncontiguous ? 7 : 6,
            1500, 1500, 1100, 1500, video, photo, noncontiguous ? 1500 : UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX,
            UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, 100);
        mockLink()->respondWithMavlinkMessage(message);
        return UnitTest::waitForCondition([&rcReceived, expected]() { return rcReceived >= expected; },
                                          TestTimeout::shortMs(), QStringLiteral("RC_CHANNELS received"));
    };

    QVERIFY(sendChannels(1500, 1100));
    QVERIFY(sendChannels(1500, 1500));
    QVERIFY(sendChannels(1500, 1500));
    QVERIFY(commands(MAV_CMD_SET_CAMERA_FOCUS).isEmpty());
    QVERIFY(sendChannels(1500, 1100));
    QTRY_COMPARE_WITH_TIMEOUT(commands(MAV_CMD_SET_CAMERA_FOCUS).size(), 1, TestTimeout::shortMs());
    QTRY_VERIFY_WITH_TIMEOUT(
        !vehicle()->isMavCommandPending(manager->currentCameraInstance()->compID(), MAV_CMD_SET_CAMERA_FOCUS),
        TestTimeout::shortMs());
    const auto focus = commands(MAV_CMD_SET_CAMERA_FOCUS).first();
    QCOMPARE(focus.target_system, vehicle()->id());
    QCOMPARE(focus.target_component, MAV_COMP_ID_CAMERA);
    QCOMPARE(focus.param1, float(FOCUS_TYPE_AUTO));
    QCOMPARE(focus.param2, 0.0f);

    QVERIFY(sendChannels(1500, 1500));
    QVERIFY(sendChannels(1500, 1900));
    QVERIFY(sendChannels(1500, 1500));
    QVERIFY(sendChannels(1500, 1100));
    QTRY_COMPARE_WITH_TIMEOUT(commands(MAV_CMD_IMAGE_START_CAPTURE).size(), 1, TestTimeout::shortMs());
    QCOMPARE(commands(MAV_CMD_SET_CAMERA_FOCUS).size(), 1);
    QCOMPARE(commands(MAV_CMD_IMAGE_START_CAPTURE).first().target_component, MAV_COMP_ID_CAMERA);

    QTRY_COMPARE_WITH_TIMEOUT(camera->capturePhotosState(), MavlinkCameraControlInterface::CapturePhotosStateIdle,
                              TestTimeout::shortMs());
    QVERIFY(sendChannels(1900, 1100));
    QVERIFY(sendChannels(1900, 1100));
    QTRY_COMPARE_WITH_TIMEOUT(commands(MAV_CMD_VIDEO_START_CAPTURE).size(), 1, TestTimeout::shortMs());
    QCOMPARE(commands(MAV_CMD_VIDEO_START_CAPTURE).first().target_component, MAV_COMP_ID_CAMERA);
    status.video_status = 1;
    camera->handleCameraCaptureStatus(status);
    QVERIFY(sendChannels(1500, 1100));
    QVERIFY(sendChannels(1900, 1100));
    QTRY_COMPARE_WITH_TIMEOUT(commands(MAV_CMD_VIDEO_STOP_CAPTURE).size(), 1, TestTimeout::shortMs());

    // Rejected RC frames must discard a gesture even when the generic RC parser
    // refuses to emit rcChannelsRawChanged for this message.
    status.video_status = 0;
    camera->handleCameraCaptureStatus(status);
    QVERIFY(sendChannels(1500, 1900));
    QVERIFY(sendChannels(1500, UINT16_MAX, true));
    QVERIFY(sendChannels(1500, 1100));
    QVERIFY(sendChannels(1500, 1500));
    QVERIFY(sendChannels(1500, 1100));
    QTRY_COMPARE_WITH_TIMEOUT(commands(MAV_CMD_SET_CAMERA_FOCUS).size(), 2, TestTimeout::shortMs());
    QTRY_VERIFY_WITH_TIMEOUT(
        !vehicle()->isMavCommandPending(manager->currentCameraInstance()->compID(), MAV_CMD_SET_CAMERA_FOCUS),
        TestTimeout::shortMs());
    QCOMPARE(commands(MAV_CMD_IMAGE_START_CAPTURE).size(), 1);

    // A link interruption must discard a pending photo press.
    QVERIFY(sendChannels(1500, 1900));
    auto* linkManager = vehicle()->vehicleLinkManager();
    emit linkManager->communicationLostChanged(true);
    emit linkManager->communicationLostChanged(false);
    QVERIFY(sendChannels(1500, 1100));
    QVERIFY(sendChannels(1500, 1500));
    QVERIFY(sendChannels(1500, 1100));
    QTRY_COMPARE_WITH_TIMEOUT(commands(MAV_CMD_SET_CAMERA_FOCUS).size(), 3, TestTimeout::shortMs());
    QTRY_VERIFY_WITH_TIMEOUT(
        !vehicle()->isMavCommandPending(manager->currentCameraInstance()->compID(), MAV_CMD_SET_CAMERA_FOCUS),
        TestTimeout::shortMs());
    QCOMPARE(commands(MAV_CMD_IMAGE_START_CAPTURE).size(), 1);

    // Switching away and back must cancel a held gesture and ignore inactive input.
    QVERIFY(sendChannels(1500, 1900));
    auto* vehicles = MultiVehicleManager::instance();
    vehicles->setActiveVehicle(nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(!vehicles->activeVehicle(), TestTimeout::shortMs());
    QVERIFY(sendChannels(1500, 1100));
    QVERIFY(sendChannels(1500, 1500));
    QVERIFY(sendChannels(1500, 1100));
    vehicles->setActiveVehicle(vehicle());
    QTRY_COMPARE_WITH_TIMEOUT(vehicles->activeVehicle(), vehicle(), TestTimeout::shortMs());
    QVERIFY(sendChannels(1500, 1900));
    QVERIFY(sendChannels(1500, 1100));
    QVERIFY(sendChannels(1500, 1500));
    QVERIFY(sendChannels(1500, 1100));
    QTRY_COMPARE_WITH_TIMEOUT(commands(MAV_CMD_SET_CAMERA_FOCUS).size(), 4, TestTimeout::shortMs());
    QTRY_VERIFY_WITH_TIMEOUT(
        !vehicle()->isMavCommandPending(manager->currentCameraInstance()->compID(), MAV_CMD_SET_CAMERA_FOCUS),
        TestTimeout::shortMs());
    QCOMPARE(commands(MAV_CMD_IMAGE_START_CAPTURE).size(), 1);

    auto* secondCamera = new VehicleCameraControl(&info, vehicle(), MAV_COMP_ID_CAMERA2, manager);
    manager->_addCameraControlToLists(secondCamera);
    QVERIFY(sendChannels(1500, 1100));
    QVERIFY(sendChannels(1500, 1900));
    manager->setCurrentCamera(1);
    QCOMPARE(manager->currentCameraInstance(), secondCamera);
    QVERIFY(sendChannels(1500, 1100));
    QVERIFY(sendChannels(1500, 1500));
    QVERIFY(sendChannels(1500, 1100));
    QTRY_COMPARE_WITH_TIMEOUT(commands(MAV_CMD_SET_CAMERA_FOCUS).size(), 5, TestTimeout::shortMs());
    QTRY_VERIFY_WITH_TIMEOUT(
        !vehicle()->isMavCommandPending(manager->currentCameraInstance()->compID(), MAV_CMD_SET_CAMERA_FOCUS),
        TestTimeout::shortMs());
    QCOMPARE(commands(MAV_CMD_SET_CAMERA_FOCUS).last().target_component, MAV_COMP_ID_CAMERA2);
    QCOMPARE(commands(MAV_CMD_IMAGE_START_CAPTURE).size(), 1);

    // Buttons received while no camera exists must never be replayed on reconnection.
    manager->_cameras.clear();
    manager->_cameraLabels.clear();
    emit manager->camerasChanged();
    QVERIFY(sendChannels(1500, 1100));
    QVERIFY(sendChannels(1500, 1900));
    manager->_addCameraControlToLists(camera);
    manager->setCurrentCamera(0);
    QVERIFY(sendChannels(1500, 1100));
    QVERIFY(sendChannels(1500, 1500));
    QVERIFY(sendChannels(1500, 1100));
    QTRY_COMPARE_WITH_TIMEOUT(commands(MAV_CMD_SET_CAMERA_FOCUS).size(), 6, TestTimeout::shortMs());
    QTRY_VERIFY_WITH_TIMEOUT(
        !vehicle()->isMavCommandPending(manager->currentCameraInstance()->compID(), MAV_CMD_SET_CAMERA_FOCUS),
        TestTimeout::shortMs());
    QCOMPARE(commands(MAV_CMD_IMAGE_START_CAPTURE).size(), 1);
}

UT_REGISTER_TEST(QGCCameraManagerTest, TestLabel::Integration, TestLabel::Vehicle)
