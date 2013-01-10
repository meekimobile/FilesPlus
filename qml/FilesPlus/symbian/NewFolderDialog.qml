import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

ConfirmDialog {
    id: newFolderDialog
    height: contentColumn.height + 120 // Workaround for Symbian only.

    property alias nameField: folderName
    property bool folderOnly: false
    property alias isNewFolder: newFolderButton.checked

    titleText: appInfo.emptyStr+qsTr("New folder / file")
    content: Column {
        id: contentColumn
        width: parent.width - 20
        anchors.centerIn: parent // Workaround for Symbian only.
        spacing: 5
        
        Row {
            visible: !folderOnly
            width: parent.width
            spacing: 10
            RadioButton {
                id: newFolderButton
                width: (parent.width - parent.spacing) / 2
                text: appInfo.emptyStr+qsTr("Folder")
                onClicked: {
                    newFileButton.checked = false;
                }
            }
            RadioButton {
                id: newFileButton
                width: (parent.width - parent.spacing) / 2
                text: appInfo.emptyStr+qsTr("File")
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
        folderName.forceActiveFocus();
    }
    
    onClosed: {
        folderName.text = "";
    }
}
