import QtQuick 1.1
import com.nokia.symbian 1.1
import CloudDriveModel 1.0
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
        isBusy = true;
        cloudDriveModel.createFolder(selectedCloudType, selectedUid, "", remoteParentPath, newRemoteFolderName);
    }

    function renameRemotePath(remotePath, newName) {
        isBusy = true;
        cloudDriveModel.moveFile(selectedCloudType, selectedUid, "", remotePath, "", "", newName);
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
        remoteParentPath = (remoteParentPath == "") ? cloudDriveModel.getParentRemotePath(selectedCloudType, remotePath) : remoteParentPath;
        cloudDriveModel.browse(selectedCloudType, selectedUid, remoteParentPath);
    }

    function mailFileSlot(remotePath, remotePathName) {
        console.debug("cloudFolderPage mailFileSlot remotePath=" + remotePath);
        recipientSelectionDialog.refresh();
        cloudDriveModel.shareFileCaller = "mailFileSlot";
        cloudDriveModel.shareFile(selectedCloudType, selectedUid, remotePathName, remotePath);
    }

    function smsFileSlot(remotePath, remotePathName) {
        console.debug("cloudFolderPage smsFileSlot remotePath=" + remotePath);
        recipientSelectionDialog.refresh();
        cloudDriveModel.shareFileCaller = "smsFileSlot";
        cloudDriveModel.shareFile(selectedCloudType, selectedUid, remotePathName, remotePath);
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

        ToolButton {
            id: backButton
            iconSource: "toolbar-back"
            platformInverted: window.platformInverted
            flat: true
            onClicked: {
                if (cloudFolderView.state == "mark") {
                    cloudFolderView.state = "";
                } else {
                    // Specify local path to focus after cd to parent directory..
//                    cloudFolderView.focusLocalPath = cloudFolderModel.currentDir;
                    goUpSlot();
                }
            }
        }

        ToolButton {
            id: refreshButton
            iconSource: "toolbar-refresh"
            platformInverted: window.platformInverted
            flat: true
            visible: (cloudFolderView.state != "mark")
            onClicked: {
                // Force resume.
                cloudDriveModel.resumeNextJob();

                refreshSlot("refreshButton onClicked");
            }
        }

        ToolButton {
            id: cloudButton
            iconSource: (!window.platformInverted ? "cloud.svg" : "cloud_inverted.svg")
            platformInverted: window.platformInverted
            flat: true
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

        ToolButton {
            id: menuButton
            iconSource: "toolbar-menu"
            platformInverted: window.platformInverted
            flat: true
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

        TextIndicator {
            id: messageLoggerCounter
            text: (messageLoggerModel.newMessageCount > 0) ? (appInfo.emptyStr + qsTr("Message") + " " + messageLoggerModel.newMessageCount) : ""
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter

            onClicked: {
                pageStack.push(Qt.resolvedUrl("MessageLoggerPage.qml"));
            }
        }
    }

    NameFilterPanel {
        id: nameFilterPanel
        requestAsType: true
        anchors.bottom: parent.bottom
        anchors.bottomMargin: (inputContext.visible ? (inputContext.height - 60) : 0) // Symbian only

        onPrevious: {
            lastFindIndex = cloudFolderModel.findIndexByNameFilter(nameFilter, --lastFindIndex, true);
            console.debug("cloudFolderPage nameFilterPanel onPrevious nameFilter " + nameFilter + " lastFindIndex " + lastFindIndex);
            if (lastFindIndex > -1) {
                cloudFolderView.positionViewAtIndex(lastFindIndex, ListView.Beginning);
                cloudFolderView.currentIndex = lastFindIndex;
            }
        }

        onNext: {
            lastFindIndex = cloudFolderModel.findIndexByNameFilter(nameFilter, ++lastFindIndex, false);
            console.debug("cloudFolderPage nameFilterPanel onNext nameFilter " + nameFilter + " lastFindIndex " + lastFindIndex);
            if (lastFindIndex > -1) {
                cloudFolderView.positionViewAtIndex(lastFindIndex, ListView.Beginning);
                cloudFolderView.currentIndex = lastFindIndex;
            }
        }

        onOpened: {
            // Turn highlight on.
            cloudFolderView.highlightFollowsCurrentItem = true;
            lastFindIndex = cloudFolderView.currentIndex;
        }

        onClosed: {
            // Turn highlight off.
            cloudFolderView.highlightFollowsCurrentItem = false;
            cloudFolderView.currentIndex = -1;
        }
    }

    CloudFolderMenu {
        id: mainMenu
        disabledMenus: ["syncConnectedItems","syncCurrentFolder","sortByMenu"]

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
        onSetNameFilter: {
            nameFilterPanel.open();
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
            window.quitSlot();
        }

        function isMenuItemVisible(menuItem) {
            // Validate each menu logic if it's specified, otherwise it's visible.
            if (menuItem.name == "paste") {
                return (clipboard.count > 0);
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

        function findIndexByRemotePathName(remotePathName) {
            for (var i=0; i<cloudFolderModel.count; i++) {
                if (cloudFolderModel.get(i).name == remotePathName) {
                    return i;
                }
            }

            return -1;
        }

        function findIndexByNameFilter(nameFilter, startIndex, backward) {
            backward = (!backward) ? false : true;
            var rx = new RegExp(nameFilter, "i");
            if (backward) {
                startIndex = (!startIndex) ? (cloudFolderModel.count - 1) : startIndex;
                startIndex = (startIndex < 0 || startIndex >= cloudFolderModel.count) ? (cloudFolderModel.count - 1) : startIndex;
                for (var i=startIndex; i>=0; i--) {
                    if (rx.test(cloudFolderModel.get(i).name)) {
                        return i;
                    }
                }
            } else {
                startIndex = (!startIndex) ? 0 : startIndex;
                startIndex = (startIndex < 0 || startIndex >= cloudFolderModel.count) ? 0 : startIndex;
                for (var i=startIndex; i<cloudFolderModel.count; i++) {
                    if (rx.test(cloudFolderModel.get(i).name)) {
                        return i;
                    }
                }
            }

            return -1;
        }

        function getNewFileName(remotePathName) {
//            console.debug("cloudFolderModel getNewFileName " + remotePathName);

            var foundIndex = findIndexByRemotePathName(remotePathName);
            if (foundIndex > -1) {
                var newRemotePathName = "";
                var tokens = remotePathName.split(".", 2);
//                console.debug("cloudFolderModel getNewFileName tokens [" + tokens + "]");

                if (tokens[0].lastIndexOf(qsTr("_Copy")) > -1) {
                    var nameTokens = tokens[0].split(qsTr("_Copy"), 2);
//                    console.debug("cloudFolderModel getNewFileName nameTokens [" + nameTokens + "]");

                    if (nameTokens.length > 1 && !isNaN(parseInt(nameTokens[1]))) {
                        newRemotePathName += nameTokens[0] + qsTr("_Copy") + (parseInt(nameTokens[1]) + 1);
                    } else {
                        newRemotePathName += nameTokens[0] + qsTr("_Copy") + "2";
                    }
                } else {
                    newRemotePathName += tokens[0] + qsTr("_Copy");
                }

                if (tokens.length > 1) {
                    newRemotePathName += "." + tokens[1];
                }

                return getNewFileName(newRemotePathName);
            } else {
                return remotePathName;
            }
        }
    }

    ListView {
        id: cloudFolderView
        width: parent.width
        height: parent.height - currentPath.height - (nameFilterPanel.visible ? nameFilterPanel.height : 0) - (inputContext.visible ? (inputContext.height - 60) : 0) // Symbian only
        anchors.top: currentPath.bottom
        highlightRangeMode: ListView.NoHighlightRange
        highlightFollowsCurrentItem: false
        highlightMoveDuration: 1
        highlightMoveSpeed: 4000
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
        clip: true
        focus: true
        pressDelay: 100
        model: cloudFolderModel
        delegate: cloudItemDelegate
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
                width: 150
                height: width
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

    ScrollDecorator {
        id: scrollbar
        flickableItem: cloudFolderView
    }

    Component {
        id: cloudItemDelegate

        CloudListItem {
            id: listItem
            listViewState: (cloudFolderView.state ? cloudFolderView.state : "")
            syncIconVisible: cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, absolutePath)
            syncIconSource: (isRunning) ? "cloud_wait.svg" : "cloud.svg"
            actionIconSource: (clipboard.count > 0) ? appInfo.emptySetting+clipboard.getActionIcon(absolutePath, cloudDriveModel.getCloudName(selectedCloudType), selectedUid) : ""

            // Override to support cloud items.
            function getIconSource() {
                var viewableImageFileTypes = ["JPG", "PNG", "SVG"];
                var viewableTextFileTypes = ["TXT", "HTML"];

                if (isDir) {
                    return "folder_list.svg";
                } else if (viewableImageFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                    var showThumbnail = appInfo.getSettingBoolValue("CloudFolderPage.thumbnail.enabled", false);
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
        ringRadius: 65
        buttonRadius: 25
        timeout: appInfo.emptySetting+appInfo.getSettingValue("popup.timer.interval", 2) * 1000
        disabledButtons: ["print","editFile","cloud","bluetooth"]

        function isButtonVisibleCallback(buttonName) {
            if (buttonName === "sync") {
                return cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, selectedFilePath);
            } else if (buttonName === "paste") {
                return (clipboard.count > 0);
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

        onMailFile: {
            mailFileSlot(srcFilePath, selectedFileName);
        }

        onSmsFile: {
            smsFileSlot(srcFilePath, selectedFileName);
        }
    }

    FileActionDialog {
        id: fileActionDialog
        selectedCloudType: cloudFolderPage.selectedCloudType
        selectedUid: cloudFolderPage.selectedUid
        remoteParentPath: cloudFolderPage.remoteParentPath

        onConfirm: {
            // It always replace existing names.
            cloudDriveModel.suspendNextJob();

            // TODO Copy/Move/Delete all files from clipboard.
            // Action is {copy, cut, delete}
            var res = true;
            for (var i=0; i<clipboard.count; i++) {
                var action = clipboard.get(i).action;
                var sourcePath = clipboard.get(i).sourcePath;
                var sourcePathName = (isOverwrite && i == 0) ? newFileName : (clipboard.get(i).sourcePathName ? clipboard.get(i).sourcePathName : cloudDriveModel.getFileName(sourcePath));
                var actualTargetPath = cloudDriveModel.getRemotePath(selectedCloudType, targetPath, sourcePathName);

                console.debug("cloudFolderPage fileActionDialog onConfirm clipboard type " + clipboard.get(i).type + " uid " + clipboard.get(i).uid + " action " + action + " sourcePathName " + sourcePathName + " sourcePath " + sourcePath + " targetPath " + targetPath + " actualTargetPath " + actualTargetPath);
                if (["copy","cut"].indexOf(action) > -1 && !clipboard.get(i).type) {
                    isBusy = true;
                    if (cloudDriveModel.isDir(sourcePath)) {
                        // TODO Check if cloud client's remotePath is absolute path.
                        if (cloudDriveModel.isRemoteAbsolutePath(selectedCloudType)) {
                            cloudDriveModel.syncFromLocal(selectedCloudType, selectedUid, sourcePath, actualTargetPath, -1, true);
                        } else {
                            cloudDriveModel.syncFromLocal_Block(selectedCloudType, selectedUid, sourcePath, actualTargetPath, -1, true);
                        }
                    } else {
                        cloudDriveModel.filePut(selectedCloudType, selectedUid, sourcePath, actualTargetPath, -1);
                    }
                    res = true;
                } else if (["copy","cut"].indexOf(action) > -1 && (cloudDriveModel.getClientType(clipboard.get(i).type) != selectedCloudType || clipboard.get(i).uid != selectedUid) ) {
                    console.debug("cloudFolderPage fileActionDialog onConfirm migrate clipboard type " + clipboard.get(i).type + " uid " + clipboard.get(i).uid + " sourcePath " + sourcePath + " actualTargetPath " + actualTargetPath);
                    cloudDriveModel.migrateFile(cloudDriveModel.getClientType(clipboard.get(i).type), clipboard.get(i).uid, sourcePath, selectedCloudType, selectedUid, targetPath, sourcePathName);
                    res = true;
                } else if (action == "copy" && clipboard.get(i).type) {
                    isBusy = true;
                    // TODO Support sourcePathName as newRemotePathName.
                    cloudDriveModel.copyFile(cloudDriveModel.getClientType(clipboard.get(i).type), clipboard.get(i).uid, "", sourcePath, "", actualTargetPath, sourcePathName);
                    res = true;
                } else if (action == "cut" && clipboard.get(i).type) {
                    isBusy = true;
                    // TODO Support sourcePathName as newRemotePathName.
                    cloudDriveModel.moveFile(cloudDriveModel.getClientType(clipboard.get(i).type), clipboard.get(i).uid, "", sourcePath, "", actualTargetPath);
                    res = true;
                } else if (action == "delete" && clipboard.get(i).type) {
                    isBusy = true;
                    cloudDriveModel.deleteFile(cloudDriveModel.getClientType(clipboard.get(i).type), clipboard.get(i).uid, "", sourcePath);
                    res = true;
                } else {
                    console.debug("cloudFolderPage fileActionDialog onConfirm invalid action " + action);
                    res = false;
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

    NewFolderDialog {
        id: newFolderDialog
        titleText: appInfo.emptyStr+qsTr("New folder")
        folderOnly: true

        onConfirm: {
            if (nameField.text !== "") {
                createRemoteFolder(nameField.text.trim());
            }
        }
    }

    RenameDialog {
        id: renameDialog

        onConfirm: {
            if (nameField.text != "" && nameField.text != sourcePathName) {
                renameRemotePath(sourcePath, nameField.text);
            }
        }
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
        }
    }

    Component.onCompleted: {
        console.debug(Utility.nowText() + " cloudFolderPage onCompleted");
        window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("CloudFolderPage"), 0.1);

        // Proceeds queued jobs during constructions.
        refreshSlot("cloudFolderPage onStatusChanged");
    }
}
