import QtQuick 1.1
import com.nokia.meego 1.0
import CloudDriveModel 1.0
import ClipboardModel 1.0
import "Utility.js" as Utility

Page {
    id: cloudFolderPage

    property string name: "cloudFolderPage"

    property string caller: name
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
    property alias contentModel: cloudFolderModel
    property bool isBusy
/*
    function proceedOperation(type, uid, localPath, remotePath, remotePathName, isDir, remoteParentPath, modelIndex) {
        console.debug("cloudDrivePathDialog proceedOperation operation " + operation + " type " + type + " uid " + uid + " localPath " + localPath + " remotePath " + remotePath + " isDir " + isDir + " remoteParentPath " + remoteParentPath + " modelIndex " + modelIndex);

        switch (operation) {
        case CloudDriveModel.FilePut:
            console.debug("cloudDrivePathDialog proceedOperation FilePut localPath " + localPath + " to remotePath " + remotePath + " isDir " + isDir
                          + " remoteParentPath " + remoteParentPath
                          + " cloudDriveModel.getDirPath(localPath) " + cloudDriveModel.getDirPath(localPath)
                          + " cloudDriveModel.getRemoteName(remotePath) " + cloudDriveModel.getRemoteName(remotePath));

            // Upload selected local path into remote parent path.
            if (remoteParentPath != "") {
                switch (type) {
                case CloudDriveModel.Dropbox:
                    // Use remoteParentPath + "/" + local file/folder name.
                    remotePath = (remoteParentPath == "/" ? "" : remoteParentPath) + "/" + cloudDriveModel.getFileName(localPath);
                    console.debug("cloudDrivePathDialog proceedOperation FilePut adjusted remotePath " + remotePath);

                    if (cloudDriveModel.isDir(localPath)) {
                        cloudDriveModel.suspendNextJob();
                        cloudDriveModel.syncFromLocal(type, uid, localPath, remotePath, modelIndex, true);
                        cloudDriveModel.resumeNextJob();
                    } else {
                        cloudDriveModel.filePut(type, uid, localPath, remotePath, modelIndex);
                    }
                    break;
                case CloudDriveModel.SkyDrive:
                    if (cloudDriveModel.isDir(localPath)) {
                        cloudDriveModel.suspendNextJob();
                        cloudDriveModel.syncFromLocal_SkyDrive(type, uid, localPath, remoteParentPath, modelIndex, true);
                        cloudDriveModel.resumeNextJob();
                    } else {
                        cloudDriveModel.filePut(type, uid, localPath, remotePath, modelIndex);
                    }
                    break;
                case CloudDriveModel.Ftp:
                    // Use remoteParentPath + "/" + local file/folder name.
                    remotePath = (remoteParentPath == "/" ? "" : remoteParentPath) + "/" + cloudDriveModel.getFileName(localPath);
                    console.debug("cloudDrivePathDialog proceedOperation FilePut adjusted remotePath " + remotePath);

                    if (cloudDriveModel.isDir(localPath)) {
                        cloudDriveModel.suspendNextJob();
                        cloudDriveModel.syncFromLocal(type, uid, localPath, remotePath, modelIndex, true);
                        cloudDriveModel.resumeNextJob();
                    } else {
                        cloudDriveModel.filePut(type, uid, localPath, remotePath, modelIndex);
                    }
                    break;
                }
            } else {
                console.debug("cloudDrivePathDialog proceedOperation FilePut ignored remoteParentPath " + remoteParentPath + " is empty.");
            }

            break;
        case CloudDriveModel.FileGet:
            console.debug("cloudDrivePathDialog proceedOperation FileGet localPath " + localPath + " from remotePath " + remotePath + " remotePathName " + remotePathName + " isDir " + isDir
                          + " remoteParentPath " + remoteParentPath
                          + " cloudDriveModel.getDirPath(localPath) " + cloudDriveModel.getDirPath(localPath)
                          + " cloudDriveModel.getRemoteName(remotePath) " + cloudDriveModel.getRemoteName(remotePath));

            // Download selected remote folder/file to parent folder of selected local path.
            if (remotePath != "") {
                var targetLocalPath = cloudDriveModel.getAbsolutePath(cloudDriveModel.getDirPath(localPath), remotePathName);
                console.debug("cloudDrivePathDialog proceedOperation FileGet targetLocalPath " + targetLocalPath);
                cloudDriveModel.metadata(type, uid, targetLocalPath, remotePath, modelIndex);
            } else {
                console.debug("cloudDrivePathDialog proceedOperation FileGet ignored remotePath " + remotePath + " remotePathName " + remotePathName + " is empty.");
            }

            break;
        case CloudDriveModel.Browse:
            // Only folder can change connection.
            if (isDir != cloudDriveModel.isDir(localPath)) {
                console.debug("cloudDrivePathDialog proceedOperation Browse localPath isDir " + cloudDriveModel.isDir(localPath) + " and remotePath isDir " + isDir + " must be the same object type.");
                return;
            }

            if (cloudDriveModel.isDir(localPath) && remotePath != "") {
                var originalRemotePath = cloudDriveModel.getItemRemotePath(localPath, type, uid);
                if (remotePath != originalRemotePath) {
                    switch (type) {
                    case CloudDriveModel.Dropbox:
                        console.debug("cloudDrivePathDialog proceedOperation Browse move from " + originalRemotePath + " to " + remotePath);
                        // Set children hash to empty to hint syncFromLocal to put remained files with empty hash.
                        cloudDriveModel.updateItemWithChildren(type, uid, localPath, originalRemotePath, localPath, remotePath, "");
                        cloudDriveModel.metadata(type, uid, localPath, remotePath, selectedModelIndex);
                        break;
                    case CloudDriveModel.SkyDrive:
                        console.debug("cloudDrivePathDialog proceedOperation Browse move from " + originalRemotePath + " to " + remotePath);
                        // Remove original cloudDriveItem of localPath. Then start sync with new remotePath.
                        cloudDriveModel.removeItemWithChildren(type, uid, localPath);
                        cloudDriveModel.metadata(type, uid, localPath, remotePath, selectedModelIndex);
                        break;
                    case CloudDriveModel.Ftp:
                        console.debug("cloudDrivePathDialog proceedOperation Browse move from " + originalRemotePath + " to " + remotePath);
                        // Remove original cloudDriveItem of localPath. Then start sync with new remotePath.
                        cloudDriveModel.removeItemWithChildren(type, uid, localPath);
                        cloudDriveModel.metadata(type, uid, localPath, remotePath, selectedModelIndex);
                        break;
                    }
                }
            } else if (!cloudDriveModel.isDir(localPath)) {
                console.debug("cloudDrivePathDialog proceedOperation Browse ignored move localPath " + localPath + " is a file.");
            } else {
                console.debug("cloudDrivePathDialog proceedOperation Browse ignored move from " + originalRemotePath + " to " + remotePath + " remotePath is empty.");
            }

            break;
        case CloudDriveModel.Metadata:
            if (cloudDriveModel.isDir(localPath) && remotePath != "") {
                console.debug("cloudDrivePathDialog proceedOperation Metadata sync from " + localPath + " to " + remotePath);
                cloudDriveModel.metadata(type, uid, localPath, remotePath, selectedModelIndex);
            } else {
                // localPath is file or remotePath is empty.
                if (type == CloudDriveModel.Dropbox) {
                    // If localPath is file or remotePath is not specified.
                    // Use remoteParentPath + "/" + folderName.
                    remotePath = (remoteParentPath == "/" ? "" : remoteParentPath) + "/" + cloudDriveModel.getFileName(localPath);
                    console.debug("cloudDrivePathDialog proceedOperation Metadata sync from " + localPath + " to " + remotePath);
                    cloudDriveModel.metadata(type, uid, localPath, remotePath, selectedModelIndex);
                } else if (type == CloudDriveModel.SkyDrive) {
                    // Find remote path by name.
                    var newRemoteFolderName = cloudDriveModel.getFileName(localPath);
                    for (var i=0; i<contentModel.count; i++) {
                        if (contentModel.get(i).name == newRemoteFolderName) {
                            remotePath = contentModel.get(i).path;
                            console.debug("cloudDrivePathDialog proceedOperation Metadata found remotePath " + remotePath + " for " + newRemoteFolderName);
                            break;
                        }
                    }
                    // Create remote folder and get its remote path (id).
                    if (remotePath == "") {
                        remotePath = cloudDriveModel.createFolder_SkyDrive(type, uid, newRemoteFolderName, remoteParentPath);
                    }
                    // Start sync with remotePath.
                    if (remotePath != "") {
                        console.debug("cloudDrivePathDialog proceedOperation Metadata sync from " + localPath + " to " + remotePath);
                        cloudDriveModel.metadata(type, uid, localPath, remotePath, selectedModelIndex);
                    }
                } else if (type == CloudDriveModel.Ftp) {
                    // If localPath is file or remotePath is not specified.
                    // Use remoteParentPath + "/" + folderName.
                    remotePath = (remoteParentPath == "/" ? "" : remoteParentPath) + "/" + cloudDriveModel.getFileName(localPath);
                    console.debug("cloudDrivePathDialog proceedOperation Metadata sync from " + localPath + " to " + remotePath);
                    cloudDriveModel.metadata(type, uid, localPath, remotePath, selectedModelIndex);
                }
            }

            break;
        }
    }

    function getTitleText(localPath, remotePathName, remoteParentPathName) {
        var text = "";

        switch (operation) {
        case CloudDriveModel.FilePut:
            text += qsTr("Upload %1 into %2").arg(cloudDriveModel.getFileName(localPath)).arg(remoteParentPathName);
            break;
        case CloudDriveModel.FileGet:
            text += (remotePathName == "") ? qsTr("Please select folder") : qsTr("Download %1").arg(remotePathName);
            break;
        case CloudDriveModel.Browse:
            text += qsTr("Connect %1 to %2").arg(cloudDriveModel.getFileName(localPath)).arg(remotePathName);
            break;
        case CloudDriveModel.Metadata:
            text += qsTr("Sync %1 to %2").arg(cloudDriveModel.getFileName(localPath)).arg(remotePathName);
            break;
        }

        return text;
    }
*/

    function goUpSlot() {
//        nameFilterPanel.close();
//        fsModel.nameFilters = [];

//        if (fsModel.isRoot()) {
//            showDrivePageSlot();
//        } else {
//            if (state == "chart") {
//                fsModel.changeDir("..", FolderSizeItemListModel.SortBySize);
//            } else {
//                fsModel.changeDir("..");
//            }
//        }
        console.debug("cloudFolderPage goUpSlot selectedCloudType " + selectedCloudType + " selectedUid " + selectedUid + " remoteParentPath " + remoteParentPath + " remoteParentParentPath " + remoteParentParentPath);
        if (cloudDriveModel.isRemoteRoot(selectedCloudType, selectedUid, remoteParentParentPath)) {
            pageStack.pop(cloudFolderPage);
        } else {
            changeRemotePath(remoteParentParentPath);
        }
    }

    // TODO Get remote item source url into model item.
    function parseCloudDriveMetadataJson(jsonText) {
        // Reset list view.
        cloudFolderModel.clear();
        cloudFolderView.currentIndex = -1;
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
            remoteParentParentPath = cloudDriveModel.getParentRemotePath(remoteParentPath);
            for (var i=0; i<json.contents.length; i++) {
                var item = json.contents[i];
                var modelItem = { "name": cloudDriveModel.getRemoteName(item.path), "path": item.path,
                    "isChecked": false, "link": "",
                    "lastModified": (new Date(item.modified)), "size": item.bytes, "isDir": item.is_dir};
                cloudFolderModel.append(modelItem);
                if (modelItem.path == originalRemotePath) {
                    selectedIndex = i;
                    selectedRemotePath = modelItem.path;
                    selectedRemotePathName = modelItem.name;
                    selectedIsDir = modelItem.isDir;
                    cloudFolderView.currentIndex = i;
                }
            }
            break;
        case CloudDriveModel.SkyDrive:
            remoteParentPath = json.property.id;
            remoteParentPathName = json.property.name;
            remoteParentParentPath = (json.property.parent_id ? json.property.parent_id : "");
            for (var i=0; i<json.data.length; i++) {
                var item = json.data[i];
                var modelItem = { "name": item.name, "path": item.id,
                    "isChecked": false, "source": (item.source ? item.source : ""),
                    "lastModified": Utility.parseJSONDate(item.updated_time), "size": item.size, "isDir": (item.type == "folder" || item.type == "album") };
                cloudFolderModel.append(modelItem);
                if (modelItem.path == originalRemotePath) {
                    selectedIndex = i;
                    selectedRemotePath = modelItem.path;
                    selectedRemotePathName = modelItem.name;
                    selectedIsDir = modelItem.isDir;
                    cloudFolderView.currentIndex = i;
                }
            }
            break;
        case CloudDriveModel.Ftp:
            remoteParentPath = json.property.path;
            remoteParentPathName = json.property.path;
            remoteParentParentPath = cloudDriveModel.getParentRemotePath(remoteParentPath);
            for (var i=0; i<json.data.length; i++) {
                var item = json.data[i];
                var modelItem = { "name": item.name, "path": item.path,
                    "isChecked": false, "link": "",
                    "lastModified": Utility.parseJSONDate(item.lastModified), "size": item.size, "isDir": item.isDir };
                cloudFolderModel.append(modelItem);
                if (modelItem.path == originalRemotePath) {
                    selectedIndex = i;
                    selectedRemotePath = modelItem.path;
                    selectedRemotePathName = modelItem.name;
                    selectedIsDir = modelItem.isDir;
                    cloudFolderView.currentIndex = i;
                }
            }
            break;
        }

        // Reset busy.
        isBusy = false;
        if (selectedIndex > -1) {
            cloudFolderView.positionViewAtIndex(selectedIndex, ListView.Contain); // Issue: ListView.Center will cause list item displayed out of list view in Meego only.
        }
    }

    function changeRemotePath(remoteParentPath) {
        console.debug("cloudFolderPage changeRemotePath " + remoteParentPath);
        cloudFolderPage.remoteParentPath = remoteParentPath;
        refreshSlot("cloudFolderPage changeRemotePath");
    }

    function getSelectedRemotePath() {
        var selectedRemotePath = "";

        if (cloudFolderView.currentIndex > -1) {
            selectedRemotePath = cloudDrivePathListModel.get(cloudFolderView.currentIndex).path;
        }

        return selectedRemotePath;
    }

    function selectProceedOperation(index) {
        if (index === 0 && selectedIsValid) { // OK button is clicked.
            console.debug("cloudDrivePathDialog selectProceedOperation index " + index
                          + " selectedCloudType " + selectedCloudType + " selectedUid " + selectedUid
                          + " localPath " + localPath
                          + " selectedRemotePath " + selectedRemotePath
                          + " selectedRemotePathName " + selectedRemotePathName
                          + " selectedModelIndex " + selectedModelIndex
                          + " remoteParentPath " + remoteParentPath);

            cloudDriveModel.suspendNextJob();
            proceedOperation(selectedCloudType, selectedUid, localPath, selectedRemotePath, selectedRemotePathName, selectedIsDir, remoteParentPath, selectedModelIndex);
            cloudDriveModel.resumeNextJob();
        }
    }

    function createRemoteFolder() {
        // Create remote folder.
        if (selectedCloudType == CloudDriveModel.Dropbox) {
            cloudDriveModel.createFolder(selectedCloudType, selectedUid, "", remoteParentPath + "/" + newRemoteFolderName, -1);
        } else if (selectedCloudType == CloudDriveModel.SkyDrive) {
            cloudDriveModel.createFolder(selectedCloudType, selectedUid, newRemoteFolderName, remoteParentPath, -1);
        } else if (selectedCloudType == CloudDriveModel.Ftp) {
            cloudDriveModel.createFolder(selectedCloudType, selectedUid, "", remoteParentPath + "/" + newRemoteFolderName, -1);
        }
    }

    function refreshSlot(caller) {
        console.debug("cloudFolderPage refreshSlot caller " + caller + " " + selectedCloudType + " " + remotePath + " " + remoteParentPath);

        selectedIndex = -1;
        isBusy = true;

        // Browse remote parent path.
        // Issue: selectedCloudType which is linked from uidDialog doesn't work with switch.
        if (selectedCloudType == CloudDriveModel.Dropbox) {
            remoteParentPath = (remoteParentPath == "") ? cloudDriveModel.getParentRemotePath(remotePath) : remoteParentPath;
            cloudDriveModel.browse(selectedCloudType, selectedUid, remoteParentPath);
        } else if (selectedCloudType == CloudDriveModel.SkyDrive) {
            remoteParentPath = (remoteParentPath == "") ? "me/skydrive" : remoteParentPath;
            cloudDriveModel.browse(selectedCloudType, selectedUid, remoteParentPath);
        } else if (selectedCloudType == CloudDriveModel.Ftp) {
            remoteParentPath = (remoteParentPath == "") ? cloudDriveModel.getParentRemotePath(remotePath) : remoteParentPath;
            cloudDriveModel.browse(selectedCloudType, selectedUid, remoteParentPath);
        }
    }

    function deleteRemotePath() {
        // Delete remote file/folder.
        cloudDriveModel.deleteFile(selectedCloudType, selectedUid, "", remotePath);
    }

    states: [
        State {
            name: "mark"
        },
        State {
            name: "list"
        }
    ]
    state: "list"

    tools: ToolBarLayout {
        id: toolBarLayout

        ToolIcon {
            id: backButton
            iconId: "toolbar-back"
            onClicked: {
                if (cloudFolderPage.state == "mark") {
                    cloudFolderPage.state = "list";
                } else {
                    // Specify local path to focus after cd to parent directory..
//                    fsListView.focusLocalPath = fsModel.currentDir;
                    goUpSlot();
                }
            }
        }

        ToolIcon {
            id: refreshButton
            iconId: "toolbar-refresh"
            visible: (cloudFolderPage.state == "list")
            onClicked: {
                refreshSlot("refreshButton onClicked");
            }
        }

        ToolIcon {
            id: menuButton
            iconId: "toolbar-view-menu"
            onClicked: {
                if (cloudFolderPage.state == "mark") {
                    if (!cloudFolderView.isAnyItemChecked()) {
                        markAllMenu.open();
                    } else {
                        markMenu.open();
                    }
                } else if (cloudFolderPage.state == "list") {
                    mainMenu.open();
                }
            }
        }
    }

    TitlePanel {
        id: currentPath
        text: remoteParentPathName
        textLeftMargin: height + 5

        Image {
            id: cloudIcon
            source: cloudDriveModel.getCloudIcon(selectedCloudType)
            width: parent.height
            height: parent.height
            fillMode: Image.PreserveAspectFit
            anchors.left: parent.left
        }
    }

    // TODO Menu, PopupToolRing.
    MainMenu {
        id: mainMenu
        disabledMenus: ["syncConnectedItems","syncCurrentFolder"]

        onPaste: {
        }
        onOpenMarkMenu: {
            cloudFolderPage.state = "mark";
        }
        onClearClipboard: {
            clipboard.clear();
        }
        onNewFolder: {
            newFolderDialog.open();
        }
        onSetNameFilter: {
        }
        onOpenSortByMenu: {
        }
        onOpenAbout: {
            pageStack.push(Qt.resolvedUrl("AboutPage.qml"));
        }
        onQuit: {
        }

        function isMenuItemVisible(menuItem) {
            // Validate each menu logic if it's specified, otherwise it's visible.
            if (menuItem.name == "paste") {
                return clipboard.count > 0;
            } else if (menuItem.name == "clearClipboard") {
                return clipboard.count > 0;
            } else if (menuItem.name == "markMenu") {
                return cloudFolderPage.state != "mark";
            } else {
                return true;
            }
        }
    }

    MarkMenu {
        id: markMenu
    }

    MarkAllMenu {
        id: markAllMenu
    }

    ListModel {
        id: cloudFolderModel
    }

    ListView {
        id: cloudFolderView
        width: parent.width
        height: parent.height - currentPath.height
        anchors.top: currentPath.bottom
        model: cloudFolderModel
        delegate: cloudItemDelegate
        highlight: Rectangle {
            width: cloudFolderView.width
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

        function isAnyItemChecked() {
            // TODO
            return false;
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

    // TODO Shows mark, copy, cut icons.
    Component {
        id: cloudItemDelegate

        ListItem {
            id: cloudListItem
            height: 80
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
                        source: cloudListItem.getIconSource(path, isDir)
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
                    visible: cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, path, true);
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

            onClicked: { // Switch type
                if (isDir) {
                    changeRemotePath(path);
                } else {
                    // Always set selectedIndex to support changeRemotePath.
                    selectedIndex = index;

                    // FileGet and FilePut can select any items because it's not related to operations.
                    // Other operations must select only the same item type that selected from local path.
                    if (cloudDriveModel.isDir(localPath) == isDir || operation == CloudDriveModel.FileGet || operation == CloudDriveModel.FilePut) {
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
                        cloudFolderView.currentIndex = -1;
                        console.debug("cloudDrivePathDialog invalid selectedIndex " + selectedIndex + " selectedRemotePath " + selectedRemotePath + " selectedRemotePathName " + selectedRemotePath + " selectedIsDir " + selectedIsDir);
                        // TODO Disable OK button.
                    }
                }
            }

            onPressAndHold: {
                var res = cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, path);
                console.debug("cloudListItem path " + path + " isRemotePathConnected " + res);
                if (!res) {
                    deleteCloudItemConfirmation.remotePath = path;
                    deleteCloudItemConfirmation.remotePathName = name;
                    deleteCloudItemConfirmation.open();
                }
            }
        }
    }

    ConfirmDialog {
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
        if (status == PageStatus.Active) {
            refreshSlot("cloudFolderPage onStatusChanged");
        }
    }

    Component.onCompleted: {
        console.debug(Utility.nowText() + " cloudFolderPage onCompleted");
        window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("CloudFolderPage"), 0.1);

        // Proceeds queued jobs during constructions.
//        cloudDriveModel.resumeNextJob();
    }
}
