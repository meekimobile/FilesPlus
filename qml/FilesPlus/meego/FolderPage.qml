import QtQuick 1.1
import com.nokia.meego 1.0
import Charts 1.0
import FolderSizeItemListModel 1.0 // For enum reference.
import CloudDriveModel 1.0 // For enum reference.
import "Utility.js" as Utility

Page {
    id: folderPage

    property string name: "folderPage"

    states: [
        State {
            name: "chart"
            when: flipable1.flipped
        },
        State {
            name: "list"
            when: !flipable1.flipped
        }
    ]

    function toggleSortFlag() {
        if (state == "chart") {
            fsModel.setSortFlag(FolderSizeItemListModel.SortBySize, false);
        } else {
            // TODO revert back to stored sortFlag.
            fsModel.revertSortFlag();
        }
    }

    Component.onCompleted: {
        console.debug(Utility.nowText() + " folderPage onCompleted");
        window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("FolderPage"), 0.1);
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            // Request cloud job queue status.
            cloudDriveModel.requestJobQueueStatus();

            // Ask whether user want to change to have full access to Dropbox.
            if (!appInfo.hasSettingValue("dropbox.fullaccess.enabled")) {
                if (cloudDriveModel.isAuthorized(CloudDriveModel.Dropbox)) {
                    // Show confirm dialog and return.
                    showDropboxFullAccessConfirmationSlot();
                    return;
                } else {
                    // New user always get full access.
                    appInfo.setSettingValue("dropbox.fullaccess.enabled", true);
                }
            }

            // Check for cloud data migration.
            if (!appInfo.hasSettingValue("cloudItems.migration.confirmation") && cloudDriveModel.getCloudDriveItemsCount() > 0) {
                cloudItemsMigrationConfirmation.open();
            }
        }
    }

    tools: toolBarLayout

    ToolBarLayout {
        id: toolBarLayout

        ToolBarButton {
            id: backButton
            buttonIconSource: "toolbar-back"
            onClicked: {
                if (fsListView.state == "mark") {
                    fsListView.state = "";
                    fsListView.unmarkAll();
                } else {
                    // Specify local path to focus after cd to parent directory..
                    fsListView.focusLocalPath = fsModel.currentDir;
                    goUpSlot();
                }
            }
        }

        ToolBarButton {
            id: refreshButton
            buttonIconSource: "toolbar-refresh"
            visible: (fsListView.state == "")

            Timer {
                id: refreshButtonTimer
                interval: 100
                repeat: true
                running: false
                onTriggered: {
                    refreshButton.rotation = 360 + (refreshButton.rotation - 12);
                }
            }

            onPressAndHold: {
                resetCacheSlot();
            }
            onClicked: {
                // Force resume.
                fsModel.resumeNextJob();
                cloudDriveModel.resumeNextJob();

                refreshSlot("refreshButton");
            }
        }

        ToolBarButton {
            id: flipButton
            buttonIconSource: (folderPage.state != "list") ? (theme.inverted ? "list.svg" : "list_inverted.svg") : (theme.inverted ? "chart.svg" : "chart_inverted.svg")
            visible: (fsListView.state == "")

            Component.onCompleted: {
                flipButton.clicked.connect(flipSlot);
            }
        }

        ToolBarButton {
            id: cloudButton
            buttonIconSource: (theme.inverted ? "cloud.svg" : "cloud_inverted.svg")
            visible: (fsListView.state == "")

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

                if (fsListView.state == "mark") {
                    if (!fsListView.isAnyItemChecked()) {
                        markAllMenu.open();
                    } else {
                        markMenu.open();
                    }
                } else if (folderPage.state == "list") {
                    mainMenu.open();
                } else if (folderPage.state == "chart") {
                    chartMenu.open();
                }
            }
        }
    }

    MainMenu {
        id: mainMenu
        disabledMenus: ["syncConnectedItems","syncFolderMenuItem"]

        onPaste: {
            fileActionDialog.targetPath = fsModel.currentDir;
            fileActionDialog.targetPathName = fsModel.getFileName(fsModel.currentDir);
            fileActionDialog.open();
        }
        onOpenMarkMenu: {
            fsListView.state = "mark";
        }
        onClearClipboard: {
            clipboard.clear();
        }
        onNewFolder: {
            newFolderDialog.open();
        }
        onSyncConnectedItems: {
            syncConnectedItemsSlot();
        }
        onSyncCurrentFolder: {
            syncFileSlot(fsModel.currentDir, -1);
        }
        onSetNameFilter: {
            nameFilterPanel.open();
        }
        onDrives: {
            // Flip back to list view, then pop folderPage.
            flipable1.flipped = false;
            pageStack.pop(folderPage);
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
                return clipboard.count > 0;
            } else if (menuItem.name == "clearClipboard") {
                return clipboard.count > 0;
            } else if (menuItem.name == "markMenu") {
                return fsListView.state != "mark";
            } else if (menuItem.name == "syncCurrentFolder") {
                return !fsModel.isRoot() && cloudDriveModel.canSync(fsModel.currentDir);
            } else {
                return true;
            }
        }
    }

    ChartMenu {
        id: chartMenu

        onDrives: {
            // Flip back to list view, then pop folderPage.
            flipable1.flipped = false;
            pageStack.pop(folderPage);
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
    }

    SortByMenu {
        id: sortByMenu

        onSelectSort: {
            console.debug("sortByMenu setSortFlag flag=" + flag);
            fsModel.sortFlag = flag;
        }

        onStatusChanged: {
            if (status == DialogStatus.Open) {
                // TODO set sortFlag before status=Open
                sortFlag = fsModel.sortFlag;
            }
        }
    }

    MarkMenu {
        id: markMenu

        onMarkAll: {
            if (isMarkAll) {
                fsListView.markAll();
            } else {
                fsListView.unmarkAll();
            }
            isMarkAll = !isMarkAll;
        }
        onCopyMarkedItems: {
            fsListView.copyMarkedItems();
            fsListView.state = "";
            fsListView.currentIndex = -1;
        }
        onCutMarkedItems: {
            fsListView.cutMarkedItems();
            fsListView.state = "";
            fsListView.currentIndex = -1;
        }
        onDeleteMarkedItems: {
            fsListView.deleteMarkedItems();
            fsListView.state = "";
            fsListView.currentIndex = -1;
        }
        onSyncMarkedItems: {
            fsListView.syncMarkedItems();
            fsListView.state = "";
            fsListView.currentIndex = -1;
        }
        onStatusChanged: {
            if (status == DialogStatus.Opening) {
                isMarkAll = !fsListView.areAllItemChecked();
            }
        }
    }

    MarkAllMenu {
        id: markAllMenu

        onMarkAll: {
            fsListView.markAll();
        }
        onMarkAllFiles: {
            fsListView.markAllFiles();
        }
        onMarkAllFolders: {
            fsListView.markAllFolders();
        }
    }

    CloudMenu {
        id: cloudMenu

        onSyncConnectedItems: {
            syncConnectedItemsSlot();
        }
        onSyncCurrentFolder: {
            syncFileSlot(fsModel.currentDir, -1);
        }
        onShowCloudDriveJobs: {
            pageStack.push(Qt.resolvedUrl("CloudDriveJobsPage.qml"));
        }

        function isMenuItemVisible(menuItem) {
            // Validate each menu logic if it's specified, otherwise it's visible.
            if (menuItem.name == "syncCurrentFolder") {
                return !fsModel.isRoot() && cloudDriveModel.canSync(fsModel.currentDir);
            } else {
                return true;
            }
        }
    }

    function refreshSlot(caller) {
        caller = (!caller) ? "folderPage refreshSlot" : caller;
        fsModel.refreshDir(caller, false);
    }

    function resetCacheSlot() {
        resetCacheConfirmation.open();
    }

    function goUpSlot() {
        nameFilterPanel.close();
        fsModel.nameFilters = [];
        quickScrollPanel.visible = false;

        if (fsModel.isRoot()) {
            // Flip back to list view, then pop folderPage.
            flipable1.flipped = false;
            pageStack.pop(folderPage);
        } else {
            if (state == "chart") {
                fsModel.changeDir("..", FolderSizeItemListModel.SortBySize);
            } else {
                fsModel.changeDir("..");
            }
        }
    }

    function changeDirSlot(name, sortBy) {
        nameFilterPanel.close();
        fsModel.nameFilters = [];
        quickScrollPanel.visible = false;

        if (sortBy) {
            fsModel.changeDir(name, sortBy);
        } else {
            fsModel.changeDir(name);
        }
    }

    function flipSlot() {
        nameFilterPanel.close();

        flipable1.flipped = !flipable1.flipped;
    }

    function orientationChangeSlot() {
        if (pieChartView && folderPage.state == "chart") {
            pieChartView.refreshItems();
        }
    }

    function activateSlot() {
        if (pieChartView && folderPage.state == "chart") {
            pieChartView.refreshItems(true);
        }
    }

    function syncFileSlot(srcFilePath, selectedIndex, operation) {
        console.debug("folderPage syncFileSlot srcFilePath=" + srcFilePath);

        // Default operation = CloudDriveModel.Metadata
        if (!operation) operation = CloudDriveModel.Metadata;

        if (!cloudDriveModel.canSync(srcFilePath)) return;

        if (!cloudDriveModel.isAuthorized()) {
            logInfo(qsTr("Sync with Cloud"),
                    qsTr("FilesPlus syncs your files via cloud storage service.\nYou will be redirected to cloud account page."),
                    2000);

            pageStack.push(Qt.resolvedUrl("CloudDriveAccountsPage.qml"));
        } else {
            // [/] impl. in DropboxClient to store item(DropboxClient, uid, filePath, jsonObj(msg).rev) [Done on FilePutRely]
            // [/] On next metadata fetching. If rev is changed, sync to newer rev either put or get.
            // [/] Syncing folder must queue each get/put jobs (by using ThreadPool).
            uidDialog.localPath = srcFilePath;
            uidDialog.selectedModelIndex = selectedIndex;
            uidDialog.operation = operation;
            uidDialog.open();
        }
    }

    function syncConnectedItemsSlot(onlyDirty) {
        cloudDriveModel.suspendNextJob();

        for (var i=0; i<fsModel.count; i++) {
            var localPath = fsModel.getProperty(i, FolderSizeItemListModel.AbsolutePathRole);
            var lastModified = fsModel.getProperty(i, FolderSizeItemListModel.LastModifiedRole);
            var isConnected = cloudDriveModel.isConnected(localPath);
            var isSyncing = cloudDriveModel.isSyncing(localPath);
            var isDirty = cloudDriveModel.isDirty(localPath, lastModified);
//            console.debug("folderPage synconnectedItemsSlot localPath " + localPath + " isConnected " + isConnected + " isSyncing " + isSyncing);
            if (isConnected && !isSyncing) {
                if (!onlyDirty || isDirty) {
                    cloudDriveModel.syncItem(localPath);
                    console.debug("folderPage synconnectedItemsSlot localPath " + localPath + " isConnected " + isConnected + " is queued for syncing.");
                }
            }
        }

        cloudDriveModel.resumeNextJob();
    }

    function syncClipboardItems() {
        uidDialog.localPath = "";
        uidDialog.operation = CloudDriveModel.Metadata;
        uidDialog.open();
    }

    function unsyncFileSlot(srcFilePath, selectedIndex) {
        console.debug("folderPage unsyncFileSlot srcFilePath=" + srcFilePath);

        if (!cloudDriveModel.isConnected(srcFilePath)) return;

        // Delete remote file/folder.
        uidDialog.localPath = srcFilePath;
        uidDialog.selectedModelIndex = selectedIndex;
        uidDialog.operation = CloudDriveModel.DeleteFile;
        uidDialog.open();
    }

    function disconnectFileSlot(srcFilePath, selectedIndex) {
        console.debug("folderPage disconnectFileSlot srcFilePath=" + srcFilePath);

        if (!cloudDriveModel.isConnected(srcFilePath)) return;

        // Delete remote file/folder.
        uidDialog.localPath = srcFilePath;
        uidDialog.selectedModelIndex = selectedIndex;
        uidDialog.operation = CloudDriveModel.Disconnect;
        uidDialog.open();
    }

    function browseRemoteFileSlot(srcFilePath, selectedIndex) {
        console.debug("folderPage browseRemoteFileSlot srcFilePath=" + srcFilePath);
        uidDialog.localPath = srcFilePath;
        uidDialog.selectedModelIndex = selectedIndex;
        uidDialog.operation = CloudDriveModel.Browse;
        uidDialog.open();
    }

    function scheduleSyncFileSlot(srcFilePath, selectedIndex) {
        console.debug("folderPage scheduleSyncFileSlot srcFilePath=" + srcFilePath);
        uidDialog.localPath = srcFilePath;
        uidDialog.selectedModelIndex = selectedIndex;
        uidDialog.operation = CloudDriveModel.ScheduleSync;
        uidDialog.open();
    }

    function uploadFileSlot(srcFilePath, selectedIndex) {
        console.debug("folderPage uploadFileSlot srcFilePath=" + srcFilePath);
        syncFileSlot(srcFilePath, selectedIndex, CloudDriveModel.FilePut);
    }

    function downloadFileSlot(srcFilePath, selectedIndex) {
        console.debug("folderPage downloadFileSlot srcFilePath=" + srcFilePath);
        syncFileSlot(srcFilePath, selectedIndex, CloudDriveModel.FileGet);
    }

    function mailFileSlot(srcFilePath, selectedIndex) {
        console.debug("folderPage mailFileSlot srcFilePath=" + srcFilePath);
        if (cloudDriveModel.isConnected(srcFilePath)) {
            uidDialog.localPath = srcFilePath;
            uidDialog.operation = CloudDriveModel.ShareFile;
            uidDialog.caller = "mailFileSlot";
            uidDialog.open();
        }
    }

    function smsFileSlot(srcFilePath, selectedIndex) {
        console.debug("folderPage smsFileSlot srcFilePath=" + srcFilePath);
        if (cloudDriveModel.isConnected(srcFilePath)) {
            uidDialog.localPath = srcFilePath;
            uidDialog.operation = CloudDriveModel.ShareFile;
            uidDialog.caller = "smsFileSlot";
            uidDialog.open();
        }
    }

    function bluetoothFileSlot(srcFilePath, selectedIndex) {
        console.debug("folderPage bluetoothFileSlot srcFilePath=" + srcFilePath);
        if (fsModel.isFile(srcFilePath)) {
            btSelectionDialog.srcFilePath = srcFilePath;
            if (btClient.isPowerOn) {
                btSelectionDialog.open();
            } else {
                if (appInfo.getSettingBoolValue("automatically.bluetooth.on", false)) {
                    btClient.powerOn();
                    btSelectionDialog.open();
                } else {
                    btPowerOnDialog.open();
                }
            }
        }
    }

    function cancelAllFolderSizeJobsSlot() {
        fsModel.cancelQueuedJobs();
        // Abort thread with rollbackFlag=false.
        fsModel.abortThread(false);
    }

    function showDropboxFullAccessConfirmationSlot() {
        dropboxFullAccessConfirmation.open();
    }

    function parseCloudDriveMetadataJson(jsonText) {
        cloudDrivePathDialog.parseCloudDriveMetadataJson(jsonText);
        cloudDrivePathDialog.open();
    }

    function updateCloudDrivePathDialogSlot(remotePath) {
        console.debug("folderPage updateCloudDrivePathDialogSlot cloudDrivePathDialog.status " + cloudDrivePathDialog.status + " DialogStatus.Open " + DialogStatus.Open);
        if (cloudDrivePathDialog.status == DialogStatus.Open) {
            if (remotePath) {
                cloudDrivePathDialog.selectedRemotePath = remotePath;
            }
            cloudDrivePathDialog.refresh();
        }
    }

    function closeCloudDrivePathDialogSlot() {
        cloudDrivePathDialog.isBusy = false;
        cloudDrivePathDialog.close();
    }

    function updateItemSlot(jobJson) {
        if (!jobJson) return;
        if (jobJson.local_file_path == "") return;

        // Show indicator on localPath up to root.
        var pathList = fsModel.getPathToRoot(jobJson.local_file_path);
        for(var i=0; i<pathList.length; i++) {
            var modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
//            console.debug("folderPage updateItemSlot " + pathList[i] + " modelIndex " + modelIndex + " jobJson.is_running " + jobJson.is_running);
            if (modelIndex == FolderSizeItemListModel.IndexOnCurrentDirButNotFound) {
                fsModel.clearIndexOnCurrentDir();
                modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
            }
            if (modelIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
                fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, jobJson.is_running);
                fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningOperationRole, FolderSizeItemListModel.SyncOperation);
                fsModel.refreshItem(modelIndex);
            }
        }

        // Show indicator on newLocalPath up to root.
        if (jobJson.new_local_file_path != "") {
            pathList = fsModel.getPathToRoot(jobJson.new_local_file_path);
            for(i=0; i<pathList.length; i++) {
                modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
//                console.debug("folderPage updateItemSlot " + pathList[i] + " modelIndex " + modelIndex + " jobJson.is_running " + jobJson.is_running);
                if (modelIndex == FolderSizeItemListModel.IndexOnCurrentDirButNotFound) {
                    fsModel.clearIndexOnCurrentDir();
                    modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
                }
                if (modelIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
                    fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, jobJson.is_running);
                    fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningOperationRole, FolderSizeItemListModel.SyncOperation);
                    fsModel.refreshItem(modelIndex);
                }
            }
        }
    }

    function updateItemProgressBarSlot(jobJson) {
        if (!jobJson) return;
        if (jobJson.local_file_path == "") return;

        // Update ProgressBar on listItem.
        var runningOperation = fsModel.mapToFolderSizeListModelOperation(jobJson.operation);
        var modelIndex = fsModel.getIndexOnCurrentDir(jobJson.local_file_path);
        if (modelIndex == FolderSizeItemListModel.IndexOnCurrentDirButNotFound) {
            fsModel.clearIndexOnCurrentDir();
            modelIndex = fsModel.getIndexOnCurrentDir(jobJson.local_file_path);
        }
        if (modelIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
            fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, jobJson.is_running);
            fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningOperationRole, runningOperation);
            fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningValueRole, jobJson.bytes);
            fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningMaxValueRole, jobJson.bytes_total);
            fsModel.refreshItem(modelIndex);
        }

        // Show indicator on parent up to root.
        // Skip i=0 as it's notified above already.
        var pathList = fsModel.getPathToRoot(jobJson.local_file_path);
        for(var i=1; i<pathList.length; i++) {
            modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
            if (modelIndex == FolderSizeItemListModel.IndexOnCurrentDirButNotFound) {
                fsModel.clearIndexOnCurrentDir();
                modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
            }
            if (modelIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
                fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, jobJson.is_running);
                fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningOperationRole, runningOperation);
                fsModel.refreshItem(modelIndex);
            }
        }
    }

    function refreshItemSlot(caller, localPath) {
//        console.debug("folderPage refreshItemSlot caller " + caller + " " + localPath);
        if (localPath) {
            fsModel.refreshItem(localPath);
        } else {
            fsModel.refreshItems();
        }
    }

    function refreshItemAfterFileGetSlot(localPath) {
        // Remove cache on target folders and its parents.
        fsModel.removeCache(localPath);

        // TODO Does it need to refresh to add gotten file to listview ?
        var index = fsModel.getIndexOnCurrentDir(localPath);
        if (index == FolderSizeItemListModel.IndexNotOnCurrentDir) {
            // do nothing.
        } else if (index == FolderSizeItemListModel.IndexOnCurrentDirButNotFound) {
            index = fsModel.addItem(localPath);
        } else {
            fsModel.refreshItem(index);
        }
    }

    function updateJobQueueCount(runningJobCount, jobQueueCount) {
        // Update (runningJobCount + jobQueueCount) on cloudButton.
        cloudButtonIndicator.text = ((runningJobCount + jobQueueCount) > 0) ? (runningJobCount + jobQueueCount) : "";
    }

    function updateMigrationProgressSlot(type, uid, localFilePath, remoteFilePath, count, total) {
        if (migrateProgressDialog.status != DialogStatus.Open) {
            migrateProgressDialog.indeterminate = false;
            migrateProgressDialog.min = 0;
            migrateProgressDialog.open();
        }
        migrateProgressDialog.source = localFilePath;
        migrateProgressDialog.value = count;
        migrateProgressDialog.max = total;
    }

    function refreshBeginSlot() {
        console.debug("folderPage refreshBeginSlot");
//            window.state = "busy";

        fsListView.lastCenterIndex = fsListView.indexAt(0, fsListView.contentY + (fsListView.height / 2));
//            console.debug("QML FolderSizeItemListModel::refreshBegin fsListView.lastCenterIndex " + fsListView.lastCenterIndex);
    }

    function refreshCompletedSlot() {
        console.debug("folderPage refreshCompletedSlot");
//            window.state = "ready";

        // Reset ListView currentIndex.
        fsListView.currentIndex = -1;

        // Auto-sync after refresh. Only dirty items will be sync'd.
        // TODO Suppress in PieView.
        if (appInfo.getSettingBoolValue("sync.after.refresh", false)) {
            syncConnectedItemsSlot(true);
        }
    }

    function fetchDirSizeStartedSlot() {
        refreshButtonTimer.restart();
    }

    function fetchDirSizeFinishedSlot() {
        refreshButtonTimer.stop();
        refreshButton.rotation = 0;

        // Refresh itemList to show changes on ListView.
        fsModel.refreshItems();
    }

    function resetPopupToolPanelSlot() {
        popupToolPanel.srcFilePath = "";
        popupToolPanel.pastePath = "";
    }

    function refreshItem(remotePath) {
        cloudDrivePathDialog.refreshItem(remotePath);
    }

    Flipable {
        id: flipable1
        width: parent.width
        height: parent.height - currentPath.height
        x: 0
        anchors.top: currentPath.bottom

        property bool flipped: false

        front: fsListView
        back: pieChartView
        onStateChanged: {
            fsModel.refreshItems();
        }

        transform: Rotation {
            id: rotation
            origin.x: flipable1.width/2
            origin.y: flipable1.height/2
            axis.x: 0; axis.y: 1; axis.z: 0     // set axis.y to 1 to rotate around y-axis
            angle: 0    // the default angle
        }

        states: [
            State {
                name: "front"
                PropertyChanges { target: rotation; angle: 0 }
                when: !flipable1.flipped
            },
            State {
                name: "back"
                PropertyChanges { target: rotation; angle: 180 }
                when: flipable1.flipped
            }
        ]

        transitions: [
            Transition {
                to: "front"
                NumberAnimation { target: rotation; property: "angle"; duration: 500 }
                ScriptAction { script: toggleSortFlag() }
            },
            Transition {
                to: "back"
                ScriptAction { script: toggleSortFlag() }
                NumberAnimation { target: rotation; property: "angle"; duration: 500 }
            }
        ]
    }

    PieChart {
        id: pieChartView
        x: 0
        y: 0
        width: parent.width - 4
        height: parent.height
        anchors.horizontalCenter: parent.horizontalCenter
        model: fsModel
        visible: (folderPage.state == "chart")
        labelFont: "Sans Serif,16"

        onChartClicked: {
            console.debug("QML pieChartView.onChartClicked");
        }
        onSliceClicked: {
            console.debug("QML pieChartView.onSliceClicked " + text + ", index=" + index + ", isDir=" + isDir);
            if (isDir) {
                // TODO keep sortBySize in PieView.
                changeDirSlot(text, FolderSizeItemListModel.SortBySize);
            } else {
                flipSlot();
            }
        }
        onActiveFocusChanged: {
            console.debug("QML pieChartView.onActiveFocusChanged");
        }
        onSceneActivated: {
            console.debug("QML pieChartView.onSceneActivated");
        }
        onSwipe: {
            console.debug("QML pieChartView.onSwipe " + swipeAngle);
            flipSlot();
        }

        Component.onCompleted: {
            console.debug(Utility.nowText() + " folderPage pieChartView onCompleted");
            window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("PieChartView"), 0.1);
        }
    }

    ListView {
        id: fsListView
        x: 0
        y: 0
        width: parent.width
        height: parent.height - (nameFilterPanel.visible ? nameFilterPanel.height : 0)
        anchors.top: parent.top
        highlightRangeMode: ListView.NoHighlightRange
        highlightFollowsCurrentItem: true
        highlightMoveDuration: 1
        highlightMoveSpeed: 4000
        highlight: Rectangle {
            width: fsListView.width
            gradient: highlightGradient
        }
        clip: true
        focus: true
        cacheBuffer: height * 2
        pressDelay: 200
        model: fsModel
        delegate: listItemDelegate
        state: ""
        states: [
            State {
                name: "mark"
            }
        ]

        property int lastCenterIndex
        property string focusLocalPath

        function isAnyItemChecked() {
            for (var i=0; i<model.count; i++) {
                var checked = model.getProperty(i, FolderSizeItemListModel.IsCheckedRole);
                if (checked) {
//                    console.debug("fsListView isAnyItemChecked item"
//                                  + " absolutePath " + model.getProperty(i, FolderSizeItemListModel.AbsolutePathRole)
//                                  + " isChecked " + checked);
                    return true;
                }
            }
            return false;
        }

        function areAllItemChecked() {
            for (var i=0; i<model.count; i++) {
                var checked = model.getProperty(i, FolderSizeItemListModel.IsCheckedRole);
                if (!checked) {
//                    console.debug("fsListView areAllItemChecked item"
//                                  + " absolutePath " + model.getProperty(i, FolderSizeItemListModel.AbsolutePathRole)
//                                  + " isChecked " + checked);
                    return false;
                }
            }
            return (model.count > 0);
        }

        function markAll() {
            for (var i=0; i<model.count; i++) {
                // Enable all as FolderSizeItemListModel and CloudDriveModel can support sourcePath as files/folders.
//                if (!model.getProperty(i, FolderSizeItemListModel.IsDirRole)) {
                    model.setProperty(i, FolderSizeItemListModel.IsCheckedRole, true);
//                }
            }
        }

        function markAllFiles() {
            for (var i=0; i<model.count; i++) {
                if (!model.getProperty(i, FolderSizeItemListModel.IsDirRole)) {
                    model.setProperty(i, FolderSizeItemListModel.IsCheckedRole, true);
                }
            }
        }

        function markAllFolders() {
            for (var i=0; i<model.count; i++) {
                if (model.getProperty(i, FolderSizeItemListModel.IsDirRole)) {
                    model.setProperty(i, FolderSizeItemListModel.IsCheckedRole, true);
                }
            }
        }

        function unmarkAll() {
            for (var i=0; i<model.count; i++) {
                model.setProperty(i, FolderSizeItemListModel.IsCheckedRole, false);
            }
        }

        function cutMarkedItems() {
            for (var i=0; i<model.count; i++) {
                if (model.getProperty(i, FolderSizeItemListModel.IsCheckedRole)) {
                    console.debug(Utility.nowText() + "fsListView cutMarkedItems item"
                                  + " absolutePath " + model.getProperty(i, FolderSizeItemListModel.AbsolutePathRole)
                                  + " isChecked " + model.getProperty(i, FolderSizeItemListModel.IsCheckedRole));

                    clipboard.addItemWithSuppressCountChanged({ "action": "cut", "sourcePath": model.getProperty(i, FolderSizeItemListModel.AbsolutePathRole) });
                }

                // Reset isChecked.
//                console.debug(Utility.nowText() + "fsListView clear check item"
//                              + " absolutePath " + model.getProperty(i, FolderSizeItemListModel.AbsolutePathRole));
                model.setProperty(i, FolderSizeItemListModel.IsCheckedRole, false);
            }

            // Emit suppressed signal.
            clipboard.emitCountChanged();
        }

        function copyMarkedItems() {
            for (var i=0; i<model.count; i++) {
                if (model.getProperty(i, FolderSizeItemListModel.IsCheckedRole)) {
                    console.debug("fsListView copyMarkedItems item"
                                  + " absolutePath " + model.getProperty(i, FolderSizeItemListModel.AbsolutePathRole)
                                  + " isChecked " + model.getProperty(i, FolderSizeItemListModel.IsCheckedRole));

                    clipboard.addItemWithSuppressCountChanged({ "action": "copy", "sourcePath": model.getProperty(i, FolderSizeItemListModel.AbsolutePathRole) });
                }

                // Reset isChecked.
                model.setProperty(i, FolderSizeItemListModel.IsCheckedRole, false);
            }

            // Emit suppressed signal.
            clipboard.emitCountChanged();
        }

        function deleteMarkedItems() {
            for (var i=0; i<model.count; i++) {
                if (model.getProperty(i, FolderSizeItemListModel.IsCheckedRole)) {
                    console.debug("fsListView deleteMarkedItems item"
                                  + " absolutePath " + model.getProperty(i, FolderSizeItemListModel.AbsolutePathRole)
                                  + " isChecked " + model.getProperty(i, FolderSizeItemListModel.IsCheckedRole));

                    clipboard.addItem({ "action": "delete", "sourcePath": model.getProperty(i, FolderSizeItemListModel.AbsolutePathRole) });
                }

                // Reset isChecked.
                model.setProperty(i, FolderSizeItemListModel.IsCheckedRole, false);
            }

            // Open confirmation dialog.
            fileActionDialog.open();
        }

        function syncMarkedItems() {
            for (var i=0; i<model.count; i++) {
                if (model.getProperty(i, FolderSizeItemListModel.IsCheckedRole)) {
                    console.debug("fsListView deleteMarkedItems item"
                                  + " absolutePath " + model.getProperty(i, FolderSizeItemListModel.AbsolutePathRole)
                                  + " isChecked " + model.getProperty(i, FolderSizeItemListModel.IsCheckedRole));

                    clipboard.addItem({ "action": "sync", "sourcePath": model.getProperty(i, FolderSizeItemListModel.AbsolutePathRole) });
                }

                // Reset isChecked.
                model.setProperty(i, FolderSizeItemListModel.IsCheckedRole, false);
            }

            // Invoke syncClipboard.
            syncClipboardItems();
        }

        onMovementStarted: {
            if (currentItem) {
                // Hide highlight and popupTool.
                currentItem.pressed = false;
                currentIndex = -1;
                popupToolPanel.visible = false;
            }
        }

        onMovementEnded: {
            fsListView.lastCenterIndex = fsListView.indexAt(0, fsListView.contentY + (fsListView.height / 2));
        }

        onCountChanged: {
//            console.debug("fsListView onCountChanged " + count + " model.count " + model.count);
            if (count == model.count) {
                if (fsListView.focusLocalPath != "") {
                    // Focus on specified local path's index.
                    // The path is set by back button or listItem onClicked as "." to avoid focusing any item.
                    var focusIndex = fsModel.getIndexOnCurrentDir(fsListView.focusLocalPath);
                    if (focusIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
                        fsListView.positionViewAtIndex(focusIndex, ListView.Center);
                    }
                    fsListView.focusLocalPath = "";
                } else {
                    // Keep focus on last center index.
                    fsListView.positionViewAtIndex(lastCenterIndex, ListView.Center);
                }
            }
        }

        Component.onCompleted: {
            console.debug(Utility.nowText() + " folderPage fsListView onCompleted");
            window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("FolderListView"), 0.1);
        }

        QuickScrollPanel {
            id: quickScrollPanel
            listView: parent
            indicatorBarTitle: (modelIndex < 0) ? ""
                               : ( fsModel.sortFlag == FolderSizeItemListModel.SortByTime
                                  ? Qt.formatDateTime(fsModel.get(modelIndex).lastModified, "d MMM yyyy")
                                  : fsModel.get(modelIndex).name )
            inverted: !theme.inverted
            scrollBarWidth: 80
            indicatorBarHeight: 80
            scrollBarColor: "grey"
            indicatorWidth: 5
            autoHideInterval: appInfo.emptySetting+appInfo.getSettingValue("QuickScrollPanel.timer.interval", 2) * 1000
        }
    }

    Gradient {
        id: highlightGradient
        GradientStop {
            position: 0
            color: "#0080D0"
        }

        GradientStop {
            position: 1
            color: "#53A3E6"
        }
    }

    Component {
        id: listItemDelegate

        CloudListItem {
            id: listItem
            listViewState: (fsListView.state ? fsListView.state : "")
            runningIconSource: fsModel.getRunningOperationIconSource(runningOperation)
            syncIconVisible: cloudDriveModel.isConnected(absolutePath) || cloudDriveModel.isSyncing(absolutePath);
            syncIconSource: {
                if (cloudDriveModel.isSyncing(absolutePath)) {
                    return "cloud_wait.svg";
                } else if (cloudDriveModel.isDirty(absolutePath, lastModified)) {
                    return "cloud_dirty.svg";
                } else {
                    return "cloud.svg";
                }
            }

            onPressAndHold: {
                if (fsListView.state != "mark") {
                    fsListView.currentIndex = index;
                    popupToolPanel.selectedFilePath = absolutePath;
                    popupToolPanel.selectedFileName = name;
                    popupToolPanel.selectedFileIndex = index;
                    popupToolPanel.isDir = isDir;
                    popupToolPanel.pastePath = (isDir) ? absolutePath : currentPath.text;
                    var panelX = x + mouseX - fsListView.contentX;
                    var panelY = y + mouseY - fsListView.contentY + flipable1.y;
                    popupToolPanel.open(panelX, panelY);
                }
            }

            onClicked: {
                if (fsListView.state == "mark") {
                    fsModel.setProperty(index, FolderSizeItemListModel.IsCheckedRole, !isChecked);
                } else {
                    if (isDir) {
                        fsListView.focusLocalPath = "."; // Put dummy path to avoid focus on lastCenterIndex.
                        changeDirSlot(name);
                    } else {
                        // If file is running, disable preview.
                        if (isRunning) return;

                        // Implement internal viewers for image(JPG,PNG), text with addon(cloud drive, print)
                        var viewableImageFileTypes = ["JPG", "PNG", "SVG"];
                        var viewableTextFileTypes = ["TXT", "HTML", "LOG", "CSV", "CONF", "INI"];
                        if (viewableImageFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                            // TODO Populate ImageViewModel with mediaUrl = image://local/...
                            pageStack.push(Qt.resolvedUrl("ImageViewPage.qml"),
                                           { model: fsModel, selectedFilePath: absolutePath });
                        } else if (viewableTextFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                            pageStack.push(Qt.resolvedUrl("TextViewPage.qml"),
                                           { filePath: absolutePath, fileName: name });
                        } else {
                            Qt.openUrlExternally(fsModel.getUrl(absolutePath));
                        }
                    }
                }
            }
        }
    }

    TitlePanel {
        id: currentPath
        text: fsModel.currentDir
    }

    NameFilterPanel {
        id: nameFilterPanel
        requestAsType: true
        anchors.bottom: parent.bottom

        onPrevious: {
            lastFindIndex = fsModel.findIndexByNameFilter(nameFilter, --lastFindIndex, true);
            console.debug("folderPage nameFilterPanel onPrevious nameFilter " + nameFilter + " lastFindIndex " + lastFindIndex);
            if (lastFindIndex > -1) {
                fsListView.positionViewAtIndex(lastFindIndex, ListView.Beginning);
                fsListView.currentIndex = lastFindIndex;
            }
        }

        onNext: {
            lastFindIndex = fsModel.findIndexByNameFilter(nameFilter, ++lastFindIndex, false);
            console.debug("folderPage nameFilterPanel onNext nameFilter " + nameFilter + " lastFindIndex " + lastFindIndex);
            if (lastFindIndex > -1) {
                fsListView.positionViewAtIndex(lastFindIndex, ListView.Beginning);
                fsListView.currentIndex = lastFindIndex;
            }
        }

        onOpened: {
            // Turn highlight on.
//            fsListView.highlightFollowsCurrentItem = true;
            lastFindIndex = fsListView.currentIndex;
        }

        onClosed: {
            // Turn highlight off.
//            fsListView.highlightFollowsCurrentItem = false;
            fsListView.currentIndex = -1;
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

        function isButtonVisibleCallback(buttonName) {
            if (buttonName === "sync") {
                return !fsModel.isRoot(selectedFilePath) && cloudDriveModel.canSync(selectedFilePath);
            } else if (buttonName === "upload") {
                return !fsModel.isRoot(selectedFilePath) && cloudDriveModel.canSync(selectedFilePath) && !cloudDriveModel.isParentConnected(selectedFilePath);
            } else if (buttonName === "download") {
                return !fsModel.isRoot(selectedFilePath) && cloudDriveModel.canSync(selectedFilePath) && !cloudDriveModel.isParentConnected(selectedFilePath);
            } else if (buttonName === "disconnect") {
                return cloudDriveModel.isConnected(selectedFilePath);
            } else if (buttonName === "unsync") {
                return cloudDriveModel.isConnected(selectedFilePath);
            } else if (buttonName === "cloudSettings") {
                return cloudDriveModel.isConnected(selectedFilePath);
            } else if (buttonName === "cloudScheduler") {
                return isDir && cloudDriveModel.isConnected(selectedFilePath);
            } else if (buttonName === "paste") {
                return (clipboard.count > 0 && isDir);
            } else if (buttonName == "mail") {
                return cloudDriveModel.isConnected(selectedFilePath);
            } else if (buttonName == "sms") {
                return cloudDriveModel.isConnected(selectedFilePath);
            } else if (buttonName == "bluetooth") {
                return fsModel.isFile(selectedFilePath);
            } else if (buttonName == "editFile") {
                return fsModel.isFile(selectedFilePath);
            } else if (buttonName == "print") {
                return fsModel.isFile(selectedFilePath);
            }

            return true;
        }

        onOpened: {
//            console.debug("popupToolRing onOpened");
        }

        onClosed: {
//            console.debug("popupToolRing onClosed");
            // Workaround to hide highlight.
            fsListView.currentIndex = -1;
        }

        onCutClicked: {
            clipboard.addItem({ "action": "cut", "sourcePath": sourcePath });
        }

        onCopyClicked: {
            clipboard.addItem({ "action": "copy", "sourcePath": sourcePath });
        }

        onPasteClicked: {
            fileActionDialog.targetPath = targetPath;
            fileActionDialog.targetPathName = selectedFileName;
            fileActionDialog.open();
        }

        onDeleteFile: {
            // Delete always clear clipboard.
            clipboard.clear();
            clipboard.addItem({ "action": "delete", "sourcePath": sourcePath });
            fileActionDialog.open();
        }

        onPrintFile: {
            gcpClient.printFileSlot(srcFilePath, srcItemIndex);
        }

        onSyncFile: {
            syncFileSlot(srcFilePath, srcItemIndex);
        }

        onUnsyncFile: {
            unsyncFileSlot(srcFilePath, srcItemIndex);
        }

        onDisconnectFile: {
            disconnectFileSlot(srcFilePath, srcItemIndex);
        }

        onBrowseRemoteFile: {
            browseRemoteFileSlot(srcFilePath, srcItemIndex);
        }

        onScheduleSyncFile: {
            scheduleSyncFileSlot(srcFilePath, srcItemIndex)
        }

        onShowTools: {
            fsListView.currentIndex = srcItemIndex;
            toolMenu.selectedIndex = srcItemIndex;
            toolMenu.selectedFilePath = srcFilePath;
            toolMenu.open();
        }

        onNewFolder: {
            newFolderDialog.open();
        }

        onRenameFile: {
            renameDialog.sourcePath = srcFilePath;
            renameDialog.open();
        }

        onMarkClicked: {
            fsListView.state = "mark";
            fsModel.setProperty(srcItemIndex, FolderSizeItemListModel.IsCheckedRole, true);
        }

        onUploadFile: {
            uploadFileSlot(srcFilePath, srcItemIndex);
        }

        onDownloadFile: {
            downloadFileSlot(srcFilePath, srcItemIndex);
        }

        onMailFile: {
            mailFileSlot(srcFilePath, srcItemIndex);
        }

        onSmsFile: {
            smsFileSlot(srcFilePath, srcItemIndex);
        }

        onBluetoothFile: {
            bluetoothFileSlot(srcFilePath, srcItemIndex);
        }

        onEditFile: {
            pageStack.push(Qt.resolvedUrl("TextViewPage.qml"),
                           { filePath: srcFilePath, fileName: fsModel.getFileName(srcFilePath) });
        }

        onShowInfo: {
            filePropertiesDialog.show(srcItemIndex);
        }
    }

    FileActionDialog {
        id: fileActionDialog

        onConfirm: {
            // It always replace existing names.
            fsModel.suspendNextJob();
            cloudDriveModel.suspendNextJob();

            // TODO Copy/Move/Delete all files from clipboard.
            // Action is {copy, cut, delete}
            var res = true;
            for (var i=0; i<clipboard.count; i++) {
                var action = clipboard.get(i).action;
                var sourcePath = clipboard.get(i).sourcePath;
                var sourcePathName = (isOverwrite && i == 0) ? newFileName : (clipboard.get(i).sourcePathName ? clipboard.get(i).sourcePathName : fsModel.getFileName(sourcePath));
                var actualTargetPath = fsModel.getAbsolutePath(targetPath, sourcePathName);

                // console.debug("folderPage fileActionDialog onConfirm clipboard action " + action + " sourcePath " + sourcePath);
                if (["copy","cut"].indexOf(action) > -1 && clipboard.get(i).type) {
//                    actualTargetPath = fsModel.getAbsolutePath(targetPath, clipboard.get(i).sourcePathName);
                    console.debug("folderPage fileActionDialog onConfirm Download type " + clipboard.get(i).type + " uid " + clipboard.get(i).uid + " sourcePath " + sourcePath + " actualTargetPath " + actualTargetPath);
                    cloudDriveModel.metadata(cloudDriveModel.getClientType(clipboard.get(i).type), clipboard.get(i).uid, actualTargetPath, sourcePath, -1, false, true); // Force get.
                } else if (action == "copy" && !clipboard.get(i).type) {
                    res = res && fsModel.copy(sourcePath, actualTargetPath);
                } else if (action == "cut" && !clipboard.get(i).type) {
                    res = res && fsModel.move(sourcePath, actualTargetPath);
                } else if (action == "delete" && !clipboard.get(i).type) {
                    res = res && fsModel.deleteFile(sourcePath);
                } else {
                    console.debug("folderPage fileActionDialog onConfirm invalid action " + action);
                }

                if (!res) {
                    break;
                }
            }

            if (res) {
                // Open progress dialogs if it's related.
                fsModel.openCopyProgressDialog(titleText);
                fsModel.openDeleteProgressDialog();

                // Reset both source and target.
                targetPath = "";
                resetPopupToolPanelSlot();

                // Clear clipboard as they should have been processed.
                clipboard.clear();
            } else {
                // Reset target only.
                targetPath = "";
                popupToolPanel.pastePath = "";

                // Cancel queued jobs.
                fsModel.cancelQueuedJobs();
                fsModel.resumeNextJob();
            }
        }

        onClosed: {
            // Always clear clipboard's delete actions.
            clipboard.clearDeleteActions();

            // Resume queued jobs.
            fsModel.resumeNextJob();
            cloudDriveModel.resumeNextJob();
        }
    }

    NewFolderDialog {
        id: newFolderDialog

        onConfirm: {
            if (nameField.text !== "") {
                var res = false;
                if (isNewFolder) {
                    res = fsModel.createDir(nameField.text);
                } else {
                    res = fsModel.createEmptyFile(nameField.text);
                }

                refreshSlot();
            }
        }
    }

    RenameDialog {
        id: renameDialog
        sourcePathName: fsModel.getFileName(sourcePath)

        onConfirm: {
            if (nameField.text != "" && nameField.text != sourcePathName) {
                var res = fsModel.renameFile(sourcePathName, nameField.text);
                refreshSlot();
            }
        }
    }

    CloudDriveUsersDialog {
        id: uidDialog

        function proceedPendingOperation() {
            // TODO
        }

        function proceedOperation(type, uid, localPath, remotePath, modelIndex, suppressBrowsing) {
            switch (operation) {
            case CloudDriveModel.Metadata:
                // TODO Check connection for specified type, uid, localPath.
                if (cloudDriveModel.isConnected(type, uid, localPath) || suppressBrowsing) {
                    // If already connected, sync it right away.
                    cloudDriveModel.metadata(type, uid, localPath, remotePath, modelIndex);
                } else {
                    // If it's not connected, show cloudDrivePathDialog.
                    cloudDrivePathDialog.open();
                    cloudDrivePathDialog.remotePath = remotePath;
                    cloudDrivePathDialog.remoteParentPath = "";
                    cloudDrivePathDialog.refresh();
                }
                break;
            case CloudDriveModel.FilePut:
                // Show cloudDrivePathDialog.
                cloudDrivePathDialog.open();
                cloudDrivePathDialog.remotePath = ""; // Undefine remote path for local path.
                cloudDrivePathDialog.remoteParentPath = "";
                cloudDrivePathDialog.refresh();
                break;
            case CloudDriveModel.FileGet:
                // Show cloudDrivePathDialog.
                cloudDrivePathDialog.open();
                cloudDrivePathDialog.remotePath = ""; // Undefine remote path for local path.
                cloudDrivePathDialog.remoteParentPath = "";
                cloudDrivePathDialog.refresh();
                break;
            case CloudDriveModel.ShareFile:
                // TODO Find way to refresh it before shareReplyFinished.
                recipientSelectionDialog.refresh();
                cloudDriveModel.shareFileCaller = uidDialog.caller;
                cloudDriveModel.shareFile(type, uid, localPath, remotePath);
                break;
            case CloudDriveModel.DeleteFile:
                cloudDriveModel.deleteFile(type, uid, localPath, remotePath);
                break;
            case CloudDriveModel.Disconnect:
                cloudDriveModel.disconnect(type, uid, localPath);
                break;
            case CloudDriveModel.Browse:
                // Show cloudDrivePathDialog.
                cloudDrivePathDialog.open();
                cloudDrivePathDialog.remotePath = remotePath;
                cloudDrivePathDialog.remoteParentPath = "";
                cloudDrivePathDialog.refresh();
                break;
            case CloudDriveModel.ScheduleSync:
                cloudDriveSchedulerDialog.localPathCronExp = cloudDriveModel.getItemCronExp(type, uid, localPath);
                cloudDriveSchedulerDialog.open();
                break;
            }
        }

        onOpening: {
            if (uidDialog.model) {
                uidDialog.model.destroy();
            }
            uidDialog.model = cloudDriveModel.getUidListModel(localPath);
        }

        onRejected: {
            console.debug("uidDialog onRejected");

            // Clear clipboard.
            clipboard.clear();
        }

        onAccepted: {
            // TODO Proceed for GoogleDrive

            // Proceed for Dropbox
            if (uidDialog.localPath != "") {
                // sync localPath.
                var remoteFilePath = cloudDriveModel.getItemRemotePath(uidDialog.localPath, uidDialog.selectedCloudType, uidDialog.selectedUid);
                console.debug("uidDialog onAccepted uidDialog.selectedIndex " + uidDialog.selectedIndex + " type " + uidDialog.selectedCloudType + " uid " + uidDialog.selectedUid + " localPath " + uidDialog.localPath + " remoteFilePath " + remoteFilePath);
//                if (remoteFilePath == "") {
//                    remoteFilePath = cloudDriveModel.getDefaultRemoteFilePath(uidDialog.localPath);
//                    console.debug("uidDialog onAccepted adjusted remoteFilePath " + remoteFilePath);
//                }
                proceedOperation(uidDialog.selectedCloudType, uidDialog.selectedUid, uidDialog.localPath, remoteFilePath, selectedModelIndex);
            } else if (clipboard.count > 0){
                // Sync from clipboard.

                // Check if all selected items are connected to cloud.
                var connectedItemCount = 0;
                var totalConnectedItemCount = 0;
                for (var i=0; i<clipboard.count; i++) {
                    if (clipboard.get(i).action == "sync") {
                        console.debug("uidDialog onAccepted clipboard item i " + i + " " + clipboard.get(i).action + " sourcePath " + clipboard.get(i).sourcePath);
                        var sourcePath = clipboard.get(i).sourcePath;
                        var remotePath = cloudDriveModel.getItemRemotePath(sourcePath, uidDialog.selectedCloudType, uidDialog.selectedUid);
                        if (remotePath != "") {
                            connectedItemCount++;
                        }
                        totalConnectedItemCount++;
                    }
                }

                if (connectedItemCount < totalConnectedItemCount) {
                    if (connectedItemCount > 0) {
                        // Select any connected items.
                        showMessageDialogSlot(qsTr("Sync marked items"), qsTr("Please select only items connected to target cloud storage or not connected at all."));
                        // Clear clipboard.
                        clipboard.clear();
                        return;
                    } else {
                        // Select only items which are not connected.
                        cloudDrivePathDialog.open();
                        cloudDrivePathDialog.remotePath = "";
                        cloudDrivePathDialog.remoteParentPath = "";
                        cloudDrivePathDialog.refresh();
                        return;
                    }
                }

                // Select only connected items.
                // Suspend for queuing.
                cloudDriveModel.suspendNextJob();

                for (var i=0; i<clipboard.count; i++) {
                    if (clipboard.get(i).action == "sync") {
                        console.debug("uidDialog onAccepted clipboard item i " + i + " " + clipboard.get(i).action + " sourcePath " + clipboard.get(i).sourcePath);

                        var sourcePath = clipboard.get(i).sourcePath;
                        var remotePath = cloudDriveModel.getItemRemotePath(sourcePath, uidDialog.selectedCloudType, uidDialog.selectedUid);
                        if (remotePath != "") {
                            // Proceed sync only connected items which already has remotePath
                            proceedOperation(uidDialog.selectedCloudType, uidDialog.selectedUid, sourcePath, remotePath, -1, true);
                        } else {
                            console.debug("uidDialog onAccepted skipped clipboard item i " + i + " " + clipboard.get(i).action + " sourcePath " + clipboard.get(i).sourcePath + " remotePath " + remotePath);
                        }
                    }
                }

                // Clear clipboard.
                clipboard.clear();

                // Resume proceed queued jobs.
                cloudDriveModel.resumeNextJob();
            }
        }
    }

    CloudDrivePathDialog {
        id: cloudDrivePathDialog
        caller: uidDialog.caller
        operation: uidDialog.operation
        localPath: uidDialog.localPath
        selectedCloudType: uidDialog.selectedCloudType
        selectedUid: uidDialog.selectedUid
        selectedModelIndex: uidDialog.selectedModelIndex

        function proceedOperation(type, uid, localPath, remotePath, remotePathName, isDir, remoteParentPath, modelIndex) {
            console.debug("cloudDrivePathDialog proceedOperation operation " + operation + " type " + type + " uid " + uid + " localPath " + localPath + " remotePath " + remotePath + " isDir " + isDir + " remoteParentPath " + remoteParentPath + " modelIndex " + modelIndex);

            switch (operation) {
            case CloudDriveModel.FilePut:
                console.debug("cloudDrivePathDialog proceedOperation FilePut localPath " + localPath + " to remotePath " + remotePath + " isDir " + isDir
                              + " remoteParentPath " + remoteParentPath
                              + " fsModel.getDirPath(localPath) " + fsModel.getDirPath(localPath)
                              + " cloudDriveModel.getRemoteName(type, remotePath) " + cloudDriveModel.getRemoteName(type, remotePath));

                // Upload selected local path into remote parent path.
                if (remoteParentPath != "") {
                    if (fsModel.isDir(localPath)) {
                        cloudDriveModel.suspendNextJob();
                        cloudDriveModel.syncFromLocal(type, uid, localPath, remoteParentPath, modelIndex, true);
                        cloudDriveModel.resumeNextJob();
                    } else {
                        cloudDriveModel.filePut(type, uid, localPath, remoteParentPath, fsModel.getFileName(localPath), modelIndex);
                    }
                } else {
                    console.debug("cloudDrivePathDialog proceedOperation FilePut ignored remoteParentPath " + remoteParentPath + " is empty.");
                }

                break;
            case CloudDriveModel.FileGet:
                console.debug("cloudDrivePathDialog proceedOperation FileGet localPath " + localPath + " from remotePath " + remotePath + " remotePathName " + remotePathName + " isDir " + isDir
                              + " remoteParentPath " + remoteParentPath
                              + " fsModel.getDirPath(localPath) " + fsModel.getDirPath(localPath)
                              + " cloudDriveModel.getRemoteName(type, remotePath) " + cloudDriveModel.getRemoteName(type, remotePath));

                // Download selected remote folder/file to parent folder of selected local path.
                if (remotePath != "") {
                    var targetLocalPath = fsModel.getAbsolutePath(fsModel.getDirPath(localPath), remotePathName);
                    console.debug("cloudDrivePathDialog proceedOperation FileGet targetLocalPath " + targetLocalPath);
                    cloudDriveModel.metadata(type, uid, targetLocalPath, remotePath, modelIndex, false, true); // Force get.
                } else {
                    console.debug("cloudDrivePathDialog proceedOperation FileGet ignored remotePath " + remotePath + " remotePathName " + remotePathName + " is empty.");
                }

                break;
            case CloudDriveModel.Browse:
                // Only folder can change connection.
                if (isDir != fsModel.isDir(localPath)) {
                    console.debug("cloudDrivePathDialog proceedOperation Browse localPath isDir " + fsModel.isDir(localPath) + " and remotePath isDir " + isDir + " must be the same object type.");
                    return;
                }

                if (fsModel.isDir(localPath) && remotePath != "") {
                    var originalRemotePath = cloudDriveModel.getItemRemotePath(localPath, type, uid);
                    if (remotePath != originalRemotePath) {
                        // TODO Check if cloud client's remotePath is absolute path.
                        if (cloudDriveModel.isRemoteAbsolutePath(type)) {
                            console.debug("cloudDrivePathDialog proceedOperation Browse move from " + originalRemotePath + " to " + remotePath);
                            // Set children hash to empty to hint syncFromLocal to put remained files with empty hash.
                            cloudDriveModel.updateItemWithChildren(type, uid, localPath, originalRemotePath, localPath, remotePath, "");
                            cloudDriveModel.metadata(type, uid, localPath, remotePath, selectedModelIndex);
                        } else {
                            console.debug("cloudDrivePathDialog proceedOperation Browse move from " + originalRemotePath + " to " + remotePath);
                            // Remove original cloudDriveItem of localPath. Then start sync with new remotePath.
                            cloudDriveModel.removeItemWithChildren(type, uid, localPath);
                            cloudDriveModel.metadata(type, uid, localPath, remotePath, selectedModelIndex);
                        }
                    }
                } else if (!fsModel.isDir(localPath)) {
                    console.debug("cloudDrivePathDialog proceedOperation Browse ignored move localPath " + localPath + " is a file.");
                } else {
                    console.debug("cloudDrivePathDialog proceedOperation Browse ignored move from " + originalRemotePath + " to " + remotePath + " remotePath is empty.");
                }

                break;
            case CloudDriveModel.Metadata:
                if (localPath != "" && fsModel.isDir(localPath) && remotePath != "") {
                    console.debug("cloudDrivePathDialog proceedOperation Metadata sync from " + localPath + " to " + remotePath);
                    cloudDriveModel.metadata(type, uid, localPath, remotePath, selectedModelIndex);
                } else {                  
                    // Check if cloud client's remotePath is absolute path.
                    if (cloudDriveModel.isRemoteAbsolutePath(type)) {
                        // If localPath is file or remotePath is not specified.
                        // Use remoteParentPath + "/" + folderName.
                        remotePath = (remoteParentPath == "/" ? "" : remoteParentPath) + "/" + fsModel.getFileName(localPath);
                        console.debug("cloudDrivePathDialog proceedOperation Metadata sync from " + localPath + " to " + remotePath);
                        cloudDriveModel.metadata(type, uid, localPath, remotePath, selectedModelIndex);
                    } else {
                        // Find remote path by name.
                        var newRemoteFolderName = fsModel.getFileName(localPath);
                        for (var i=0; i<contentModel.count; i++) {
                            // TODO Find exactly match with name and isDir.
                            if (contentModel.get(i).name == newRemoteFolderName) {
                                remotePath = contentModel.get(i).absolutePath;
                                console.debug("cloudDrivePathDialog proceedOperation Metadata found remotePath " + remotePath + " for " + newRemoteFolderName);
                                break;
                            }
                        }
                        // Create remote folder and get its remote path (id).
                        if (remotePath == "") {
                            if (fsModel.isDir(localPath)) {
                                remotePath = cloudDriveModel.createFolder_Block(type, uid, remoteParentPath, newRemoteFolderName);
                                console.debug("cloudDrivePathDialog proceedOperation Metadata sync from " + localPath + " to " + remotePath);
                                cloudDriveModel.metadata(type, uid, localPath, remotePath, selectedModelIndex);
                            } else {
                                // Proceed put file as selected file name doesn't exist on remote parent path.
                                console.debug("cloudDrivePathDialog proceedOperation Metadata filePut from " + localPath + " to " + remoteParentPath);
                                cloudDriveModel.filePut(type, uid, localPath, remoteParentPath, newRemoteFolderName, selectedModelIndex);
                            }
                        } else {
                            // Start sync with remotePath.
                            console.debug("cloudDrivePathDialog proceedOperation Metadata sync from " + localPath + " to " + remotePath);
                            cloudDriveModel.metadata(type, uid, localPath, remotePath, selectedModelIndex);
                        }
                    }
                }
                break;
            }
        }

        onConfirm: {
            if (selectedIsValid) { // OK button is clicked.
                console.debug("cloudDrivePathDialog onConfirm "
                              + " selectedCloudType " + selectedCloudType + " selectedUid " + selectedUid
                              + " localPath " + localPath
                              + " selectedRemotePath " + selectedRemotePath
                              + " selectedRemotePathName " + selectedRemotePathName
                              + " selectedModelIndex " + selectedModelIndex
                              + " remoteParentPath " + remoteParentPath);

                // TODO Close dialog before resumeJob.
                close();
                cloudDriveModel.suspendNextJob();

                if (localPath != "") {
                    // Select only 1 item.
                    proceedOperation(selectedCloudType, selectedUid, localPath, selectedRemotePath, selectedRemotePathName, selectedIsDir, remoteParentPath, selectedModelIndex);
                } else {
                    // Sync masked items.
                    for (var i=0; i<clipboard.count; i++) {
                        if (clipboard.get(i).action == "sync") {
                            console.debug("cloudDrivePathDialog onConfirm clipboard item i " + i + " " + clipboard.get(i).action + " sourcePath " + clipboard.get(i).sourcePath);
                            var sourcePath = clipboard.get(i).sourcePath;
                            // proceedOperation() needs only localPath and remoteParentPath.
                            proceedOperation(selectedCloudType, selectedUid, sourcePath, selectedRemotePath, selectedRemotePathName, selectedIsDir, remoteParentPath, selectedModelIndex);
                        }
                    }
                }

                cloudDriveModel.resumeNextJob();
            }
        }

        onCreateRemoteFolder: {
            // Create remote folder.
            cloudDriveModel.createFolder(selectedCloudType, selectedUid, "", remoteParentPath, newRemoteFolderName);
        }

        onRefreshRequested: {
            console.debug("cloudDrivePathDialog onRefreshRequested " + selectedCloudType + " " + remotePath + " " + remoteParentPath);

            // Browse remote parent path.
            remoteParentPath = (remoteParentPath == "") ? cloudDriveModel.getParentRemotePath(selectedCloudType, remotePath) : remoteParentPath;
            cloudDriveModel.browse(selectedCloudType, selectedUid, remoteParentPath);
        }

        onDeleteRemotePath: {
            // Delete remote file/folder.
            cloudDriveModel.deleteFile(selectedCloudType, selectedUid, "", remotePath);
        }
    }

    CloudDriveSchedulerDialog {
        id: cloudDriveSchedulerDialog
        titleText: appInfo.emptyStr+qsTr("Schedule sync %1").arg(fsModel.getFileName(localPath))
        caller: uidDialog.caller
        operation: uidDialog.operation
        localPath: uidDialog.localPath
        selectedCloudType: uidDialog.selectedCloudType
        selectedUid: uidDialog.selectedUid
        selectedModelIndex: uidDialog.selectedModelIndex

        onSelectedCronExp: {
            console.debug("folderPage cloudDriveSchedulerDialog onSelectedCronExp " + cronExp);
            cloudDriveModel.updateItemCronExp(selectedCloudType, selectedUid, localPath, cronExp);
        }
    }

    FilePropertiesDialog {
        id: filePropertiesDialog

        function show(index) {
            selectedIndex = index;
            selectedItem = fsModel.get(selectedIndex);
            populateCloudItemModel();
            open();
        }

        function populateCloudItemModel() {
            cloudItemModel.clear();
            if (selectedItem) {
                var cloudItems = Utility.createJsonObj(cloudDriveModel.getItemListJson(selectedItem.absolutePath));
                for (var i = 0; i < cloudItems.length; i++) {
                    var cloudItem = cloudItems[i];
                    var uidJson = Utility.createJsonObj(cloudDriveModel.getStoredUid(cloudItem.type, cloudItem.uid));
                    var modelItem = { type: cloudItem.type, uid: cloudItem.uid, email: uidJson.email, absolutePath: (cloudItem.remote_path ? cloudItem.remote_path : qsTr("Not available")) };
                    cloudItemModel.append(modelItem);
                }
            }
        }

        onSyncAll: {
            if (cloudDriveModel.canSync(selectedItem.absolutePath)) {
                cloudDriveModel.syncItem(selectedItem.absolutePath);
            }
            close();
        }
        onSyncAdd: {
            syncFileSlot(selectedItem.absolutePath, selectedIndex);
            close();
        }
        onDisconnect: {
            cloudDriveModel.disconnect(type, uid, selectedItem.absolutePath);
            close();
        }
    }
}
