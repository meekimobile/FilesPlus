import QtQuick 1.1
import com.nokia.symbian 1.1
import Charts 1.0
import FolderSizeItemListModel 1.0
import GCPClient 1.0
import CloudDriveModel 1.0
import AppInfo 1.0
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
            fsModel.sortFlag = FolderSizeItemListModel.SortBySize;
        }
    }

//    onStatusChanged: {
//        if (status == PageStatus.Active) {
//            // Show/Hide flipButton according to setting.
//            flipButton.visible = fsListView.state == "" && appInfo.getSettingValue("FolderPie.enabled", true);
//        }
//    }

    Component.onCompleted: {
        console.debug(Utility.nowText() + " folderPage onCompleted");
    }

    tools: toolBarLayout

    ToolBarLayout {
        id: toolBarLayout

        ToolButton {
            id: backButton
            iconSource: "toolbar-back"
            flat: true
            onClicked: {
                if (fsListView.state == "mark") {
                    fsListView.state = "";
                    fsListView.unmarkAll();
                } else {
                    goUpSlot();
                }
            }
        }

        ToolButton {
            id: refreshButton
            iconSource: "toolbar-refresh"
            flat: true
            visible: (fsListView.state == "")

            Component.onCompleted: {
                refreshButton.clicked.connect(refreshSlot);
            }
        }

        ToolButton {
            id: flipButton
            iconSource: (folderPage.state != "list") ? "list.svg" : "chart.svg"
            flat: true
            visible: (fsListView.state == "")

            Component.onCompleted: {
                flipButton.clicked.connect(flipSlot);
            }
        }

        ToolButton {
            id: menuButton
            iconSource: "toolbar-menu"
            flat: true
            onClicked: {
                if (fsListView.state == "mark") {
                    markMenu.open();
                } else {
                    mainMenu.open();
                }
            }
        }
    }

    AppInfo {
        id: appInfo
        domain: "MeekiMobile"
        app: "FilesPlus"
    }

    MainMenu {
        id: mainMenu

        onQuit: {
            if (fsModel.isRunning()) {
                messageDialog.titleText = "Notify";
                messageDialog.message = "Reset Cache is running. Please wait until it's done.";
                messageDialog.open();
            } else {
                Qt.quit();
            }
        }
        onPaste: {
            fileActionDialog.targetPath = fsModel.currentDir;
            fileActionDialog.open();
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

    SettingMenu {
        id: settingMenu
        onResetCache: {
            resetCacheSlot();
        }
        onResetCloudPrint: {
            resetCloudPrintSlot();
        }
        onRegisterDropboxUser: {
            registerDropboxUserSlot();
        }
        onShowCloudPrintJobs: {
            showCloudPrintJobsSlot();
        }
    }

    MarkMenu {
        id: markMenu
    }

    ConfirmDialog {
        id: resetCacheConfirmation
        titleText: "Reset folder cache"
        contentText: "Resetting folder cache will take time depends on numbers of sub folders/files under current folder.\n\nPlease click OK to continue."
        onConfirm: {
            fsModel.refreshDir(true);
        }
    }

    function refreshSlot() {
        fsModel.nameFilters = [];
        fsModel.refreshDir(false);
    }

    function resetCacheSlot() {
        resetCacheConfirmation.open();
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
        if (pieChartView && folderPage.state == "chart") {
            pieChartView.refreshItems();
        }
    }

    function printFileSlot(srcFilePath, selectedIndex) {
        console.debug("folderPage printFileSlot srcFilePath=" + srcFilePath);

        // Set source file to GCPClient.
        gcpClient.selectedFilePath = srcFilePath;
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
            messageDialog.message = "FilesPlus print via Google CloudPrint service.\
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

    function syncFileSlot(srcFilePath, selectedIndex, operation) {
        console.debug("folderPage syncFileSlot srcFilePath=" + srcFilePath);

        // Default operation = CloudDriveModel.Metadata
        if (!operation) operation = CloudDriveModel.Metadata;

        if (!cloudDriveModel.isAuthorized()) {
            // TODO implement for other cloud drive.
            messageDialog.message = "Files+ sync your files via Dropbox service.\
\nYou will redirect to authorization page.";
            messageDialog.titleText = "Sync with Dropbox";
            messageDialog.open();

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

    function uploadFileSlot(srcFilePath, selectedIndex) {
        console.debug("folderPage uploadFileSlot srcFilePath=" + srcFilePath);
        syncFileSlot(srcFilePath, selectedIndex, CloudDriveModel.FilePut);
    }

    function dropboxAccessTokenSlot() {
        cloudDriveModel.accessToken(CloudDriveModel.Dropbox);
    }

    function resetCloudPrintSlot() {
        popupToolPanel.selectedFilePath = "";
        popupToolPanel.selectedFileIndex = -1;
        gcpClient.authorize();
    }

    function registerDropboxUserSlot() {
        cloudDriveModel.requestToken(CloudDriveModel.Dropbox);
    }

    function showCloudDriveAccountsSlot() {
        pageStack.push(Qt.resolvedUrl("CloudDriveAccountsPage.qml"),
                       { cloudDriveModel: cloudDriveModel },
                       false);
    }

    function showCloudPrintJobsSlot() {
        pageStack.push(Qt.resolvedUrl("PrintJobsPage.qml"));
    }

    function showCloudDriveJobsSlot() {
        pageStack.push(Qt.resolvedUrl("CloudDriveJobsPage.qml"), { }, false);
    }

    function cancelAllCloudDriveJobsSlot() {
        cancelQueuedCloudDriveJobsConfirmation.open();
    }

    function cancelAllFolderSizeJobsSlot() {
        cancelQueuedFolderSizeJobsConfirmation.open();
    }

    function syncAllConnectedItemsSlot() {
        cloudDriveModel.syncItems();
    }

    FolderSizeItemListModel {
        id: fsModel
        currentDir: "C:/"
        sortFlag: FolderSizeItemListModel.SortByType

        function getActionName(fileAction) {
            switch (fileAction) {
            case FolderSizeItemListModel.CopyFile:
                return "Copy";
                break;
            case FolderSizeItemListModel.MoveFile:
                return "Move";
                break;
            }
        }

        onCurrentDirChanged: {
            console.debug("QML FolderSizeItemListModel::currentDirChanged");
            currentPath.text = fsModel.currentDir;
        }

        onRefreshBegin: {
            console.debug("QML FolderSizeItemListModel::refreshBegin");
//            window.state = "busy";
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
        }

        onRequestResetCache: {
            console.debug("QML FolderSizeItemListModel::onRequestResetCache");
            messageDialog.titleText = "First time loading";
            messageDialog.message = "Thank you for download FilesPlus.\
\nThis is first time running, FolderPie needs to load information from your drive.\
\n\nIt will take time depends on numbers of sub folders/files under current folder.\
\n\nPlease click OK to continue.";
            messageDialog.open();

            fsModel.refreshDir(true);
        }

        onCopyStarted: {
            console.debug("folderPage fsModel onCopyStarted " + fileAction + " from " + sourcePath + " to " + targetPath + " " + err + " " + msg);
            // TODO Reopen copyProgressDialog if it's not opened.
            var sourceFileName = fsModel.getFileName(sourcePath);
            var targetFileName = fsModel.getFileName(targetPath);
            var targetDirPath = fsModel.getDirPath(targetPath);
            copyProgressDialog.source = getActionName(fileAction) + " " + sourceFileName;
            copyProgressDialog.target = "to " + ((sourceFileName == targetFileName) ? targetDirPath : targetFileName);
            copyProgressDialog.lastValue = 0;
            copyProgressDialog.indeterminate = false;
            if (copyProgressDialog.status != DialogStatus.Open) {
                copyProgressDialog.formatValue = true;
                copyProgressDialog.open();
            }
        }

        onCopyProgress: {
//            console.debug("folderPage fsModel onCopyProgress " + fileAction + " from " + sourcePath + " to " + targetPath + " " + bytes + " / " + bytesTotal);
            copyProgressDialog.newValue = bytes;
        }

        onCopyFinished: {
            console.debug("folderPage fsModel onCopyFinished " + fileAction + " from " + sourcePath + " to " + targetPath + " " + err + " " + msg + " " + bytes + " " + totalBytes);
            copyProgressDialog.count += 1;

            // Show message if error.
            if (err < 0) {
                messageDialog.titleText = "Copy error";
                messageDialog.message = msg;
                messageDialog.autoClose = true;
                messageDialog.open();

                copyProgressDialog.message += "Copy " + sourcePath + " is failed.\n";

                // TODO stop queued jobs
                // TODO Reset popupToolPanel and clipboard ?
            } else {
                // Reset popupToolPanel
                popupToolPanel.srcFilePath = "";
                popupToolPanel.pastePath = "";

                // Remove finished sourcePath from clipboard.
                clipboard.clear();
            }
        }

        onDeleteStarted: {
            console.debug("folderPage fsModel onDeleteStarted " + sourcePath);
            deleteProgressDialog.source = sourcePath;
            deleteProgressDialog.indeterminate = false;
            if (deleteProgressDialog.status != DialogStatus.Open) {
                deleteProgressDialog.titleText = "Deleting";
                deleteProgressDialog.open();
            }
        }

        onDeleteFinished: {
            console.debug("folderPage fsModel onDeleteFinished " + sourcePath);
            deleteProgressDialog.value += 1;

            // Reset cloudDriveModel hash on parent.
            var paths = fsModel.getPathToRoot(sourcePath);
            for (var i=0; i<paths.length; i++) {
//                console.debug("folderPage fsModel onDeleteFinished updateItems paths[" + i + "] " + paths[i]);
                cloudDriveModel.updateItems(CloudDriveModel.Dropbox, paths[i], cloudDriveModel.dirtyHash);
            }
        }

        onCreateFinished: {
            console.debug("folderPage fsModel onCreateFinished " + targetPath);
            // Reset cloudDriveModel hash on parent.
            // TODO reset has upto root.
            var paths = fsModel.getPathToRoot(targetPath);
            for (var i=0; i<paths.length; i++) {
                console.debug("folderPage fsModel onCreateFinished updateItems paths[" + i + "] " + paths[i]);
                cloudDriveModel.updateItems(CloudDriveModel.Dropbox, paths[i], cloudDriveModel.dirtyHash);
            }

            // TODO request cloudDriveModel.createFolder
        }

        onFetchDirSizeUpdated: {
            if (!refreshButton.checked) refreshButton.checked = true;
            refreshButton.rotation = 360 + (refreshButton.rotation - 6);
        }

        onFetchDirSizeFinished: {
            refreshButton.rotation = 0;

            // Refresh itemList to show changes on ListView.
            fsModel.refreshItems();
        }
    }

    function getImageSourcesModel(jsonText, fileName) {
//        console.debug("getImageSourcesModel jsonText " + jsonText + " fileName " + fileName);

        var supportedFileTypes = ["JPG", "PNG", "SVG"];

        // Construct model.
        var model = Qt.createQmlObject(
                    'import QtQuick 1.1; ListModel {}', folderPage);

        if (jsonText != "") {
            var json = JSON.parse(jsonText);

            for (var i=0; i<json.length; i++)
            {
//                console.debug("getImageSourcesModel i = " + i + " json[i].absolute_path " + json[i].absolute_path);
                if (json[i].is_dir) {
                    // skip dir
                } else if (supportedFileTypes.indexOf(json[i].file_type.toUpperCase()) != -1) {
                    console.debug("getImageSourcesModel model.append " + json[i].absolute_path);
                    model.append({
                                     name: json[i].name,
                                     absolutePath: json[i].absolute_path,
                                     isSelected: (fileName == json[i].name)
                                 });
                }
            }
        }

        return model;
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

        onChartClicked: {
            console.debug("QML pieChartView.onChartClicked");
        }
        onSliceClicked: {
            console.debug("QML pieChartView.onSliceClicked " + text + ", index=" + index + ", isDir=" + isDir);
            if (isDir)
                fsModel.changeDir(text);
            else
                fsModel.refreshDir(false);
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
            gradient: Gradient {
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
        }
        clip: true
        focus: true
        model: fsModel
        delegate: listDelegate
        states: [
            State {
                name: "mark"
            }
        ]

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

        function unmarkAll() {
            for (var i=0; i<model.count; i++) {
                model.setProperty(i, FolderSizeItemListModel.IsCheckedRole, false);
            }
        }

        function cutMarkedItems() {
            for (var i=0; i<model.count; i++) {
                if (model.getProperty(i, FolderSizeItemListModel.IsCheckedRole)) {
                    console.debug("fsListView cutMarkedItems item"
                                  + " absolutePath " + model.getProperty(i, FolderSizeItemListModel.AbsolutePathRole)
                                  + " isChecked " + model.getProperty(i, FolderSizeItemListModel.IsCheckedRole));

                    clipboard.addItem({ "action": "cut", "sourcePath": model.getProperty(i, FolderSizeItemListModel.AbsolutePathRole) });
                }

                // Reset isChecked.
                model.setProperty(i, FolderSizeItemListModel.IsCheckedRole, false);
            }
        }

        function copyMarkedItems() {
            for (var i=0; i<model.count; i++) {
                if (model.getProperty(i, FolderSizeItemListModel.IsCheckedRole)) {
                    console.debug("fsListView copyMarkedItems item"
                                  + " absolutePath " + model.getProperty(i, FolderSizeItemListModel.AbsolutePathRole)
                                  + " isChecked " + model.getProperty(i, FolderSizeItemListModel.IsCheckedRole));

                    clipboard.addItem({ "action": "copy", "sourcePath": model.getProperty(i, FolderSizeItemListModel.AbsolutePathRole) });
                }

                // Reset isChecked.
                model.setProperty(i, FolderSizeItemListModel.IsCheckedRole, false);
            }
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
            // Always clear clipboard before delete marked items.
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

    }

    Component {
        id: listDelegate

        ListItem {
            id: listItem
            property real mouseX: 0
            property real mouseY: 0
            property string fileName: name
            property string filePath: absolutePath
            property int clipboardIndex: clipboard.getModelIndex(absolutePath)

            function getIconSource() {
                var viewableImageFileTypes = ["JPG", "PNG", "SVG"];
                var viewableTextFileTypes = ["TXT", "HTML"];

                if (isDir) {
                    return "folder.svg";
                } else if (viewableImageFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                    return "photos.svg";
                } else {
                    return "notes.svg";
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
                        visible: (listItem.clipboardIndex != -1)
                        source: clipboard.getActionIcon(listItem.clipboardIndex)
                    }
                    Image {
                        id: markIcon
                        z: 1
                        width: 32
                        height: 32
                        anchors.left: parent.left
                        anchors.bottom: parent.bottom
                        visible: (fsListView.state == "mark" && isChecked)
                        source: "check_mark.svg"
                    }
                    Image {
                        id: icon1
                        width: 48
                        height: 48
                        fillMode: Image.PreserveAspectFit
                        anchors.centerIn: parent
                        source: listItem.getIconSource()
                    }
                }
                Column {
                    width: parent.width - iconRect.width
                    height: parent.height

                    Row {
                        width: parent.width
                        height: parent.height / 2
                        ListItemText {
                            mode: listItem.mode
                            role: "Title"
                            text: name
                            width: parent.width - sizeText.width
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter
                        }
                        ListItemText {
                            id: sizeText
                            mode: listItem.mode
                            role: "Subtitle"
                            text: Utility.formatFileSize(size, 1)
                            width: 80
                            height: parent.height
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
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
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                                visible: !isRunning
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
                                            return "refresh.svg"
                                        case FolderSizeItemListModel.UploadOperation:
                                            return "upload.svg"
                                        case FolderSizeItemListModel.DownloadOperation:
                                            return "download.svg"
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
                            width: 32
                            height: parent.height
                            fillMode: Image.PreserveAspectFit
                            z: 1
                            visible: cloudDriveModel.isConnected(absolutePath);
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
                    popupToolPanel.forFile = !isDir;
                    popupToolPanel.pastePath = (isDir) ? absolutePath : currentPath.text;
                    var panelX = x + mouseX - fsListView.contentX;
                    var panelY = y + mouseY - fsListView.contentY + flipable1.y;
                    popupToolPanel.open(panelX, panelY);
                }
            }

            onClicked: {
                if (fsListView.state == "mark") {
                    if (listItem.clipboardIndex == -1) {
                        fsModel.setProperty(index, FolderSizeItemListModel.IsCheckedRole, !isChecked);
                    }
                } else {
                    if (isDir) {
                        fsModel.changeDir(name);
                    } else {
                        // If file is running, disable preview.
                        if (isRunning) return;

                        // Implement internal viewers for image(JPG,PNG), text with addon(cloud drive, print)
                        var viewableImageFileTypes = ["JPG", "PNG", "SVG"];
                        var viewableTextFileTypes = ["TXT", "HTML", "LOG", "CSV"];
                        if (viewableImageFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                            fsModel.nameFilters = ["*.jpg", "*.png", "*.svg"];
                            fsModel.refreshDir(false);
                            pageStack.push(Qt.resolvedUrl("ImageViewPage.qml"), {
                                               fileName: name,
                                               model: fsModel
                                           });
                        } else if (viewableTextFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                            pageStack.push(Qt.resolvedUrl("TextViewPage.qml"),
                                           { filePath: absolutePath });
                        } else {
                            Qt.openUrlExternally(fsModel.getUrl(absolutePath));
                        }
                    }
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
        }
    }

    TitlePanel {
        id: currentPath

//        MouseArea {
//            anchors.fill: parent

//            onPressAndHold: {
//                if (folderPage.state == "list") {
//                    popupToolPanel.forFile = false;
//                    popupToolPanel.selectedFilePath = currentPath.text
//                    popupToolPanel.pastePath = currentPath.text;
//                    var panelX = currentPath.x + mouseX;
//                    var panelY = currentPath.y + mouseY;
//                    popupToolPanel.x = panelX;
//                    popupToolPanel.y = panelY;
//                    popupToolPanel.visible = true;
//                }
//            }
//        }
    }

    MessageDialog {
        id: messageDialog
    }

    ClipboardModel {
        id: clipboard
    }

    PopupToolRing {
        id: popupToolPanel
        ringRadius: 65
        buttonRadius: 25
        clipboardCount: clipboard.count

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
            clipboard.addItem({ "action": "delete", "sourcePath": sourcePath });
            fileActionDialog.open();
        }

        onPrintFile: {
            printFileSlot(srcFilePath, srcItemIndex);
        }

        onSyncFile: {
            syncFileSlot(srcFilePath, srcItemIndex);
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
    }

    ConfirmDialog {
        id: fileActionDialog

        property string targetPath

        titleText: fileActionDialog.getTitleText()
        contentText: fileActionDialog.getText()

        function getTitleText() {
            var text = "";
            if (clipboard.count == 1) {
                text = getActionName(clipboard.get(0).action);
            } else {
                // TODO if all copy, show "Multiple copy".
                text = "Multiple actions";
            }

            return text;
        }

        function getActionName(action) {
            if (action == "copy") return "Copy";
            else if (action == "cut") return "Move";
            else if (action == "delete") return "Delete";
            else return "Invalid action";
        }

        function getText() {
            // Exmaple of clipboard entry { "action": "cut", "sourcePath": sourcePath }
            var text = "";
            if (clipboard.count == 1) {
                text = getActionName(clipboard.get(0).action)
                        + "\nfile " + clipboard.get(0).sourcePath
                        + ((clipboard.get(0).action == "delete")?"":("\nto " + targetPath))
                        + " ?";
            } else {
                var cutCount = 0;
                var copyCount = 0;
                var deleteCount = 0;
                for (var i=0; i<clipboard.count; i++) {
                    console.debug("fileActionDialog getText clipboard i " + i + " action " + clipboard.get(i).action + " sourcePath " + clipboard.get(i).sourcePath);
                    if (clipboard.get(i).action == "copy") {
                        copyCount++;
                    } else if (clipboard.get(i).action == "cut") {
                        cutCount++;
                    } else if (clipboard.get(i).action == "delete") {
                        deleteCount++;
                    }
                }

                if (deleteCount>0) text = text + ("Delete " + deleteCount + " file" + ((deleteCount>1)?"s":"") + "\n");
                if (copyCount>0) text = text + ("Copy " + copyCount + " file" + ((copyCount>1)?"s":"") + "\n");
                if (cutCount>0) text = text + ("Move " + cutCount + " file" + ((cutCount>1)?"s":"") + "\n");
                if (copyCount>0 || cutCount>0) text = text + "to " + targetPath;
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
                console.debug("fileActionDialog openCopyProgressDialog estimate total action " + action + " sourcePath " + sourcePath);
                if (action == "copy" || action == "cut") {
                    var jsonText = fsModel.getItemJson(sourcePath);
                    console.debug("fileActionDialog openCopyProgressDialog estimate total jsonText " + jsonText);
                    var itemJson = Utility.createJsonObj(jsonText);
                    console.debug("fileActionDialog openCopyProgressDialog estimate total itemJson " + itemJson + " itemJson.size " + itemJson.size + " itemJson.sub_file_count " + itemJson.sub_file_count);
                    totalBytes += itemJson.size;
                    totalFiles += itemJson.sub_file_count + 1;  // +1 for itself.
                    totalFolders += itemJson.sub_dir_count;
                }
            }
            console.debug("fileActionDialog openCopyProgressDialog estimate totalBytes " + totalBytes + " totalFiles " + totalFiles + " totalFolders " + totalFolders);
            if (totalBytes > 0) {
                // Open ProgressDialog.
                copyProgressDialog.min = 0;
                copyProgressDialog.max = totalBytes;
                copyProgressDialog.value = 0;
                copyProgressDialog.minCount = 0;
                copyProgressDialog.maxCount = totalFiles + totalFolders;
                copyProgressDialog.count = 0;
                copyProgressDialog.indeterminate = true;
                copyProgressDialog.autoClose = false;
                copyProgressDialog.formatValue = true;
                copyProgressDialog.titleText = fileActionDialog.titleText;
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
                console.debug("fileActionDialog openDeleteProgressDialog estimate total action " + action + " sourcePath " + sourcePath);
                if (action == "delete") {
                    var jsonText = fsModel.getItemJson(sourcePath);
                    console.debug("fileActionDialog openDeleteProgressDialog estimate total jsonText " + jsonText);
                    var itemJson = Utility.createJsonObj(jsonText);
                    console.debug("fileActionDialog openDeleteProgressDialog estimate total itemJson " + itemJson + " itemJson.sub_file_count " + itemJson.sub_file_count);
                    totalFiles += itemJson.sub_file_count + ((itemJson.is_dir) ? 0 : 1);  // +1 for file.
                    totalFolders += itemJson.sub_dir_count;
                }
            }
            console.debug("fileActionDialog openDeleteProgressDialog estimate totalFiles " + totalFiles + " totalFolders " + totalFolders);
            if (totalFiles > 0) {
                deleteProgressDialog.titleText = "Deleting";
                deleteProgressDialog.min = 0;
                deleteProgressDialog.max = totalFiles;
                deleteProgressDialog.value = 0;
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
                    messageDialog.message = "I can't " + getActionName(clipboard.get(0).action).toLowerCase()
                            + "\nfile " + clipboard.get(0).sourcePath
                            + "\nto " + targetPath;
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
        titleText: "New Folder"
        titleIcon: "FilesPlusIcon.svg"
        buttonTexts: ["Ok", "Cancel"]
        content: Rectangle {
            anchors.margins: 5
            anchors.fill: parent
            color: "transparent"

            TextField {
                id: folderName
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                placeholderText: "Please input folder name."
            }
        }

        onStatusChanged: {
            if (status == DialogStatus.Open) {
                folderName.forceActiveFocus();
            }

            if (status == DialogStatus.Closed) {
                folderName.text = "";
            }
        }

        onButtonClicked: {
            if (index === 0) {
                var res = fsModel.createDir(folderName.text);
                refreshSlot();
            }
        }
    }

    CommonDialog {
        id: renameDialog

        property string sourcePath

        titleText: "Rename"
        titleIcon: "FilesPlusIcon.svg"
        buttonTexts: ["Ok", "Cancel"]
        content: Column {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - 10
            spacing: 3

            Text {
                width: parent.width
                text: fsModel.getFileName(renameDialog.sourcePath) + "\nto";
                color: "white"
                horizontalAlignment: Text.AlignHCenter
            }

            TextField {
                id: newName
                width: parent.width
                placeholderText: "Please input new name."
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
            if (index === 0) {
                var res = fsModel.renameFile(fsModel.getFileName(renameDialog.sourcePath), newName.text);
                refreshSlot();
            }
        }
    }

    CommonDialog {
        id: fileOverwriteDialog
        titleText: "File overwrite"
        titleIcon: "FilesPlusIcon.svg"
        buttonTexts: ["Ok", "Cancel"]
        content: Column {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - 10
            spacing: 3

            Text {
                text: "Please input new file name"
                color: "white"
                height: 48
                verticalAlignment: Text.AlignVCenter
            }

            TextField {
                id: fileName
                width: parent.width
            }

            CheckBox {
                id: overwriteFile
                text: "Overwrite existing file"
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
            }
        }
    }

    ConfirmDialog {
        id: cancelQueuedCloudDriveJobsConfirmation
        titleText: "Cancel sync jobs"
        onOpening: {
            contentText = "Cancel " + cloudDriveModel.getQueuedJobCount() + " jobs ?";
        }
        onConfirm: {
            cloudDriveModel.cancelQueuedJobs();
        }
    }

    ConfirmDialog {
        id: cancelQueuedFolderSizeJobsConfirmation
        titleText: "Cancel file action jobs"
        content: Column {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - 20
            spacing: 3
                Text {
                id: content
                color: "white"
                font.pixelSize: 18
                wrapMode: Text.Wrap
                width: parent.width - 20
                height: implicitHeight
                anchors.horizontalCenter: parent.horizontalCenter
            }
            CheckBox {
                id: rollbackFlag
                text: "Rollback changes"
                checked: false
            }
        }
        onOpening: {
            var jobCount = fsModel.getQueuedJobCount();
            if (jobCount > 0) {
                content.text = "Cancel " + jobCount + " jobs and abort file action ?";
            } else {
                content.text = "Abort file action ?";
            }
            rollbackFlag.checked = false;
        }
        onConfirm: {
            fsModel.cancelQueuedJobs();
            // Abort thread with rollbackFlag.
            fsModel.abortThread(rollbackFlag.checked);
        }
    }

    SelectionDialog {
        id: printerSelectionDialog

        property string srcFilePath

        titleText: "Print " + fsModel.getFileName(srcFilePath) + " to"
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

    UploadProgressDialog {
        id: uploadProgressDialog
    }

    DownloadProgressDialog {
        id: downloadProgressDialog
    }

    ProgressDialog {
        id: copyProgressDialog
        onClosed: {
            // Refresh view after copied/moved.
            refreshSlot();
        }
        onCancelled: {
            cancelAllFolderSizeJobsSlot();
        }
    }

    ProgressDialog {
        id: deleteProgressDialog
        titleText: "Deleting"
        onClosed: {
            // Refresh view after copied/moved.
            refreshSlot();
        }
        onCancelled: {
            cancelAllFolderSizeJobsSlot();
        }
    }

    GCPClient {
        id: gcpClient

        property string selectedFilePath

        onAuthorizeRedirectSignal: {
            console.debug("folderPage gcpClient onAuthorizeRedirectSignal " + url);
            pageStack.push(Qt.resolvedUrl("AuthPage.qml"), { url: url, redirectFrom: "GCPClient" });
        }

        onAccessTokenReplySignal: {
            console.debug("folderPage gcpClient onAccessTokenReplySignal " + err + " " + errMsg + " " + msg);

            if (err == 0) {
                // Resume printing
                printFileSlot(gcpClient.selectedFilePath);
            } else {
                gcpClient.refreshAccessToken();
            }
        }

        onRefreshAccessTokenReplySignal: {
            console.debug("folderPage gcpClient onRefreshAccessTokenReplySignal " + err + " " + errMsg + " " + msg);

            if (err == 0) {
                // Resume printing if selectedFilePath exists.
                if (gcpClient.selectedFilePath != "") {
                    printFileSlot(gcpClient.selectedFilePath);
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
                printFileSlot(gcpClient.selectedFilePath);
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
            gcpClient.selectedFilePath = "";
            popupToolPanel.selectedFilePath = "";
            popupToolPanel.selectedFileIndex = -1;
        }

        onDownloadProgress: {
//            console.debug("sandBox gcpClient onDownloadProgress " + bytesReceived + " / " + bytesTotal);

            // Shows in progress bar.
            if (downloadProgressDialog.status != DialogStatus.Open) {
                downloadProgressDialog.titleText = "Searching for printers"
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
                return FolderSizeItemListModel.NoOperation;
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

            for (var i=0; i<dbUidList.length; i++)
            {
                var json = JSON.parse(dbUidList[i]);
                model.append({
                                 type: CloudDriveModel.Dropbox,
                                 uid: json.uid,
                                 email: json.email,
                                 hash: cloudDriveModel.getItemHash(localPath, CloudDriveModel.Dropbox, json.uid),
                                 name: "",
                                 shared: 0,
                                 normal: 0,
                                 quota: 0
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

        function getCloudName(type) {
            switch (type) {
            case CloudDriveModel.Dropbox:
                return "Dropbox";
            case CloudDriveModel.GoogleDrive:
                return "GoogleDrive";
            }
        }

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
                // TODO find better way to proceed after got accessToken.
                if (popupToolPanel.selectedFilePath) {
                    uidDialog.proceedPendingOperation();
//                    syncFileSlot(popupToolPanel.selectedFilePath, popupToolPanel.selectedFileIndex);
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

            var jsonText = cloudDriveModel.getJobJson(nonce);
            var cloudDriveJobJson = JSON.parse(jsonText);

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                /*
{
    "referral_link": "https://www.dropbox.com/referrals/r1a2n3d4m5s6t7",
    "display_name": "John P. User",
    "uid": 12345678,
    "country": "US",
    "quota_info": {
        "shared": 253738410565,
        "quota": 107374182400000,
        "normal": 680031877871
    }
}
                  */
                var jsonObj = Utility.createJsonObj(msg);
                console.debug("jsonObj.email " + jsonObj.email);

                // Send info to cloudDriveAccountsPage.
                var p = pageStack.find(function (page) { return (page.name == "cloudDriveAccountsPage"); });
                if (p) {
                    p.updateAccountInfoSlot(cloudDriveJobJson.type, jsonObj.uid, jsonObj.name, jsonObj.quota_info.shared, jsonObj.quota_info.normal, jsonObj.quota_info.quota);
                }
            } else {
                messageDialog.titleText = "CloudDrive Account Info"
                messageDialog.message = "Error " + err + " " + errMsg + " " + msg;
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

            if (err == 0) {
                // Update ProgressBar on listItem.
                // TODO also show running on its parents.
                fsModel.setProperty(json.local_file_path, FolderSizeItemListModel.IsRunningRole, isRunning);

                // Remove cache on target folders and its parents.
                fsModel.removeCache(json.local_file_path);

                // TODO Does it need to refresh to add gotten file to listview ?
                refreshSlot();
            } else {
                messageDialog.titleText = getCloudName(job.type) + " File Get";
                messageDialog.message = "Error " + err + " " + errMsg + " " + msg;
                messageDialog.open();
            }

            // Show indicator on parent up to root.
            // Skip i=0 as it's notified above already.
            var pathList = fsModel.getPathToRoot(json.local_file_path);
            for(var i=1; i<pathList.length; i++) {
                modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
                if (modelIndex > -1) {
                    fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, isRunning);
                }
            }
        }

        onFilePutReplySignal: {
            console.debug("folderPage cloudDriveModel onFilePutReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jsonText = cloudDriveModel.getJobJson(nonce);
//            console.debug("folderPage cloudDriveModel jsonText " + jsonText);

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            var json = JSON.parse(jsonText);
            var modelIndex = json.model_index;
            var isRunning = json.is_running;

            if (err == 0) {
                // Update ProgressBar on listItem.
                // TODO also show running on its parents.
                fsModel.setProperty(json.local_file_path, FolderSizeItemListModel.IsRunningRole, isRunning);
            } else {
                messageDialog.titleText = getCloudName(json.type) + " File Put";
                messageDialog.message = "Error " + err + " " + errMsg + " " + msg;
                messageDialog.open();
            }

            // Show indicator on parent up to root.
            // Skip i=0 as it's notified above already.
            var pathList = fsModel.getPathToRoot(json.local_file_path);
            for(var i=1; i<pathList.length; i++) {
                modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
                if (modelIndex > -1) {
                    fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, isRunning);
                }
            }
        }

        onMetadataReplySignal: {
            console.debug("folderPage cloudDriveModel onMetadataReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            // Get job json.
            var jsonText = cloudDriveModel.getJobJson(nonce);
            var json = JSON.parse(jsonText);
            var uid = json.uid;
            var type = json.type;
            var localPath = json.local_file_path;
            var remotePath = json.remote_file_path;
            var modelIndex = json.model_index;
            var isRunning = json.is_running;

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                // Found metadata.
                var jsonObj = Utility.createJsonObj(msg);
                var localPathHash = cloudDriveModel.getItemHash(localPath, type, uid);

                // TODO why don't it use json.remote_file_path ?
                // Get remotePath if localPath is already connected if localPathHash exists. Otherwise getDefaultRemoteFilePath().
                if (localPathHash == "") {
                    remotePath = cloudDriveModel.getDefaultRemoteFilePath(localPath);
                } else {
                    remotePath = cloudDriveModel.getItemRemotePath(localPath, type, uid);
                }
//                console.debug("cloudDriveModel onMetadataReplySignal folder localPathHash " + localPathHash + " localPath " + localPath + " remotePath " + remotePath);

                // If remotePath was deleted, remove link from localPath.
                if (jsonObj.is_deleted) {
                    // TODO if dir, should remove all sub items.
                    cloudDriveModel.removeItem(type, uid, localPath);
                    // Update ProgressBar on listItem.
                    fsModel.setProperty(json.local_file_path, FolderSizeItemListModel.IsRunningRole, isRunning);

                    // Notify removed link.

                    messageDialog.titleText = getCloudName(type) + " Metadata";
                    messageDialog.message = "File " + localPath + " was removed remotely.\nLink will be removed.";
                    messageDialog.autoClose = true;
                    messageDialog.open();
                    return;
                }

                // Sync starts from itself.
                if (jsonObj.is_dir) { // Sync folder.

                    // Sync based on remote contents.
//                    console.debug("cloudDriveModel onMetadataReplySignal folder jsonObj.rev " + jsonObj.rev + " jsonObj.hash " + jsonObj.hash + " localPathHash " + localPathHash);
                    if (jsonObj.hash != localPathHash) { // Sync all json(remote)'s contents.
                        for(var i=0; i<jsonObj.contents.length; i++) {
                            var item = jsonObj.contents[i];
                            var itemLocalPath = cloudDriveModel.getDefaultLocalFilePath(item.path);
                            var itemLocalHash = cloudDriveModel.getItemHash(itemLocalPath, type, uid);
                            if (item.is_dir) {
                                // This flow will trigger recursive metadata calling.
//                                console.debug("dir item.path = " + item.path + " itemLocalHash " + itemLocalHash + " item.rev " + item.rev);
                                cloudDriveModel.metadata(type, uid, itemLocalPath, item.path, -1);
                            } else {
//                                console.debug("file item.path = " + item.path + " itemLocalHash " + itemLocalHash + " item.rev " + item.rev);
                                if (itemLocalHash != item.rev) {
                                    cloudDriveModel.metadata(type, uid, itemLocalPath, item.path, -1);
                                }
                            }
                        }

                        // Add cloudDriveItem for currentDir.
                        cloudDriveModel.addItem(type, uid, localPath, remotePath, jsonObj.hash);
                    }

                    // Sync based on local contents.
                    cloudDriveModel.syncFromLocal(type, uid, localPath, remotePath, modelIndex);
                } else { // Sync file.
//                    console.debug("cloudDriveModel onMetadataReplySignal file jsonObj.rev " + jsonObj.rev + " localPathHash " + localPathHash);
                    if (jsonObj.rev > localPathHash || !fsModel.isFile(localPath)) {
                        // TODO it should be specified with localPath instead of always use defaultLocalPath.
                        cloudDriveModel.fileGet(type, uid, remotePath, localPath, modelIndex);
                    } else if (jsonObj.rev < localPathHash) {
                        cloudDriveModel.filePut(type, uid, localPath, remotePath, modelIndex);
                    }
                }
            } else if (err == 203) { // If metadata is not found, put it to cloud right away recursively.
                if (fsModel.isDir(localPath)) {
                    // Remote folder will be created in syncFromLocal if it's required.
                    cloudDriveModel.syncFromLocal(type, uid, localPath, remotePath, modelIndex);

                    // Request metadata for current dir.
                    // Once it got reply, it should get hash already.
                    // Because its sub files/dirs are in prior queue.
//                    cloudDriveModel.metadata(type, uid, localPath, remotePath, modelIndex);
                } else {
                    cloudDriveModel.filePut(type, uid, localPath, remotePath, modelIndex);
                }
            } else if (err == 202) { // Nonce already in used.
                console.debug("MetadataReply Error " + err + " " + errMsg + " " + msg + ". I will retry again.");

                // Retry request metadata as
                metadata(type, uid, localPath, remotePath, modelIndex);
            } else {
                messageDialog.titleText = getCloudName(type) + " Metadata";
                messageDialog.message = "Error " + err + " " + errMsg + " " + msg;
                messageDialog.open();
            }

            // Update ProgressBar on listItem.
            fsModel.setProperty(localPath, FolderSizeItemListModel.IsRunningRole, isRunning);
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
                messageDialog.titleText = getCloudName(json.type) + " Create Folder";
                messageDialog.message = "Error " + err + " " + errMsg + " " + msg;
                messageDialog.autoClose = true;
                messageDialog.open();
            }

            // Update ProgressBar on listItem.
            // TODO also show running on its parents.
            fsModel.setProperty(json.local_file_path, FolderSizeItemListModel.IsRunningRole, isRunning);
        }

        onDownloadProgress: {
//            console.debug("folderPage cloudDriveModel onDownloadProgress " + nonce + " " + bytesReceived + " / " + bytesTotal);

            var jsonText = cloudDriveModel.getJobJson(nonce);
//            console.debug("folderPage cloudDriveModel jsonText " + jsonText);

            var json = JSON.parse(jsonText);
            var isRunning = json.is_running

            // Update ProgressBar on listItem.
            var modelIndex = fsModel.getIndexOnCurrentDir(json.local_file_path);
            if (modelIndex > -1) {
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
                if (modelIndex > -1) {
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
            if (modelIndex > -1) {
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
                if (modelIndex > -1) {
                    fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, isRunning);
                    fsModel.setProperty(modelIndex, FolderSizeItemListModel.RunningOperationRole, FolderSizeItemListModel.SyncOperation);
                }
            }
        }

        onLocalChangedSignal: {
            // Reset CloudDriveItem hash upto root.
            var paths = fsModel.getPathToRoot(localPath);
            for (var i=0; i<paths.length; i++) {
                console.debug("folderPage cloudDriveModel onLocalChangedSignal updateItems paths[" + i + "] " + paths[i]);
                cloudDriveModel.updateItems(CloudDriveModel.Dropbox, paths[i], cloudDriveModel.dirtyHash);
            }
        }

        onJobQueueStatusSignal: {
            // Send info to cloudDriveAccountsPage.
            var p = pageStack.find(function (page) { return (page.name == "settingPage"); });
            if (p) {
                p.updateJobQueueCount(runningJobCount, jobQueueCount);
            }
        }
    }

    CloudDriveUsersDialog {
        id: uidDialog

        function proceedPendingOperation() {
            // TODO
        }

        function proceedOperation(type, uid, localPath, remotePath, modelIndex) {
            switch (operation) {
            case CloudDriveModel.Metadata:
                cloudDriveModel.metadata(type, uid, localPath, remotePath, modelIndex);
                break;
            case CloudDriveModel.FilePut:
                if (fsModel.isDir(localPath)) {
                    cloudDriveModel.syncFromLocal(type, uid, localPath, remotePath, modelIndex, true);
                    // Get metadata for newly sync'd folder.
//                    cloudDriveModel.metadata(type, uid, localPath, remotePath, modelIndex);
                } else {
                    cloudDriveModel.filePut(type, uid, localPath, remotePath, modelIndex);
                }
                break;
            case CloudDriveModel.FileGet:
                cloudDriveModel.fileGet(type, uid, localPath, remotePath, modelIndex);
                break;
            }
        }

        onOpened: {
            uidDialog.model = cloudDriveModel.getUidListModel(localPath);
        }

        onAccepted: {
            // TODO Proceed for GoogleDrive

            // Proceed for Dropbox
            if (uidDialog.localPath != "") {
                // sync localPath.
                var remoteFilePath = cloudDriveModel.getDefaultRemoteFilePath(uidDialog.localPath);
                console.debug("uidDialog.selectedIndex " + uidDialog.selectedIndex + " type " + uidDialog.selectedCloudType + " uid " + uidDialog.selectedUid + " localPath " + uidDialog.localPath + " remoteFilePath " + remoteFilePath);
                proceedOperation(uidDialog.selectedCloudType, uidDialog.selectedUid, uidDialog.localPath, remoteFilePath, selectedModelIndex);
            } else if (clipboard.count > 0){
                // Sync from clipboard.
                for (var i=0; i<clipboard.count; i++) {
                    if (clipboard.get(i).action == "sync") {
                        console.debug("uidDialog onAccepted clipboard item i " + i + " " + clipboard.get(i).action + " " + clipboard.get(i).sourcePath);

                        var sourcePath = clipboard.get(i).sourcePath;
                        var remotePath = cloudDriveModel.getItemRemotePath(sourcePath, uidDialog.selectedCloudType, uidDialog.selectedUid);
                        if (remotePath == "") {
                            remotePath = cloudDriveModel.getDefaultRemoteFilePath(sourcePath);
                        }
                        proceedOperation(uidDialog.selectedCloudType, uidDialog.selectedUid, sourcePath, remotePath, -1);
                    }
                }

                // Clear clipboard.
                clipboard.clear();
            }
        }
    }
}
