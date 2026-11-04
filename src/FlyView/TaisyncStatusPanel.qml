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
        spacing: ScreenTools.defaultFontPixelHeight * 0.4

        QGCLabel {
            text: "TAISYNC"
            font.bold: true
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "white"
            opacity: 0.3
        }

        QGCLabel {
            text: TaisyncRadio.linkStatus + " (" + TaisyncRadio.linkQuality + "%)"
            color: TaisyncRadio.connected ? "#00E676" : "#FF5252"
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        QGCLabel { text: "<b>SNR:</b> " + TaisyncRadio.groundSnr + " | " + TaisyncRadio.airSnr; color: "white" }
        QGCLabel { text: "<b>RSSI:</b> " + TaisyncRadio.groundRssi0 + "/" + TaisyncRadio.groundRssi1; color: "white" }
        QGCLabel { text: "<b>UL:</b> " + TaisyncRadio.uplinkRate; color: "white" }
        QGCLabel { text: "<b>DL:</b> " + TaisyncRadio.downlinkRate; color: "white" }
        QGCLabel { text: "<b>Freq:</b> " + TaisyncRadio.txFreq; color: "white" }

        QGCLabel {
            text: TaisyncRadio.mcs
            color: "white"
            font.pointSize: ScreenTools.smallFontPointSize
            Layout.maximumWidth: ScreenTools.defaultFontPixelWidth * 12
            wrapMode: Text.Wrap
        }
    }
}