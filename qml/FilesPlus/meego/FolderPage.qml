import QtQuick 1.1
import com.nokia.meego 1.0
import Charts 1.0
import FolderSizeItemListModel 1.0
import GCPClient 1.0
import CloudDriveModel 1.0
import BluetoothClient 1.0
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

        ToolIcon {
            id: backButton
            iconId: "toolbar-back"
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

        ToolIcon {
            id: refreshButton
            iconId: "toolbar-refresh"
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

            MouseArea {
                anchors.fill: parent
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
        }

        ToolIcon {
            id: flipButton
            iconSource: (folderPage.state != "list") ? (theme.inverted ? "list.svg" : "list_inverted.svg") : (theme.inverted ? "chart.svg" : "chart_inverted.svg")
            visible: (fsListView.state == "")

            Component.onCompleted: {
                flipButton.clicked.connect(flipSlot);
            }
        }

        ToolIcon {
            id: cloudButton
            iconSource: (theme.inverted ? "cloud.svg" : "cloud_inverted.svg")
            visible: (fsListView.state == "")
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

        onPaste: {
            fileActionDialog.targetPath = fsModel.currentDir;
            fileActionDialog.targetPathName = fsModel.currentDir;
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
        }
        onCutMarkedItems: {
            fsListView.cutMarkedItems();
            fsListView.state = "";
        }
        onDeleteMarkedItems: {
            fsListView.deleteMarkedItems();
            fsListView.state = "";
        }
        onSyncMarkedItems: {
            fsListView.syncMarkedItems();
            fsListView.state = "";
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
            window.showMessageDialogSlot(qsTr("Sync with Cloud"),
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
                // TODO configurable suppress confirmation.
                btPowerOnDialog.open();
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
//            console.debug("folderPage updateItemSlot " + pathList[i] + " " + modelIndex + " " + jobJson.is_running);
            if (modelIndex == FolderSizeItemListModel.IndexOnCurrentDirButNotFound) {
                fsModel.clearIndexOnCurrentDir();
                modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
            }
            if (modelIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
                fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, jobJson.is_running);
                fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningOperationRole, FolderSizeItemListModel.SyncOperation);
            }
        }

        // Show indicator on newLocalPath up to root.
        if (jobJson.new_local_file_path != "") {
            pathList = fsModel.getPathToRoot(jobJson.new_local_file_path);
            for(i=0; i<pathList.length; i++) {
                modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
//                console.debug("folderPage updateItemSlot " + pathList[i] + " " + modelIndex + " " + jobJson.is_running);
                if (modelIndex == FolderSizeItemListModel.IndexOnCurrentDirButNotFound) {
                    fsModel.clearIndexOnCurrentDir();
                    modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
                }
                if (modelIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
                    fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, jobJson.is_running);
                    fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningOperationRole, FolderSizeItemListModel.SyncOperation);
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
        highlightFollowsCurrentItem: false
        highlightMoveDuration: 1
        highlightMoveSpeed: 4000
        highlight: Rectangle {
            width: fsListView.width
            gradient: highlightGradient
        }
        clip: true
        focus: true
        pressDelay: 100
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
//                console.debug("fsListView onMovementStarted currentItem.pressed " + currentItem.pressed);
                currentItem.pressed = false;
//                console.debug("fsListView onMovementStarted currentItem.pressed " + currentItem.pressed);
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
                            fsModel.nameFilters = ["*.jpg", "*.png", "*.svg"];
                            fsModel.refreshDir("folderPage listItem onClicked", false);
                            pageStack.push(Qt.resolvedUrl("ImageViewPage.qml"), {
                                               fileName: name,
                                               model: fsModel
                                           });
                        } else if (viewableTextFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                            pageStack.push(Qt.resolvedUrl("TextViewPage.qml"),
                                           { filePath: absolutePath, fileName: fsModel.getFileName(absolutePath) });
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
            fsListView.highlightFollowsCurrentItem = true;
            lastFindIndex = fsListView.currentIndex;
        }

        onClosed: {
            // Turn highlight off.
            fsListView.highlightFollowsCurrentItem = false;
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
                return (clipboard.count > 0);
            } else if (buttonName == "mail") {
                return cloudDriveModel.isConnected(selectedFilePath);
            } else if (buttonName == "sms") {
                return cloudDriveModel.isConnected(selectedFilePath);
            } else if (buttonName == "bluetooth") {
                return fsModel.isFile(selectedFilePath);
            } else if (buttonName == "editFile") {
                return fsModel.isFile(selectedFilePath);
            }

            return true;
        }

        onOpened: {
//            console.debug("popupToolRing onOpened");
            fsListView.highlightFollowsCurrentItem = true;
        }

        onClosed: {
//            console.debug("popupToolRing onClosed");
            // Workaround to hide highlight.
            fsListView.currentIndex = -1;
            fsListView.highlightFollowsCurrentItem = false;
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
    }

    ConfirmDialog {
        id: fileActionDialog

        property string sourcePath
        property string sourcePathName
        property string targetPath
        property string targetPathName
        property bool isOverwrite: false

        titleText: appInfo.emptyStr+fileActionDialog.getTitleText()
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
            // Exmaple of clipboard entry { "action": "cut", "sourcePath": sourcePath }
            // Exmaple of clipboard entry { "action": "cut", "type": "Dropbox", "uid": "asdfdg", "sourcePath": sourcePath, "sourcePathName": sourcePathName }
            var text = "";
            if (clipboard.count == 1) {
                text = getActionName(clipboard.get(0).action, clipboard.get(0).type, clipboard.get(0).uid)
                        + " " + (clipboard.get(0).sourcePathName ? clipboard.get(0).sourcePathName : clipboard.get(0).sourcePath)
                        + ((clipboard.get(0).action == "delete")?"":("\n" + qsTr("to") + " " + targetPathName))
                        + " ?";
            } else {
                var cutCount = 0;
                var copyCount = 0;
                var deleteCount = 0;
                var downloadCount = 0;
                for (var i=0; i<clipboard.count; i++) {
//                    console.debug("fileActionDialog getText clipboard i " + i + " action " + clipboard.get(i).action + " sourcePath " + clipboard.get(i).sourcePath);
                    if (["copy","cut"].indexOf(clipboard.get(i).action) > -1 && clipboard.get(i).type) {
                        downloadCount++;
                    } else if (clipboard.get(i).action == "copy") {
                        copyCount++;
                    } else if (clipboard.get(i).action == "cut") {
                        cutCount++;
                    } else if (clipboard.get(i).action == "delete") {
                        deleteCount++;
                    }
                }

                if (downloadCount>0) text = text + (qsTr("Download %n item(s)\n", "", downloadCount));
                if (deleteCount>0) text = text + (qsTr("Delete %n item(s)\n", "", deleteCount));
                if (copyCount>0) text = text + (qsTr("Copy %n item(s)\n", "", copyCount));
                if (cutCount>0) text = text + (qsTr("Move %n item(s)\n", "", cutCount));
                if (copyCount>0 || cutCount>0 || downloadCount>0) text = text + qsTr("to") + " " + targetPath;
                text = text + " ?";
            }

            return text;
        }

        function getActionName(actionText, type, uid) {
            if (type) {
                if (["copy","cut"].indexOf(actionText) > -1) return qsTr("Download");
                else return qsTr("Invalid action");
            } else {
                if (actionText == "copy") return qsTr("Copy");
                else if (actionText == "cut") return qsTr("Move");
                else if (actionText == "delete") return qsTr("Delete");
                else return qsTr("Invalid action");
            }
        }

        function canCopy() {
            return fsModel.canCopy(fsModel.getAbsolutePath(targetPath, fileActionDialog.sourcePathName), targetPath);
        }

        function getNewName() {
            return fsModel.getNewFileName(fileActionDialog.sourcePathName, fileActionDialog.targetPath);
        }

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
                var sourcePathName = (isOverwrite && i == 0) ? fileName.text : (clipboard.get(i).sourcePathName ? clipboard.get(i).sourcePathName : fsModel.getFileName(sourcePath));
                var actualTargetPath = fsModel.getAbsolutePath(targetPath, sourcePathName);

                // console.debug("folderPage fileActionDialog onConfirm clipboard action " + action + " sourcePath " + sourcePath);
                if (["copy","cut"].indexOf(action) > -1 && clipboard.get(i).type) {
//                    actualTargetPath = fsModel.getAbsolutePath(targetPath, clipboard.get(i).sourcePathName);
                    console.debug("folderPage fileActionDialog onConfirm Download type " + clipboard.get(i).type + " uid " + clipboard.get(i).uid + " sourcePath " + sourcePath + " actualTargetPath " + actualTargetPath);
                    cloudDriveModel.metadata(cloudDriveModel.getClientType(clipboard.get(i).type), clipboard.get(i).uid, actualTargetPath, sourcePath, -1);
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

        onOpening: {
            if (clipboard.count == 1 && clipboard.get(0).action != "delete") {
                // Copy/Move/Delete first file from clipboard.
                // Check if there is existing file on target folder. Then show overwrite dialog.
                fileActionDialog.sourcePath = clipboard.get(0).sourcePath;
                fileActionDialog.sourcePathName = (clipboard.get(0).sourcePathName) ? clipboard.get(0).sourcePathName : fsModel.getFileName(clipboard.get(0).sourcePath);
                if (!canCopy()) {
                    fileActionDialog.isOverwrite = true;
                    fileName.forceActiveFocus();
                    fileName.text = getNewName();
                }
            }
        }

        onClosed: {
            fileActionDialog.isOverwrite = false;
            fileActionDialog.sourcePath = "";
            fileActionDialog.sourcePathName = "";
            fileName.text = "";
            overwriteFile.checked = false;

            // Always clear clipboard's delete actions.
            clipboard.clearDeleteActions();

            // Resume queued jobs.
            fsModel.resumeNextJob();
            cloudDriveModel.resumeNextJob();
        }
    }

    ConfirmDialog {
        id: newFolderDialog
        titleText: appInfo.emptyStr+qsTr("New folder / file")
        titleIcon: "FilesPlusIcon.svg"
        buttonTexts: [appInfo.emptyStr+qsTr("OK"), appInfo.emptyStr+qsTr("Cancel")]
        content: Column {
            width: parent.width - 10
            height: 120
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 10

            Row {
                width: parent.width
                spacing: 10
                RadioButton {
                    id: newFolderButton
                    width: (parent.width - parent.spacing) / 2
                    text: "<font color='white'>" + appInfo.emptyStr+qsTr("Folder") + "</font>"
                    onClicked: {
                        newFileButton.checked = false;
                    }
                }
                RadioButton {
                    id: newFileButton
                    width: (parent.width - parent.spacing) / 2
                    text: "<font color='white'>" + appInfo.emptyStr+qsTr("File") + "</font>"
                    onClicked: {
                        newFolderButton.checked = false;
                    }
                }
            }

            TextField {
                id: folderName
                width: parent.width
                placeholderText: appInfo.emptyStr+(newFolderButton.checked ? qsTr("Please input folder name.") : qsTr("Please input file name."))
            }
        }

        onOpened: {
            newFolderButton.checked = true;
            newFileButton.checked = false;
            folderName.text = "";
        }

        onClosed: {
            folderName.text = "";
        }

        onConfirm: {
            if (folderName.text !== "") {
                var res = false;
                if (newFolderButton.checked) {
                    res = fsModel.createDir(folderName.text);
                } else {
                    res = fsModel.createEmptyFile(folderName.text);
                }

                refreshSlot();
            }
        }
    }

    ConfirmDialog {
        id: renameDialog

        property string sourcePath

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
                text: appInfo.emptyStr+qsTr("Rename %1 to").arg(fsModel.getFileName(renameDialog.sourcePath));
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
            newName.text = fsModel.getFileName(renameDialog.sourcePath);
            newName.forceActiveFocus();
        }

        onClosed: {
            newName.text = "";
        }

        onConfirm: {
            if (newName.text != "" && newName.text != fsModel.getFileName(renameDialog.sourcePath)) {
                var res = fsModel.renameFile(fsModel.getFileName(renameDialog.sourcePath), newName.text);
                refreshSlot();
            }
        }
    }

    ConfirmDialog {
        id: cancelQueuedFolderSizeJobsConfirmation
        titleText: appInfo.emptyStr+qsTr("Cancel file action jobs")
        content: Column {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - 20
            spacing: 3
                Text {
                id: content
                color: "white"
                font.pointSize: 16
                wrapMode: Text.Wrap
                width: parent.width - 20
                height: implicitHeight
                anchors.horizontalCenter: parent.horizontalCenter
            }
            CheckBox {
                id: rollbackFlag
                text: appInfo.emptyStr+qsTr("Rollback changes")
                checked: false
            }
        }
        onOpening: {
            var jobCount = fsModel.getQueuedJobCount();
            if (jobCount > 0) {
                content.text = appInfo.emptyStr+qsTr("Cancel %n job(s) and abort file action ?", "", jobCount);
            } else {
                content.text = appInfo.emptyStr+qsTr("Abort file action ?");
            }
            rollbackFlag.checked = false;
        }
        onConfirm: {
            fsModel.cancelQueuedJobs();
            // Abort thread with rollbackFlag.
            fsModel.abortThread(rollbackFlag.checked);
        }
    }

    ConfirmDialog {
        id: dropboxFullAccessConfirmation
        titleText: appInfo.emptyStr+appInfo.emptySetting
                   + ( appInfo.getSettingBoolValue("dropbox.fullaccess.enabled", true)
                      ? qsTr("Dropbox full access")
                      : qsTr("Dropbox app access") )
        contentText: appInfo.emptyStr+appInfo.emptySetting
                     + ( appInfo.getSettingBoolValue("dropbox.fullaccess.enabled", true)
                        ? qsTr("Change to have full access to your Dropbox?")
                        : qsTr("Change to have app access to your Dropbox?") )
                     + "\n\n" + qsTr("FilesPlus needs to convert its database. It will not effect your data.")
                     + "\n\n" + qsTr("Please click OK to continue.")
        onConfirm: {
            // Update setting.
            if (!appInfo.hasSettingValue("dropbox.fullaccess.enabled")) {
                // Set to full access if no setting.
                appInfo.setSettingValue("dropbox.fullaccess.enabled", true);
            } else {
                // Do nothing, value has been set by setting page.
            }

            // Migrate DAT to DB.
            appInfo.setSettingValue("cloudItems.migration.confirmation", true);
            cloudDriveModel.migrateCloudDriveItemsToDB();

            // Update DB.
            cloudDriveModel.updateDropboxPrefix(appInfo.getSettingBoolValue("dropbox.fullaccess.enabled", false));

            window.showMessageDialogSlot("Dropbox " + appInfo.emptyStr+qsTr("notify"),
                                         qsTr("You have changed Dropbox access method.\nPlease re-authorize your accounts before proceed your actions."));
        }
        onReject: {
            // Update setting.
            if (!appInfo.hasSettingValue("dropbox.fullaccess.enabled")) {
                // Set to app access if no setting.
                appInfo.setSettingValue("dropbox.fullaccess.enabled", false);
            } else {
                // Rollback.
                appInfo.setSettingValue("dropbox.fullaccess.enabled", !appInfo.getSettingBoolValue("dropbox.fullaccess.enabled", false) );
            }

            // Check for cloud data migration.
            if (!appInfo.hasSettingValue("cloudItems.migration.confirmation") && cloudDriveModel.getCloudDriveItemsCount() > 0) {
                cloudItemsMigrationConfirmation.open();
            }
        }
    }

    ConfirmDialog {
        id: cloudItemsMigrationConfirmation
        titleText: appInfo.emptyStr+qsTr("Cloud data conversion")
        contentText: appInfo.emptyStr+qsTr("FilesPlus needs to convert its database. It will not effect your data.")
                     + "\n\n" + qsTr("Please click OK to continue.")
        onConfirm: {
            // Migrate DAT to DB.
            cloudDriveModel.migrateCloudDriveItemsToDB();
            appInfo.setSettingValue("cloudItems.migration.confirmation", "true");
        }
        onReject: {
            appInfo.setSettingValue("cloudItems.migration.confirmation", "false");
        }
    }

    ProgressDialog {
        id: migrateProgressDialog
        formatValue: false
        autoClose: true
        titleText: appInfo.emptyStr+qsTr("Cloud data conversion")
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
                cloudDriveModel.removeItemWithChildren(type, uid, localPath);
                fsModel.refreshItem(localPath);
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
                    // TODO Check if cloud client's remotePath is absolute path.
                    if (cloudDriveModel.isRemoteAbsolutePath(type)) {
                        // Use remoteParentPath + "/" + local file/folder name.
                        remotePath = (remoteParentPath == "/" ? "" : remoteParentPath) + "/" + fsModel.getFileName(localPath);
                        console.debug("cloudDrivePathDialog proceedOperation FilePut adjusted remotePath " + remotePath);

                        if (fsModel.isDir(localPath)) {
                            cloudDriveModel.suspendNextJob();
                            cloudDriveModel.syncFromLocal(type, uid, localPath, remotePath, modelIndex, true);
                            cloudDriveModel.resumeNextJob();
                        } else {
                            cloudDriveModel.filePut(type, uid, localPath, remotePath, modelIndex);
                        }
                    } else {
                        if (fsModel.isDir(localPath)) {
                            cloudDriveModel.suspendNextJob();
                            cloudDriveModel.syncFromLocal_Block(type, uid, localPath, remoteParentPath, modelIndex, true);
                            cloudDriveModel.resumeNextJob();
                        } else {
                            cloudDriveModel.filePut(type, uid, localPath, remoteParentPath, modelIndex);
                        }
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
                    cloudDriveModel.metadata(type, uid, targetLocalPath, remotePath, modelIndex);
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
                if (fsModel.isDir(localPath) && remotePath != "") {
                    console.debug("cloudDrivePathDialog proceedOperation Metadata sync from " + localPath + " to " + remotePath);
                    cloudDriveModel.metadata(type, uid, localPath, remotePath, selectedModelIndex);
                } else {
                    // TODO Check if cloud client's remotePath is absolute path.
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
                            if (contentModel.get(i).name == newRemoteFolderName) {
                                remotePath = contentModel.get(i).absolutePath;
                                console.debug("cloudDrivePathDialog proceedOperation Metadata found remotePath " + remotePath + " for " + newRemoteFolderName);
                                break;
                            }
                        }
                        // Create remote folder and get its remote path (id).
                        if (remotePath == "") {
                            remotePath = cloudDriveModel.createFolder_Block(type, uid, remoteParentPath, newRemoteFolderName);
                        }
                        // Start sync with remotePath.
                        if (remotePath != "") {
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
                proceedOperation(selectedCloudType, selectedUid, localPath, selectedRemotePath, selectedRemotePathName, selectedIsDir, remoteParentPath, selectedModelIndex);
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

    BluetoothSelectionDialog {
        id: btSelectionDialog
        model: ListModel {
            id: btSelectionModel
            function findDeviceAddress(deviceAddress) {
                for (var i=0; i<btSelectionModel.count; i++) {
                    var existingDeviceAddress = btSelectionModel.get(i).deviceAddress;
                    if (existingDeviceAddress === deviceAddress) {
                        return i;
                    }
                }
                return -1;
            }
        }
        discovery: btClient.discovery
        onSelected: {
            btClient.sendFile(localPath, deviceAddress);
        }
        onRejected: {
            btClient.stopDiscovery();
            if (appInfo.getSettingBoolValue("keep.bluetooth.off", false)) {
                btClient.powerOff();
            }
        }
        onStatusChanged: {
            if (status == DialogStatus.Open) {
                btClient.requestDiscoveredDevices();
                btClient.startDiscovery(false, true);
            }
        }
    }

    UploadProgressDialog {
        id: btUploadProgressDialog
        titleText: appInfo.emptyStr+qsTr("Bluetooth transfering")
        buttonTexts: [appInfo.emptyStr+qsTr("Cancel")]
        onButtonClicked: {
            if (index == 0) {
                if (!btClient.isFinished) {
                    btClient.uploadAbort();
                }
            }
        }
    }

    ConfirmDialog {
        id: btPowerOnDialog
        titleText: appInfo.emptyStr+qsTr("Bluetooth transfering")
        titleIcon: "FilesPlusIcon.svg"
        contentText: appInfo.emptyStr+qsTr("Transfering requires Bluetooth.\
\n\nPlease click 'OK' to turn Bluetooth on.")
        onConfirm: {
            btClient.powerOn();
            btSelectionDialog.open();
        }
    }

    BluetoothClient {
        id: btClient

        onHostModeStateChanged: {
            if (hostMode != BluetoothClient.HostPoweredOff) {
                btClient.startDiscovery();
            }
        }

        onDiscoveryChanged: {
            console.debug("btClient.onDiscoveryChanged " + discovery);
        }

        onServiceDiscovered: {
            var i = btSelectionModel.findDeviceAddress(deviceAddress);
            console.debug("btClient.onServiceDiscovered " + deviceName + " " + deviceAddress + " " + isTrusted + " " + isPaired + " i " + i);
            var jsonObj = {
                "deviceName": deviceName,
                "deviceAddress": deviceAddress,
                "isTrusted": isTrusted,
                "isPaired": isPaired
            };
            if (i === -1) {
                btSelectionModel.append(jsonObj);
            } else {
                btSelectionModel.set(i, jsonObj);
            }
        }

        onRequestPowerOn: {
            btPowerOnDialog.open();
        }

        onUploadStarted: {
            btUploadProgressDialog.srcFilePath = localPath;
            btUploadProgressDialog.autoClose = true;
            btUploadProgressDialog.min = 0;
            btUploadProgressDialog.max = 0;
            btUploadProgressDialog.value = 0;
            btUploadProgressDialog.indeterminate = true;
            btUploadProgressDialog.message = "";
            btUploadProgressDialog.open();
        }

        onUploadProgress: {
            btUploadProgressDialog.indeterminate = false;
            btUploadProgressDialog.max = bytesTotal;
            btUploadProgressDialog.value = bytesSent;
        }

        onUploadFinished: {
            btUploadProgressDialog.indeterminate = false;
            btUploadProgressDialog.message = msg;
            btClient.powerOff();
        }

        Component.onCompleted: {
            console.debug(Utility.nowText() + " folderPage btClient onCompleted");
            window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("BTClient"), 0.1);

            // Workaround for meego to initialize SelectionDialog height.
            btClient.requestDiscoveredDevices();
        }
    }
}
