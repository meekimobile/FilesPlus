import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

ConfirmDialog {
    id: fileAttributesDialog

    property string srcFilePath

    signal confirm(bool attribute1)

    titleText: appInfo.emptyStr+qsTr("%1 info").arg(fsModel.getFileName(srcFilePath))
//    titleIcon: "FilesPlusIcon.svg"
//    buttonTexts: [appInfo.emptyStr+qsTr("OK"), appInfo.emptyStr+qsTr("Cancel")]
    content: Column {
        width: parent.width - 10
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 5
        
        Text {
            id: fileInfo
            text: "File info"
        }

        Row {
            width: parent.width
            spacing: 5

            CheckBox {
                id: attribute1
                text: appInfo.emptyStr+qsTr("Attribute 1")
            }

            CheckBox {
                id: attribute2
                text: appInfo.emptyStr+qsTr("Attribute 2")
            }
        }
    }
}
