import QtQuick 1.1
import com.nokia.meego 1.0
import Charts 1.0
import FolderSizeItemListModel 1.0
import GCPClient 1.0
import CloudDriveModel 1.0
import AppInfo 1.0
import QtMobility.contacts 1.1
import QtMobility.connectivity 1.2
import BluetoothClient 1.0
import MessageClient 1.0
import ClipboardModel 1.0
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

        // Proceeds queued jobs during constructions.
        fsModel.proceedNextJob();
        cloudDriveModel.resumeNextJob();
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

        onQuit: {
			quitSlot();
        }
        onPaste: {
            fileActionDialog.targetPath = fsModel.currentDir;
            fileActionDialog.open();
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
    }

    MarkAllMenu {
        id: markAllMenu
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
            showDrivePageSlot();
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

    function showDrivePageSlot() {
        pageStack.push(Qt.resolvedUrl("DrivePage.qml"), { cloudDriveModel: cloudDriveModel }, true);
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

    function printFileSlot(srcFilePath, selectedIndex) {
        console.debug("folderPage printFileSlot srcFilePath=" + srcFilePath);

        // Set source file to GCPClient.
        gcpClient.selectedFilePath = srcFilePath;
        if (srcFilePath == "") return;

        if (gcpClient.getContentType(srcFilePath) == "") {
            console.debug("folderPage printFileSlot File type is not supported. (" + srcFilePath + ")");

            messageDialog.titleText = appInfo.emptyStr+qsTr("Print Error");
            messageDialog.message = appInfo.emptyStr+qsTr("Can't print %1 \
\nFile type is not supported. Only JPEG, PNG, Text and PDF are supported.").arg(srcFilePath);
            messageDialog.open();
            return;
        }

        if (!gcpClient.isAuthorized()) {
            messageDialog.message = appInfo.emptyStr+qsTr("FilesPlus prints via Google CloudPrint service.\
\nPlease enable printer on your desktop with Chrome or with CloudPrint-ready printer.\
\nYou will be redirected to authorization page.");
            messageDialog.titleText = appInfo.emptyStr+qsTr("Print with CloudPrint");
            messageDialog.open();

            gcpClient.authorize();
        } else {
            var printerListModel = gcpClient.getPrinterListModel();
            console.debug("folderPage printFileSlot printerListModel.count=" + printerListModel.count);
            if (printerListModel.count > 0) {
                printerSelectionDialog.srcFilePath = srcFilePath;
                printerSelectionDialog.model = printerListModel;
                printerSelectionDialog.open();
            } else {
                // TODO Open progress dialog.
                downloadProgressDialog.titleText = appInfo.emptyStr+qsTr("Search for printers");
                downloadProgressDialog.indeterminate = true;
                downloadProgressDialog.open();
                gcpClient.search("");
            }
        }
    }

    function printURLSlot(url) {
        console.debug("folderPage printURLSlot url=" + url);

        // Set source URL to GCPClient.
        gcpClient.selectedURL = url;
        if (url == "") return;

        if (!gcpClient.isAuthorized()) {
            messageDialog.message = appInfo.emptyStr+qsTr("FilesPlus prints via Google CloudPrint service.\
\nPlease enable printer on your desktop with Chrome or with CloudPrint-ready printer.\
\nYou will be redirected to authorization page.");
            messageDialog.titleText = appInfo.emptyStr+qsTr("Print with CloudPrint");
            messageDialog.open();

            gcpClient.authorize();
        } else {
            var printerListModel = gcpClient.getPrinterListModel();
            console.debug("folderPage printFileSlot printerListModel.count=" + printerListModel.count);
            if (printerListModel.count > 0) {
                printerSelectionDialog.srcFilePath = "";
                printerSelectionDialog.srcURL = url;
                printerSelectionDialog.model = printerListModel;
                printerSelectionDialog.open();
            } else {
                // TODO Open progress dialog.
                downloadProgressDialog.titleText = appInfo.emptyStr+qsTr("Search for printers");
                downloadProgressDialog.indeterminate = true;
                downloadProgressDialog.open();
                gcpClient.search("");
            }
        }
    }

    function setGCPClientAuthCode(code) {
        var res = gcpClient.parseAuthorizationCode(code);
        if (res) {
            gcpClient.accessToken();
        }
    }

    function setGCDClientAuthCode(code) {
        var res = cloudDriveModel.parseAuthorizationCode(CloudDriveModel.GoogleDrive, code);
        if (res) {
            cloudDriveModel.accessToken(CloudDriveModel.GoogleDrive);
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

    function dropboxAccessTokenSlot() {
        cloudDriveModel.accessToken(CloudDriveModel.Dropbox);
    }

    function skyDriveAccessTokenSlot(pin) {
        console.debug("folderPage skyDriveAccessTokenSlot pin " + pin);
        if (pin == "" || pin == "PinNotFound") {
            messageDialog.titleText =  appInfo.emptyStr+"SkyDrive "+qsTr("Access Token");
            messageDialog.message = appInfo.emptyStr+qsTr("Error") + " " + qsTr("PIN code is not found.");
            messageDialog.open();
        } else {
            cloudDriveModel.accessToken(CloudDriveModel.SkyDrive, pin);
        }
    }

    function resetCloudPrintSlot() {
        popupToolPanel.selectedFilePath = "";
        popupToolPanel.selectedFileIndex = -1;
        gcpClient.authorize();
    }

    function registerDropboxUserSlot() {
        cloudDriveModel.requestToken(CloudDriveModel.Dropbox);
    }

    function registerSkyDriveAccountSlot() {
        cloudDriveModel.authorize(CloudDriveModel.SkyDrive);
    }

    function showCloudDriveAccountsSlot() {
        pageStack.push(Qt.resolvedUrl("CloudDriveAccountsPage.qml"),
                       { cloudDriveModel: cloudDriveModel },
                       false);
    }

    function showCloudPrintJobsSlot() {
        pageStack.push(Qt.resolvedUrl("PrintJobsPage.qml"));
    }

    function showWebViewPageSlot() {
        pageStack.push(Qt.resolvedUrl("WebViewPage.qml"));
    }

    function showCloudDriveJobsSlot() {
        pageStack.push(Qt.resolvedUrl("CloudDriveJobsPage.qml"), { }, false);
    }

    function cancelAllCloudDriveJobsSlot() {
        cancelQueuedCloudDriveJobsConfirmation.open();
    }

    function cancelAllFolderSizeJobsSlot() {
//        cancelQueuedFolderSizeJobsConfirmation.open();

        fsModel.cancelQueuedJobs();
        // Abort thread with rollbackFlag=false.
        fsModel.abortThread(false);
    }

    function syncAllConnectedItemsSlot() {
        cloudDriveModel.syncItems();
    }

    function requestJobQueueStatusSlot() {
        cloudDriveModel.requestJobQueueStatus();
    }

    function showDropboxFullAccessConfirmationSlot() {
        dropboxFullAccessConfirmation.open();
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
        delegate: listDelegate
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

            // Invoke CloudDriveModel syncClipboard
            cloudDriveModel.syncClipboardItems();
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
        id: listDelegate

        ListItem {
            id: listItem
            width: fsListView.width
            height: 80

            property string fileName: name
            property string filePath: absolutePath

            function getIconSource() {
                var viewableImageFileTypes = ["JPG", "PNG", "SVG"];
                var viewableTextFileTypes = ["TXT", "HTML"];

                if (isDir) {
                    return "folder_list.svg";
                } else if (viewableImageFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                    var showThumbnail = appInfo.getSettingBoolValue("thumbnail.enabled", false);
//                    console.debug("folderPage listDelegate getIconSource showThumbnail " + showThumbnail);
                    if (showThumbnail) {
                        return "image://local/" + absolutePath;
                    } else {
                        return "photos_list.svg";
                    }
                } else {
                    return "notes_list.svg";
                }
            }

            Row {
                id: listDelegateRow
                anchors.fill: parent
                anchors.margins: 4

                Rectangle {
                    id: iconRect
                    width: 60
                    height: parent.height
                    color: "transparent"

                    Image {
                        id: cutCopyIcon
                        z: 1
                        width: 32
                        height: 32
                        visible: (source != "")
                        source: (clipboard.count > 0) ? appInfo.emptySetting+clipboard.getActionIcon(absolutePath) : ""
                    }
                    Image {
                        id: markIcon
                        z: 1
                        width: 32
                        height: 32
                        anchors.left: parent.left
                        anchors.bottom: parent.bottom
                        visible: (fsListView.state == "mark" && isChecked)
                        source: (theme.inverted) ? "ok.svg" : "ok_inverted.svg"
                    }
                    Image {
                        id: icon1
                        asynchronous: true
                        sourceSize.width: (fileType.toUpperCase() == "SVG") ? undefined : 48
                        sourceSize.height: (fileType.toUpperCase() == "SVG") ? undefined : 48
                        width: 48
                        height: 48
                        fillMode: Image.PreserveAspectFit
                        anchors.centerIn: parent
                        source: appInfo.emptySetting+listItem.getIconSource()
                    }
                }
                Column {
                    width: parent.width - iconRect.width
                    height: parent.height

                    Row {
                        width: parent.width
                        height: parent.height / 2
                        Text {
                            text: name
                            width: parent.width - sizeText.width
                            height: parent.height
                            font.pointSize: 18
                            elide: Text.ElideMiddle
                            verticalAlignment: Text.AlignVCenter
                            color: (theme.inverted) ? "white" : "black"
                        }
                        Text {
                            id: sizeText
                            text: Utility.formatFileSize(size, 1)
                            width: 85
                            height: parent.height
                            font.pointSize: 16
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                            color: (theme.inverted) ? "white" : "black"
                        }
                    }
                    Row {
                        width: parent.width
                        height: parent.height / 2
                        spacing: 2
                        Rectangle {
                            width: parent.width - syncIcon.width - parent.spacing
                            height: parent.height
                            color: "transparent"
                            Text {
                                id: listItemSubTitle
                                text: {
                                    var sub = ""
                                    if (subDirCount > 0) sub += appInfo.emptyStr+qsTr("%n dir(s)", "", subDirCount);
                                    if (subFileCount > 0) sub += ((sub == "") ? "" : " ") + appInfo.emptyStr+qsTr("%n file(s)", "", subFileCount);
                                    sub += ((sub == "") ? "" : ", ") + appInfo.emptyStr+qsTr("last modified") + " " + Qt.formatDateTime(lastModified, "d MMM yyyy h:mm:ss ap");

                                    return sub;
                                }
                                width: parent.width
                                height: parent.height
                                font.pointSize: 16
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                                visible: !isRunning
                                color: "grey"
                            }
                            Row {
                                width: parent.width
                                height: parent.height
                                visible: isRunning
                                anchors.verticalCenter: parent.verticalCenter
                                Image {
                                    id: runningIcon
                                    width: 24
                                    height: 24
                                    anchors.verticalCenter: parent.verticalCenter
                                    source: {
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
                                    visible: (runningOperation != FolderSizeItemListModel.NoOperation)
                                }
                                ProgressBar {
                                    id: syncProgressBar
                                    width: parent.width - runningIcon.width
                                    anchors.verticalCenter: parent.verticalCenter
                                    indeterminate: isRunning && (isDir || (runningValue == runningMaxValue))
                                    value: runningValue
                                    maximumValue: runningMaxValue
                                }
                            }
                        }
                        Image {
                            id: syncIcon
                            width: (visible) ? 30 : 0
                            height: parent.height
                            fillMode: Image.PreserveAspectFit
                            z: 1
                            visible: cloudDriveModel.isConnected(absolutePath) || cloudDriveModel.isSyncing(absolutePath);
                            source: {
                                if (cloudDriveModel.isSyncing(absolutePath)) {
                                    return "cloud_wait.svg";
                                } else if (cloudDriveModel.isDirty(absolutePath, lastModified)) {
                                    return "cloud_dirty.svg";
                                } else {
                                    return "cloud.svg";
                                }
                            }
                        }
                    }
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

    MessageDialog {
        id: messageDialog
    }

    ClipboardModel {
        id: clipboard

        function getActionIcon(localPath) {
            var jsonText = clipboard.getItemJsonText(localPath);
            if (jsonText != "") {
                var json = Utility.createJsonObj(jsonText);
                if (json) {
                    if (json.action == "copy") {
                        return (!window.platformInverted) ? "copy.svg" : "copy_inverted.svg";
                    } else if (json.action == "cut") {
                        return (!window.platformInverted) ? "trim.svg" : "trim_inverted.svg";
                    }
                }
            }
            return "";
        }

        function get(index) {
            var jsonText = clipboard.getItemJsonText(index);
            if (jsonText != "") {
                var json = Utility.createJsonObj(jsonText);
                return json;
            }

            return null;
        }

        function addItem(json) {
            console.debug(Utility.nowText() + "clipboard additem json " + json);
            clipboard.addClipboardItem(json.sourcePath, Utility.createJsonText(json) );
        }

        function addItemWithSuppressCountChanged(json) {
            console.debug(Utility.nowText() + "clipboard addItemWithSuppressCountChanged json " + json);
            clipboard.addClipboardItem(json.sourcePath, Utility.createJsonText(json), true);
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
        clipboardCount: clipboard.count
        timeout: appInfo.emptySetting+appInfo.getSettingValue("popup.timer.interval", 2) * 1000

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
            printFileSlot(srcFilePath, srcItemIndex);
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

    CommonDialog {
        id: newFolderDialog
        titleText: appInfo.emptyStr+qsTr("New folder / file")
        titleIcon: "FilesPlusIcon.svg"
        buttonTexts: [appInfo.emptyStr+qsTr("OK"), appInfo.emptyStr+qsTr("Cancel")]
        content: Column {
            width: parent.width - 10
            height: 120
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 5

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

        onStatusChanged: {
            if (status == DialogStatus.Open) {
                newFolderButton.checked = true;
                newFileButton.checked = false;
                folderName.text = "";
            }

            if (status == DialogStatus.Closed) {
                folderName.text = "";
            }
        }

        onButtonClicked: {
            folderName.closeSoftwareInputPanel();

            if (index === 0 && folderName.text !== "") {
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

    CommonDialog {
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

        onStatusChanged: {
            if (status == DialogStatus.Open) {
                newName.text = fsModel.getFileName(renameDialog.sourcePath);
                newName.forceActiveFocus();
            }

            if (status == DialogStatus.Closed) {
                newName.text = "";
            }
        }

        onButtonClicked: {
            newName.closeSoftwareInputPanel();

            if (index === 0) {
                if (newName.text != "" && newName.text != fsModel.getFileName(renameDialog.sourcePath)) {
                    var res = fsModel.renameFile(fsModel.getFileName(renameDialog.sourcePath), newName.text);
                    refreshSlot();
                }
            }

        }
    }

    CommonDialog {
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

        onStatusChanged: {
            if (status == DialogStatus.Open) {
                fileName.forceActiveFocus();
                fileName.text = fsModel.getNewFileName(sourcePath, targetPath);
            }

            if (status == DialogStatus.Closed) {
                fileName.text = "";
                overwriteFile.checked = false;
            }
        }

        onButtonClicked: {
            // If paste to current folder, targetPath is ended with / already.
            // If paste to selected folder, targetPath is not ended with /.
            if (index === 0) {
                var res = false;
                if (isCopy) {
                    res = fsModel.copy(sourcePath, fsModel.getAbsolutePath(targetPath, fileName.text) );
                } else {
                    res = fsModel.move(sourcePath, fsModel.getAbsolutePath(targetPath, fileName.text) );
                }
            } else {
                copyProgressDialog.close();
            }
        }
    }

    ConfirmDialog {
        id: cancelQueuedCloudDriveJobsConfirmation
        titleText: appInfo.emptyStr+qsTr("Cancel sync jobs")
        onOpening: {
            contentText = appInfo.emptyStr+qsTr("Cancel %n job(s) ?", "", cloudDriveModel.getQueuedJobCount());
        }
        onConfirm: {
            cloudDriveModel.cancelQueuedJobs();
            requestJobQueueStatusSlot();
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

    PrinterSelectionDialog {
        id: printerSelectionDialog
        onAccepted: {
            // Print on selected printer index.
            var pid = gcpClient.getPrinterId(printerSelectionDialog.selectedIndex);
            var printerType = gcpClient.getPrinterType(printerSelectionDialog.selectedIndex);
            console.debug("printerSelectionDialog onAccepted pid=" + pid + "printerType=" + printerType + " srcFilePath=" + srcFilePath + " srcURL=" + srcURL);
            if (pid != "") {
                // TODO Add options for PrintQualities and Colors.
                // Issue: Fix HP ePrint to print in color by setting capabilities.
                var capabilities = "";
                if (printerType.toUpperCase() == "HP") {
                    capabilities = "\
{\"capabilities\":[ \
{\"name\":\"ns1:PrintQualities\",\"type\":\"Feature\",\"options\":[{\"name\":\"Normal\"}]}, \
{\"name\":\"ns1:Colors\",\"type\":\"Feature\",\"options\":[{\"name\":\"Color\"}]} \
]}";
                }
                console.debug("printerSelectionDialog onAccepted capabilities=" + capabilities);

                if (srcFilePath != "") {
                    // Open uploadProgressBar for printing.
                    uploadProgressDialog.srcFilePath = srcFilePath;
                    uploadProgressDialog.titleText = appInfo.emptyStr+qsTr("Printing");
                    uploadProgressDialog.autoClose = false;
                    uploadProgressDialog.open();

                    gcpClient.submit(pid, srcFilePath, capabilities);
                } else if (srcURL != "") {
                    // Open uploadProgressBar for printing.
                    uploadProgressDialog.srcFilePath = srcURL;
                    uploadProgressDialog.titleText = appInfo.emptyStr+qsTr("Printing");
                    uploadProgressDialog.autoClose = false;
                    uploadProgressDialog.open();

                    gcpClient.submit(pid, srcURL, capabilities, srcURL, "url", "");
                }
            }
        }
        onRejected: {
//            // Reset popupToolPanel.
//            popupToolPanel.selectedFilePath = "";
//            popupToolPanel.selectedFileIndex = -1;
        }
    }

    UploadProgressDialog {
        id: uploadProgressDialog
    }

    DownloadProgressDialog {
        id: downloadProgressDialog
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

    GCPClient {
        id: gcpClient

        property string selectedFilePath
        property string selectedURL

        function getPrinterListModel() {
            var printerList = gcpClient.getStoredPrinterList();
            console.debug("gcpClient.getPrinterListModel() " + printerList);
            // Construct model.
            var model = Qt.createQmlObject(
                        'import QtQuick 1.1; ListModel {}', folderPage);

            for (var i=0; i<printerList.length; i++)
            {
                model.append({
                                 name: printerList[i]
                             });
            }

            console.debug("gcpClient.getPrinterListModel() model.count " + model.count);
            return model;
        }

        function resumePrinting() {
            if (gcpClient.selectedFilePath.trim() != "") {
                printFileSlot(gcpClient.selectedFilePath);
                return true;
            } else if (gcpClient.selectedURL.trim() != "") {
                printURLSlot(gcpClient.selectedURL);
                return true;
            }

            return false;
        }

        onAuthorizeRedirectSignal: {
            console.debug("folderPage gcpClient onAuthorizeRedirectSignal " + url);
            pageStack.push(Qt.resolvedUrl("AuthPage.qml"), { url: url, redirectFrom: "GCPClient" }, true);
        }

        onAccessTokenReplySignal: {
            console.debug("folderPage gcpClient onAccessTokenReplySignal " + err + " " + errMsg + " " + msg);

            if (err == 0) {
                // Resume printing
                resumePrinting();
            } else {
                gcpClient.refreshAccessToken();
            }
        }

        onRefreshAccessTokenReplySignal: {
            console.debug("folderPage gcpClient onRefreshAccessTokenReplySignal " + err + " " + errMsg + " " + msg);

            if (err == 0) {
                // Resume printing if selectedFilePath exists.
                if (resumePrinting()) {
                    // Do nothing if printing is resumed.
                } else {
                    messageDialog.titleText = appInfo.emptyStr+qsTr("Reset CloudPrint");
                    messageDialog.message = appInfo.emptyStr+qsTr("Resetting is done.");
                    messageDialog.open();
                }
            } else {
                // TODO Notify failed refreshAccessToken.
            }
        }

        onAccountInfoReplySignal: {
            console.debug("folderPage gcpClient onAccountInfoReplySignal " + err + " " + errMsg + " " + msg);

            var jsonObj = Utility.createJsonObj(msg);
            console.debug("jsonObj.email " + jsonObj.email);
        }

        onSearchReplySignal: {
            console.debug("folderPage gcpClient onSearchReplySignal " + err + " " + errMsg + " " + msg);

            if (err == 0) {
                // Once search done, open printerSelectionDialog
                // TODO any case that error=0 but no printers returned.
                resumePrinting();
            } else {
                gcpClient.refreshAccessToken();
            }
        }

        onSubmitReplySignal: {
//            console.debug("folderPage gcpClient onSubmitReplySignal " + err + " " + errMsg + " " + msg);

            // Notify submit result.
            var jsonObj = Utility.createJsonObj(msg);
            var message = "";
            if (err != 0) {
                message = qsTr("Error") + " " + err + " " + errMsg + "\n";
            }
            message += jsonObj.message;

            // Show reply message on progressDialog and also turn indeterminate off.
            if (uploadProgressDialog.status != DialogStatus.Open) {
                uploadProgressDialog.titleText = appInfo.emptyStr+qsTr("Printing");
                uploadProgressDialog.srcFilePath = popupToolPanel.selectedFilePath;
                uploadProgressDialog.autoClose = false;
                uploadProgressDialog.open();
            }
            uploadProgressDialog.indeterminate = false;
            uploadProgressDialog.message = appInfo.emptyStr+message;

            // Reset popupToolPanel selectedFile.
            gcpClient.selectedFilePath = "";
            gcpClient.selectedURL = "";
            popupToolPanel.selectedFilePath = "";
            popupToolPanel.selectedFileIndex = -1;
        }

        onDownloadProgress: {
//            console.debug("sandBox gcpClient onDownloadProgress " + bytesReceived + " / " + bytesTotal);

            // Shows in progress bar.
//            if (downloadProgressDialog.status != DialogStatus.Open) {
//                downloadProgressDialog.titleText = qsTr("Searching for printers"
//                downloadProgressDialog.indeterminate = false;
//                downloadProgressDialog.open();
//            }
            downloadProgressDialog.min = 0;
            downloadProgressDialog.max = bytesTotal;
            downloadProgressDialog.value = bytesReceived;
        }

        onUploadProgress: {
//            console.debug("sandBox gcpClient onUploadProgress " + bytesSent + " / " + bytesTotal);

            // Shows in progress bar.
//            if (uploadProgressDialog.status != DialogStatus.Open) {
//                uploadProgressDialog.indeterminate = false;
//                uploadProgressDialog.open();
//            }
            uploadProgressDialog.min = 0;
            uploadProgressDialog.max = bytesTotal;
            uploadProgressDialog.value = bytesSent;
        }

        Component.onCompleted: {
            console.debug(Utility.nowText() + " folderPage gcpClient onCompleted");
            window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("GCPClient"), 0.1);
        }
    }

    CloudDriveModel {
        id: cloudDriveModel
        dropboxFullAccess: appInfo.emptySetting+appInfo.getSettingBoolValue("dropbox.fullaccess.enabled", false)

        property string shareFileCaller: uidDialog.caller

        function syncClipboardItems() {
            uidDialog.localPath = "";
            uidDialog.operation = CloudDriveModel.Metadata;
            uidDialog.open();
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

        function getUidListModel(localPath) {
            console.debug("getUidListModel localPath " + localPath);

            // TODO Get uid list from GDClient.

            // Get uid list.
            var uidList = cloudDriveModel.getStoredUidList();

            // Construct model.
            var model = Qt.createQmlObject(
                        'import QtQuick 1.1; ListModel {}', folderPage);

            for (var i=0; i<uidList.length; i++)
            {
                var json = JSON.parse(uidList[i]);
                var localHash = cloudDriveModel.getItemHash(localPath, getClientType(json.type), json.uid);
                console.debug("getUidListModel i " + i + " type " + json.type + " uid " + json.uid + " email " + json.email + " localHash " + localHash);
                model.append({
                                 type: getClientType(json.type),
                                 uid: json.uid,
                                 email: json.email,
                                 hash: localHash,
                                 name: "",
                                 shared: 0,
                                 normal: 0,
                                 quota: 0
                             });
            }

            return model;
        }

        function getUidEmail(type, uid) {
            // Get uid list from DropboxClient.
            var dbUidList = cloudDriveModel.getStoredUidList(type);

            for (var i=0; i<dbUidList.length; i++)
            {
                var json = JSON.parse(dbUidList[i]);
                if (uid == json.uid) return json.email;
            }

            return "";
        }

        function getCloudIcon(type) {
            switch (type) {
            case CloudDriveModel.Dropbox:
                return "dropbox_icon.png";
            case CloudDriveModel.GoogleDrive:
                return "drive_icon.png";
            case CloudDriveModel.SkyDrive:
                return "skydrive_icon.png";
            case CloudDriveModel.Ftp:
                return "ftp_icon.png";
            }
        }

        function getCloudName(type) {
            switch (type) {
            case CloudDriveModel.Dropbox:
                return "Dropbox";
            case CloudDriveModel.GoogleDrive:
                return "GoogleDrive";
            case CloudDriveModel.SkyDrive:
                return "SkyDrive";
            case CloudDriveModel.Ftp:
                return "FTP";
            }
        }

        function getClientType(typeText) {
            if (typeText.toLowerCase() == "dropboxclient") {
                return CloudDriveModel.Dropbox;
            } else if (typeText.toLowerCase() == "skydriveclient") {
                return CloudDriveModel.SkyDrive;
            } else if (typeText.toLowerCase() == "ftpclient") {
                return CloudDriveModel.Ftp;
            }

            return -1;
        }

        function getRemoteName(remotePath) {
            var name = "";
            if (remotePath != "") {
                name = remotePath.substr(remotePath.lastIndexOf("/") + 1);
            }

            return name;
        }

        onRequestTokenReplySignal: {
            console.debug("folderPage cloudDriveModel onRequestTokenReplySignal " + err + " " + errMsg + " " + msg);

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                // TODO how to check if app has been authorized by user.
                cloudDriveModel.authorize(CloudDriveModel.Dropbox);
            } else {
                messageDialog.titleText = appInfo.emptyStr+qsTr("CloudDrive Request Token");
                messageDialog.message = appInfo.emptyStr+qsTr("Error") + " " + err + " " + errMsg + " " + msg;
                messageDialog.open();
            }
        }

        onAuthorizeRedirectSignal: {
            console.debug("folderPage cloudDriveModel onAuthorizeRedirectSignal " + url + " redirectFrom " + redirectFrom);

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            pageStack.push(Qt.resolvedUrl("AuthPage.qml"), { url: url, redirectFrom: redirectFrom }, true);
        }

        onAccessTokenReplySignal: {
            console.debug("folderPage cloudDriveModel onAccessTokenReplySignal " + err + " " + errMsg + " " + msg);

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                // TODO find better way to proceed after got accessToken.
                if (popupToolPanel.selectedFilePath) {
                    uidDialog.proceedPendingOperation();
//                    syncFileSlot(popupToolPanel.selectedFilePath, popupToolPanel.selectedFileIndex);
                } else {
                    // TODO Get account info and show in dialog.
                    messageDialog.titleText = appInfo.emptyStr+qsTr("CloudDrive Access Token");
                    messageDialog.message = appInfo.emptyStr+qsTr("CloudDrive user is authorized.\nPlease proceed your sync action.");
                    messageDialog.open();

                    // Refresh account page.
                    var p = pageStack.find(function (page) { return page.name == "cloudDriveAccountsPage"; });
                    if (p) p.refreshCloudDriveAccountsSlot();
                }
            } else {
                messageDialog.titleText = appInfo.emptyStr+qsTr("CloudDrive Access Token");
                messageDialog.message = appInfo.emptyStr+qsTr("Error") + " " + err + " " + errMsg + " " + msg;
                messageDialog.open();
            }
        }

        onAccountInfoReplySignal: {
            console.debug("folderPage cloudDriveModel onAccountInfoReplySignal " + err + " " + errMsg + " " + msg);

            var jsonText = cloudDriveModel.getJobJson(nonce);
            var cloudDriveJobJson = JSON.parse(jsonText);

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                var jsonObj = Utility.createJsonObj(msg);

                // Send info to cloudDriveAccountsPage.
                var p;
                p = pageStack.find(function (page) { return (page.name == "cloudDriveAccountsPage"); });
                if (p) {
                    switch (cloudDriveJobJson.type) {
                    case CloudDriveModel.Dropbox:
                        p.updateAccountInfoSlot(cloudDriveJobJson.type, jsonObj.uid, jsonObj.name, jsonObj.email, jsonObj.quota_info.shared, jsonObj.quota_info.normal, jsonObj.quota_info.quota);
                        break;
                    case CloudDriveModel.SkyDrive:
                        p.updateAccountInfoSlot(cloudDriveJobJson.type, jsonObj.uid, jsonObj.name, jsonObj.email, 0, 0, 0);
                        break;
                    }

                }

                // Send info to drivePage.
                p = pageStack.find(function (page) { return (page.name == "drivePage"); });
                if (p) {
                    switch (cloudDriveJobJson.type) {
                    case CloudDriveModel.Dropbox:
                        p.updateAccountInfoSlot(cloudDriveJobJson.type, jsonObj.uid, jsonObj.name, jsonObj.email, jsonObj.quota_info.shared, jsonObj.quota_info.normal, jsonObj.quota_info.quota);
                        break;
                    case CloudDriveModel.SkyDrive:
                        p.updateAccountInfoSlot(cloudDriveJobJson.type, jsonObj.uid, jsonObj.name, jsonObj.email, 0, 0, 0);
                        break;
                    }
                }
            } else if (err == 204) {
                // TODO Refactor to support all SkyDriveClient services.
                cloudDriveModel.refreshToken(cloudDriveJobJson.type, cloudDriveJobJson.uid);
            } else {
                messageDialog.titleText = appInfo.emptyStr+qsTr("CloudDrive Account Info");
                messageDialog.message = appInfo.emptyStr+qsTr("Error") + " " + err + " " + errMsg + " " + msg;
                messageDialog.open();
            }
        }

        onQuotaReplySignal: {
            console.debug("folderPage cloudDriveModel onQuotaReplySignal " + err + " " + errMsg + " " + msg);

            var jsonText = cloudDriveModel.getJobJson(nonce);
            var cloudDriveJobJson = JSON.parse(jsonText);

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                var jsonObj = Utility.createJsonObj(msg);

                // Send info to cloudDriveAccountsPage.
                var p;
                p = pageStack.find(function (page) { return (page.name == "cloudDriveAccountsPage"); });
                if (p) {
                    switch (cloudDriveJobJson.type) {
                    case CloudDriveModel.SkyDrive:
                        p.updateAccountInfoSlot(cloudDriveJobJson.type, cloudDriveJobJson.uid, "", cloudDriveModel.getUidEmail(cloudDriveJobJson.type, cloudDriveJobJson.uid), 0, jsonObj.quota-jsonObj.available, jsonObj.quota);
                        break;
                    }

                }

                // Send info to drivePage.
                p = pageStack.find(function (page) { return (page.name == "drivePage"); });
                if (p) {
                    switch (cloudDriveJobJson.type) {
                    case CloudDriveModel.SkyDrive:
                        p.updateAccountInfoSlot(cloudDriveJobJson.type, cloudDriveJobJson.uid, "", cloudDriveModel.getUidEmail(cloudDriveJobJson.type, cloudDriveJobJson.uid), 0, jsonObj.quota-jsonObj.available, jsonObj.quota);
                        break;
                    }
                }
            } else if (err == 204) {
                // TODO Refactor to support all SkyDriveClient services.
                cloudDriveModel.refreshToken(cloudDriveJobJson.type, cloudDriveJobJson.uid);
            } else {
                messageDialog.titleText = appInfo.emptyStr+qsTr("CloudDrive Quota");
                messageDialog.message = appInfo.emptyStr+qsTr("Error") + " " + err + " " + errMsg + " " + msg;
                messageDialog.open();
            }
        }

        onFileGetReplySignal: {
            console.debug("folderPage cloudDriveModel onFileGetReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jsonText = cloudDriveModel.getJobJson(nonce);
//            console.debug("folderPage cloudDriveModel jsonText " + jsonText);

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            var json = JSON.parse(jsonText);
            var modelIndex = json.model_index;
            var isRunning = json.is_running;

            // Update ProgressBar on listItem.
            // TODO also show running on its parents.
            fsModel.setProperty(json.local_file_path, FolderSizeItemListModel.IsRunningRole, isRunning);

            // Show indicator on parent up to root.
            // Skip i=0 as it's notified above already.
            var pathList = fsModel.getPathToRoot(json.local_file_path);
            for(var i=1; i<pathList.length; i++) {
                modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
                if (modelIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
                    fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, isRunning);
                }
            }

            if (err == 0) {
                // Remove cache on target folders and its parents.
                fsModel.removeCache(json.local_file_path);

                // TODO Does it need to refresh to add gotten file to listview ?
                var index = fsModel.getIndexOnCurrentDir(json.local_file_path);
                if (index == FolderSizeItemListModel.IndexNotOnCurrentDir) {
                    // do nothing.
                } else if (index == FolderSizeItemListModel.IndexOnCurrentDirButNotFound) {
                    refreshSlot();
                } else {
                    fsModel.refreshItem(index);
                }
            } else {
                messageDialog.titleText = getCloudName(json.type) + " " + appInfo.emptyStr+qsTr("File Get");
                messageDialog.message = appInfo.emptyStr+qsTr("Error") + " " + err + " " + errMsg + " " + msg;
                messageDialog.open();
            }
        }

        onFilePutReplySignal: {
            console.debug("folderPage cloudDriveModel onFilePutReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jsonText = cloudDriveModel.getJobJson(nonce);
//            console.debug("folderPage cloudDriveModel onFilePutReplySignal jsonText " + jsonText);

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            var json = JSON.parse(jsonText);
            var modelIndex = json.model_index;
            var isRunning = json.is_running;

            // Update ProgressBar on listItem.
            // TODO also show running on its parents.
            fsModel.setProperty(json.local_file_path, FolderSizeItemListModel.IsRunningRole, isRunning);

            // Show indicator on parent up to root.
            // Skip i=0 as it's notified above already.
            var pathList = fsModel.getPathToRoot(json.local_file_path);
            for(var i=1; i<pathList.length; i++) {
                modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
                if (modelIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
                    fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, isRunning);
                }
            }

            if (err == 0) {
                // Do nothing.
            } else {
                messageDialog.titleText = getCloudName(json.type) + " " + appInfo.emptyStr+qsTr("File Put");
                messageDialog.message = appInfo.emptyStr+qsTr("Error") + " " + err + " " + errMsg + " " + msg;
                messageDialog.open();
            }
        }

        onMetadataReplySignal: {
            // Get job json.
            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));
            console.debug("folderPage cloudDriveModel onMetadataReplySignal " + getCloudName(jobJson.type) + " " + nonce + " " + err + " " + errMsg + " " + msg);

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                // Found metadata.
                var jsonObj = Utility.createJsonObj(msg);
                var itemJson = Utility.createJsonObj(cloudDriveModel.getItemJson(jobJson.local_file_path, jobJson.type, jobJson.uid));
                var localPathHash = cloudDriveModel.getItemHash(jobJson.local_file_path, jobJson.type, jobJson.uid);
//                console.debug("folderPage cloudDriveModel onMetadataReplySignal jobJson " + jobJson.local_file_path + " " + jobJson.type + " " + jobJson.uid + " " + localPathHash);

                if (jobJson.type == CloudDriveModel.Dropbox) {
                    // If remotePath was deleted, remove link from localPath.
                    if (jsonObj.is_deleted) {
                        // If dir, should remove all sub items.
                        cloudDriveModel.removeItemWithChildren(jobJson.type, jobJson.uid, jobJson.local_file_path);

                        // Update ProgressBar on listItem.
                        fsModel.setProperty(jobJson.local_file_path, FolderSizeItemListModel.IsRunningRole, jobJson.is_running);

                        // Notify removed link.
                        messageDialog.titleText = getCloudName(jobJson.type) + " Metadata";
                        messageDialog.message = appInfo.emptyStr+qsTr("%1 was removed remotely.\nLink will be removed.").arg(jobJson.local_file_path);
                        messageDialog.autoClose = true;
                        messageDialog.open();
                    } else {
                        // Sync starts from itself.
                        if (jsonObj.is_dir) { // Sync folder.
                            // If there is no local folder, create it and connect.
                            if (!fsModel.isDir(jobJson.local_file_path)) {
                                // TODO Add item to ListView.
                                fsModel.createDirPath(jobJson.local_file_path);
                            }

                            // Sync based on remote contents.
    //                        console.debug("cloudDriveModel onMetadataReplySignal folder jsonObj.rev " + jsonObj.rev + " jsonObj.hash " + jsonObj.hash + " localPathHash " + localPathHash);
                            if (jsonObj.hash != localPathHash) { // Sync all json(remote)'s contents.
                                for(var i=0; i<jsonObj.contents.length; i++) {
                                    var item = jsonObj.contents[i];
                                    var itemLocalPath = fsModel.getAbsolutePath(jobJson.local_file_path, cloudDriveModel.getRemoteName(item.path));
                                    var itemLocalHash = cloudDriveModel.getItemHash(itemLocalPath, jobJson.type, jobJson.uid);
                                    if (item.is_dir) {
                                        // This flow will trigger recursive metadata calling.
        //                                console.debug("dir item.path = " + item.path + " itemLocalHash " + itemLocalHash + " item.rev " + item.rev);
                                        cloudDriveModel.metadata(jobJson.type, jobJson.uid, itemLocalPath, item.path, -1);
                                    } else {
        //                                console.debug("file item.path = " + item.path + " itemLocalHash " + itemLocalHash + " item.rev " + item.rev);
                                        if (itemLocalHash != item.rev) {
                                            cloudDriveModel.metadata(jobJson.type, jobJson.uid, itemLocalPath, item.path, -1);
                                        }
                                    }
                                }

    //                            // Add cloudDriveItem for currentDir.
    //                            cloudDriveModel.addItem(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, jsonObj.hash);
                            }

                            // Add or Update timestamp from local to cloudDriveItem.
                            cloudDriveModel.addItem(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, jsonObj.hash);

                            // Sync based on local contents.
                            cloudDriveModel.syncFromLocal(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, jobJson.modelIndex);
                        } else { // Sync file.
    //                        console.debug("cloudDriveModel onMetadataReplySignal file jsonObj.rev " + jsonObj.rev + " localPathHash " + localPathHash);

                            // If (rev is newer or there is no local file), get from remote.
                            if (jsonObj.rev > localPathHash || !fsModel.isFile(localPath)) {
                                // TODO Add item to ListView.
                                cloudDriveModel.fileGet(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, jobJson.modelIndex);
                            } else if (jsonObj.rev < localPathHash) {
                                cloudDriveModel.filePut(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, jobJson.modelIndex);
                            } else {
                                // Update lastModified on cloudDriveItem.
                                cloudDriveModel.addItem(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, jsonObj.rev);
                            }
                        }
                    }
                }

                // TODO Supports SkyDrive.
                if (jobJson.type == CloudDriveModel.SkyDrive) {
                    console.debug("folderPage cloudDriveModel onMetadataReplySignal SkyDrive jobJson " + jobJson.type + " " + jobJson.uid + " " + jobJson.local_file_path + " " + jobJson.remote_file_path + " " + localPathHash);

                    if (jsonObj.property) {
                        // Suspend next job.
                        cloudDriveModel.suspendNextJob();

                        // Generate hash from updated_time.
                        var remotePathHash = jsonObj.property.updated_time;
    //                    console.debug("folderPage cloudDriveModel onMetadataReplySignal remotePathHash " + remotePathHash);

                        // Sync starts from itself.
                        if (jsonObj.property.type == "folder" || jsonObj.property.type == "album") { // Sync folder.
                            // If there is no local folder, create it and connect.
                            if (!fsModel.isDir(jobJson.local_file_path)) {
                                // TODO Add item to ListView.
                                fsModel.createDirPath(jobJson.local_file_path);
                            }

                            // Sync based on remote contents.
    //                        console.debug("cloudDriveModel onMetadataReplySignal folder jsonObj.rev " + jsonObj.rev + " jsonObj.hash " + jsonObj.hash + " localPathHash " + localPathHash);
                            if (remotePathHash != localPathHash) { // Sync all json(remote)'s contents.
                                for(var i=0; i<jsonObj.data.length; i++) {
                                    var item = jsonObj.data[i];
                                    var itemLocalPath = fsModel.getAbsolutePath(jobJson.local_file_path, item.name);
                                    var itemLocalHash = cloudDriveModel.getItemHash(itemLocalPath, jobJson.type, jobJson.uid);
                                    var itemRemotePath = item.id;
                                    var itemRemoteHash = item.updated_time;
                                    if (item.type == "folder" || item.type == "album") {
                                        // This flow will trigger recursive metadata calling.
                                        console.debug("cloudDriveModel onMetadataReplySignal dir itemRemotePath " + itemRemotePath + " itemLocalHash " + itemLocalHash + " itemRemoteHash " + itemRemoteHash + " " + item.updated_time);
                                        cloudDriveModel.metadata(jobJson.type, jobJson.uid, itemLocalPath, itemRemotePath, -1);
                                    } else {
                                        console.debug("cloudDriveModel onMetadataReplySignal file itemRemotePath " + itemRemotePath + " itemLocalHash " + itemLocalHash + " itemRemoteHash " + itemRemoteHash + " " + item.updated_time);
                                        if (itemRemoteHash > itemLocalHash) {
                                            cloudDriveModel.fileGet(jobJson.type, jobJson.uid, itemRemotePath, itemLocalPath, -1);
                                        } else if (itemRemoteHash < itemLocalHash) {
                                            // Put file to remote parent path. Or root if parent_id is null.
                                            itemRemotePath = (item.parent_id) ? item.parent_id : "me/skydrive";
                                            cloudDriveModel.filePut(jobJson.type, jobJson.uid, itemLocalPath, itemRemotePath, -1);
                                        } else {
                                            cloudDriveModel.addItem(jobJson.type, jobJson.uid, itemLocalPath, itemRemotePath, itemRemoteHash);
                                        }
                                    }
                                }
                            }

                            // Add or Update timestamp from local to cloudDriveItem.
                            cloudDriveModel.addItem(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, remotePathHash);

                            // Sync based on local contents.
                            cloudDriveModel.syncFromLocal_SkyDrive(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, jobJson.modelIndex);
                        } else { // Sync file.
                            console.debug("folderPage cloudDriveModel onMetadataReplySignal file jobJson " + jobJson.local_file_path + " " + jobJson.remote_file_path + " " + jobJson.type + " " + jobJson.uid + " remotePathHash " + remotePathHash + " localPathHash " + localPathHash);

                            // If (rev is newer or there is no local file), get from remote.
                            if (remotePathHash > localPathHash || !fsModel.isFile(jobJson.local_file_path)) {
                                // TODO Add item to ListView.
                                cloudDriveModel.fileGet(jobJson.type, jobJson.uid, jobJson.remote_file_path, jobJson.local_file_path, jobJson.modelIndex);
                            } else if (remotePathHash < localPathHash) {
                                // Put file to remote parent path. Or root if parent_id is null.
                                var remoteParentPath = (jsonObj.property.parent_id) ? jsonObj.property.parent_id : "me/skydrive";
                                cloudDriveModel.filePut(jobJson.type, jobJson.uid, jobJson.local_file_path, remoteParentPath, jobJson.modelIndex);
                            } else {
                                // Update lastModified on cloudDriveItem.
                                cloudDriveModel.addItem(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, remotePathHash);
                            }
                        }

                        // Resume next jobs.
                        cloudDriveModel.resumeNextJob();
                    } else {
                        console.debug("folderPage cloudDriveModel onMetadataReplySignal property is not found.");
                    }
                }

                // TODO Supports FTP.
                if (jobJson.type == CloudDriveModel.Ftp) {
                    console.debug("folderPage cloudDriveModel onMetadataReplySignal Ftp jobJson " + jobJson.type + " " + jobJson.uid + " " + jobJson.local_file_path + " " + jobJson.remote_file_path + " " + localPathHash);

                    if (jsonObj.property) {
                        // Suspend next job.
                        cloudDriveModel.suspendNextJob();

                        // Generate hash from updated_time.
                        var remotePathHash = jsonObj.property.lastModified;
    //                    console.debug("folderPage cloudDriveModel onMetadataReplySignal remotePathHash " + remotePathHash);

                        // Sync starts from itself.
                        if (jsonObj.property.isDir) { // Sync folder.
                            // If there is no local folder, create it and connect.
                            if (!fsModel.isDir(jobJson.local_file_path)) {
                                // TODO Add item to ListView.
                                fsModel.createDirPath(jobJson.local_file_path);
                            }

                            // Sync based on remote contents.
                            console.debug("cloudDriveModel onMetadataReplySignal folder remotePathHash " + remotePathHash + " localPathHash " + localPathHash);
                            if (remotePathHash != localPathHash) { // Sync all json(remote)'s contents.
                                for(var i=0; i<jsonObj.data.length; i++) {
                                    var item = jsonObj.data[i];
                                    var itemLocalPath = fsModel.getAbsolutePath(jobJson.local_file_path, item.name);
                                    var itemLocalHash = cloudDriveModel.getItemHash(itemLocalPath, jobJson.type, jobJson.uid);
                                    var itemRemotePath = item.path;
                                    var itemRemoteHash = item.lastModified;
                                    if (item.isDir) {
                                        // This flow will trigger recursive metadata calling.
                                        console.debug("cloudDriveModel onMetadataReplySignal dir itemRemotePath " + itemRemotePath + " itemLocalHash " + itemLocalHash + " itemRemoteHash " + itemRemoteHash + " " + item.lastModified);
                                        cloudDriveModel.metadata(jobJson.type, jobJson.uid, itemLocalPath, itemRemotePath, -1);
                                    } else {
                                        console.debug("cloudDriveModel onMetadataReplySignal file itemRemotePath " + itemRemotePath + " itemLocalHash " + itemLocalHash + " itemRemoteHash " + itemRemoteHash + " " + item.lastModified);
                                        if (itemRemoteHash > itemLocalHash) {
                                            cloudDriveModel.fileGet(jobJson.type, jobJson.uid, itemRemotePath, itemLocalPath, -1);
                                        } else if (itemRemoteHash < itemLocalHash) {
                                            // Put file to remote parent path. Or root if parent_id is null.
                                            itemRemotePath = (item.parent_id) ? item.parent_id : "me/skydrive";
                                            cloudDriveModel.filePut(jobJson.type, jobJson.uid, itemLocalPath, itemRemotePath, -1);
                                        } else {
                                            cloudDriveModel.addItem(jobJson.type, jobJson.uid, itemLocalPath, itemRemotePath, itemRemoteHash);
                                        }
                                    }
                                }
                            }

                            // Add or Update timestamp from local to cloudDriveItem.
                            cloudDriveModel.addItem(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, remotePathHash);

                            // Sync based on local contents.
                            cloudDriveModel.syncFromLocal(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, jobJson.modelIndex);
                        } else { // Sync file.
                            console.debug("folderPage cloudDriveModel onMetadataReplySignal file jobJson " + jobJson.local_file_path + " " + jobJson.remote_file_path + " " + jobJson.type + " " + jobJson.uid + " remotePathHash " + remotePathHash + " localPathHash " + localPathHash);

                            // If (rev is newer or there is no local file), get from remote.
                            if (remotePathHash > localPathHash || !fsModel.isFile(jobJson.local_file_path)) {
                                // TODO Add item to ListView.
                                cloudDriveModel.fileGet(jobJson.type, jobJson.uid, jobJson.remote_file_path, jobJson.local_file_path, jobJson.modelIndex);
                            } else if (remotePathHash < localPathHash) {
                                cloudDriveModel.filePut(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, jobJson.modelIndex);
                            } else {
                                // Update lastModified on cloudDriveItem.
                                cloudDriveModel.addItem(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, remotePathHash);
                            }
                        }

                        // Resume next jobs.
                        cloudDriveModel.resumeNextJob();
                    } else {
                        console.debug("folderPage cloudDriveModel onMetadataReplySignal property is not found.");
                    }
                }

            } else if (err == 203) { // If metadata is not found, put it to cloud right away recursively.
                console.debug("folderPage cloudDriveModel onMetadataReplySignal " + err + " " + errMsg + " " + msg);

                if (jobJson.type == CloudDriveModel.Dropbox) {
                    if (fsModel.isDir(jobJson.local_file_path)) {
                        // Remote folder will be created in syncFromLocal if it's required.
                        cloudDriveModel.syncFromLocal(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, jobJson.modelIndex);

                        // Request metadata for current dir.
                        // Once it got reply, it should get hash already.
                        // Because its sub files/dirs are in prior queue.
                        // cloudDriveModel.metadata(type, uid, localPath, remotePath, modelIndex);
                    } else {
                        cloudDriveModel.filePut(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, jobJson.modelIndex);
                    }
                }

                // TODO Supports SkyDrive.
                if (jobJson.type == CloudDriveModel.SkyDrive) {

                }

                // TODO Supports Ftp.
                if (jobJson.type == CloudDriveModel.Ftp) {
                    if (fsModel.isDir(jobJson.local_file_path)) {
                        // Remote folder will be created in syncFromLocal if it's required.
                        cloudDriveModel.syncFromLocal(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, jobJson.modelIndex);
                    } else {
                        cloudDriveModel.filePut(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, jobJson.modelIndex);
                    }
                }
            } else if (err == 204) { // Refresh token
                // TODO Refactor to support all SkyDriveClient services.
                if (jobJson.type == CloudDriveModel.SkyDrive) {
                    cloudDriveModel.refreshToken(jobJson.type, jobJson.uid);
                }
            } else {
                messageDialog.titleText = getCloudName(jobJson.type) + " Metadata";
                messageDialog.message = appInfo.emptyStr+qsTr("Error") + " " + err + " " + errMsg + " " + msg;
                messageDialog.open();

                // Reset running.
                jobJson.is_running = false;
            }

            // Show indicator on path up to root.
            var pathList = fsModel.getPathToRoot(jobJson.local_file_path);
            for(var i=0; i<pathList.length; i++) {
                var modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
                if (modelIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
                    fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, jobJson.is_running);
                }
            }

            // Workaround: Refresh item once got reply. To fix unexpected showing cloud_wait icon.
            fsModel.refreshItem(jobJson.local_file_path);
        }

        onCreateFolderReplySignal: {
            console.debug("folderPage cloudDriveModel onCreateFolderReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jsonText = cloudDriveModel.getJobJson(nonce);
//            console.debug("folderPage cloudDriveModel jsonText " + jsonText);

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);
            var json = JSON.parse(jsonText);
            var modelIndex = json.model_index;
            var isRunning = json.is_running;

            if (err == 0) {
                // Do nothing.
            } else if (err == 202) { // Folder already exists.
                // Do nothing.
            } else {
                messageDialog.titleText = getCloudName(json.type) + " " + appInfo.emptyStr+qsTr("Create Folder");
                messageDialog.message = appInfo.emptyStr+qsTr("Error") + " " + err + " " + errMsg + " " + msg;
                messageDialog.open();
            }

            // TODO *** app freeze here *** TODO Does it need?
            // Update ProgressBar if localPath is specified.
            /*
            if (json.local_file_path != "") {
                fsModel.setProperty(json.local_file_path, FolderSizeItemListModel.IsRunningRole, isRunning);

                // Show indicator on parent up to root.
                // Skip i=0 as it's notified above already.
                var pathList = fsModel.getPathToRoot(json.local_file_path);
                for(var i=1; i<pathList.length; i++) {
                    modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
                    if (modelIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
                        fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, isRunning);
                    }
                }
            }
            */

            // Refresh cloudDrivePathDialog if it's opened.
            console.debug("folderPage cloudDriveModel onCreateFolderReplySignal cloudDrivePathDialog.status " + cloudDrivePathDialog.status + " DialogStatus.Open " + DialogStatus.Open);
            if (cloudDrivePathDialog.status == DialogStatus.Open) {
                cloudDrivePathDialog.selectedRemotePath = json.new_remote_file_path;
                cloudDrivePathDialog.refresh();
            }
        }

        onDownloadProgress: {
//            console.debug("folderPage cloudDriveModel onDownloadProgress " + nonce + " " + bytesReceived + " / " + bytesTotal);

            var jsonText = cloudDriveModel.getJobJson(nonce);
//            console.debug("folderPage cloudDriveModel jsonText " + jsonText);

            var json = JSON.parse(jsonText);
            var isRunning = json.is_running

            // Update ProgressBar on listItem.
            var modelIndex = fsModel.getIndexOnCurrentDir(json.local_file_path);
            if (modelIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
                fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, isRunning);
                fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningOperationRole, mapToFolderSizeListModelOperation(json.operation) );
                fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningValueRole, bytesReceived);
                fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningMaxValueRole, bytesTotal);
            }

            // Show indicator on parent up to root.
            // Skip i=0 as it's notified above already.
            var pathList = fsModel.getPathToRoot(json.local_file_path);
            for(var i=1; i<pathList.length; i++) {
                modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
                if (modelIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
                    fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, isRunning);
                    fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningOperationRole, FolderSizeItemListModel.SyncOperation);
                }
            }
        }

        onUploadProgress: {
//            console.debug("folderPage cloudDriveModel onUploadProgress " + nonce + " " + bytesSent + " / " + bytesTotal);

            var jsonText = cloudDriveModel.getJobJson(nonce);
//            console.debug("folderPage cloudDriveModel jsonText " + jsonText);

            var json = JSON.parse(jsonText);
            var isRunning = json.is_running

            // Update ProgressBar on listItem.
            var modelIndex = fsModel.getIndexOnCurrentDir(json.local_file_path);
            if (modelIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
                fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, isRunning);
                fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningOperationRole, mapToFolderSizeListModelOperation(json.operation) );
                fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningValueRole, bytesSent);
                fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningMaxValueRole, bytesTotal);
            }

            // Show indicator on parent up to root.
            // Skip i=0 as it's notified above already.
            var pathList = fsModel.getPathToRoot(json.local_file_path);
            for(var i=1; i<pathList.length; i++) {
                modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
                if (modelIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
                    fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, isRunning);
                    fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningOperationRole, FolderSizeItemListModel.SyncOperation);
                }
            }
        }

        onLocalChangedSignal: {
            // TODO Disable becuase it can damage stored hash.
            // Reset CloudDriveItem hash upto root.
//            var paths = fsModel.getPathToRoot(localPath);
//            for (var i=0; i<paths.length; i++) {
//                console.debug("folderPage cloudDriveModel onLocalChangedSignal updateItems paths[" + i + "] " + paths[i]);
//                cloudDriveModel.updateItems(paths[i], cloudDriveModel.dirtyHash);
//            }
        }

        onJobQueueStatusSignal: {
            // Send info to cloudDriveAccountsPage.
            if (pageStack) {
                var p = pageStack.find(function (page) { return (page.name == "settingPage"); });
                if (p) {
                    p.updateJobQueueCount(runningJobCount, jobQueueCount);
                    p.updateCloudDriveItemCount(itemCount);
                }
            }

            // Update (runningJobCount + jobQueueCount) on cloudButton.
            cloudButtonIndicator.text = ((runningJobCount + jobQueueCount) > 0) ? (runningJobCount + jobQueueCount) : "";
        }

        onRefreshRequestSignal: {
            refreshSlot("cloudDriveModel onRefreshRequestSignal");
        }

        onMoveFileReplySignal: {
            console.debug("folderPage cloudDriveModel onMoveFileReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var json = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            console.debug("folderPage cloudDriveModel onMoveFileReplySignal json " + cloudDriveModel.getJobJson(nonce));

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                // Cloud items have been managed by CloudDriveModel::moveFileReplyFilter.
                // Sync after move.
                cloudDriveModel.metadata(json.type, json.uid, json.new_local_file_path, json.new_remote_file_path, json.model_index);
            } else {
                messageDialog.titleText = getCloudName(json.type) + " " + appInfo.emptyStr+qsTr("Move");
                messageDialog.message = appInfo.emptyStr+qsTr("Error") + " " + err + " " + errMsg + " " + msg;
                messageDialog.open();
            }

            // Update ProgressBar on listItem.
            // TODO also show running on its parents.
            fsModel.setProperty(json.new_local_file_path, FolderSizeItemListModel.IsRunningRole, false);
            fsModel.refreshItem(json.new_local_file_path);
        }

        onDeleteFileReplySignal: {
            console.debug("folderPage cloudDriveModel onDeleteFileReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var json = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));
            var modelIndex = json.model_index;
            var isRunning = json.is_running;

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                // do nothing.
            } else {
                messageDialog.titleText = getCloudName(json.type) + " " + appInfo.emptyStr+qsTr("Delete");
                messageDialog.message = appInfo.emptyStr+qsTr("Error") + " " + err + " " + errMsg + " " + msg;
                messageDialog.open();
            }

            // Update ProgressBar if localPath is specified.
            if (json.local_file_path != "") {
                fsModel.setProperty(json.local_file_path, FolderSizeItemListModel.IsRunningRole, isRunning);

                // Show indicator on parent up to root.
                // Skip i=0 as it's notified above already.
                var pathList = fsModel.getPathToRoot(json.local_file_path);
                for(var i=1; i<pathList.length; i++) {
                    modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
                    if (modelIndex < FolderSizeItemListModel.IndexNotOnCurrentDir) {
                        fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, isRunning);
                    }
                }
            }

            // Refresh cloudDrivePathDialog if it's opened.
            console.debug("folderPage cloudDriveModel onDeleteFileReplySignal cloudDrivePathDialog.status " + cloudDrivePathDialog.status + " DialogStatus.Open " + DialogStatus.Open);
            if (cloudDrivePathDialog.status == DialogStatus.Open) {
                cloudDrivePathDialog.refresh();
            }
        }

        onShareFileReplySignal: {
            console.debug("folderPage cloudDriveModel onShareFileReplySignal " + nonce + " " + err + " " + errMsg + " " + msg + " " + url + " " + expires);

            var json = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            var isRunning = json.is_running;

            if (err == 0) {
                // TODO Select delivery service. (email, sms)
                // Open recipientSelectionDialog for sending share link.
                var senderEmail = getUidEmail(json.type, json.uid);
                if (senderEmail != "") {
                    recipientSelectionDialog.srcFilePath = json.local_file_path;
                    recipientSelectionDialog.messageSubject = appInfo.emptyStr+qsTr("Share file on %1").arg(cloudDriveModel.getCloudName(json.type));
                    recipientSelectionDialog.messageBody = appInfo.emptyStr+qsTr("Please download file with below link.") + "\n" + url;
                    recipientSelectionDialog.senderEmail = senderEmail;
                    recipientSelectionDialog.open();
                }
            } else {
                messageDialog.titleText = getCloudName(json.type) + " " + appInfo.emptyStr+qsTr("Share");
                messageDialog.message = appInfo.emptyStr+qsTr("Error") + " " + err + " " + errMsg + " " + msg;
                messageDialog.open();
            }

            // Update ProgressBar on listItem.
            // TODO also show running on its parents.
            fsModel.setProperty(json.local_file_path, FolderSizeItemListModel.IsRunningRole, isRunning);
        }

        onJobEnqueuedSignal: {
            console.debug("folderPage cloudDriveModel onJobEnqueuedSignal " + nonce + " " + localPath);
            fsModel.refreshItem(localPath);
        }

        onBrowseReplySignal: {
            console.debug("folderPage cloudDriveModel onBrowseReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var json = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            if (err == 0) {
                cloudDrivePathDialog.parseCloudDriveMetadataJson(msg);
                cloudDrivePathDialog.open();
            } else if (err == 204) { // Refresh token
                // TODO Refactor to support all SkyDriveClient services.
                if (json.type == CloudDriveModel.SkyDrive) {
                    cloudDrivePathDialog.close();
                    cloudDriveModel.refreshToken(json.type, json.uid);
                }
            } else {
                messageDialog.titleText = getCloudName(json.type) + " " + appInfo.emptyStr+qsTr("Browse");
                messageDialog.message = appInfo.emptyStr+qsTr("Error") + " " + err + " " + errMsg + " " + msg;
                messageDialog.open();

                // Reset cloudDrivePathDialog.isBusy.
                cloudDrivePathDialog.isBusy = false;
                cloudDrivePathDialog.close();
            }

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);
        }

        onMigrateProgressSignal: {
            if (migrateProgressDialog.status != DialogStatus.Open) {
                migrateProgressDialog.indeterminate = false;
                migrateProgressDialog.min = 0;
                migrateProgressDialog.open();
            }
            migrateProgressDialog.source = localFilePath;
            migrateProgressDialog.value = count;
            migrateProgressDialog.max = total;
        }

        Component.onCompleted: {
            console.debug(Utility.nowText() + " folderPage cloudDriveModel onCompleted");
            window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("CloudDriveModel"), 0.1);
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
                            // Put from local.
                            cloudDriveModel.syncFromLocal(type, uid, localPath, remotePath, modelIndex, true);
                            // Queue sync selected local path after all put to refresh hash.
                            cloudDriveModel.metadata(type, uid, localPath, remotePath, modelIndex);
                            cloudDriveModel.resumeNextJob();
                        } else {
                            cloudDriveModel.filePut(type, uid, localPath, remotePath, modelIndex);
                        }
                        break;
                    case CloudDriveModel.SkyDrive:
                        if (fsModel.isDir(localPath)) {
                            cloudDriveModel.suspendNextJob();
                            // Put from local.
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
                            // Put from local.
                            cloudDriveModel.syncFromLocal(type, uid, localPath, remotePath, modelIndex, true);
                            // Queue sync selected local path after all put to refresh hash.
//                            cloudDriveModel.metadata(type, uid, localPath, remotePath, modelIndex);
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
                    var targetLocalPath;
                    switch (type) {
                    case CloudDriveModel.Dropbox:
                        targetLocalPath = fsModel.getAbsolutePath(fsModel.getDirPath(localPath), cloudDriveModel.getRemoteName(remotePath));
                        break;
                    case CloudDriveModel.SkyDrive:
                        targetLocalPath = fsModel.getAbsolutePath(fsModel.getDirPath(localPath), remotePathName);
                        break;
                    case CloudDriveModel.Ftp:
                        targetLocalPath = fsModel.getAbsolutePath(fsModel.getDirPath(localPath), cloudDriveModel.getRemoteName(remotePath));
                        break;
                    }
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
                    switch (type) {
                    case CloudDriveModel.Dropbox:
                        var originalRemotePath = cloudDriveModel.getItemRemotePath(localPath, type, uid);
                        if (remotePath != originalRemotePath) {
                            console.debug("cloudDrivePathDialog proceedOperation Browse move from " + originalRemotePath + " to " + remotePath);
                            // Set children hash to empty to hint syncFromLocal to put remained files with empty hash.
                            cloudDriveModel.updateItemWithChildren(type, uid, localPath, originalRemotePath, localPath, remotePath, "");
                            cloudDriveModel.metadata(type, uid, localPath, remotePath, selectedModelIndex);
                        }
                        break;
                    case CloudDriveModel.SkyDrive:
                        var originalRemotePath = cloudDriveModel.getItemRemotePath(localPath, type, uid);
                        if (remotePath != originalRemotePath) {
                            console.debug("cloudDrivePathDialog proceedOperation Browse move from " + originalRemotePath + " to " + remotePath);
                            // Remove original cloudDriveItem of localPath. Then start sync with new remotePath.
                            cloudDriveModel.removeItemWithChildren(type, uid, localPath);
                            cloudDriveModel.metadata(type, uid, localPath, remotePath, selectedModelIndex);
                        }
                        break;
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
                        // TODO
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
            // Update model.
            favContactModel.update();

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
            recipientSelectionDialog.model = favContactModel.getFavListModel(shareFileCaller);
        }
        onAccepted: {
            console.debug("recipientSelectionDialog onAccepted shareFileCaller " + shareFileCaller + " email " + selectedEmail + " senderEmail " + senderEmail + " selectedNumber " + selectedNumber);
            if (shareFileCaller == "mailFileSlot") {
                Qt.openUrlExternally("mailto:" + selectedEmail + "?subject=" + messageSubject + "&body=" + messageBody);
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
