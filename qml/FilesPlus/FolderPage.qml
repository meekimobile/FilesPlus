import QtQuick 1.1
import com.nokia.symbian 1.1
import Charts 1.0
import FolderSizeItemListModel 1.0
import GCPClient 1.0
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
        onResetCache: {
            resetCacheConfirmation.open();
        }
    }

    SortByMenu {
        id: sortByMenu

        onSelectSort: {
            console.debug("sortByMenu setSortFlag flag=" + flag);
            fsModel.setSortFlag(flag);
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
        height: parent.height - bluePanel.height
        x: 0
        anchors.top: bluePanel.bottom

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

                Row {
                    anchors.fill: listItem.paddingItem
                    spacing: 5
                    Image {
                        id: icon1
                        x: 0
                        y: 0
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
    }

    Rectangle {
        id: bluePanel
        anchors.top: parent.top
        width: parent.width
        height: 40
        color: "transparent"

        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            border.color: "grey"
            border.width: 1
            radius: 4
            color: "transparent"

            Text {
                id: currentPath
                anchors.fill: parent
                anchors.margins: 4
                color: "white"

                MouseArea {
                    anchors.fill: parent

                    onPressAndHold: {
                        if (folderPage.state == "list") {
                            popupToolPanel.forFile = false;
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
    }

    CommonDialog {
        id: uploadProgressDialog

        property alias srcFilePath: uploadFilePath.text
        property alias message: uploadMessage.text
        property alias min: uploadProgressBar.minimumValue
        property alias max: uploadProgressBar.maximumValue
        property alias value: uploadProgressBar.value
        property bool autoClose: true

        titleText: "Uploading"
        titleIcon: "FilesPlusIcon.svg"
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
                        uploadProgressDialog.autoClose = true;
                        uploadProgressDialog.close();
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
                color: "white"
                width: parent.width
                height: 1
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
    }

    CommonDialog {
        id: downloadProgressDialog

        property alias targetFilePath: targetFilePath.text
        property alias message: downloadMessage.text
        property alias min: downloadProgressBar.minimumValue
        property alias max: downloadProgressBar.maximumValue
        property alias value: downloadProgressBar.value
        property bool autoClose: true

        titleText: "Downloading"
        titleIcon: "FilesPlusIcon.svg"
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
                        downloadProgressDialog.autoClose = true;
                        downloadProgressDialog.close();
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
                color: "white"
                width: parent.width
                height: 1
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
    }

    GCPClient {
        id: gcpClient

        onAuthorizeRedirectSignal: {
            console.debug("sandboxPage gcpClient onAuthorizeRedirectSignal " + url);
            pageStack.push(Qt.resolvedUrl("AuthPage.qml"), { url: url });
        }

        onAccessTokenReplySignal: {
            console.debug("sandboxPage gcpClient onAccessTokenReplySignal " + err + " " + errMsg + " " + msg);

            if (err == 0) {
                // Resume printing
                printFileSlot(popupToolPanel.selectedFilePath);
            } else {
                gcpClient.refreshAccessToken();
            }
        }

        onRefreshAccessTokenReplySignal: {
            console.debug("sandboxPage gcpClient onRefreshAccessTokenReplySignal " + err + " " + errMsg + " " + msg);

            if (err == 0) {
                // Resume printing
                printFileSlot(popupToolPanel.selectedFilePath);
            } else {
                // TODO Notify failed refreshAccessToken.
            }
        }

        onAccountInfoReplySignal: {
            console.debug("sandboxPage gcpClient onAccountInfoReplySignal " + err + " " + errMsg + " " + msg);

            var jsonObj = Utility.createJsonObj(msg);
            console.debug("jsonObj.email " + jsonObj.email);
        }

        onSearchReplySignal: {
            console.debug("sandboxPage gcpClient onSearchReplySignal " + err + " " + errMsg + " " + msg);

            if (err == 0) {
                // Once search done, open printerSelectionDialog
                printFileSlot(popupToolPanel.selectedFilePath);
            } else {
                gcpClient.refreshAccessToken();
            }
        }

        onSubmitReplySignal: {
//            console.debug("sandboxPage gcpClient onSubmitReplySignal " + err + " " + errMsg + " " + msg);

            // Notify submit result.
            var jsonObj = Utility.createJsonObj(msg);
            var message = "";
            if (err != 0) {
                message = "Error " + err + " " + errMsg + "\n";
            }
            message += jsonObj.message;

            uploadProgressDialog.message = message;
            uploadProgressDialog.buttonTexts = ["OK"];
        }

        onDownloadProgress: {
//            console.debug("sandBox gcpClient onDownloadProgress " + bytesReceived + " / " + bytesTotal);

            // Shows in progress bar.
            if (downloadProgressDialog.status != DialogStatus.Open) {
                downloadProgressDialog.open();
                downloadProgressDialog.buttonTexts = "";
                downloadProgressDialog.min = 0;
                downloadProgressDialog.max = bytesTotal;
            }
            downloadProgressDialog.value = bytesReceived;
        }

        onUploadProgress: {
//            console.debug("sandBox gcpClient onUploadProgress " + bytesSent + " / " + bytesTotal);

            // Shows in progress bar.
            if (uploadProgressDialog.status != DialogStatus.Open) {
                uploadProgressDialog.open();
                uploadProgressDialog.buttonTexts = "";
                uploadProgressDialog.min = 0;
                uploadProgressDialog.max = bytesTotal;
            }
            uploadProgressDialog.value = bytesSent;
        }
    }
}
