import QtQuick 1.1
import com.nokia.symbian 1.1

CommonDialog {
    id: confirmDialog

    property alias contentText: content.text

    z: 2
    titleIcon: "FilesPlusIcon.svg"
    buttonTexts: ["OK", "Cancel"]
    content: Text {
        id: content
        color: "white"
        font.pixelSize: 18
        wrapMode: Text.Wrap
        width: parent.width - 8
        height: implicitHeight
        anchors.horizontalCenter: parent.horizontalCenter
    }
    
    signal confirm()
    
    onButtonClicked: {
        if (index === 0) confirm();
    }
}
