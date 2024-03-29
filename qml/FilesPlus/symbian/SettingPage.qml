import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: settingPage

    property string name: "settingPage"
    property int jobQueueCount: cloudDriveModel.queuedJobCount
    property int cloudDriveItemCount: cloudDriveModel.itemCount
    property bool inverted: window.platformInverted

    function getIndexByName(name) {
        for (var i=0; i<settingModel.count; i++) {
            if (settingModel.get(i).name == name) {
                return i;
            }
        }

        return -1;
    }

    state: "Personalization" // NOTE Default showing personalization.
    states: [
        State {
            name: "Personalization"
            PropertyChanges {
                target: settingList
                model: personalizationSettingModel
            }
        },
        State {
            name: "CloudPrint"
            PropertyChanges {
                target: settingList
                model: cloudPrintSettingModel
            }
        },
        State {
            name: "CloudDrive"
            PropertyChanges {
                target: settingList
                model: cloudDriveSettingModel
            }
        },
        State {
            name: "Developer"
            PropertyChanges {
                target: settingList
                model: developerSettingModel
            }
        }
    ]

    tools: ToolBarLayout {
        ToolBarButton {
            id: back
            buttonIconSource: "toolbar-back"
            onClicked: {
                pageStack.pop();
            }
        }
        ToolBarButton {
            id: personalizationTab
            buttonIconSource: (!inverted) ? "favourite.svg" : "favourite_inverted.svg"
            checked: (settingPage.state == "Personalization")
            onClicked: {
                settingPage.state = "Personalization";
            }
        }
        ToolBarButton {
            id: cloudPrintTab
            buttonIconSource: (!inverted) ? "print.svg" : "print_inverted.svg"
            checked: (settingPage.state == "CloudPrint")
            onClicked: {
                settingPage.state = "CloudPrint";
            }
        }
        ToolBarButton {
            id: cloudDriveTab
            buttonIconSource: (!inverted) ? "cloud.svg" : "cloud_inverted.svg"
            checked: (settingPage.state == "CloudDrive")
            onClicked: {
                settingPage.state = "CloudDrive";
            }
        }
        ToolBarButton {
            id: advancedTab
            buttonIconSource: (!inverted) ? "toolbar_extension.svg" : "toolbar_extension_inverted.svg"
            checked: (settingPage.state == "Developer")
            onClicked: {
                settingPage.state = "Developer";
            }
        }
    }

    TitlePanel {
        id: titlePanel
        text: qsTr("Settings") + " - " + getTitle(settingPage.state) + appInfo.emptyStr
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
        id: cloudPrintSettingModel
        ListElement {
            name: "showCloudPrintJobs"
            options: ""
            type: "button"
            group: "CloudPrint"
        }
        ListElement {
            name: "printFromURL"
            options: ""
            type: "button"
            group: "CloudPrint"
        }
        ListElement {
            name: "resetCloudPrint"
            options: ""
            type: "button"
            group: "CloudPrint"
        }
    }

    ListModel {
        id: cloudDriveSettingModel
        ListElement {
            name: "showCloudDriveJobs"
            options: ""
            type: "button"
            group: "CloudDrive"
        }
        ListElement {
            name: "cancelAllCloudDriveJobs"
            options: ""
            type: "button"
            group: "CloudDrive"
        }
        ListElement {
            name: "syncAllConnectedItems"
            options: ""
            type: "button"
            group: "CloudDrive"
        }
        ListElement {
            name: "showCloudDriveAccounts"
            options: ""
            type: "button"
            group: "CloudDrive"
        }
        ListElement {
            name: "CloudFolderPage.thumbnail.enabled"
            options: ""
            type: "switch"
            group: "CloudDrive"
        }
        ListElement {
            name: "drivepage.clouddrive.enabled"
            options: ""
            type: "switch"
            group: "CloudDrive"
        }
        ListElement {
            name: "CloudDriveModel.metadata.root.connection.enabled"
            options: ""
            type: "switch"
            group: "CloudDrive"
        }
        ListElement {
            name: "CloudDriveModel.syncDirtyItems.enabled"
            options: ""
            type: "switch"
            group: "CloudDrive"
        }
        ListElement {
            name: "CloudDriveModel.schedulerTimeoutFilter.syncOnlyOnCharging.enabled"
            options: ""
            type: "switch"
            group: "CloudDrive"
        }
        ListElement {
            name: "CloudDriveModel.schedulerTimeoutFilter.syncOnlyOnBatteryOk.enabled"
            options: ""
            type: "switch"
            group: "CloudDrive"
        }
        ListElement {
            name: "CloudDriveModel.schedulerTimeoutFilter.syncOnlyOnWlan.enabled"
            options: ""
            type: "switch"
            group: "CloudDrive"
        }
        ListElement {
            name: "CloudDriveModel.proceedNextJob.syncOnlyOnCharging.enabled"
            options: ""
            type: "switch"
            group: "CloudDrive"
        }
        ListElement {
            name: "CloudDriveModel.proceedNextJob.syncOnlyOnBatteryOk.enabled"
            options: ""
            type: "switch"
            group: "CloudDrive"
        }
        ListElement {
            name: "CloudDriveModel.proceedNextJob.syncOnlyOnWlan.enabled"
            options: ""
            type: "switch"
            group: "CloudDrive"
        }
    }

    ListModel {
        id: personalizationSettingModel
        ListElement {
            name: "locale"
            options: ""
            type: "button"
            group: "Personalization"
        }
        ListElement {
            name: "Theme.inverted"
            options: ""
            type: "switch"
            group: "Personalization"
        }
        ListElement {
            name: "popup.timer.interval"
            options: "2,10,1,5" // min,max,step,default
            type: "slider"
            group: "Personalization"
        }
        ListElement {
            name: "listItem.compact.enabled"
            options: ""
            type: "switch"
            group: "Personalization"
        }
        ListElement {
            name: "GridView.compact.enabled"
            options: ""
            type: "switch"
            group: "Personalization"
        }
        ListElement {
            name: "thumbnail.enabled"
            options: ""
            type: "switch"
            group: "Personalization"
        }
        ListElement {
            name: "automatically.bluetooth.on"
            options: ""
            type: "switch"
            group: "Personalization"
        }
        ListElement {
            name: "keep.bluetooth.off"
            options: ""
            type: "switch"
            group: "Personalization"
        }
        ListElement {
            name: "statusbar.enabled"
            options: ""
            type: "switch"
            group: "Personalization"
        }
    }

    ListModel {
        id: developerSettingModel
        ListElement {
            name: "resetCache"
            options: ""
            type: "button"
            group: "FolderPie"
        }
        ListElement {
            name: "Monitoring.enabled"
            options: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "Logging.enabled"
            options: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "drivepage.systemdrive.enabled"
            options: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "drivepage.privatedrive.enabled"
            options: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "FolderSizeModelThread.getDirContent.showHiddenSystem.enabled"
            options: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "FolderSizeModelThread.forceDeleteReadOnly.enabled"
            options: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "FtpClient.getItemListJson.showHiddenSystem.enabled"
            options: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "WebDavClient.createPropertyJson.showHiddenSystem.enabled"
            options: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "drivepage.trash.enabled"
            options: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "FolderSizeItemListModel.deleteFile.use.trash.enabled"
            options: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "AuthPage.openAuthorizationWithBrowser"
            options: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "CustomQNetworkAccessManager.ignoreSSLCertificateErrors"
            options: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "WebDavClient.ignoreSSLSelfSignedCertificateErrors"
            options: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "LocalFileImageProvider.cache.enabled"
            options: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "local.image.preview.size"
            options: "360,640,140,640"
            type: "slider"
            group: "Developer"
        }
        ListElement {
            name: "LocalFileImageProvider.CacheImageWorker.enabled"
            options: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "RemoteImageProvider.CacheImageWorker.enabled"
            options: ""
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "GCDClient.dirtyBeforeSync.enabled"
            options: ""
            type: "switch"
            group: "CloudDrive"
        }
        ListElement {
            name: "GCDClient.patchFile.setModifiedDate.enabled"
            options: ""
            type: "switch"
            group: "CloudDrive"
        }
        ListElement {
            name: "BoxClient.maxUploadFileSize"
            options: "10,30,10,20"
            type: "slider"
            group: "CloudDrive"
        }
    }

    GridView {
        id: settingList
        width: parent.width
        height: parent.height - titlePanel.height
        anchors.top: titlePanel.bottom
        delegate: settingListItemDelegate
        cacheBuffer: 1200
        cellWidth: parent.width
        cellHeight: 52
        flow: GridView.LeftToRight
        snapMode: GridView.SnapToRow
        pressDelay: 100
    }

    ScrollDecorator {
        id: scrollbar
        flickableItem: settingList
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
        else if (name == "CloudDriveModel.metadata.root.connection.enabled") return qsTr("Sync to cloud storage root") + appInfo.emptyStr;
        else if (name == "CloudDriveModel.syncDirtyItems.enabled") return qsTr("Sync dirty items automatically") + appInfo.emptyStr;
        else if (name == "GCDClient.dirtyBeforeSync.enabled") return qsTr("Dirty before sync (GoogleDrive)") + appInfo.emptyStr;
        else if (name == "GCDClient.patchFile.setModifiedDate.enabled") return qsTr("Set modified date to uploaded file  (GoogleDrive)") + appInfo.emptyStr;
        else if (name == "CloudDriveModel.schedulerTimeoutFilter.syncOnlyOnCharging.enabled") return qsTr("Schedule sync only on charging") + appInfo.emptyStr;
        else if (name == "CloudDriveModel.schedulerTimeoutFilter.syncOnlyOnBatteryOk.enabled") return qsTr("Schedule sync only if battery is OK (>40%)") + appInfo.emptyStr;
        else if (name == "CloudDriveModel.schedulerTimeoutFilter.syncOnlyOnWlan.enabled") return qsTr("Schedule sync only on WiFi") + appInfo.emptyStr;
        else if (name == "CloudDriveModel.proceedNextJob.syncOnlyOnCharging.enabled") return qsTr("Sync only on charging") + appInfo.emptyStr;
        else if (name == "CloudDriveModel.proceedNextJob.syncOnlyOnBatteryOk.enabled") return qsTr("Sync only if battery is OK (>40%)") + appInfo.emptyStr;
        else if (name == "CloudDriveModel.proceedNextJob.syncOnlyOnWlan.enabled") return qsTr("Sync only on WiFi") + appInfo.emptyStr;
        else if (name == "FolderPie.enabled") return qsTr("FolderPie feature") + appInfo.emptyStr;
        else if (name == "resetCache") return qsTr("Reset current folder cache") + appInfo.emptyStr;
        else if (name == "Theme.inverted") return qsTr("Theme") + appInfo.emptyStr;
        else if (name == "popup.timer.interval") return qsTr("Popup interval") + appInfo.emptyStr;
        else if (name == "listItem.compact.enabled") return qsTr("Compact list item") + appInfo.emptyStr;
        else if (name == "GridView.compact.enabled") return qsTr("Compact grid item") + appInfo.emptyStr;
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
        else if (name == "AuthPage.openAuthorizationWithBrowser") return qsTr("Open authorization in browser") + appInfo.emptyStr;
        else if (name == "CustomQNetworkAccessManager.ignoreSSLCertificateErrors") return qsTr("Ignore all SSL certificate errors") + appInfo.emptyStr;
        else if (name == "WebDavClient.ignoreSSLSelfSignedCertificateErrors") return qsTr("Ignore WebDAV SSL self-signed certificate errors") + appInfo.emptyStr;
        else if (name == "LocalFileImageProvider.cache.enabled") return qsTr("Cache local images") + appInfo.emptyStr;
        else if (name == "local.image.preview.size") return qsTr("Local image preview size") + appInfo.emptyStr;
        else if (name == "LocalFileImageProvider.CacheImageWorker.enabled") return qsTr("Queue local image caching") + appInfo.emptyStr;
        else if (name == "RemoteImageProvider.CacheImageWorker.enabled") return qsTr("Queue cloud image caching") + appInfo.emptyStr;
        else if (name == "FolderSizeModelThread.getDirContent.showHiddenSystem.enabled") return qsTr("Show hidden files on local drive") + appInfo.emptyStr;
        else if (name == "FolderSizeModelThread.forceDeleteReadOnly.enabled") return qsTr("Delete read-only files") + appInfo.emptyStr;
        else if (name == "FtpClient.getItemListJson.showHiddenSystem.enabled") return qsTr("Show hidden files on FTP") + appInfo.emptyStr;
        else if (name == "WebDavClient.createPropertyJson.showHiddenSystem.enabled") return qsTr("Show hidden files on WebDAV") + appInfo.emptyStr;
        else if (name == "FolderSizeItemListModel.deleteFile.use.trash.enabled") return qsTr("Delete by moving to trash") + appInfo.emptyStr;
        else if (name == "BoxClient.maxUploadFileSize") return qsTr("BOX max upload file size (MB)") + appInfo.emptyStr;
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
        } else if (name == "GridView.compact.enabled") {
            fsModel.refreshItems();
            cloudDriveModel.refreshItems();
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
        } else if (name == "FtpClient.getItemListJson.showHiddenSystem.enabled") {
            var p = window.findPage("cloudFolderPage");
            if (p) p.refreshSlot("settingPage buttonClickedHandler FtpClient.getItemListJson.showHiddenSystem.enabled");
        } else if (name == "WebDavClient.createPropertyJson.showHiddenSystem.enabled") {
            var p = window.findPage("cloudFolderPage");
            if (p) p.refreshSlot("settingPage buttonClickedHandler WebDavClient.createPropertyJson.showHiddenSystem.enabled");
        }
    }

    function getOptionValueText(name, value) {
        if (name == "popup.timer.interval") return qsTr("%n sec.", "", value) + appInfo.emptyStr;
        else return value;
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
                property variant options
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
                property: "options"
                value: options
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
//                console.debug("settingListItemDelegate onCompleted " + name + " " + type + " " + options + " " + group);
                if (type == "section") {
                    itemLoader.sourceComponent = settingListSectionDelegate;
                } else if (type == "button") {
                    itemLoader.sourceComponent = buttonItemDelegate;
                } else if (type == "switch") {
                    itemLoader.sourceComponent = switchItemDelegate;
                } else if (type == "slider") {
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
            text: getTitle(name)
            platformInverted: inverted
            onClicked: {
                buttonClickedHandler(name);
            }
        }
    }

    Component {
        id: switchItemDelegate
        Row {
            width: parent.width - 20
            anchors.centerIn: parent
            Text {
                id: settingKey
                color: (!inverted) ? "white" : "black"
                font.pointSize: 6
                width: parent.width - settingValue.width
                height: parent.height
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                elide: Text.ElideRight
                maximumLineCount: 2
                text: getTitle(name)
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
            width: parent.width - 20
            anchors.centerIn: parent
            Text {
                id: sliderLabel
                color: (!inverted) ? "white" : "black"
                font.pointSize: 6
                width: parent.width - sliderValue.width
                height: parent.height
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                elide: Text.ElideRight
                maximumLineCount: 2
                text: getTitle(name)
            }
            Slider {
                id: sliderValue
                width: parent.width * 0.5
                anchors.verticalCenter: parent.verticalCenter
                valueIndicatorText: getOptionValueText(name, value)
                valueIndicatorVisible: true

                onPressedChanged: {
                    console.debug("settingPage sliderItemDelegate sliderValue onPressedChanged " + pressed + " value " + value);
                    if (!pressed) {
                        appInfo.setSettingValue(name, value);
                    }
                }

                Component.onCompleted: {
                    // options: min,max,step,default
                    var optionValues = options.split(',');
                    minimumValue = optionValues[0];
                    maximumValue = optionValues[1];
                    stepSize = optionValues[2];
                    value = appInfo.getSettingValue(name, optionValues[3]);
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

    SelectionDialog {
        id: languageSelector
        titleText: appInfo.emptyStr+qsTr("Languages")
        model: languageModel
        delegate: selectionItemDelegate // For symbian only.
        onStatusChanged: {
            if (status == DialogStatus.Opening) {
                selectedIndex = languageModel.getIndexByLocale( (appInfo.getLocale() !== "") ? appInfo.getLocale() : appInfo.getSystemLocale() );
                if (selectedIndex != -1) {
                    var listView = content[0].children[0].children[0]; // For symbian only.
                    listView.positionViewAtIndex(selectedIndex, ListView.Contain);
                }
            }
        }

        onAccepted: {
            var index = selectedIndex;
            var selectedLocale = model.get(index).locale;
            console.debug("languageSelector onAccepted locale " + selectedLocale);
            appInfo.setLocale(selectedLocale);
        }
    }

    ListModel {
        id: languageModel
        ListElement { locale: ""; name: "System default" }
        ListElement { locale: "de"; name: "Deutsch" }
        ListElement { locale: "en"; name: "English" }
        ListElement { locale: "it"; name: "Italiano" }
        ListElement { locale: "ru"; name: "Русский" }
        ListElement { locale: "zh"; name: "中文(中国大陆)" }

        function getIndexByLocale(locale) {
            // Find exact match language_country.
            for (var i=0; i<languageModel.count; i++) {
                var item = languageModel.get(i);
                if (locale === item.locale) {
                    return i;
                }
            }

            // Find language only match.
            for (i=0; i<languageModel.count; i++) {
                item = languageModel.get(i);
                if (locale.indexOf(item.locale) === 0) {
                    return i;
                }
            }

            return -1;
        }

        function getLanguage(locale, defaultLocale) {
            var i = getIndexByLocale(locale)
            if (i > -1) {
                return languageModel.get(i).name;
            } else {
                i = getIndexByLocale(defaultLocale);
                if (i > -1) {
                    return languageModel.get(i).name;
                }
            }

            return "Locale not found";
        }
    }

    Component { // For symbian only.
        id: selectionItemDelegate

        MenuItem {
            platformInverted: root.platformInverted
            text: name
            onClicked: {
                selectedIndex = index;
                root.accept();
            }

            Image {
                id: checkIcon
                visible: (index == root.selectedIndex)
                source: "ok.svg"
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
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
