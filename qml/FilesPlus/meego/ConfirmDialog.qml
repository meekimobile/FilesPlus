import QtQuick 1.1
import com.nokia.meego 1.1

CommonDialog {
    id: confirmDialog

    property alias contentText: content.text

    z: 2
    titleIcon: "FilesPlusIcon.svg"
    buttonTexts: ["OK", "Cancel"]
    content: Text {
        id: content
        color: "white"
        font.pointSize: 6
        wrapMode: Text.Wrap
        width: parent.width - 20
        height: implicitHeight
        anchors.horizontalCenter: parent.horizontalCenter
    }
    
    signal confirm()
    signal closed()
    signal opening()
    
    onButtonClicked: {
        if (index === 0) confirm();
    }

    onStatusChanged: {
        if (status == DialogStatus.Opening) {
            opening();
        } else if (status == DialogStatus.Closed) {
            closed();
        }
    }
}
