import QtQuick 1.1
import com.nokia.symbian 1.1
import Charts 1.0
import FolderSizeItemListModel 1.0
import GCPClient 1.0
import CloudDriveModel 1.0
import "Utility.js" as Utility

Page {
    id: folderPage

    property string name: "folderPage"
    property alias currentDir: fsModel.currentDir

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

    onStateChanged: {
        console.debug("folderPage onStateChanged state=" + folderPage.state);
        if (state == "chart") {
            fsModel.setSortFlag(FolderSizeItemListModel.SortBySize);
        }
    }

    tools: toolBarLayout

    ToolBarLayout {
        id: toolBarLayout
        x: 0

        ToolButton {
            id: backButton
            iconSource: "toolbar-back"
            flat: true

            Component.onCompleted: {
                backButton.clicked.connect(goUpSlot);
            }
        }

        ToolButton {
            id: refreshButton
            iconSource: "toolbar-refresh"
            flat: true

            Component.onCompleted: {
                refreshButton.clicked.connect(refreshSlot);
            }
        }

        ToolButton {
            id: flipButton
            iconSource: (folderPage.state != "list") ? "list.svg" : "chart.svg"
            flat: true

            Component.onCompleted: {
                flipButton.clicked.connect(flipSlot);
            }
        }

        ToolButton {
            id: menuButton
            iconSource: "toolbar-menu"
            flat: true
            onClicked: {
                mainMenu.open();
            }
        }
    }

    MainMenu {
        id: mainMenu
    }

    SortByMenu {
        id: sortByMenu

        onSelectSort: {
            console.debug("sortByMenu setSortFlag flag=" + flag);
            fsModel.setSortFlag(flag);
        }

        onStatusChanged: {
            if (status == DialogStatus.Open) {
                // TODO remove dependency
                sortFlag = fsModel.getSortFlag();
            }
        }
    }

    SettingMenu {
        id: settingMenu
        onResetCache: {
            resetCacheConfirmation.open();
        }
        onResetCloudPrint: {
            popupToolPanel.selectedFilePath = "";
            popupToolPanel.selectedFileIndex = -1;
//            gcpClient.refreshAccessToken();
            gcpClient.authorize();
        }
        onResetCloudDrive: {
            popupToolPanel.selectedFilePath = "";
            popupToolPanel.selectedFileIndex = -1;
//            gcpClient.refreshAccessToken();
            cloudDriveModel.authorize(CloudDriveModel.GoogleDrive);
        }
        onRegisterDropboxUser: {
            cloudDriveModel.requestToken(CloudDriveModel.Dropbox);
        }
    }

    ConfirmDialog {
        id: resetCacheConfirmation
        titleText: "Reset Cache"
        contentText: "Resetting Cache will take time depends on numbers of sub folders/files under current folder.\n\nPlease click OK to continue."
        onConfirm: {
            resetCacheSlot();
        }
    }

    function refreshSlot() {
        fsModel.refreshDir(false);
    }

    function resetCacheSlot() {
        fsModel.refreshDir(true);
    }

    function goUpSlot() {
        if (fsModel.isRoot()) {
            pageStack.push(Qt.resolvedUrl("DrivePage.qml"), {}, true);
        } else {
            fsModel.changeDir("..");
        }
    }

    function flipSlot() {
        flipable1.flipped = !flipable1.flipped;
    }

    function orientationChangeSlot() {
        pieChart1.refreshItems();
    }

    function printFileSlot(srcFilePath) {
        console.debug("folderPage printFileSlot srcFilePath=" + srcFilePath);
        if (srcFilePath == "") return;

        if (gcpClient.getContentType(srcFilePath) == "") {
            console.debug("folderPage printFileSlot File type is not supported. (" + srcFilePath + ")");

            messageDialog.titleText = "Print Error";
            messageDialog.message = "Can't print " + srcFilePath + ".\
\nFile type is not supported. Only JPEG, PNG, Text and PDF are supported.";
            messageDialog.open();
            return;
        }

        if (!gcpClient.isAuthorized()) {
            messageDialog.message = "Files+ print via Google CloudPrint service.\
\nPlease enable printer on your desktop with Chrome or with CloudPrint-ready printer.\
\nYou will redirect to authorization page.";
            messageDialog.titleText = "Print with CloudPrint";
            messageDialog.open();

            gcpClient.authorize();
        } else {
            var printerList = gcpClient.getStoredPrinterList();
            console.debug("folderPage printFileSlot gcpClient.getStoredPrinterList()=" + printerList);
            if (printerList.length > 0) {
                printerSelectionDialog.srcFilePath = srcFilePath;
                printerSelectionDialog.model = printerList;
                printerSelectionDialog.open();
            } else {
                // TODO Open progress dialog.
                downloadProgressDialog.titleText = "Search for printers";
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

    function syncFileSlot(srcFilePath) {
        console.debug("folderPage syncFileSlot srcFilePath=" + srcFilePath);

        if (!cloudDriveModel.isAuthorized()) {
            // TODO implement for other cloud drive.
            messageDialog.message = "Files+ sync your files via Dropbox service.\
\nYou will redirect to authorization page.";
            messageDialog.titleText = "Sync with Dropbox";
            messageDialog.open();

            cloudDriveModel.requestToken(CloudDriveModel.Dropbox);
        } else {
            // TODO
            // [/] impl. in DropboxClient to store item(DropboxClient, uid, filePath, jsonObj(msg).rev) [Done on FilePutRely]
            // [/] On next metadata fetching. If rev is changed, sync to newer rev either put or get.
            // [ ] Syncing folder must queue each get/put jobs (by using ThreadPool).
            uidDialog.localPath = srcFilePath;
            uidDialog.open();
        }
    }

    function dropboxAccessTokenSlot() {
        cloudDriveModel.accessToken(CloudDriveModel.Dropbox);
    }

    FolderSizeItemListModel {
        id: fsModel
        currentDir: "C:/"

        onCurrentDirChanged: {
            console.debug("QML FolderSizeItemListModel::currentDirChanged");
            currentPath.text = fsModel.currentDir;

            refreshSlot();
        }

        onRefreshBegin: {
            console.debug("QML FolderSizeItemListModel::refreshBegin");
            window.state = "busy";
        }

        onDataChanged: {
//            console.debug("QML FolderSizeItemListModel::dataChanged");
        }

        onRefreshCompleted: {
            console.debug("QML FolderSizeItemListModel::refreshCompleted");
            window.state = "ready";
        }

        onRequestResetCache: {
            console.debug("QML FolderSizeItemListModel::onRequestResetCache");
            messageDialog.titleText = "First time loading";
            messageDialog.message = "Thank you for download FolderPie.\
\nThis is first time running, FolderPie needs to load information from your drive.\
\n\nIt will take time depends on numbers of sub folders/files under current folder.\
\n\nPlease click OK to continue.";
            messageDialog.open();

            resetCacheSlot();
        }
    }

    Flipable {
        id: flipable1
        width: parent.width
        height: parent.height - currentPath.height
        x: 0
        anchors.top: currentPath.bottom

        property bool flipped: false

        front: ListView {
            id: fsListView
            x: 0
            y: 0
            anchors.fill: parent
            highlightRangeMode: ListView.NoHighlightRange
            highlightFollowsCurrentItem: false
            highlight: Rectangle { color: "#00AAFF" }
            snapMode: ListView.SnapToItem
            clip: true
            focus: true
            model: fsModel
            delegate: listDelegate
        }

        back: PieChart {
            id: pieChart1
            x: 0
            y: 0
            width: parent.width - 4
            height: parent.height
            anchors.horizontalCenter: parent.horizontalCenter
            model: fsModel

            onChartClicked: {
                console.debug("QML pieChart1.onChartClicked");
            }
            onSliceClicked: {
                console.debug("QML pieChart1.onSliceClicked " + text + ", index=" + index + ", isDir=" + isDir);
                if (isDir)
                    fsModel.changeDir(text);
                else
                    fsModel.refreshDir(false);
            }
            onActiveFocusChanged: {
                console.debug("QML pieChart1.onActiveFocusChanged");
            }
            onSceneActivated: {
                console.debug("QML pieChart1.onSceneActivated");
                refreshSlot();
            }
            onSwipe: {
                console.debug("QML pieChart1.onSwipe " + swipeAngle);
                flipSlot();
            }
        }

        Component {
            id: listDelegate

            ListItem {
                id: listItem
                property real mouseX: 0
                property real mouseY: 0
                property string fileName: name
                property string filePath: absolutePath

                Image {
                    id: cutCopyIcon
                    x: 0
                    y: 0
                    z: 1
                    width: 32
                    height: 32
                    visible: (popupToolPanel.srcFilePath == absolutePath);
                    source: (popupToolPanel.isCopy) ? "copy.svg" : "trim.svg"
                }

                Image {
                    id: syncIcon
                    x: 0
                    z: 1
                    width: 32
                    height: 32
                    anchors.left: listItem.left
                    anchors.bottom: listItem.bottom
                    visible: cloudDriveModel.isConnected(absolutePath);
                    source: "refresh.svg"
                }

                Row {
                    id: listDelegateRow
                    anchors.fill: listItem.paddingItem
                    spacing: 5
                    Image {
                        id: icon1
                        width: 48
                        height: 48
                        sourceSize.width: 48
                        sourceSize.height: 48
                        source: (isDir) ? "folder.svg" : "notes.svg"
                    }
                    Column {
                        width: parent.width - icon1.width - 72
                        ListItemText {
                            mode: listItem.mode
                            role: "Title"
                            text: name
                            width: parent.width
                            verticalAlignment: Text.AlignVCenter
                        }
                        ListItemText {
                            id: listItemSubTitle
                            mode: listItem.mode
                            role: "SubTitle"
                            text: {
                                var sub = ""
                                if (subDirCount > 0) sub += subDirCount + " dir" + ((subDirCount > 1) ? "s" : "");
                                if (subFileCount > 0) sub += ((sub == "") ? "" : " ") + subFileCount + " file" + ((subFileCount > 1) ? "s" : "");
                                sub += ((sub == "") ? "" : ", ") + "last modified " + Qt.formatDateTime(lastModified, "d MMM yyyy h:mm:ss ap");

                                return sub;
                            }
                            width: parent.width
                            verticalAlignment: Text.AlignVCenter
                        }
                        ProgressBar {
                            id: syncProgressBar
                            width: parent.width
                            indeterminate: false
                            visible: false
                        }
                    }
                    ListItemText {
                        mode: listItem.mode
                        role: "Subtitle"
                        text: Utility.formatFileSize(size, 1)
                        width: 72
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                onPressAndHold: {
                    popupToolPanel.selectedFilePath = absolutePath;
                    popupToolPanel.selectedFileIndex = index;
                    popupToolPanel.forFile = !isDir;
                    popupToolPanel.pastePath = (isDir) ? absolutePath : currentPath.text;
                    var panelX = x + mouseX - fsListView.contentX + 30;
                    var panelY = y + mouseY - fsListView.contentY;
                    popupToolPanel.x = panelX;
                    popupToolPanel.y = panelY;
                    popupToolPanel.visible = true;
                }

                onClicked: {
                    if (isDir) {
                        fsModel.changeDir(name);
                    } else {
                        // TODO Implement internal viewers for image(JPG,PNG), text with addon(cloud drive, print)
                        Qt.openUrlExternally(fsModel.getUrl(absolutePath));
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onPressed: {
                        parent.mouseX = mouseX;
                        parent.mouseY = mouseY;
                        mouse.accepted = false;
                    }
                }

                Timer {
                    id: syncProgressBarTimer
                    interval: 1000
                    repeat: true
                    running: cloudDriveModel.isSyncing(absolutePath)
                    triggeredOnStart: true

                    onTriggered: {
                        var jsonText = cloudDriveModel.getFirstJobJson(absolutePath);
                        if (jsonText != "") {
                            var json = JSON.parse(jsonText);
                            listItemSubTitle.visible = !json.is_running;
                            syncProgressBar.visible = json.is_running;
                            syncProgressBar.value = json.bytes;
                            syncProgressBar.maximumValue = json.bytes_total;
                        }
                    }

                    onRunningChanged: {
                        if (!running) {
                            listItemSubTitle.visible = true;
                            syncProgressBar.visible = false;
                        }
                    }
                }
            }
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

    PopupToolPanel {
        id: popupToolPanel

        onPrintFile: {
            printFileSlot(srcFilePath);
        }

        onSyncFile: {
            syncFileSlot(srcFilePath);
        }
    }

    CommonDialog {
        id: fileDeleteDialog
        titleText: "Delete file"
        titleIcon: "FilesPlusIcon.svg"
        buttonTexts: ["Ok", "Cancel"]
        content: Text {
            color: "white"
            wrapMode: Text.WordWrap
            text: "Delete " + popupToolPanel.selectedFilePath + " ?"
        }

        onButtonClicked: {
            if (index === 0) {
                console.debug("fileDeleteDialog OK delete index " + popupToolPanel.selectedFileIndex);
                fsModel.removeRow(popupToolPanel.selectedFileIndex);
            }
        }
    }

    CommonDialog {
        id: fileActionDialog
        titleText: (popupToolPanel.isCopy) ? "Copy file" : "Move file"
        titleIcon: "FilesPlusIcon.svg"
        buttonTexts: ["Ok", "Cancel"]
        content: Text {
            color: "white"
            wrapMode: Text.WordWrap
            text: ((popupToolPanel.isCopy) ? "Copy " : "Move ") + popupToolPanel.srcFilePath + " to " + popupToolPanel.pastePath + " ?"
        }

        onButtonClicked: {
            if (index === 0) {
                console.debug("fileActionDialog OK isCopy " + popupToolPanel.isCopy + " from " + popupToolPanel.srcFilePath + " to " + popupToolPanel.pastePath);
                var res = fsModel.copyFile(popupToolPanel.srcFilePath, popupToolPanel.pastePath);
                if (res) {
                    // TODO if paste on currentPath's subFolder, it should use fsModel.removeRow.
                    if (!popupToolPanel.isCopy) fsModel.deleteFile(popupToolPanel.srcFilePath);
                    popupToolPanel.srcFilePath = "";
                    popupToolPanel.pastePath = "";

                    refreshSlot();
                    // TODO Remove folderSizeCache for source file's path and target path.
                } else {
                    messageDialog.titleText = (popupToolPanel.isCopy) ? "Copy" : "Move";
                    messageDialog.message = "I can't " + ((popupToolPanel.isCopy) ? "copy" : "move")
                            + " file " + popupToolPanel.srcFilePath
                            + " to " + popupToolPanel.pastePath + "\n"
                            + "The file is existing on target path.";
                    messageDialog.open();
                    popupToolPanel.pastePath = "";
                }
            }
        }
    }

    CommonDialog {
        id: messageDialog

        property alias message: contentText.text

        titleIcon: "FilesPlusIcon.svg"
        buttonTexts: ["Ok"]
        content: Text {
            id: contentText
            width: parent.width - 8
            color: "white"
            wrapMode: Text.WordWrap
            height: implicitHeight
            anchors.horizontalCenter: parent.horizontalCenter
        }

        onStatusChanged: {
            if (status == DialogStatus.Closed) {
                message = "";
            }
        }
    }

    TitlePanel {
        id: currentPath

        MouseArea {
            anchors.fill: parent

            onPressAndHold: {
                if (folderPage.state == "list") {
                    popupToolPanel.forFile = false;
                    popupToolPanel.selectedFilePath = currentPath.text
                    popupToolPanel.pastePath = currentPath.text;
                    var panelX = currentPath.x + mouseX;
                    var panelY = currentPath.y + mouseY;
                    popupToolPanel.x = panelX;
                    popupToolPanel.y = panelY;
                    popupToolPanel.visible = true;
                }
            }
        }
    }

    SelectionDialog {
        id: printerSelectionDialog

        property string srcFilePath

        titleText: "Print " + srcFilePath + " to"
        titleIcon: "FilesPlusIcon.svg"

        onAccepted: {
            // Print on selected printer index.
            var pid = gcpClient.getPrinterId(printerSelectionDialog.selectedIndex);
            console.debug("printerSelectionDialog onAccepted pid=" + pid + " srcFilePath=" + srcFilePath);
            if (pid != "") {
                // Open uploadProgressBar for printing.
                uploadProgressDialog.srcFilePath = srcFilePath;
                uploadProgressDialog.titleText = "Printing";
                uploadProgressDialog.autoClose = false;
                uploadProgressDialog.open();

                gcpClient.submit(pid, srcFilePath);
            }
        }

        onRejected: {
            // Reset popupToolPanel.
            popupToolPanel.selectedFilePath = "";
            popupToolPanel.selectedFileIndex = -1;
        }
    }

    CommonDialog {
        id: uploadProgressDialog

        property alias srcFilePath: uploadFilePath.text
        property alias message: uploadMessage.text
        property alias min: uploadProgressBar.minimumValue
        property alias max: uploadProgressBar.maximumValue
        property alias value: uploadProgressBar.value
        property alias indeterminate: uploadProgressBar.indeterminate
        property bool autoClose: true

        titleText: "Uploading"
        titleIcon: "FilesPlusIcon.svg"
        buttonTexts: ["OK"]
        content: Column {
            width: parent.width - 8
            spacing: 4
            anchors.horizontalCenter: parent.horizontalCenter

            Text {
                id: uploadFilePath
                width: parent.width
                color: "white"
                wrapMode: Text.WordWrap
            }
            ProgressBar {
                id: uploadProgressBar
                width: parent.width

                onValueChanged: {
                    uploadProgressText.text = value + " / " + maximumValue;
                    if (uploadProgressDialog.autoClose && value == maximumValue) {
                        uploadProgressDialog.close();
                    } else if (value == maximumValue) {
                        indeterminate = true;
                    }
                }
            }
            Text {
                id: uploadProgressText
                width: parent.width
                color: "grey"
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignRight
            }
            Rectangle {
                color: "grey"
                width: parent.width
                height: 1
                visible: (uploadMessage.text != "")
            }
            Text {
                id: uploadMessage
                width: parent.width
                color: "white"
                wrapMode: Text.WordWrap
            }
        }

        onButtonClicked: {
            uploadProgressDialog.close();
        }

        onStatusChanged: {
            if (status == DialogStatus.Closed) {
                srcFilePath = "";
                autoClose = true;
                message = "";
                indeterminate = false;
            }
        }
    }

    CommonDialog {
        id: downloadProgressDialog

        property alias targetFilePath: targetFilePath.text
        property alias message: downloadMessage.text
        property alias min: downloadProgressBar.minimumValue
        property alias max: downloadProgressBar.maximumValue
        property alias value: downloadProgressBar.value
        property alias indeterminate: downloadProgressBar.indeterminate
        property bool autoClose: true

        titleText: "Downloading"
        titleIcon: "FilesPlusIcon.svg"
        buttonTexts: ["OK"]
        content: Column {
            width: parent.width - 8
            spacing: 4
            anchors.horizontalCenter: parent.horizontalCenter

            Text {
                id: targetFilePath
                width: parent.width
                color: "white"
                wrapMode: Text.WordWrap
            }
            ProgressBar {
                id: downloadProgressBar
                width: parent.width

                onValueChanged: {
                    downloadProgressText.text = value + " / " + maximumValue;
                    if (downloadProgressDialog.autoClose && value == maximumValue) {
                        downloadProgressDialog.close();
                    } else if (value == maximumValue) {
                        indeterminate = true;
                    }
                }
            }
            Text {
                id: downloadProgressText
                width: parent.width
                color: "grey"
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignRight
            }
            Rectangle {
                color: "grey"
                width: parent.width
                height: 1
                visible: (downloadMessage.text != "")
            }
            Text {
                id: downloadMessage
                width: parent.width
                color: "white"
                wrapMode: Text.WordWrap
            }
        }

        onButtonClicked: {
            downloadProgressDialog.close();
        }

        onStatusChanged: {
            if (status == DialogStatus.Closed) {
                targetFilePath = "";
                autoClose = true;
                message = "";
                indeterminate = false;
            }
        }
    }

    GCPClient {
        id: gcpClient

        onAuthorizeRedirectSignal: {
            console.debug("folderPage gcpClient onAuthorizeRedirectSignal " + url);
            pageStack.push(Qt.resolvedUrl("AuthPage.qml"), { url: url, redirectFrom: "GCPClient" });
        }

        onAccessTokenReplySignal: {
            console.debug("folderPage gcpClient onAccessTokenReplySignal " + err + " " + errMsg + " " + msg);

            if (err == 0) {
                // Resume printing
                printFileSlot(popupToolPanel.selectedFilePath);
            } else {
                gcpClient.refreshAccessToken();
            }
        }

        onRefreshAccessTokenReplySignal: {
            console.debug("folderPage gcpClient onRefreshAccessTokenReplySignal " + err + " " + errMsg + " " + msg);

            if (err == 0) {
                // Resume printing if selectedFilePath exists.
                if (popupToolPanel.selectedFilePath != "") {
                    printFileSlot(popupToolPanel.selectedFilePath);
                } else {
                    messageDialog.titleText = "Reset CloudPrint"
                    messageDialog.message = "Resetting is done.";
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
                printFileSlot(popupToolPanel.selectedFilePath);
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
                message = "Error " + err + " " + errMsg + "\n";
            }
            message += jsonObj.message;

            // Show reply message on progressDialog and also turn indeterminate off.
            if (uploadProgressDialog.status != DialogStatus.Open) {
                uploadProgressDialog.titleText = "Printing";
                uploadProgressDialog.srcFilePath = popupToolPanel.selectedFilePath;
                uploadProgressDialog.autoClose = false;
                uploadProgressDialog.open();
            }
            uploadProgressDialog.indeterminate = false;
            uploadProgressDialog.message = message;

            // Reset popupToolPanel selectedFile.
            popupToolPanel.selectedFilePath = "";
            popupToolPanel.selectedFileIndex = -1;
        }

        onDownloadProgress: {
//            console.debug("sandBox gcpClient onDownloadProgress " + bytesReceived + " / " + bytesTotal);

            // Shows in progress bar.
            if (downloadProgressDialog.status != DialogStatus.Open) {
                downloadProgressDialog.indeterminate = false;
                downloadProgressDialog.open();
            }
            downloadProgressDialog.min = 0;
            downloadProgressDialog.max = bytesTotal;
            downloadProgressDialog.value = bytesReceived;
        }

        onUploadProgress: {
//            console.debug("sandBox gcpClient onUploadProgress " + bytesSent + " / " + bytesTotal);

            // Shows in progress bar.
            if (uploadProgressDialog.status != DialogStatus.Open) {
                uploadProgressDialog.indeterminate = false;
                uploadProgressDialog.open();
            }
            uploadProgressDialog.min = 0;
            uploadProgressDialog.max = bytesTotal;
            uploadProgressDialog.value = bytesSent;
        }
    }

    CloudDriveModel {
        id: cloudDriveModel

        onRequestTokenReplySignal: {
            console.debug("folderPage cloudDriveModel onRequestTokenReplySignal " + err + " " + errMsg + " " + msg);

            if (err == 0) {
                // TODO how to check if app has been authorized by user.
                cloudDriveModel.authorize(CloudDriveModel.Dropbox);
            } else {
                messageDialog.titleText = "CloudDrive Request Token"
                messageDialog.message = "Error " + err + " " + errMsg + " " + msg;
                messageDialog.open();
            }
        }

        onAuthorizeRedirectSignal: {
            console.debug("folderPage cloudDriveModel onAuthorizeRedirectSignal " + url + " redirectFrom " + redirectFrom);
            pageStack.push(Qt.resolvedUrl("AuthPage.qml"), { url: url, redirectFrom: redirectFrom });
        }

        onAccessTokenReplySignal: {
            console.debug("folderPage cloudDriveModel onAccessTokenReplySignal " + err + " " + errMsg + " " + msg);

            if (err == 0) {
                if (popupToolPanel.selectedFilePath) {
                    syncFileSlot(popupToolPanel.selectedFilePath);
                } else {
                    // TODO Get account info and show in dialog.
                    messageDialog.titleText = "CloudDrive Access Token"
                    messageDialog.message = "New user is authorized";
                    messageDialog.open();
                }
            } else {
                messageDialog.titleText = "CloudDrive Access Token"
                messageDialog.message = "Error " + err + " " + errMsg + " " + msg;
                messageDialog.open();
            }
        }

        onAccountInfoReplySignal: {
            console.debug("folderPage cloudDriveModel onAccountInfoReplySignal " + err + " " + errMsg + " " + msg);

            if (err == 0) {
                var jsonObj = Utility.createJsonObj(msg);
                console.debug("jsonObj.email " + jsonObj.email);
            } else {
                messageDialog.titleText = "CloudDrive Account Info"
                messageDialog.message = "Error " + err + " " + errMsg + " " + msg;
                messageDialog.open();
            }
        }

        onFileGetReplySignal: {
            console.debug("folderPage cloudDriveModel onFileGetReplySignal " + err + " " + errMsg + " " + msg);

            if (err == 0) {
//                var jsonObj = Utility.createJsonObj(msg);
//                var uid = uidDialog.selectedUid;
//                var localPath = popupToolPanel.selectedFilePath; // TODO remove dependency.
//                var localPathHash = cloudDriveModel.getItemHash(localPath, CloudDriveModel.Dropbox, uid);
//                var remotePath;
//                if (localPathHash == "") {
//                    remotePath = cloudDriveModel.getDefaultRemoteFilePath(localPath);
//                } else {
//                    remotePath = cloudDriveModel.getItemRemotePath(localPath, CloudDriveModel.Dropbox, uid);
//                }
//                cloudDriveModel.addItem(CloudDriveModel.Dropbox, uid, localPath, remotePath, jsonObj.rev);
            } else {
                messageDialog.titleText = "CloudDrive File Get"
                messageDialog.message = "Error " + err + " " + errMsg + " " + msg;
                messageDialog.open();
            }

            // Force refresh list view.
            fsModel.refreshItems();
        }

        onFilePutReplySignal: {
            console.debug("folderPage cloudDriveModel onFilePutReplySignal " + err + " " + errMsg + " " + msg);

            if (err == 0) {
//                var jsonObj = Utility.createJsonObj(msg);
//                var uid = uidDialog.selectedUid;
//                var localPath = popupToolPanel.selectedFilePath; // TODO remove dependency.
//                var localPathHash = cloudDriveModel.getItemHash(localPath, CloudDriveModel.Dropbox, uid);
//                var remotePath;

//                if (localPathHash == "") {
//                    remotePath = cloudDriveModel.getDefaultRemoteFilePath(localPath);
//                } else {
//                    remotePath = cloudDriveModel.getItemRemotePath(localPath, CloudDriveModel.Dropbox, uid);
//                }
//                cloudDriveModel.addItem(CloudDriveModel.Dropbox, uid, localPath, remotePath, jsonObj.rev);
            } else {
                messageDialog.titleText = "CloudDrive File Put"
                messageDialog.message = "Error " + err + " " + errMsg + " " + msg;
                messageDialog.open();
            }

            // Force refresh list view.
            fsModel.refreshItems();
        }

        onMetadataReplySignal: {
            console.debug("folderPage cloudDriveModel onMetadataReplySignal " + err + " " + errMsg + " " + msg);

            var uid = uidDialog.selectedUid;
            var localPath = popupToolPanel.selectedFilePath; // TODO remove dependency.
            var remotePath;

            if (err == 0) {
                var jsonObj = Utility.createJsonObj(msg);
                var localPathHash = cloudDriveModel.getItemHash(localPath, CloudDriveModel.Dropbox, uid);

                // localPath is already connected if localPathHash exists.
                if (localPathHash == "") {
                    remotePath = cloudDriveModel.getDefaultRemoteFilePath(localPath);
                } else {
                    remotePath = cloudDriveModel.getItemRemotePath(localPath, CloudDriveModel.Dropbox, uid);
                }

                // TODO Implement sync folder.
                if (jsonObj.is_dir) {
                    // Sync folder.
                    console.debug("Sync folder jsonObj.hash " + jsonObj.hash + " localPathHash " + localPathHash);
                    if (jsonObj.hash != localPathHash) {
                        var i;
                        var item;
                        for(i=0; i<jsonObj.contents.length; i++) {
                            item = jsonObj.contents[i];
                            if (item.is_dir) {
                                // TODO invoke metadata
                                // This flow will trigger recursive metadata calling.
                                console.debug("dir item = " + item);
                            } else {
                                // TODO invoke metadata
                                console.debug("file item = " + item);
//                                cloudDriveModel.metadata(CloudDriveModel.Dropbox, )
                            }
                        }
                    }
                } else {
                    // Sync file.
                    console.debug("Sync file jsonObj.rev " + jsonObj.rev + " localPathHash " + localPathHash);
                    if (jsonObj.rev > localPathHash) {
                        // TODO it should be specified with localPath instead of always use defaultLocalPath.
                        cloudDriveModel.fileGet(CloudDriveModel.Dropbox, uid, remotePath, localPath);
                    } else if (jsonObj.rev < localPathHash) {
                        cloudDriveModel.filePut(CloudDriveModel.Dropbox, uid, localPath, remotePath);
                    }
                }
            } else if (err == 203) {
                // If metadata is not found, put it to cloud right away.
                remotePath = cloudDriveModel.getDefaultRemoteFilePath(localPath);
                cloudDriveModel.filePut(CloudDriveModel.Dropbox, uid, localPath, remotePath);
            } else {
                messageDialog.titleText = "Dropbox Metadata"
                messageDialog.message = "Error " + err + " " + errMsg + " " + msg;
                messageDialog.open();
            }

            // Force refresh list view.
            fsModel.refreshItems();
        }

        onDownloadProgress: {
            console.debug("folderPage cloudDriveModel onDownloadProgress " + nonce + " " + bytesReceived + " / " + bytesTotal);

            // Shows in progress bar.
//            if (downloadProgressDialog.status != DialogStatus.Open) {
//                downloadProgressDialog.indeterminate = false;
//                downloadProgressDialog.open();
//            }
//            downloadProgressDialog.min = 0;
//            downloadProgressDialog.max = bytesTotal;
//            downloadProgressDialog.value = bytesReceived;
        }

        onUploadProgress: {
            console.debug("folderPage cloudDriveModel onUploadProgress " + nonce + " " + bytesSent + " / " + bytesTotal);

            // Shows in progress bar.
//            if (uploadProgressDialog.status != DialogStatus.Open) {
//                uploadProgressDialog.indeterminate = false;
//                uploadProgressDialog.open();
//            }
//            uploadProgressDialog.min = 0;
//            uploadProgressDialog.max = bytesTotal;
//            uploadProgressDialog.value = bytesSent;
        }
    }

    function getUidListModel(localPath) {
        console.debug("getUidListModel localPath " + localPath);

        // TODO Get uid list from GDClient.

        // Get uid list from DropboxClient.
        var dbUidList = cloudDriveModel.getStoredUidList(CloudDriveModel.Dropbox);

        // Construct model.
        var model = Qt.createQmlObject(
                    'import QtQuick 1.1; ListModel {}', folderPage);

        for (var i=0; i<dbUidList.length; ++i)
        {
            model.append({ type: CloudDriveModel.Dropbox,
                             uid: dbUidList[i],
                             hash: cloudDriveModel.getItemHash(localPath, CloudDriveModel.Dropbox, dbUidList[i])
                         });
        }

        return model;
    }

    function getCloudIcon(type) {
        switch (type) {
        case CloudDriveModel.Dropbox:
            return "dropbox_icon.png";
        case CloudDriveModel.GoogleDrive:
            return "drive_icon.png";
        }
    }

    SelectionDialog {
        id: uidDialog

        property string localPath
        property int selectedCloudType
        property string selectedUid

        titleText: "Please select Cloud Account"
        delegate: ListItem {
            id: uidDialogListViewItem
            Row {
                anchors.fill: uidDialogListViewItem.paddingItem
                Image {
                    width: 48
                    height: 48
                    source: getCloudIcon(type)
                }
                ListItemText {
                    width: parent.width - 120
                    mode: uidDialogListViewItem.mode
                    role: "SubTitle"
                    text: uid
                }
                ListItemText {
                    width: 60
                    mode: uidDialogListViewItem.mode
                    role: "Title"
                    text: (hash != "") ? "Sync" : ""
                }
            }

            onClicked: {
                uidDialog.selectedIndex = index;
                uidDialog.selectedCloudType = uidDialog.model.get(uidDialog.selectedIndex).type;
                uidDialog.selectedUid = uidDialog.model.get(uidDialog.selectedIndex).uid;
                uidDialog.accept();
            }
        }

        onAccepted: {
            // TODO Proceed for GoogleDrive
            // Proceed for Dropbox
            console.debug("uidDialog.selectedIndex " + uidDialog.selectedIndex);
            console.debug("uidDialog type " + uidDialog.selectedCloudType + " uid " + uidDialog.selectedUid + " localPath " + localPath);
            var remoteFilePath = cloudDriveModel.getDefaultRemoteFilePath(localPath);
            console.debug("uidDialog remoteFilePath " + remoteFilePath);
            cloudDriveModel.metadata(uidDialog.selectedCloudType, uidDialog.selectedUid, remoteFilePath);
        }

        onStatusChanged: {
            if (status == DialogStatus.Open) {
                uidDialog.model = getUidListModel(localPath);
            }
        }
    }
}
