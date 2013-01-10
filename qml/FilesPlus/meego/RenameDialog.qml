import QtQuick 1.1
import com.nokia.meego 1.0
import "Utility.js" as Utility

ConfirmDialog {
    id: renameDialog
    
    property string sourcePath
    property string sourcePathName
    property alias nameField: newName

    titleText: appInfo.emptyStr+qsTr("Rename")
    content: Column {
        width: parent.width - 10
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 5
        
        Text {
            width: parent.width
            text: appInfo.emptyStr+qsTr("Rename %1 to").arg(sourcePathName);
            color: "white"
            font.pointSize: 16
            elide: Text.ElideMiddle
        }
        
        TextField {
            id: newName
            width: parent.width
            placeholderText: appInfo.emptyStr+qsTr("Please input new name.")
            text: sourcePathName
        }
    }
    
    onOpened: {
        newName.text = sourcePathName;
        newName.forceActiveFocus();
    }
    
    onClosed: {
        newName.text = "";
    }
}
