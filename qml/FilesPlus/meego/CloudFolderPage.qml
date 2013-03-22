import QtQuick 1.1
import com.nokia.meego 1.0
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
        if (cloudDriveModel.isRemoteRoot(selectedCloudType, selectedUid, remoteParentPath) || remoteParentParentPath == "") {
            pageStack.pop(cloudFolderPage);
        } else {
            changeRemotePath(remoteParentParentPath);
        }
    }

    function parseCloudDriveMetadataJson(jsonText) {
        originalRemotePath = (originalRemotePath == "") ? remotePath : originalRemotePath;

        cloudFolderModel.clear();
        var responseJson = cloudDriveModel.parseCloudDriveMetadataJson(selectedCloudType, selectedUid, originalRemotePath, jsonText,  cloudFolderModel);

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

    function syncCurrentFolderSlot() {
        var isConnected = cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, remoteParentPath);
        console.debug("cloudFolderPage syncCurrentFolder remoteParentPath " + remoteParentPath + " isConnected " + isConnected);
        if (isConnected) {
            cloudDriveModel.syncItemByRemotePath(selectedCloudType, selectedUid, remoteParentPath);
        }
    }

    function syncConnectedItemsSlot() {
        // Suspend for queuing.
        cloudDriveModel.suspendNextJob();

        for (var i=0; i<cloudFolderModel.count; i++) {
            var remotePath = cloudFolderModel.get(i).absolutePath;
            var isConnected = cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, remotePath);
            console.debug("cloudFolderPage synconnectedItemsSlot remotePath " + remotePath + " isConnected " + isConnected);
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

    function updateItemSlot(jobJson, caller) {
        if (!jobJson) return;
//        console.debug("cloudFolderPage updateItemSlot caller " + caller + " jobJson " + JSON.stringify(jobJson));

        if (jobJson.remote_file_path == "") return;

        if (jobJson.type == selectedCloudType && jobJson.uid == selectedUid) {
            var i = cloudFolderModel.findIndexByRemotePath(jobJson.remote_file_path);
//            console.debug("cloudFolderPage updateItemSlot caller " + caller + " jobJson " + JSON.stringify(jobJson) + " model index " + i);
            if (i >= 0) {
                cloudFolderModel.set(i, { isRunning: jobJson.is_running, isConnected: cloudDriveModel.isRemotePathConnected(jobJson.type, jobJson.uid, jobJson.remote_file_path) });
            }
        }
    }

    function refreshItemAfterFilePutSlot(jobJson) {
        if (!jobJson) return;
        console.debug("cloudFolderPage refreshItemAfterFilePutSlot jobJson " + JSON.stringify(jobJson));

        if (jobJson.type == selectedCloudType && jobJson.uid == selectedUid) {
            if (jobJson.remote_file_path == remoteParentPath) {
                refreshSlot("cloudFolderPage refreshItemAfterFilePutSlot");
            } else if (jobJson.remote_file_path == "/" && remoteParentPath == "") {
                refreshSlot("cloudFolderPage refreshItemAfterFilePutSlot Dropbox");
            }
        } else if (jobJson.target_type == selectedCloudType && jobJson.target_uid == selectedUid) {
            if (jobJson.new_remote_file_path == remoteParentPath) {
                refreshSlot("cloudFolderPage refreshItemAfterFilePutSlot");
            } else if (jobJson.new_remote_file_path == "/" && remoteParentPath == "") {
                refreshSlot("cloudFolderPage refreshItemAfterFilePutSlot Dropbox");
            }
        }
    }

    function refreshItem(remotePath) {
        var timestamp = (new Date()).getTime();
        var i = cloudFolderModel.findIndexByRemotePath(remotePath);
        console.debug("cloudFolderPage refreshItem i " + i);
        if (i >= 0) {
            cloudFolderModel.setProperty(i, "timestamp", timestamp);
        }
    }

    tools: ToolBarLayout {
        id: toolBarLayout

        ToolBarButton {
            id: backButton
            buttonIconSource: "toolbar-back"
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

        ToolBarButton {
            id: refreshButton
            buttonIconSource: "toolbar-refresh"
            visible: (cloudFolderView.state != "mark")
            onClicked: {
                // Force resume.
                cloudDriveModel.resumeNextJob();

                refreshSlot("refreshButton onClicked");
            }
        }

        ToolBarButton {
            id: cloudButton
            buttonIconSource: (theme.inverted ? "cloud.svg" : "cloud_inverted.svg")
            visible: (cloudFolderView.state != "mark")

            TextIndicator {
                id: cloudButtonIndicator
                color: "#00AAFF"
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10
            }

            onClicked: {
                cloudMenu.open();
            }
        }

        ToolBarButton {
            id: menuButton
            buttonIconSource: "toolbar-view-menu"
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
        text: cloudDriveModel.isRemoteAbsolutePath(selectedCloudType) ? cloudDriveModel.getPathFromUrl(remoteParentPath) : remoteParentPathName
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

    NameFilterPanel {
        id: nameFilterPanel
        requestAsType: true
        anchors.bottom: parent.bottom

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
//            cloudFolderView.highlightFollowsCurrentItem = true;
            lastFindIndex = cloudFolderView.currentIndex;
        }

        onClosed: {
            // Turn highlight off.
//            cloudFolderView.highlightFollowsCurrentItem = false;
            cloudFolderView.currentIndex = -1;
        }
    }

    CloudFolderMenu {
        id: mainMenu
        disabledMenus: ["syncConnectedItems","syncCurrentFolder"]

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
        onOpenSortByMenu: {
            sortByMenu.open();
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

    SortByMenu {
        id: sortByMenu

        onSelectSort: {
            console.debug("sortByMenu setSortFlag flag=" + flag);
            cloudDriveModel.sortFlag = flag;
        }

        onStatusChanged: {
            if (status == DialogStatus.Open) {
                // TODO set sortFlag before status=Open
                sortFlag = cloudDriveModel.sortFlag;
            }
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

    CloudMenu {
        id: cloudMenu

        onSyncCurrentFolder: {
            syncCurrentFolderSlot();
        }
        onSyncConnectedItems: {
            syncConnectedItemsSlot();
        }
        onShowCloudDriveJobs: {
            pageStack.push(Qt.resolvedUrl("CloudDriveJobsPage.qml"));
        }

        function isMenuItemVisible(menuItem) {
            // Validate each menu logic if it's specified, otherwise it's visible.
            if (menuItem.name == "syncCurrentFolder") {
                return !cloudDriveModel.isRemoteRoot(selectedCloudType, selectedUid, remoteParentPath) && cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, remoteParentPath);
            } else {
                return true;
            }
        }
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
        height: parent.height - currentPath.height - (nameFilterPanel.visible ? nameFilterPanel.height : 0)
        anchors.top: currentPath.bottom
        highlightRangeMode: ListView.NoHighlightRange
        highlightFollowsCurrentItem: true
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
        pressDelay: 200
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
                // Hide highlight and popupTool.
                currentItem.pressed = false;
                currentIndex = -1;
                popupToolPanel.visible = false;
            }
        }

        QuickScrollPanel {
            id: quickScrollPanel
            listView: parent
            indicatorBarTitle: (modelIndex < 0) ? ""
                               : ( cloudDriveModel.sortFlag == CloudDriveModel.SortByTime
                                  ? Qt.formatDateTime(cloudFolderModel.get(modelIndex).lastModified, "d MMM yyyy")
                                  : cloudFolderModel.get(modelIndex).name )
            inverted: !theme.inverted
            scrollBarWidth: 80
            indicatorBarHeight: 80
            scrollBarColor: "grey"
            indicatorWidth: 5
        }
    }

    Component {
        id: cloudItemDelegate

        CloudListItem {
            id: listItem
            listViewState: (cloudFolderView.state ? cloudFolderView.state : "")
            syncIconVisible: isConnected
            syncIconSource: (isRunning) ? "cloud_wait.svg" : "cloud.svg"
            actionIconSource: (clipboard.count > 0) ? appInfo.emptySetting+clipboard.getActionIcon(absolutePath, cloudDriveModel.getCloudName(selectedCloudType), selectedUid) : ""
            listItemIconSource: appInfo.emptySetting+listItem.getIconSource(timestamp)

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

            function getMediaSource(source, remoteFilePath) {
                if (source) {
                    return source;
                } else {
                    return cloudDriveModel.media(selectedCloudType, selectedUid, remoteFilePath);
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

                        isBusy = true;

                        // Check if it's viewable.
                        var url;
                        if (cloudDriveModel.isViewable(selectedCloudType)) {
                            var viewableImageFileTypes = ["JPG", "PNG", "GIF", "SVG"];
                            var viewableTextFileTypes = ["TXT", "HTML", "LOG", "CSV", "CONF", "INI"];
                            if (viewableImageFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                                // NOTE ImageViewPage will populate ImageViewModel with mediaUrl.
                                pageStack.push(Qt.resolvedUrl("ImageViewPage.qml"),
                                               { model: cloudFolderModel, selectedCloudType: selectedCloudType, selectedUid: selectedUid, selectedFilePath: absolutePath });
                            } else if (viewableTextFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                                // View with internal web viewer.
                                url = getMediaSource(source, absolutePath);
                                appInfo.addToClipboard(url);
                                pageStack.push(Qt.resolvedUrl("WebViewPage.qml"));
                            } else {
                                // Open with system web browser.
                                url = getMediaSource(source, absolutePath);
                                if (url != "") {
                                    Qt.openUrlExternally(url);
                                }
                            }
                        } else {
                            // File is not viewable but may be browsable.
                            // TODO Provide alternate link.
                            url = getMediaSource(source, absolutePath);
                            if (url != "") {
                                Qt.openUrlExternally(url);
                            }
                        }

                        isBusy = false;
                    }
                }
            }

            onListItemIconError: {
                if (cloudDriveModel.isImageUrlCachable(selectedCloudType)) {
                    cloudDriveModel.cacheImage(absolutePath, thumbnail, 48, 48, cloudFolderPage.name); // Use default icon size.
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
        disabledButtons: ["editFile","cloud","bluetooth"]

        function isButtonVisibleCallback(buttonName) {
            if (buttonName === "sync") {
                return cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, selectedFilePath);
            } else if (buttonName === "paste") {
                return (clipboard.count > 0);
            } else if (buttonName == "print") {
                return !popupToolPanel.isDir && cloudDriveModel.isViewable(selectedCloudType);
            }

            return true;
        }

        onOpened: {
//            console.debug("popupToolRing onOpened");
        }

        onClosed: {
//            console.debug("popupToolRing onClosed");
//            // Workaround to hide highlight.
            cloudFolderView.currentIndex = -1;
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

        onPrintFile: {
            // Check if it's viewable.
            if (!cloudDriveModel.isViewable(selectedCloudType)) return;

            isBusy = true;

            var url;
            if (!cloudFolderModel.get(srcItemIndex).source) {
                // Request media to get URL.
                url = cloudDriveModel.media(selectedCloudType, selectedUid, srcFilePath);
            } else {
                // Get media URL.
                url = cloudFolderModel.get(srcItemIndex).source;
            }
            if (url && url != "") {
                gcpClient.printURLSlot(url);
            }

            isBusy = false;
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

        onShowInfo: {
            filePropertiesDialog.show(srcItemIndex);
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
                    if (cloudDriveModel.isDir(sourcePath)) {
                        // Sync from local into target path (remote parent path).
                        cloudDriveModel.syncFromLocal(selectedCloudType, selectedUid, sourcePath, targetPath, -1, true);
                    } else {
                        cloudDriveModel.filePut(selectedCloudType, selectedUid, sourcePath, targetPath, sourcePathName, -1);
                    }
                    res = true;
                } else if (["copy","cut"].indexOf(action) > -1 && (cloudDriveModel.getClientType(clipboard.get(i).type) != selectedCloudType || clipboard.get(i).uid != selectedUid) ) {
                    console.debug("cloudFolderPage fileActionDialog onConfirm migrate clipboard type " + clipboard.get(i).type + " uid " + clipboard.get(i).uid + " sourcePath " + sourcePath + " actualTargetPath " + actualTargetPath);
                    cloudDriveModel.migrateFile(cloudDriveModel.getClientType(clipboard.get(i).type), clipboard.get(i).uid, sourcePath, selectedCloudType, selectedUid, targetPath, sourcePathName);
                    res = true;
                } else if (action == "copy" && clipboard.get(i).type) {
                    cloudDriveModel.copyFile(cloudDriveModel.getClientType(clipboard.get(i).type), clipboard.get(i).uid, "", sourcePath, "", targetPath, sourcePathName);
                    res = true;
                } else if (action == "cut" && clipboard.get(i).type) {
                    cloudDriveModel.moveFile(cloudDriveModel.getClientType(clipboard.get(i).type), clipboard.get(i).uid, "", sourcePath, "", targetPath, sourcePathName);
                    res = true;
                } else if (action == "delete" && clipboard.get(i).type) {
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

    FilePropertiesDialog {
        id: filePropertiesDialog
        isCloudFolder: true

        function show(index) {
            selectedIndex = index;
            selectedItem = cloudFolderModel.get(selectedIndex);
            populateCloudItemModel();
            open();
        }

        function populateCloudItemModel() {
            cloudItemModel.clear();
            if (selectedItem) {
                var cloudItems = Utility.createJsonObj(cloudDriveModel.getItemListJsonByRemotePath(selectedCloudType, selectedUid, selectedItem.absolutePath));
                for (var i = 0; i < cloudItems.length; i++) {
                    var cloudItem = cloudItems[i];
                    var modelItem = { absolutePath: (cloudItem.local_path ? cloudItem.local_path : qsTr("Not available")) };
                    cloudItemModel.append(modelItem);
                }
            }
        }

        onSyncAll: {
            cloudDriveModel.syncItemByRemotePath(selectedCloudType, selectedUid, selectedItem.absolutePath);
            close();
        }
        onDisconnect: {
            cloudDriveModel.disconnect(type, uid, absolutePath);
            close();
        }
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            // Request cloud job queue status.
            cloudDriveModel.requestJobQueueStatus();
        }
    }

    Component.onCompleted: {
        console.debug(Utility.nowText() + " cloudFolderPage onCompleted");
        window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("CloudFolderPage"), 0.1);

        // Proceeds queued jobs during constructions.
        refreshSlot("cloudFolderPage onStatusChanged");
    }
}
