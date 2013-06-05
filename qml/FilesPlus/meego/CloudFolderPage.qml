import QtQuick 1.1
import com.nokia.meego 1.0
import CloudDriveModel 1.0
import "Utility.js" as Utility

Page {
    id: cloudFolderPage

    property string name: "cloudFolderPage"
    property bool inverted: !theme.inverted

    property string caller: name
    property int operation
    property string localPath
    property string remotePath // Specified remote path
    property string originalRemotePath
    property int selectedCloudType
    property string selectedUid
    property int selectedModelIndex
    property string selectedRemotePath
    property string selectedRemotePathName
    property string remoteParentPath // Current browse remote path
    property string remoteParentPathName // Shows on titlebar
    property string remoteParentParentPath // For change up
    property int selectedIndex: -1
    property bool selectedIsDir
    property bool selectedIsValid
    property bool isBusy

    state: "list"
    states: [
        State {
            name: "list"
        },
        State {
            name: "grid"
        }
    ]

    function goUpSlot() {
        console.debug("cloudFolderPage goUpSlot selectedCloudType " + selectedCloudType + " selectedUid " + selectedUid + " cloudDriveModel.remoteParentPath " + cloudDriveModel.remoteParentPath + " cloudDriveModel.remoteParentParentPath " + cloudDriveModel.remoteParentParentPath);
        if (cloudDriveModel.isRemoteRoot(selectedCloudType, selectedUid, cloudDriveModel.remoteParentPath) || cloudDriveModel.remoteParentParentPath == "") {
            pageStack.pop(cloudFolderPage);
        } else {
            changeRemotePath(cloudDriveModel.remoteParentParentPath);
        }
    }

    function postBrowseReplySlot() {
        console.debug("cloudFolderPage postBrowseReplySlot selectedCloudType " + selectedCloudType + " selectedUid " + selectedUid + " cloudDriveModel.remoteParentPath " + cloudDriveModel.remoteParentPath + " cloudDriveModel.remoteParentParentPath " + cloudDriveModel.remoteParentParentPath);
        remoteParentPath = cloudDriveModel.remoteParentPath;
        remoteParentPathName = cloudDriveModel.remoteParentPathName;
        remoteParentParentPath = cloudDriveModel.remoteParentParentPath;

        originalRemotePath = (originalRemotePath == "") ? remotePath : originalRemotePath;

        selectedIsValid = true;
        selectedIndex = cloudDriveModel.findIndexByRemotePath(originalRemotePath);
        cloudFolderView.currentIndex = selectedIndex;
        cloudFolderGridView.currentIndex = selectedIndex;

        // Reset busy.
        isBusy = false;
        if (selectedIndex > -1) {
            cloudFolderView.positionViewAtIndex(selectedIndex, ListView.Contain); // Issue: ListView.Center will cause list item displayed out of list view in Meego only.
            cloudFolderGridView.positionViewAtIndex(selectedIndex, ListView.Contain); // Issue: ListView.Center will cause list item displayed out of list view in Meego only.
        }
    }

    function changeRemotePath(remoteParentPath) {
        console.debug("cloudFolderPage changeRemotePath " + remoteParentPath);
        quickScrollPanel.visible = false;
        selectedIndex = -1;
        isBusy = true;

        // Browse remote parent path.
        var actualRemoteParentPath = (remoteParentPath == "") ? cloudDriveModel.getParentRemotePath(selectedCloudType, remotePath) : remoteParentPath;
        cloudDriveModel.browse(selectedCloudType, selectedUid, actualRemoteParentPath);
    }

    function createRemoteFolder(newRemoteFolderName) {
        isBusy = true;
        cloudDriveModel.createFolder(selectedCloudType, selectedUid, "", remoteParentPath, newRemoteFolderName);
    }

    function renameRemotePath(remotePath, newName) {
        isBusy = true;
        cloudDriveModel.moveFile(selectedCloudType, selectedUid, "", remotePath, "", "", newName);
    }

    function deleteRemotePath(remotePath) {
        // Delete remote file/folder.
        cloudDriveModel.deleteFile(selectedCloudType, selectedUid, "", remotePath);
    }

    function refreshSlot(caller) {
        console.debug("cloudFolderPage refreshSlot caller " + caller + " " + selectedCloudType + " " + remotePath + " " + remoteParentPath);
        quickScrollPanel.visible = false;
        selectedIndex = -1;
        isBusy = true;

        // Browse remote parent path.
        var actualRemoteParentPath = (remoteParentPath == "") ? cloudDriveModel.getParentRemotePath(selectedCloudType, remotePath) : remoteParentPath;
        cloudDriveModel.browse(selectedCloudType, selectedUid, actualRemoteParentPath);
    }

    function mailFileSlot(remotePath, remotePathName) {
        console.debug("cloudFolderPage mailFileSlot remotePath=" + remotePath);
        recipientSelectionDialog.refresh();
        cloudDriveModel.shareFileCaller = "mailFileSlot";
        cloudDriveModel.shareFile(selectedCloudType, selectedUid, remotePathName, remotePath);
    }

    function smsFileSlot(remotePath, remotePathName) {
        console.debug("cloudFolderPage smsFileSlot remotePath=" + remotePath);
        recipientSelectionDialog.refresh();
        cloudDriveModel.shareFileCaller = "smsFileSlot";
        cloudDriveModel.shareFile(selectedCloudType, selectedUid, remotePathName, remotePath);
    }

    function resetBusySlot(caller) {
        isBusy = false;
    }

    function syncCurrentFolderSlot() {
        var isConnected = cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, remoteParentPath);
        console.debug("cloudFolderPage syncCurrentFolder remoteParentPath " + remoteParentPath + " isConnected " + isConnected);
        if (isConnected) {
            cloudDriveModel.syncItemByRemotePath(selectedCloudType, selectedUid, remoteParentPath);
        }
    }

    function syncConnectedItemsSlot() {
        // Suspend for queuing.
        cloudDriveModel.suspendNextJob();

        for (var i=0; i<cloudDriveModel.count; i++) {
            var remotePath = cloudDriveModel.get(i).absolutePath;
            var isConnected = cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, remotePath);
            console.debug("cloudFolderPage synconnectedItemsSlot remotePath " + remotePath + " isConnected " + isConnected);
            if (isConnected) {
                cloudDriveModel.syncItemByRemotePath(selectedCloudType, selectedUid, remotePath);
            }
        }

        // Resume proceed queued jobs.
        cloudDriveModel.resumeNextJob();
    }

    function syncClipboardItems() {
        // Suspend for queuing.
        cloudDriveModel.suspendNextJob();

        for (var i=0; i<clipboard.count; i++) {
            if (clipboard.get(i).action == "sync") {
                console.debug("cloudFolderPage syncClipboardItems clipboard item i " + i + " " + clipboard.get(i).action + " type " + clipboard.get(i).type + " sourcePath " + clipboard.get(i).sourcePath);
                var res = cloudDriveModel.syncItemByRemotePath(cloudDriveModel.getClientType(clipboard.get(i).type), clipboard.get(i).uid, clipboard.get(i).sourcePath);
                if (!res) {
                    console.debug("cloudFolderPage syncClipboardItems skipped clipboard item i " + i + " " + clipboard.get(i).action + " type " + clipboard.get(i).type + " sourcePath " + clipboard.get(i).sourcePath);
                }
            }
        }

        // Clear clipboard.
        clipboard.clear();

        // Resume proceed queued jobs.
        cloudDriveModel.resumeNextJob();
    }

    function updateJobQueueCount(runningJobCount, jobQueueCount) {
        // Update (runningJobCount + jobQueueCount) on cloudButton.
        cloudButtonIndicator.text = ((runningJobCount + jobQueueCount) > 0) ? (runningJobCount + jobQueueCount) : "";
    }

    function updateItemSlot(jobJson, caller) {
        if (!jobJson) return;
//        console.debug("cloudFolderPage updateItemSlot caller " + caller + " jobJson " + JSON.stringify(jobJson));

        if (jobJson.remote_file_path == "") return;

        if (jobJson.type == selectedCloudType && jobJson.uid == selectedUid) {
            var i = cloudDriveModel.findIndexByRemotePath(jobJson.remote_file_path);
            console.debug("cloudFolderPage updateItemSlot caller " + caller + " jobJson " + JSON.stringify(jobJson) + " model index " + i);
            if (i >= 0) {
                cloudDriveModel.setProperty(i, "isRunning", jobJson.is_running);
                cloudDriveModel.setProperty(i, "isConnected", cloudDriveModel.isRemotePathConnected(jobJson.type, jobJson.uid, jobJson.remote_file_path));
            }
        }
    }

    function refreshItemAfterFilePutSlot(jobJson) {
        if (!jobJson) return;
        console.debug("cloudFolderPage refreshItemAfterFilePutSlot jobJson " + JSON.stringify(jobJson));

        if (jobJson.type == selectedCloudType && jobJson.uid == selectedUid) {
            if (jobJson.remote_file_path == remoteParentPath) {
                refreshSlot("cloudFolderPage refreshItemAfterFilePutSlot");
            } else if (jobJson.remote_file_path == "/" && remoteParentPath == "") {
                refreshSlot("cloudFolderPage refreshItemAfterFilePutSlot Dropbox");
            }
        } else if (jobJson.target_type == selectedCloudType && jobJson.target_uid == selectedUid) {
            if (jobJson.new_remote_file_path == remoteParentPath) {
                refreshSlot("cloudFolderPage refreshItemAfterFilePutSlot");
            } else if (jobJson.new_remote_file_path == "/" && remoteParentPath == "") {
                refreshSlot("cloudFolderPage refreshItemAfterFilePutSlot Dropbox");
            }
        }
    }

    function refreshItem(remotePath) {
        var timestamp = (new Date()).getTime();
        var i = cloudDriveModel.findIndexByRemotePath(remotePath);
        console.debug("cloudFolderPage refreshItem remotePath " + remotePath + " i " + i);
        if (i >= 0) {
            cloudDriveModel.setProperty(i, "timestamp", timestamp);
        }
    }

    // Selection functions.

    function cutMarkedItems() {
        for (var i=0; i<cloudDriveModel.count; i++) {
            if (cloudDriveModel.get(i).isChecked) {
                console.debug(Utility.nowText() + "cloudFolderPage cutMarkedItems item"
                              + " absolutePath " + cloudDriveModel.get(i).absolutePath
                              + " isChecked " + cloudDriveModel.get(i).isChecked);

                clipboard.addItemWithSuppressCountChanged({ "action": "cut",
                                                              "type": cloudDriveModel.getCloudName(selectedCloudType),
                                                              "uid": selectedUid,
                                                              "sourcePath": cloudDriveModel.get(i).absolutePath,
                                                              "sourcePathName": cloudDriveModel.get(i).name });
            }

            // Reset isChecked.
            cloudDriveModel.setProperty(i, "isChecked", false);
        }

        // Emit suppressed signal.
        clipboard.emitCountChanged();
    }

    function copyMarkedItems() {
        for (var i=0; i<cloudDriveModel.count; i++) {
            if (cloudDriveModel.get(i).isChecked) {
                console.debug(Utility.nowText() + "cloudFolderPage copyMarkedItems item"
                              + " absolutePath " + cloudDriveModel.get(i).absolutePath
                              + " isChecked " + cloudDriveModel.get(i).isChecked);

                clipboard.addItemWithSuppressCountChanged({ "action": "copy",
                                                              "type": cloudDriveModel.getCloudName(selectedCloudType),
                                                              "uid": selectedUid,
                                                              "sourcePath": cloudDriveModel.get(i).absolutePath,
                                                              "sourcePathName": cloudDriveModel.get(i).name });
            }

            // Reset isChecked.
            cloudDriveModel.setProperty(i, "isChecked", false);
        }

        // Emit suppressed signal.
        clipboard.emitCountChanged();
    }

    function deleteMarkedItems() {
        for (var i=0; i<cloudDriveModel.count; i++) {
            if (cloudDriveModel.get(i).isChecked) {
                console.debug(Utility.nowText() + "cloudFolderPage deleteMarkedItems item"
                              + " absolutePath " + cloudDriveModel.get(i).absolutePath
                              + " isChecked " + cloudDriveModel.get(i).isChecked);

                clipboard.addItem({ "action": "delete",
                                      "type": cloudDriveModel.getCloudName(selectedCloudType),
                                      "uid": selectedUid,
                                      "sourcePath": cloudDriveModel.get(i).absolutePath,
                                      "sourcePathName": cloudDriveModel.get(i).name });
            }

            // Reset isChecked.
            cloudDriveModel.setProperty(i, "isChecked", false);
        }

        // Open confirmation dialog.
        fileActionDialog.open();
    }

    function syncMarkedItems() {
        for (var i=0; i<cloudDriveModel.count; i++) {
            if (cloudDriveModel.get(i).isChecked) {
                console.debug(Utility.nowText() + " cloudFolderPage syncMarkedItems item"
                              + " absolutePath " + cloudDriveModel.get(i).absolutePath
                              + " isChecked " + cloudDriveModel.get(i).isChecked);

                clipboard.addItem({ "action": "sync",
                                      "type": cloudDriveModel.getCloudName(selectedCloudType),
                                      "uid": selectedUid,
                                      "sourcePath": cloudDriveModel.get(i).absolutePath,
                                      "sourcePathName": cloudDriveModel.get(i).name });
            }

            // Reset isChecked.
            cloudDriveModel.setProperty(i, "isChecked", false);
        }

        // Invoke syncClipboard.
        syncClipboardItems();
    }

    tools: ToolBarLayout {
        id: toolBarLayout

        ToolBarButton {
            id: backButton
            buttonIconSource: "toolbar-back"
            onClicked: {
                if (cloudDriveModel.state == "mark") {
                    cloudDriveModel.state = "";
                } else {
                    // TODO Specify local path to focus after cd to parent directory..
//                    cloudFolderView.focusLocalPath = cloudDriveModel.currentDir;
                    goUpSlot();
                }
            }
        }

        ToolBarButton {
            id: refreshButton
            buttonIconSource: "toolbar-refresh"
            visible: (cloudDriveModel.state != "mark")
            onClicked: {
                // Force resume.
                cloudDriveModel.resumeNextJob();

                refreshSlot("refreshButton onClicked");
            }
            onPressAndHold: {
                if (cloudFolderPage.state == "grid" && cloudDriveModel.isImageUrlCachable(selectedCloudType)) {
                    clearCachedImagesConfirmation.open();
                }
            }
        }

        ToolBarButton {
            id: actionButton

            buttonIconSource: {
                if (cloudDriveModel.state == "mark") {
                    return (!inverted ? "ok.svg" : "ok_inverted.svg");
                } else {
                    if (cloudFolderPage.state == "list") {
                        return (!inverted ? "grid.svg" : "grid_inverted.svg");
                    } else {
                        return (!inverted ? "list.svg" : "list_inverted.svg");
                    }
                }
            }

            onClicked: {
                if (cloudDriveModel.state == "mark") {
                    if (markMenu.isMarkAll) {
                        cloudDriveModel.unmarkAll();
                    } else {
                        cloudDriveModel.markAll();
                    }
                    markMenu.isMarkAll = !markMenu.isMarkAll;
                } else {
                    if (cloudFolderPage.state == "list") {
                        cloudFolderPage.state = "grid";
                    } else {
                        cloudFolderPage.state = "list";
                    }
                }
            }
        }

        ToolBarButton {
            id: cloudButton
            buttonIconSource: (!inverted ? "cloud.svg" : "cloud_inverted.svg")
            visible: (cloudDriveModel.state != "mark")

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

                if (cloudDriveModel.state == "mark") {
                    if (!cloudDriveModel.isAnyItemChecked()) {
                        markAllMenu.open();
                    } else {
                        markMenu.open();
                    }
                } else {
                    mainMenu.open();
                }
            }
        }
    }

    TitlePanel {
        id: currentPath
        text: remoteParentPathName
        textLeftMargin: height + 5

        Image {
            id: cloudIcon
            source: cloudDriveModel.getCloudIcon(selectedCloudType)
            width: parent.height
            height: parent.height
            fillMode: Image.PreserveAspectFit
            anchors.left: parent.left
        }
    }

    NameFilterPanel {
        id: nameFilterPanel
        requestAsType: true
        anchors.bottom: parent.bottom

        property variant view: (cloudFolderPage.state == "list") ? cloudFolderView : cloudFolderGridView

        onPrevious: {
            lastFindIndex = cloudDriveModel.findIndexByNameFilter(nameFilter, --lastFindIndex, true);
            console.debug("cloudFolderPage nameFilterPanel onPrevious nameFilter " + nameFilter + " lastFindIndex " + lastFindIndex);
            if (lastFindIndex > -1) {
                view.positionViewAtIndex(lastFindIndex, ListView.Beginning);
                view.currentIndex = lastFindIndex;
            }
        }

        onNext: {
            lastFindIndex = cloudDriveModel.findIndexByNameFilter(nameFilter, ++lastFindIndex, false);
            console.debug("cloudFolderPage nameFilterPanel onNext nameFilter " + nameFilter + " lastFindIndex " + lastFindIndex);
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

    CloudFolderMenu {
        id: mainMenu
        disabledMenus: ["syncConnectedItems","syncCurrentFolder"]

        onPaste: {
            console.debug("cloudFolderPage mainMenu onPaste " + cloudDriveModel.remoteParentPath + " " + cloudDriveModel.remoteParentPathName);
            fileActionDialog.targetPath = cloudDriveModel.remoteParentPath;
            fileActionDialog.targetPathName = cloudDriveModel.remoteParentPathName;
            fileActionDialog.open();
        }
        onOpenMarkMenu: {
            cloudDriveModel.state = "mark";
        }
        onClearClipboard: {
            clipboard.clear();
        }
        onNewFolder: {
            newFolderDialog.open();
        }
        onSetNameFilter: {
            nameFilterPanel.open();
        }
        onDrives: {
            pageStack.pop(cloudFolderPage);
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
                return (clipboard.count > 0);
            } else if (menuItem.name == "clearClipboard") {
                return clipboard.count > 0;
            } else if (menuItem.name == "markMenu") {
                return cloudDriveModel.state != "mark";
            }

            return true;
        }
    }

    BookmarksMenu {
        id: bookmarksMenu

        onAddBookmark: {
            bookmarksModel.addBookmark(selectedCloudType, selectedUid, remoteParentPath, remoteParentPathName);
        }
        onOpenBookmarks: {
            pageStack.push(Qt.resolvedUrl("BookmarksPage.qml"));
        }
    }

    SortByMenu {
        id: sortByMenu

        onSelectSort: {
            console.debug("sortByMenu setSortFlag flag=" + flag);
            cloudFolderView.positionViewAtBeginning();
            cloudFolderGridView.positionViewAtBeginning();
            cloudDriveModel.setSortFlag(selectedCloudType, selectedUid, selectedRemotePath, flag);
        }

        onStatusChanged: {
            if (status == DialogStatus.Open) {
                // Set sortFlag before status=Open
                sortFlag = cloudDriveModel.getSortFlag(selectedCloudType, selectedUid, selectedRemotePath);
            }
        }
    }

    MarkMenu {
        id: markMenu

        onMarkAll: {
            if (isMarkAll) {
                cloudDriveModel.markAll();
            } else {
                cloudDriveModel.unmarkAll();
            }
            isMarkAll = !isMarkAll;
        }
        onCutMarkedItems: {
            cloudFolderPage.cutMarkedItems();
            cloudDriveModel.state = "";
            cloudFolderView.currentIndex = -1;
            cloudFolderGridView.currentIndex = -1;
        }
        onCopyMarkedItems: {
            cloudFolderPage.copyMarkedItems();
            cloudDriveModel.state = "";
            cloudFolderView.currentIndex = -1;
            cloudFolderGridView.currentIndex = -1;
        }
        onDeleteMarkedItems: {
            cloudFolderPage.deleteMarkedItems();
            cloudDriveModel.state = "";
            cloudFolderView.currentIndex = -1;
            cloudFolderGridView.currentIndex = -1;
        }
        onSyncMarkedItems: {
            cloudFolderPage.syncMarkedItems();
            cloudDriveModel.state = "";
            cloudFolderView.currentIndex = -1;
            cloudFolderGridView.currentIndex = -1;
        }
        onStatusChanged: {
            if (status == DialogStatus.Opening) {
                isMarkAll = !cloudDriveModel.areAllItemChecked();
            }
        }

        function isMenuItemVisible(menuItem) {
            // Validate each menu logic if it's specified, otherwise it's visible.
            return true;
        }
    }

    MarkAllMenu {
        id: markAllMenu

        onMarkAll: cloudDriveModel.markAll()
        onMarkAllFiles: cloudDriveModel.markAllFiles()
        onMarkAllFolders: cloudDriveModel.markAllFolders();
    }

    CloudMenu {
        id: cloudMenu

        onSyncCurrentFolder: {
            syncCurrentFolderSlot();
        }
        onSyncConnectedItems: {
            syncConnectedItemsSlot();
        }
        onShowCloudDriveJobs: {
            pageStack.push(Qt.resolvedUrl("CloudDriveJobsPage.qml"));
        }

        function isMenuItemVisible(menuItem) {
            // Validate each menu logic if it's specified, otherwise it's visible.
            if (menuItem.name == "syncCurrentFolder") {
                return !cloudDriveModel.isRemoteRoot(selectedCloudType, selectedUid, remoteParentPath) && cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, remoteParentPath);
            } else {
                return true;
            }
        }
    }

    OpenMenu {
        id: openMenu

        property int selectedIndex: (cloudFolderPage.state == "list") ? cloudFolderView.currentIndex : cloudFolderGridView.currentIndex

        function show() {
            var absolutePath = cloudDriveModel.get(selectedIndex).absolutePath;
            var source = cloudDriveModel.get(selectedIndex).source;
            var alternative = cloudDriveModel.get(selectedIndex).alternative;

            source = (source) ? source : cloudDriveModel.media(selectedCloudType, selectedUid, absolutePath);
            cloudDriveModel.setProperty(selectedIndex, "source", source);
            console.debug("cloudFolderPage openMenu show source " + cloudDriveModel.get(selectedIndex).source + " alternative " + cloudDriveModel.get(selectedIndex).alternative);

            if (source != "" && (!alternative || alternative == "")) {
                openMedia();
            } else if (source == "" && alternative != "") {
                openWeb();
            } else {
                open();
            }
        }

        onOpenMedia: {
            Qt.openUrlExternally(cloudDriveModel.get(selectedIndex).source);
        }
        onOpenWeb: {
            Qt.openUrlExternally(cloudDriveModel.get(selectedIndex).alternative);
        }
    }

    ListView {
        id: cloudFolderView
        visible: (cloudFolderPage.state == "list")
        width: parent.width
        height: parent.height - currentPath.height - (nameFilterPanel.visible ? nameFilterPanel.height : 0)
        anchors.top: currentPath.bottom
        highlightRangeMode: ListView.NoHighlightRange
        highlightFollowsCurrentItem: true
        highlightMoveDuration: 1
        highlightMoveSpeed: 4000
        highlight: Rectangle {
            gradient: highlightGradient
        }
        clip: true
        focus: true
        cacheBuffer: height * 2
        pressDelay: 200
        model: cloudDriveModel
        delegate: cloudItemDelegate

        Rectangle {
            id: busyPanel
            color: "black"
            opacity: 0.7
            visible: isBusy
            anchors.fill: parent

            BusyIndicator {
                id: busyIcon
                visible: isBusy
                running: isBusy
                platformStyle: BusyIndicatorStyle { size: "large" }
                anchors.centerIn: parent
            }

            MouseArea {
                enabled: isBusy
                anchors.fill: parent
                onPressed: {
                    // do nothing.
                }
            }
        }

        onMovementStarted: {
            if (currentItem) {
                // Hide highlight and popupTool.
                currentItem.pressed = false;
                currentIndex = -1;
                popupToolPanel.visible = false;
            }
        }

        QuickScrollPanel {
            id: quickScrollPanel
            listView: parent
            indicatorBarTitle: (modelIndex < 0) ? ""
                               : ( sortByMenu.sortFlag == CloudDriveModel.SortByTime
                                  ? Qt.formatDateTime(cloudDriveModel.get(modelIndex).lastModified, "d MMM yyyy")
                                  : cloudDriveModel.get(modelIndex).name )
            scrollBarWidth: 80
            indicatorBarHeight: 80
            scrollBarColor: "grey"
            indicatorWidth: 5
            autoHideInterval: appInfo.emptySetting+appInfo.getSettingValue("QuickScrollPanel.timer.interval", 2) * 1000
        }
    }

    GridView {
        id: cloudFolderGridView
        visible: (cloudFolderPage.state == "grid")
        width: parent.width
        height: parent.height - currentPath.height - (nameFilterPanel.visible ? nameFilterPanel.height : 0)
        anchors.top: currentPath.bottom
        cellWidth: appInfo.emptySetting + (appInfo.getSettingValue("GridView.compact.enabled", false) ? 120 : 160)
        cellHeight: appInfo.emptySetting + (appInfo.getSettingValue("GridView.compact.enabled", false) ? 120 : 160)
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
        model: cloudDriveModel
        delegate: gridItemDelegate

        Rectangle {
            id: gridBusyPanel
            color: "black"
            opacity: 0.7
            visible: isBusy
            anchors.fill: parent

            BusyIndicator {
                id: gridBusyIcon
                visible: isBusy
                running: isBusy
                platformStyle: BusyIndicatorStyle { size: "large" }
                anchors.centerIn: parent
            }

            MouseArea {
                enabled: isBusy
                anchors.fill: parent
                onPressed: {
                    // do nothing.
                }
            }
        }

        property int lastContentY

        onMovementStarted: {
            if (currentItem) {
                // Hide highlight and popupTool.
                currentIndex = -1;
                popupToolPanel.visible = false;
            }
        }

        QuickScrollPanel {
            id: gridQuickScrollPanel
            listView: parent
            indicatorBarTitle: (modelIndex < 0) ? ""
                               : ( sortByMenu.sortFlag == CloudDriveModel.SortByTime
                                  ? Qt.formatDateTime(cloudDriveModel.get(modelIndex).lastModified, "d MMM yyyy")
                                  : cloudDriveModel.get(modelIndex).name )
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
        id: cloudItemDelegate

        CloudListItem {
            id: listItem
            listViewState: (cloudDriveModel.state ? cloudDriveModel.state : "")
            syncIconVisible: isConnected
            syncIconSource: (isRunning) ? "cloud_wait.svg" : "cloud.svg"
            actionIconSource: (clipboard.count > 0) ? appInfo.emptySetting+clipboard.getActionIcon(absolutePath, cloudDriveModel.getCloudName(selectedCloudType), selectedUid) : ""
            listItemIconSource: appInfo.emptySetting+listItem.getIconSource(timestamp)
            omitShowingZeroSizeDir: true

            // Override to support cloud items.
            function getIconSource(timestamp) {
                if (isDir) {
                    return "folder_list.svg";
                } else if (viewableImageFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                    var showThumbnail = appInfo.getSettingBoolValue("CloudFolderPage.thumbnail.enabled", false);
                    if (showThumbnail && thumbnail && thumbnail != "") {
                        // Dropbox's thumbnail url can't open with Image directly. Need to invoke cacheImage() or use RemoteImageProvider to download.
                        // Always cache thumbnail by using RemoteImageProvider.
                        if (cloudDriveModel.isImageUrlCachable(selectedCloudType) && fileType.toUpperCase() != "SVG") {
                            return "image://remote/" + thumbnail + "#t=" + timestamp;
                        } else {
                            return thumbnail;
                        }
                    } else {
                        return "photos_list.svg";
                    }
                } else {
                    return "notes_list.svg";
                }
            }

            function getMediaSource(source, remoteFilePath) {
                var url = "";
                if (source) {
                    url = source;
                } else {
                    url = cloudDriveModel.media(selectedCloudType, selectedUid, remoteFilePath);
                }
//                console.debug("cloudFolderPage listItem getMediaSource url " + url);
                return url;
            }

            onPressAndHold: {
                if (cloudDriveModel.state != "mark") {
                    cloudFolderView.currentIndex = index;
                    popupToolPanel.selectedFilePath = absolutePath;
                    popupToolPanel.selectedFileName = name;
                    popupToolPanel.selectedFileIndex = index;
                    popupToolPanel.isDir = isDir;
                    popupToolPanel.pastePath = (isDir) ? absolutePath : remoteParentPath;
                    var panelX = x + mouseX - cloudFolderView.contentX + cloudFolderView.x;
                    var panelY = y + mouseY - cloudFolderView.contentY + cloudFolderView.y;
                    popupToolPanel.open(panelX, panelY);
                }
            }

            onClicked: {
                if (cloudDriveModel.state == "mark") {
                    cloudDriveModel.setProperty(index, "isChecked", !isChecked);
                } else {
                    if (isDir) {
                        changeRemotePath(absolutePath);
                    } else {
                        // If file is running, disable preview.
                        if (isRunning) return;

                        isBusy = true;

                        if (source) console.debug("cloudFolderPage listItem onClicked source " + source);
                        if (alternative) console.debug("cloudFolderPage listItem onClicked alternative " + alternative);
                        if (thumbnail) console.debug("cloudFolderPage listItem onClicked thumbnail " + thumbnail);
                        if (preview) console.debug("cloudFolderPage listItem onClicked preview " + preview);

                        // Check if it's viewable.
                        var url;
                        if (cloudDriveModel.isViewable(selectedCloudType)) {
                            if (viewableImageFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                                // NOTE ImageViewPage will populate ImageViewModel with mediaUrl.
                                pageStack.push(Qt.resolvedUrl("ImageViewPage.qml"),
                                               { model: cloudDriveModel, selectedCloudType: selectedCloudType, selectedUid: selectedUid, selectedFilePath: absolutePath });
                            } else if (viewableTextFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                                // View with internal web viewer.
                                url = getMediaSource(source, absolutePath);
                                if (url != "") {
                                    appInfo.addToClipboard(url);
                                    pageStack.push(Qt.resolvedUrl("WebViewPage.qml"));
                                }
                            } else {
                                // Shows options if both source and alternative are available.
                                openMenu.show();
                            }
                        } else {
                            // File is not viewable but may be browsable.
                            // Shows options if both source and alternative are available.
                            openMenu.show();
                        }

                        isBusy = false;
                    }
                }
            }

            onListItemIconError: {
                if (cloudDriveModel.isImageUrlCachable(selectedCloudType)) {
                    cloudDriveModel.cacheImage(absolutePath, thumbnail, 48, 48, cloudFolderPage.name); // Use default icon size.
                }
            }
        }
    }

    Component {
        id: gridItemDelegate

        CloudGridItem {
            id: gridItem
            gridViewState: (cloudDriveModel.state ? cloudDriveModel.state : "")
            syncIconVisible: isConnected && !isRunning
            syncIconSource: (isRunning) ? "cloud_wait.svg" : "cloud.svg"
            actionIconSource: (clipboard.count > 0) ? appInfo.emptySetting+clipboard.getActionIcon(absolutePath, cloudDriveModel.getCloudName(selectedCloudType), selectedUid) : ""
            gridItemIconSource: appInfo.emptySetting+gridItem.getIconSource(timestamp)
            width: cloudFolderGridView.cellWidth
            height: cloudFolderGridView.cellHeight
            gridItemIconBusyVisible: true
            subIconMargin: appInfo.emptySetting + (appInfo.getSettingValue("GridView.compact.enabled", false) ? 10 : 32) // 32 for 3 columns, 10 for 4 columns

            function getIconSource(timestamp) {
                if (isDir) {
                    return "folder_list.svg";
                } else if (viewableImageFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                    var previewUrl = preview;
                    if (previewUrl == "") {
                        previewUrl = cloudDriveModel.media(selectedCloudType, selectedUid, absolutePath);
                        cloudDriveModel.setProperty(index, "preview", previewUrl);
                    }
                    return getImageSource(previewUrl, timestamp);
                } else {
                    return "notes_list.svg";
                }
            }

            function getImageSource(url, timestamp) {
                if (cloudDriveModel.isImageUrlCachable(selectedCloudType) && (fileType.toUpperCase() != "SVG")) {
                    // Cache only cloud, cachable and not-SVG image.
                    return "image://remote/" + url + "#t=" + timestamp;
                } else {
                    return url + "#t=" + timestamp;
                }
            }

            function getMediaSource(source, remoteFilePath) {
                var url = "";
                if (source) {
                    url = source;
                } else {
                    url = cloudDriveModel.media(selectedCloudType, selectedUid, remoteFilePath);
                }
//                console.debug("cloudFolderPage gridItem getMediaSource url " + url);
                return url;
            }

            onPressAndHold: {
                if (cloudDriveModel.state != "mark") {
                    cloudFolderGridView.currentIndex = index;
                    popupToolPanel.selectedFilePath = absolutePath;
                    popupToolPanel.selectedFileName = name;
                    popupToolPanel.selectedFileIndex = index;
                    popupToolPanel.isDir = isDir;
                    popupToolPanel.pastePath = (isDir) ? absolutePath : remoteParentPath;
                    var panelX = x + mouse.x - cloudFolderGridView.contentX + cloudFolderGridView.x;
                    var panelY = y + mouse.y - cloudFolderGridView.contentY + cloudFolderGridView.y;
                    popupToolPanel.open(panelX, panelY);
                }
            }

            onClicked: {
                if (cloudDriveModel.state == "mark") {
                    cloudDriveModel.setProperty(index, "isChecked", !isChecked);
                } else {
                    if (isDir) {
                        changeRemotePath(absolutePath);
                    } else {
                        // If file is running, disable preview.
                        if (isRunning) return;

                        isBusy = true;

                        if (source) console.debug("cloudFolderPage listItem onClicked source " + source);
                        if (alternative) console.debug("cloudFolderPage listItem onClicked alternative " + alternative);
                        if (thumbnail) console.debug("cloudFolderPage listItem onClicked thumbnail " + thumbnail);
                        if (preview) console.debug("cloudFolderPage listItem onClicked preview " + preview);

                        // Check if it's viewable.
                        var url;
                        if (cloudDriveModel.isViewable(selectedCloudType)) {
                            if (viewableImageFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                                // NOTE ImageViewPage will populate ImageViewModel with mediaUrl.
                                pageStack.push(Qt.resolvedUrl("ImageViewPage.qml"),
                                               { model: cloudDriveModel, selectedCloudType: selectedCloudType, selectedUid: selectedUid, selectedFilePath: absolutePath });
                            } else if (viewableTextFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                                // View with internal web viewer.
                                url = getMediaSource(source, absolutePath);
                                if (url != "") {
                                    appInfo.addToClipboard(url);
                                    pageStack.push(Qt.resolvedUrl("WebViewPage.qml"));
                                }
                            } else {
                                // Shows options if both source and alternative are available.
                                openMenu.show();
                            }
                        } else {
                            // File is not viewable but may be browsable.
                            // Shows options if both source and alternative are available.
                            openMenu.show();
                        }

                        isBusy = false;
                    }
                }
            }

            onListItemIconError: {
                if (cloudDriveModel.isImageUrlCachable(selectedCloudType)) {
                    cloudDriveModel.cacheImage(absolutePath, preview, -1, -1, cloudFolderPage.name); // Use original size because previewUrl is already specified with size.
                }
            }
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
        disabledButtons: ["editFile","cloud","bluetooth"]

        function isButtonVisibleCallback(buttonName) {
            if (buttonName === "sync") {
                return cloudDriveModel.isRemotePathConnected(selectedCloudType, selectedUid, selectedFilePath);
            } else if (buttonName === "paste") {
                return (clipboard.count > 0);
            } else if (buttonName == "print") {
                return !popupToolPanel.isDir && cloudDriveModel.isViewable(selectedCloudType);
            }

            return true;
        }

        onOpened: {
//            console.debug("popupToolRing onOpened");
        }

        onClosed: {
//            console.debug("popupToolRing onClosed");
//            // Workaround to hide highlight.
            cloudFolderView.currentIndex = -1;
            cloudFolderGridView.currentIndex = -1;
        }

        onCutClicked: {
            clipboard.addItem({ "action": "cut", "type": cloudDriveModel.getCloudName(selectedCloudType), "uid": selectedUid, "sourcePath": sourcePath, "sourcePathName": selectedFileName });
        }

        onCopyClicked: {
            clipboard.addItem({ "action": "copy", "type": cloudDriveModel.getCloudName(selectedCloudType), "uid": selectedUid, "sourcePath": sourcePath, "sourcePathName": selectedFileName });
        }

        onPasteClicked: {
            console.debug("cloudFolderPage popupToolPanel onPasteClicked " + targetPath + " " + selectedFileName);
            fileActionDialog.targetPath = targetPath;
            fileActionDialog.targetPathName = selectedFileName;
            fileActionDialog.open();
        }

        onDeleteFile: {
            // Delete always clear clipboard.
            clipboard.clear();
            clipboard.addItem({ "action": "delete", "type": cloudDriveModel.getCloudName(selectedCloudType), "uid": selectedUid, "sourcePath": sourcePath, "sourcePathName": selectedFileName });
            fileActionDialog.open();
        }

        onPrintFile: {
            // Check if it's viewable.
            if (!cloudDriveModel.isViewable(selectedCloudType)) return;

            isBusy = true;

            var url;
            if (!cloudDriveModel.get(srcItemIndex).source) {
                // Request media to get URL.
                url = cloudDriveModel.media(selectedCloudType, selectedUid, srcFilePath);
            } else {
                // Get media URL.
                url = cloudDriveModel.get(srcItemIndex).source;
            }
            if (url && url != "") {
                gcpClient.printURLSlot(url);
            }

            isBusy = false;
        }

        onSyncFile: {
            cloudDriveModel.syncItemByRemotePath(selectedCloudType, selectedUid, srcFilePath);
        }

        onNewFolder: {
            newFolderDialog.open();
        }

        onRenameFile: {
            renameDialog.sourcePath = srcFilePath;
            renameDialog.sourcePathName = selectedFileName;
            renameDialog.open();
        }

        onMarkClicked: {
            cloudDriveModel.state = "mark";
            cloudDriveModel.setProperty(srcItemIndex, "isChecked", true);
        }

        onMailFile: {
            mailFileSlot(srcFilePath, selectedFileName);
        }

        onSmsFile: {
            smsFileSlot(srcFilePath, selectedFileName);
        }

        onShowInfo: {
            filePropertiesDialog.show(srcItemIndex);
        }
    }

    FileActionDialog {
        id: fileActionDialog
        selectedCloudType: cloudFolderPage.selectedCloudType
        selectedUid: cloudFolderPage.selectedUid
        remoteParentPath: cloudDriveModel.remoteParentPath

        onConfirm: {
            // It always replace existing names.
            cloudDriveModel.suspendNextJob();

            // TODO Copy/Move/Delete all files from clipboard.
            // Action is {copy, cut, delete}
            var res = true;
            for (var i=0; i<clipboard.count; i++) {
                var action = clipboard.get(i).action;
                var sourcePath = clipboard.get(i).sourcePath;
                var sourcePathName = (isOverwrite && i == 0) ? newFileName : (clipboard.get(i).sourcePathName ? clipboard.get(i).sourcePathName : cloudDriveModel.getFileName(sourcePath));
                var actualTargetPath = cloudDriveModel.getRemotePath(selectedCloudType, targetPath, sourcePathName);

                console.debug("cloudFolderPage fileActionDialog onConfirm clipboard type " + clipboard.get(i).type + " uid " + clipboard.get(i).uid + " action " + action + " sourcePathName " + sourcePathName + " sourcePath " + sourcePath + " targetPath " + targetPath + " actualTargetPath " + actualTargetPath);
                if (["copy","cut"].indexOf(action) > -1 && !clipboard.get(i).type) {
                    if (cloudDriveModel.isDir(sourcePath)) {
                        // Sync from local into target path (remote parent path).
                        cloudDriveModel.syncFromLocal(selectedCloudType, selectedUid, sourcePath, targetPath, -1, true);
                    } else {
                        cloudDriveModel.filePut(selectedCloudType, selectedUid, sourcePath, targetPath, sourcePathName, -1);
                    }
                    res = true;
                } else if (["copy","cut"].indexOf(action) > -1 && (cloudDriveModel.getClientType(clipboard.get(i).type) != selectedCloudType || clipboard.get(i).uid != selectedUid) ) {
                    console.debug("cloudFolderPage fileActionDialog onConfirm migrate clipboard type " + clipboard.get(i).type + " uid " + clipboard.get(i).uid + " sourcePath " + sourcePath + " targetPath " + targetPath + " sourcePathName " + sourcePathName);
                    cloudDriveModel.migrateFile(cloudDriveModel.getClientType(clipboard.get(i).type), clipboard.get(i).uid, sourcePath, selectedCloudType, selectedUid, targetPath, sourcePathName);
                    res = true;
                } else if (action == "copy" && clipboard.get(i).type) {
                    cloudDriveModel.copyFile(cloudDriveModel.getClientType(clipboard.get(i).type), clipboard.get(i).uid, "", sourcePath, "", targetPath, sourcePathName);
                    res = true;
                } else if (action == "cut" && clipboard.get(i).type) {
                    cloudDriveModel.moveFile(cloudDriveModel.getClientType(clipboard.get(i).type), clipboard.get(i).uid, "", sourcePath, "", targetPath, sourcePathName);
                    res = true;
                } else if (action == "delete" && clipboard.get(i).type) {
                    cloudDriveModel.deleteFile(cloudDriveModel.getClientType(clipboard.get(i).type), clipboard.get(i).uid, "", sourcePath);
                    res = true;
                } else {
                    console.debug("cloudFolderPage fileActionDialog onConfirm invalid action " + action);
                    res = false;
                }

                if (!res) {
                    break;
                }
            }

            // Reset targetPath and popupToolPanel's paths.
            if (res) {
                // Reset both source and target.
                targetPath = "";
                popupToolPanel.srcFilePath = "";
                popupToolPanel.pastePath = "";

                // Clear clipboard as they should have been processed.
                clipboard.clear();
            } else {
                // Reset target only.
                targetPath = "";
                popupToolPanel.pastePath = "";
            }
        }

        onClosed: {
            // Always clear clipboard's delete actions.
            clipboard.clearDeleteActions();

            // Resume queued jobs.
            cloudDriveModel.resumeNextJob();
        }
    }

    NewFolderDialog {
        id: newFolderDialog
        titleText: appInfo.emptyStr+qsTr("New folder")
        folderOnly: true

        onConfirm: {
            if (nameField.text !== "") {
                createRemoteFolder(nameField.text.trim());
            }
        }
    }

    RenameDialog {
        id: renameDialog

        onConfirm: {
            if (nameField.text != "" && nameField.text != sourcePathName) {
                renameRemotePath(sourcePath, nameField.text);
            }
        }
    }

    FilePropertiesDialog {
        id: filePropertiesDialog
        isCloudFolder: true

        function show(index) {
            selectedIndex = index;
            selectedItem = cloudDriveModel.get(selectedIndex);
            populateCloudItemModel();
            open();
        }

        function populateCloudItemModel() {
            cloudItemModel.clear();
            if (selectedItem) {
                var cloudItems = Utility.createJsonObj(cloudDriveModel.getItemListJsonByRemotePath(selectedCloudType, selectedUid, selectedItem.absolutePath));
                for (var i = 0; i < cloudItems.length; i++) {
                    var cloudItem = cloudItems[i];
                    var modelItem = { absolutePath: (cloudItem.local_path ? cloudItem.local_path : qsTr("Not available")) };
                    cloudItemModel.append(modelItem);
                }
            }
        }

        onSyncAll: {
            cloudDriveModel.syncItemByRemotePath(selectedCloudType, selectedUid, selectedItem.absolutePath);
            close();
        }
        onDisconnect: {
            cloudDriveModel.disconnect(type, uid, absolutePath, selectedItem.absolutePath);
            close();
        }
    }

    ConfirmDialog {
        id: clearCachedImagesConfirmation
        titleText: appInfo.emptyStr+qsTr("Reset image cache")
        contentText: appInfo.emptyStr+qsTr("Reset image cache on current folder?")
        onConfirm: {
            cloudDriveModel.clearCachedImagesOnCurrentRemotePath();
        }
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            // Request cloud job queue status.
            cloudDriveModel.requestJobQueueStatus();
        }
    }

    Component.onCompleted: {
        console.debug(Utility.nowText() + " cloudFolderPage onCompleted");
        window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("CloudFolderPage"), 0.1);

        // Proceeds queued jobs during constructions.
        refreshSlot("cloudFolderPage onStatusChanged");
    }
}
