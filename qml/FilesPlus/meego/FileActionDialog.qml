import QtQuick 1.1
import com.nokia.meego 1.0

Dialog {
    id: fileActionDialog
    z: 2
    width: parent.width - 40
    opacity: 0.6
    style: DialogStyle { dim: 0.9 }
    
    property string sourcePath
    property string sourcePathName
    property string targetPath
    property string targetPathName
    property int selectedCloudType: -1
    property string selectedUid
    property string remoteParentPath
    property bool isOverwrite: false
    
    property alias titleText: titleText.text
    property alias titleIcon: titleIcon.source
    property variant buttonTexts: [appInfo.emptyStr+qsTr("OK"), appInfo.emptyStr+qsTr("Cancel")]

    property alias newFileName: fileName.text

    signal opening()
    signal opened()
    signal closing()
    signal closed()
    signal buttonClicked(int index)
    signal confirm()
    signal reject() // Override Dialog.reject() will prevent closing by clicking outside.

    title: Item {
        id: titleRow
        width: parent.width
        height: 60
        Text {
            id: titleText
            color: "white"
            font.pointSize: 20
            elide: Text.ElideRight
            width: parent.width - titleIcon.width
            height: implicitHeight
            anchors.verticalCenter: parent.verticalCenter
            text: appInfo.emptyStr+fileActionDialog.getTitleText()
        }
        Image {
            id: titleIcon
            source: "FilesPlusIcon.svg"
            width: 48
            height: 48
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                // To stop mouse click event to emit reject.
                console.debug("commonDialog title onClicked");
            }
        }
    }
    content: Column {
        id: contentColumn
        anchors.centerIn: parent
        width: parent.width - 10
        spacing: 5
        
        Text {
            text: appInfo.emptyStr+fileActionDialog.getText()
            color: "white"
            width: parent.width
            font.pointSize: 16
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        }
        
        Column {
            width: parent.width
            height: fileActionDialog.isOverwrite ? childrenRect.height : 0
            visible: fileActionDialog.isOverwrite
            spacing: 5
            
            Rectangle {
                color: "grey"
                width: parent.width
                height: 1
            }
            
            Text {
                text: appInfo.emptyStr+qsTr("Item exists, please input new name.")
                color: "white"
                width: parent.width
                font.pointSize: 16
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            }
            
            TextField {
                id: fileName
                width: parent.width
            }
            
            CheckBox {
                id: overwriteFile
                width: parent.width
                text: "<span style='color:white;font:16pt'>" + appInfo.emptyStr+qsTr("Overwrite existing item") + "</span>"
                checked: false
                
                onClicked: {
                    if (checked) {
                        fileName.text = fileActionDialog.sourcePathName;
                    } else {
                        fileName.text = fileActionDialog.getNewName();
                    }
                }
            }
        }
        
        onHeightChanged: {
            console.debug("cloudFolderPage contentColumn onHeightChanged " + height + " fileActionDialog.height " + fileActionDialog.height);
        }
    }
    buttons: Row {
        id: buttonRow
        width: parent.width - 20
        height: 80
        spacing: 5
        anchors.horizontalCenter: parent.horizontalCenter

        property int buttonWidth: (width / buttonTexts.length) - spacing

        Repeater {
            model: buttonTexts
            Button {
                text: modelData
                width: buttonRow.buttonWidth
                anchors.verticalCenter: parent.verticalCenter
                onClicked: {
                    buttonClicked(index);
                    close();
                }
            }
        }
    }

    function getTitleText() {
        var text = "";
        if (clipboard.count == 1) {
            text = getActionName(clipboard.get(0).action, clipboard.get(0).type, clipboard.get(0).uid);
        } else {
            // TODO if all copy, show "Multiple copy".
            text = qsTr("Multiple actions");
        }
        
        return text;
    }
    
    function getText() {
        // Example of clipboard entry { "action": "cut", "sourcePath": sourcePath }
        // Example of clipboard entry { "action": "cut", "type": "Dropbox", "uid": "asdfdg", "sourcePath": sourcePath, "sourcePathName": sourcePathName }
        // Example of clipboard entry { "action": "extract", "sourcePath": compressedFilePath, "extractFileList": extractFileList }
        var text = "";
        if (clipboard.count == 1) {
            // Add action name.
            text = getActionName(clipboard.get(0).action, clipboard.get(0).type, clipboard.get(0).uid);
            // Add cloud source.
            if (clipboard.get(0).type) {
                text = text + ((cloudDriveModel.getClientType(clipboard.get(0).type) != selectedCloudType || clipboard.get(0).uid != selectedUid) ? (" (" + clipboard.get(0).type + " " + clipboard.get(0).uid + ")") : "");
            }
            // Add content.
            text = text + " "
                    + (clipboard.get(0).sourcePathName ? clipboard.get(0).sourcePathName : clipboard.get(0).sourcePath)
                    + ((clipboard.get(0).action == "extract") ? (" (" + qsTr("%n item(s)", "", clipboard.get(0).extractFileList.length) + ")") : "")
                    + ((clipboard.get(0).action == "delete") ? "" : ("\n" + qsTr("to") + " " + targetPathName))
                    + " ?";
        } else {
            console.debug("fileActionDialog getText selectedCloudType " + selectedCloudType + " " + cloudDriveModel.getCloudName(selectedCloudType) + " " + selectedUid);
            var cutCount = 0;
            var copyCount = 0;
            var deleteCount = 0;
            var extractCount = 0;
            var uploadCount = 0;
            var downloadCount = 0;
            var migrateCount = 0;
            for (var i=0; i<clipboard.count; i++) {
                console.debug("fileActionDialog getText clipboard i " + i + " type " + clipboard.get(i).type + " uid " + clipboard.get(i).uid + " action " + clipboard.get(i).action + " sourcePath " + clipboard.get(i).sourcePath);
                if (selectedCloudType != -1) {
                    // selectedCloudType and selectedUid are defined. Parent is CloudFolderPage.
                    if (["copy","cut"].indexOf(clipboard.get(i).action) > -1 && !clipboard.get(i).type) {
                        uploadCount++;
                    } else if (["copy","cut"].indexOf(clipboard.get(i).action) > -1 && (cloudDriveModel.getClientType(clipboard.get(i).type) != selectedCloudType || clipboard.get(i).uid != selectedUid) ) {
                        migrateCount++;
                    } else if (clipboard.get(i).action == "copy") {
                        copyCount++;
                    } else if (clipboard.get(i).action == "cut") {
                        cutCount++;
                    } else if (clipboard.get(i).action == "delete") {
                        deleteCount++;
                    }
                } else {
                    // selectedCloudType or selectedUid is undefined. Parent is FolderPage.
                    if (["copy","cut"].indexOf(clipboard.get(i).action) > -1 && clipboard.get(i).type) {
                        downloadCount++;
                    } else if (clipboard.get(i).action == "copy") {
                        copyCount++;
                    } else if (clipboard.get(i).action == "cut") {
                        cutCount++;
                    } else if (clipboard.get(i).action == "delete") {
                        deleteCount++;
                    } else if (clipboard.get(i).action == "extract") {
                        extractCount++;
                    }
                }
            }
            
            if (uploadCount>0) text = text + qsTr("Upload %n item(s)", "", uploadCount) + "\n";
            if (downloadCount>0) text = text + qsTr("Download %n item(s)", "", downloadCount) + "\n";
            if (migrateCount>0) text = text + qsTr("Migrate %n item(s)", "", migrateCount) + "\n";
            if (deleteCount>0) text = text + qsTr("Delete %n item(s)", "", deleteCount) + "\n";
            if (extractCount>0) text = text + qsTr("Extract %n item(s)", "", extractCount) + "\n";
            if (copyCount>0) text = text + qsTr("Copy %n item(s)", "", copyCount) + "\n";
            if (cutCount>0) text = text + qsTr("Move %n item(s)", "", cutCount) + "\n";
            if (uploadCount+downloadCount+migrateCount+copyCount+cutCount+extractCount > 0) text = text + qsTr("to") + " " + targetPathName;
            text = text + " ?";
        }
        
        return text;
    }
    
    function getActionName(actionText, type, uid) {
        console.debug("fileActionDialog getActionName selectedCloudType " + selectedCloudType + " " + cloudDriveModel.getCloudName(selectedCloudType) + " " + selectedUid + " actionText " + actionText + " type " + cloudDriveModel.getClientType(type) + " " + type + " uid " + uid);
        if (selectedCloudType != -1) {
            // selectedCloudType and selectedUid are defined. Parent is CloudFolderPage.
            if (type && type != -1) {
                if (cloudDriveModel.getClientType(type) == selectedCloudType && uid == selectedUid) {
                    if (actionText == "copy") return qsTr("Copy");
                    else if (actionText == "cut") return qsTr("Move");
                    else if (actionText == "delete") return qsTr("Delete");
                    else return qsTr("Invalid action");
                } else {
                    if (["copy","cut"].indexOf(actionText) > -1) return qsTr("Migrate");
                    else return qsTr("Invalid action");
                }
            } else {
                if (["copy","cut"].indexOf(actionText) > -1) return qsTr("Upload");
                else return qsTr("Invalid action");
            }
        } else {
            // selectedCloudType or selectedUid is undefined. Parent is FolderPage.
            if (type && type != -1) {
                if (["copy","cut"].indexOf(actionText) > -1) return qsTr("Download");
                else return qsTr("Invalid action");
            } else {
                if (actionText == "copy") return qsTr("Copy");
                else if (actionText == "cut") return qsTr("Move");
                else if (actionText == "delete") return qsTr("Delete");
                else if (actionText == "extract") return qsTr("Extract");
                else return qsTr("Invalid action");
            }
        }
    }
    
    function canCopy() {
        if (selectedCloudType != -1) {
            // selectedCloudType and selectedUid are defined. Parent is CloudFolderPage.
            var foundIndex = -1;
            var sourcePathName = (clipboard.get(0).sourcePathName) ? clipboard.get(0).sourcePathName : cloudDriveModel.getFileName(clipboard.get(0).sourcePath);
            if (targetPath == remoteParentPath) {
                foundIndex = cloudDriveModel.findIndexByRemotePathName(sourcePathName);
            }

            console.debug("fileActionDialog canCopy " + sourcePathName + " foundIndex " + foundIndex);
            return (foundIndex < 0);
        } else {
            // selectedCloudType or selectedUid is undefined. Parent is FolderPage.
            return fsModel.canCopy(fsModel.getAbsolutePath(targetPath, fileActionDialog.sourcePathName), targetPath);
        }
    }
    
    function getNewName() {
        if (selectedCloudType != -1) {
            // selectedCloudType and selectedUid are defined. Parent is CloudFolderPage.
            return cloudDriveModel.getNewFileName(fileActionDialog.sourcePathName);
        } else {
            // selectedCloudType or selectedUid is undefined. Parent is FolderPage.
            return fsModel.getNewFileName(fileActionDialog.sourcePathName, fileActionDialog.targetPath);
        }
    }
    
    onOpening: {
        if (clipboard.count == 1 && clipboard.get(0).action != "delete") {
            // Copy/Move/Delete first file from clipboard.
            // Check if there is existing file on target folder. Then show overwrite dialog.
            fileActionDialog.sourcePath = clipboard.get(0).sourcePath;
            fileActionDialog.sourcePathName = (clipboard.get(0).sourcePathName) ? clipboard.get(0).sourcePathName : cloudDriveModel.getFileName(clipboard.get(0).sourcePath);
            if (!canCopy() && clipboard.get(0).action != "extract") {
                fileActionDialog.isOverwrite = true;
                fileName.forceActiveFocus();
                fileName.text = getNewName();
                overwriteFile.checked = false;
            } else {
                fileActionDialog.isOverwrite = false;
            }
        } else {
            fileActionDialog.isOverwrite = false;
        }

        console.debug("cloudFolderPage fileActionDialog onOpening fileActionDialog.height " + fileActionDialog.height + " contentColumn.height " + contentColumn.height + " contentColumn.implicitHeight " + contentColumn.implicitHeight);
    }
    
    onButtonClicked: {
        if (index == 0) {
            confirm();
            accept();
        } else {
            reject();
        }
    }
    // REMARK Avoid override to prevent click outside as reject.
    onStatusChanged: {
        if (status == DialogStatus.Opening) {
            opening();
        } else if (status == DialogStatus.Open) {
            opened();
        } else if (status == DialogStatus.Closing) {
            closing();
        } else if (status == DialogStatus.Closed) {
            closed();
        }
    }
}
