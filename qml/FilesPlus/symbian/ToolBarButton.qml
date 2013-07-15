import QtQuick 1.1
import com.nokia.symbian 1.1

ToolButton {
    id: toolBarButton
    iconSource: buttonIconSource
    platformInverted: window.platformInverted
    flat: true

    property string buttonIconSource
    property bool isPressAndHold: false

    signal pressAndHold()
    signal clicked()

    onPressedChanged: {
        if (pressed) {
            isPressAndHold = false;
        }
    }

    onPlatformPressAndHold: {
        isPressAndHold = true;
        pressAndHold();
    }

    Rectangle {
        id: checkableBG
        color: "#00AAFF"
        opacity: 0.2
        visible: checked
        width: parent.width
        height: parent.height
        y: 0
    }
}
