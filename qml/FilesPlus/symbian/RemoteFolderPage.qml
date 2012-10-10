import QtQuick 1.1
import com.nokia.symbian 1.1
import CloudDriveModel 1.0
import "Utility.js" as Utility

// TODO Not done yet.

Page {
    id: remoteFolderPage

    property string name: "remoteFolderPage"
    property string currentDir

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
    }

    tools: toolBarLayout

    ToolBarLayout {
        id: toolBarLayout

        ToolButton {
            id: backButton
            iconSource: "toolbar-back"
            platformInverted: window.platformInverted
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
            platformInverted: window.platformInverted
            flat: true
            visible: (fsListView.state == "")

            Component.onCompleted: {
                refreshButton.clicked.connect(refreshSlot);
            }
        }

        ToolButton {
            id: flipButton
            iconSource: (folderPage.state != "list") ? (!window.platformInverted ? "list.svg" : "list_inverted.svg") : (!window.platformInverted ? "chart.svg" : "chart_inverted.svg")
            platformInverted: window.platformInverted
            flat: true
            visible: (fsListView.state == "")

            Component.onCompleted: {
                flipButton.clicked.connect(flipSlot);
            }
        }

        ToolButton {
            id: cloudButton
            iconSource: (!window.platformInverted ? "cloud.svg" : "cloud_inverted.svg")
            platformInverted: window.platformInverted
            visible: (fsListView.state == "")
            onClicked: {
                syncConnectedItemsSlot();
            }
        }

        ToolButton {
            id: menuButton
            iconSource: "toolbar-menu"
            platformInverted: window.platformInverted
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

    function refreshSlot() {
        fsModel.nameFilters = [];
        fsModel.refreshDir("folderPage refreshSlot", false);
    }

    function goUpSlot() {
        if (fsModel.isRoot()) {
            // Flip back to list view, then push drivePage.
            flipable1.flipped = false;
            pageStack.push(Qt.resolvedUrl("DrivePage.qml"), {}, true);
        } else {
            if (state == "chart") {
                fsModel.changeDir("..", FolderSizeItemListModel.SortBySize);
            } else {
                fsModel.changeDir("..");
            }
        }
    }

    function flipSlot() {
        flipable1.flipped = !flipable1.flipped;
    }

    function orientationChangeSlot() {
    }

    function activateSlot() {
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

        onMovementStarted: {
            if (currentItem) currentItem.state = "normal";
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
            property real mouseX: 0
            property real mouseY: 0
            property string fileName: name
            property string filePath: absolutePath
            property int clipboardIndex: clipboard.getModelIndex(absolutePath)

            function getIconSource() {
                var viewableImageFileTypes = ["JPG", "PNG", "SVG"];
                var viewableTextFileTypes = ["TXT", "HTML"];

                if (isDir) {
                    return "folder_list.svg";
                } else if (viewableImageFileTypes.indexOf(fileType.toUpperCase()) != -1) {
                    var showThumbnail = appInfo.getSettingValue("thumbnail.enabled", false);
//                    console.debug("folderPage listDelegate getIconSource showThumbnail " + showThumbnail);
                    if (showThumbnail == "true") {
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
                        source: (!window.platformInverted) ? "ok.svg" : "ok_inverted.svg"
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
                        ListItemText {
                            mode: listItem.mode
                            role: "Title"
                            text: name
                            width: parent.width - sizeText.width
                            height: parent.height
                            elide: Text.ElideMiddle
                            verticalAlignment: Text.AlignVCenter
                            platformInverted: window.platformInverted
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
                            platformInverted: window.platformInverted
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
                                    if (subDirCount > 0) sub += appInfo.emptyStr+qsTr("%n dir(s)", "", subDirCount);
                                    if (subFileCount > 0) sub += ((sub == "") ? "" : " ") + appInfo.emptyStr+qsTr("%n file(s)", "", subFileCount);
                                    sub += ((sub == "") ? "" : ", ") + appInfo.emptyStr+qsTr("last modified") + " " + Qt.formatDateTime(lastModified, "d MMM yyyy h:mm:ss ap");

                                    return sub;
                                }
                                width: parent.width
                                height: parent.height
                                elide: Text.ElideMiddle
                                verticalAlignment: Text.AlignVCenter
                                visible: !isRunning
                                platformInverted: window.platformInverted
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
                                            return (!window.platformInverted) ? "refresh.svg" : "refresh_inverted.svg"
                                        case FolderSizeItemListModel.UploadOperation:
                                            return (!window.platformInverted) ? "upload.svg" : "upload_inverted.svg"
                                        case FolderSizeItemListModel.DownloadOperation:
                                            return (!window.platformInverted) ? "download.svg" : "download_inverted.svg"
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
                    popupToolPanel.isDir = isDir;
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
    }

    MessageDialog {
        id: messageDialog
    }

    ClipboardModel {
        id: clipboard
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
        ringRadius: 65
        buttonRadius: 25
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

        onMailFile: {
            mailFileSlot(srcFilePath, srcItemIndex);
        }

        onSmsFile: {
            smsFileSlot(srcFilePath, srcItemIndex);
        }

        onBluetoothFile: {
            bluetoothFileSlot(srcFilePath, srcItemIndex);
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
                    console.debug("fileActionDialog getText clipboard i " + i + " action " + clipboard.get(i).action + " sourcePath " + clipboard.get(i).sourcePath);
                    if (clipboard.get(i).action == "copy") {
                        copyCount++;
                    } else if (clipboard.get(i).action == "cut") {
                        cutCount++;
                    } else if (clipboard.get(i).action == "delete") {
                        deleteCount++;
                    }
                }

                if (deleteCount>0) text = text + (qsTr("Delete %n file(s)\n", "", deleteCount));
                if (copyCount>0) text = text + (qsTr("Copy %n file(s)\n", "", copyCount));
                if (cutCount>0) text = text + (qsTr("Move %n file(s)\n", "", cutCount));
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
//                    console.debug("fileActionDialog openCopyProgressDialog estimate total itemJson " + itemJson + " itemJson.size " + itemJson.size + " itemJson.sub_file_count " + itemJson.sub_file_count);
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
                copyProgressDialog.minCount = 0;
                copyProgressDialog.maxCount = totalFiles + totalFolders;
                copyProgressDialog.count = 0;
                copyProgressDialog.indeterminate = true;
                copyProgressDialog.autoClose = false;
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
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 5

            Row {
                width: parent.width
                spacing: platformStyle.paddingMedium
                RadioButton {
                    id: newFolderButton
                    text: appInfo.emptyStr+qsTr("Folder")
                    onClicked: {
                        newFileButton.checked = false;
                    }
                }
                RadioButton {
                    id: newFileButton
                    text: appInfo.emptyStr+qsTr("File")
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
            spacing: 3

            Text {
                width: parent.width
                text: appInfo.emptyStr+qsTr("Rename %1 to").arg(fsModel.getFileName(renameDialog.sourcePath));
                color: "white"
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
            spacing: 3

            Text {
                text: appInfo.emptyStr+qsTr("Please input new file name.")
                color: "white"
                width: parent.width
                height: 48
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            }

            TextField {
                id: fileName
                width: parent.width
            }

            CheckBox {
                id: overwriteFile
                text: appInfo.emptyStr+qsTr("Overwrite existing file")
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
    }

    CloudDriveModel {
        id: cloudDriveModel

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

            // Get uid list from DropboxClient.
            var dbUidList = cloudDriveModel.getStoredUidList(CloudDriveModel.Dropbox);

            // Construct model.
            var model = Qt.createQmlObject(
                        'import QtQuick 1.1; ListModel {}', folderPage);

            for (var i=0; i<dbUidList.length; i++)
            {
                var json = JSON.parse(dbUidList[i]);
                var localHash = cloudDriveModel.getItemHash(localPath, CloudDriveModel.Dropbox, json.uid);
                console.debug("getUidListModel i " + i + " uid " + json.uid + " email " + json.email + " localHash " + localHash);
                model.append({
                                 type: CloudDriveModel.Dropbox,
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
                console.debug("jsonObj.email " + jsonObj.email);

                // Send info to cloudDriveAccountsPage.
                var p = pageStack.find(function (page) { return (page.name == "cloudDriveAccountsPage"); });
                if (p) {
                    p.updateAccountInfoSlot(cloudDriveJobJson.type, jsonObj.uid, jsonObj.name, jsonObj.email, jsonObj.quota_info.shared, jsonObj.quota_info.normal, jsonObj.quota_info.quota);
                }
            } else {
                messageDialog.titleText = appInfo.emptyStr+qsTr("CloudDrive Account Info");
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
                if (modelIndex > -1) {
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
                messageDialog.titleText = getCloudName(job.type) + " " + appInfo.emptyStr+qsTr("File Get");
                messageDialog.message = appInfo.emptyStr+qsTr("Error") + " " + err + " " + errMsg + " " + msg;
                messageDialog.open();
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

            // Update ProgressBar on listItem.
            // TODO also show running on its parents.
            fsModel.setProperty(json.local_file_path, FolderSizeItemListModel.IsRunningRole, isRunning);

            // Show indicator on parent up to root.
            // Skip i=0 as it's notified above already.
            var pathList = fsModel.getPathToRoot(json.local_file_path);
            for(var i=1; i<pathList.length; i++) {
                modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
                if (modelIndex > -1) {
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
                    messageDialog.message = appInfo.emptyStr+qsTr("%1 was removed remotely.\nLink will be removed.").arg(localPath);
                    messageDialog.autoClose = true;
                    messageDialog.open();
//                    return;
                } else {
                    // Sync starts from itself.
                    if (jsonObj.is_dir) { // Sync folder.
                        // TODO if there is no local folder, create it and connect.
                        if (!fsModel.isDir(localPath)) {
                            fsModel.createDirPath(localPath);
                        }

                        // Sync based on remote contents.
//                        console.debug("cloudDriveModel onMetadataReplySignal folder jsonObj.rev " + jsonObj.rev + " jsonObj.hash " + jsonObj.hash + " localPathHash " + localPathHash);
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

//                            // Add cloudDriveItem for currentDir.
//                            cloudDriveModel.addItem(type, uid, localPath, remotePath, jsonObj.hash);
                        }

                        // Add or Update timestamp from local to cloudDriveItem.
                        cloudDriveModel.addItem(type, uid, localPath, remotePath, jsonObj.hash);

                        // Sync based on local contents.
                        cloudDriveModel.syncFromLocal(type, uid, localPath, remotePath, modelIndex);
                    } else { // Sync file.
//                        console.debug("cloudDriveModel onMetadataReplySignal file jsonObj.rev " + jsonObj.rev + " localPathHash " + localPathHash);

                        // TODO if (rev is newer or there is no local file), get from remote.
                        if (jsonObj.rev > localPathHash || !fsModel.isFile(localPath)) {
                            // TODO it should be specified with localPath instead of always use defaultLocalPath.
                            cloudDriveModel.fileGet(type, uid, remotePath, localPath, modelIndex);
                        } else if (jsonObj.rev < localPathHash) {
                            cloudDriveModel.filePut(type, uid, localPath, remotePath, modelIndex);
                        } else {
                            // Update lastModified on cloudDriveItem.
                            cloudDriveModel.addItem(type, uid, localPath, remotePath, jsonObj.rev);
                        }
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
                messageDialog.message = appInfo.emptyStr+qsTr("Error") + " " + err + " " + errMsg + " " + msg;
                messageDialog.open();
            }

            // Update ProgressBar on listItem.
            fsModel.setProperty(localPath, FolderSizeItemListModel.IsRunningRole, isRunning);

            // Show indicator on parent up to root.
            // Skip i=0 as it's notified above already.
            var pathList = fsModel.getPathToRoot(localPath);
            for(var i=1; i<pathList.length; i++) {
                modelIndex = fsModel.getIndexOnCurrentDir(pathList[i]);
                if (modelIndex > -1) {
                    fsModel.setProperty(modelIndex, FolderSizeItemListModel.IsRunningRole, isRunning);
                }
            }
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
                messageDialog.autoClose = true;
                messageDialog.open();
            }

            // Update ProgressBar on listItem.
            // TODO also show running on its parents.
            fsModel.setProperty(json.local_file_path, FolderSizeItemListModel.IsRunningRole, isRunning);

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
        }

        onMoveFileReplySignal: {
            console.debug("folderPage cloudDriveModel onMoveFileReplySignal " + nonce + " " + err + " " + errMsg + " " + msg);

            var jobJson = Utility.createJsonObj(cloudDriveModel.getJobJson(nonce));

            console.debug("folderPage cloudDriveModel onMoveFileReplySignal jobJson " + cloudDriveModel.getJobJson(nonce));

            // Remove finished job.
            cloudDriveModel.removeJob(nonce);

            if (err == 0) {
                var msgJson = Utility.createJsonObj(msg);
                if (msgJson.is_dir) {
                    // Connect folder to cloud.
                    switch (jobJson.type) {
                    case CloudDriveModel.Dropbox:
                        cloudDriveModel.removeItem(CloudDriveModel.Dropbox, jobJson.uid, jobJson.local_file_path);
                        cloudDriveModel.addItem(CloudDriveModel.Dropbox, jobJson.uid, jobJson.new_local_file_path, jobJson.new_remote_file_path, msgJson.hash);
                        for (var i=0; i<msgJson.contents.length; i++) {
                            var contentJson = msgJson.contents[i];
                            if (contentJson.is_dir) {
                                // TODO connect its sub files/folders.
                                cloudDriveModel.addItem(CloudDriveModel.Dropbox, jobJson.uid, cloudDriveModel.getDefaultLocalFilePath(contentJson.path), contentJson.path, contentJson.rev);
                            } else {
                                cloudDriveModel.addItem(CloudDriveModel.Dropbox, jobJson.uid, cloudDriveModel.getDefaultLocalFilePath(contentJson.path), contentJson.path, contentJson.rev);
                            }
                        }
                        break;
                    }
                } else {
                    // Connect file to cloud.
                    switch (json.type) {
                    case CloudDriveModel.Dropbox:
                        cloudDriveModel.removeItem(CloudDriveModel.Dropbox, jobJson.uid, jobJson.local_file_path);
                        cloudDriveModel.addItem(CloudDriveModel.Dropbox, jobJson.uid, jobJson.new_local_file_path, jobJson.new_remote_file_path, msgJson.rev);
                        break;
                    }
                }
            } else {
                messageDialog.titleText = getCloudName(json.type) + " " + appInfo.emptyStr+qsTr("Move");
                messageDialog.message = appInfo.emptyStr+qsTr("Error") + " " + err + " " + errMsg + " " + msg;
                messageDialog.autoClose = true;
                messageDialog.open();
            }

            // Update ProgressBar on listItem.
            // TODO also show running on its parents.
            fsModel.setProperty(jobJson.new_local_file_path, FolderSizeItemListModel.IsRunningRole, false);
            fsModel.refreshItem(jobJson.new_local_file_path);
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
                messageDialog.autoClose = true;
                messageDialog.open();
            }

            // Update ProgressBar on listItem.
            // TODO also show running on its parents.
            fsModel.setProperty(json.local_file_path, FolderSizeItemListModel.IsRunningRole, isRunning);

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
                    recipientSelectionDialog.messageSubject = appInfo.emptyStr+qsTr("Share file on Dropbox");
                    recipientSelectionDialog.messageBody = appInfo.emptyStr+qsTr("Please download file with below link.") + "\n" + url;
                    recipientSelectionDialog.senderEmail = senderEmail;
                    recipientSelectionDialog.open();
                }
            } else if (err == 202) { // Forbidden nonce is already used.
                // Retry.
                cloudDriveModel.shareFile(json.type, json.uid, json.local_file_path, json.remote_file_path);
            } else {
                messageDialog.titleText = getCloudName(json.type) + " " + appInfo.emptyStr+qsTr("Share");
                messageDialog.message = appInfo.emptyStr+qsTr("Error") + " " + err + " " + errMsg + " " + msg;
                messageDialog.autoClose = true;
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
            case CloudDriveModel.ShareFile:
                cloudDriveModel.shareFile(type, uid, localPath, remotePath);
                break;
            case CloudDriveModel.DeleteFile:
                cloudDriveModel.deleteFile(type, uid, localPath, remotePath);
                break;
            }
        }

        onOpening: {
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