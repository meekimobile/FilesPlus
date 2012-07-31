import QtQuick 1.1
import com.nokia.meego 1.0
import "Utility.js" as Utility

CommonDialog {
    id: messageDialog
    
    property alias message: messageDialog.contentText
    property bool autoClose: false
    property int autoCloseInterval: 3000

    titleIcon: "FilesPlusIcon.svg"
    buttonTexts: [appInfo.emptyStr+qsTr("OK")]
    
    SequentialAnimation {
        id: hideAction

        PauseAnimation { duration: autoCloseInterval }
        ScriptAction {
            script: close();
        }
    }

    onButtonClicked: {
        if (index == 0) {
            accept();
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
