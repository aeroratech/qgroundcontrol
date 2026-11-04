import QtQuick
import QtQuick.Layouts
import QGroundControl
import QGroundControl.Controls
import QGroundControl.Taisync

Rectangle {
    id: root
    implicitWidth: mainLayout.implicitWidth + (ScreenTools.defaultFontPixelWidth * 2)
    implicitHeight: mainLayout.implicitHeight + (ScreenTools.defaultFontPixelHeight * 1.5)
    width: implicitWidth
    height: implicitHeight
    radius: ScreenTools.defaultFontPixelWidth * 0.5
    color: Qt.rgba(0, 0, 0, 0.75)
    border.color: TaisyncRadio.connected ? "#00E676" : "#FF5252"
    border.width: 1

    ColumnLayout {
        id: mainLayout
        anchors.centerIn: parent
        spacing: ScreenTools.defaultFontPixelHeight * 0.25

        QGCLabel {
            text: "TAISYNC: " + TaisyncRadio.linkStatus + " (" + TaisyncRadio.linkQuality + "%)"
            font.bold: true
            color: "white"
        }
        QGCLabel {
            text: "GND SNR: " + TaisyncRadio.groundSnr + " dB | AIR SNR: " + TaisyncRadio.airSnr + " dB"
            color: "white"
        }
        QGCLabel {
            text: "GND RSSI: " + TaisyncRadio.groundRssi0 + " / " + TaisyncRadio.groundRssi1 + " dBm"
            color: "white"
        }
        QGCLabel {
            text: "MCS: " + TaisyncRadio.mcs + " | UL/DL: " + TaisyncRadio.uplinkRate + "/" + TaisyncRadio.downlinkRate
            color: "white"
        }
    }
}