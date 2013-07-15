import QtQuick 1.1
import com.nokia.meego 1.0

ToolIcon {
    id: toolBarButton
    iconId: (buttonIconSource.indexOf("toolbar-") == 0) ? buttonIconSource : ""
    iconSource: (buttonIconSource.indexOf("toolbar-") == 0) ? "" : buttonIconSource

    property string buttonIconSource
    property bool checked: false

    signal pressAndHold()

    Component.onCompleted: {
//        console.debug("toolBarButton onCompleted " + children[0] + " " + children[1]);
        var mouseArea = children[1];
        mouseArea.pressAndHold.connect(pressAndHold);
    }

    Rectangle {
        id: checkableBG
        color: "#00AAFF"
        opacity: 0.2
        visible: checked
        width: parent.width
        height: parent.height
        y: (window.inPortrait) ? 0 : 5
    }
}
