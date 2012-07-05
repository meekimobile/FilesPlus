import QtQuick 1.1
import com.nokia.meego 1.1
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
            settingModel.set(i, { title: "Cancel queued jobs (" + jobQueueCount + ")" });
        }
    }

    function updateCloudDriveItemCount(cloudDriveItemCount) {
        var i = getIndexByName("syncAllConnectedItems");
        if (i > -1) {
            settingModel.set(i, { title: "Sync all connected items (" + cloudDriveItemCount + ")" });
        }
    }

    tools: ToolBarLayout {
        ToolButton {
            id: back
            iconSource: "toolbar-back"
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
        text: "Settings"
        z: 1
    }

    ConfirmDialog {
        id: quitConfirmation
        titleText: "Enable logging"
        contentText: "Turn on logging requires restart.\nFilesPlus is exiting now.\n\nPlease confirm."
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
            title: "Show cloud print jobs"
            type: "button"
            group: "CloudPrint"
        }
        ListElement {
            name: "resetCloudPrint"
            title: "Reset cloud print"
            type: "button"
            group: "CloudPrint"
        }
//        ListElement {
//            name: "showCloudDriveJobs"
//            title: "Show cloud drive jobs"
//            type: "button"
//            group: "CloudDrive"
//        }
        ListElement {
            name: "cancelAllCloudDriveJobs"
            title: "Cancel queued jobs"
            type: "button"
            group: "CloudDrive"
        }
        ListElement {
            name: "syncAllConnectedItems"
            title: "Sync all connected items"
            type: "button"
            group: "CloudDrive"
        }
        ListElement {
            name: "showCloudDriveAccounts"
            title: "Show accounts"
            type: "button"
            group: "CloudDrive"
        }
//        ListElement {
//            name: "FolderPie.enabled"
//            title: "FolderPie feature"
//            type: "switch"
//            group: "FolderPie"
//        }
        ListElement {
            name: "resetCache"
            title: "Reset current folder cache"
            type: "button"
            group: "FolderPie"
        }
        ListElement {
            name: "Monitoring.enabled"
            title: "Monitoring (RAM,CPU)"
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "Logging.enabled"
            title: "Logging (Debug)"
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

    function buttonClickedHandler(name) {
        var p = pageStack.find(function (page) { return page.name == "folderPage"; });
        if (p) {
            if (name == "showCloudPrintJobs") {
                p.showCloudPrintJobsSlot();
            } else if (name == "resetCloudPrint") {
                pageStack.pop();
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
                    messageDialog.titleText = "Monitoring";
                    messageDialog.message = "Monitoring is enabled. Log file is " + appInfo.getMonitoringFilePath();
                    messageDialog.open();
                }
            }
        }
    }

    Component {
        id: settingListItemDelegate

        Rectangle {
            width: settingList.width
            height: 60
            color: "transparent"
            Row {
                visible: (type == "switch")
                width: parent.width - 40
                anchors.centerIn: parent
                Text {
                    id: settingKey
                    color: "white"
                    font.pointSize: 7
                    width: parent.width - settingValue.width
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    text: title
                }
                Switch {
                    id: settingValue
                    anchors.verticalCenter: parent.verticalCenter
                    checked: appInfo.getSettingValue(name, false);
                    onClicked: {
                        appInfo.setSettingValue(name, checked);
                        buttonClickedHandler(name);
                    }
                }
            }
            Button {
                id: settingButton
                visible: (type == "button")
                width: parent.width - 20
                anchors.centerIn: parent
                text: title
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
