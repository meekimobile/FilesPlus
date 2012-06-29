import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

CommonDialog {
    id: messageDialog
    
    property alias message: contentText.text
    property bool autoClose: false
    property int autoCloseInterval: 3000

    titleIcon: "FilesPlusIcon.svg"
    buttonTexts: ["Ok"]
    content: Text {
        id: contentText
        width: parent.width - 20
        height: implicitHeight
        color: "white"
//        font.pointSize: 6
        wrapMode: Text.WordWrap
        anchors.horizontalCenter: parent.horizontalCenter
    }
    
    SequentialAnimation {
        id: hideAction

        PauseAnimation { duration: autoCloseInterval }
        ScriptAction {
            script: close();
        }
    }

    onStatusChanged: {
        if (status == DialogStatus.Open) {
            if (autoClose) hideAction.restart();
        }

        if (status == DialogStatus.Closed) {
            hideAction.stop();
            titleText = "";
            message = "";
        }
    }
}
