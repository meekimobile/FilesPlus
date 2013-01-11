import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

CommonDialog {
    id: messageDialog

    property alias message: content.text
    property bool autoClose: false
    property int autoCloseInterval: 3000

    titleIcon: "FilesPlusIcon.svg"
    buttonTexts: [appInfo.emptyStr+qsTr("OK")]
    content: Flickable {
        id: contentFlick
        width: parent.width - 20
        height: Math.min(content.implicitHeight, screen.height - ((screen.height > screen.width) ? 260 : 180) )
        anchors.horizontalCenter: parent.horizontalCenter
        contentWidth: content.width
        contentHeight: content.height
        flickableDirection: Flickable.VerticalFlick
        clip: true

        Text {
            id: content
            color: "white"
            font.pointSize: 6
            wrapMode: Text.Wrap
            verticalAlignment: Text.AlignVCenter
            width: contentFlick.width - 20
            height: implicitHeight
        }
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
