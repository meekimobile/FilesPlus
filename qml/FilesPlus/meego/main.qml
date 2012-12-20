import QtQuick 1.1
import com.nokia.meego 1.0
import AppInfo 1.0
import ClipboardModel 1.0
import GCPClient 1.0
import CloudDriveModel 1.0
import FolderSizeItemListModel 1.0
import "Utility.js" as Utility

PageStackWindow {
    id: window
    showStatusBar: true
    showToolBar: true
    initialPage: DrivePage {
        id: drivePage
    }

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

    AppInfo {
        id: appInfo
        domain: "MeekiMobile"
        app: "FilesPlus"

        Component.onCompleted: {
            appInfo.startMonitoring();
        }
        onLocaleChanged: {
            console.debug("appInfo onLocaleChanged");
            var p = pageStack.find(function (page) { return page.name == "folderPage"; });
            if (p) p.requestJobQueueStatusSlot();
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

    GCPClient {
        id: gcpClient

        property string selectedFilePath
        property string selectedURL

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
                var p = pageStack.find(function (page) { return page.name == "printJobsPage"; });
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

        property string shareFileCaller

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
            if (["dropboxclient","dropbox"].indexOf(typeText.toLowerCase()) != -1) {
                return CloudDriveModel.Dropbox;
            } else if (["skydriveclient","skydrive"].indexOf(typeText.toLowerCase()) != -1) {
                return CloudDriveModel.SkyDrive;
            } else if (["ftpclient","ftp"].indexOf(typeText.toLowerCase()) != -1) {
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

//        function setGCDClientAuthCode(code) {
//            var res = cloudDriveModel.parseAuthorizationCode(CloudDriveModel.GoogleDrive, code);
//            if (res) {
//                cloudDriveModel.accessToken(CloudDriveModel.GoogleDrive);
//            }
//        }

        onRequestTokenReplySignal: {
            console.debug("window cloudDriveModel onRequestTokenReplySignal " + err + " " + errMsg + " " + msg);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                cloudDriveModel.authorize(jobJson.type);
            } else {
                pageStack.currentPage.showMessageDialogSlot(
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

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                // TODO find better way to proceed after got accessToken.
                if (popupToolPanel.selectedFilePath) {
//                    uidDialog.proceedPendingOperation();
//                    syncFileSlot(popupToolPanel.selectedFilePath, popupToolPanel.selectedFileIndex);
                } else {
                    // TODO Get account info and show in dialog.
                    pageStack.currentPage.showMessageDialogSlot(
                                getCloudName(jobJson.type) + " " + qsTr("Access Token"),
                                qsTr("CloudDrive user is authorized.\nPlease proceed your sync action."));

                    // Refresh account page.
                    var p = pageStack.find(function (page) { return page.name == "cloudDriveAccountsPage"; });
                    if (p) p.refreshCloudDriveAccountsSlot();
                }
            } else {
                pageStack.currentPage.showMessageDialogSlot(
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
                var jsonObj = Utility.createJsonObj(msg);

                // Send info to currentPage.
                var sharedValue = (jsonObj.quota_info && jsonObj.quota_info.shared) ? jsonObj.quota_info.shared : 0;
                var normalValue = (jsonObj.quota_info && jsonObj.quota_info.normal) ? jsonObj.quota_info.normal : 0;
                var quotaValue = (jsonObj.quota_info && jsonObj.quota_info.quota) ? jsonObj.quota_info.quota : 0;
                pageStack.currentPage.updateAccountInfoSlot(jobJson.type, jsonObj.uid, jsonObj.name, jsonObj.email,
                                                            sharedValue,
                                                            normalValue,
                                                            quotaValue);
            } else if (err == 204) {
                // TODO Refactor to support all SkyDriveClient services.
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid);
            } else {
                pageStack.currentPage.showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("Account Info"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }
        }

        onQuotaReplySignal: {
            console.debug("window cloudDriveModel onQuotaReplySignal " + err + " " + errMsg + " " + msg);

            var jobJson = JSON.parse(cloudDriveModel.getJobJson(nonce));

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                var jsonObj = Utility.createJsonObj(msg);

                // Send info to currentPage.
                pageStack.currentPage.updateAccountInfoSlot(jobJson.type, jobJson.uid, "", cloudDriveModel.getUidEmail(jobJson.type, jobJson.uid),
                                                            0,
                                                            jsonObj.quota-jsonObj.available,
                                                            jsonObj.quota);
            } else if (err == 204) {
                // TODO Refactor to support all SkyDriveClient services.
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid);
            } else {
                pageStack.currentPage.showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("Account Quota"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }
        }

        onFileGetReplySignal: {
            console.debug("window cloudDriveModel onFileGetReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jsonText = cloudDriveModel.getJobJson(nonce);
//            console.debug("window cloudDriveModel jsonText " + jsonText);

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            var jobJson = JSON.parse(jsonText);


            // Update ProgressBar on listItem and its parents.
            var p = pageStack.find(function (page) { return (page.name == "folderPage"); });
            if (p) {
                p.updateFolderSizeItemSlot(jobJson.local_file_path, jobJson.is_running);
            }

            if (err == 0) {
                // Remove cache on target folders and its parents.
                var p = pageStack.find(function (page) { return (page.name == "folderPage"); });
                if (p) {
                    p.refreshItemAfterFileGetSlot(jobJson.local_file_path);
                }
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid);
            } else {
                pageStack.currentPage.showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("File Get"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }
        }

        onFilePutReplySignal: {
            console.debug("window cloudDriveModel onFilePutReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jsonText = cloudDriveModel.getJobJson(nonce);
//            console.debug("window cloudDriveModel onFilePutReplySignal jsonText " + jsonText);

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            var jobJson = JSON.parse(jsonText);

            // Update ProgressBar on listItem and its parent.
            var p = pageStack.find(function (page) { return (page.name == "folderPage"); });
            if (p) {
                p.updateFolderSizeItemSlot(jobJson.local_file_path, jobJson.is_running);
            }

            if (err == 0) {
                // Do nothing.
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid);
            } else {
                pageStack.currentPage.showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("File Put"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }
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
                        pageStack.currentPage.showMessageDialogSlot(
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
                            }

                            // Sync based on remote contents.
    //                        console.debug("cloudDriveModel onMetadataReplySignal folder jsonObj.rev " + jsonObj.rev + " jsonObj.hash " + jsonObj.hash + " localPathHash " + localPathHash);
                            if (jsonObj.hash != localPathHash) { // Sync all json(remote)'s contents.
                                for(var i=0; i<jsonObj.contents.length; i++) {
                                    var item = jsonObj.contents[i];
                                    var itemLocalPath = cloudDriveModel.getAbsolutePath(jobJson.local_file_path, cloudDriveModel.getRemoteName(item.path));
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

                // TODO Supports SkyDrive.
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
                            console.debug("window cloudDriveModel onMetadataReplySignal file jobJson " + jobJson.local_file_path + " " + jobJson.remote_file_path + " " + jobJson.type + " " + jobJson.uid + " remotePathHash " + remotePathHash + " localPathHash " + localPathHash);

                            // If (rev is newer or there is no local file), get from remote.
                            if (remotePathHash > localPathHash || !cloudDriveModel.isFile(jobJson.local_file_path)) {
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
                        console.debug("window cloudDriveModel onMetadataReplySignal property is not found.");
                    }
                }

                // TODO Supports FTP.
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

                if (jobJson.type == CloudDriveModel.Dropbox) {
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

                // TODO Supports SkyDrive.
                if (jobJson.type == CloudDriveModel.SkyDrive) {

                }

                // TODO Supports Ftp.
                if (jobJson.type == CloudDriveModel.Ftp) {
                    if (cloudDriveModel.isDir(jobJson.local_file_path)) {
                        // Remote folder will be created in syncFromLocal if it's required.
                        cloudDriveModel.syncFromLocal(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, jobJson.modelIndex);
                    } else {
                        cloudDriveModel.filePut(jobJson.type, jobJson.uid, jobJson.local_file_path, jobJson.remote_file_path, jobJson.modelIndex);
                    }
                }

                // Resume next jobs.
                cloudDriveModel.resumeNextJob();
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid);
            } else {
                pageStack.currentPage.showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("Metadata"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg);

                // Reset running.
                jobJson.is_running = false;
            }

            // Update ProgressBar on listItem and its parent.
            var p = pageStack.find(function (page) { return (page.name == "folderPage"); });
            if (p) {
                p.updateFolderSizeItemSlot(jobJson.local_file_path, jobJson.is_running);
            }

            // Workaround: Refresh item once got reply. To fix unexpected showing cloud_wait icon.
//            fsModel.refreshItem(jobJson.local_file_path);
        }

        onCreateFolderReplySignal: {
            console.debug("window cloudDriveModel onCreateFolderReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jsonText = cloudDriveModel.getJobJson(nonce);
//            console.debug("window cloudDriveModel jsonText " + jsonText);

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);
            var jobJson = JSON.parse(jsonText);

            if (err == 0) {
                // Do nothing.
            } else if (err == 202) { // Folder already exists.
                // Do nothing.
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid);
            } else {
                pageStack.currentPage.showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("Create Folder"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }

            var p = pageStack.find(function (page) { return (page.name == "folderPage"); });
            if (p) {
                // TODO *** create SkyDrive folder freezes here *** TODO Does it need?
                // Update ProgressBar if localPath is specified.
                p.updateFolderSizeItemSlot(jobJson.local_file_path, jobJson.is_running);
                // Refresh cloudDrivePathDialog if it's opened.
                p.updateCloudDrivePathDialogSlot(jobJson.new_remote_file_path);
            }
        }

        onDownloadProgress: {
//            console.debug("window cloudDriveModel onDownloadProgress " + nonce + " " + bytesReceived + " / " + bytesTotal);

            var jsonText = cloudDriveModel.getJobJson(nonce);
//            console.debug("window cloudDriveModel jsonText " + jsonText);

            var jobJson = JSON.parse(jsonText);

            // Update ProgressBar on listItem and its parent.
            var p = pageStack.find(function (page) { return (page.name == "folderPage"); });
            if (p) {
                p.updateFolderSizeItemProgressBarSlot(jobJson.local_file_path, jobJson.is_running, jobJson.operation, bytesReceived, bytesTotal);
            }
        }

        onUploadProgress: {
//            console.debug("window cloudDriveModel onUploadProgress " + nonce + " " + bytesSent + " / " + bytesTotal);

            var jsonText = cloudDriveModel.getJobJson(nonce);
//            console.debug("window cloudDriveModel jsonText " + jsonText);

            var jobJson = JSON.parse(jsonText);

            // Update ProgressBar on listItem and its parent.
            var p = pageStack.find(function (page) { return (page.name == "folderPage"); });
            if (p) {
                p.updateFolderSizeItemProgressBarSlot(jobJson.local_file_path, jobJson.is_running, jobJson.operation, bytesSent, bytesTotal);
            }
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
            // Send info to cloudDriveAccountsPage.
            if (pageStack) {
                var p = pageStack.find(function (page) { return (page.name == "settingPage"); });
                if (p) {
                    p.updateJobQueueCount(runningJobCount, jobQueueCount);
                    p.updateCloudDriveItemCount(itemCount);
                }

                p = pageStack.find(function (page) { return (page.name == "folderPage"); });
                if (p) {
                    // Update (runningJobCount + jobQueueCount) on cloudButton.
                    p.updateJobQueueCount(runningJobCount, jobQueueCount);
                }
            }
        }

        onRefreshRequestSignal: {
            var p = pageStack.find(function (page) { return (page.name == "folderPage"); });
            if (p) {
                p.refreshSlot("cloudDriveModel onRefreshRequestSignal");
            }
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
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid);
            } else {
                pageStack.currentPage.showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("Move"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }

            // Update ProgressBar on NEW listItem and its parent.
            var p = pageStack.find(function (page) { return (page.name == "folderPage"); });
            if (p) {
                p.updateFolderSizeItemSlot(jobJson.new_local_file_path, jobJson.is_running);
            }
        }

        onDeleteFileReplySignal: {
            console.debug("window cloudDriveModel onDeleteFileReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                // do nothing.
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid);
            } else {
                pageStack.currentPage.showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("Delete"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }

            var p = pageStack.find(function (page) { return (page.name == "folderPage"); });
            if (p) {
                // Update ProgressBar on listItem and its parent.
                p.updateFolderSizeItemSlot(jobJson.local_file_path, jobJson.is_running);
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
                // TODO Select delivery service. (email, sms)
                // Open recipientSelectionDialog for sending share link.
                showRecipientSelectionDialogSlot(jobJson.type, jobJson.uid, jobJson.local_file_path, url);
            } else if (err == 204) { // Refresh token
                cloudDriveModel.refreshToken(jobJson.type, jobJson.uid);
            } else {
                pageStack.currentPage.showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("Share"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg);
            }

            var p = pageStack.find(function (page) { return (page.name == "folderPage"); });
            if (p) {
                // Update ProgressBar on listItem and its parent.
                p.updateFolderSizeItemSlot(jobJson.local_file_path, jobJson.is_running);
            }
        }

        onJobEnqueuedSignal: {
            console.debug("window cloudDriveModel onJobEnqueuedSignal " + nonce + " " + localPath);
            var p = pageStack.find(function (page) { return (page.name == "folderPage"); });
            if (p) {
                p.refreshItemSlot(localPath);
            }
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
                pageStack.currentPage.showMessageDialogSlot(
                            getCloudName(jobJson.type) + " " + qsTr("Browse"),
                            qsTr("Error") + " " + err + " " + errMsg + " " + msg);

                // Reset and close cloudDrivePathDialog.
                var p = pageStack.find(function (page) { return (page.name == "folderPage"); });
                if (p) {
                    p.closeCloudDrivePathDialogSlot();
                }
            }
        }

        onMigrateProgressSignal: {
            var p = pageStack.find(function (page) { return (page.name == "folderPage"); });
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
        interval: 1000

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
            // Load folderPage then push drivePage to increase performance.
//            console.debug(Utility.nowText() + " window pushPagesTimer push folderPage");
//            var folderPage = pageStack.push(Qt.resolvedUrl("FolderPage.qml"));
//            console.debug(Utility.nowText() + " window pushPagesTimer push drivePage");
//            folderPage.showDrivePageSlot(); // Push drivePage from folderPage to pass reference to cloudDriveModel.
        }
    }

    Component.onCompleted: {
        console.debug(Utility.nowText() + " window onCompleted");
        window.updateLoadingProgressSlot(qsTr("Loading"), 0.1);

        // Set to portrait to show splash screen. Then it will set back to default once it's destroyed.
        screen.allowedOrientations = Screen.Portrait;

        platformWindow.activeChanged.connect(activateSlot);
    }

    function activateSlot() {
        console.debug("window activateSlot platformWindow.active " + platformWindow.active);
        if (platformWindow.active) {
            var p = pageStack.find(function (page) { return page.name == "folderPage"; });
            if (p) p.activateSlot();
        }
    }
}
