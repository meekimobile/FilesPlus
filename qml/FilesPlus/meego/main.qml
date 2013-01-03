import QtQuick 1.1
import QtMobility.contacts 1.1
import com.nokia.meego 1.0
import AppInfo 1.0
import ClipboardModel 1.0
import GCPClient 1.0
import CloudDriveModel 1.0
import FolderSizeItemListModel 1.0
import MessageClient 1.0
import "Utility.js" as Utility

PageStackWindow {
    id: window
    showStatusBar: true
    showToolBar: true

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
        if (fsModel.isRunning()) {
            messageDialog.titleText = appInfo.emptyStr+qsTr("Notify");
            messageDialog.message = appInfo.emptyStr+qsTr("Reset Cache is running. Please wait until it's done.");
            messageDialog.open();
        } else {
            Qt.quit();
        }
    }

    AppInfo {
        id: appInfo
        domain: "MeekiMobile"
        app: "FilesPlus"

        Component.onCompleted: {
            appInfo.startMonitoring();
        }
        onLocaleChanged: {
            console.debug("appInfo onLocaleChanged");
            var p = findPage("folderPage");
            if (p) p.requestJobQueueStatusSlot();
        }
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
                showMessageDialogSlot(getActionName(fileAction) + " " + qsTr("error"),
                                      msg,
                                      5000);

                if (copyProgressDialog.status == DialogStatus.Open) {
                    copyProgressDialog.message = getActionName(fileAction) + " " + sourcePath + " " + qsTr("failed") + ".\n";
                }

                // TODO stop queued jobs
                fsModel.cancelQueuedJobs();
            }

            // TODO Connect to cloud if copied/moved file/folder is in connected folder.
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
            if (err < 0) {
                showMessageDialogSlot(getActionName(fileAction) + " " + qsTr("error"),
                                      msg,
                                      5000);

                if (deleteProgressDialog.status == DialogStatus.Open) {
                    deleteProgressDialog.message = getActionName(fileAction) + " " + sourcePath + " " + qsTr("failed") + ".\n";
                }

                // TODO stop queued jobs
                fsModel.cancelQueuedJobs();
            }

            // Delete file from clouds.
            if (err == 0) {
                if (cloudDriveModel.isConnected(sourcePath)) {
                    var json = Utility.createJsonObj(cloudDriveModel.getItemListJson(sourcePath));
                    for (var i=0; i<json.length; i++) {
                        cloudDriveModel.deleteFile(json[i].type, json[i].uid, json[i].local_path, json[i].remote_path);
                    }
                }

                // Reset cloudDriveModel hash on parent. CloudDriveModel will update with actual hash once it got reply.
                var paths = fsModel.getPathToRoot(sourcePath);
                for (var i=1; i<paths.length; i++) {
//                    console.debug("fsModel onDeleteFinished updateItems paths[" + i + "] " + paths[i]);
                    cloudDriveModel.updateItems(paths[i], cloudDriveModel.dirtyHash);
                }
            }
        }

        onCreateFinished: {
            console.debug("fsModel onCreateFinished " + targetPath);

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
        }

        onRenameFinished: {
            console.debug("fsModel onRenameFinished sourcePath " + sourcePath + " targetPath " + targetPath + " err " + err + " msg " + msg);

            // Rename file on clouds.
            if (err == 0 && cloudDriveModel.isConnected(sourcePath)) {
//                console.debug("fsModel onRenameFinished itemList " + cloudDriveModel.getItemListJson(sourcePath));
                var json = Utility.createJsonObj(cloudDriveModel.getItemListJson(sourcePath));
                for (var i=0; i<json.length; i++) {
                    console.debug("fsModel onRenameFinished item " + json[i].type + " " + json[i].uid + " " + json[i].local_path + " " + json[i].remote_path);
                    cloudDriveModel.moveFile(json[i].type, json[i].uid, sourcePath, json[i].remote_path, targetPath, json[i].remote_path, fsModel.getFileName(targetPath));
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

        Component.onCompleted: {
            console.debug(Utility.nowText() + " window fsModel onCompleted");
            window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("FolderModel"), 0.1);

            // Proceeds queued jobs during constructions.
            fsModel.proceedNextJob();
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
            cloudDriveModel.requestJobQueueStatus();
        }
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
            fsModel.resumeNextJob();
        }
    }

    ProgressDialog {
        id: deleteProgressDialog
        autoClose: false
        titleText: appInfo.emptyStr+qsTr("Deleting")
        onOk: {
            // Refresh view after copied/moved.
            fsModel.refreshDir("deleteProgressDialog onOk");
        }
        onCancel: {
            fsModel.cancelQueuedJobs();
            // Abort thread with rollbackFlag=false.
            fsModel.abortThread(false);
            // Refresh view after copied/moved.
            fsModel.refreshDir("deleteProgressDialog onCancel");
        }
        onOpened: {
            fsModel.resumeNextJob();
        }
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
            console.debug(Utility.nowText() + " clipboard additem json " + json);
            clipboard.addClipboardItem(json.sourcePath, Utility.createJsonText(json) );
        }

        function addItemWithSuppressCountChanged(json) {
            console.debug(Utility.nowText() + " clipboard addItemWithSuppressCountChanged json " + json);
            clipboard.addClipboardItem(json.sourcePath, Utility.createJsonText(json), true);
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
            var model = Qt.createQmlObject('import QtQuick 1.1; ListModel {}', favContactModel);

            console.debug("getFavListModel favContactModel.contacts.length " + favContactModel.contacts.length + " error " + favContactModel.error);
            for (var i=0; i<favContactModel.contacts.length; i++) {
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

            model.append({
                             displayLabel: qsTr("Other recipient"),
                             email: "",
                             phoneNumber: "",
                             favorite: false
                         });

            return model;
        }

        Component.onCompleted: {
            console.debug(Utility.nowText() + " folderPage favContactModel onCompleted");
            window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("ContactModel"), 0.1);
        }
    }

    MessageClient {
        id: msgClient
    }

    GCPClient {
        id: gcpClient

        property string selectedFilePath
        property string selectedURL
        property alias printerModel: gcpPrinterModel

        ListModel {
            id: gcpPrinterModel
        }

        function setGCPClientAuthCode(code) {
            var res = gcpClient.parseAuthorizationCode(code);
            if (res) {
                gcpClient.accessToken();
            }
        }

        function getPrinterListModel() {
            var printerList = gcpClient.getStoredPrinterList();
            console.debug("gcpClient.getPrinterListModel() " + printerList);

            gcpPrinterModel.clear();
            for (var i=0; i<printerList.length; i++)
            {
                gcpPrinterModel.append({
                                 name: printerList[i]
                             });
            }

            console.debug("gcpClient.getPrinterListModel() gcpPrinterModel.count " + gcpPrinterModel.count);
            return gcpPrinterModel;
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
                    showPrinterSelectionDialog(srcFilePath, "");
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
                var printerListModel = gcpClient.getPrinterListModel();
                console.debug("folderPage printFileSlot printerListModel.count=" + printerListModel.count);
                if (printerListModel.count > 0) {
                    showPrinterSelectionDialog("", url);
                } else {
                    // TODO Open progress dialog.
                    downloadProgressDialog.titleText = appInfo.emptyStr+qsTr("Search for printers");
                    downloadProgressDialog.indeterminate = true;
                    downloadProgressDialog.open();
                    gcpClient.search("");
                }
            }
        }

        function resetCloudPrintSlot() {
//            popupToolPanel.selectedFilePath = "";
//            popupToolPanel.selectedFileIndex = -1;
            gcpClient.authorize();
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

                    gcpClient.jobs("");
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

            gcpClient.jobs("");
        }

        Component.onCompleted: {
            console.debug(Utility.nowText() + " folderPage gcpClient onCompleted");
            window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("GCPClient"), 0.1);
        }
    }

    CloudDriveModel {
        id: cloudDriveModel
        dropboxFullAccess: appInfo.emptySetting+appInfo.getSettingBoolValue("dropbox.fullaccess.enabled", false)

        property alias jobsModel: cloudDriveJobsModel
        property string shareFileCaller

        ListModel {
            id: cloudDriveJobsModel

            function findIndexByJobId(jobId) {
                for (var i=0; i<cloudDriveJobsModel.count; i++) {
                    if (cloudDriveJobsModel.get(i).job_id == jobId) {
//                        console.debug("cloudDriveJobsModel findIndexByJobId " + i + " " + cloudDriveJobsModel.get(i).job_id);
                        return i;
                    }
                }

                return -1;
            }

            function removeJob(jobId) {
                for (var i=0; i<cloudDriveJobsModel.count; i++) {
                    if (cloudDriveJobsModel.get(i).job_id == jobId) {
                        cloudDriveJobsModel.remove(i);
                        return i;
                    }
                }

                return -1;
            }
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
                var localHash = cloudDriveModel.getItemHash(localPath, getClientType(json.type), json.uid);
                console.debug("window cloudDriveModel getUidListModel i " + i + " type " + json.type + " uid " + json.uid + " email " + json.email + " localHash " + localHash);
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

        function getClientType(typeText) {
            if (["dropboxclient","dropbox"].indexOf(typeText.toLowerCase()) != -1) {
                return CloudDriveModel.Dropbox;
            } else if (["skydriveclient","skydrive"].indexOf(typeText.toLowerCase()) != -1) {
                return CloudDriveModel.SkyDrive;
            } else if (["gcdclient","googledriveclient","googledrive"].indexOf(typeText.toLowerCase()) != -1) {
                return CloudDriveModel.GoogleDrive;
            } else if (["ftpclient","ftp"].indexOf(typeText.toLowerCase()) != -1) {
                return CloudDriveModel.Ftp;
            }

            return -1;
        }

        function accessTokenSlot(clientTypeName, pin) {
            console.debug("folderPage accessTokenSlot clientTypeName " + clientTypeName + " pin " + pin);
            var clientType = getClientType(clientTypeName);
            if (pin == "" || pin == "PinNotFound") {
                messageDialog.titleText =  appInfo.emptyStr+getCloudName(clientType)+" "+qsTr("Access Token");
                messageDialog.message = appInfo.emptyStr+qsTr("Error") + " " + qsTr("PIN code is not found.");
                messageDialog.open();
            } else {
                cloudDriveModel.accessToken(clientType, pin);
            }
        }

        function parseCloudDriveMetadataJson(selectedCloudType, originalRemotePath, jsonText,  model) {
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

            // TODO Generalize
            switch (selectedCloudType) {
            case CloudDriveModel.Dropbox:
                remoteParentPath = json.path;
                remoteParentPathName = json.path;
                remoteParentParentPath = cloudDriveModel.getParentRemotePath(selectedCloudType, remoteParentPath);
                for (var i=0; i<json.contents.length; i++) {
                    var item = json.contents[i];
                    var modelItem = { "name": cloudDriveModel.getRemoteName(selectedCloudType, item.path), "absolutePath": item.path,
                        "isChecked": false, "source": "", "thumbnail": "", "fileType": cloudDriveModel.getFileType(item.path), "isRunning": false, "runningOperation": "", "runningValue": 0, "runningMaxValue": 0,
                        "subDirCount": 0, "subFileCount": 0, "isDirty": false,
                        "lastModified": (new Date(item.modified)), "size": item.bytes, "isDir": item.is_dir};
                    model.append(modelItem);
                    if (modelItem.absolutePath == originalRemotePath) {
                        selectedIndex = i;
                        selectedRemotePath = modelItem.absolutePath;
                        selectedRemotePathName = modelItem.name;
                        selectedIsDir = modelItem.isDir;
                    }
                }
                break;
            case CloudDriveModel.SkyDrive:
                remoteParentPath = json.property.id;
                remoteParentPathName = json.property.name;
                remoteParentParentPath = (json.property.parent_id ? json.property.parent_id : "");
                for (var i=0; i<json.data.length; i++) {
                    var item = json.data[i];
                    var modelItem = { "name": item.name, "absolutePath": item.id,
                        "isChecked": false, "source": (item.source ? item.source : ""), "thumbnail": (item.picture ? item.picture : ""), "fileType": cloudDriveModel.getFileType(item.name), "isRunning": false, "runningOperation": "", "runningValue": 0, "runningMaxValue": 0,
                        "subDirCount": 0, "subFileCount": 0, "isDirty": false,
                        "lastModified": Utility.parseJSONDate(item.updated_time), "size": item.size, "isDir": (item.type == "folder" || item.type == "album") };
                    model.append(modelItem);
                    if (modelItem.absolutePath == originalRemotePath) {
                        selectedIndex = i;
                        selectedRemotePath = modelItem.absolutePath;
                        selectedRemotePathName = modelItem.name;
                        selectedIsDir = modelItem.isDir;
                    }
                }
                break;
            case CloudDriveModel.GoogleDrive:
                remoteParentPath = json.property.id;
                remoteParentPathName = json.property.title;
                remoteParentParentPath = ((json.property.parents && json.property.parents.length > 0) ? json.property.parents[0].id : "");
                for (var i=0; i<json.items.length; i++) {
                    var item = json.items[i];
                    var modelItem = { "name": item.title, "absolutePath": item.id,
                        "isChecked": false, "source": (item.webContentLink ? item.webContentLink : ""), "thumbnail": (item.thumbnailLink ? item.thumbnailLink : ""), "fileType": (item.fileExtension ? item.fileExtension : ""), "isRunning": false, "runningOperation": "", "runningValue": 0, "runningMaxValue": 0,
                        "subDirCount": 0, "subFileCount": 0, "isDirty": false,
                        "lastModified": Utility.parseJSONDate(item.modifiedDate), "size": (item.fileSize ? item.fileSize : 0), "isDir": (item.mimeType == "application/vnd.google-apps.folder") };
                    model.append(modelItem);
                    if (modelItem.absolutePath == originalRemotePath) {
                        selectedIndex = i;
                        selectedRemotePath = modelItem.absolutePath;
                        selectedRemotePathName = modelItem.name;
                        selectedIsDir = modelItem.isDir;
                    }
                }
                break;
            case CloudDriveModel.Ftp:
                remoteParentPath = json.property.path;
                remoteParentPathName = json.property.path;
                remoteParentParentPath = cloudDriveModel.getParentRemotePath(selectedCloudType, remoteParentPath);
                for (var i=0; i<json.data.length; i++) {
                    var item = json.data[i];
                    var modelItem = { "name": item.name, "absolutePath": item.path,
                        "isChecked": false, "source": "", "thumbnail": "", "fileType": cloudDriveModel.getFileType(item.name), "isRunning": false, "runningOperation": "", "runningValue": 0, "runningMaxValue": 0,
                        "subDirCount": 0, "subFileCount": 0, "isDirty": false,
                        "lastModified": Utility.parseJSONDate(item.lastModified), "size": item.size, "isDir": item.isDir };
                    model.append(modelItem);
                    if (modelItem.absolutePath == originalRemotePath) {
                        selectedIndex = i;
                        selectedRemotePath = modelItem.absolutePath;
                        selectedRemotePathName = modelItem.name;
                        selectedIsDir = modelItem.isDir;
                    }
                }
                break;
            }

            var responseJson = { "selectedIndex": selectedIndex,
                "selectedRemotePath": selectedRemotePath, "selectedRemotePathName": selectedRemotePathName, "selectedIsDir": selectedIsDir,
                "remoteParentPath": remoteParentPath, "remoteParentPathName": remoteParentPathName, "remoteParentParentPath": remoteParentParentPath
            };

            return responseJson;
        }

        onRequestTokenReplySignal: {
            console.debug("window cloudDriveModel onRequestTokenReplySignal " + err + " " + errMsg + " " + msg);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                cloudDriveModel.authorize(jobJson.type);
            } else {
                showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("Request Token"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }
        }

        onAuthorizeRedirectSignal: {
            console.debug("window cloudDriveModel onAuthorizeRedirectSignal " + url + " redirectFrom " + redirectFrom);

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            pageStack.push(Qt.resolvedUrl("AuthPage.qml"), { url: url, redirectFrom: redirectFrom }, true);
        }

        onAccessTokenReplySignal: {
            console.debug("window cloudDriveModel onAccessTokenReplySignal " + err + " " + errMsg + " " + msg);

            var jobJson = JSON.parse(cloudDriveModel.getJobJson(nonce));

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                // TODO find better way to proceed after got accessToken.
//                if (popupToolPanel.selectedFilePath) {
//                    uidDialog.proceedPendingOperation();
//                    syncFileSlot(popupToolPanel.selectedFilePath, popupToolPanel.selectedFileIndex);
//                } else {
                    // TODO Get account info and show in dialog.
                    showMessageDialogSlot(getCloudName(jobJson.type) + " " + qsTr("Access Token"),
                                          qsTr("CloudDrive user is authorized.\nPlease proceed your sync action."),
                                          2000);

                    // Refresh account page.
                    var p = findPage("cloudDriveAccountsPage");
                    if (p) p.refreshCloudDriveAccountsSlot();
//                }
            } else {
                showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("Access Token"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }
        }

        onAccountInfoReplySignal: {
            console.debug("window cloudDriveModel onAccountInfoReplySignal " + err + " " + errMsg + " " + msg);

            var jobJson = JSON.parse(cloudDriveModel.getJobJson(nonce));

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                // Do nothing.
            } else if (err == 204) {
                // TODO Refactor to support all SkyDriveClient services.
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid);
            } else {
                showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("Account Info"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }
        }

        onQuotaReplySignal: {
            console.debug("window cloudDriveModel onQuotaReplySignal " + err + " " + errMsg + " " + msg);

            var jobJson = JSON.parse(cloudDriveModel.getJobJson(nonce));

            // TODO Don't remove job here to support requeue by refreshToken()

            if (err == 0) {
                var jsonObj = Utility.createJsonObj(msg);

                // Send info to currentPage.
                if (pageStack.currentPage.updateAccountInfoSlot) {
                    switch (jobJson.type) {
                    case CloudDriveModel.Dropbox:
                        // Send info to currentPage.
                        var sharedValue = (jsonObj.quota_info && jsonObj.quota_info.shared) ? jsonObj.quota_info.shared : 0;
                        var normalValue = (jsonObj.quota_info && jsonObj.quota_info.normal) ? jsonObj.quota_info.normal : 0;
                        var quotaValue = (jsonObj.quota_info && jsonObj.quota_info.quota) ? jsonObj.quota_info.quota : 0;
                        pageStack.currentPage.updateAccountInfoSlot(jobJson.type, jsonObj.uid, jsonObj.name, jsonObj.email,
                                                                    sharedValue,
                                                                    normalValue,
                                                                    quotaValue);
                        break;
                    case CloudDriveModel.SkyDrive:
                        pageStack.currentPage.updateAccountInfoSlot(jobJson.type, jobJson.uid, "", cloudDriveModel.getUidEmail(jobJson.type, jobJson.uid),
                                                                    0,
                                                                    jsonObj.quota-jsonObj.available,
                                                                    jsonObj.quota);
                        break;
                    case CloudDriveModel.GoogleDrive:
                        pageStack.currentPage.updateAccountInfoSlot(jobJson.type, jobJson.uid, "", cloudDriveModel.getUidEmail(jobJson.type, jobJson.uid),
                                                                    0,
                                                                    jsonObj.quotaBytesUsed,
                                                                    jsonObj.quotaBytesTotal);
                        break;
                    default:
                        pageStack.currentPage.updateAccountInfoSlot(jobJson.type, jobJson.uid, "", cloudDriveModel.getUidEmail(jobJson.type, jobJson.uid),
                                                                    0,
                                                                    0,
                                                                    -1);
                    }
                }
            } else if (err == 204) {
                // TODO Refactor to support all SkyDriveClient services.
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid, nonce);
                return;
            } else {
                showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("Account Quota"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg,
                            2000);
            }

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);
        }

        onFileGetReplySignal: {
            console.debug("window cloudDriveModel onFileGetReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jsonText = cloudDriveModel.getJobJson(nonce);
//            console.debug("window cloudDriveModel jsonText " + jsonText);

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            var jobJson = JSON.parse(jsonText);

            if (err == 0) {
                // Remove cache on target folders and its parents.
                var p = findPage("folderPage");
                if (p) {
                    p.refreshItemAfterFileGetSlot(jobJson.local_file_path);
                }
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid);
            } else {
                showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("File Get"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }

            // Update ProgressBar on listItem and its parents.
            pageStack.find(function (page) {
                if (page.updateItemSlot) page.updateItemSlot(jobJson);
            });
        }

        onFilePutReplySignal: {
            console.debug("window cloudDriveModel onFilePutReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jsonText = cloudDriveModel.getJobJson(nonce);
//            console.debug("window cloudDriveModel onFilePutReplySignal jsonText " + jsonText);

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            var jobJson = JSON.parse(jsonText);

            if (err == 0) {
                // Refresh cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.refreshSlot("cloudDriveModel onFilePutReplySignal");
                }
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid);
            } else {
                showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("File Put"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg);

                // Reset cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.resetBusySlot("cloudDriveModel onFilePutReplySignal");
                }
            }

            // Update ProgressBar on listItem and its parent.
            pageStack.find(function (page) {
                if (page.updateItemSlot) page.updateItemSlot(jobJson);
            });
        }

        onMetadataReplySignal: {
            // Get job json.
            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));
            console.debug("window cloudDriveModel onMetadataReplySignal " + getCloudName(jobJson.type) + " " + nonce + " " + err + " " + errMsg + " " + msg);

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                // Found metadata.
                var jsonObj = Utility.createJsonObj(msg);
                var itemJson = Utility.createJsonObj(cloudDriveModel.getItemJson(jobJson.local_file_path, jobJson.type, jobJson.uid));
                var localPathHash = cloudDriveModel.getItemHash(jobJson.local_file_path, jobJson.type, jobJson.uid);
