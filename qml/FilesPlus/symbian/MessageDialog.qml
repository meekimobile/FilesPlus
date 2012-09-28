import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

CommonDialog {
    id: messageDialog
    height: contentText.implicitHeight + 120
    
    property alias message: contentText.text
    property bool autoClose: false
    property int autoCloseInterval: 5000

    titleIcon: "FilesPlusIcon.svg"
    buttonTexts: [appInfo.emptyStr+qsTr("OK")]

    content: Text {
        id: contentText
        width: parent.width - 20
        color: "white"
        font.pointSize: 6
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
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
            autoClose = false;
        }
    }
}
