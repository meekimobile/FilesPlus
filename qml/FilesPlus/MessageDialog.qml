import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

CommonDialog {
    id: messageDialog
    
    property alias message: contentText.text
    property bool autoClosed: false
    property int autoClosedInterval: 3000

    titleIcon: "FilesPlusIcon.svg"
    buttonTexts: ["Ok"]
    content: Text {
        id: contentText
        width: parent.width - 20
        color: "white"
        wrapMode: Text.WordWrap
        height: implicitHeight
        anchors.horizontalCenter: parent.horizontalCenter
    }
    
    SequentialAnimation {
        id: hideAction

        PauseAnimation { duration: autoClosedInterval }
        ScriptAction {
            script: close();
        }
    }

    onStatusChanged: {
        if (status == DialogStatus.Open) {
            if (autoClosed) hideAction.restart();
        }

        if (status == DialogStatus.Closed) {
            hideAction.stop();
            titleText = "";
            message = "";
        }
    }
}
