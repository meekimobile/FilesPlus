import QtQuick 1.1
import com.nokia.meego 1.0
import AppInfo 1.0
import ClipboardModel 1.0
import GCPClient 1.0
import CloudDriveModel 1.0
import BookmarksModel 1.0
import FolderSizeItemListModel 1.0
import SystemInfoHelper 1.0
import "Utility.js" as Utility

PageStackWindow {
    id: window
    showStatusBar: appInfo.emptySetting+appInfo.getSettingBoolValue("statusbar.enabled", true)
    showToolBar: true

    property bool inverted: !theme.inverted

    state: "ready"
    states: [
        State {
            name: "busy"
            PropertyChanges { target: window.pageStack; busy: true }
            PropertyChanges { target: busyPanel; visible: true }
            PropertyChanges { target: busyindicator1; running: true }
        },
        State {
            name: "ready"
            PropertyChanges { target: window.pageStack; busy: false }
            PropertyChanges { target: busyPanel; visible: false }
            PropertyChanges { target: busyindicator1; running: false }
        }
    ]

    onStateChanged: {
        console.debug(Utility.nowText() + " window onStateChanged " + state);
    }

    onOrientationChangeFinished: {
        console.debug("window onOrientationChangeFinished");
        var p = pageStack.find(function(page) {
            if (page.name == "folderPage") {
                page.orientationChangeSlot();
            } else if (page.name == "imageViewPage") {
                page.orientationChangeSlot();
            }
        });
    }

    function updateLoadingProgressSlot(message, progressValue) {
        console.debug("window updateLoadingProgressSlot " + message + " " + progressValue);
        if (splashScreen) {
            splashScreen.updateLoadingProgress(message, progressValue)
        }
    }

    function findPage(pageName) {
        var p = pageStack.find(function (page) { return (page.name == pageName); });
        return p;
    }

    function quitSlot() {
        quitConfirmation.open();
    }

    AppInfo {
        id: appInfo

        onLocaleChanged: {
            console.debug("appInfo onLocaleChanged " + locale);
        }
    }

    FolderSizeItemListModel {
        id: fsModel

        property string state: ""

        // Test only. ISSUE: Setting currentDir to path that requires long caching time ("/home/user") will freeze UI at splash screen.

        function getActionName(fileAction) {
            switch (fileAction) {
            case FolderSizeItemListModel.CopyFile:
                return qsTr("Copy");
            case FolderSizeItemListModel.MoveFile:
                return qsTr("Move");
            case FolderSizeItemListModel.DeleteFile:
                return qsTr("Delete");
            case FolderSizeItemListModel.TrashFile:
                return qsTr("Trash");
            }
        }

        function getRunningOperationIconSource(runningOperation) {
            switch (runningOperation) {
            case FolderSizeItemListModel.ReadOperation:
                return "next.svg"
            case FolderSizeItemListModel.WriteOperation:
                return "back.svg"
            case FolderSizeItemListModel.SyncOperation:
                return (!inverted) ? "refresh.svg" : "refresh_inverted.svg"
            case FolderSizeItemListModel.UploadOperation:
                return (!inverted) ? "upload.svg" : "upload_inverted.svg"
            case FolderSizeItemListModel.DownloadOperation:
                return (!inverted) ? "download.svg" : "download_inverted.svg"
            default:
                return ""
            }
        }

        function mapToFolderSizeListModelOperation(cloudDriveModelOperation) {
            switch (cloudDriveModelOperation) {
            case CloudDriveModel.Metadata:
                return FolderSizeItemListModel.SyncOperation;
            case CloudDriveModel.FileGet:
            case CloudDriveModel.FileGetResume:
                return FolderSizeItemListModel.DownloadOperation;
            case CloudDriveModel.FilePut:
            case CloudDriveModel.FilePutResume:
            case CloudDriveModel.SyncFromLocal:
                return FolderSizeItemListModel.UploadOperation;
            case CloudDriveModel.MigrateFile:
            case CloudDriveModel.MigrateFilePut:
                return FolderSizeItemListModel.DownloadOperation;
            default:
                return FolderSizeItemListModel.SyncOperation;
            }
        }

        function openCopyProgressDialog(titleText) {
            // Estimate total.
            var totalBytes = 0;
            var totalFiles = 0;
            var totalFolders = 0;
            for (var i=0; i<clipboard.count; i++) {
                var action = clipboard.get(i).action;
                var sourcePath = clipboard.get(i).sourcePath;
//                console.debug("fileActionDialog openCopyProgressDialog estimate total action " + action + " sourcePath " + sourcePath);
                if ((action == "copy" || action == "cut") && !clipboard.get(i).type)  {
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
                copyProgressDialog.titleText = titleText;
                copyProgressDialog.message = "";
                copyProgressDialog.open();
            }
        }

        function openDeleteProgressDialog() {
            // Suppress dialog if FolderSizeItemListModel.deleteFIle uses trash.
            if (appInfo.getSettingBoolValue("FolderSizeItemListModel.deleteFIle.use.trash.enabled", false)) {
                return;
            }

            // Estimate total.
            var totalFiles = 0;
            var totalFolders = 0;
            for (var i=0; i<clipboard.count; i++) {
                var action = clipboard.get(i).action;
                var sourcePath = clipboard.get(i).sourcePath;
//                console.debug("fileActionDialog openDeleteProgressDialog estimate total action " + action + " sourcePath " + sourcePath);
                if (action == "delete" && !clipboard.get(i).type) {
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

        function findIndexByNameFilter(nameFilter, startIndex, backward) {
            backward = (!backward) ? false : true;
            var rx = new RegExp(nameFilter, "i");
            if (backward) {
                startIndex = (!startIndex) ? (fsModel.count - 1) : startIndex;
                startIndex = (startIndex < 0 || startIndex >= fsModel.count) ? (fsModel.count - 1) : startIndex;
                for (var i=startIndex; i>=0; i--) {
                    if (rx.test(fsModel.getProperty(i, FolderSizeItemListModel.NameRole))) {
                        return i;
                    }
                }
            } else {
                startIndex = (!startIndex) ? 0 : startIndex;
                startIndex = (startIndex < 0 || startIndex >= fsModel.count) ? 0 : startIndex;
                for (var i=startIndex; i<fsModel.count; i++) {
                    if (rx.test(fsModel.getProperty(i, FolderSizeItemListModel.NameRole))) {
                        return i;
                    }
                }
            }

            return -1;
        }

        onCurrentDirChanged: {
        }

        onRefreshBegin: {
            var p = findPage("folderPage");
            if (p) {
                p.refreshBeginSlot();
            }
        }

        onRefreshCompleted: {
            var p = findPage("folderPage");
            if (p) {
                p.refreshCompletedSlot();
            }
        }

        onRequestResetCache: {
            requestResetCacheConfirmation.open();
        }

        onCopyStarted: {
            console.debug("fsModel onCopyStarted " + fileAction + " from " + sourcePath + " to " + targetPath + " " + err + " " + msg);
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
//            console.debug("fsModel onCopyProgress " + fileAction + " from " + sourcePath + " to " + targetPath + " " + bytes + " / " + bytesTotal);
            copyProgressDialog.newValue = bytes+0;
        }

        onCopyFinished: {
            console.debug("fsModel onCopyFinished " + fileAction + " from " + sourcePath + " to " + targetPath + " " + err + " " + msg + " " + bytes + " " + totalBytes + " " + count);
            copyProgressDialog.count += count;

            // Show message if error.
            if (err < 0) {
                logError(getActionName(fileAction) + " " + qsTr("error"), msg);

                if (copyProgressDialog.status == DialogStatus.Open) {
                    copyProgressDialog.message = getActionName(fileAction) + " " + sourcePath + " " + qsTr("failed") + ".\n";
                }

                // TODO stop queued jobs
                fsModel.cancelQueuedJobs();
            }

            // TODO Connect to cloud if copied/moved file/folder is in connected folder.
            // TODO Make it configurable.
            var paths
            if (err == 0) {
                if (fileAction == FolderSizeItemListModel.MoveFile && cloudDriveModel.isParentConnected(sourcePath)) {
                    // Delete file from clouds.
                    var json = Utility.createJsonObj(cloudDriveModel.getItemListJson(sourcePath));
                    for (var i=0; i<json.length; i++) {
                        cloudDriveModel.deleteFile(json[i].type, json[i].uid, json[i].local_path, json[i].remote_path);
                    }

                    // Reset cloudDriveModel hash on parent. CloudDriveModel will update with actual hash once it got reply.
                    paths = fsModel.getPathToRoot(sourcePath);
                    if (paths.length > 1) {
//                        console.debug("fsModel onCopyFinished updateItems paths[1] " + paths[1]);
                        cloudDriveModel.updateItems(paths[1], cloudDriveModel.dirtyHash);
//                        cloudDriveModel.syncItem(paths[1]); // To be sync by syncDirtyItems()
                    }
                }

                if (cloudDriveModel.isParentConnected(targetPath)) {
                    // Reset cloudDriveModel hash on parent. CloudDriveModel will update with actual hash once it got reply.
                    paths = fsModel.getPathToRoot(targetPath);
                    if (paths.length > 1) {
//                        console.debug("fsModel onCopyFinished updateItems paths[1] " + paths[1]);
                        cloudDriveModel.updateItems(paths[1], cloudDriveModel.dirtyHash);
                    }
                }
            }
        }

        onDeleteStarted: {
            console.debug("fsModel onDeleteStarted " + fileAction + " sourcePath " + sourcePath);
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
            console.debug("fsModel onDeleteProgress " + fileAction + " sourceSubPath " + sourceSubPath + " err " + err + " msg " + msg);
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
            console.debug("fsModel onDeleteFinished " + fileAction + " sourcePath " + sourcePath);

            // Show message if error.
            if (err != 0) {
                logError(getActionName(fileAction) + " " + qsTr("error"), msg);

                if (deleteProgressDialog.status == DialogStatus.Open) {
                    deleteProgressDialog.message = getActionName(fileAction) + " " + sourcePath + " " + qsTr("failed") + ".\n";
                }

                // TODO stop queued jobs
                fsModel.cancelQueuedJobs();
            }

            // Delete file from clouds.
            // TODO Make it configurable.
            if (err == 0) {
                if (cloudDriveModel.isConnected(sourcePath)) {
                    var json = Utility.createJsonObj(cloudDriveModel.getItemListJson(sourcePath));
                    for (var i=0; i<json.length; i++) {
                        cloudDriveModel.deleteFile(json[i].type, json[i].uid, json[i].local_path, json[i].remote_path);
                    }

                    // Reset cloudDriveModel hash on parent. CloudDriveModel will update with actual hash once it got reply.
                    var paths = fsModel.getPathToRoot(sourcePath);
                    for (var i=1; i<paths.length; i++) {
//                        console.debug("fsModel onDeleteFinished updateItems paths[" + i + "] " + paths[i]);
                        cloudDriveModel.updateItems(paths[i], cloudDriveModel.dirtyHash);
                    }
                }
            }
        }

        onCreateFinished: {
            console.debug("fsModel onCreateFinished targetPath " + targetPath + " err " + err + " msg " + msg);;

            if (err == 0) {
                // If created item is not found, refresh.
                var targetIndex = fsModel.getIndexOnCurrentDir(targetPath);
                console.debug("fsModel onCreateFinished targetIndex " + targetIndex + " FolderSizeItemListModel.IndexOnCurrentDirButNotFound " + FolderSizeItemListModel.IndexOnCurrentDirButNotFound);
                if (targetIndex === FolderSizeItemListModel.IndexOnCurrentDirButNotFound) {
                    fsModel.clearIndexOnCurrentDir();
                    fsModel.refreshDir("fsModel onCreateFinished");
                }

                // Reset cloudDriveModel hash on parent.
                // Reset hash upto root.
                var paths = fsModel.getPathToRoot(targetPath);
                for (var i=0; i<paths.length; i++) {
                    console.debug("fsModel onCreateFinished updateItems paths[" + i + "] " + paths[i]);
                    cloudDriveModel.updateItems(paths[i], cloudDriveModel.dirtyHash);
                }

                // TODO request cloudDriveModel.createFolder
                // TODO Make it configurable.
            } else {
                logError(qsTr("Create") + " " + qsTr("error"), msg);
            }
        }

        onRenameFinished: {
            console.debug("fsModel onRenameFinished sourcePath " + sourcePath + " targetPath " + targetPath + " err " + err + " msg " + msg);

            if (err != 0) {
                logError(qsTr("Rename") + " " + qsTr("error"), msg);
            }

            // Rename file on clouds by specify empty newRemoteParentPath to moveFIle method.
            // TODO Make it configurable.
            if (err == 0 && cloudDriveModel.isConnected(sourcePath)) {
//                console.debug("fsModel onRenameFinished itemList " + cloudDriveModel.getItemListJson(sourcePath));
                var json = Utility.createJsonObj(cloudDriveModel.getItemListJson(sourcePath));
                for (var i=0; i<json.length; i++) {
                    console.debug("fsModel onRenameFinished item " + json[i].type + " " + json[i].uid + " " + json[i].local_path + " " + json[i].remote_path);
                    cloudDriveModel.moveFile(json[i].type, json[i].uid, sourcePath, json[i].remote_path, targetPath, "", fsModel.getFileName(targetPath));
                }
            }
        }

        onFetchDirSizeStarted: {
            var p = findPage("folderPage");
            if (p) {
                p.fetchDirSizeStartedSlot();
            }
        }

        onFetchDirSizeFinished: {
            var p = findPage("folderPage");
            if (p) {
                p.fetchDirSizeFinishedSlot();
            }
        }

        onDirectoryChanged: {
            console.debug("window fsModel onDirectoryChanged " + dirPath);

            // Check if dirPath is connected and not syncing, then trigger synchronization.
            if (cloudDriveModel.isConnected(dirPath)) {
                if (!cloudDriveModel.isSyncing(dirPath)) {
                    // Reset cloudDriveModel hash on parent. CloudDriveModel will update with actual hash once it got reply.
                    cloudDriveModel.updateItems(dirPath, cloudDriveModel.dirtyHash);
//                    cloudDriveModel.syncItem(dirPath); // To be sync by syncDirtyItems()
                } else {
                    console.debug("window fsModel onDirectoryChanged " + dirPath + " is synchronizing, suppress synchronization request.");
                }
            }
        }

        onInitializeDBFinished: {
            fsModel.requestTrashStatus();
        }

        onTrashChanged: {
            console.debug("window fsModel onTrashChanged");

            // Update trash size.
            if (appInfo.getSettingBoolValue("drivepage.trash.enabled", false)) {
                var trashObj = Utility.createJsonObj(fsModel.getTrashJsonText());
                var trashMaxSize = (trashObj.absolute_path != "") ? fsModel.getMaxTrashSize() : -1;
                var trashAvailableSize = (trashObj.absolute_path != "") ? (trashMaxSize - trashObj.size) : 0;
                var p = findPage("drivePage");
                if (p) {
                    p.updateLogicalDriveSlot(fsModel.getTrashPath(), trashAvailableSize, trashMaxSize);
                }
            }
        }

        onTrashFinished: {
            console.debug("fsModel onTrashFinished " + fileAction + " sourcePath " + sourcePath);

            // Show message if error.
            if (err != 0) {
                logError(getActionName(fileAction) + " " + qsTr("error"), msg);

                // TODO stop queued jobs
                fsModel.cancelQueuedJobs();
            }

            // Delete file from clouds.
            // TODO Make it configurable.
            if (err == 0) {
                if (cloudDriveModel.isConnected(sourcePath)) {
                    var json = Utility.createJsonObj(cloudDriveModel.getItemListJson(sourcePath));
                    for (var i=0; i<json.length; i++) {
                        cloudDriveModel.deleteFile(json[i].type, json[i].uid, json[i].local_path, json[i].remote_path);
                    }

                    // Reset cloudDriveModel hash on parent. CloudDriveModel will update with actual hash once it got reply.
                    var paths = fsModel.getPathToRoot(sourcePath);
                    for (var i=1; i<paths.length; i++) {
//                        console.debug("fsModel onTrashFinished updateItems paths[" + i + "] " + paths[i]);
                        cloudDriveModel.updateItems(paths[i], cloudDriveModel.dirtyHash);
                    }
                }
            }
        }

        Component.onCompleted: {
            console.debug(Utility.nowText() + " window fsModel onCompleted");
            window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("FolderModel"), 0.1);

            // Proceeds queued jobs during constructions.
            fsModel.proceedNextJob();
        }
    }

    ConfirmDialog {
        id: quitConfirmation
        titleText: appInfo.emptyStr+qsTr("Quit")

        function getText() {
            var text = "";
            text += (cloudDriveModel.getRunningJobCount()+fsModel.getRunningJobCount() > 0) ? qsTr("There are %n running job(s).", "", cloudDriveModel.getRunningJobCount()+fsModel.getRunningJobCount())+"\n" : "";
            text += (cloudDriveModel.getQueuedJobCount()+fsModel.getQueuedJobCount() > 0) ? qsTr("There are %n queued job(s).", "", cloudDriveModel.getQueuedJobCount()+fsModel.getQueuedJobCount())+"\n" : "";
            text += (text != "") ? "\n" : "";
            text += qsTr("Please click OK to continue.");
            return text;
        }

        onOpening: {
            contentText = appInfo.emptyStr+getText();
        }
        onConfirm: {
//            var runningCount = cloudDriveModel.getRunningJobCount()+fsModel.getRunningJobCount();
//            if (runningCount > 0) {
//                cloudDriveModel.suspendNextJob(true);
//                logInfo(qsTr("Quit"), qsTr("FilesPlus will quit automatically once current running jobs are aborted."), 3000);
//            } else {
//                Qt.quit();
//            }
            cloudDriveModel.suspendNextJob(true);
            Qt.quit();
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
            fsModel.refreshDir("requestResetCacheConfirmation", true);
        }
    }

    ConfirmDialog {
        id: resetCacheConfirmation
        titleText: appInfo.emptyStr+qsTr("Reset folder cache")
        contentText: appInfo.emptyStr+qsTr("Resetting folder cache will take time depends on numbers of sub folders/files under current folder.\n\nPlease click OK to continue.")
        onConfirm: {
            fsModel.refreshDir("resetCacheConfirmation", true);
        }
    }

    ConfirmDialog {
        id: cancelQueuedCloudDriveJobsConfirmation
        titleText: appInfo.emptyStr+qsTr("Cancel Cloud Drive Jobs")
        onOpening: {
            contentText = appInfo.emptyStr+qsTr("Cancel %n job(s) ?", "", cloudDriveModel.getQueuedJobCount());
        }
        onConfirm: {
            cloudDriveModel.cancelQueuedJobs();
            cloudDriveModel.removeJobs();
            cloudDriveModel.requestJobQueueStatus();
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

            // Re-initialize DropboxClient.
            cloudDriveModel.refreshDropboxClient();

            // Migrate DAT to DB.
            appInfo.setSettingValue("cloudItems.migration.confirmation", true);
            cloudDriveModel.migrateCloudDriveItemsToDB();

            // Update DB.
            cloudDriveModel.updateDropboxPrefix(appInfo.getSettingBoolValue("dropbox.fullaccess.enabled", false));

            logInfo("Dropbox " + appInfo.emptyStr+qsTr("notify"),
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

    ProgressDialog {
        id: copyProgressDialog
        autoClose: false
        onOk: {
            // Refresh view after copied/moved.
            fsModel.refreshDir("copyProgressDialog onOk");
        }
        onCancel: {
            fsModel.cancelQueuedJobs();
            // Abort thread with rollbackFlag=false.
            fsModel.abortThread(false);
            // Refresh view after copied/moved.
            fsModel.refreshDir("copyProgressDialog onCancel");
        }
        onOpened: {
            // Set CDM flags to pause while fsModel is running.
            cloudDriveModel.suspendScheduledJob();
            cloudDriveModel.setIsPaused(true);

            fsModel.resumeNextJob();
        }
        onClosed: {
            // Reset CDM flags.
            cloudDriveModel.resumeScheduledJob();
            cloudDriveModel.setIsPaused(false);
        }
    }

    ProgressDialog {
        id: deleteProgressDialog
        autoClose: false
        autoCloseByValue: true
        titleText: appInfo.emptyStr+qsTr("Deleting")
        onOk: {
            // Refresh view after copied/moved.
            // NOTE FolderSizeItemListModel.deleteProgressFilter() already remove item from model.
            fsModel.refreshDir("deleteProgressDialog onOk");
        }
        onCancel: {
            fsModel.cancelQueuedJobs();
            // Abort thread with rollbackFlag=false.
            fsModel.abortThread(false);
            // Refresh view after copied/moved.
            // NOTE FolderSizeItemListModel.deleteProgressFilter() already remove item from model.
            fsModel.refreshDir("deleteProgressDialog onCancel");
        }
        onOpened: {
            // Set CDM flags to pause while fsModel is running.
            cloudDriveModel.suspendScheduledJob();
            cloudDriveModel.setIsPaused(true);

            fsModel.resumeNextJob();
        }
        onClosed: {
            // Reset CDM flags.
            cloudDriveModel.resumeScheduledJob();
            cloudDriveModel.setIsPaused(false);
        }
    }

    ClipboardModel {
        id: clipboard

        function getActionIcon(localPath, type, uid) {
            var jsonText = clipboard.getItemJsonText(localPath);
            if (jsonText != "") {
                var json = Utility.createJsonObj(jsonText);
                if (json) {
                    if (json.type == type && json.uid == uid) {
                        if (json.action == "copy") {
                            return (!inverted) ? "copy.svg" : "copy_inverted.svg";
                        } else if (json.action == "cut") {
                            return (!inverted) ? "trim.svg" : "trim_inverted.svg";
                        }
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
            console.debug(Utility.nowText() + " clipboard additem json " + json);
            clipboard.addClipboardItem(json.sourcePath, Utility.createJsonText(json) );
        }

        function addItemWithSuppressCountChanged(json) {
            console.debug(Utility.nowText() + " clipboard addItemWithSuppressCountChanged json " + json);
            clipboard.addClipboardItem(json.sourcePath, Utility.createJsonText(json), true);
        }
    }

    SystemInfoHelper {
        id: helper
    }

    GCPClient {
        id: gcpClient

        property string selectedFilePath
        property string selectedURL

        function setGCPClientAuthCode(code) {
            var res = gcpClient.parseAuthorizationCode(code);
            if (res) {
                gcpClient.accessToken();
            }
        }

        function getPrinterListModel() {
            var printerList = gcpClient.getStoredPrinterList();
            console.debug("gcpClient.getPrinterListModel() printerList " + printerList);

            // Construct model.
            var model = Qt.createQmlObject('import QtQuick 1.1; ListModel {}', gcpClient);

            model.clear();
            for (var i=0; i<printerList.length; i++) {
                model.append({ name: printerList[i] });
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

        function printFileSlot(srcFilePath, selectedIndex) {
            console.debug("gcpClient printFileSlot srcFilePath=" + srcFilePath);

            // Set source file to GCPClient.
            gcpClient.selectedFilePath = srcFilePath;
            if (srcFilePath == "") return;

            if (gcpClient.getContentType(srcFilePath) == "") {
                console.debug("window printFileSlot File type is not supported. (" + srcFilePath + ")");

                messageDialog.titleText = appInfo.emptyStr+qsTr("Print Error");
                messageDialog.message = appInfo.emptyStr+qsTr("Can't print %1\
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
                showPrinterSelectionDialog(srcFilePath, "");
            }
        }

        function printURLSlot(url) {
            console.debug("gcpClient printURLSlot url=" + url);

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
                showPrinterSelectionDialog("", url);
            }
        }

        function resetCloudPrintSlot() {
//            popupToolPanel.selectedFilePath = "";
//            popupToolPanel.selectedFileIndex = -1;
            gcpClient.authorize();
        }

        onAuthorizeRedirectSignal: {
            console.debug("window gcpClient onAuthorizeRedirectSignal " + url);
            pageStack.push(Qt.resolvedUrl("AuthPage.qml"), { url: url, redirectFrom: "GCPClient" }, true);
        }

        onAccessTokenReplySignal: {
            console.debug("window gcpClient onAccessTokenReplySignal " + err + " " + errMsg + " " + msg);

            if (err == 0) {
                // Search for printers.
                gcpClient.search("");
            } else {
                gcpClient.refreshAccessToken();
            }
        }

        onRefreshAccessTokenReplySignal: {
            console.debug("window gcpClient onRefreshAccessTokenReplySignal " + err + " " + errMsg + " " + msg);

            if (err == 0) {
                // Notify successful refreshAccessToken.
                logWarn("CloudPrint " + qsTr("Refresh Token"), qsTr("Token was refreshed."), 2000);
                // Search for printers.
                gcpClient.search("");
            } else {
                // Notify failed refreshAccessToken.
                logError("CloudPrint " + qsTr("Refresh Token"), qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }
        }

        onAccountInfoReplySignal: {
            console.debug("window gcpClient onAccountInfoReplySignal " + err + " " + errMsg + " " + msg);

            var jsonObj = Utility.createJsonObj(msg);
            console.debug("jsonObj.email " + jsonObj.email);
        }

        onSearchReplySignal: {
            console.debug("window gcpClient onSearchReplySignal " + err + " " + errMsg + " " + msg);

            if (err == 0) {
                // Update printer model.
                if (printerSelectionDialog.model) {
                    printerSelectionDialog.model.destroy();
                }
                printerSelectionDialog.model = getPrinterListModel();
                // Once search done, resume printing.
                resumePrinting();
            } else {
                gcpClient.refreshAccessToken();
            }
        }

        onSubmitReplySignal: {
            console.debug("window gcpClient onSubmitReplySignal " + err + " " + errMsg + " " + msg);

            // Notify submit result.
            var jsonObj = Utility.createJsonObj(msg);
            var message = "";
            if (err == 0) {
                // Do nothing.
            } else if (err == 202) {
                gcpClient.refreshAccessToken();
            } else {
                message = qsTr("Error") + " " + err + " " + errMsg + "\n";
            }
            message += jsonObj.message;

            // Show reply message on progressDialog and also turn indeterminate off.
            if (uploadProgressDialog.status != DialogStatus.Open) {
                uploadProgressDialog.titleText = appInfo.emptyStr+qsTr("Printing");
                uploadProgressDialog.srcFilePath = selectedFilePath;
                uploadProgressDialog.autoClose = false;
                uploadProgressDialog.open();
            }
            uploadProgressDialog.indeterminate = false;
            uploadProgressDialog.message = appInfo.emptyStr+message;

            // Reset selected item.
            gcpClient.selectedFilePath = "";
            gcpClient.selectedURL = "";
        }

        onDownloadProgress: {
//            console.debug("sandBox gcpClient onDownloadProgress " + bytesReceived + " / " + bytesTotal);
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

        onJobsReplySignal: {
            console.debug("printJobsPage gcpClient onJobsReplySignal " + err + " " + errMsg + " " + msg);

            if (err == 0) {
                var p = findPage("printJobsPage");
                if (p) {
                    p.addJobsFromJson(msg);
                }
            } else if (err == 202) { // You have to be signed in to your Google Account.
                gcpClient.refreshAccessToken();
            }
        }

        onDeletejobReplySignal: {
            console.debug("printJobsPage gcpClient onDeletejobReplySignal " + err + " " + errMsg + " " + msg);
        }

        Component.onCompleted: {
            console.debug(Utility.nowText() + " window gcpClient onCompleted");
            window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("GCPClient"), 0.1);

            if (gcpClient.isAuthorized()) {
                // Populate contentTypeHash.
                gcpClient.loadContentTypeHash(appInfo.getConfigPath() + "/mime.types");
                gcpClient.search("");
            }
        }
    }

    CloudDriveModel {
        id: cloudDriveModel

        property string shareFileCaller
        property string state: ""

        function refreshCloudDriveAccounts(caller) {
            var p = findPage("drivePage");
            if (p) p.refreshSlot(caller);
        }

        function getUidListModel(localPath) {
            console.debug("window cloudDriveModel getUidListModel localPath " + localPath);

            // Get uid list.
            var uidList = cloudDriveModel.getStoredUidList();

            // Construct model.
            var model = Qt.createQmlObject('import QtQuick 1.1; ListModel {}', cloudDriveModel);

            for (var i=0; i<uidList.length; i++)
            {
                var json = JSON.parse(uidList[i]);
                var cloudItem = Utility.createJsonObj(cloudDriveModel.getItemJson(localPath, getClientType(json.type), json.uid));
                console.debug("window cloudDriveModel getUidListModel i " + i + " type " + json.type + " uid " + json.uid + " email " + json.email + " localHash " + cloudItem.hash + " remotePath " + cloudItem.remote_path);
                model.append({
                                 type: getClientType(json.type),
                                 uid: json.uid,
                                 email: json.email,
                                 token: json.token,
                                 secret: json.secret,
                                 hash: cloudItem.hash,
                                 name: "",
                                 shared: 0,
                                 normal: 0,
                                 quota: -1
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
            case CloudDriveModel.WebDAV:
                return "webdav_icon.png";
            }
        }

        function getClientType(typeText) {
            if (typeText) {
                if (["dropboxclient","dropbox"].indexOf(typeText.toLowerCase()) != -1) {
                    return CloudDriveModel.Dropbox;
                } else if (["skydriveclient","skydrive"].indexOf(typeText.toLowerCase()) != -1) {
                    return CloudDriveModel.SkyDrive;
                } else if (["gcdclient","googledriveclient","googledrive"].indexOf(typeText.toLowerCase()) != -1) {
                    return CloudDriveModel.GoogleDrive;
                } else if (["ftpclient","ftp"].indexOf(typeText.toLowerCase()) != -1) {
                    return CloudDriveModel.Ftp;
                } else if (["webdavclient","webdav"].indexOf(typeText.toLowerCase()) != -1) {
                    return CloudDriveModel.WebDAV;
                }
            }

            return -1;
        }

        function accessTokenSlot(clientTypeName, pin) {
            console.debug("window accessTokenSlot clientTypeName " + clientTypeName + " pin " + pin);
            var clientType = getClientType(clientTypeName);
            if (pin == "" || pin == "PinNotFound") {
                messageDialog.titleText =  appInfo.emptyStr+getCloudName(clientType)+" "+qsTr("Access Token");
                messageDialog.message = appInfo.emptyStr+qsTr("Error") + " " + qsTr("PIN code is not found.");
                messageDialog.open();
            } else {
                cloudDriveModel.accessToken(clientType, pin);
            }
        }

        function parseCloudDriveMetadataJson(selectedCloudType, selectedUid, originalRemotePath, jsonText,  model) {
            // Parse jsonText and return collected information.
            var json = Utility.createJsonObj(jsonText);

            var selectedIndex = -1;
            var selectedRemotePath = "";
            var selectedRemotePathName = "";
            var selectedIsDir = false;
            var selectedIsValid = true;
            var remoteParentPath = "";
            var remoteParentPathName = "";
            var remoteParentParentPath = "";

            // Generalize by using common metadata json.
            var parsedObj = parseCommonCloudDriveMetadataJson(selectedCloudType, selectedUid, json);
            remoteParentPath = parsedObj.absolutePath;
            remoteParentPathName = cloudDriveModel.isRemoteAbsolutePath(selectedCloudType) ? parsedObj.absolutePath : parsedObj.name;
            remoteParentParentPath = parsedObj.parentPath;
            for (var i=0; i < parsedObj.children.length; i++) {
                var modelItem = parsedObj.children[i];
                model.append(modelItem);
                if (modelItem.absolutePath == originalRemotePath) {
                    selectedIndex = i;
                    selectedRemotePath = modelItem.absolutePath;
                    selectedRemotePathName = modelItem.name;
                    selectedIsDir = modelItem.isDir;
                }
            }

            var responseJson = { "selectedIndex": selectedIndex,
                "selectedRemotePath": selectedRemotePath, "selectedRemotePathName": selectedRemotePathName, "selectedIsDir": selectedIsDir,
                "remoteParentPath": remoteParentPath, "remoteParentPathName": remoteParentPathName, "remoteParentParentPath": remoteParentParentPath
            };

            return responseJson;
        }

        function parseCommonCloudDriveMetadataJson(selectedCloudType, selectedUid, jsonObj) {
            // Parse common JSON object from cloud drive specific JSON object.
            var parsedObj = {
                "name": "", "absolutePath": "", "parentPath": "",
                "isChecked": false, "source": "", "thumbnail": "", "preview": "", "alternative": "",
                "fileType": "", "isRunning": false, "runningOperation": "", "runningValue": 0, "runningMaxValue": 0,
                "subDirCount": 0, "subFileCount": 0, "isDirty": false,
                "lastModified": (new Date()), "size": 0, "isDir": false,
                "isDeleted": false,
                "isConnected": false,
                "timestamp": 0,
                "hash": "",
                "children": []
            };

            parsedObj.name = jsonObj.name;
            parsedObj.absolutePath = jsonObj.absolutePath;
            parsedObj.parentPath = jsonObj.parentPath;
            parsedObj.size = jsonObj.size;
            parsedObj.isDeleted = jsonObj.isDeleted;
            parsedObj.isDir = jsonObj.isDir;
            parsedObj.lastModified = Utility.parseDate(jsonObj.lastModified);
            parsedObj.hash = jsonObj.hash;
            parsedObj.source = jsonObj.source;
            parsedObj.alternative = jsonObj.alternative;
            parsedObj.thumbnail = jsonObj.thumbnail;
            parsedObj.preview = jsonObj.preview;
            parsedObj.fileType = jsonObj.fileType;
            if (jsonObj.children) {
                for(var i=0; i<jsonObj.children.length; i++) {
                    var parsedChildObj = parseCommonCloudDriveMetadataJson(selectedCloudType, selectedUid, jsonObj.children[i]);
                    parsedObj.children.push(parsedChildObj);
                }
            }
            // Get connection status.
            parsedObj.isConnected = cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, parsedObj.absolutePath);

            return parsedObj;
        }

        function findIndexByRemotePath(remotePath) {
//            console.debug("cloudDriveModel.findIndexByRemotePath cloudDriveModel.count " + cloudDriveModel.count);
            for (var i=0; i<cloudDriveModel.count; i++) {
                if (cloudDriveModel.get(i).absolutePath == remotePath) {
                    return i;
                }
            }

            return -1;
        }

        function findIndexByRemotePathName(remotePathName) {
            for (var i=0; i<cloudDriveModel.count; i++) {
                if (cloudDriveModel.get(i).name == remotePathName) {
                    return i;
                }
            }

            return -1;
        }

        function findIndexByNameFilter(nameFilter, startIndex, backward) {
            backward = (!backward) ? false : true;
            var rx = new RegExp(nameFilter, "i");
            if (backward) {
                startIndex = (!startIndex) ? (cloudDriveModel.count - 1) : startIndex;
                startIndex = (startIndex < 0 || startIndex >= cloudDriveModel.count) ? (cloudDriveModel.count - 1) : startIndex;
                for (var i=startIndex; i>=0; i--) {
                    if (rx.test(cloudDriveModel.get(i).name)) {
                        return i;
                    }
                }
            } else {
                startIndex = (!startIndex) ? 0 : startIndex;
                startIndex = (startIndex < 0 || startIndex >= cloudDriveModel.count) ? 0 : startIndex;
                for (var i=startIndex; i<cloudDriveModel.count; i++) {
                    if (rx.test(cloudDriveModel.get(i).name)) {
                        return i;
                    }
                }
            }

            return -1;
        }

        function getNewFileName(remotePathName) {
//            console.debug("cloudDriveModel getNewFileName " + remotePathName);

            var foundIndex = findIndexByRemotePathName(remotePathName);
            if (foundIndex > -1) {
                var newRemotePathName = "";
                var tokens = remotePathName.split(".", 2);
//                console.debug("cloudDriveModel getNewFileName tokens [" + tokens + "]");

                if (tokens[0].lastIndexOf(qsTr("_Copy")) > -1) {
                    var nameTokens = tokens[0].split(qsTr("_Copy"), 2);
//                    console.debug("cloudDriveModel getNewFileName nameTokens [" + nameTokens + "]");

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

        onRequestTokenReplySignal: {
            console.debug("window cloudDriveModel onRequestTokenReplySignal " + err + " " + errMsg + " " + msg);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            if (err == 0) {
                cloudDriveModel.authorize(jobJson.type);
            } else {
                logError(getCloudName(jobJson.type) + " " + qsTr("Request Token"),
                         qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }

            // Remove finished job.
            cloudDriveModel.removeJob("cloudDriveModel.onRequestTokenReplySignal", jobJson.job_id);
        }

        onAuthorizeRedirectSignal: {
            console.debug("window cloudDriveModel onAuthorizeRedirectSignal " + url + " redirectFrom " + redirectFrom);

            pageStack.push(Qt.resolvedUrl("AuthPage.qml"), { url: url, redirectFrom: redirectFrom }, true);

            // Remove finished job.
            cloudDriveModel.removeJob("cloudDriveModel.onAuthorizeRedirectSignal", nonce);
        }

        onAccessTokenReplySignal: {
            console.debug("window cloudDriveModel onAccessTokenReplySignal " + err + " " + errMsg + " " + msg);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            if (err == 0) {
                // TODO Get account info and show in dialog.
                if (jobJson.operation == CloudDriveModel.RefreshToken) {
                    logWarn(getCloudName(jobJson.type) + " " + qsTr("Refresh Token"),
                            qsTr("Token was refreshed.") );
                } else {
                    logInfo(getCloudName(jobJson.type) + " " + qsTr("Access Token"),
                            qsTr("CloudDrive user is authorized.\nPlease proceed your sync action."),
                            2000);

                    // Refresh to get newly authorized cloud drive.
                    cloudDriveModel.refreshCloudDriveAccounts("window onAccessTokenReplySignal jobJson.type " + jobJson.type + " jobJson.uid " + jobJson.uid);
                }
            } else {
                logError(getCloudName(jobJson.type) + " " + qsTr("Access Token"),
                         qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }

            // Remove finished job.
            cloudDriveModel.removeJob("cloudDriveModel.onAccessTokenReplySignal", jobJson.job_id);
        }

        onAccountInfoReplySignal: {
            console.debug("window cloudDriveModel onAccountInfoReplySignal " + err + " " + errMsg + " " + msg);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            if (err == 0) {
                // Do nothing.
            } else if (err == 204) {
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid, jobJson.job_id);
                return;
            } else {
                logError(getCloudName(jobJson.type) + " " + qsTr("Account Info"),
                         qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }

            // Remove finished job.
            cloudDriveModel.removeJob("cloudDriveModel.onAccountInfoReplySignal", jobJson.job_id);
        }

        onQuotaReplySignal: {
            console.debug("window cloudDriveModel onQuotaReplySignal " + nonce + " " + err + " " + errMsg + " " + msg + " normalBytes " + normalBytes + " sharedBytes " + sharedBytes + " quotaBytes " + quotaBytes);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            if (err == 0) {
                var jsonObj = Utility.createJsonObj(msg);

                // Check if email is included, otherwise get from account.
                var email = jsonObj.email;
                if (!email) {
                    var accountJsonObj = Utility.createJsonObj(cloudDriveModel.getStoredUid(jobJson.type, jobJson.uid));
                    email = accountJsonObj.email;
                }

                // Send info to cloudDriveAccountsModel.
                cloudDriveAccountsModel.updateAccountInfoSlot(jobJson.type, jobJson.uid, jsonObj.name, email, sharedBytes, normalBytes, quotaBytes);

                // Send info to relevant pages.
                pageStack.find(function (page) {
                    if (page.updateAccountInfoSlot) {
                        page.updateAccountInfoSlot(jobJson.type, jobJson.uid, jsonObj.name, jsonObj.email, sharedBytes, normalBytes, quotaBytes);
                    }
                });
            } else if (err == 204) {
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid, jobJson.job_id);
                return;
            } else {
                logError(getCloudName(jobJson.type) + " " + qsTr("Account Quota"),
                         qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }

            // Remove finished job.
            cloudDriveModel.removeJob("cloudDriveModel.onQuotaReplySignal", jobJson.job_id);
        }

        onBrowseReplySignal: {
            console.debug("window cloudDriveModel onBrowseReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            if (err == 0) {
                pageStack.find(function (page) {
                    if (page.name == "cloudFolderPage") {
                        page.postBrowseReplySlot();
                    } else if (page.name == "folderPage") {
                        page.postBrowseReplySlot();
                    }
                });
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid, jobJson.job_id);
                return;
            } else {
                logError(getCloudName(jobJson.type) + " " + qsTr("Browse"),
                         qsTr("Error") + " " + err + " " + errMsg + " " + msg);

                // Reset and close cloudDrivePathDialog.
                var p = findPage("folderPage");
                if (p) {
                    p.closeCloudDrivePathDialogSlot();
                }

                // Reset cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.resetBusySlot("cloudDriveModel onBrowseReplySignal");
                }
            }

            // Remove finished job.
            cloudDriveModel.removeJob("cloudDriveModel.onBrowseReplySignal", jobJson.job_id);
        }

        onFileGetReplySignal: {
            console.debug("window cloudDriveModel onFileGetReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            if (err == 0) {
                // Remove cache on target folders and its parents.
                var p = findPage("folderPage");
                if (p) {
                    p.refreshItemAfterFileGetSlot(jobJson.local_file_path);
                }
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid, jobJson.job_id);
                return;
            } else {
                logError(getCloudName(jobJson.type) + " " + qsTr("File Get"),
                         qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }

            // Remove finished job.
            // TODO Remove only success job.
            if (err == 0) {
                cloudDriveModel.removeJob("cloudDriveModel.onFileGetReplySignal", jobJson.job_id);
            }

            // Update ProgressBar on listItem and its parents. Needs to update after removeJob as isSyncing check if job exists.
            pageStack.find(function (page) {
                if (page.updateItemSlot) page.updateItemSlot(jobJson);
            });
        }

        onFilePutReplySignal: {
            console.debug("window cloudDriveModel onFilePutReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            if (err == 0) {
                // Refresh cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.refreshItemAfterFilePutSlot(jobJson);
                }
            } else if (err == 204) { // Refresh token
                // TODO Whether it should stop and let user manually resume?
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid, jobJson.job_id);
                return;
            } else {
                logError(getCloudName(jobJson.type) + " " + qsTr("File Put"),
                         qsTr("Error") + " " + err + " " + errMsg + " " + msg);

                // Reset cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.resetBusySlot("cloudDriveModel onFilePutReplySignal");
                }
            }

            // Remove finished job.
            // TODO Remove only success job.
            if (err == 0) {
                cloudDriveModel.removeJob("cloudDriveModel.onFilePutReplySignal", jobJson.job_id);
            }

            // Update ProgressBar on listItem and its parents. Needs to update after removeJob as isSyncing check if job exists.
            pageStack.find(function (page) {
                if (page.updateItemSlot) page.updateItemSlot(jobJson);
            });
        }

        onMetadataReplySignal: {
            // Get job json.
            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            console.debug("window cloudDriveModel onMetadataReplySignal " + getCloudName(jobJson.type) + " " + nonce + " " + err + " " + errMsg + " " + msg);

            if (err == 0) {
                // Do nothing.
            } else if (err == 203) {
                // Do nothing.
            } else {
                logError(getCloudName(jobJson.type) + " " + qsTr("Metadata"),
                         qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }

            // Remove finished job.
            // TODO Remove only success job.
            if (err == 0 || err == 203) {
                cloudDriveModel.removeJob("cloudDriveModel.onMetadataReplySignal", jobJson.job_id);
            }

            // Update ProgressBar on listItem and its parents. Needs to update after removeJob as isSyncing check if job exists.
            pageStack.find(function (page) {
                if (page.updateItemSlot) page.updateItemSlot(jobJson);
            });
        }

        onCreateFolderReplySignal: {
            console.debug("window cloudDriveModel onCreateFolderReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));
            var msgJson = Utility.createJsonObj(msg);

            // NOTE creatFolder always have jobJson.remote_file_path = remoteParentPath.
            if (err == 0) {
                // Refresh cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.refreshSlot("cloudDriveModel onCreateFolderReplySignal");
                }
            } else if (err == 202 && jobJson.type == CloudDriveModel.Dropbox && jobJson.local_file_path != "") {
                // Dropbox Folder already exists. Do nothing
                logError(getCloudName(jobJson.type) + " " + qsTr("Create Folder"),
                        qsTr("Error") + " " + err + " " + errMsg + " " + msg +
                        "\n\n" +
                        qsTr("Please proceed with sync.") );
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid, jobJson.job_id);
                return;
            } else if (err == 299 && jobJson.type == CloudDriveModel.SkyDrive && msgJson.error && msgJson.error.code == "resource_already_exists") {
                // SkyDrive Folder already exists. Do nothing
                logError(getCloudName(jobJson.type) + " " + qsTr("Create Folder"),
                        qsTr("Error") + " " + err + " " + errMsg + " " + msg +
                        "\n\n" +
                        qsTr("Please proceed with sync.") );
            } else {
                logError(getCloudName(jobJson.type) + " " + qsTr("Create Folder"),
                         qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }

            // Remove finished job.
            cloudDriveModel.removeJob("cloudDriveModel.onCreateFolderReplySignal", jobJson.job_id);

            // TODO *** create SkyDrive folder freezes here *** TODO Does it need?
            // Update ProgressBar if localPath is specified.
            if (jobJson.type == CloudDriveModel.SkyDrive && pageStack.currentPage.name == "folderPage") {
                // Refresh all items because jobJson.local_file_path is new folder name.
                // TODO jobJson.local_file_path is no longer contains new folder name. jobJson.new_remote_file_name does.
                pageStack.currentPage.refreshItemSlot("cloudDriveModel onCreateFolderReplySignal SkyDrive");
            } else {
                // Refresh only created folder and its parents.
                pageStack.find(function (page) {
                    if (page.updateItemSlot) page.updateItemSlot(jobJson);
                });
            }

            if (pageStack.currentPage.name == "folderPage") {
                // Refresh cloudDrivePathDialog if it's opened.
                pageStack.currentPage.updateCloudDrivePathDialogSlot(jobJson.new_remote_file_path);
            }

            // Reset cloudFolderPage.
            var p = findPage("cloudFolderPage");
            if (p) {
                p.resetBusySlot("cloudDriveModel onCreateFolderReplySignal");
            }
        }

        onDownloadProgress: {
//            console.debug("window cloudDriveModel onDownloadProgress " + nonce + " " + bytesReceived + " / " + bytesTotal);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));
            cloudDriveJobsModel.updateJobProgressBar(jobJson);

            // Update ProgressBar on listItem and its parent.
            pageStack.find(function (page) {
                if (page.updateItemProgressBarSlot) page.updateItemProgressBarSlot(jobJson);
            });
        }

        onUploadProgress: {
//            console.debug("window cloudDriveModel onUploadProgress " + nonce + " " + bytesSent + " / " + bytesTotal);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));
            cloudDriveJobsModel.updateJobProgressBar(jobJson);

            // Update ProgressBar on listItem and its parent.
            pageStack.find(function (page) {
                if (page.updateItemProgressBarSlot) page.updateItemProgressBarSlot(jobJson);
            });
        }

        onLocalChangedSignal: {
            // TODO Disable becuase it can damage stored hash.
            // Reset CloudDriveItem hash upto root.
//            var paths = fsModel.getPathToRoot(localPath);
//            for (var i=0; i<paths.length; i++) {
//                console.debug("window cloudDriveModel onLocalChangedSignal updateItems paths[" + i + "] " + paths[i]);
//                cloudDriveModel.updateItems(paths[i], cloudDriveModel.dirtyHash);
//            }
        }

        onRefreshFolderCacheSignal: {
            // TODO Remove folder cache.
            fsModel.removeCache(localPath, true);
        }

        onJobQueueStatusSignal: {
            if (pageStack) {
                var p = findPage("settingPage");
                if (p) {
                    p.updateCloudDriveItemCount(itemCount);
                }

                // Update job queue count on current page.
                pageStack.find(function (page) {
                    if (page.updateJobQueueCount) page.updateJobQueueCount(runningJobCount, jobQueueCount);
                });
            }
        }

        onRefreshRequestSignal: {
            console.debug("window cloudDriveModel onRefreshRequestSignal " + nonce);

            if (nonce != "") {
                // Update item after removing job as isSyncing will check if job exists.
                var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

                // Remove finished job.
                cloudDriveModel.removeJob("cloudDriveModel.onRefreshRequestSignal", jobJson.job_id);

                // Update ProgressBar on listItem and its parents. Needs to update after removeJob as isSyncing check if job exists.
                pageStack.find(function (page) {
                    if (page.updateItemSlot) page.updateItemSlot(jobJson, "window cloudDriveModel onRefreshRequestSignal");
                });
            } else {
                // For refresh request without specified nonce(jobId), just refresh.
                var p = findPage("folderPage");
                if (p) {
                    p.refreshSlot("cloudDriveModel onRefreshRequestSignal");
                }
            }
        }

        onMoveToTrashRequestSignal: {
            console.debug("window cloudDriveModel onMoveToTrashRequestSignal " + nonce + " " + localPath);

            if (nonce != "") {
                // Update item after removing job as isSyncing will check if job exists.
                var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

                fsModel.trash(localPath);

                // Remove finished job.
                cloudDriveModel.removeJob("cloudDriveModel.onMoveToTrashRequestSignal", jobJson.job_id);
            }
        }

        onCopyFileReplySignal: {
            console.debug("window cloudDriveModel onCopyFileReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            if (err == 0) {
                // Cloud items have been managed by CloudDriveModel::copyFileReplyFilter.

                // Refresh cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.refreshSlot("cloudDriveModel onCopyFileReplySignal");
                }
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid, jobJson.job_id);
                return;
            } else {
                logError(getCloudName(jobJson.type) + " " + qsTr("Copy"),
                         qsTr("Error") + " " + err + " " + errMsg + " " + msg);

                // Reset cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.resetBusySlot("cloudDriveModel onCopyFileReplySignal");
                }
            }

            // Remove finished job.
            cloudDriveModel.removeJob("cloudDriveModel.onCopyFileReplySignal", jobJson.job_id);

            // Update ProgressBar on NEW listItem and its parents. Needs to update after removeJob as isSyncing check if job exists.
            pageStack.find(function (page) {
                if (page.updateItemSlot) page.updateItemSlot(jobJson);
            });
        }

        onMoveFileReplySignal: {
            console.debug("window cloudDriveModel onMoveFileReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            if (err == 0) {
                // Cloud items have been managed by CloudDriveModel::moveFileReplyFilter.

                // Refresh cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.refreshSlot("cloudDriveModel onMoveFileReplySignal");
                }
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid, jobJson.job_id);
                return;
            } else {
                logError(getCloudName(jobJson.type) + " " + qsTr("Move"),
                         qsTr("Error") + " " + err + " " + errMsg + " " + msg);

                // Reset cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.resetBusySlot("cloudDriveModel onMoveFileReplySignal");
                }
            }

            // Remove finished job.
            cloudDriveModel.removeJob("cloudDriveModel.onMoveFileReplySignal", jobJson.job_id);

            // Update ProgressBar on listItem and its parents. Needs to update after removeJob as isSyncing check if job exists.
            pageStack.find(function (page) {
                if (page.updateItemSlot) page.updateItemSlot(jobJson);
            });
        }

        onDeleteFileReplySignal: {
            console.debug("window cloudDriveModel onDeleteFileReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            if (err == 0) {
                // Refresh cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.refreshSlot("cloudDriveModel onDeleteFileReplySignal");
                }
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid, jobJson.job_id);
                return;
            } else {
                logError(getCloudName(jobJson.type) + " " + qsTr("Delete"),
                         qsTr("Error") + " " + err + " " + errMsg + " " + msg);

                // Reset cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.resetBusySlot("cloudDriveModel onDeleteFileReplySignal");
                }
            }

            // Remove finished job.
            cloudDriveModel.removeJob("cloudDriveModel.onDeleteFileReplySignal", jobJson.job_id);

            // Update ProgressBar on listItem and its parents. Needs to update after removeJob as isSyncing check if job exists.
            pageStack.find(function (page) {
                if (page.updateItemSlot) page.updateItemSlot(jobJson);
            });

            var p = findPage("folderPage");
            if (p) {
                // Refresh cloudDrivePathDialog if it's opened.
                p.updateCloudDrivePathDialogSlot();
            }
        }

        onShareFileReplySignal: {
            console.debug("window cloudDriveModel onShareFileReplySignal " + nonce + " " + err + " " + errMsg + " " + msg + " " + url + " " + expires);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            if (err == 0) {
                // Show share UI. For Meego only.
                helper.shareUrl(url,
                                qsTr("Share file on %1").arg(cloudDriveModel.getCloudName(jobJson.type)),
                                qsTr("Please download file with below link."));
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid, jobJson.job_id);
                return;
            } else {
                logError(getCloudName(jobJson.type) + " " + qsTr("Share"),
                         qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }

            // Remove finished job.
            cloudDriveModel.removeJob("cloudDriveModel.onShareFileReplySignal", jobJson.job_id);

            // Update ProgressBar on listItem and its parents. Needs to update after removeJob as isSyncing check if job exists.
            pageStack.find(function (page) {
                if (page.updateItemSlot) page.updateItemSlot(jobJson);
            });
        }

        onDeltaReplySignal: {
            console.debug("window cloudDriveModel onDeltaReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            if (err == 0) {
                // TODO
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid, jobJson.job_id);
                return;
            } else {
                logError(getCloudName(jobJson.type) + " " + qsTr("Delta"),
                         qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }

            // Remove finished job.
            cloudDriveModel.removeJob("cloudDriveModel.onDeltaReplySignal", jobJson.job_id);

            // Update ProgressBar on listItem and its parents. Needs to update after removeJob as isSyncing check if job exists.
            pageStack.find(function (page) {
                if (page.updateItemSlot) page.updateItemSlot(jobJson);
            });
        }

        onMigrateFileReplySignal: {
            console.debug("window cloudDriveModel onMigrateFileReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            if (err == 0) {
                // Do nothing.
            } else if (err == 203) {
                // Do nothing.
            } else {
                logError(getCloudName(jobJson.type) + " " + qsTr("Migrate"),
                         qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }

            // Remove finished job.
            cloudDriveModel.removeJob("cloudDriveModel.onMigrateFileReplySignal", jobJson.job_id);

            // Update ProgressBar on listItem and its parents. Needs to update after removeJob as isSyncing check if job exists.
            pageStack.find(function (page) {
                if (page.updateItemSlot) page.updateItemSlot(jobJson);
            });
        }

        onMigrateFilePutReplySignal: {
            console.debug("window cloudDriveModel onMigrateFilePutReplySignal " + nonce + " " + err + " " + errMsg + " " + msg + " " + errorOnTarget);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            if (err == 0) {
                // Refresh cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.refreshItemAfterFilePutSlot(jobJson);
                }
            } else if (err == 204) { // Refresh token
                if (errorOnTarget) {
                    cloudDriveModel.refreshToken(jobJson.target_type, jobJson.target_uid, jobJson.job_id);
                } else {
                    cloudDriveModel.refreshToken(jobJson.type, jobJson.uid, jobJson.job_id);
                }
                return;
            } else {
                if (errorOnTarget) {
                    logError(getCloudName(jobJson.target_type) + " " + qsTr("Migrate"),
                             qsTr("Error") + " " + err + " " + errMsg + " " + msg);
                } else {
                    logError(getCloudName(jobJson.type) + " " + qsTr("Migrate"),
                             qsTr("Error") + " " + err + " " + errMsg + " " + msg);
                }
            }

            // Remove finished job.
            // TODO Remove only success job.
            if (err == 0) {
                cloudDriveModel.removeJob("cloudDriveModel.onMigrateFilePutReplySignal", jobJson.job_id);
            }

            // Update ProgressBar on listItem and its parents. Needs to update after removeJob as isSyncing check if job exists.
            pageStack.find(function (page) {
                if (page.updateItemSlot) page.updateItemSlot(jobJson);
            });
        }

        onJobEnqueuedSignal: {
            // Get job json.
            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));
            cloudDriveJobsModel.append(jobJson);

            console.debug("window cloudDriveModel onJobEnqueuedSignal " + nonce + " " + cloudDriveModel.getOperationName(jobJson.operation) + " localPath " + localPath);

            // Update cloud list item on relavant pages.
            pageStack.find(function (page) {
                if (page.updateItemSlot) page.updateItemSlot(jobJson, "cloudDriveModel onJobEnqueuedSignal");
            });
        }

        onJobUpdatedSignal: {
            // NOTE It's emitted from updateJob() to update job in job model.
            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));
            cloudDriveJobsModel.updateJob(jobJson);

//            console.debug("window cloudDriveModel onJobUpdatedSignal " + nonce + " " + cloudDriveModel.getOperationName(jobJson.operation));

            // Update cloud list item on relavant pages.
            pageStack.find(function (page) {
                if (page.updateItemSlot) page.updateItemSlot(jobJson, "cloudDriveModel onJobUpdatedSignal");
            });
        }

        onJobRemovedSignal: {
            // NOTE It's emitted from removeJob() to remove job from job model.
            cloudDriveJobsModel.removeJob(nonce);
        }

        onMigrateProgressSignal: {
            var p = findPage("folderPage");
            if (p) {
                p.updateMigrationProgressSlot(type, uid, localFilePath, remoteFilePath, count, total);
            }
        }

        onCacheImageFinished: {
            console.debug("window cloudDriveModel onCacheImageFinished " + absoluteFilePath + " " + err + " " + errMsg + " caller " + caller);
            // Refresh item only if there is no error.
            if (err == 0) {
                var p = findPage(caller);
                if (p && p.refreshItem) {
                    p.refreshItem(absoluteFilePath);
                }
            }
        }

        onLogRequestSignal: {
            if (logType == "error") {
                logError(titleText, message, autoCloseInterval);
            } else if (logType == "warn") {
                logWarn(titleText, message, autoCloseInterval);
            } else if (logType == "info") {
                logInfo(titleText, message, autoCloseInterval);
            }
        }

        onRefreshItemAfterFileGetSignal: {
            console.debug("window cloudDriveModel onRefreshItemAfterFileGetSignal nonce " + nonce + " localPath " + localPath);
            var p = findPage("folderPage");
            if (p) {
                p.refreshItemAfterFileGetSlot(localPath);
            }
        }

        Component.onCompleted: {
            console.debug(Utility.nowText() + " window cloudDriveModel onCompleted");
            window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("CloudDriveModel"), 0.1);

            // Proceeds queued jobs during constructions.
            cloudDriveModel.resumeNextJob();
        }
    }

    ListModel {
        id: cloudDriveJobsModel

        // TODO Cache found index. But most running jobs usually be on prior index (queue).
        function findIndexByJobId(jobId) {
            for (var i=0; i<cloudDriveJobsModel.count; i++) {
                if (cloudDriveJobsModel.get(i).job_id == jobId) {
//                        console.debug("cloudDriveJobsModel findIndexByJobId " + i + " " + cloudDriveJobsModel.get(i).job_id);
                    return i;
                }
            }

            return -1;
        }

        function updateJob(jobJson) {
            if (!jobJson) return;

            var i = findIndexByJobId(jobJson.job_id);
            if (i >= 0) {
                cloudDriveJobsModel.set(i, jobJson);
            }
        }

        function updateJobProgressBar(jobJson) {
            if (!jobJson) return;

            var i = findIndexByJobId(jobJson.job_id);
            if (i >= 0) {
                cloudDriveJobsModel.setProperty(i, "bytes", jobJson.bytes);
            }
        }

        function removeJob(jobId) {
            var i = findIndexByJobId(jobId);
            if (i >= 0) {
                cloudDriveJobsModel.remove(i);
            }

            return i;
        }

        function removeQueuedJobs() {
            var i = 0;
            while (i<cloudDriveJobsModel.count) {
                var job = cloudDriveJobsModel.get(i);
                if (!job.is_running) {
                    console.debug("cloudDriveJobsModel removeQueuedJobs " + i + " " + job.job_id);
                    cloudDriveModel.removeJob("window cloudDriveJobsModel removeQueuedJobs", job.job_id);
                }
            }
        }
    }

    ListModel {
        id: cloudDriveAccountsModel

        function parseCloudDriveAccountsModel(replicatedModel) {
            // Check if authorized before parsing.
            if (!cloudDriveModel.isAuthorized()) return;

            // Clear model.
            cloudDriveAccountsModel.clear();

            // Get uid list.
            var dbUidList = cloudDriveModel.getStoredUidList();

            for (var i=0; i<dbUidList.length; i++)
            {
                var json = JSON.parse(dbUidList[i]);
                var cloudType = cloudDriveModel.getClientType(json.type);
                console.debug("window cloudDriveAccountsModel parseCloudDriveAccountsModel type " + json.type + " " + cloudType + " i " + i + " uid " + json.uid + " email " + json.email);
                var modelItem = {
                    logicalDrive: json.email,
                    availableSpace: 0,
                    totalSpace: -1,
                    driveType: 7, // Cloud Drive in driveGrid.
                    name: "",
                    cloudDriveType: cloudType,
                    iconSource: cloudDriveModel.getCloudIcon(cloudType),
                    uid: json.uid,
                    email: json.email,
                    token: json.token,
                    secret: json.secret,
                    type: json.type
                };

                cloudDriveAccountsModel.append(modelItem);
                if (replicatedModel) {
                    replicatedModel.append(modelItem);
                }

                // Request quota.
                cloudDriveModel.quota(modelItem.cloudDriveType, modelItem.uid);
            }
        }

        function updateAccountInfoSlot(type, uid, name, email, shared, normal, quota) {
    //        console.debug("drivePage updateAccountInfoSlot uid " + uid + " type " + type + " shared " + shared + " normal " + normal + " quota " + quota);
            for (var i=0; i<cloudDriveAccountsModel.count; i++) {
                var item = cloudDriveAccountsModel.get(i);
    //            console.debug("drivePage updateAccountInfoSlot item i " + i + " uid " + item.uid + " driveType " + item.driveType + " cloudDriveType " + item.cloudDriveType);
                if (item.uid == uid && item.driveType == 7 && item.cloudDriveType == type) {
                    console.debug("window cloudDriveAccountsModel updateAccountInfoSlot found item i " + i + " uid " + item.uid + " driveType " + item.driveType + " cloudDriveType " + item.cloudDriveType);
                    cloudDriveAccountsModel.set(i, { availableSpace: (quota - shared - normal), totalSpace: quota });
                    if (email) {
                        cloudDriveAccountsModel.set(i, { logicalDrive: email, email: email });
                    }
                    if (name) {
                        cloudDriveAccountsModel.set(i, { name: name });
                    }
                }
            }
        }
    }

    BookmarksModel {
        id: bookmarksModel

        onAddErrorSignal: {
            logError(qsTr("Add bookmark"), qsTr("Bookmark cannot be added. (%1 %2)").arg(type).arg(msg));
        }
    }

    MessageDialog {
        id: messageDialog
    }

    MessageLoggerModel {
        id: messageLoggerModel
    }

    function logError(titleText, message, autoCloseInterval) {
        // Append to logger.
        messageLoggerModel.log("error", titleText, message);

        // Show alert.
        showMessageDialogSlot(titleText, message, autoCloseInterval);
    }

    function logWarn(titleText, message, autoCloseInterval) {
        // Append to logger.
        messageLoggerModel.log("warn", titleText, message);
    }

    function logInfo(titleText, message, autoCloseInterval) {
        // Append to logger.
        messageLoggerModel.log("info", titleText, message);

        // Show alert.
        showMessageDialogSlot(titleText, message, autoCloseInterval);
    }

    function showMessageDialogSlot(titleText, message, autoCloseInterval) {
        if (autoCloseInterval) {
            messageDialog.autoClose = true;
            messageDialog.autoCloseInterval = autoCloseInterval;
        } else {
            messageDialog.autoClose = false;
            messageDialog.autoCloseInterval = 0;
        }
        messageDialog.titleText = appInfo.emptyStr+titleText;
        messageDialog.message = appInfo.emptyStr+message;
        messageDialog.open();
    }

    UploadProgressDialog {
        id: uploadProgressDialog
    }

    DownloadProgressDialog {
        id: downloadProgressDialog
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
            gcpClient.selectedFilePath = "";
            gcpClient.selectedURL = "";
        }
    }

    function showPrinterSelectionDialog(srcFilePath, srcUrl) {
        printerSelectionDialog.srcFilePath = srcFilePath;
        printerSelectionDialog.srcURL = srcUrl;
        printerSelectionDialog.open();
    }

    Rectangle {
        id: busyPanel
        x: 0
        y: 0
        width: parent.width
        height: parent.height
        color: "black"
        visible: false
        smooth: false
        opacity: 0

        BusyIndicator {
            id: busyindicator1
            style: BusyIndicatorStyle { size: "large" }
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            running: false
            visible: true
        }

        Text {
            id: busyText1
            width: 180
            text: appInfo.emptyStr+qsTr("Please wait while loading.")
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pointSize: 8
            anchors.bottom: busyindicator1.top
            anchors.horizontalCenter: parent.horizontalCenter
            color: "white"
        }
    }

    transitions: [
        Transition {
            from: "ready"
            to: "busy"
            NumberAnimation {
                target: busyPanel
                property: "opacity"
                duration: 300
                to: 0.7
                easing.type: Easing.Linear
            }
        },
        Transition {
            from: "busy"
            to: "ready"
            NumberAnimation {
                target: busyPanel
                property: "opacity"
                duration: 300
                to: 0
                easing.type: Easing.Linear
            }
        }
    ]

    Splash {
        id: splashScreen
        interval: 3000

        onLoaded: {
            // Set theme.inverted = true -> black theme.
            theme.inverted = appInfo.getSettingBoolValue("Theme.inverted", true);
            appInfo.setSettingValue("Theme.inverted", theme.inverted);

            // Set timer to push pages later after shows splash screen.
            pushPagesTimer.start();
        }
    }

    Timer {
        id: pushPagesTimer
        interval: 200
        running: false
        onTriggered: {
            // TODO PageStack pushs page in blocking mode. Push returns once page is completed.
            console.debug(Utility.nowText() + " window pushPagesTimer push drivePage");
            pageStack.push(Qt.resolvedUrl("DrivePage.qml"));
        }
    }

    Component.onCompleted: {
        console.debug(Utility.nowText() + " window onCompleted");
        window.updateLoadingProgressSlot(qsTr("Loading"), 0.5);

        // Set to portrait to show splash screen. Then it will set back to default once it's destroyed.
        screen.allowedOrientations = Screen.Portrait;

        // Connect activation signal to slot.
        platformWindow.activeChanged.connect(activateSlot);
    }

    function activateSlot() {
        console.debug("window activateSlot platformWindow.active " + platformWindow.active);
        if (platformWindow.active) {
            var p = findPage("folderPage");
            if (p) p.activateSlot();
        }
    }
}
