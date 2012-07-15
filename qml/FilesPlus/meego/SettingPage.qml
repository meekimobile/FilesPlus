import QtQuick 1.1
import com.nokia.meego 1.0
import AppInfo 1.0

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
            settingModel.set(i, { title: qsTr("Cancel queued jobs") + " (" + jobQueueCount + ")" });
        }
    }

    function updateCloudDriveItemCount(cloudDriveItemCount) {
        var i = getIndexByName("syncAllConnectedItems");
        if (i > -1) {
            settingModel.set(i, { title: qsTr("Sync all connected items") + " (" + cloudDriveItemCount + ")" });
        }
    }

    tools: ToolBarLayout {
        ToolIcon {
            id: back
            iconId: "toolbar-back"
            onClicked: {
                pageStack.pop();
            }
        }
    }

    AppInfo {
        id: appInfo
        domain: "MeekiMobile"
        app: "FilesPlus"
    }

    TitlePanel {
        id: titlePanel
        text: qsTr("Settings")
        z: 1
    }

    ConfirmDialog {
        id: quitConfirmation
        titleText: qsTr("Logging (Debug)")
        contentText: qsTr("Changing logging switch requires restart.\nFilesPlus is exiting now.\n\nPlease confirm.")
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
//        ListElement {
//            name: "Monitoring.enabled"
//            title: ""
//            type: "switch"
//            group: "Developer"
//        }
        ListElement {
            name: "Logging.enabled"
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
    }

    function getTitle(name) {
        if (name == "showCloudPrintJobs") return qsTr("Show cloud print jobs");
        else if (name == "resetCloudPrint") return qsTr("Reset cloud print");
        else if (name == "showCloudDriveJobs") return qsTr("Show cloud drive jobs");
        else if (name == "cancelAllCloudDriveJobs") return qsTr("Cancel queued jobs");
        else if (name == "syncAllConnectedItems") return qsTr("Sync all connected items");
        else if (name == "showCloudDriveAccounts") return qsTr("Show accounts");
        else if (name == "FolderPie.enabled") return qsTr("FolderPie feature");
        else if (name == "resetCache") return qsTr("Reset current folder cache");
        else if (name == "Logging.enabled") return qsTr("Logging (Debug)");
        else if (name == "Monitoring.enabled") return qsTr("Monitoring (RAM,CPU)");
        else return qsTr(name);
    }

    function buttonClickedHandler(name) {
        var p = pageStack.find(function (page) { return page.name == "folderPage"; });
        if (p) {
            if (name == "showCloudPrintJobs") {
                p.showCloudPrintJobsSlot();
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
            } else if (name == "Logging.enabled") {
                quitConfirmation.open();
            } else if (name == "Monitoring.enabled") {
                if (appInfo.isMonitoring()) {
                    messageDialog.titleText = qsTr("Monitoring");
                    messageDialog.message = qsTr("Monitoring is enabled. Log file is ") + appInfo.getMonitoringFilePath();
                    messageDialog.open();
                }
            }
        }
    }

    Component {
        id: settingListItemDelegate

        Rectangle {
            width: settingList.width
            height: 80
            color: "transparent"
            Row {
                visible: (type == "switch")
                width: parent.width - 40
                anchors.centerIn: parent
                Text {
                    id: settingKey
                    color: "white"
                    font.pointSize: 18
                    width: parent.width - settingValue.width
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    text: (title=="") ? getTitle(name) : title
                }
                Switch {
                    id: settingValue
                    anchors.verticalCenter: parent.verticalCenter
                    checked: appInfo.getSettingValue(name, false)
                    onCheckedChanged: {
//                        console.debug("settingListItemDelegate settingValue checked " + checked + " appInfo.getSettingValue " + name + " " + appInfo.getSettingValue(name, false));
                        if (appInfo.setSettingValue(name, checked)) {
                            console.debug("settingListItemDelegate settingValue " + name + " checked " + checked + " is changed.");
                            // Handle only if it's enabled.
                            buttonClickedHandler(name);
                        }
                    }
                }
            }
            Button {
                id: settingButton
                visible: (type == "button")
                width: parent.width - 20
                anchors.centerIn: parent
                text: (title=="") ? getTitle(name) : title
                onClicked: {
                    buttonClickedHandler(name);
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
                    text: section
                    color: "grey"
                    font.pointSize: 14
                }
                Rectangle {
                    width: parent.width - sectionText.width - parent.spacing
                    height: 1
                    anchors.verticalCenter: sectionText.verticalCenter
                    color: "grey"
                }
            }
        }
    }
}
