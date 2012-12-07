import QtQuick 1.1
import com.nokia.meego 1.0
import CloudDriveModel 1.0
import "Utility.js" as Utility

CommonDialog {
    id: cloudDrivePathDialog
    width: parent.width * 0.9

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
        // Reset list view.
        cloudDrivePathListModel.clear();
        cloudDrivePathListView.currentIndex = -1;
        selectedIndex = -1;
        selectedRemotePath = "";
        selectedRemotePathName = "";
        selectedIsDir = false;
        selectedIsValid = true;
        originalRemotePath = (originalRemotePath == "") ? remotePath : originalRemotePath;

        var json = Utility.createJsonObj(jsonText);

        switch (selectedCloudType) {
        case CloudDriveModel.Dropbox:
            remoteParentPath = json.path;
            remoteParentPathName = json.path;
            remoteParentParentPath = cloudDriveModel.getRemoteParentPath(remoteParentPath);
            for (var i=0; i<json.contents.length; i++) {
                var item = json.contents[i];
                var modelItem = { "name": cloudDriveModel.getRemoteName(item.path), "path": item.path, "lastModified": (new Date(item.modified)), "size": item.bytes, "isDir": item.is_dir };
                cloudDrivePathListModel.append(modelItem);
                if (item.path == originalRemotePath) {
                    selectedIndex = i;
                    selectedRemotePath = modelItem.path;
                    selectedRemotePathName = modelItem.name;
                    selectedIsDir = modelItem.isDir;
                    cloudDrivePathListView.currentIndex = i;
                }
            }
            break;
        case CloudDriveModel.SkyDrive:
            remoteParentPath = json.property.id;
            remoteParentPathName = json.property.name;
            remoteParentParentPath = (json.property.parent_id ? json.property.parent_id : "");
            for (var i=0; i<json.data.length; i++) {
                var item = json.data[i];
                var modelItem = { "name": item.name, "path": item.id, "lastModified": Utility.parseJSONDate(item.updated_time), "size": item.size, "isDir": (item.type == "folder" || item.type == "album") };
                cloudDrivePathListModel.append(modelItem);
                if (item.id == originalRemotePath) {
                    selectedIndex = i;
                    selectedRemotePath = modelItem.path;
                    selectedRemotePathName = modelItem.name;
                    selectedIsDir = modelItem.isDir;
                    cloudDrivePathListView.currentIndex = i;
                }
            }
            break;
        }

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

    titleIcon: "FilesPlusIcon.svg"
    titleText: appInfo.emptyStr+getTitleText(localPath, selectedRemotePathName, remoteParentPathName)
    buttonTexts: [appInfo.emptyStr+qsTr("OK"), appInfo.emptyStr+qsTr("Cancel")]
    content: Column {
        width: parent.width
        spacing: 5

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
            height: (window.inPortrait ? 350 : (newFolderNameInput.activeFocus ? 70 : 210) )
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

                BusyIndicator {
                    id: busyIcon
                    visible: isBusy
                    running: isBusy
                    platformStyle: BusyIndicatorStyle { size: "large" }
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
            height: 70
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
                        text: name
                        font.pointSize: 18
                        color: "white"
                        elide: Text.ElideMiddle
                    }
                    Text {
                        width: parent.width
                        text: Qt.formatDateTime(lastModified, "d MMM yyyy h:mm:ss ap")
                        font.pointSize: 16
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
                    switch (selectedCloudType) {
                    case CloudDriveModel.Dropbox:
                        changeRemotePath(path + "/.");
                        break;
                    case CloudDriveModel.SkyDrive:
                        changeRemotePath(path);
                        break;
                    }
                } else {
                    // Always set selectedIndex to support changeRemotePath.
                    selectedIndex = index;

                    // FileGet and FilePut can select any items because it's not related to operations.
                    // Other operations must select only the same item type that selected from local path.
                    if (fsModel.isDir(localPath) == isDir || operation == CloudDriveModel.FileGet || operation == CloudDriveModel.FilePut) {
                        selectedRemotePath = path;
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
                var res = cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, path);
                console.debug("cloudDrivePathItem path " + path + " isRemotePathConnected " + res);
                if (!res) {
                    deleteCloudItemConfirmation.remotePath = path;
                    deleteCloudItemConfirmation.remotePathName = name;
                    deleteCloudItemConfirmation.open();
                }
            }
        }
    }

    CommonDialog {
        id: newFolderDialog
        titleText: appInfo.emptyStr+qsTr("New folder")
        titleIcon: "FilesPlusIcon.svg"
        buttonTexts: [appInfo.emptyStr+qsTr("OK"), appInfo.emptyStr+qsTr("Cancel")]
        content: Column {
            width: parent.width - 10
            height: 80
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 5

            TextField {
                id: folderName
                width: parent.width
                placeholderText: appInfo.emptyStr+qsTr("Please input folder name.")
            }
        }

        onStatusChanged: {
            if (status == DialogStatus.Open) {
                folderName.text = "";
            }
        }

        onButtonClicked: {
            if (index === 0 && folderName.text !== "") {
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
