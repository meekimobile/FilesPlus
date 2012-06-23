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
            name: "Monitoring.enabled"
            title: "Monitoring"
            type: "switch"
        }
        ListElement {
            name: "Logging.enabled"
            title: "Logging"
            type: "switch"
        }
//        ListElement {
//            name: "FolderPie.enabled"
//            title: "FolderPie feature"
//            type: "switch"
//        }
        ListElement {
            name: "showCloudPrintJobs"
            title: "Show CloudPrint jobs"
            type: "button"
        }
        ListElement {
            name: "resetCloudPrint"
            title: "Reset CloudPrint"
            type: "button"
        }
        ListElement {
            name: "registerDropboxUser"
            title: "Register new Dropbox account"
            type: "button"
        }
        ListElement {
            name: "resetCache"
            title: "Reset current folder cache"
            type: "button"
        }
    }

    ListView {
        id: settingList
        width: parent.width
        height: parent.height - titlePanel.height
        anchors.top: titlePanel.bottom
        model: settingModel
        delegate: settingListItemDelegate
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
                    text: title
                }
                Switch {
                    id: settingValue
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
            Rectangle {
                width: parent.width
                height: 1
                color: "grey"
                anchors.bottom: parent.bottom
            }
        }
    }
}
