import QtQuick 1.1
import com.nokia.symbian 1.1

ToolButton {
    id: toolBarButton
    platformInverted: window.platformInverted
    flat: true

    signal pressAndHold()

    MouseArea {
        anchors.fill: parent
        onPressAndHold: {
            toolBarButton.pressAndHold(mouse);
        }
        onClicked: {
            toolBarButton.clicked(mouse);
        }
    }
}
