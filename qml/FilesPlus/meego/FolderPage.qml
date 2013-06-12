import QtQuick 1.1
import com.nokia.meego 1.0
import Charts 1.0
import FolderSizeItemListModel 1.0 // For enum reference.
import CloudDriveModel 1.0 // For enum reference.
import "Utility.js" as Utility

Page {
    id: folderPage

    property string name: "folderPage"
    property bool inverted: !theme.inverted

    state: "list"
    states: [
        State {
            name: "chart"
        },
        State {
            name: "list"
        },
        State {
            name: "grid"
        }
    ]
    transitions: [
        Transition {
            ScriptAction { script: toggleSortFlag() }
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
                if (fsModel.state == "mark") {
                    fsModel.state = "";
                    fsModel.unmarkAll();
                } else {
                    // Specify local path to focus after cd to parent directory..
                    fsListView.focusLocalPath = fsModel.currentDir;
                    fsGridView.focusLocalPath = fsModel.currentDir;
                    goUpSlot();
                }
            }
        }

        ToolBarButton {
            id: refreshButton
            buttonIconSource: "toolbar-refresh"
            visible: (fsModel.state == "")

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
            id: actionButton

            buttonIconSource: {
                if (fsModel.state == "mark") {
                    return (!inverted ? "ok.svg" : "ok_inverted.svg");
                } else {
                    if (folderPage.state == "chart") {
                        return (!inverted ? "list.svg" : "list_inverted.svg");
                    } else if (folderPage.state == "list") {
                            return (!inverted ? "grid.svg" : "grid_inverted.svg");
                    } else {
                        return (!inverted ? "chart.svg" : "chart_inverted.svg");
                    }
                }
            }

            onClicked: {
                if (fsModel.state == "mark") {
                    if (markMenu.isMarkAll) {
                        fsModel.unmarkAll();
                    } else {
                        fsModel.markAll();
                    }
                    markMenu.isMarkAll = !markMenu.isMarkAll;
                } else {
                    if (folderPage.state == "chart") {
                        folderPage.state = "list";
                    } else if (folderPage.state == "list") {
                        folderPage.state = "grid";
                    } else {
                        folderPage.state = "chart";
                    }
                }
            }
        }

        ToolBarButton {
            id: cloudButton
            buttonIconSource: (!inverted ? "cloud.svg" : "cloud_inverted.svg")
            visible: (fsModel.state == "")

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

                if (fsModel.state == "mark") {
                    if (!fsModel.isAnyItemChecked()) {
                        markAllMenu.open();
                    } else {
                        markMenu.open();
                    }
                } else if (folderPage.state != "chart") {
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
            fsModel.state = "mark";
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
            folderPage.state = "list";
            pageStack.pop(folderPage);
        }
        onOpenBookmarks: {
            bookmarksMenu.open();
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
                return fsModel.state != "mark";
            } else if (menuItem.name == "syncCurrentFolder") {
                return !fsModel.isRoot() && cloudDriveModel.canSync(fsModel.currentDir);
            } else {
                return true;
            }
        }
    }

    BookmarksMenu {
        id: bookmarksMenu

        onAddBookmark: {
            bookmarksModel.addBookmark(-1, "", fsModel.currentDir, fsModel.currentDir);
        }
        onOpenBookmarks: {
            pageStack.push(Qt.resolvedUrl("BookmarksPage.qml"));
        }
    }

    ChartMenu {
        id: chartMenu

        onDrives: {
            // Flip back to list view, then pop folderPage.
            folderPage.state = "list";
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
                fsModel.markAll();
            } else {
                fsModel.unmarkAll();
            }
            isMarkAll = !isMarkAll;
        }
        onCopyMarkedItems: {
            folderPage.copyMarkedItems();
            fsModel.state = "";
            fsListView.currentIndex = -1;
            fsGridView.currentIndex = -1;
        }
        onCutMarkedItems: {
            folderPage.cutMarkedItems();
            fsModel.state = "";
            fsListView.currentIndex = -1;
            fsGridView.currentIndex = -1;
        }
        onDeleteMarkedItems: {
            folderPage.deleteMarkedItems();
            fsModel.state = "";
            fsListView.currentIndex = -1;
            fsGridView.currentIndex = -1;
        }
        onSyncMarkedItems: {
            folderPage.syncMarkedItems();
            fsModel.state = "";
            fsListView.currentIndex = -1;
            fsGridView.currentIndex = -1;
        }
        onStatusChanged: {
            if (status == DialogStatus.Opening) {
                isMarkAll = !fsModel.areAllItemChecked();
            }
        }
    }

    MarkAllMenu {
        id: markAllMenu

        onMarkAll: {
            fsModel.markAll();
        }
        onMarkAllFiles: {
            fsModel.markAllFiles();
        }
        onMarkAllFolders: {
            fsModel.markAllFolders();
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

    CompressedFileMenu {
        id: compressedFileMenu

        onOpenCompressedFile: {
            var item = fsModel.get(selectedIndex);
            pageStack.push(Qt.resolvedUrl("CompressedFolderPage.qml"),
                           { compressedFilePath: item.absolutePath, compressedFileName: item.name });
        }
        onExtract: {
            var item = fsModel.get(selectedIndex);
            compressedFolderModel.extract(item.absolutePath);
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
            folderPage.state = "list";
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

    function cancelAllFolderSizeJobsSlot() {
        fsModel.cancelQueuedJobs();
        // Abort thread with rollbackFlag=false.
        fsModel.abortThread(false);
    }

    function showDropboxFullAccessConfirmationSlot() {
        dropboxFullAccessConfirmation.open();
    }

    function postBrowseReplySlot() {
        cloudDrivePathDialog.postBrowseReplySlot();
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
        fsGridView.currentIndex = -1;

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

    // Selection functions.

    function cutMarkedItems() {
        for (var i=0; i<fsModel.count; i++) {
            if (fsModel.getProperty(i, FolderSizeItemListModel.IsCheckedRole)) {
                console.debug("folderPage cutMarkedItems item"
                              + " absolutePath " + fsModel.getProperty(i, FolderSizeItemListModel.AbsolutePathRole)
                              + " isChecked " + fsModel.getProperty(i, FolderSizeItemListModel.IsCheckedRole));

                clipboard.addItemWithSuppressCountChanged({ "action": "cut", "sourcePath": fsModel.getProperty(i, FolderSizeItemListModel.AbsolutePathRole) });
            }

            // Reset isChecked.
            fsModel.setProperty(i, FolderSizeItemListModel.IsCheckedRole, false);
        }

        // Emit suppressed signal.
        clipboard.emitCountChanged();
    }

    function copyMarkedItems() {
        for (var i=0; i<fsModel.count; i++) {
            if (fsModel.getProperty(i, FolderSizeItemListModel.IsCheckedRole)) {
                console.debug("folderPage copyMarkedItems item"
                              + " absolutePath " + fsModel.getProperty(i, FolderSizeItemListModel.AbsolutePathRole)
                              + " isChecked " + fsModel.getProperty(i, FolderSizeItemListModel.IsCheckedRole));

                clipboard.addItemWithSuppressCountChanged({ "action": "copy", "sourcePath": fsModel.getProperty(i, FolderSizeItemListModel.AbsolutePathRole) });
            }

            // Reset isChecked.
            fsModel.setProperty(i, FolderSizeItemListModel.IsCheckedRole, false);
        }

        // Emit suppressed signal.
        clipboard.emitCountChanged();
    }

    function deleteMarkedItems() {
        for (var i=0; i<fsModel.count; i++) {
            if (fsModel.getProperty(i, FolderSizeItemListModel.IsCheckedRole)) {
                console.debug("folderPage deleteMarkedItems item"
                              + " absolutePath " + fsModel.getProperty(i, FolderSizeItemListModel.AbsolutePathRole)
                              + " isChecked " + fsModel.getProperty(i, FolderSizeItemListModel.IsCheckedRole));

                clipboard.addItem({ "action": "delete", "sourcePath": fsModel.getProperty(i, FolderSizeItemListModel.AbsolutePathRole) });
            }

            // Reset isChecked.
            fsModel.setProperty(i, FolderSizeItemListModel.IsCheckedRole, false);
        }

        // Open confirmation dialog.
        fileActionDialog.open();
    }

    function syncMarkedItems() {
        for (var i=0; i<fsModel.count; i++) {
            if (fsModel.getProperty(i, FolderSizeItemListModel.IsCheckedRole)) {
                console.debug("folderPage syncMarkedItems item"
                              + " absolutePath " + fsModel.getProperty(i, FolderSizeItemListModel.AbsolutePathRole)
                              + " isChecked " + fsModel.getProperty(i, FolderSizeItemListModel.IsCheckedRole));

                clipboard.addItem({ "action": "sync", "sourcePath": fsModel.getProperty(i, FolderSizeItemListModel.AbsolutePathRole) });
            }

            // Reset isChecked.
            fsModel.setProperty(i, FolderSizeItemListModel.IsCheckedRole, false);
        }

        // Invoke syncClipboard.
        syncClipboardItems();
    }

    PieChart {
        id: pieChartView
        width: parent.width - 4
        height: parent.height - currentPath.height
        anchors.top: currentPath.bottom
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
                folderPage.state = "list";
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
        }

        Component.onCompleted: {
            console.debug(Utility.nowText() + " folderPage pieChartView onCompleted");
            window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("PieChartView"), 0.1);
        }
    }

    ListView {
        id: fsListView
        visible: (folderPage.state == "list")
        width: parent.width
        height: parent.height - currentPath.height - (nameFilterPanel.visible ? nameFilterPanel.height : 0)
        anchors.top: currentPath.bottom
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

        property int lastCenterIndex
        property string focusLocalPath

        onMovementStarted: {
            if (currentItem) {
                // Hide highlight and popupTool.
                currentItem.pressed = false;
                currentIndex = -1;
                popupToolPanel.visible = false;
            }
        }

        onMovementEnded: {
            lastCenterIndex = fsListView.indexAt(0, contentY + (height / 2));
        }

        onCountChanged: {
//            console.debug("fsListView onCountChanged " + count + " model.count " + model.count);
            if (count == model.count) {
                if (focusLocalPath != "") {
                    // Focus on specified local path's index.
                    // The path is set by back button or listItem onClicked as "." to avoid focusing any item.
                    var focusIndex = fsModel.getIndexOnCurrentDir(focusLocalPath);
                    if (focusIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
                        fsListView.positionViewAtIndex(focusIndex, ListView.Center);
                    }
                    focusLocalPath = "";
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
            scrollBarWidth: 80
            indicatorBarHeight: 80
            scrollBarColor: "grey"
            indicatorWidth: 5
            autoHideInterval: appInfo.emptySetting+appInfo.getSettingValue("QuickScrollPanel.timer.interval", 2) * 1000
        }
    }

    GridView {
        id: fsGridView
        visible: (folderPage.state == "grid")
        width: parent.width
        height: parent.height - currentPath.height - (nameFilterPanel.visible ? nameFilterPanel.height : 0)
        anchors.top: currentPath.bottom
        cellWidth: appInfo.emptySetting + (appInfo.getSettingBoolValue("GridView.compact.enabled", false) ? 120 : 160)
        cellHeight: appInfo.emptySetting + (appInfo.getSettingBoolValue("GridView.compact.enabled", false) ? 120 : 160)
        highlightRangeMode: GridView.NoHighlightRange
        highlightFollowsCurrentItem: true
        highlightMoveDuration: 1
        highlight: Rectangle {
            gradient: highlightGradient
        }
        clip: true
        focus: true
        cacheBuffer: height * 2
        pressDelay: 200
        model: fsModel
        delegate: gridItemDelegate

        property int lastCenterIndex
        property string focusLocalPath

        onMovementStarted: {
            if (currentItem) {
                // Hide highlight and popupTool.
                currentIndex = -1;
                popupToolPanel.visible = false;
            }
        }

        onMovementEnded: {
            lastCenterIndex = fsGridView.indexAt(0, contentY + (height / 2));
        }

        onCountChanged: {
//            console.debug("fsListView onCountChanged " + count + " model.count " + model.count);
            if (count == model.count) {
                if (focusLocalPath != "") {
                    // Focus on specified local path's index.
                    // The path is set by back button or listItem onClicked as "." to avoid focusing any item.
                    var focusIndex = fsModel.getIndexOnCurrentDir(focusLocalPath);
                    if (focusIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
                        fsGridView.positionViewAtIndex(focusIndex, GridView.Center);
                    }
                    focusLocalPath = "";
                } else {
                    // Keep focus on last center index.
                    fsGridView.positionViewAtIndex(lastCenterIndex, GridView.Center);
                }
            }
        }

        QuickScrollPanel {
            id: gridQuickScrollPanel
            listView: parent
            indicatorBarTitle: (modelIndex < 0) ? ""
                               : ( fsModel.sortFlag == FolderSizeItemListModel.SortByTime
                                  ? Qt.formatDateTime(fsModel.get(modelIndex).lastModified, "d MMM yyyy")
                                  : fsModel.get(modelIndex).name )
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
            listViewState: (fsModel.state ? fsModel.state : "")
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
            isImageUrlCachable: true
            opacity: isHidden ? 0.5 : 1

            onPressAndHold: {
                if (fsModel.state != "mark") {
                    fsListView.currentIndex = index;
                    popupToolPanel.selectedFilePath = absolutePath;
                    popupToolPanel.selectedFileName = name;
                    popupToolPanel.selectedFileIndex = index;
                    popupToolPanel.isDir = isDir;
                    popupToolPanel.pastePath = (isDir) ? absolutePath : currentPath.text;
                    var panelX = x + mouseX - fsListView.contentX;
                    var panelY = y + mouseY - fsListView.contentY + fsListView.y;
                    popupToolPanel.open(panelX, panelY);
                }
            }

            onClicked: {
                if (fsModel.state == "mark") {
                    fsModel.setProperty(index, FolderSizeItemListModel.IsCheckedRole, !isChecked);
                } else {
                    if (isDir) {
                        fsListView.focusLocalPath = "."; // Put dummy path to avoid focus on lastCenterIndex.
                        changeDirSlot(name);
                    } else {
                        // If file is running, disable preview.
                        if (isRunning) return;

                        // Implement internal viewers for image(JPG,PNG), text with addon(cloud drive, print)
                        if (viewableImageFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                            // TODO Populate ImageViewModel with mediaUrl = image://local/...
                            pageStack.push(Qt.resolvedUrl("ImageViewPage.qml"),
                                           { model: fsModel, selectedFilePath: absolutePath });
                        } else if (viewableTextFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                            pageStack.push(Qt.resolvedUrl("TextViewPage.qml"),
                                           { filePath: absolutePath, fileName: name });
                        } else if (fileType.toUpperCase() === "ZIP") {
                            compressedFileMenu.selectedIndex = index;
                            compressedFileMenu.open();
                        } else {
                            Qt.openUrlExternally(fsModel.getUrl(absolutePath));
                        }
                    }
                }
            }
        }
    }

    Component {
        id: gridItemDelegate

        CloudGridItem {
            id: gridItem
            gridViewState: (fsModel.state ? fsModel.state : "")
            runningIconSource: fsModel.getRunningOperationIconSource(runningOperation)
            syncIconVisible: !runningIconVisible && (cloudDriveModel.isConnected(absolutePath) || cloudDriveModel.isSyncing(absolutePath))
            syncIconSource: {
                if (cloudDriveModel.isSyncing(absolutePath)) {
                    return "cloud_wait.svg";
                } else if (cloudDriveModel.isDirty(absolutePath, lastModified)) {
                    return "cloud_dirty.svg";
                } else {
                    return "cloud.svg";
                }
            }
            opacity: isHidden ? 0.5 : 1
            width: fsGridView.cellWidth
            height: fsGridView.cellHeight
            gridItemIconBusyVisible: true
            isImageUrlCachable: true
            subIconMargin: appInfo.emptySetting + (appInfo.getSettingBoolValue("GridView.compact.enabled", false) ? 10 : 32) // 32 for 3 columns, 10 for 4 columns

            onPressAndHold: {
                if (fsModel.state != "mark") {
                    fsGridView.currentIndex = index;
                    popupToolPanel.selectedFilePath = absolutePath;
                    popupToolPanel.selectedFileName = name;
                    popupToolPanel.selectedFileIndex = index;
                    popupToolPanel.isDir = isDir;
                    popupToolPanel.pastePath = (isDir) ? absolutePath : currentPath.text;
                    var panelX = x + mouse.x - fsGridView.contentX;
                    var panelY = y + mouse.y - fsGridView.contentY + fsGridView.y;
                    popupToolPanel.open(panelX, panelY);
                }
            }

            onClicked: {
                if (fsModel.state == "mark") {
                    fsModel.setProperty(index, FolderSizeItemListModel.IsCheckedRole, !isChecked);
                } else {
                    if (isDir) {
                        fsGridView.focusLocalPath = "."; // Put dummy path to avoid focus on lastCenterIndex.
                        changeDirSlot(name);
                    } else {
                        // If file is running, disable preview.
                        if (isRunning) return;

                        // Implement internal viewers for image(JPG,PNG), text with addon(cloud drive, print)
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

        property variant view: (folderPage.state == "list") ? fsListView : fsGridView

        onPrevious: {
            lastFindIndex = fsModel.findIndexByNameFilter(nameFilter, --lastFindIndex, true);
            console.debug("folderPage nameFilterPanel onPrevious nameFilter " + nameFilter + " lastFindIndex " + lastFindIndex);
            if (lastFindIndex > -1) {
                view.positionViewAtIndex(lastFindIndex, ListView.Beginning);
                view.currentIndex = lastFindIndex;
            }
        }

        onNext: {
            lastFindIndex = fsModel.findIndexByNameFilter(nameFilter, ++lastFindIndex, false);
            console.debug("folderPage nameFilterPanel onNext nameFilter " + nameFilter + " lastFindIndex " + lastFindIndex);
            if (lastFindIndex > -1) {
                view.positionViewAtIndex(lastFindIndex, ListView.Beginning);
                view.currentIndex = lastFindIndex;
            }
        }

        onOpened: {
            // Turn highlight on.
//            view.highlightFollowsCurrentItem = true;
            lastFindIndex = view.currentIndex;
        }

        onClosed: {
            // Turn highlight off.
//            view.highlightFollowsCurrentItem = false;
            view.currentIndex = -1;
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
            } else if (buttonName == "share") {
                return fsModel.isFile(selectedFilePath) || cloudDriveModel.isConnected(selectedFilePath);
            } else if (buttonName == "shareFile") {
                return fsModel.isFile(selectedFilePath);
            } else if (buttonName == "shareUrl") {
                return cloudDriveModel.isConnected(selectedFilePath);
            } else if (buttonName == "editFile") {
                return fsModel.isFile(selectedFilePath);
            } else if (buttonName == "compress") {
                return !compressedFolderModel.isCompressedFile(selectedFilePath);
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
            fsGridView.currentIndex = -1;
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
            fsGridView.currentIndex = srcItemIndex;
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
            fsModel.state = "mark";
            fsModel.setProperty(srcItemIndex, FolderSizeItemListModel.IsCheckedRole, true);
        }

        onUploadFile: {
            uploadFileSlot(srcFilePath, srcItemIndex);
        }

        onDownloadFile: {
            downloadFileSlot(srcFilePath, srcItemIndex);
        }

        onShareFile: {
            helper.shareFile(srcFilePath);
        }

        onShareUrl: {
            if (cloudDriveModel.isConnected(srcFilePath)) {
                uidDialog.localPath = srcFilePath;
                uidDialog.operation = CloudDriveModel.ShareFile;
                uidDialog.caller = "shareUrlSlot";
                uidDialog.open();
            }
        }

        onEditFile: {
            pageStack.push(Qt.resolvedUrl("TextViewPage.qml"),
                           { filePath: srcFilePath, fileName: fsModel.getFileName(srcFilePath) });
        }

        onShowInfo: {
            filePropertiesDialog.show(srcItemIndex);
        }

        onCompress: {
            compressedFolderModel.compress(srcFilePath);
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
                } else if (action == "extract" && !clipboard.get(i).type) {
                    res = res && compressedFolderModel.extract(sourcePath, clipboard.get(i).extractFileList, targetPath);
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
                    // Check if it's required to dirty before syncing.
                    if (cloudDriveModel.isDirtyBeforeSync(type)) {
                        cloudDriveModel.updateItem(type, uid, localPath, cloudDriveModel.dirtyHash);
                    }
                    cloudDriveModel.metadata(type, uid, localPath, remotePath, modelIndex);
                } else {
                    // If it's not connected, show cloudDrivePathDialog.
                    cloudDrivePathDialog.open();
                    cloudDrivePathDialog.remotePath = remotePath;
                    cloudDrivePathDialog.changeRemotePath("");
                }
                break;
            case CloudDriveModel.FilePut:
                // Show cloudDrivePathDialog.
                cloudDrivePathDialog.open();
                cloudDrivePathDialog.remotePath = ""; // Undefine remote path for local path.
                cloudDrivePathDialog.changeRemotePath("");
                break;
            case CloudDriveModel.FileGet:
                // Show cloudDrivePathDialog.
                cloudDrivePathDialog.open();
                cloudDrivePathDialog.remotePath = ""; // Undefine remote path for local path.
                cloudDrivePathDialog.changeRemotePath("");
                break;
            case CloudDriveModel.ShareFile:
                cloudDriveModel.shareFileCaller = uidDialog.caller;
                cloudDriveModel.shareFile(type, uid, localPath, remotePath);
                break;
            case CloudDriveModel.DeleteFile:
                // NOTE Keep local disconnected item because you have requested to delete only connected cloud item.
                cloudDriveModel.deleteFile(type, uid, localPath, remotePath, true);
                break;
            case CloudDriveModel.Disconnect:
                cloudDriveModel.disconnect(type, uid, localPath);
                break;
            case CloudDriveModel.Browse:
                // Show cloudDrivePathDialog.
                cloudDrivePathDialog.open();
                cloudDrivePathDialog.remotePath = remotePath;
                cloudDrivePathDialog.changeRemotePath("");
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
                        cloudDrivePathDialog.changeRemotePath("");
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
                        for (var i=0; i<cloudDriveModel.count; i++) {
                            // TODO Find exactly match with name and isDir.
                            if (cloudDriveModel.get(i).name == newRemoteFolderName) {
                                remotePath = cloudDriveModel.get(i).absolutePath;
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
            var actualRemoteParentPath = (remoteParentPath == "") ? cloudDriveModel.getParentRemotePath(selectedCloudType, remotePath) : remoteParentPath;
            cloudDriveModel.browse(selectedCloudType, selectedUid, actualRemoteParentPath);
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
        showAttributes: false // For Meego only.

        function show(index) {
            selectedIndex = index;
            selectedItem = fsModel.get(selectedIndex);
            isHidden = selectedItem.isHidden;
            isReadOnly = selectedItem.isReadOnly;
            populateCloudItemModel();
            open();
        }

        function populateCloudItemModel() {
            cloudItemModel.clear();
            if (selectedItem) {
                var cloudItems = Utility.createJsonObj(cloudDriveModel.getItemListJson(selectedItem.absolutePath));
                for (var i = 0; i < cloudItems.length; i++) {
                    var cloudItem = cloudItems[i];
                    var tokenPairJsonText = cloudDriveModel.getStoredUid(cloudItem.type, cloudItem.uid);
                    if (tokenPairJsonText != "") {
                        var uidJson = Utility.createJsonObj(tokenPairJsonText);
                        var modelItem = { type: cloudItem.type, uid: cloudItem.uid, email: uidJson.email, absolutePath: (cloudItem.remote_path ? cloudItem.remote_path : qsTr("Not available")) };
                        cloudItemModel.append(modelItem);
                    }
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
        onScheduleSync: {
            uidDialog.selectedCloudType = type;
            uidDialog.selectedUid = uid;
            uidDialog.localPath = selectedItem.absolutePath;
            cloudDriveSchedulerDialog.localPathCronExp = cloudDriveModel.getItemCronExp(type, uid, selectedItem.absolutePath);
            cloudDriveSchedulerDialog.open();
        }
        onToggleHidden: {
            var res = fsModel.setFileAttribute(absolutePath, FolderSizeItemListModel.Hidden, value);
            if (res) {
                refreshSlot("folderPage filePropertiesDialog onToggleHidden");
            }
        }
        onToggleReadOnly: {
            var res = fsModel.setFileAttribute(absolutePath, FolderSizeItemListModel.ReadOnly, value);
            if (res) {
                refreshSlot("folderPage filePropertiesDialog onToggleReadOnly");
            }
        }
    }
}
