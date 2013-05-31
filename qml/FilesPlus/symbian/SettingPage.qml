import QtQuick 1.1
import com.nokia.symbian 1.1
import com.nokia.extras 1.0

Page {
    id: settingPage

    property string name: "settingPage"
    property int jobQueueCount: 0
    property int cloudDriveItemCount: 0
    property bool inverted: window.platformInverted

    function getIndexByName(name) {
        for (var i=0; i<settingModel.count; i++) {
            if (settingModel.get(i).name == name) {
                return i;
            }
        }

        return -1;
    }

    function updateJobQueueCount(runningJobCount, jobQueueCount) {
        var i = getIndexByName("cancelAllCloudDriveJobs");
        if (i > -1) {
            settingPage.jobQueueCount = jobQueueCount;
        }
    }

    function updateCloudDriveItemCount(cloudDriveItemCount) {
        var i = getIndexByName("syncAllConnectedItems");
        if (i > -1) {
            settingPage.cloudDriveItemCount = cloudDriveItemCount;
        }
    }

    tools: ToolBarLayout {
        ToolButton {
            id: back
            iconSource: "toolbar-back"
            platformInverted: inverted
            onClicked: {
                pageStack.pop();
            }
        }
    }

    TitlePanel {
        id: titlePanel
        text: qsTr("Settings") + appInfo.emptyStr
        z: 1
    }

    ConfirmDialog {
        id: quitConfirmation
        titleText: appInfo.emptyStr+qsTr("Logging (Debug)")
        contentText: appInfo.emptyStr+qsTr("Changing logging switch requires restart.\nFilesPlus is exiting now.\n\nPlease confirm.")
        onConfirm: {
            Qt.quit();
        }
    }

    ListModel {
        id: settingModel
        ListElement {
            name: "CloudPrint"
            title: ""
            type: "section"
            group: "CloudPrint"
        }
        ListElement {
            name: "showCloudPrintJobs"
            title: ""
            type: "button"
            group: "CloudPrint"
        }
        ListElement {
            name: "printFromURL"
            title: ""
            type: "button"
            group: "CloudPrint"
        }
        ListElement {
            name: "resetCloudPrint"
            title: ""
            type: "button"
            group: "CloudPrint"
        }
        ListElement {
            name: "Spacer"
            title: ""
            type: "spacer"
            group: "Spacer"
        }
        ListElement {
            name: "CloudDrive"
            title: ""
            type: "section"
            group: "CloudDrive"
        }
        ListElement {
            name: "showCloudDriveJobs"
            title: ""
            type: "button"
            group: "CloudDrive"
        }
        ListElement {
            name: "cancelAllCloudDriveJobs"
            title: ""
            type: "button"
            group: "CloudDrive"
        }
        ListElement {
            name: "syncAllConnectedItems"
            title: ""
            type: "button"
            group: "CloudDrive"
        }
        ListElement {
            name: "showCloudDriveAccounts"
            title: ""
            type: "button"
            group: "CloudDrive"
        }
//        ListElement {
//            name: "sync.after.refresh"
//            title: ""
//            type: "switch"
//            group: "CloudDrive"
//        }
        ListElement {
            name: "CloudFolderPage.thumbnail.enabled"
            title: ""
            type: "switch"
            group: "CloudDrive"
        }
        ListElement {
            name: "drivepage.clouddrive.enabled"
            title: ""
            type: "switch"
            group: "CloudDrive"
        }
        ListElement {
            name: "Spacer"
            title: ""
            type: "spacer"
            group: "Spacer"
        }
        ListElement {
            name: "FolderPie"
            title: ""
            type: "section"
            group: "FolderPie"
        }
        ListElement {
            name: "resetCache"
            title: ""
            type: "button"
            group: "FolderPie"
        }
        ListElement {
            name: "Personalization"
            title: ""
            type: "section"
            group: "Personalization"
        }
        ListElement {
            name: "locale"
            title: ""
            type: "button"
            group: "Personalization"
        }
        ListElement {
            name: "Theme.inverted"
            title: ""
            type: "switch"
            group: "Personalization"
        }
        ListElement {
            name: "popup.timer.interval"
            title: ""
            type: "popupInterval"
            group: "Personalization"
        }
        ListElement {
            name: "listItem.compact.enabled"
            title: ""
            type: "switch"
            group: "Personalization"
        }
        ListElement {
            name: "thumbnail.enabled"
            title: ""
            type: "switch"
            group: "Personalization"
        }
        ListElement {
            name: "automatically.bluetooth.on"
            title: ""
            type: "switch"
            group: "Personalization"
        }
        ListElement {
            name: "keep.bluetooth.off"
            title: ""
            type: "switch"
            group: "Personalization"
        }
        ListElement {
            name: "statusbar.enabled"
            title: ""
            type: "switch"
            group: "Personalization"
        }
        ListElement {
            name: "Spacer"
            title: ""
            type: "spacer"
            group: "Spacer"
        }
        ListElement {
            name: "Developer"
            title: ""
            type: "section"
            group: "Developer"
        }
        ListElement {
            name: "Monitoring.enabled"
            title: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "Logging.enabled"
            title: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "dropbox.fullaccess.enabled"
            title: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "drivepage.systemdrive.enabled"
            title: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "drivepage.privatedrive.enabled"
            title: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "drivepage.trash.enabled"
            title: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "WebDavClient.ignoreSSLSelfSignedCertificateErrors"
            title: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "LocalFileImageProvider.cache.enabled"
            title: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "LocalFileImageProvider.CacheImageWorker.enabled"
            title: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "RemoteImageProvider.CacheImageWorker.enabled"
            title: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "CloudDriveModel.metadata.root.connection.enabled"
            title: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "CloudDriveModel.syncDirtyItems.enabled"
            title: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "FolderSizeModelThread.getDirContent.showHiddenSystem.enabled"
            title: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "FolderSizeItemListModel.deleteFile.use.trash.enabled"
            title: ""
            type: "switch"
            group: "Developer"
        }
    }

    GridView {
        id: settingList
        width: parent.width
        height: parent.height - titlePanel.height
        anchors.top: titlePanel.bottom
        model: settingModel
        delegate: settingListItemDelegate
        cacheBuffer: 1200
        cellWidth: (window.inPortrait) ? parent.width : (parent.width / 2)
        cellHeight: (window.inPortrait) ? 52 : 50
        flow: (window.inPortrait) ? GridView.LeftToRight : GridView.TopToBottom
        snapMode: GridView.SnapToRow
        pressDelay: 100
    }

    function getTitle(name) {
        if (name == "showCloudPrintJobs") return qsTr("Show cloud print jobs") + appInfo.emptyStr;
        else if (name == "printFromURL") return qsTr("Print from URL") + appInfo.emptyStr;
        else if (name == "resetCloudPrint") return qsTr("Reset cloud print") + appInfo.emptyStr;
        else if (name == "showCloudDriveJobs") return qsTr("Show cloud drive jobs") + appInfo.emptyStr;
        else if (name == "cancelAllCloudDriveJobs") return qsTr("Cancel queued jobs") + " (" + settingPage.jobQueueCount + ")" + appInfo.emptyStr;
        else if (name == "syncAllConnectedItems") return qsTr("Sync all connected items") + " (" + settingPage.cloudDriveItemCount + ")" + appInfo.emptyStr;
        else if (name == "showCloudDriveAccounts") return qsTr("Show accounts") + appInfo.emptyStr;
        else if (name == "sync.after.refresh") return qsTr("Auto-sync after refresh") + appInfo.emptyStr;
        else if (name == "drivepage.clouddrive.enabled") return qsTr("Show cloud storage on drive page") + appInfo.emptyStr;
        else if (name == "FolderPie.enabled") return qsTr("FolderPie feature") + appInfo.emptyStr;
        else if (name == "resetCache") return qsTr("Reset current folder cache") + appInfo.emptyStr;
        else if (name == "Theme.inverted") return qsTr("Theme") + appInfo.emptyStr;
        else if (name == "popup.timer.interval") return qsTr("Popup interval") + appInfo.emptyStr;
        else if (name == "listItem.compact.enabled") return qsTr("Compact list item") + appInfo.emptyStr;
        else if (name == "locale") return languageModel.getLanguage(appInfo.getLocale(), appInfo.getSystemLocale()) + appInfo.emptyStr;
        else if (name == "thumbnail.enabled") return qsTr("Show thumbnail on local drive") + appInfo.emptyStr;
        else if (name == "CloudFolderPage.thumbnail.enabled") return qsTr("Show thumbnail on cloud drive") + appInfo.emptyStr;
        else if (name == "automatically.bluetooth.on") return qsTr("Turn bluetooth on automatically") + appInfo.emptyStr;
        else if (name == "keep.bluetooth.off") return qsTr("Keep bluetooth off") + appInfo.emptyStr;
        else if (name == "statusbar.enabled") return qsTr("Show status bar") + appInfo.emptyStr;
        else if (name == "Logging.enabled") return qsTr("Logging (Debug)") + appInfo.emptyStr;
        else if (name == "dropbox.fullaccess.enabled") return qsTr("Dropbox full access") + appInfo.emptyStr;
        else if (name == "Monitoring.enabled") return qsTr("Monitoring (RAM,CPU)") + appInfo.emptyStr;
        else if (name == "Personalization") return qsTr("Personalization") + appInfo.emptyStr;
        else if (name == "Developer") return qsTr("Developer") + appInfo.emptyStr;
        else if (name == "drivepage.systemdrive.enabled") return qsTr("Show system drive on drive page") + appInfo.emptyStr;
        else if (name == "drivepage.privatedrive.enabled") return qsTr("Show private drive on drive page") + appInfo.emptyStr;
        else if (name == "drivepage.trash.enabled") return qsTr("Show trash on drive page") + appInfo.emptyStr;
        else if (name == "WebDavClient.ignoreSSLSelfSignedCertificateErrors") return qsTr("Ignore WebDAV SSL self-signed certificate errors") + appInfo.emptyStr;
        else if (name == "LocalFileImageProvider.cache.enabled") return qsTr("Cache local images") + appInfo.emptyStr;
        else if (name == "LocalFileImageProvider.CacheImageWorker.enabled") return qsTr("Queue local image caching") + appInfo.emptyStr;
        else if (name == "RemoteImageProvider.CacheImageWorker.enabled") return qsTr("Queue cloud image caching") + appInfo.emptyStr;
        else if (name == "CloudDriveModel.metadata.root.connection.enabled") return qsTr("Sync to cloud storage root") + appInfo.emptyStr;
        else if (name == "CloudDriveModel.syncDirtyItems.enabled") return qsTr("Sync dirty items automatically") + appInfo.emptyStr;
        else if (name == "FolderSizeModelThread.getDirContent.showHiddenSystem.enabled") return qsTr("Show hidden/system files") + appInfo.emptyStr;
        else if (name == "FolderSizeItemListModel.deleteFile.use.trash.enabled") return qsTr("Delete by moving to trash") + appInfo.emptyStr;
        else return qsTr(name) + appInfo.emptyStr;
    }

    function buttonClickedHandler(name) {
        if (name == "showCloudPrintJobs") {
            pageStack.push(Qt.resolvedUrl("PrintJobsPage.qml"));
        } else if (name == "printFromURL") {
            pageStack.push(Qt.resolvedUrl("WebViewPage.qml"));
        } else if (name == "resetCloudPrint") {
            gcpClient.resetCloudPrintSlot();
        } else if (name == "showCloudDriveJobs") {
            pageStack.push(Qt.resolvedUrl("CloudDriveJobsPage.qml"));
        } else if (name == "cancelAllCloudDriveJobs") {
            cancelQueuedCloudDriveJobsConfirmation.open();
        } else if (name == "syncAllConnectedItems") {
            cloudDriveModel.syncItems();
        } else if (name == "showCloudDriveAccounts") {
            pageStack.push(Qt.resolvedUrl("CloudDriveAccountsPage.qml"));
        } else if (name == "resetCache") {
            resetCacheConfirmation.open();
        } else if (name == "Theme.inverted") {
            window.platformInverted = !window.platformInverted;
        } else if (name == "locale") {
            languageSelector.open();
        } else if (name == "Logging.enabled") {
            quitConfirmation.open();
        } else if (name == "dropbox.fullaccess.enabled") {
            dropboxFullAccessConfirmation.open();
        } else if (name == "Monitoring.enabled") {
            if (appInfo.isMonitoring()) {
                appInfo.toggleMonitoring();
                logInfo(qsTr("Monitoring"),
                        qsTr("Monitoring is enabled. Log file is ") + appInfo.getMonitoringFilePath());
            } else {
                appInfo.toggleMonitoring();
            }
        } else if (name == "drivepage.clouddrive.enabled") {
            var p = window.findPage("drivePage");
            if (p) p.refreshSlot("settingPage buttonClickedHandler drivepage.systemdrive.enabled");
        } else if (name == "drivepage.systemdrive.enabled") {
            var p = window.findPage("drivePage");
            if (p) p.refreshSlot("settingPage buttonClickedHandler drivepage.systemdrive.enabled");
        } else if (name == "drivepage.privatedrive.enabled") {
            var p = window.findPage("drivePage");
            if (p) p.refreshSlot("settingPage buttonClickedHandler drivepage.privatedrive.enabled");
        } else if (name == "drivepage.trash.enabled") {
            var p = window.findPage("drivePage");
            if (p) p.refreshSlot("settingPage buttonClickedHandler drivepage.trash.enabled");
        } else if (name == "FolderSizeModelThread.getDirContent.showHiddenSystem.enabled") {
            var p = window.findPage("folderPage");
            if (p) p.refreshSlot("settingPage buttonClickedHandler FolderSizeModelThread.getDirContent.showHiddenSystem.enabled");
        }
    }

    Component {
        id: settingListItemDelegate

        Item {
            width: settingList.cellWidth
            height: settingList.cellHeight

            Loader {
                id: itemLoader
                width: parent.width
                anchors.centerIn: parent
                property variant name
                property variant title
                property variant type
                property variant group
            }

            Binding {
                target: itemLoader
                property: "name"
                value: name
            }

            Binding {
                target: itemLoader
                property: "title"
                value: title
            }

            Binding {
                target: itemLoader
                property: "type"
                value: type
            }

            Binding {
                target: itemLoader
                property: "group"
                value: group
            }

            Component.onCompleted: {
//                console.debug("settingListItemDelegate onCompleted " + name + " " + type + " " + title + " " + group);
                if (type == "section") {
                    itemLoader.sourceComponent = settingListSectionDelegate;
                } else if (type == "button") {
                    itemLoader.sourceComponent = buttonItemDelegate;
                } else if (type == "switch") {
                    itemLoader.sourceComponent = switchItemDelegate;
                } else if (type == "popupInterval") {
                    itemLoader.sourceComponent = sliderItemDelegate;
                } else if (type == "locale") {
                    itemLoader.sourceComponent = localeSelectorItemDelegate;
                }
            }
        }
    }

    Component {
        id: buttonItemDelegate
        Button {
            id: settingButton
            width: parent.width - 20
            height: 44
            anchors.centerIn: parent
            text: (title=="") ? getTitle(name) : title
            platformInverted: inverted
            onClicked: {
                buttonClickedHandler(name);
            }
        }
    }

    Component {
        id: switchItemDelegate
        Row {
            width: parent.width - 40
            anchors.centerIn: parent
            Text {
                id: settingKey
                color: (!inverted) ? "white" : "black"
                font.pointSize: 6
                width: parent.width - settingValue.width
                height: parent.height
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                text: (title=="") ? getTitle(name) : title
            }
            Switch {
                id: settingValue
                anchors.verticalCenter: parent.verticalCenter
                checked: appInfo.getSettingBoolValue(name, false)
                onCheckedChanged: {
//                        console.debug("settingListItemDelegate settingValue checked " + checked + " appInfo.getSettingValue " + name + " " + appInfo.getSettingValue(name, false));
                    if (appInfo.setSettingValue(name, checked)) {
                        console.debug("settingListItemDelegate switch " + name + " checked " + checked + " is changed.");
                        // Handle only if it's enabled.
                        buttonClickedHandler(name);
                    }
                }
            }
        }
    }

    Component {
        id: sliderItemDelegate
        Row {
            width: parent.width - 40
            anchors.centerIn: parent
            Text {
                id: sliderLabel
                color: (!inverted) ? "white" : "black"
                font.pointSize: 6
                width: parent.width - sliderValue.width
                height: parent.height
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                text: (title=="") ? getTitle(name) : title
            }
            Slider {
                id: sliderValue
                width: parent.width * 0.65
                anchors.verticalCenter: parent.verticalCenter
                minimumValue: 2
                maximumValue: 10
                stepSize: 1
                valueIndicatorText: appInfo.emptyStr+qsTr("%n sec.", "", value)
                valueIndicatorVisible: true

                onPressedChanged: {
                    console.debug("settingPage sliderItemDelegate sliderValue onPressedChanged " + pressed + " value " + value);
                    if (!pressed) {
                        appInfo.setSettingValue(name, value);
                    }
                }

                Component.onCompleted: {
                    value = appInfo.getSettingValue(name, 5);
                    console.debug("settingPage sliderItemDelegate sliderValue onCompleted value " + value);
                }
            }
        }
    }

    Component {
        id: localeSelectorItemDelegate
        ButtonRow {
            id: localeSelector
            width: parent.width - 20
            anchors.centerIn: parent
            checkedButton: localeCN
            Button {
                id: localeEN
                property string locale: "en"
                text: appInfo.emptyStr+qsTr("English")
                checkable: true
                onClicked: {
                    appInfo.setLocale(locale);
                    buttonClickedHandler("locale");
                }
            }
            Button {
                id: localeRU
                property string locale: "ru"
                text: appInfo.emptyStr+qsTr("Russian")
                checkable: true
                onClicked: {
                    appInfo.setLocale(locale);
                    buttonClickedHandler("locale");
                }
            }
            Button {
                id: localeCN
                property string locale: "zh"
                text: appInfo.emptyStr+qsTr("Chinese")
                checkable: true
                onClicked: {
                    appInfo.setLocale(locale);
                    buttonClickedHandler("locale");
                }
            }

            Component.onCompleted: {
                var currentLocale = appInfo.getLocale();
                for (var i=0; i<localeSelector.children.length; i++) {
                    var child = localeSelector.children[i];
                    if (currentLocale.indexOf(child.locale) == 0) {
                        localeSelector.checkedButton = child;
                        break;
                    }
                }
            }
        }
    }

    Component {
        id: settingListSectionDelegate

        Row {
            width: parent.width - 20
            anchors.centerIn: parent
            spacing: 5

            Text {
                id: sectionText
                text: getTitle(name)
                color: (!inverted) ? "grey" : "black"
                font.pointSize: 6
                anchors.verticalCenter: parent.verticalCenter
            }

            Rectangle {
                width: parent.width - parent.spacing - sectionText.width
                height: 1
                color: (!inverted) ? "grey" : "black"
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    TumblerDialog {
        id: languageSelector
        titleText: appInfo.emptyStr+qsTr("Languages")
        acceptButtonText: appInfo.emptyStr+qsTr("OK")
        rejectButtonText: appInfo.emptyStr+qsTr("Cancel")
        columns: [ languageColumn ]
        onStatusChanged: {
            if (status == DialogStatus.Opening) {
                columns[0].selectedIndex = languageModel.getIndexByLocale( (appInfo.getLocale() !== "") ? appInfo.getLocale() : appInfo.getSystemLocale() );
            }
        }

        onAccepted: {
            var index = columns[0].selectedIndex;
            var selectedLocale = columns[0].items.get(index).locale;
            console.debug("languageSelector onAccepted locale " + selectedLocale);
            appInfo.setLocale(selectedLocale);
        }
    }

    TumblerColumn {
        id: languageColumn
        selectedIndex: 0
        items: ListModel {
            id: languageModel
            ListElement { locale: "en"; value: "English" }
            ListElement { locale: "ru"; value: "Русский" }
            ListElement { locale: "zh"; value: "中文(中国大陆)" }
            ListElement { locale: "de"; value: "Deutsch" }
            ListElement { locale: "it"; value: "Italiano" }

            function getIndexByLocale(locale) {
                for (var i=0; i<languageModel.count; i++) {
                    var item = languageModel.get(i);
                    if (locale.indexOf(item.locale) == 0) {
                        return i;
                    }
                }

                return -1;
            }

            function getLanguage(locale, defaultLocale) {
                var i = getIndexByLocale(locale)
                if (i > -1) {
                    return languageModel.get(i).value;
                } else {
                    i = getIndexByLocale(defaultLocale);
                    if (i > -1) {
                        return languageModel.get(i).value;
                    }
                }

                return "Locale not found";
            }
        }
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            cloudDriveModel.requestJobQueueStatus();
        }
    }
}

//            Column {
//                id: sliceColorsSelector
//                visible: (type == "sliceColors")
//                width: parent.width - 20
//                anchors.centerIn: parent
//                Text {
//                    width: parent.width
//                    text: "Slice colors"
//                    color: (!window.platformInverted) ? "white" : "black"
//                    font.pointSize: 7
//                }
//                Row {
//                    id: colorRow
//                    width: parent.width
//                    spacing: 2
//                    Repeater {
//                        id: repeater
//                        model: appInfo.getSettingValue("sliceColors", "cyan,magenta,yellow,red,green,blue,orchid,orange").split(",")
//                        Rectangle {
//                            width: (sliceColorsSelector.width - repeater.count * colorRow.spacing) / repeater.count
//                            height: 40
//                            radius: 3
//                            color: modelData

//                            MouseArea {
//                                anchors.fill: parent
//                                onClicked: {
//                                    console.debug("color " + parent.color + " " + repeater.count);
//                                }
//                            }
//                        }
//                    }
//                }
//            }
