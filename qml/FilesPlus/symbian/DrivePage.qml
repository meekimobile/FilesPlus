import QtQuick 1.1
import com.nokia.symbian 1.1
import QtMobility.systeminfo 1.2
import SystemInfoHelper 1.0
import CloudDriveModel 1.0
import "Utility.js" as Utility

Page {
    id: drivePage

    property string name: "drivePage"
    property variant cloudDriveModel

    tools: toolBarLayout

    function quitSlot() {
        var p = pageStack.find(function (page) { return page.name == "folderPage"; });
        if (p) p.quitSlot();
    }

    function updateAccountInfoSlot(type, uid, name, email, shared, normal, quota) {
        console.debug("drivePage updateAccountInfoSlot uid " + uid + " type " + type + " shared " + shared + " normal " + normal + " quota " + quota);
        for (var i=0; i<driveGrid.model.count; i++) {
            var item = driveGrid.model.get(i);
            console.debug("drivePage updateAccountInfoSlot item i " + i + " uid " + item.uid + " driveType " + item.driveType + " cloudDriveType " + item.cloudDriveType);
            if (item.uid == uid && item.driveType == 7 && item.cloudDriveType == type) {
                console.debug("drivePage updateAccountInfoSlot found item i " + i + " uid " + item.uid + " driveType " + item.driveType + " cloudDriveType " + item.cloudDriveType);
                driveGrid.model.set(i, { name: name, email: email, availableSpace: (quota - shared - normal), totalSpace: quota });
            }
        }
    }

    ToolBarLayout {
        id: toolBarLayout

        ToolButton {
            id: backButton
            iconSource: "toolbar-back"
            platformInverted: window.platformInverted
            flat: true
            onClicked: {
                quitSlot();
            }
        }

        ToolButton {
            id: menuButton
            iconSource: "toolbar-menu"
            platformInverted: window.platformInverted
            flat: true
            onClicked: {
                mainMenu.open();
            }
        }
    }

    MainMenu {
        id: mainMenu
        // TODO refactor to use id.
        enabledMenus: [appInfo.emptyStr+qsTr("About"), appInfo.emptyStr+qsTr("More Apps"), appInfo.emptyStr+qsTr("Settings"), appInfo.emptyStr+qsTr("Exit")]

        onQuit: {
            quitSlot();
        }
    }

    StorageInfo {
        id: storageInfo
    }

    SystemInfoHelper {
        id: systemInfoHelper
    }

    MessageDialog {
        id: messageDialog
    }

    function parseLocalStorage(model) {
        var simulatedDriveNames = systemInfoHelper.getDriveList();
        var drives = storageInfo.logicalDrives;

        for (var i=0; i<drives.length; i++)
        {
            // Workaround for QtSimulator
            var driveName = drives[i];
            if (i < simulatedDriveNames.length) {
                driveName = simulatedDriveNames[i];
            }

            var driveType = systemInfoHelper.getDriveTypeInt(drives[i]);
            // 0=No Drive, 6=Ram Drive
            if (driveType != 0 && driveType != 6) {
                model.append({
                                 logicalDrive: driveName,
                                 availableSpace: storageInfo.availableDiskSpace(drives[i]),
                                 totalSpace: storageInfo.totalDiskSpace(drives[i]),
                                 driveType: driveType,
                                 email: "",
                                 uid: "",
                                 name: "",
                                 cloudDriveType: -1,
                                 iconSource: ""
                });
            }
        }
    }

    function parseCloudStorage(type, model) {
        // TODO Check if authorized before parsing.
        if (!cloudDriveModel.isAuthorized(type)) return;

        // Get uid list from DropboxClient.
        var dbUidList = cloudDriveModel.getStoredUidList(type);

        for (var i=0; i<dbUidList.length; i++)
        {
            var json = JSON.parse(dbUidList[i]);
            console.debug("parseCloudStorage type " + type + " i " + i + " uid " + json.uid + " email " + json.email);
            model.append({
                             logicalDrive: json.email,
                             availableSpace: 0,
                             totalSpace: 0,
                             driveType: 7, // Cloud Drive in driveGrid.
                             email: json.email,
                             uid: json.uid,
                             name: "",
                             cloudDriveType: type,
                             iconSource: cloudDriveModel.getCloudIcon(type)
            });

            // Request accountInfo.
            cloudDriveModel.accountInfo(type, json.uid);
        }
    }

    DriveGrid {
        id: driveGrid
        width: parent.width
        height: parent.height - titlePanel.height
        anchors.top: titlePanel.bottom
        model: ListModel {
            id: driveGridModel
        }

        onDriveSelected: {
//            console.debug("driveSelection onDriveSelected " + driveName + " index " + index);
            if (index > -1) {
                var driveType = driveGridModel.get(index).driveType;
                if (driveType == 7) {
                    // TODO
                    var cloudUid = driveGridModel.get(index).uid;
                    console.debug("driveSelection onDriveSelected " + driveName + " index " + index + " cloudUid " + cloudUid);
                } else {
                    var p = pageStack.find(function(page) { return (page.name == "folderPage"); });
                    if (p) p.currentDir = driveName;
                    pageStack.pop(drivePage, true);
                }
            }
        }
    }

    TitlePanel {
        id: titlePanel
        text: appInfo.emptyStr+qsTr("Drives")
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            driveGrid.currentIndex = -1;

            // TODO Parse all storages.
            driveGridModel.clear();
            parseLocalStorage(driveGridModel);
            if (appInfo.getSettingBoolValue("drivepage.clouddrive.enabled", false)) {
                parseCloudStorage(CloudDriveModel.Dropbox, driveGridModel);
            }

            // Stop startup logging.
            appInfo.stopLogging();
        }
    }

    Component.onCompleted: {
        console.debug(Utility.nowText() + " drivePage onCompleted");
        window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("DrivePage"), 0.2);
    }
}
