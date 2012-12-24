import QtQuick 1.1
import com.nokia.meego 1.0
import Charts 1.0
import FolderSizeItemListModel 1.0
import GCPClient 1.0
import CloudDriveModel 1.0
import QtMobility.contacts 1.1
import QtMobility.connectivity 1.2
import BluetoothClient 1.0
import MessageClient 1.0
import "Utility.js" as Utility

Page {
    id: folderPage

    property string name: "folderPage"
    property alias currentDir: fsModel.currentDir

    states: [
        State {
            name: "chart"
            when: flipable1.flipped
            PropertyChanges {
                target: mainMenu
                disabledMenus: [appInfo.emptyStr+qsTr("Paste"), appInfo.emptyStr+qsTr("Mark multiple items"), appInfo.emptyStr+qsTr("Clear clipboard"), appInfo.emptyStr+qsTr("New folder / file"), appInfo.emptyStr+qsTr("Sync current folder"), appInfo.emptyStr+qsTr("Sync connected items"), appInfo.emptyStr+qsTr("Set name filter"), appInfo.emptyStr+qsTr("Sort by")]
            }
        },
        State {
            name: "list"
            when: !flipable1.flipped
            PropertyChanges {
                target: mainMenu
                disabledMenus: []
            }
        }
    ]

    onStateChanged: {
        console.debug("folderPage onStateChanged state=" + folderPage.state);
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
            quitSlot();
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

        onQuit: {
            quitSlot();
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

    ConfirmDialog {
        id: requestResetCacheConfirmation
        titleText: appInfo.emptyStr+qsTr("First time loading");
        contentText: appInfo.emptyStr+qsTr("Thank you for download FilesPlus.\
\nThis is first time running, FilesPlus needs to load information from your drive.\
\n\nIt will take time depends on numbers of sub folders/files under current folder.\
\n\nPlease click OK to continue.");
        onConfirm: {
            fsModel.refreshDir("folderPage requestResetCacheConfirmation", true);
        }
    }

    ConfirmDialog {
        id: resetCacheConfirmation
        titleText: appInfo.emptyStr+qsTr("Reset folder cache")
        contentText: appInfo.emptyStr+qsTr("Resetting folder cache will take time depends on numbers of sub folders/files under current folder.\n\nPlease click OK to continue.")
        onConfirm: {
            fsModel.refreshDir("folderPage resetCacheConfirmation", true);
        }
    }

    function refreshSlot(caller) {
        caller = (!caller) ? "folderPage refreshSlot" : caller;
        fsModel.refreshDir(caller, false);
    }

    function setNameFiltersAndRefreshSlot(caller) {
        caller = (!caller) ? "folderPage setNameFiltersAndRefreshSlot" : caller;
        if (nameFilterPanel.nameFilters.trim() == "") {
            fsModel.nameFilters = [];
        } else {
            fsModel.nameFilters = [nameFilterPanel.nameFilters.trim()];
        }
        fsModel.refreshDir(caller, false);
    }

    function resetCacheSlot() {
        resetCacheConfirmation.open();
    }

    function goUpSlot() {
        nameFilterPanel.close();
        fsModel.nameFilters = [];

        if (fsModel.isRoot()) {
            // Flip back to list view, then push drivePage.
            flipable1.flipped = false;
//            showDrivePageSlot();
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

    function quitSlot() {
        if (fsModel.isRunning()) {
            messageDialog.titleText = appInfo.emptyStr+qsTr("Notify");
            messageDialog.message = appInfo.emptyStr+qsTr("Reset Cache is running. Please wait until it's done.");
            messageDialog.open();
        } else {
            Qt.quit();
        }
    }

    function syncFileSlot(srcFilePath, selectedIndex, operation) {
        console.debug("folderPage syncFileSlot srcFilePath=" + srcFilePath);

        // Default operation = CloudDriveModel.Metadata
        if (!operation) operation = CloudDriveModel.Metadata;

        if (!cloudDriveModel.canSync(srcFilePath)) return;

        if (!cloudDriveModel.isAuthorized()) {
            // TODO implement for other cloud drive.
            // TODO Go to account page.
            messageDialog.message = appInfo.emptyStr+qsTr("FilesPlus syncs your files via Dropbox service.\
\nYou will be redirected to authorization page.");
            messageDialog.titleText = appInfo.emptyStr+qsTr("Sync with Dropbox");
            messageDialog.autoClose = true;
            messageDialog.open();

            // TODO Supports multiple cloud storage providers.
            cloudDriveModel.requestToken(CloudDriveModel.Dropbox);
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

    function updateFolderSizeItemSlot(localPath, isRunning) {
//        console.debug("folderPage updateFolderSizeItemSlot localPath " + localPath + " isRunning " + isRunning);
        if (localPath == "") return;

        // Show indicator on localPath up to root.
        var pathList = fsModel.getPathToRoot(localPath);
        for(var i=0; i<pathList.length; i++) {
            var modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
//            console.debug("folderPage updateFolderSizeItemSlot " + pathList[i] + " " + modelIndex + " " + isRunning);
            if (modelIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
                fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, isRunning);
            }
        }
    }

    function updateFolderSizeItemProgressBarSlot(localPath, isRunning, cloudDriveOperation, runningValue, runningMaxValue) {
        if (localPath == "") return;

        // Update ProgressBar on listItem.
        var runningOperation = fsModel.mapToFolderSizeListModelOperation(cloudDriveOperation);
        var modelIndex = fsModel.getIndexOnCurrentDir(localPath);
        if (modelIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
            fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, isRunning);
            fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningOperationRole, runningOperation);
            fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningValueRole, runningValue);
            fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningMaxValueRole, runningMaxValue);
        }

        // Show indicator on parent up to root.
        // Skip i=0 as it's notified above already.
        var pathList = fsModel.getPathToRoot(localPath);
        for(var i=1; i<pathList.length; i++) {
            modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
            if (modelIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
                fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, isRunning);
//                fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningOperationRole, FolderSizeItemListModel.SyncOperation);
                fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningOperationRole, runningOperation);
            }
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
            refreshSlot();
        } else {
            fsModel.refreshItem(index);
        }
    }

    function updateJobQueueCount(runningJobCount, jobQueueCount) {
        // Update (runningJobCount + jobQueueCount) on cloudButton.
        cloudButtonIndicator.text = ((runningJobCount + jobQueueCount) > 0) ? (runningJobCount + jobQueueCount) : "";
    }

    function showRecipientSelectionDialogSlot(cloudDriveType, uid, localPath, url) {
        // Open recipientSelectionDialog for sending share link.
        var senderEmail = cloudDriveModel.getUidEmail(cloudDriveType, uid);
        if (senderEmail != "") {
            recipientSelectionDialog.srcFilePath = json.local_file_path;
            recipientSelectionDialog.messageSubject = appInfo.emptyStr+qsTr("Share file on %1").arg(cloudDriveModel.getCloudName(cloudDriveType));
            recipientSelectionDialog.messageBody = appInfo.emptyStr+qsTr("Please download file with below link.") + "\n" + url;
            recipientSelectionDialog.senderEmail = senderEmail;
            recipientSelectionDialog.open();
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

    FolderSizeItemListModel {
        id: fsModel

        // Test only. ISSUE: Setting currentDir to path that requires long caching time ("/home/user") will freeze UI at splash screen.

        function getActionName(fileAction) {
            switch (fileAction) {
            case FolderSizeItemListModel.CopyFile:
                return qsTr("Copy");
            case FolderSizeItemListModel.MoveFile:
                return qsTr("Move");
            case FolderSizeItemListModel.DeleteFile:
                return qsTr("Delete");
            }
        }

        function getRunningOperationIconSource(runningOperation) {
            switch (runningOperation) {
            case FolderSizeItemListModel.ReadOperation:
                return "next.svg"
            case FolderSizeItemListModel.WriteOperation:
                return "back.svg"
            case FolderSizeItemListModel.SyncOperation:
                return (theme.inverted) ? "refresh.svg" : "refresh_inverted.svg"
            case FolderSizeItemListModel.UploadOperation:
                return (theme.inverted) ? "upload.svg" : "upload_inverted.svg"
            case FolderSizeItemListModel.DownloadOperation:
                return (theme.inverted) ? "download.svg" : "download_inverted.svg"
            default:
                return ""
            }
        }

        function mapToFolderSizeListModelOperation(cloudDriveModelOperation) {
            switch (cloudDriveModelOperation) {
            case CloudDriveModel.Metadata:
                return FolderSizeItemListModel.SyncOperation;
            case CloudDriveModel.FileGet:
                return FolderSizeItemListModel.DownloadOperation;
            case CloudDriveModel.FilePut:
                return FolderSizeItemListModel.UploadOperation;
            default:
                return FolderSizeItemListModel.SyncOperation;
            }
        }

        onCurrentDirChanged: {
            console.debug("QML FolderSizeItemListModel::currentDirChanged");
            currentPath.text = fsModel.currentDir;
        }

        onRefreshBegin: {
            console.debug("QML FolderSizeItemListModel::refreshBegin");
//            window.state = "busy";

            fsListView.lastCenterIndex = fsListView.indexAt(0, fsListView.contentY + (fsListView.height / 2));
//            console.debug("QML FolderSizeItemListModel::refreshBegin fsListView.lastCenterIndex " + fsListView.lastCenterIndex);
        }

        onDataChanged: {
            // Suppress as it's too noisy from CloudDriveModel activities.
//            console.debug("QML FolderSizeItemListModel::dataChanged");
        }

        onRefreshCompleted: {
            console.debug("QML FolderSizeItemListModel::refreshCompleted");
//            window.state = "ready";

            // Reset ListView currentIndex.
            fsListView.currentIndex = -1;

            // Auto-sync after refresh. Only dirty items will be sync'd.
            // TODO Suppress in PieView.
            if (appInfo.getSettingBoolValue("sync.after.refresh", false)) {
                syncConnectedItemsSlot(true);
            }
        }

        onRequestResetCache: {
            console.debug("QML FolderSizeItemListModel::onRequestResetCache");
            requestResetCacheConfirmation.open();
        }

        onCopyStarted: {
            console.debug("folderPage fsModel onCopyStarted " + fileAction + " from " + sourcePath + " to " + targetPath + " " + err + " " + msg);
            // TODO Reopen copyProgressDialog if it's not opened.
            var sourceFileName = fsModel.getFileName(sourcePath);
            var targetFileName = fsModel.getFileName(targetPath);
            var targetDirPath = fsModel.getDirPath(targetPath);
            copyProgressDialog.source = appInfo.emptyStr+getActionName(fileAction) + " " + sourceFileName;
            copyProgressDialog.target = appInfo.emptyStr+qsTr("to") + " " + ((sourceFileName == targetFileName) ? targetDirPath : targetFileName);
            copyProgressDialog.lastValue = 0;
            copyProgressDialog.newValue = 0;
            copyProgressDialog.indeterminate = false;
            if (copyProgressDialog.status != DialogStatus.Open) {
                copyProgressDialog.formatValue = true;
                copyProgressDialog.open();
            }
        }

        onCopyProgress: {
//            console.debug("folderPage fsModel onCopyProgress " + fileAction + " from " + sourcePath + " to " + targetPath + " " + bytes + " / " + bytesTotal);
            copyProgressDialog.newValue = bytes+0;
        }

        onCopyFinished: {
            console.debug("folderPage fsModel onCopyFinished " + fileAction + " from " + sourcePath + " to " + targetPath + " " + err + " " + msg + " " + bytes + " " + totalBytes + " " + count);
            copyProgressDialog.count += count;

            // Show message if error.
            if (err < 0) {
                messageDialog.titleText = getActionName(fileAction) + " " + appInfo.emptyStr+qsTr("error");
                messageDialog.message = msg;
                messageDialog.autoClose = true;
                messageDialog.open();

                copyProgressDialog.message = getActionName(fileAction) + " " + sourcePath + " " + appInfo.emptyStr+qsTr("failed") + ".\n";

                // Clear sourcePath from clipboard.
                clipboard.clear();

                // TODO Connect to cloud if copied/moved file/folder is in connected folder.

                // TODO stop queued jobs
            } else {
                // Reset popupToolPanel
                popupToolPanel.srcFilePath = "";
                popupToolPanel.pastePath = "";

                // Remove finished sourcePath from clipboard.
                clipboard.clear();
            }
        }

        onDeleteStarted: {
            console.debug("folderPage fsModel onDeleteStarted " + fileAction + " sourcePath " + sourcePath);
            if (fileAction == FolderSizeItemListModel.DeleteFile) {
                deleteProgressDialog.source = sourcePath;
                if (deleteProgressDialog.status != DialogStatus.Open) {
                    deleteProgressDialog.indeterminate = false;
                    deleteProgressDialog.titleText = appInfo.emptyStr+qsTr("Deleting");
                    deleteProgressDialog.open();
                }
            }
        }

        onDeleteProgress: {
            console.debug("folderPage fsModel onDeleteProgress " + fileAction + " sourceSubPath " + sourceSubPath + " err " + err + " msg " + msg);
            // Update progress only.
            // TODO Save state to property only. Use timer to update state on UI.
            if (fileAction == FolderSizeItemListModel.DeleteFile) {
                if (err >= 0) {
                    deleteProgressDialog.value += 1;
                    deleteProgressDialog.message = msg;
                } else {
                    deleteProgressDialog.message = msg;
                }
            }
        }

        onDeleteFinished: {
            console.debug("folderPage fsModel onDeleteFinished " + fileAction + " sourcePath " + sourcePath);

            // Show message if error.
            if (err < 0) {
                messageDialog.titleText = getActionName(fileAction) + " " + appInfo.emptyStr+qsTr("error");
                messageDialog.message = msg;
                messageDialog.autoClose = true;
                messageDialog.open();

                deleteProgressDialog.message = getActionName(fileAction) + " " + sourcePath + " " + appInfo.emptyStr+qsTr("failed") + ".\n";
            } else {
                // Reset popupToolPanel
                popupToolPanel.selectedFilePath = "";

                // Remove finished sourcePath from clipboard.
                clipboard.clear();
            }

            // Delete file from clouds.
            if (err == 0) {
                if (cloudDriveModel.isConnected(sourcePath)) {
                    var json = Utility.createJsonObj(cloudDriveModel.getItemListJson(sourcePath));
                    for (var i=0; i<json.length; i++) {
                        switch (json[i].type) {
                        case CloudDriveModel.Dropbox:
                            cloudDriveModel.deleteFile(CloudDriveModel.Dropbox, json[i].uid, json[i].local_path, json[i].remote_path);
                            break;
                        case CloudDriveModel.SkyDrive:
                            cloudDriveModel.deleteFile(CloudDriveModel.SkyDrive, json[i].uid, json[i].local_path, json[i].remote_path);
                            break;
                        case CloudDriveModel.Ftp:
                            cloudDriveModel.deleteFile(CloudDriveModel.Ftp, json[i].uid, json[i].local_path, json[i].remote_path);
                            break;
                        }
                    }
                }

                // Reset cloudDriveModel hash on parent. CloudDriveModel will update with actual hash once it got reply.
                var paths = fsModel.getPathToRoot(sourcePath);
                for (var i=1; i<paths.length; i++) {
//                    console.debug("folderPage fsModel onDeleteFinished updateItems paths[" + i + "] " + paths[i]);
                    cloudDriveModel.updateItems(paths[i], cloudDriveModel.dirtyHash);
                }
            }
        }

        onCreateFinished: {
            console.debug("folderPage fsModel onCreateFinished " + targetPath);

            // If created item is not found, refresh.
            var targetIndex = fsModel.getIndexOnCurrentDir(targetPath);
            console.debug("folderPage fsModel onCreateFinished targetIndex " + targetIndex + " FolderSizeItemListModel.IndexOnCurrentDirButNotFound " + FolderSizeItemListModel.IndexOnCurrentDirButNotFound);
            if (targetIndex === FolderSizeItemListModel.IndexOnCurrentDirButNotFound) {
                fsModel.clearIndexOnCurrentDir();
                refreshSlot();
            }

            // Reset cloudDriveModel hash on parent.
            // Reset hash upto root.
            var paths = fsModel.getPathToRoot(targetPath);
            for (var i=0; i<paths.length; i++) {
                console.debug("folderPage fsModel onCreateFinished updateItems paths[" + i + "] " + paths[i]);
                cloudDriveModel.updateItems(paths[i], cloudDriveModel.dirtyHash);
            }

            // TODO request cloudDriveModel.createFolder
        }

        onRenameFinished: {
            console.debug("folderPage fsModel onRenameFinished sourcePath " + sourcePath + " targetPath " + targetPath + " err " + err + " msg " + msg);

            // Rename file on clouds.
            if (err == 0 && cloudDriveModel.isConnected(sourcePath)) {
//                console.debug("folderPage fsModel onRenameFinished itemList " + cloudDriveModel.getItemListJson(sourcePath));
                var json = Utility.createJsonObj(cloudDriveModel.getItemListJson(sourcePath));
                for (var i=0; i<json.length; i++) {
                    switch (json[i].type) {
                    case CloudDriveModel.Dropbox:
                        console.debug("folderPage fsModel onRenameFinished item " + json[i].type + " " + json[i].uid + " " + json[i].local_path + " " + json[i].remote_path);
                        var newRemotePath = cloudDriveModel.getParentRemotePath(json[i].remote_path) + "/" + fsModel.getFileName(targetPath);
                        cloudDriveModel.moveFile(CloudDriveModel.Dropbox, json[i].uid, sourcePath, json[i].remote_path, targetPath, newRemotePath);
                        break;
                    case CloudDriveModel.SkyDrive:
                        // TODO
                        break;
                    case CloudDriveModel.Ftp:
                        // TODO
                        break;
                    }
                }
            }
        }

        onFetchDirSizeStarted: {
            refreshButtonTimer.restart();
        }

        onFetchDirSizeUpdated: {
        }

        onFetchDirSizeFinished: {
            refreshButtonTimer.stop();
            refreshButton.rotation = 0;

            // Refresh itemList to show changes on ListView.
            fsModel.refreshItems();
        }

        Component.onCompleted: {
            console.debug(Utility.nowText() + " folderPage fsModel onCompleted");
            window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("FolderModel"), 0.1);

            // Proceeds queued jobs during constructions.
            fsModel.proceedNextJob();
        }
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

        states: State {
            name: "back"
            PropertyChanges { target: rotation; angle: 180 }
            when: flipable1.flipped
        }

        transitions: Transition {
            NumberAnimation { target: rotation; property: "angle"; duration: 500 }
        }
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
        anchors.fill: parent
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
            // Always clear clipboard before delete marked items.
            clipboard.clear();

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
            // Always clear clipboard before sync marked items.
            clipboard.clear();

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
    }

    NameFilterPanel {
        id: nameFilterPanel
        anchors.bottom: parent.bottom

        onRequestRefresh: {
            setNameFiltersAndRefreshSlot(caller);
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

        property string targetPath

        titleText: appInfo.emptyStr+fileActionDialog.getTitleText()
        contentText: appInfo.emptyStr+fileActionDialog.getText() + "\n"

        function getTitleText() {
            var text = "";
            if (clipboard.count == 1) {
                text = getActionName(clipboard.get(0).action);
            } else {
                // TODO if all copy, show "Multiple copy".
                text = qsTr("Multiple actions");
            }

            return text;
        }

        function getActionName(action) {
            if (action == "copy") return qsTr("Copy");
            else if (action == "cut") return qsTr("Move");
            else if (action == "delete") return qsTr("Delete");
            else return qsTr("Invalid action");
        }

        function getText() {
            // Exmaple of clipboard entry { "action": "cut", "sourcePath": sourcePath }
            var text = "";
            if (clipboard.count == 1) {
                text = getActionName(clipboard.get(0).action)
                        + " " + clipboard.get(0).sourcePath
                        + ((clipboard.get(0).action == "delete")?"":("\n" + qsTr("to") + " " + targetPath))
                        + " ?";
            } else {
                var cutCount = 0;
                var copyCount = 0;
                var deleteCount = 0;
                for (var i=0; i<clipboard.count; i++) {
//                    console.debug("fileActionDialog getText clipboard i " + i + " action " + clipboard.get(i).action + " sourcePath " + clipboard.get(i).sourcePath);
                    if (clipboard.get(i).action == "copy") {
                        copyCount++;
                    } else if (clipboard.get(i).action == "cut") {
                        cutCount++;
                    } else if (clipboard.get(i).action == "delete") {
                        deleteCount++;
                    }
                }

                if (deleteCount>0) text = text + (qsTr("Delete %n item(s)\n", "", deleteCount));
                if (copyCount>0) text = text + (qsTr("Copy %n item(s)\n", "", copyCount));
                if (cutCount>0) text = text + (qsTr("Move %n item(s)\n", "", cutCount));
                if (copyCount>0 || cutCount>0) text = text + qsTr("to") + " " + targetPath;
                text = text + " ?";
            }

            return text;
        }

        function openCopyProgressDialog() {
            // Estimate total.
            var totalBytes = 0;
            var totalFiles = 0;
            var totalFolders = 0;
            for (var i=0; i<clipboard.count; i++) {
                var action = clipboard.get(i).action;
                var sourcePath = clipboard.get(i).sourcePath;
//                console.debug("fileActionDialog openCopyProgressDialog estimate total action " + action + " sourcePath " + sourcePath);
                if (action == "copy" || action == "cut") {
                    var jsonText = fsModel.getItemJson(sourcePath);
//                    console.debug("fileActionDialog openCopyProgressDialog estimate total jsonText " + jsonText);
                    var itemJson = Utility.createJsonObj(jsonText);
                    console.debug("fileActionDialog openCopyProgressDialog estimate total itemJson " + itemJson + " itemJson.size " + itemJson.size + " itemJson.sub_dir_count " + itemJson.sub_dir_count + " itemJson.sub_file_count " + itemJson.sub_file_count);
                    totalBytes += itemJson.size;
                    totalFiles += itemJson.sub_file_count + ((itemJson.is_dir) ? 0 : 1);  // +1 for file.
                    totalFolders += itemJson.sub_dir_count + ((itemJson.is_dir) ? 1 : 0);  // +1 for folder.
                }
            }
            console.debug("fileActionDialog openCopyProgressDialog estimate totalBytes " + totalBytes + " totalFiles " + totalFiles + " totalFolders " + totalFolders);
            if (totalBytes > 0 || totalFiles + totalFolders > 0) {
                // Open ProgressDialog.
                copyProgressDialog.min = 0;
                copyProgressDialog.max = totalBytes;
                copyProgressDialog.value = 0;
                copyProgressDialog.newValue = 0;
                copyProgressDialog.lastValue = 0;
                copyProgressDialog.accuDeltaValue = 0;
                copyProgressDialog.minCount = 0;
                copyProgressDialog.maxCount = totalFiles + totalFolders;
                copyProgressDialog.count = 0;
                copyProgressDialog.indeterminate = true;
                copyProgressDialog.autoClose = true;
                copyProgressDialog.formatValue = true;
                copyProgressDialog.titleText = fileActionDialog.titleText;
                copyProgressDialog.message = "";
                copyProgressDialog.open();
            }
        }

        function openDeleteProgressDialog() {
            // Estimate total.
            var totalFiles = 0;
            var totalFolders = 0;
            for (var i=0; i<clipboard.count; i++) {
                var action = clipboard.get(i).action;
                var sourcePath = clipboard.get(i).sourcePath;
//                console.debug("fileActionDialog openDeleteProgressDialog estimate total action " + action + " sourcePath " + sourcePath);
                if (action == "delete") {
                    var jsonText = fsModel.getItemJson(sourcePath);
//                    console.debug("fileActionDialog openDeleteProgressDialog estimate total jsonText " + jsonText);
                    var itemJson = Utility.createJsonObj(jsonText);
//                    console.debug("fileActionDialog openDeleteProgressDialog estimate total itemJson " + itemJson + " itemJson.sub_file_count " + itemJson.sub_file_count);
                    totalFiles += itemJson.sub_file_count + ((itemJson.is_dir) ? 0 : 1);  // +1 for file.
                    totalFolders += itemJson.sub_dir_count + ((itemJson.is_dir) ? 1 : 0);  // +1 for folder.
                }
            }
            console.debug("fileActionDialog openDeleteProgressDialog estimate totalFiles " + totalFiles + " totalFolders " + totalFolders);
            if (totalFiles + totalFolders > 0) {
                deleteProgressDialog.minCount = 0;
                deleteProgressDialog.maxCount = 0;
                deleteProgressDialog.count = 0;
                deleteProgressDialog.min = 0;
                deleteProgressDialog.max = totalFiles + totalFolders;
                deleteProgressDialog.value = 0;
                deleteProgressDialog.newValue = 0;
                deleteProgressDialog.lastValue = 0;
                deleteProgressDialog.accuDeltaValue = 0;
                deleteProgressDialog.autoClose = true;
                deleteProgressDialog.titleText = appInfo.emptyStr+qsTr("Deleting");
                deleteProgressDialog.message = "";
                deleteProgressDialog.open();
            }
        }

        onConfirm: {

            if (clipboard.count == 1) {
                // Open progress dialogs if it's related.
                openCopyProgressDialog();
                openDeleteProgressDialog();

                // Copy/Move/Delete first file from clipboard.
                // Check if there is existing file on target folder. Then show overwrite dialog.
                if (clipboard.get(0).action != "delete" && !fsModel.canCopy(clipboard.get(0).sourcePath, targetPath)) {
                    fileOverwriteDialog.sourcePath = clipboard.get(0).sourcePath;
                    fileOverwriteDialog.targetPath = targetPath;
                    fileOverwriteDialog.isCopy = (clipboard.get(0).action == "copy");
                    fileOverwriteDialog.open();
                    return;
                }

                fsModel.suspendNextJob();

                var res = false;
                var actualTargetPath = fsModel.getAbsolutePath(targetPath, fsModel.getFileName(clipboard.get(0).sourcePath));
                if (clipboard.get(0).action == "copy") {
                    res = fsModel.copy(clipboard.get(0).sourcePath, actualTargetPath);
                } else if (clipboard.get(0).action == "cut") {
                    res = fsModel.move(clipboard.get(0).sourcePath, actualTargetPath);
                } else if (clipboard.get(0).action == "delete") {
                    res = fsModel.deleteFile(clipboard.get(0).sourcePath);
                }

                if (res) {
                    // Reset both source and target.
                    clipboard.clear();
                    targetPath = "";
                    popupToolPanel.srcFilePath = "";
                    popupToolPanel.pastePath = "";
                } else {
                    // Close copyProgressDialog as it failed.
                    copyProgressDialog.close();

                    // Close copyProgressDialog as it failed.
                    deleteProgressDialog.close();

                    messageDialog.titleText = getActionName(clipboard.get(0).action);
                    messageDialog.message = appInfo.emptyStr+qsTr("Can't %1 %2 to %3.").arg(getActionName(clipboard.get(0).action).toLowerCase())
                        .arg(clipboard.get(0).sourcePath).arg(targetPath);
                    messageDialog.open();

                    // Reset target only.
                    targetPath = "";
                    popupToolPanel.pastePath = "";
                }
            } else {
                // Open progress dialogs if it's related.
                openCopyProgressDialog();
                openDeleteProgressDialog();

                // TODO openOverwriteDialog.
                // It always replace existing names.

                fsModel.suspendNextJob();

                // TODO Copy/Move/Delete all files from clipboard.
                // Action is {copy, cut, delete}
                for (var i=0; i<clipboard.count; i++) {
                    var action = clipboard.get(i).action;
                    var sourcePath = clipboard.get(i).sourcePath;
                    var actualTargetPath = fsModel.getAbsolutePath(targetPath, fsModel.getFileName(sourcePath));

                    console.debug("folderPage fileActionDialog onButtonClicked clipboard action " + action + " sourcePath " + sourcePath);
                    if (action == "copy") {
                        fsModel.copy(sourcePath, actualTargetPath);
                    } else if (action == "cut") {
                        fsModel.move(sourcePath, actualTargetPath);
                    } else if (action == "delete") {
                        fsModel.deleteFile(sourcePath);
                    } else {
                        console.debug("folderPage fileActionDialog onButtonClicked invalid action " + action);
                    }
                }
            }

            // Clear clipboard as they should have been processed.
            clipboard.clear();
        }

        onClosed: {
            // Always clear clipboard's delete actions.
            clipboard.clearDeleteActions();
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
        property string targetPath

        onOpened: {
            fileName.forceActiveFocus();
            fileName.text = fsModel.getNewFileName(sourcePath, targetPath);
        }

        onClosed: {
            fileName.text = "";
            overwriteFile.checked = false;
        }

        onConfirm: {
            // If paste to current folder, targetPath is ended with / already.
            // If paste to selected folder, targetPath is not ended with /.
            var res = false;
            if (isCopy) {
                res = fsModel.copy(sourcePath, fsModel.getAbsolutePath(targetPath, fileName.text) );
            } else {
                res = fsModel.move(sourcePath, fsModel.getAbsolutePath(targetPath, fileName.text) );
            }
        }

        onReject: {
            copyProgressDialog.close();
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

            messageDialog.titleText = "Dropbox " + appInfo.emptyStr+qsTr("notify");
            messageDialog.message = appInfo.emptyStr+qsTr("You have changed Dropbox access method.\
\nPlease re-authorize your accounts before proceed your actions.");
            messageDialog.open();
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
        id: copyProgressDialog
        autoClose: false
        onOk: {
            // Refresh view after copied/moved.
            refreshSlot();
        }
        onCancel: {
            cancelAllFolderSizeJobsSlot();
            // Refresh view after copied/moved.
            refreshSlot();
        }
        onOpened: {
            fsModel.resumeNextJob();
        }
    }

    ProgressDialog {
        id: deleteProgressDialog
        autoClose: false
        titleText: appInfo.emptyStr+qsTr("Deleting")
        onOk: {
            // Refresh view after copied/moved.
            refreshSlot();
        }
        onCancel: {
            cancelAllFolderSizeJobsSlot();
            // Refresh view after copied/moved.
            refreshSlot();
        }
        onOpened: {
            fsModel.resumeNextJob();
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
                if (selectedCloudType == CloudDriveModel.Dropbox) {
                    cloudDrivePathDialog.remoteParentPath = "/"; // Upload start browsing from root.
                } else if (selectedCloudType == CloudDriveModel.SkyDrive) {
                    cloudDrivePathDialog.remoteParentPath = "me/skydrive"; // Upload start browsing from root.
                } else {
                    cloudDrivePathDialog.remoteParentPath = "";
                }
                cloudDrivePathDialog.refresh();
                break;
            case CloudDriveModel.FileGet:
                // Show cloudDrivePathDialog.
                cloudDrivePathDialog.open();
                cloudDrivePathDialog.remotePath = ""; // Undefine remote path for local path.
                if (selectedCloudType == CloudDriveModel.Dropbox) {
                    cloudDrivePathDialog.remoteParentPath = "/"; // Download start browsing from root.
                } else if (selectedCloudType == CloudDriveModel.SkyDrive) {
                    cloudDrivePathDialog.remoteParentPath = "me/skydrive"; // Download start browsing from root.
                } else {
                    cloudDrivePathDialog.remoteParentPath = "";
                }
                cloudDrivePathDialog.refresh();
                break;
            case CloudDriveModel.ShareFile:
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
                              + " cloudDriveModel.getRemoteName(remotePath) " + cloudDriveModel.getRemoteName(remotePath));

                // Upload selected local path into remote parent path.
                if (remoteParentPath != "") {
                    switch (type) {
                    case CloudDriveModel.Dropbox:
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
                        break;
                    case CloudDriveModel.SkyDrive:
                        if (fsModel.isDir(localPath)) {
                            cloudDriveModel.suspendNextJob();
                            cloudDriveModel.syncFromLocal_SkyDrive(type, uid, localPath, remoteParentPath, modelIndex, true);
                            cloudDriveModel.resumeNextJob();
                        } else {
                            cloudDriveModel.filePut(type, uid, localPath, remotePath, modelIndex);
                        }
                        break;
                    case CloudDriveModel.Ftp:
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
                        break;
                    }
                } else {
                    console.debug("cloudDrivePathDialog proceedOperation FilePut ignored remoteParentPath " + remoteParentPath + " is empty.");
                }

                break;
            case CloudDriveModel.FileGet:
                console.debug("cloudDrivePathDialog proceedOperation FileGet localPath " + localPath + " from remotePath " + remotePath + " remotePathName " + remotePathName + " isDir " + isDir
                              + " remoteParentPath " + remoteParentPath
                              + " fsModel.getDirPath(localPath) " + fsModel.getDirPath(localPath)
                              + " cloudDriveModel.getRemoteName(remotePath) " + cloudDriveModel.getRemoteName(remotePath));

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
                    // localPath is file or remotePath is empty.
                    if (type == CloudDriveModel.Dropbox) {
                        // If localPath is file or remotePath is not specified.
                        // Use remoteParentPath + "/" + folderName.
                        remotePath = (remoteParentPath == "/" ? "" : remoteParentPath) + "/" + fsModel.getFileName(localPath);
                        console.debug("cloudDrivePathDialog proceedOperation Metadata sync from " + localPath + " to " + remotePath);
                        cloudDriveModel.metadata(type, uid, localPath, remotePath, selectedModelIndex);
                    } else if (type == CloudDriveModel.SkyDrive) {
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
                        remotePath = (remoteParentPath == "/" ? "" : remoteParentPath) + "/" + fsModel.getFileName(localPath);
                        console.debug("cloudDrivePathDialog proceedOperation Metadata sync from " + localPath + " to " + remotePath);
                        cloudDriveModel.metadata(type, uid, localPath, remotePath, selectedModelIndex);
                    }
                }

                break;
            }
        }

        onButtonClicked: {
            if (index === 0 && selectedIsValid) { // OK button is clicked.
                console.debug("cloudDrivePathDialog onButtonClicked index " + index
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
            if (selectedCloudType == CloudDriveModel.Dropbox) {
                cloudDriveModel.createFolder(selectedCloudType, selectedUid, "", remoteParentPath + "/" + newRemoteFolderName, -1);
            } else if (selectedCloudType == CloudDriveModel.SkyDrive) {
                cloudDriveModel.createFolder(selectedCloudType, selectedUid, newRemoteFolderName, remoteParentPath, -1);
            } else if (selectedCloudType == CloudDriveModel.Ftp) {
                cloudDriveModel.createFolder(selectedCloudType, selectedUid, "", remoteParentPath + "/" + newRemoteFolderName, -1);
            }
        }

        onRefreshRequested: {
            console.debug("cloudDrivePathDialog onRefreshRequested " + selectedCloudType + " " + remotePath + " " + remoteParentPath);

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

    ContactModel {
        id: favContactModel
        filter: DetailFilter {
            detail: ContactDetail.Favorite
            field: Favorite.Favorite
            matchFlags: Filter.MatchFixedString // Meego only
            value: "true"
        }
        sortOrders: [
            SortOrder {
                detail: ContactDetail.Favorite
                field: Favorite.Favorite
                direction: Qt.AscendingOrder // Meego ascending true > false
            },
            SortOrder {
                detail: ContactDetail.Name
                field: Name.FirstName
                direction: Qt.AscendingOrder
            },
            SortOrder {
                detail: ContactDetail.Name
                field: Name.LastName
                direction: Qt.AscendingOrder
            }
        ]

        function getFavListModel() {
            // Construct model.
            var model = Qt.createQmlObject(
                        'import QtQuick 1.1; ListModel {}', folderPage);

            console.debug("getFavListModel favContactModel.contacts.length " + favContactModel.contacts.length + " error " + favContactModel.error);
            for (var i=0; i<favContactModel.contacts.length; i++)
            {
                var contact = favContactModel.contacts[i];
                var favorite = (contact.favorite) ? contact.favorite.favorite : false;
//                console.debug("getFavListModel contact i " + i + " displayLabel " + contact.displayLabel + " email " + contact.email.emailAddress + " favorite " + favorite);
                model.append({
                                 displayLabel: contact.displayLabel,
                                 email: contact.email.emailAddress,
                                 phoneNumber: contact.phoneNumber.number,
                                 favorite: favorite
                             });
            }

            return model;
        }

        Component.onCompleted: {
            console.debug(Utility.nowText() + " folderPage favContactModel onCompleted");
            window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("ContactModel"), 0.1);
        }
    }

    RecipientSelectionDialog {
        id: recipientSelectionDialog
        shareFileCaller: cloudDriveModel.shareFileCaller
        onOpening: {
            model.destroy();

            model = favContactModel.getFavListModel();

            // Update model for next opening.
            favContactModel.update();
        }
        onAccepted: {
            console.debug("recipientSelectionDialog onAccepted shareFileCaller " + shareFileCaller + " email " + selectedEmail + " senderEmail " + senderEmail + " selectedNumber " + selectedNumber);
            console.debug("recipientSelectionDialog onAccepted messageBody " + messageBody);
            if (shareFileCaller == "mailFileSlot") {
                // ISSUE Mail client doesn't get all message body if it contains URI with & in query string. Worksround by using message client.
//                Qt.openUrlExternally("mailto:" + selectedEmail + "?subject=" + messageSubject + "&body=" + messageBody);
                msgClient.sendEmail(selectedEmail, messageSubject, messageBody);
            } else if (shareFileCaller == "smsFileSlot") {
                // TODO Doesn't work on meego. Needs to use MessageClient.
//                Qt.openUrlExternally("sms:" + selectedNumber + "?body=" + messageBody);
                msgClient.sendSMS(selectedNumber, messageBody);
            }
        }
        onRejected: {
//            // Reset popupToolPanel.
//            popupToolPanel.selectedFilePath = "";
//            popupToolPanel.selectedFileIndex = -1;
//            srcFilePath = "";
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

    MessageClient {
        id: msgClient
    }
}
