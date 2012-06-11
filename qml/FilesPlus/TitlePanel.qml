import QtQuick 1.1
import "Utility.js" as Utility

Rectangle {
    property alias text: headerText.text

    id: titlePanel
    anchors.top: parent.top
    width: parent.width
    height: 32
    gradient: Gradient {
        GradientStop {
            position: 0
            color: "#606060"
        }

        GradientStop {
            position: 0.790
            color: "#202020"
        }

        GradientStop {
            position: 1
            color: "#101010"
        }
    }
    
    Text {
        id: headerText
        anchors.fill: parent
        anchors.margins: 3
        anchors.verticalCenter: parent.verticalCenter
        color: "white"
    }
}
