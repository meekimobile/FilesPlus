import QtQuick 1.1
import com.nokia.meego 1.0
import CloudDriveModel 1.0
import "Utility.js" as Utility

ConfirmDialog {
    id: cloudDrivePathDialog
    width: parent.width

    property string caller
    property int operation
    property string localPath
    property string remotePath // Specified remote path
    property string originalRemotePath
    property int selectedCloudType
    property string selectedUid
    property int selectedModelIndex
    property string selectedRemotePath
    property string selectedRemotePathName
    property string remoteParentPath // Current browse remote path
    property string remoteParentPathName // Shows on titlebar
    property string remoteParentParentPath // For change up
    property int selectedIndex: -1
    property bool selectedIsDir
    property bool selectedIsValid
    property bool isBusy

    signal opening()
    signal opened()
    signal createRemoteFolder(string newRemoteFolderName)
    signal refreshRequested(string remoteParentPath)
    signal deleteRemotePath(string remotePath)

    function proceedOperation(type, uid, localPath, remotePath, modelIndex) {
        console.debug("cloudDrivePathDialog proceedOperation() implementation is required.");
    }

    function getTitleText(localPath, remotePathName, remoteParentPathName) {
        var text = "";

        switch (operation) {
        case CloudDriveModel.FilePut:
            text += qsTr("Upload %1 into %2").arg(fsModel.getFileName(localPath)).arg(remoteParentPathName);
            break;
        case CloudDriveModel.FileGet:
            text += (remotePathName == "") ? qsTr("Please select folder") : qsTr("Download %1").arg(remotePathName);
            break;
        case CloudDriveModel.Browse:
            text += qsTr("Connect %1 to %2").arg(fsModel.getFileName(localPath)).arg(remotePathName);
            break;
        case CloudDriveModel.Metadata:
            if (localPath != "") {
                text += qsTr("Sync %1 to %2").arg(fsModel.getFileName(localPath)).arg(remotePathName);
            } else if (clipboard.count > 0) {
                var syncCount = clipboard.countByAction("sync");
                text += qsTr("Sync %n item(s) to", "disambiguation", syncCount);
            }
            break;
        }

        return text;
    }

    function postBrowseReplySlot() {
        console.debug("cloudDrivePathDialog postBrowseReplySlot selectedCloudType " + selectedCloudType + " selectedUid " + selectedUid + " cloudDriveModel.remoteParentPath " + cloudDriveModel.remoteParentPath + " cloudDriveModel.remoteParentParentPath " + cloudDriveModel.remoteParentParentPath);
        remoteParentPath = cloudDriveModel.remoteParentPath;
        remoteParentPathName = cloudDriveModel.remoteParentPathName;
        remoteParentParentPath = cloudDriveModel.remoteParentParentPath;

        originalRemotePath = (originalRemotePath == "") ? remotePath : originalRemotePath;

        selectedIsValid = true;
        selectedIndex = cloudDriveModel.findIndexByRemotePath(originalRemotePath);
        selectedRemotePath = ""; // Requires to reset otherwise it will incorrectly select its parent path for syncing.
        selectedRemotePathName = "";
        selectedIsDir = false;
        if (selectedIndex > -1) {
            var selectedItem = cloudDriveModel.get(selectedIndex);
            selectedRemotePath = selectedItem.absolutePath;
            selectedRemotePathName = selectedItem.name;
            selectedIsDir = selectedItem.isDir;
        }
        cloudDrivePathListView.currentIndex = selectedIndex;

        // Reset busy.
        cloudDrivePathDialog.isBusy = false;
        if (selectedIndex > -1) {
            cloudDrivePathListView.positionViewAtIndex(selectedIndex, ListView.Contain); // Issue: ListView.Center will cause list item displayed out of list view in Meego only.
        }
    }

    function changeRemotePath(remoteParentPath) {
        console.debug("cloudDrivePathDialog changeRemotePath " + remoteParentPath);
        cloudDrivePathDialog.isBusy = true;
        refreshRequested(remoteParentPath);
    }

    function refresh() {
        cloudDrivePathDialog.isBusy = true;
        refreshRequested(cloudDriveModel.remoteParentPath);
    }

    function refreshItem(remotePath) {
        var timestamp = (new Date()).getTime();
        var i = cloudDriveModel.findIndexByRemotePath(remotePath);
        console.debug("cloudDrivePathDialog refreshItem i " + i);
        if (i >= 0) {
            cloudDriveModel.setProperty(i, "timestamp", timestamp);
        }
    }

    titleText: appInfo.emptyStr+getTitleText(localPath, selectedRemotePathName, remoteParentPathName)
    content: Item {
        width: parent.width
        height: childrenRect.height

        Rectangle {
            id: toolBar
            width: parent.width
            height: 70
            z: 1
            radius: 0
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#000000" }
                GradientStop { position: 0.05; color: "#000000" }
                GradientStop { position: 0.05; color: "#808080" }
                GradientStop { position: 1.0; color: "#404040" }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    // DO nothing
                }
            }

            Row {
                width: parent.width - 10
                anchors.centerIn: parent
                spacing: 5

                Button {
                    id: refreshButton
                    width: 60
                    height: 60
                    visible: !newFolderNameInput.visible
                    iconSource: (theme.inverted ? "refresh.svg" : "refresh_inverted.svg")
                    onClicked: {
                        refresh();
                    }
                }

                Button {
                    id: cdUpButton
                    width: 60
                    height: 60
                    visible: !newFolderNameInput.visible
                    iconSource: (theme.inverted ? "back.svg" : "back_inverted.svg")
                    onClicked: {
                        changeRemotePath(remoteParentParentPath);
                    }
                }

                Text {
                    id: currentRemotePath
                    visible: !newFolderNameInput.visible
                    width: parent.width - cdUpButton.width - refreshButton.width - newFolderButton.width - (3*parent.spacing)
                    height: 60
                    font.pointSize: 18
                    color: "white"
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    verticalAlignment: Text.AlignVCenter
                    text: remoteParentPathName

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (appInfo.getSettingBoolValue("CloudDriveModel.metadata.root.connection.enabled", false)) {
                                console.debug("cloudDrivePathDialog currentRemotePath onClicked cloudDriveModel.remoteParentPath " + cloudDriveModel.remoteParentPath + " cloudDriveModel.remoteParentPathName " + cloudDriveModel.remoteParentPathName);
                                selectedRemotePath = cloudDriveModel.remoteParentPath;
                                selectedRemotePathName = cloudDriveModel.remoteParentPathName;
                                selectedIndex = -1;
                                cloudDrivePathListView.currentIndex = -1;
                            }
                        }
                    }
                }

                TextField {
                    id: newFolderNameInput
                    visible: false
                    width: parent.width - newFolderCancelButton.width - newFolderOkButton.width - (2*parent.spacing)
                    height: 60
                    placeholderText: appInfo.emptyStr+qsTr("New folder name")
                }

                Button {
                    id: newFolderCancelButton
                    width: 60
                    height: 60
                    visible: newFolderNameInput.visible
                    iconSource: (theme.inverted ? "close_stop.svg" : "close_stop_inverted.svg")
                    onClicked: {
                        newFolderButton.focus = true;
                        newFolderNameInput.text = "";
                        newFolderNameInput.visible = false;
                    }
                }

                Button {
                    id: newFolderOkButton
                    width: 60
                    height: 60
                    visible: newFolderNameInput.visible
                    iconSource: (theme.inverted ? "ok.svg" : "ok_inverted.svg")
                    onClicked: {
                        newFolderButton.focus = true;
                        if (newFolderNameInput.text.trim() != "") {
                            isBusy = true;
                            newFolderNameInput.visible = false;
                            createRemoteFolder(newFolderNameInput.text.trim())
                        }
                    }
                }

                Button {
                    id: newFolderButton
                    width: 60
                    height: 60
                    visible: !newFolderNameInput.visible
                    iconSource: (theme.inverted ? "folder_add.svg" : "folder_add_inverted.svg")
                    onClicked: {
                        // Show new folder name field.
//                        newFolderNameInput.visible = true;
//                        newFolderNameInput.text = "";
                        newFolderDialog.open();
                    }
                }
            }
        }

        ListView {
            id: cloudDrivePathListView
            width: parent.width
            height: (window.inPortrait ? 560 : 240) // 7 items for portrait, 3 items for landscape.
            model: cloudDriveModel
            delegate: cloudDrivePathItemDelegate
            highlight: Rectangle {
                width: cloudDrivePathListView.width
                gradient: Gradient {
                    GradientStop {
                        position: 0
                        color: "#0080D0"
                    }

                    GradientStop {
                        position: 1
                        color: "#53A3E6"
                    }
                }
            }
            highlightFollowsCurrentItem: true
            highlightMoveSpeed: 2000
            pressDelay: 100
            clip: true
            anchors.top: toolBar.bottom
            anchors.topMargin: 5
            onMovementStarted: {
                if (currentItem) {
                    currentItem.pressed = false;
                }
                currentIndex = -1;
            }

            Rectangle {
                id: busyPanel
                color: "black"
                opacity: 0.7
                visible: isBusy
                anchors.fill: parent
                z: 2

                BusyIndicator {
                    id: busyIcon
                    visible: isBusy
                    running: isBusy
                    platformStyle: BusyIndicatorStyle { size: "large"; __invertedString: "inverted" } // Force black mode for Meego only.
                    anchors.centerIn: parent
                }

                MouseArea {
                    enabled: isBusy
                    anchors.fill: parent
                    onPressed: {
                        // do nothing.
                    }
                }
            }
        }

        ScrollDecorator {
            id: scrollBar
            flickableItem: cloudDrivePathListView
        }
    }

    Component {
        id: cloudDrivePathItemDelegate

        CloudListItem {
            id: listItem
            listViewState: (cloudDrivePathListView.state ? cloudDrivePathListView.state : "")
            syncIconVisible: isConnected
            syncIconSource: "cloud.svg"
            actionIconSource: (clipboard.count > 0) ? appInfo.emptySetting+clipboard.getActionIcon(absolutePath, cloudDriveModel.getCloudName(selectedCloudType), selectedUid) : ""
            inverted: false
            omitShowingZeroSizeDir: true
            isImageUrlCachable: cloudDriveModel.isImageUrlCachable(selectedCloudType)

            // Override to support cloud items.
            function getIconSource(timestamp) {
                var viewableImageFileTypes = ["JPG", "PNG", "SVG"];
                var viewableTextFileTypes = ["TXT", "HTML"];

                if (isDir) {
                    return "folder_list.svg";
                } else if (viewableImageFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                    var showThumbnail = appInfo.getSettingBoolValue("CloudFolderPage.thumbnail.enabled", false);
                    if (showThumbnail && thumbnail && thumbnail != "") {
                        // Dropbox's thumbnail url can't open with Image directly. Need to invoke cacheImage() or use RemoteImageProvider to download.
                        // Always cache thumbnail by using RemoteImageProvider.
                        if (cloudDriveModel.isImageUrlCachable(selectedCloudType) && fileType.toUpperCase() != "SVG") {
                            return "image://remote/" + thumbnail + "#t=" + timestamp;
                        } else {
                            return thumbnail;
                        }
                    } else {
                        return "photos_list.svg";
                    }
                } else {
                    return "notes_list.svg";
                }
            }

            onClicked: { // Switch type
                if (index == selectedIndex && isDir) {
                    // Second click on current item. Folder only.
                    changeRemotePath(absolutePath);
                } else {
                    // Always set selectedIndex to support changeRemotePath.
                    selectedIndex = index;

                    // FileGet and FilePut can select any items because it's not related to operations.
                    // Other operations must select only the same item type that selected from local path.
                    if ((localPath != "" && fsModel.isDir(localPath) == isDir) || operation == CloudDriveModel.FileGet || operation == CloudDriveModel.FilePut) {
                        selectedRemotePath = absolutePath;
                        selectedRemotePathName = name;
                        selectedIsDir = isDir;
                        selectedIsValid = true;
                        console.debug("cloudDrivePathDialog valid selectedIndex " + selectedIndex + " selectedRemotePath " + selectedRemotePath + " selectedRemotePathName " + selectedRemotePathName + " selectedIsDir " + selectedIsDir);
                    } else if (clipboard.count > 0) {
                        selectedRemotePath = "";
                        selectedRemotePathName = "";
                        selectedIsDir = isDir;
                        selectedIsValid = true;
                        cloudDrivePathListView.currentIndex = -1;
                        console.debug("cloudDrivePathDialog valid clipboard.count " + clipboard.count + " selectedIndex " + selectedIndex + " selectedRemotePath " + selectedRemotePath + " selectedRemotePathName " + selectedRemotePath + " selectedIsDir " + selectedIsDir);
                    } else {
                        selectedRemotePath = "";
                        selectedRemotePathName = "";
                        selectedIsDir = isDir;
                        selectedIsValid = false;
                        cloudDrivePathListView.currentIndex = -1;
                        console.debug("cloudDrivePathDialog invalid selectedIndex " + selectedIndex + " selectedRemotePath " + selectedRemotePath + " selectedRemotePathName " + selectedRemotePath + " selectedIsDir " + selectedIsDir);
                        // TODO Disable OK button.
                    }
                }
            }

            onPressAndHold: {
                var res = cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, absolutePath);
                console.debug("cloudDrivePathItem absolutePath " + absolutePath + " isRemotePathConnected " + res);
                if (!res) {
                    cloudDrivePathListView.currentIndex = -1;
                    deleteCloudItemConfirmation.remotePath = absolutePath;
                    deleteCloudItemConfirmation.remotePathName = name;
                    deleteCloudItemConfirmation.open();
                }
            }

            onListItemIconError: {
                if (cloudDriveModel.isImageUrlCachable(selectedCloudType)) {
                    cacheImageHelper.cacheImage(absolutePath, thumbnail, 48, 48, "folderPage"); // Use default icon size.
                }
            }
        }
    }

    ConfirmDialog {
        id: newFolderDialog
        titleText: appInfo.emptyStr+qsTr("New folder")
        content: Item {
            width: parent.width
            height: 80

            TextField {
                id: folderName
                width: parent.width
                placeholderText: appInfo.emptyStr+qsTr("Please input folder name.")
                anchors.centerIn: parent
            }
        }

        onOpened: {
            folderName.text = "";
        }

        onConfirm: {
            if (folderName.text !== "") {
                isBusy = true;
                createRemoteFolder(folderName.text.trim());
            }
        }
    }

    ConfirmDialog {
        id: deleteCloudItemConfirmation

        property string remotePath
        property string remotePathName

        titleText: appInfo.emptyStr+qsTr("Delete")
        contentText: appInfo.emptyStr+qsTr("Delete %1 ?").arg(deleteCloudItemConfirmation.remotePathName);
        onConfirm: {
            isBusy = true;
            deleteRemotePath(deleteCloudItemConfirmation.remotePath);
        }
    }

    onStatusChanged: {
        if (status == DialogStatus.Opening) {
            cloudDrivePathDialog.isBusy = true;
            cloudDrivePathDialog.originalRemotePath = "";
            cloudDrivePathDialog.selectedRemotePath = "";
            cloudDrivePathDialog.selectedRemotePathName = "";
            newFolderNameInput.focus = false;
            newFolderNameInput.visible = false;
            opening();
        } else if (status == DialogStatus.Open) {
            opened();
        }
    }
}
