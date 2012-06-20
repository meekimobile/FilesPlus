import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

CommonDialog {
    id: messageDialog
    
    property alias message: contentText.text
    
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
    
    onStatusChanged: {
        if (status == DialogStatus.Closed) {
            titleText = "";
            message = "";
        }
    }
}
