// import QtQuick 1.0 // to target S60 5th Edition or Maemo 5
import QtQuick 1.1

Rectangle {
    id: popupMenu
    x: 0
    y: 0
    width: implicitWidth
    height: implicitHeight
    visible: false
    color: "white"
    border.color: "gray"
    border.width: 1
    radius: 5

    onVisibleChanged: {
        console.debug("itemMenu visible=" + visible);
        if (visible) itemMenuTimer.restart();
    }

    Timer {
        id: itemMenuTimer
        interval: 3000
        running: false;
        onTriggered: itemMenu.visible = false
    }

    Column {
        anchors.margins: 5
        spacing: 0
        Rectangle {
            width: 146
            height: 30
            color: "black"
            border.color: "gray"
            border.width: 1
            radius: 5
            Text { text: "one"; color: "white" }
        }
        Rectangle {
            width: 145
            height: 30
            color: "black"
            border.color: "gray"
            border.width: 1
            radius: 5
            Text { text: "two"; color: "white" }
        }
        Rectangle {
            width: 145
            height: 30
            color: "black"
            border.color: "gray"
            border.width: 1
            radius: 5
            Text { text: "three"; color: "white" }
        }
    }
}
