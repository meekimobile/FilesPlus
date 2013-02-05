import QtQuick 1.1
import "Utility.js" as Utility

Rectangle {
    property alias text: headerText.text
    property alias horizontalAlignment: headerText.horizontalAlignment
    property int textLeftMargin: 10

    id: titlePanel
    anchors.top: parent.top
    width: parent.width
    height: 40
    gradient: Gradient {
        GradientStop {
            position: 0
            color: (theme.inverted) ? "#242424" : "#FFFFFF"
        }

        GradientStop {
            position: 0.790
            color: (theme.inverted) ? "#0F0F0F" : "#F0F0F0"
        }

        GradientStop {
            position: 1
            color: (theme.inverted) ? "#000000" : "#DBDBDB"
        }
    }
    
    Text {
        id: headerText
        width: parent.width - 20
        height: parent.height
        anchors.left: parent.left
        anchors.leftMargin: textLeftMargin
        verticalAlignment: Text.AlignVCenter
        color: (theme.inverted) ? "white" : "black"
        font.pointSize: 18
        elide: Text.ElideMiddle
    }

    Rectangle {
        width: parent.width
        height: 1
        anchors.bottom: parent.bottom
        color: (theme.inverted) ? "black" : "grey"
    }

    TextIndicator {
        id: messageLoggerCounter
        text: (messageLoggerModel.messageCount > 0) ? (appInfo.emptyStr + qsTr("Message") + " " + messageLoggerModel.newMessageCount + "/" + messageLoggerModel.messageCount) : ""
        height: parent.height
        radius: 0
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        visible: (messageLoggerModel.messageCount > 0)
        borderColor: "transparent"
        borderColorInverted: "transparent"
        textColor: "white"
        textColorInverted: (messageLoggerModel.newMessageCount > 0) ? "white" : "black"
        pressedTextColor: "white"
        pressedTextColorInverted: "white"
        gradient: {
            if (_pressed) {
                return pressedGradient;
            } else if (messageLoggerModel.newMessageCount > 0) {
                return highlightGradient;
            } else {
                return transparentGradient;
            }
        }

        onClicked: {
//            console.debug("titlePanel onClicked pageStack.depth " + pageStack.depth);
            if (pageStack.currentPage.name != "messageLoggerPage") {
                pageStack.push(Qt.resolvedUrl("MessageLoggerPage.qml"));
            }
        }
    }

    Gradient {
        id: transparentGradient
        GradientStop {
            position: 0
            color: "transparent"
        }

        GradientStop {
            position: 1
            color: "transparent"
        }
    }

    Gradient {
        id: highlightGradient
        GradientStop {
            position: 0
            color: "#0091d9"
        }

        GradientStop {
            position: 0.500
            color: "#00aaff"
        }

        GradientStop {
            position: 1
            color: "#001f3c"
        }
    }

    Gradient {
        id: pressedGradient
        GradientStop {
            position: 0
            color: "#141414"
        }

        GradientStop {
            position: 0.500
            color: "#4d4d4d"
        }

        GradientStop {
            position: 1
            color: "#141414"
        }
    }
}
