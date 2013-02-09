import QtQuick 1.1
import "Utility.js" as Utility

Rectangle {
    id: textIndicator
    color: "#00AAFF"
    border.color: (!inverted) ? borderColor : borderColorInverted
    border.width: 1
    width: Math.max(size, textLabel.width + 4)
    height: size
    radius: size / 2
    visible: (text != "")
    states: [
        State {
            name: "pressed"
            when: _pressed
            PropertyChanges {
                target: textIndicator
                color: (!inverted) ? pressedColor : pressedColorInverted
                explicit: true
            }
            PropertyChanges {
                target: textLabel
                color: (!inverted) ? pressedTextColor : pressedTextColorInverted
                explicit: true
            }
        }
    ]

    property alias text: textLabel.text
    property alias textPointSize: textLabel.font.pointSize

    property int size: 24
    property color textColor: "white"
    property color textColorInverted: "white"
    property color borderColor: "white"
    property color borderColorInverted: "black"
    property bool inverted: !theme.inverted
    property bool _pressed: false
    property color pressedColor: "white"
    property color pressedColorInverted: "black"
    property color pressedTextColor: "black"
    property color pressedTextColorInverted: "white"
    property bool mouseAreaDisabled: true

    signal clicked(variant mouse)
    signal pressAndHold(variant mouse)

    Text {
        id: textLabel
        color: (!inverted) ? textColor : textColorInverted
        font.pointSize: 14
        anchors.centerIn: parent
        text: ""
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: !mouseAreaDisabled
        onPressed: _pressed = true;
        onReleased: _pressed = false;
        onClicked: textIndicator.clicked(mouse);
        onPressAndHold: textIndicator.pressAndHold(mouse);
    }
}
