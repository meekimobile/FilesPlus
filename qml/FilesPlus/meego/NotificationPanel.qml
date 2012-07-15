import QtQuick 1.1
import com.nokia.meego 1.0
import "Utility.js" as Utility

Rectangle {
    id: notification
    
    property string text: notifyMessage.text
    
    anchors.centerIn: parent
    width: parent.width - 20
    height: implicitHeight
    color: "black"
    opacity: 0.8
    z: 2
    visible: false
    
    Timer {
        id: notificationTimer
        interval: 5000
        running: false
        onTriggered: {
            hideAnimation.start();
        }
    }
    
    SequentialAnimation {
        id: hideAnimation
        NumberAnimation { target: notification; property: "opacity"; duration: 200; easing.type: Easing.Linear; to: 0 }
        PropertyAction { target: notification; property: "visible"; value: false }
    }
    
    MouseArea {
        anchors.fill: parent
        onClicked: {
            hideAnimation.start();
        }
    }
    
    Text {
        id: notifyMessage
        anchors.fill: parent
        anchors.margins: 10
        color: "white"
        font.pointSize: 14
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
    }
}
