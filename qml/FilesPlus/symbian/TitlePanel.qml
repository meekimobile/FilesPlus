import QtQuick 1.1
import "Utility.js" as Utility

Rectangle {
    property alias text: headerText.text
    property alias horizontalAlignment: headerText.horizontalAlignment
    property int textLeftMargin: 10

    id: titlePanel
    anchors.top: parent.top
    width: parent.width
    height: 32
    gradient: Gradient {
        GradientStop {
            position: 0
            color: (!window.platformInverted) ? "#242424" : "#FFFFFF"
        }

        GradientStop {
            position: 0.790
            color: (!window.platformInverted) ? "#0F0F0F" : "#F0F0F0"
        }

        GradientStop {
            position: 1
            color: (!window.platformInverted) ? "#000000" : "#DBDBDB"
        }
    }
    
    Text {
        id: headerText
        width: parent.width - 20
        height: parent.height
        anchors.left: parent.left
        anchors.leftMargin: textLeftMargin
        verticalAlignment: Text.AlignVCenter
        color: (!window.platformInverted) ? "white" : "black"
        font.pointSize: 6
        elide: Text.ElideMiddle
    }

    Rectangle {
        width: parent.width
        height: 1
        anchors.bottom: parent.bottom
        color: (!window.platformInverted) ? "black" : "grey"
    }

    TextIndicator {
        id: messageLoggerCounter
        text: (messageLoggerModel.newMessageCount > 0) ? (appInfo.emptyStr + qsTr("Message") + " " + messageLoggerModel.newMessageCount) : ""
        height: parent.height - 6
        anchors.right: parent.right
        anchors.rightMargin: 3
        anchors.verticalCenter: parent.verticalCenter

        onClicked: {
            pageStack.push(Qt.resolvedUrl("MessageLoggerPage.qml"));
        }
    }
}
