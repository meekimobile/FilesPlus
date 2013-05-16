import QtQuick 1.1
import com.nokia.meego 1.0

ToolIcon {
    id: toolBarButton
    iconId: (buttonIconSource.indexOf("toolbar-") == 0) ? buttonIconSource : ""
    iconSource: (buttonIconSource.indexOf("toolbar-") == 0) ? "" : buttonIconSource

    property string buttonIconSource

    signal pressAndHold()

    Component.onCompleted: {
//        console.debug("toolBarButton onCompleted " + children[0] + " " + children[1]);
        var mouseArea = children[1];
        mouseArea.pressAndHold.connect(pressAndHold);
    }
}
