import QtQuick 1.1
import com.nokia.symbian 1.1
import AppInfo 1.0

Page {
    id: settingPage

    property string name: "settingPage"

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
    }

    ListModel {
        id: settingModel
        ListElement {
            name: "showCloudPrintJobs"
            title: "Show CloudPrint jobs"
            type: "button"
            group: "CloudPrint"
        }
        ListElement {
            name: "resetCloudPrint"
            title: "Reset CloudPrint"
            type: "button"
            group: "CloudPrint"
        }
        ListElement {
            name: "syncAllConnectedItems"
            title: "Sync all connected items"
            type: "button"
            group: "CloudDrive"
        }
        ListElement {
            name: "registerDropboxUser"
            title: "Register new Dropbox account"
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
            title: "Monitoring"
            type: "switch"
            group: "Developer"
        }
        ListElement {
            name: "Logging.enabled"
            title: "Logging"
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
        // Pop current page.
        pageStack.pop();

        var p = pageStack.find(function (page) { return page.name == "folderPage"; });
        if (p) {
            if (name == "showCloudPrintJobs") {
                p.showCloudPrintJobsSlot();
            } else if (name == "resetCloudPrint") {
                p.resetCloudPrintSlot();
            } else if (name == "syncAllConnectedItems") {
                p.syncAllConnectedItemsSlot();
            } else if (name == "registerDropboxUser") {
                p.registerDropboxUserSlot();
            } else if (name == "resetCache") {
                p.resetCacheSlot();
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
                anchors.fill: parent
                anchors.margins: 5
                Text {
                    id: settingKey
                    color: "white"
                    font.pointSize: 7
                    width: parent.width - settingValue.width
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    anchors.leftMargin: 5
                    text: title
                }
                Switch {
                    id: settingValue
                    anchors.verticalCenter: parent.verticalCenter
                    checked: appInfo.getSettingValue(name, false);
                    onClicked: {
                        appInfo.setSettingValue(name, checked);
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
