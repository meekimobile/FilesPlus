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

    function goUpSlot() {
        console.debug("cloudFolderPage goUpSlot selectedCloudType " + selectedCloudType + " selectedUid " + selectedUid + " remoteParentPath " + remoteParentPath + " remoteParentParentPath " + remoteParentParentPath);
        if (cloudDriveModel.isRemoteRoot(selectedCloudType, selectedUid, remoteParentParentPath)) {
            pageStack.pop(cloudFolderPage);
        } else {
            changeRemotePath(remoteParentParentPath);
        }
    }

    function parseCloudDriveMetadataJson(jsonText) {
        originalRemotePath = (originalRemotePath == "") ? remotePath : originalRemotePath;

        cloudFolderModel.clear();
        var responseJson = cloudDriveModel.parseCloudDriveMetadataJson(selectedCloudType, originalRemotePath, jsonText,  cloudFolderModel);

        selectedIsValid = true;
        selectedIndex = responseJson.selectedIndex;
        selectedRemotePath = responseJson.selectedRemotePath;
        selectedRemotePathName = responseJson.selectedRemotePathName;
        selectedIsDir = responseJson.selectedIsDir;
        remoteParentPath = responseJson.remoteParentPath;
        remoteParentPathName = responseJson.remoteParentPathName;
        remoteParentParentPath = responseJson.remoteParentParentPath;

        cloudFolderView.currentIndex = responseJson.selectedIndex;

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

    function createRemoteFolder(newRemoteFolderName) {
        // Create remote folder.
        if (selectedCloudType == CloudDriveModel.Dropbox) {
            cloudDriveModel.createFolder(selectedCloudType, selectedUid, "", remoteParentPath + "/" + newRemoteFolderName, -1);
        } else if (selectedCloudType == CloudDriveModel.SkyDrive) {
            cloudDriveModel.createFolder(selectedCloudType, selectedUid, newRemoteFolderName, remoteParentPath, -1);
        } else if (selectedCloudType == CloudDriveModel.Ftp) {
            cloudDriveModel.createFolder(selectedCloudType, selectedUid, "", remoteParentPath + "/" + newRemoteFolderName, -1);
        }
    }

    function deleteRemotePath(remotePath) {
        // Delete remote file/folder.
        cloudDriveModel.deleteFile(selectedCloudType, selectedUid, "", remotePath);
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
        } else if (selectedCloudType == CloudDriveModel.GoogleDrive) {
            remoteParentPath = (remoteParentPath == "") ? cloudDriveModel.getParentRemotePath(remotePath) : remoteParentPath;
            cloudDriveModel.browse(selectedCloudType, selectedUid, remoteParentPath);
        } else if (selectedCloudType == CloudDriveModel.Ftp) {
            remoteParentPath = (remoteParentPath == "") ? cloudDriveModel.getParentRemotePath(remotePath) : remoteParentPath;
            cloudDriveModel.browse(selectedCloudType, selectedUid, remoteParentPath);
        }
    }

    function resetBusySlot(caller) {
        isBusy = false;
    }

    function syncConnectedItemsSlot() {
        // Suspend for queuing.
        cloudDriveModel.suspendNextJob();

        for (var i=0; i<cloudFolderModel.count; i++) {
            var remotePath = cloudFolderModel.get(i).absolutePath;
            var isConnected = cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, remotePath);
            console.debug("folderPage synconnectedItemsSlot remotePath " + remotePath + " isConnected " + isConnected);
            if (isConnected) {
                cloudDriveModel.syncItemByRemotePath(selectedCloudType, selectedUid, remotePath);
            }
        }

        // Resume proceed queued jobs.
        cloudDriveModel.resumeNextJob();
    }

    function syncClipboardItems() {
        // Suspend for queuing.
        cloudDriveModel.suspendNextJob();

        for (var i=0; i<clipboard.count; i++) {
            if (clipboard.get(i).action == "sync") {
                console.debug("cloudFolderPage syncClipboardItems clipboard item i " + i + " " + clipboard.get(i).action + " type " + clipboard.get(i).type + " sourcePath " + clipboard.get(i).sourcePath);
                var res = cloudDriveModel.syncItemByRemotePath(cloudDriveModel.getClientType(clipboard.get(i).type), clipboard.get(i).uid, clipboard.get(i).sourcePath);
                if (!res) {
                    console.debug("cloudFolderPage syncClipboardItems skipped clipboard item i " + i + " " + clipboard.get(i).action + " type " + clipboard.get(i).type + " sourcePath " + clipboard.get(i).sourcePath);
                }
            }
        }

        // Clear clipboard.
        clipboard.clear();

        // Resume proceed queued jobs.
        cloudDriveModel.resumeNextJob();
    }

    function updateJobQueueCount(runningJobCount, jobQueueCount) {
        // Update (runningJobCount + jobQueueCount) on cloudButton.
        cloudButtonIndicator.text = ((runningJobCount + jobQueueCount) > 0) ? (runningJobCount + jobQueueCount) : "";
    }

    function updateItemSlot(jobJson) {
        if (!jobJson) return;
        if (jobJson.remote_file_path == "") return;

        var i = cloudFolderModel.findIndexByRemotePath(jobJson.remote_file_path);
        if (i >= 0) {
            cloudFolderModel.set(i, { isRunning: jobJson.is_running });
        }
    }

    tools: ToolBarLayout {
        id: toolBarLayout

        ToolIcon {
            id: backButton
            iconId: "toolbar-back"
            onClicked: {
                if (cloudFolderView.state == "mark") {
                    cloudFolderView.state = "";
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
            visible: (cloudFolderView.state != "mark")
            onClicked: {
                refreshSlot("refreshButton onClicked");
            }
        }

        ToolIcon {
            id: cloudButton
            iconSource: (theme.inverted ? "cloud.svg" : "cloud_inverted.svg")
            visible: (cloudFolderView.state != "mark")
            onClicked: {
                syncConnectedItemsSlot();
            }

            TextIndicator {
                id: cloudButtonIndicator
                color: "#00AAFF"
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10
            }
        }

        ToolIcon {
            id: menuButton
            iconId: "toolbar-view-menu"
            onClicked: {
                // Hide popupToolPanel.
                popupToolPanel.visible = false;

                if (cloudFolderView.state == "mark") {
                    if (!cloudFolderView.isAnyItemChecked()) {
                        markAllMenu.open();
                    } else {
                        markMenu.open();
                    }
                } else {
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

    CloudFolderMenu {
        id: mainMenu
        disabledMenus: ["syncConnectedItems","syncCurrentFolder","setNameFilter","sortByMenu"]

        onPaste: {
            fileActionDialog.targetPath = remoteParentPath;
            fileActionDialog.targetPathName = remoteParentPathName;
            fileActionDialog.open();
        }
        onOpenMarkMenu: {
            cloudFolderView.state = "mark";
        }
        onClearClipboard: {
            clipboard.clear();
        }
        onNewFolder: {
            newFolderDialog.open();
        }
        onDrives: {
            pageStack.pop(cloudFolderPage);
        }
        onOpenSettings: {
            pageStack.push(Qt.resolvedUrl("SettingPage.qml"));
        }
        onOpenAbout: {
            pageStack.push(Qt.resolvedUrl("AboutPage.qml"));
        }
        onQuit: {
        }

        function isMenuItemVisible(menuItem) {
            // Validate each menu logic if it's specified, otherwise it's visible.
            if (menuItem.name == "paste") {
                return (clipboard.count > 0 && selectedCloudType != CloudDriveModel.Ftp);
            } else if (menuItem.name == "clearClipboard") {
                return clipboard.count > 0;
            } else if (menuItem.name == "markMenu") {
                return cloudFolderView.state != "mark";
            }

            return true;
        }
    }

    MarkMenu {
        id: markMenu

        onMarkAll: {
            if (isMarkAll) {
                cloudFolderView.markAll();
            } else {
                cloudFolderView.unmarkAll();
            }
            isMarkAll = !isMarkAll;
        }
        onCutMarkedItems: {
            cloudFolderView.cutMarkedItems();
            cloudFolderView.state = "";
        }
        onCopyMarkedItems: {
            cloudFolderView.copyMarkedItems();
            cloudFolderView.state = "";
        }
        onDeleteMarkedItems: {
            cloudFolderView.deleteMarkedItems();
            cloudFolderView.state = "";
        }
        onSyncMarkedItems: {
            cloudFolderView.syncMarkedItems();
            cloudFolderView.state = "";
        }
        onStatusChanged: {
            if (status == DialogStatus.Opening) {
                isMarkAll = !cloudFolderView.areAllItemChecked();
            }
        }

        function isMenuItemVisible(menuItem) {
            // Validate each menu logic if it's specified, otherwise it's visible.
            if (menuItem.name == "copyMarked") {
                return (selectedCloudType != CloudDriveModel.Ftp);
            } else if (menuItem.name == "cutMarked") {
                return (selectedCloudType != CloudDriveModel.Ftp);
            }

            return true;
        }
    }

    MarkAllMenu {
        id: markAllMenu

        onMarkAll: cloudFolderView.markAll()
        onMarkAllFiles: cloudFolderView.markAllFiles()
        onMarkAllFolders: cloudFolderView.markAllFolders();
    }

    ListModel {
        id: cloudFolderModel

        function findIndexByRemotePath(remotePath) {
            for (var i=0; i<cloudFolderModel.count; i++) {
                if (cloudFolderModel.get(i).absolutePath == remotePath) {
                    return i;
                }
            }

            return -1;
        }
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
        highlightFollowsCurrentItem: false
        highlightMoveSpeed: 4000
        pressDelay: 100
        clip: true
        state: ""
        states: [
            State {
                name: "mark"
            }
        ]

        function isAnyItemChecked() {
            for (var i=0; i<model.count; i++) {
                var checked = model.get(i).isChecked;
                if (checked) {
                    return true;
                }
            }
            return false;
        }

        function areAllItemChecked() {
            for (var i=0; i<model.count; i++) {
                var checked = model.get(i).isChecked;
                if (!checked) {
                    return false;
                }
            }
            return (model.count > 0);
        }

        function markAll() {
            for (var i=0; i<model.count; i++) {
                model.setProperty(i, "isChecked", true);
            }
        }

        function markAllFiles() {
            for (var i=0; i<model.count; i++) {
                if (!model.get(i).isDir) {
                    model.setProperty(i, "isChecked", true);
                }
            }
        }

        function markAllFolders() {
            for (var i=0; i<model.count; i++) {
                if (model.get(i).isDir) {
                    model.setProperty(i, "isChecked", true);
                }
            }
        }

        function unmarkAll() {
            for (var i=0; i<model.count; i++) {
                model.setProperty(i, "isChecked", false);
            }
        }

        function cutMarkedItems() {
            for (var i=0; i<model.count; i++) {
                if (model.get(i).isChecked) {
                    console.debug(Utility.nowText() + "cloudFolderView cutMarkedItems item"
                                  + " absolutePath " + model.get(i).absolutePath
                                  + " isChecked " + model.get(i).isChecked);

                    clipboard.addItemWithSuppressCountChanged({ "action": "cut", "type": cloudDriveModel.getCloudName(selectedCloudType), "uid": selectedUid, "sourcePath": model.get(i).absolutePath, "sourcePathName": model.get(i).name });
                }

                // Reset isChecked.
                model.setProperty(i, "isChecked", false);
            }

            // Emit suppressed signal.
            clipboard.emitCountChanged();
        }

        function copyMarkedItems() {
            for (var i=0; i<model.count; i++) {
                if (model.get(i).isChecked) {
                    console.debug(Utility.nowText() + "cloudFolderView copyMarkedItems item"
                                  + " absolutePath " + model.get(i).absolutePath
                                  + " isChecked " + model.get(i).isChecked);

                    clipboard.addItemWithSuppressCountChanged({ "action": "copy", "type": cloudDriveModel.getCloudName(selectedCloudType), "uid": selectedUid, "sourcePath": model.get(i).absolutePath, "sourcePathName": model.get(i).name });
                }

                // Reset isChecked.
                model.setProperty(i, "isChecked", false);
            }

            // Emit suppressed signal.
            clipboard.emitCountChanged();
        }

        function deleteMarkedItems() {
            // Always clear clipboard before delete marked items.
            clipboard.clear();

            for (var i=0; i<model.count; i++) {
                if (model.get(i).isChecked) {
                    console.debug(Utility.nowText() + "cloudFolderView deleteMarkedItems item"
                                  + " absolutePath " + model.get(i).absolutePath
                                  + " isChecked " + model.get(i).isChecked);

                    clipboard.addItem({ "action": "delete", "type": cloudDriveModel.getCloudName(selectedCloudType), "uid": selectedUid, "sourcePath": model.get(i).absolutePath, "sourcePathName": model.get(i).name });
                }

                // Reset isChecked.
                model.setProperty(i, "isChecked", false);
            }

            // Open confirmation dialog.
            fileActionDialog.open();
        }

        function syncMarkedItems() {
            // Always clear clipboard before sync marked items.
            clipboard.clear();

            for (var i=0; i<model.count; i++) {
                if (model.get(i).isChecked) {
                    console.debug(Utility.nowText() + " cloudFolderView syncMarkedItems item"
                                  + " absolutePath " + model.get(i).absolutePath
                                  + " isChecked " + model.get(i).isChecked);

                    clipboard.addItem({ "action": "sync", "type": cloudDriveModel.getCloudName(selectedCloudType), "uid": selectedUid, "sourcePath": model.get(i).absolutePath, "sourcePathName": model.get(i).name });
                }

                // Reset isChecked.
                model.setProperty(i, "isChecked", false);
            }

            // Invoke syncClipboard.
            syncClipboardItems();
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

        onMovementStarted: {
            if (currentItem) {
                currentItem.pressed = false;
            }
            currentIndex = -1;
        }
    }

    Component {
        id: cloudItemDelegate

        CloudListItem {
            id: listItem
            listViewState: (cloudFolderView.state ? cloudFolderView.state : "")
            syncIconVisible: cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, absolutePath)
            syncIconSource: (isRunning) ? "cloud_wait.svg" : "cloud.svg"

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

            onPressAndHold: {
                if (cloudFolderView.state != "mark") {
                    cloudFolderView.currentIndex = index;
                    popupToolPanel.selectedFilePath = absolutePath;
                    popupToolPanel.selectedFileName = name;
                    popupToolPanel.selectedFileIndex = index;
                    popupToolPanel.isDir = isDir;
                    popupToolPanel.pastePath = (isDir) ? absolutePath : remoteParentPath;
                    var panelX = x + mouseX - cloudFolderView.contentX + cloudFolderView.x;
                    var panelY = y + mouseY - cloudFolderView.contentY + cloudFolderView.y;
                    popupToolPanel.open(panelX, panelY);
                }
            }

            onClicked: {
                if (cloudFolderView.state == "mark") {
                    cloudFolderModel.setProperty(index, "isChecked", !isChecked);
                } else {
                    if (isDir) {
                        changeRemotePath(absolutePath);
                    } else {
                        // If file is running, disable preview.
                        if (isRunning) return;

                        if (source && source != "") {
                            Qt.openUrlExternally(source);
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: popupToolPanelBG
        color: "black"
        opacity: 0.7
        z: 1
        anchors.fill: parent
        visible: popupToolPanel.visible
        MouseArea {
            anchors.fill: parent
            onClicked: {
                // do nothing.
                popupToolPanel.visible = false;
            }
        }
    }

    PopupToolRing {
        id: popupToolPanel
        ringRadius: 80
        buttonRadius: 35
        timeout: appInfo.emptySetting+appInfo.getSettingValue("popup.timer.interval", 2) * 1000
        disabledButtons: ["print","editFile","cloud","share"]

        function isButtonVisibleCallback(buttonName) {
            if (buttonName === "sync") {
                return cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, selectedFilePath);
            } else if (buttonName === "copy") {
                return (selectedCloudType != CloudDriveModel.Ftp);
            } else if (buttonName === "cut") {
                return (selectedCloudType != CloudDriveModel.Ftp);
            } else if (buttonName === "paste") {
                return (clipboard.count > 0 && selectedCloudType != CloudDriveModel.Ftp);
            }

            return true;
        }

        onOpened: {
//            console.debug("popupToolRing onOpened");
            cloudFolderView.highlightFollowsCurrentItem = true;
        }

        onClosed: {
//            console.debug("popupToolRing onClosed");
            // Workaround to hide highlight.
            cloudFolderView.currentIndex = -1;
            cloudFolderView.highlightFollowsCurrentItem = false;
        }

        onCutClicked: {
            clipboard.addItem({ "action": "cut", "type": cloudDriveModel.getCloudName(selectedCloudType), "uid": selectedUid, "sourcePath": sourcePath, "sourcePathName": selectedFileName });
        }

        onCopyClicked: {
            clipboard.addItem({ "action": "copy", "type": cloudDriveModel.getCloudName(selectedCloudType), "uid": selectedUid, "sourcePath": sourcePath, "sourcePathName": selectedFileName });
        }

        onPasteClicked: {
            fileActionDialog.targetPath = targetPath;
            fileActionDialog.targetPathName = selectedFileName;
            fileActionDialog.open();
        }

        onDeleteFile: {
            // Delete always clear clipboard.
            clipboard.clear();
            clipboard.addItem({ "action": "delete", "type": cloudDriveModel.getCloudName(selectedCloudType), "uid": selectedUid, "sourcePath": sourcePath, "sourcePathName": selectedFileName });
            fileActionDialog.open();
        }

        onSyncFile: {
            cloudDriveModel.syncItemByRemotePath(selectedCloudType, selectedUid, srcFilePath);
        }

        onNewFolder: {
            newFolderDialog.open();
        }

        onRenameFile: {
            renameDialog.sourcePath = srcFilePath;
            renameDialog.sourcePathName = selectedFileName;
            renameDialog.open();
        }

        onMarkClicked: {
            cloudFolderView.state = "mark";
            cloudFolderModel.setProperty(srcItemIndex, "isChecked", true);
        }
    }

    ConfirmDialog {
        id: fileActionDialog

        property string targetPath
        property string targetPathName

        titleText: appInfo.emptyStr+fileActionDialog.getTitleText()
        contentText: appInfo.emptyStr+fileActionDialog.getText() + "\n"

        function getTitleText() {
            var text = "";
            if (clipboard.count == 1) {
                text = getActionName(clipboard.get(0).action, clipboard.get(0).type);
            } else {
                // TODO if all copy, show "Multiple copy".
                text = qsTr("Multiple actions");
            }

            return text;
        }

        function getText() {
            // Exmaple of clipboard entry { "action": "cut", "sourcePath": sourcePath }
            // Exmaple of clipboard entry { "action": "cut", "type": "Dropbox", "uid": "asdfdg", "sourcePath": sourcePath, "sourcePathName": sourcePathName }
            var text = "";
            if (clipboard.count == 1) {
                text = getActionName(clipboard.get(0).action, clipboard.get(0).type)
                        + " " + (clipboard.get(0).sourcePathName ? clipboard.get(0).sourcePathName : clipboard.get(0).sourcePath)
                        + ((clipboard.get(0).action == "delete")?"":("\n" + qsTr("to") + " " + targetPathName))
                        + " ?";
            } else {
                var cutCount = 0;
                var copyCount = 0;
                var deleteCount = 0;
                var uploadCount = 0;
                for (var i=0; i<clipboard.count; i++) {
//                    console.debug("fileActionDialog getText clipboard i " + i + " action " + clipboard.get(i).action + " sourcePath " + clipboard.get(i).sourcePath);
                    if (["copy","cut"].indexOf(clipboard.get(i).action) > -1 && !clipboard.get(i).type) {
                        uploadCount++;
                    } else if (clipboard.get(i).action == "copy") {
                        copyCount++;
                    } else if (clipboard.get(i).action == "cut") {
                        cutCount++;
                    } else if (clipboard.get(i).action == "delete") {
                        deleteCount++;
                    }
                }

                if (uploadCount>0) text = text + (qsTr("Upload %n item(s)\n", "", uploadCount));
                if (deleteCount>0) text = text + (qsTr("Delete %n item(s)\n", "", deleteCount));
                if (copyCount>0) text = text + (qsTr("Copy %n item(s)\n", "", copyCount));
                if (cutCount>0) text = text + (qsTr("Move %n item(s)\n", "", cutCount));
                if (copyCount>0 || cutCount>0) text = text + qsTr("to") + " " + targetPathName;
                text = text + " ?";
            }

            return text;
        }

        function getActionName(actionText, type) {
            if (type) {
                if (actionText == "copy") return qsTr("Copy");
                else if (actionText == "cut") return qsTr("Move");
                else if (actionText == "delete") return qsTr("Delete");
                else return qsTr("Invalid action");
            } else {
                if (["copy","cut"].indexOf(actionText) > -1) return qsTr("Upload");
                else return qsTr("Invalid action");
            }
        }

        onConfirm: {
            if (clipboard.count == 1) {
                // Copy/Move/Delete first file from clipboard.
                // Check if there is existing file on target folder. Then show overwrite dialog.
                // TODO How to check if folder/file exists.
//                if (clipboard.get(0).action != "delete" && !fsModel.canCopy(clipboard.get(0).sourcePath, targetPath)) {
//                    fileOverwriteDialog.sourcePath = clipboard.get(0).sourcePath;
//                    fileOverwriteDialog.sourcePathName = clipboard.get(0).sourcePathName;
//                    fileOverwriteDialog.targetPath = targetPath;
//                    fileOverwriteDialog.isCopy = (clipboard.get(0).action == "copy");
//                    fileOverwriteDialog.open();
//                    return;
//                }
            }

            // It always replace existing names.
            cloudDriveModel.suspendNextJob();

            // TODO Copy/Move/Delete all files from clipboard.
            // Action is {copy, cut, delete}
            var res = true;
            for (var i=0; i<clipboard.count; i++) {
                var action = clipboard.get(i).action;
                var sourcePath = clipboard.get(i).sourcePath;
                var actualTargetPath = cloudDriveModel.getRemotePath(selectedCloudType, targetPath, cloudDriveModel.getFileName(sourcePath));

                console.debug("folderPage fileActionDialog onConfirm clipboard action " + action + " sourcePath " + sourcePath + " actualTargetPath " + actualTargetPath);
                if (["copy","cut"].indexOf(action) > -1 && !clipboard.get(i).type) {
                    isBusy = true;
                    if (cloudDriveModel.isDir(sourcePath)) {
                        switch (selectedCloudType) {
                        case CloudDriveModel.SkyDrive:
                            cloudDriveModel.syncFromLocal_SkyDrive(selectedCloudType, selectedUid, sourcePath, actualTargetPath, -1, true);
                            break;
                        default:
                            cloudDriveModel.syncFromLocal(selectedCloudType, selectedUid, sourcePath, actualTargetPath, -1, true);
                        }
                    } else {
                        cloudDriveModel.filePut(selectedCloudType, selectedUid, sourcePath, actualTargetPath, -1);
                    }
                    res = true;
                } else if (action == "copy") {
                    isBusy = true;
                    cloudDriveModel.copyFile(cloudDriveModel.getClientType(clipboard.get(i).type), clipboard.get(i).uid, "", sourcePath, "", actualTargetPath);
                    res = true;
                } else if (action == "cut") {
                    isBusy = true;
                    cloudDriveModel.moveFile(cloudDriveModel.getClientType(clipboard.get(i).type), clipboard.get(i).uid, "", sourcePath, "", actualTargetPath);
                    res = true;
                } else if (action == "delete") {
                    isBusy = true;
                    cloudDriveModel.deleteFile(cloudDriveModel.getClientType(clipboard.get(i).type), clipboard.get(i).uid, "", sourcePath);
                    res = true;
                } else {
                    console.debug("folderPage fileActionDialog onButtonClicked invalid action " + action);
                }

                if (!res) {
                    break;
                }
            }

            // Reset targetPath and popupToolPanel's paths.
            if (res) {
                // Reset both source and target.
                targetPath = "";
                popupToolPanel.srcFilePath = "";
                popupToolPanel.pastePath = "";

                // Clear clipboard as they should have been processed.
                clipboard.clear();
            } else {
                // Reset target only.
                targetPath = "";
                popupToolPanel.pastePath = "";
            }
        }

        onClosed: {
            // Always clear clipboard's delete actions.
            clipboard.clearDeleteActions();

            // Resume queued jobs.
            cloudDriveModel.resumeNextJob();
        }
    }

    ConfirmDialog {
        id: newFolderDialog
        titleText: appInfo.emptyStr+qsTr("New folder")
        titleIcon: "FilesPlusIcon.svg"
        buttonTexts: [appInfo.emptyStr+qsTr("OK"), appInfo.emptyStr+qsTr("Cancel")]
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
        id: renameDialog

        property string sourcePath
        property string sourcePathName

        titleText: appInfo.emptyStr+qsTr("Rename")
        titleIcon: "FilesPlusIcon.svg"
        buttonTexts: [appInfo.emptyStr+qsTr("OK"), appInfo.emptyStr+qsTr("Cancel")]
        content: Column {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - 10
            height: 120
            spacing: 3

            Text {
                width: parent.width
                text: appInfo.emptyStr+qsTr("Rename %1 to").arg(renameDialog.sourcePathName);
                color: "white"
                font.pointSize: 16
                elide: Text.ElideMiddle
            }

            TextField {
                id: newName
                width: parent.width
                placeholderText: appInfo.emptyStr+qsTr("Please input new name.")
            }
        }

        onOpened: {
            newName.text = sourcePathName;
            newName.forceActiveFocus();
        }

        onClosed: {
            newName.text = "";
        }

        onConfirm: {
            if (newName.text != "" && newName.text != sourcePathName) {
                isBusy = true;
                switch (selectedCloudType) {
                case CloudDriveModel.SkyDrive:
                    cloudDriveModel.moveFile(selectedCloudType, selectedUid, "", sourcePath, "", newName.text);
                    break;
                default:
                    cloudDriveModel.moveFile(selectedCloudType, selectedUid, "", sourcePath, "", cloudDriveModel.getParentRemotePath(sourcePath) + "/" + newName.text);
                }
            }
        }
    }

    ConfirmDialog {
        id: fileOverwriteDialog
        titleText: appInfo.emptyStr+qsTr("File overwrite")
        titleIcon: "FilesPlusIcon.svg"
        buttonTexts: [appInfo.emptyStr+qsTr("OK"), appInfo.emptyStr+qsTr("Cancel")]
        content: Column {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - 10
            height: 160
            spacing: 3

            Text {
                text: appInfo.emptyStr+qsTr("Please input new file name.")
                color: "white"
                width: parent.width
                height: 48
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
                text: "<font color='white'>" + appInfo.emptyStr+qsTr("Overwrite existing file") + "</font>"
                checked: false

                onClicked: {
                    if (checked) {
                        fileName.text = fsModel.getFileName(fileOverwriteDialog.sourcePath);
                    } else {
                        fileName.text = fsModel.getNewFileName(fileOverwriteDialog.sourcePath, fileOverwriteDialog.targetPath);
                    }
                }
            }
        }

        property bool isCopy
        property string sourcePath
        property string sourcePathName
        property string targetPath

        onOpened: {
            fileName.forceActiveFocus();
            // TODO Find new name from model.
            fileName.text = sourcePathName;
        }

        onClosed: {
            fileName.text = "";
            overwriteFile.checked = false;
        }

        onConfirm: {
            // If paste to current folder, targetPath is ended with / already.
            // If paste to selected folder, targetPath is not ended with /.
            var res = false;
            var actualTargetPath = cloudDriveModel.getRemotePath(selectedCloudType, targetPath, fileName.text);
            if (isCopy) {
                isBusy = true;
                cloudDriveModel.copyFile(selectedCloudType, selectedUid, "", sourcePath, "", actualTargetPath);
            } else {
                isBusy = true;
                cloudDriveModel.moveFile(selectedCloudType, selectedUid, "", sourcePath, "", actualTargetPath);
            }
        }

        onReject: {
            copyProgressDialog.close();
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
