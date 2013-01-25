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
    property int selectedIndex
    property bool selectedIsDir
    property bool selectedIsValid
    property alias contentModel: cloudDrivePathListModel
    property bool isBusy

    signal opening()
    signal opened()
    signal createRemoteFolder(string newRemoteFolderName)
    signal refreshRequested()
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
            text += qsTr("Sync %1 to %2").arg(fsModel.getFileName(localPath)).arg(remotePathName);
            break;
        }

        return text;
    }

    function parseCloudDriveMetadataJson(jsonText) {
        originalRemotePath = (originalRemotePath == "") ? remotePath : originalRemotePath;

        cloudDrivePathListModel.clear();
        var responseJson = cloudDriveModel.parseCloudDriveMetadataJson(selectedCloudType, selectedUid, originalRemotePath, jsonText,  cloudDrivePathListModel);
        console.debug("cloudDrivePathDialog parseCloudDriveMetadataJson responseJson " + JSON.stringify(responseJson));

        selectedIsValid = true;
        selectedIndex = responseJson.selectedIndex;
        selectedRemotePath = responseJson.selectedRemotePath;
        selectedRemotePathName = responseJson.selectedRemotePathName;
        selectedIsDir = responseJson.selectedIsDir;
        remoteParentPath = responseJson.remoteParentPath;
        remoteParentPathName = responseJson.remoteParentPathName;
        remoteParentParentPath = responseJson.remoteParentParentPath;

        cloudDrivePathListView.currentIndex = responseJson.selectedIndex;

        // Reset busy.
        cloudDrivePathDialog.isBusy = false;
        if (selectedIndex > -1) {
            cloudDrivePathListView.positionViewAtIndex(selectedIndex, ListView.Contain); // Issue: ListView.Center will cause list item displayed out of list view in Meego only.
        }
    }

    function changeRemotePath(remoteParentPath) {
        console.debug("cloudDrivePathDialog changeRemotePath " + remoteParentPath);
        cloudDrivePathDialog.remoteParentPath = remoteParentPath;
        refresh();
    }

    function refresh() {
        cloudDrivePathDialog.selectedIndex = -1;
        cloudDrivePathDialog.isBusy = true;
        refreshRequested();
    }

    function getSelectedRemotePath() {
        var selectedRemotePath = "";

        if (cloudDrivePathListView.currentIndex > -1) {
            selectedRemotePath = cloudDrivePathListModel.get(cloudDrivePathListView.currentIndex).path;
        }

        return selectedRemotePath;
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
                    text: cloudDriveModel.isRemoteAbsolutePath(selectedCloudType) ? remoteParentPath : remoteParentPathName
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
            model: ListModel {
                id: cloudDrivePathListModel
            }
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
            syncIconVisible: cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, absolutePath)
            syncIconSource: "cloud.svg"
            actionIconSource: (clipboard.count > 0) ? appInfo.emptySetting+clipboard.getActionIcon(absolutePath, cloudDriveModel.getCloudName(selectedCloudType), selectedUid) : ""
            inverted: false

            // Override to support cloud items.
            function getIconSource() {
                var viewableImageFileTypes = ["JPG", "PNG", "SVG"];
                var viewableTextFileTypes = ["TXT", "HTML"];

                if (isDir) {
                    return "folder_list.svg";
                } else if (viewableImageFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                    var showThumbnail = appInfo.getSettingBoolValue("thumbnail.enabled", false);
                    if (showThumbnail && thumbnail && thumbnail != "") {
                        return thumbnail;
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
                    if (fsModel.isDir(localPath) == isDir || operation == CloudDriveModel.FileGet || operation == CloudDriveModel.FilePut) {
                        selectedRemotePath = absolutePath;
                        selectedRemotePathName = name;
                        selectedIsDir = isDir;
                        selectedIsValid = true;
                        console.debug("cloudDrivePathDialog selectedIndex " + selectedIndex + " selectedRemotePath " + selectedRemotePath + " selectedRemotePathName " + selectedRemotePathName + " selectedIsDir " + selectedIsDir);
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
                    deleteCloudItemConfirmation.remotePath = absolutePath;
                    deleteCloudItemConfirmation.remotePathName = name;
                    deleteCloudItemConfirmation.open();
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
            newFolderNameInput.focus = false;
            newFolderNameInput.visible = false;
            opening();
        } else if (status == DialogStatus.Open) {
            opened();
        }
    }
}
