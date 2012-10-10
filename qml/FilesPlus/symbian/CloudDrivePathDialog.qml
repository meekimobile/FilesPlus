import QtQuick 1.1
import com.nokia.symbian 1.1
import CloudDriveModel 1.0
import "Utility.js" as Utility

CommonDialog {
    id: cloudDrivePathDialog

    property string caller
    property int operation
    property string localPath
    property string remotePath
    property string originalRemotePath
    property int selectedCloudType
    property string selectedUid
    property int selectedModelIndex
    property string selectedRemotePath
    property string remoteParentPath
    property int selectedIndex
    property bool selectedIsDir
    property bool selectedIsValid
    property alias contentModel: cloudDrivePathListModel
    property bool isBusy

    signal opening()
    signal opened()
    signal createRemoteFolder(string newRemotePath)
    signal refreshRequested()
    signal deleteRemotePath(string remotePath)

    function proceedOperation(type, uid, localPath, remotePath, modelIndex) {
        console.debug("cloudDrivePathDialog proceedOperation() implementation is required.");
    }

    function getTitleText(localPath, remotePath, remoteParentPath) {
        var text = "";

        switch (operation) {
        case CloudDriveModel.FilePut:
            text += qsTr("Upload %1 into %2").arg(fsModel.getFileName(localPath)).arg(remoteParentPath);
            break;
        case CloudDriveModel.FileGet:
            text += (remotePath == "") ? qsTr("Please select folder") : qsTr("Download %1").arg(fsModel.getFileName(remotePath));
            break;
        case CloudDriveModel.Browse:
            text += qsTr("Connect %1 to %2").arg(fsModel.getFileName(localPath)).arg(remotePath);
            break;
        case CloudDriveModel.Metadata:
            text += qsTr("Sync %1 to %2").arg(fsModel.getFileName(localPath)).arg(remotePath);
            break;
        }

        return text;
    }

    function parseCloudDriveMetadataJson(jsonText) {
        // Reset list view.
        cloudDrivePathListModel.clear();
        cloudDrivePathListView.currentIndex = -1;
        selectedIndex = -1;
        selectedRemotePath = "";
        selectedIsDir = false;
        selectedIsValid = true;
        originalRemotePath = (originalRemotePath == "") ? remotePath : originalRemotePath;

        var json = Utility.createJsonObj(jsonText);
        remoteParentPath = json.path;

        for (var i=0; i<json.contents.length; i++) {
            var item = json.contents[i];
            var modelItem = { "path": item.path, "lastModified": (new Date(item.modified)), "isDir": item.is_dir };
            cloudDrivePathListModel.append(modelItem);
            if (item.path == originalRemotePath) {
                selectedIndex = i;
                selectedRemotePath = item.path;
                selectedIsDir = item.is_dir;
                cloudDrivePathListView.currentIndex = i;
            }
        }

        // Reset busy.
        cloudDrivePathDialog.isBusy = false;
        if (selectedIndex > -1) {
            cloudDrivePathListView.positionViewAtIndex(selectedIndex, ListView.Center);
        }
    }

    function changeRemotePath(remotePath) {
        cloudDrivePathDialog.remotePath = remotePath;
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

    titleIcon: "FilesPlusIcon.svg"
    titleText: appInfo.emptyStr+getTitleText(localPath, selectedRemotePath, remoteParentPath)
    buttonTexts: [appInfo.emptyStr+qsTr("OK"), appInfo.emptyStr+qsTr("Cancel")]
    content: Column {
        width: parent.width
        spacing: 5

        Rectangle {
            id: toolBar
            width: parent.width
            height: 60
            z: 1
            radius: 0
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#808080" }
                GradientStop { position: 1.0; color: "#404040" }
            }

            Row {
                width: parent.width - 10
                anchors.centerIn: parent
                spacing: 5

                Button {
                    id: refreshButton
                    width: 50
                    height: 50
                    visible: !newFolderNameInput.visible
                    iconSource: "refresh.svg"
                    onClicked: {
                        refresh();
                    }
                }

                Button {
                    id: cdUpButton
                    width: 50
                    height: 50
                    visible: !newFolderNameInput.visible
                    iconSource: "back.svg"
                    onClicked: {
                        changeRemotePath(remoteParentPath);
                    }
                }

                Text {
                    id: currentRemotePath
                    visible: !newFolderNameInput.visible
                    width: parent.width - cdUpButton.width - refreshButton.width - newFolderButton.width - (3*parent.spacing)
                    height: 50
                    font.pointSize: 8
                    color: "white"
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    verticalAlignment: Text.AlignVCenter
                    text: remoteParentPath
                }

                TextField {
                    id: newFolderNameInput
                    visible: false
                    width: parent.width - newFolderCancelButton.width - newFolderOkButton.width - (2*parent.spacing)
                    height: 50
                    placeholderText: appInfo.emptyStr+qsTr("New folder name")
                }

                Button {
                    id: newFolderCancelButton
                    width: 50
                    height: 50
                    visible: newFolderNameInput.visible
                    iconSource: "close_stop.svg"
                    onClicked: {
                        newFolderButton.focus = true;
                        newFolderNameInput.text = "";
                        newFolderNameInput.visible = false;
                    }
                }

                Button {
                    id: newFolderOkButton
                    width: 50
                    height: 50
                    visible: newFolderNameInput.visible
                    iconSource: "ok.svg"
                    onClicked: {
                        newFolderButton.focus = true;
                        if (newFolderNameInput.text.trim() != "") {
                            createRemoteFolder(remoteParentPath + "/" + newFolderNameInput.text.trim())
                            newFolderNameInput.visible = false;
                            isBusy = true;
                        }
                    }
                }

                Button {
                    id: newFolderButton
                    width: 50
                    height: 50
                    visible: !newFolderNameInput.visible
                    iconSource: "folder_add.svg"
                    onClicked: {
                        // Show new folder name field.
                        newFolderNameInput.visible = true;
                        newFolderNameInput.text = "";
                    }
                }
            }
        }

        ListView {
            id: cloudDrivePathListView
            width: parent.width
            height: (window.inPortrait ? 300 : 180)
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
            onMovementStarted: {
                if (currentItem) {
                    currentItem.pressed = false;
                }
                currentIndex = -1;
            }

            Rectangle {
                id: busyPanel
                color: "black"
                opacity: 0.5
                visible: isBusy
                anchors.fill: parent

                BusyIndicator {
                    id: busyIcon
                    visible: isBusy
                    running: isBusy
                    width: 100
                    height: 100
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
    }

    Component {
        id: cloudDrivePathItemDelegate

        ListItem {
            id: cloudDrivePathItem
            height: 60
            Row {
                anchors.fill: parent
                spacing: 5

                Rectangle {
                    id: iconRect
                    width: 60
                    height: parent.height
                    color: "transparent"

                    Image {
                        id: icon
                        asynchronous: true
                        width: 48
                        height: 48
                        fillMode: Image.PreserveAspectFit
                        source: cloudDrivePathItem.getIconSource(path, isDir)
                        anchors.centerIn: parent
                    }
                }

                Column {
                    width: parent.width - iconRect.width - (2*parent.spacing) - connectionIcon.width - parent.spacing
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2
                    Text {
                        width: parent.width
                        text: cloudDriveModel.getRemoteName(path)
                        font.pointSize: 8
                        color: "white"
                        elide: Text.ElideMiddle
                    }
                    Text {
                        width: parent.width
                        text: Qt.formatDateTime(lastModified, "d MMM yyyy h:mm:ss ap")
                        font.pointSize: 6
                        color: "lightgray"
                        elide: Text.ElideMiddle
                    }
                }

                Image {
                    id: connectionIcon
                    width: (visible) ? 30 : 0
                    height: parent.height
                    fillMode: Image.PreserveAspectFit
                    z: 1
                    visible: cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, path);
                    source: "cloud.svg"
                }
            }

            function getIconSource(path, isDir) {
                var viewableImageFileTypes = ["JPG", "PNG", "SVG"];
                var viewableTextFileTypes = ["TXT", "HTML"];
                if (isDir) {
                    return "folder_list.svg";
                } else {
                    var dotPos = path.lastIndexOf(".");
                    var fileType = (dotPos > -1) ? path.substr(1+dotPos) : "";
                    if (viewableImageFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                        return "photos_list.svg";
                    } else {
                        return "notes_list.svg";
                    }
                }
            }

            onClicked: {
                if (index == selectedIndex && isDir) {
                    // Second click on current item. Folder only.
                    // Use path with /. to guide onRefreshRequested to get parent path as itself.
                    changeRemotePath(path + "/.");
                } else {
                    // Always set selectedIndex to support changeRemotePath.
                    selectedIndex = index;

                    // FileGet and FilePut can select any items because it's not related to operations.
                    // Other operations must select only the same item type that selected from local path.
                    if (fsModel.isDir(localPath) == isDir || operation == CloudDriveModel.FileGet || operation == CloudDriveModel.FilePut) {
                        selectedRemotePath = path;
                        selectedIsDir = isDir;
                        selectedIsValid = true;
                        console.debug("cloudDrivePathDialog selectedIndex " + selectedIndex + " selectedRemotePath " + selectedRemotePath + " selectedIsDir " + selectedIsDir);
                    } else {
                        selectedRemotePath = "";
                        selectedIsDir = isDir;
                        selectedIsValid = false;
                        cloudDrivePathListView.currentIndex = -1;
                        console.debug("cloudDrivePathDialog invalid selectedIndex " + selectedIndex + " selectedRemotePath " + selectedRemotePath + " selectedIsDir " + selectedIsDir);
                        // TODO Disable OK button.
                    }
                }
            }

            onPressAndHold: {
                var res = cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, path);
                console.debug("cloudDrivePathItem path " + path + " isRemotePathConnected " + res);
                if (!res) {
                    deleteCloudItemConfirmation.remotePath = path;
                    deleteCloudItemConfirmation.open();
                }
            }
        }
    }

    ConfirmDialog {
        id: deleteCloudItemConfirmation

        property string remotePath

        titleText: appInfo.emptyStr+qsTr("Delete")
        contentText: appInfo.emptyStr+qsTr("Delete %1 ?").arg(deleteCloudItemConfirmation.remotePath);
        onConfirm: {
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
