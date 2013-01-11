import QtQuick 1.1
import "Utility.js" as Utility

Rectangle {
    id: textIndicator
    color: "#00AAFF"
    border.color: (inverted) ? borderColorInverted : borderColor
    border.width: 1
    width: Math.max(20, textLabel.width + 4)
    height: 20
    radius: 10
    visible: (text != "")
    
    property alias text: textLabel.text
    property color textColor: textLabel.color
    property color textPointSize: textLabel.font.pointSize
    property color borderColor: "white"
    property color borderColorInverted: "black"
    property bool inverted: window.platformInverted

    Text {
        id: textLabel
        color: "white"
        font.pointSize: 5
        anchors.centerIn: parent
        text: ""
    }
}
