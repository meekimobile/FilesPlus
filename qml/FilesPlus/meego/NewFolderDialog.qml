import QtQuick 1.1
import com.nokia.meego 1.0
import "Utility.js" as Utility

ConfirmDialog {
    id: newFolderDialog

    property alias nameField: folderName
    property bool folderOnly: false
    property alias isNewFolder: newFolderButton.checked

    titleText: appInfo.emptyStr+qsTr("New folder / file")
    content: Column {
        id: contentColumn
        width: parent.width - 10
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 5
        
        Row {
            visible: !folderOnly
            width: parent.width
            spacing: 10
            RadioButton {
                id: newFolderButton
                width: (parent.width - parent.spacing) / 2
                text: "<font color='white'>" + appInfo.emptyStr+qsTr("Folder") + "</font>"
                onClicked: {
                    newFileButton.checked = false;
                }
            }
            RadioButton {
                id: newFileButton
                width: (parent.width - parent.spacing) / 2
                text: "<font color='white'>" + appInfo.emptyStr+qsTr("File") + "</font>"
                onClicked: {
                    newFolderButton.checked = false;
                }
            }
        }
        
        TextField {
            id: folderName
            width: parent.width
            placeholderText: appInfo.emptyStr+(newFolderButton.checked ? qsTr("Please input folder name.") : qsTr("Please input file name."))
        }
    }
    
    onOpened: {
        newFolderButton.checked = true;
        newFileButton.checked = false;
        folderName.text = "";
    }
    
    onClosed: {
        folderName.text = "";
    }
}
