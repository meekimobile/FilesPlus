import QtQuick 1.1
import "Utility.js" as Utility

Rectangle {
    id: textIndicator
    color: "#00AAFF"
    border.color: "white"
    border.width: 2
    width: Math.max(20, textLabel.width + 4)
    height: 20
    radius: 10
    visible: (text != "")
    
    property alias text: textLabel.text
    property color textColor: textLabel.color
    property color textPointSize: textLabel.font.pointSize

    Text {
        id: textLabel
        color: "white"
        font.pointSize: 5
        anchors.centerIn: parent
        text: ""
    }
}
