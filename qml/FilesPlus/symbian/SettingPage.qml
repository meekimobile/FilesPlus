import QtQuick 1.1
import com.nokia.symbian 1.1
import AppInfo 1.0
import com.nokia.extras 1.0

Page {
    id: settingPage

    property string name: "settingPage"
    property int cloudDriveItemCount: 0

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
            settingModel.set(i, { title: getTitle("cancelAllCloudDriveJobs") + " (" + jobQueueCount + ")" + appInfo.emptyStr });
        }
    }

    function updateCloudDriveItemCount(cloudDriveItemCount) {
        var i = getIndexByName("syncAllConnectedItems");
        if (i > -1) {
            settingModel.set(i, { title: getTitle("syncAllConnectedItems") + " (" + cloudDriveItemCount + ")" + appInfo.emptyStr });
        }
    }

    tools: ToolBarLayout {
        ToolButton {
            id: back
            iconSource: "toolbar-back"
            platformInverted: window.platformInverted
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

    MessageDialog {
        id: messageDialog
    }

    ListModel {
        id: settingModel
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
//        ListElement {
//            name: "showCloudDriveJobs"
//            title: ""
//            type: "button"
//            group: "CloudDrive"
//        }
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
        ListElement {
            name: "sync.after.refresh"
            title: ""
            type: "switch"
            group: "CloudDrive"
        }
//        ListElement {
//            name: "drivepage.clouddrive.enabled"
//            title: ""
//            type: "switch"
//            group: "CloudDrive"
//        }
//        ListElement {
//            name: "FolderPie.enabled"
//            title: ""
//            type: "switch"
//            group: "FolderPie"
//        }
        ListElement {
            name: "resetCache"
            title: ""
            type: "button"
            group: "FolderPie"
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
            name: "thumbnail.enabled"
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
    }

    ListView {
        id: settingList
        width: parent.width
        height: parent.height - titlePanel.height
        anchors.top: titlePanel.bottom
        model: settingModel
        delegate: settingListItemDelegate
        section.property: "group"
        section.criteria: ViewSection.FullString
        section.delegate: settingListSectionDelegate
        cacheBuffer: 1200
    }

    function getTitle(name) {
        if (name == "showCloudPrintJobs") return qsTr("Show cloud print jobs") + appInfo.emptyStr;
        else if (name == "printFromURL") return qsTr("Print from URL") + appInfo.emptyStr;
        else if (name == "resetCloudPrint") return qsTr("Reset cloud print") + appInfo.emptyStr;
        else if (name == "showCloudDriveJobs") return qsTr("Show cloud drive jobs") + appInfo.emptyStr;
        else if (name == "cancelAllCloudDriveJobs") return qsTr("Cancel queued jobs") + appInfo.emptyStr;
        else if (name == "syncAllConnectedItems") return qsTr("Sync all connected items") + appInfo.emptyStr;
        else if (name == "showCloudDriveAccounts") return qsTr("Show accounts") + appInfo.emptyStr;
        else if (name == "sync.after.refresh") return qsTr("Auto-sync after refresh") + appInfo.emptyStr;
//        else if (name == "drivepage.clouddrive.enabled") return qsTr("Show cloud storage on drive page") + appInfo.emptyStr;
        else if (name == "FolderPie.enabled") return qsTr("FolderPie feature") + appInfo.emptyStr;
        else if (name == "resetCache") return qsTr("Reset current folder cache") + appInfo.emptyStr;
        else if (name == "Theme.inverted") return qsTr("Theme") + appInfo.emptyStr;
        else if (name == "popup.timer.interval") return qsTr("Popup interval") + appInfo.emptyStr;
        else if (name == "locale") return languageModel.getLanguage(appInfo.getLocale(), "en") + appInfo.emptyStr;
        else if (name == "thumbnail.enabled") return qsTr("Show thumbnail") + appInfo.emptyStr;
        else if (name == "keep.bluetooth.off") return qsTr("Keep bluetooth off") + appInfo.emptyStr;
        else if (name == "Logging.enabled") return qsTr("Logging (Debug)") + appInfo.emptyStr;
        else if (name == "dropbox.fullaccess.enabled") return qsTr("Dropbox full access") + appInfo.emptyStr;
        else if (name == "Monitoring.enabled") return qsTr("Monitoring (RAM,CPU)") + appInfo.emptyStr;
        else if (name == "Personalization") return qsTr("Personalization") + appInfo.emptyStr;
        else if (name == "Developer") return qsTr("Developer") + appInfo.emptyStr;
        else return qsTr(name) + appInfo.emptyStr;
    }

    function buttonClickedHandler(name) {
        var p = pageStack.find(function (page) { return page.name == "folderPage"; });
        if (p) {
            if (name == "showCloudPrintJobs") {
                p.showCloudPrintJobsSlot();
            } else if (name == "printFromURL") {
                p.showWebViewPageSlot();
            } else if (name == "resetCloudPrint") {
                p.resetCloudPrintSlot();
            } else if (name == "showCloudDriveJobs") {
                p.showCloudDriveJobsSlot();
            } else if (name == "cancelAllCloudDriveJobs") {
                p.cancelAllCloudDriveJobsSlot();
            } else if (name == "syncAllConnectedItems") {
                p.syncAllConnectedItemsSlot();
            } else if (name == "showCloudDriveAccounts") {
                p.showCloudDriveAccountsSlot();
            } else if (name == "resetCache") {
                pageStack.pop();
                p.resetCacheSlot();
            } else if (name == "Theme.inverted") {
                window.platformInverted = !window.platformInverted;
            } else if (name == "locale") {
                languageSelector.open();
            } else if (name == "Logging.enabled") {
                quitConfirmation.open();
            } else if (name == "dropbox.fullaccess.enabled") {
                pageStack.pop();
                p.showDropboxFullAccessConfirmationSlot();
            } else if (name == "Monitoring.enabled") {
                if (appInfo.isMonitoring()) {
                    messageDialog.titleText = appInfo.emptyStr+qsTr("Monitoring");
                    messageDialog.message = appInfo.emptyStr+qsTr("Monitoring is enabled. Log file is ") + appInfo.getMonitoringFilePath();
                    messageDialog.open();
                }
            }
        }
    }

    Component {
        id: settingListItemDelegate

        Rectangle {
            width: settingList.width
            height: 70
            color: "transparent"
            Row {
                visible: (type == "switch")
                width: parent.width - 40
                anchors.centerIn: parent
                Text {
                    id: settingKey
                    color: (!window.platformInverted) ? "white" : "black"
                    font.pointSize: 7
                    width: parent.width - settingValue.width
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    text: (title=="") ? getTitle(name) : title
                }
                Switch {
                    id: settingValue
                    anchors.verticalCenter: parent.verticalCenter
                    checked: (type == "switch") && appInfo.getSettingValue(name, false)
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
            Row {
                visible: (type == "popupInterval")
                width: parent.width - 40
                height: parent.height
                anchors.horizontalCenter: parent.horizontalCenter
                Text {
                    id: sliderLabel
                    color: (!window.platformInverted) ? "white" : "black"
                    font.pointSize: 7
                    width: parent.width / 3
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    text: (title=="") ? getTitle(name) : title
                }
                Slider {
                    id: sliderValue
                    width: parent.width - sliderLabel.width
                    height: parent.height
                    platformInverted: window.platformInverted
                    minimumValue: 2
                    maximumValue: 10
                    stepSize: 1
                    valueIndicatorText: appInfo.emptyStr+qsTr("%n sec.", "", value)
                    valueIndicatorVisible: true

                    onPressedChanged: {
                        console.debug("popupInterval pressed " + pressed + " value " + value);
                        if (!pressed) {
                            appInfo.setSettingValue(name, value);
                        }
                    }

                    Component.onCompleted: {
                        if (type == "popupInterval") {
                            var v = appInfo.getSettingValue(name, 5);
                            console.debug("settingPage popupInterval getSettingValue " + v);
                            value = v;
                        }
                    }
                }
            }
            Button {
                id: settingButton
                visible: (type == "button")
                width: parent.width - 20
                anchors.centerIn: parent
                platformInverted: window.platformInverted
                text: (title=="") ? getTitle(name) : title
                onClicked: {
                    buttonClickedHandler(name);
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

            ButtonRow {
                id: localeSelector
                visible: (type == "locale")
                width: parent.width - 20
                anchors.centerIn: parent
                checkedButton: localeCN
                Button {
                    id: localeEN
                    property string locale: "en"
                    text: appInfo.emptyStr+qsTr("English")
                    checkable: true
                    platformInverted: window.platformInverted
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
                    platformInverted: window.platformInverted
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
                    platformInverted: window.platformInverted
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
    }

    Component {
        id: settingListSectionDelegate

        Rectangle {
            width: settingList.width
            height: 24
            color: "transparent"
            Row {
                width: parent.width
                spacing: 5
                Text {
                    id: sectionText
                    text: getTitle(section)
                    color: (!window.platformInverted) ? "grey" : "black"
                }
                Rectangle {
                    width: parent.width - sectionText.width - parent.spacing
                    height: 1
                    anchors.verticalCenter: sectionText.verticalCenter
                    color: (!window.platformInverted) ? "grey" : "black"
                }
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
                columns[0].selectedIndex = languageModel.getIndexByLocale(appInfo.getLocale());
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
}
