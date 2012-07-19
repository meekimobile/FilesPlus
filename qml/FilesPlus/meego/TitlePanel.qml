import QtQuick 1.1
import "Utility.js" as Utility

Rectangle {
    property alias text: headerText.text
    property alias horizontalAlignment: headerText.horizontalAlignment

    id: titlePanel
    anchors.top: parent.top
    width: parent.width
    height: 40
    gradient: Gradient {
        GradientStop {
            position: 0
            color: (theme.inverted) ? "#242424" : "#DBDBDB"
        }

        GradientStop {
            position: 0.790
            color: (theme.inverted) ? "#0F0F0F" : "#F0F0F0"
        }

        GradientStop {
            position: 1
            color: (theme.inverted) ? "#000000" : "#FFFFFF"
        }
    }
    
    Text {
        id: headerText
        anchors.fill: parent
        anchors.margins: 3
        anchors.verticalCenter: parent.verticalCenter
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
}
