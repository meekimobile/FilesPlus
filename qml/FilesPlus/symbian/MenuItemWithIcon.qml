import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

MenuItem {
    id: root
    platformInverted: window.platformInverted

    property string name
    property string iconSource
    property bool checked

    Image {
        id: menuIcon

        anchors {
            left: parent.left
            leftMargin: platformStyle.paddingMedium * 2
            verticalCenter: parent.verticalCenter
        }

        source: (checked ? (!window.platformInverted ? "ok.svg" : "ok_inverted.svg") : iconSource)
        visible: source != undefined
        sourceSize.width: platformStyle.graphicSizeSmall
        sourceSize.height: platformStyle.graphicSizeSmall
    }
}