//                console.debug("window cloudDriveModel onMetadataReplySignal jobJson " + jobJson.local_file_path + " " + jobJson.type + " " + jobJson.uid + " " + localPathHash);

                if (jobJson.type == CloudDriveModel.Dropbox) {
                    // If remotePath was deleted, remove link from localPath.
                    if (jsonObj.is_deleted) {
                        // If dir, should remove all sub items.
                        cloudDriveModel.removeItemWithChildren(jobJson.type, jobJson.uid, jobJson.local_file_path);

                        // TODO Why does it need here? As there is already below.
                        // Update ProgressBar on listItem.
//                        fsModel.setProperty(jobJson.local_file_path, FolderSizeItemListModel.IsRunningRole, jobJson.is_running);

                        // Notify removed link.
                        showMessageDialogSlot(
                                    getCloudName(jobJson.type) + " " + qsTr("Metadata"),
                                    qsTr("%1 was removed remotely.\nLink will be removed.").arg(jobJson.local_file_path),
                                    2000);
                    } else {
                        // Suspend next job.
                        cloudDriveModel.suspendNextJob();

                        // Sync starts from itself.
                        if (jsonObj.is_dir) { // Sync folder.
                            // If there is no local folder, create it and connect.
                            if (!cloudDriveModel.isDir(jobJson.local_file_path)) {
                                // TODO Add item to ListView.
                                cloudDriveModel.createDirPath(jobJson.local_file_path);
                                // Remove cache on target folders and its parents.
                                var p = findPage("folderPage");
                                if (p) {
                                    p.refreshItemAfterFileGetSlot(jobJson.local_file_path);
                                }
                            }

                            // Sync based on remote contents.
    //                        console.debug("cloudDriveModel onMetadataReplySignal folder jsonObj.rev " + jsonObj.rev + " jsonObj.hash " + jsonObj.hash + " localPathHash " + localPathHash);
                            if (jsonObj.hash != localPathHash) { // Sync all json(remote)'s contents.
                                for(var i=0; i<jsonObj.contents.length; i++) {
                                    var item = jsonObj.contents[i];
                                    var itemLocalPath = cloudDriveModel.getAbsolutePath(jobJson.local_file_path, cloudDriveModel.getRemoteName(jobJson.type, item.path));
                                    var itemLocalHash = cloudDriveModel.getItemHash(itemLocalPath, jobJson.type, jobJson.uid);
                                    var itemRemotePath = item.path;
                                    var itemRemoteHash = item.rev;
                                    if (item.is_dir) {
                                        // This flow will trigger recursive metadata calling.
//                                        console.debug("cloudDriveModel onMetadataReplySignal dir itemRemotePath " + itemRemotePath + " itemLocalHash " + itemLocalHash + " itemRemoteHash " + itemRemoteHash + " " + item.updated_time);
                                        cloudDriveModel.metadata(jobJson.type, jobJson.uid, itemLocalPath, item.path, -1);
                                    } else {
//                                        console.debug("cloudDriveModel onMetadataReplySignal file itemRemotePath " + itemRemotePath + " itemLocalHash " + itemLocalHash + " itemRemoteHash " + itemRemoteHash + " " + item.updated_time);
                                        if (itemRemoteHash > itemLocalHash) {
                                            cloudDriveModel.fileGet(jobJson.type, jobJson.uid, itemRemotePath, itemLocalPath, -1);
                                        } else if (itemRemoteHash < itemLocalHash) {
                                            cloudDriveModel.filePut(jobJson.type, jobJson.uid, itemLocalPath, itemRemotePath, -1);
                                        } else {
                                            cloudDriveModel.addItem(jobJson.type, jobJson.uid, itemLocalPath, itemRemotePath, itemRemoteHash);
                                        }
                                    }
                                }
                            }

                            // Add or Update timestamp from local to cloudDriveItem.
                            cloudDriveModel.addItem(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, jsonObj.hash);

                            // Sync based on local contents.
                            cloudDriveModel.syncFromLocal(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, jobJson.modelIndex);
                        } else { // Sync file.
    //                        console.debug("cloudDriveModel onMetadataReplySignal file jsonObj.rev " + jsonObj.rev + " localPathHash " + localPathHash);

                            // If (rev is newer or there is no local file), get from remote.
                            if (jsonObj.rev > localPathHash || !cloudDriveModel.isFile(jobJson.local_file_path)) {
                                // TODO Add item to ListView.
                                cloudDriveModel.fileGet(jobJson.type, jobJson.uid, jobJson.remote_file_path, jobJson.local_file_path, jobJson.modelIndex);
                            } else if (jsonObj.rev < localPathHash) {
                                cloudDriveModel.filePut(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, jobJson.modelIndex);
                            } else {
                                // Update lastModified on cloudDriveItem.
                                cloudDriveModel.addItem(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, jsonObj.rev);
                            }
                        }

                        // Resume next jobs.
                        cloudDriveModel.resumeNextJob();
                    }
                }

                if (jobJson.type == CloudDriveModel.SkyDrive) {
                    console.debug("window cloudDriveModel onMetadataReplySignal SkyDrive jobJson " + jobJson.type + " " + jobJson.uid + " " + jobJson.local_file_path + " " + jobJson.remote_file_path + " " + localPathHash);

                    if (jsonObj.property) {
                        // Suspend next job.
                        cloudDriveModel.suspendNextJob();

                        // Generate hash from updated_time.
                        var remotePathHash = jsonObj.property.updated_time;
    //                    console.debug("window cloudDriveModel onMetadataReplySignal remotePathHash " + remotePathHash);

                        // Sync starts from itself.
                        if (jsonObj.property.type == "folder" || jsonObj.property.type == "album") { // Sync folder.
                            // If there is no local folder, create it and connect.
                            if (!cloudDriveModel.isDir(jobJson.local_file_path)) {
                                // TODO Add item to ListView.
                                cloudDriveModel.createDirPath(jobJson.local_file_path);
                                // Remove cache on target folders and its parents.
                                var p = findPage("folderPage");
                                if (p) {
                                    p.refreshItemAfterFileGetSlot(jobJson.local_file_path);
                                }
                            }

                            // Sync based on remote contents.
    //                        console.debug("cloudDriveModel onMetadataReplySignal folder jsonObj.rev " + jsonObj.rev + " jsonObj.hash " + jsonObj.hash + " localPathHash " + localPathHash);
                            if (remotePathHash != localPathHash) { // Sync all json(remote)'s contents.
                                for(var i=0; i<jsonObj.data.length; i++) {
                                    var item = jsonObj.data[i];
                                    var itemLocalPath = cloudDriveModel.getAbsolutePath(jobJson.local_file_path, item.name);
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
                                            itemRemotePath = (item.parent_id) ? item.parent_id : cloudDriveModel.getRemoteRoot(jobJson.type);
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
                            cloudDriveModel.syncFromLocal_Block(jobJson.type, jobJson.uid, jobJson.local_file_path, jsonObj.property.parent_id, jobJson.modelIndex);
                        } else { // Sync file.
                            console.debug("window cloudDriveModel onMetadataReplySignal file jobJson " + jobJson.local_file_path + " " + jobJson.remote_file_path + " " + jobJson.type + " " + jobJson.uid + " remotePathHash " + remotePathHash + " localPathHash " + localPathHash);

                            // If (rev is newer or there is no local file), get from remote.
                            if (remotePathHash > localPathHash || !cloudDriveModel.isFile(jobJson.local_file_path)) {
                                // TODO Add item to ListView.
                                cloudDriveModel.fileGet(jobJson.type, jobJson.uid, jobJson.remote_file_path, jobJson.local_file_path, jobJson.modelIndex);
                            } else if (remotePathHash < localPathHash) {
                                // Put file to remote parent path. Or root if parent_id is null.
                                var remoteParentPath = (jsonObj.property.parent_id) ? jsonObj.property.parent_id : cloudDriveModel.getRemoteRoot(jobJson.type);
                                cloudDriveModel.filePut(jobJson.type, jobJson.uid, jobJson.local_file_path, remoteParentPath, jobJson.modelIndex);
                            } else {
                                // Update lastModified on cloudDriveItem.
                                cloudDriveModel.addItem(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, remotePathHash);
                            }
                        }

                        // Resume next jobs.
                        cloudDriveModel.resumeNextJob();
                    } else {
                        console.debug("window cloudDriveModel onMetadataReplySignal property is not found.");
                    }
                }

                if (jobJson.type == CloudDriveModel.GoogleDrive) {
                    console.debug("window cloudDriveModel onMetadataReplySignal GoogleDrive jobJson " + jobJson.type + " " + jobJson.uid + " " + jobJson.local_file_path + " " + jobJson.remote_file_path + " " + localPathHash);

                    if (jsonObj.property) {
                        // Suspend next job.
                        cloudDriveModel.suspendNextJob();

                        // Generate hash from updated_time.
                        var remotePathHash = jsonObj.property.modifiedDate;
    //                    console.debug("window cloudDriveModel onMetadataReplySignal remotePathHash " + remotePathHash);

                        // Sync starts from itself.
                        if (jsonObj.property.mimeType == "application/vnd.google-apps.folder") { // Sync folder.
                            // If there is no local folder, create it and connect.
                            if (!cloudDriveModel.isDir(jobJson.local_file_path)) {
                                // TODO Add item to ListView.
                                cloudDriveModel.createDirPath(jobJson.local_file_path);
                                // Remove cache on target folders and its parents.
                                var p = findPage("folderPage");
                                if (p) {
                                    p.refreshItemAfterFileGetSlot(jobJson.local_file_path);
                                }
                            }

                            // Sync based on remote contents.
                            if (remotePathHash != localPathHash) { // Sync all json(remote)'s contents.
                                for(var i=0; i<jsonObj.items.length; i++) {
                                    var item = jsonObj.items[i];
                                    var itemLocalPath = cloudDriveModel.getAbsolutePath(jobJson.local_file_path, item.title);
                                    var itemLocalHash = cloudDriveModel.getItemHash(itemLocalPath, jobJson.type, jobJson.uid);
                                    var itemRemotePath = item.id;
                                    var itemRemoteHash = item.modifiedDate;
                                    if (item.mimeType == "application/vnd.google-apps.folder") {
                                        // This flow will trigger recursive metadata calling.
                                        console.debug("cloudDriveModel onMetadataReplySignal dir itemRemotePath " + itemRemotePath + " itemLocalHash " + itemLocalHash + " itemRemoteHash " + itemRemoteHash + " " + item.updated_time);
                                        cloudDriveModel.metadata(jobJson.type, jobJson.uid, itemLocalPath, itemRemotePath, -1);
                                    } else {
                                        console.debug("cloudDriveModel onMetadataReplySignal file itemRemotePath " + itemRemotePath + " itemLocalHash " + itemLocalHash + " itemRemoteHash " + itemRemoteHash + " " + item.updated_time);
                                        if (itemRemoteHash > itemLocalHash) {
                                            // TODO Uses downloadUrl if it exists.
                                            // TODO It should be downloadUrl because it will not be albe to create connection in CloudDriveModel.fileGetReplyFilter.
//                                            itemRemotePath = (item.downloadUrl) ? item.downloadUrl : itemRemotePath;
                                            cloudDriveModel.fileGet(jobJson.type, jobJson.uid, itemRemotePath, itemLocalPath, -1);
                                        } else if (itemRemoteHash < itemLocalHash) {
                                            // Put file to remote parent path. Or root if parent_id is null.
                                            itemRemotePath = (item.parents && item.parents.length > 0) ? item.parents[0].id : cloudDriveModel.getRemoteRoot(jobJson.type);
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
                            var remoteParentPath = (jsonObj.property.parents && jsonObj.property.parents.length > 0) ? jsonObj.property.parents[0].id : cloudDriveModel.getRemoteRoot(jobJson.type);
                            cloudDriveModel.syncFromLocal_Block(jobJson.type, jobJson.uid, jobJson.local_file_path, remoteParentPath, jobJson.modelIndex);
                        } else { // Sync file.
                            console.debug("window cloudDriveModel onMetadataReplySignal file jobJson " + jobJson.local_file_path + " " + jobJson.remote_file_path + " " + jobJson.type + " " + jobJson.uid + " remotePathHash " + remotePathHash + " localPathHash " + localPathHash);

                            // If (rev is newer or there is no local file), get from remote.
                            if (remotePathHash > localPathHash || !cloudDriveModel.isFile(jobJson.local_file_path)) {
                                // TODO Add item to ListView.
                                // TODO Uses downloadUrl if it exists.
                                // TODO It should be downloadUrl because it will not be albe to create connection in CloudDriveModel.fileGetReplyFilter.
//                                var remoteParentPath = (jsonObj.property.downloadUrl) ? jsonObj.property.downloadUrl : jobJson.remote_file_path;
                                cloudDriveModel.fileGet(jobJson.type, jobJson.uid, jobJson.remote_file_path, jobJson.local_file_path, jobJson.modelIndex);
                            } else if (remotePathHash < localPathHash) {
                                // Put file to remote parent path. Or root if parent_id is null.
                                var remoteParentPath = (jsonObj.property.parents && jsonObj.property.parents.length > 0) ? jsonObj.property.parents[0].id : cloudDriveModel.getRemoteRoot(jobJson.type);
                                cloudDriveModel.filePut(jobJson.type, jobJson.uid, jobJson.local_file_path, remoteParentPath, jobJson.modelIndex);
                            } else {
                                // Update lastModified on cloudDriveItem.
                                cloudDriveModel.addItem(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, remotePathHash);
                            }
                        }

                        // Resume next jobs.
                        cloudDriveModel.resumeNextJob();
                    } else {
                        console.debug("window cloudDriveModel onMetadataReplySignal property is not found.");
                    }
                }

                if (jobJson.type == CloudDriveModel.Ftp) {
                    console.debug("window cloudDriveModel onMetadataReplySignal Ftp jobJson " + jobJson.type + " " + jobJson.uid + " " + jobJson.local_file_path + " " + jobJson.remote_file_path + " " + localPathHash);

                    if (jsonObj.property) {
                        // Suspend next job.
                        cloudDriveModel.suspendNextJob();

                        // Generate hash from updated_time.
                        var remotePathHash = jsonObj.property.lastModified;
    //                    console.debug("window cloudDriveModel onMetadataReplySignal remotePathHash " + remotePathHash);

                        // Sync starts from itself.
                        if (jsonObj.property.isDir) { // Sync folder.
                            // If there is no local folder, create it and connect.
                            if (!cloudDriveModel.isDir(jobJson.local_file_path)) {
                                // TODO Add item to ListView.
                                cloudDriveModel.createDirPath(jobJson.local_file_path);
                                // Remove cache on target folders and its parents.
                                var p = findPage("folderPage");
                                if (p) {
                                    p.refreshItemAfterFileGetSlot(jobJson.local_file_path);
                                }
                            }

                            // Sync based on remote contents.
                            console.debug("cloudDriveModel onMetadataReplySignal folder remotePathHash " + remotePathHash + " localPathHash " + localPathHash);
                            if (remotePathHash != localPathHash) { // Sync all json(remote)'s contents.
                                for(var i=0; i<jsonObj.data.length; i++) {
                                    var item = jsonObj.data[i];
                                    var itemLocalPath = cloudDriveModel.getAbsolutePath(jobJson.local_file_path, item.name);
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
                                            itemRemotePath = (item.parent_id) ? item.parent_id : cloudDriveModel.getRemoteRoot(jobJson.type);
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
                            console.debug("window cloudDriveModel onMetadataReplySignal file jobJson " + jobJson.local_file_path + " " + jobJson.remote_file_path + " " + jobJson.type + " " + jobJson.uid + " remotePathHash " + remotePathHash + " localPathHash " + localPathHash);

                            // If (rev is newer or there is no local file), get from remote.
                            if (remotePathHash > localPathHash || !cloudDriveModel.isFile(jobJson.local_file_path)) {
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
                        console.debug("window cloudDriveModel onMetadataReplySignal property is not found.");
                    }
                }

            } else if (err == 203) { // If metadata is not found, put it to cloud right away recursively.
                console.debug("window cloudDriveModel onMetadataReplySignal " + err + " " + errMsg + " " + msg);

                // Suspend next job.
                cloudDriveModel.suspendNextJob();

                if (cloudDriveModel.isRemoteAbsolutePath(jobJson.type)) {
                    if (cloudDriveModel.isDir(jobJson.local_file_path)) {
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

                // Resume next jobs.
                cloudDriveModel.resumeNextJob();
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid);
            } else {
                showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("Metadata"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg);

                // Reset running.
                jobJson.is_running = false;
            }

            // Update ProgressBar on listItem and its parent.
            pageStack.find(function (page) {
                if (page.updateItemSlot) page.updateItemSlot(jobJson);
            });
        }

        onCreateFolderReplySignal: {
            console.debug("window cloudDriveModel onCreateFolderReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jsonText = cloudDriveModel.getJobJson(nonce);
//            console.debug("window cloudDriveModel jsonText " + jsonText);

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            var jobJson = JSON.parse(jsonText);
            var msgJson = JSON.parse(msg);

            if (err == 0) {
                // Refresh cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.refreshSlot("cloudDriveModel onCreateFolderReplySignal");
                }
            } else if (err == 202 && jobJson.type == CloudDriveModel.Dropbox && jobJson.local_file_path != "") {
                // Dropbox Folder already exists. proceed sync.
                cloudDriveModel.metadata(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, jobJson.model_index);
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid);
            } else if (err == 299 && jobJson.type == CloudDriveModel.SkyDrive && msgJson.error && msgJson.error.code == "resource_already_exists") {
                // SkyDrive Folder already exists. Do nothing
                showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("Create Folder"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg +
                            "\n\n" +
                            qsTr("Please proceed with sync."),
                            2000);

                // Reset cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.resetBusySlot("cloudDriveModel onCreateFolderReplySignal");
                }
            } else {
                showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("Create Folder"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg,
                            2000);

                // Reset cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.resetBusySlot("cloudDriveModel onCreateFolderReplySignal");
                }
            }

            // TODO *** create SkyDrive folder freezes here *** TODO Does it need?
            // Update ProgressBar if localPath is specified.
            if (jobJson.type == CloudDriveModel.SkyDrive && pageStack.currentPage.name == "folderPage") {
                // Refresh all items because jobJson.local_file_path is new folder name.
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
        }

        onDownloadProgress: {
//            console.debug("window cloudDriveModel onDownloadProgress " + nonce + " " + bytesReceived + " / " + bytesTotal);

            var jsonText = cloudDriveModel.getJobJson(nonce);
//            console.debug("window cloudDriveModel jsonText " + jsonText);

            var jobJson = JSON.parse(jsonText);

            // Update ProgressBar on listItem and its parent.
            pageStack.find(function (page) {
                if (page.updateItemProgressBarSlot) page.updateItemProgressBarSlot(jobJson);
            });
        }

        onUploadProgress: {
//            console.debug("window cloudDriveModel onUploadProgress " + nonce + " " + bytesSent + " / " + bytesTotal);

            var jsonText = cloudDriveModel.getJobJson(nonce);
//            console.debug("window cloudDriveModel jsonText " + jsonText);

            var jobJson = JSON.parse(jsonText);

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

        onJobQueueStatusSignal: {
            if (pageStack) {
                if (pageStack.currentPage.name == "settingPage") {
                    pageStack.currentPage.updateCloudDriveItemCount(itemCount);
                }

                // Update job queue count on current page.
                pageStack.find(function (page) {
                    if (page.updateJobQueueCount) page.updateJobQueueCount(runningJobCount, jobQueueCount);
                });
            }
        }

        onRefreshRequestSignal: {
            if (pageStack.currentPage.name == "folderPage") {
                pageStack.currentPage.refreshSlot("cloudDriveModel onRefreshRequestSignal");
            }
        }

        onCopyFileReplySignal: {
            console.debug("window cloudDriveModel onCopyFileReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            console.debug("window cloudDriveModel onCopyFileReplySignal json " + cloudDriveModel.getJobJson(nonce));

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                // Cloud items have been managed by CloudDriveModel::copyFileReplyFilter.
                // Sync after copy.
                cloudDriveModel.metadata(jobJson.type, jobJson.uid, jobJson.new_local_file_path, jobJson.new_remote_file_path, jobJson.model_index);

                // Refresh cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.refreshSlot("cloudDriveModel onCopyFileReplySignal");
                }
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid);
            } else {
                showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("Copy"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg);

                // Reset cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.resetBusySlot("cloudDriveModel onCopyFileReplySignal");
                }
            }

            // Update ProgressBar on NEW listItem and its parent.
            pageStack.find(function (page) {
                if (page.updateItemSlot) page.updateItemSlot(jobJson);
            });
        }

        onMoveFileReplySignal: {
            console.debug("window cloudDriveModel onMoveFileReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            console.debug("window cloudDriveModel onMoveFileReplySignal json " + cloudDriveModel.getJobJson(nonce));

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                // Cloud items have been managed by CloudDriveModel::moveFileReplyFilter.
                // Sync after move.
                cloudDriveModel.metadata(jobJson.type, jobJson.uid, jobJson.new_local_file_path, jobJson.new_remote_file_path, jobJson.model_index);

                // Refresh cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.refreshSlot("cloudDriveModel onMoveFileReplySignal");
                }
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid);
            } else {
                showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("Move"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg);

                // Reset cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.resetBusySlot("cloudDriveModel onMoveFileReplySignal");
                }
            }

            // Update ProgressBar on NEW listItem and its parent.
            pageStack.find(function (page) {
                if (page.updateItemSlot) page.updateItemSlot(jobJson);
            });
        }

        onDeleteFileReplySignal: {
            console.debug("window cloudDriveModel onDeleteFileReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                // Refresh cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.refreshSlot("cloudDriveModel onDeleteFileReplySignal");
                }
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid);
            } else {
                showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("Delete"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg);

                // Reset cloudFolderPage.
                var p = findPage("cloudFolderPage");
                if (p) {
                    p.resetBusySlot("cloudDriveModel onDeleteFileReplySignal");
                }
            }

            // Update ProgressBar on listItem and its parent.
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

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                // Open recipientSelectionDialog for sending share link.
                showRecipientSelectionDialogSlot(jobJson.type, jobJson.uid, jobJson.local_file_path,
                                                 (jobJson.local_file_path.indexOf("/") >= 0) ? cloudDriveModel.getFileName(jobJson.local_file_path) : jobJson.local_file_path,
                                                 url);
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid);
            } else {
                showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("Share"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }

            // Update ProgressBar on listItem and its parent.
            pageStack.find(function (page) {
                if (page.updateItemSlot) page.updateItemSlot(jobJson);
            });
        }

        onJobEnqueuedSignal: {
            // Get job json.
            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));
            cloudDriveJobsModel.append(jobJson);

            console.debug("window cloudDriveModel onJobEnqueuedSignal " + nonce + " " + localPath + " " + cloudDriveModel.getOperationName(jobJson.operation));

            pageStack.find(function (page) {
                if (page.updateItemSlot) page.updateItemSlot(jobJson);
            });
        }

        onJobRemovedSignal: {
            cloudDriveJobsModel.removeJob(nonce);
        }

        onBrowseReplySignal: {
            console.debug("window cloudDriveModel onBrowseReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                pageStack.currentPage.parseCloudDriveMetadataJson(msg);
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid);
            } else {
                showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("Browse"),
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
        }

        onMigrateProgressSignal: {
            var p = findPage("folderPage");
            if (p) {
                p.updateMigrationProgressSlot(type, uid, localFilePath, remoteFilePath, count, total);
            }
        }

        Component.onCompleted: {
            console.debug(Utility.nowText() + " window cloudDriveModel onCompleted");
            window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("CloudDriveModel"), 0.1);

            // Proceeds queued jobs during constructions.
            cloudDriveModel.resumeNextJob();
        }
    }

    MessageDialog {
        id: messageDialog
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
        model: gcpClient.printerModel
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

    function showPrinterSelectionDialog(srcFilePath, srcUrl) {
        printerSelectionDialog.srcFilePath = srcFilePath;
        printerSelectionDialog.srcURL = srcUrl;
        printerSelectionDialog.open();
    }

    RecipientSelectionDialog {
        id: recipientSelectionDialog
        shareFileCaller: cloudDriveModel.shareFileCaller

        function refresh() {
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
    }

    function showRecipientSelectionDialogSlot(cloudDriveType, uid, localPath, fileName, url) {
        // Open recipientSelectionDialog for sending share link.
        var senderEmail = cloudDriveModel.getUidEmail(cloudDriveType, uid);
        if (senderEmail != "") {
            recipientSelectionDialog.srcFilePath = localPath;
            recipientSelectionDialog.srcFileName = fileName;
            recipientSelectionDialog.messageSubject = appInfo.emptyStr+qsTr("Share file on %1").arg(cloudDriveModel.getCloudName(cloudDriveType));
            recipientSelectionDialog.messageBody = appInfo.emptyStr+qsTr("Please download file with below link.") + "\n" + url;
            recipientSelectionDialog.senderEmail = senderEmail;
            recipientSelectionDialog.open();
        }
    }

    Text {
        id: statusBarTitle
        x: 4
        y: 0
        width: parent.width / 2
        height: 26
        color: "white"
        text: "FilesPlus"
        smooth: false
        verticalAlignment: Text.AlignVCenter
        font.pointSize: 18
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
            x: 0
            y: 0
            width: 80
            height: 80
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
