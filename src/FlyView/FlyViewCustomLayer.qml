import QtQuick
import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.Taisync 1.0

Item {
    id: _root

    property var parentToolInsets               // Insets from the parent FlyViewWidgetLayer
    property var totalToolInsets: _totalToolInsets // Returned insets including our custom HUD
    property var mapControl

    TaisyncStatusPanel {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 1.5
        anchors.bottomMargin: ScreenTools.defaultFontPixelHeight * 1.5
    }

    QGCToolInsets {
        id: _totalToolInsets
        leftEdgeTopInset:       _root.parentToolInsets.leftEdgeTopInset
        leftEdgeCenterInset:    _root.parentToolInsets.leftEdgeCenterInset
        leftEdgeBottomInset:    _root.parentToolInsets.leftEdgeBottomInset
        rightEdgeTopInset:      _root.parentToolInsets.rightEdgeTopInset
        rightEdgeCenterInset:   _root.parentToolInsets.rightEdgeCenterInset
        rightEdgeBottomInset:   _root.parentToolInsets.rightEdgeBottomInset
        topEdgeLeftInset:       taisyncPanel.y + taisyncPanel.height
        topEdgeCenterInset:     _root.parentToolInsets.topEdgeCenterInset
        topEdgeRightInset:      _root.parentToolInsets.topEdgeRightInset
        bottomEdgeLeftInset:    _root.parentToolInsets.bottomEdgeLeftInset
        bottomEdgeCenterInset:  _root.parentToolInsets.bottomEdgeCenterInset
        bottomEdgeRightInset:   _root.parentToolInsets.bottomEdgeRightInset
    }
}