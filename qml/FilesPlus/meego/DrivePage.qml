import QtQuick 1.1
import com.nokia.meego 1.0
import QtMobility.systeminfo 1.2
import SystemInfoHelper 1.0
import CloudDriveModel 1.0
import "Utility.js" as Utility

Page {
    id: drivePage

    property string name: "drivePage"

    tools: toolBarLayout

    function updateAccountInfoSlot(type, uid, name, email, shared, normal, quota) {
//        console.debug("drivePage updateAccountInfoSlot uid " + uid + " type " + type + " shared " + shared + " normal " + normal + " quota " + quota);
        for (var i=0; i<driveGrid.model.count; i++) {
            var item = driveGrid.model.get(i);
//            console.debug("drivePage updateAccountInfoSlot item i " + i + " uid " + item.uid + " driveType " + item.driveType + " cloudDriveType " + item.cloudDriveType);
            if (item.uid == uid && item.driveType == 7 && item.cloudDriveType == type) {
                console.debug("drivePage updateAccountInfoSlot found item i " + i + " uid " + item.uid + " driveType " + item.driveType + " cloudDriveType " + item.cloudDriveType);
                driveGrid.model.set(i, { name: name, email: email, availableSpace: (quota - shared - normal), totalSpace: quota });
            }
        }
    }

    ToolBarLayout {
        id: toolBarLayout

        ToolIcon {
            id: backButton
            iconId: "toolbar-back"
            onClicked: {
                window.quitSlot();
            }
        }

        ToolIcon {
            id: refreshButton
            iconId: "toolbar-refresh"
            onClicked: {
                // TODO Parse all storages.
                driveGridModel.clear();
                parseLocalStorage(driveGridModel);
                if (appInfo.getSettingBoolValue("drivepage.clouddrive.enabled", false)) {
                    parseCloudStorage(driveGridModel);
                }
            }
        }

        ToolIcon {
            id: menuButton
            iconId: "toolbar-view-menu"
            onClicked: {
                driveMenu.open();
            }
        }
    }

    DriveMenu {
        id: driveMenu

        onQuit: {
            window.quitSlot();
        }
    }

    StorageInfo {
        id: storageInfo
    }

    SystemInfoHelper {
        id: systemInfoHelper
    }

    function parseLocalStorage(model) {
        var simulatedDriveNames = systemInfoHelper.getDriveList();
        var drives = storageInfo.logicalDrives;

        for (var i=0; i<drives.length; i++)
        {
            // Hide root.
//            if (drives[i] == "/") continue;

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

    function parseCloudStorage(model) {
        // TODO Check if authorized before parsing.
        if (!cloudDriveModel.isAuthorized()) return;

        // Get uid list from DropboxClient.
        var dbUidList = cloudDriveModel.getStoredUidList();

        for (var i=0; i<dbUidList.length; i++)
        {
            var json = JSON.parse(dbUidList[i]);
            var cloudType = cloudDriveModel.getClientType(json.type);
            console.debug("parseCloudStorage type " + json.type + " i " + i + " uid " + json.uid + " email " + json.email);
            model.append({
                             logicalDrive: json.email,
                             availableSpace: 0,
                             totalSpace: 0,
                             driveType: 7, // Cloud Drive in driveGrid.
                             email: json.email,
                             uid: json.uid,
                             name: "",
                             cloudDriveType: cloudType,
                             iconSource: cloudDriveModel.getCloudIcon(cloudType)
            });

            // Request quota.
            switch (cloudType) {
            case CloudDriveModel.Dropbox:
                cloudDriveModel.accountInfo(cloudType, json.uid);
                break;
            case CloudDriveModel.SkyDrive:
                cloudDriveModel.quota(cloudType, json.uid);
                break;
            }
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
                if (driveType == 7) { // Cloud drive is selected.
                    var cloudUid = driveGridModel.get(index).uid;
                    var cloudDriveType = driveGridModel.get(index).cloudDriveType;
                    console.debug("driveSelection onDriveSelected " + driveName + " index " + index + " cloudUid " + cloudUid);
                    pageStack.push(Qt.resolvedUrl("CloudFolderPage.qml"), {
                                       remotePath: "",
                                       remoteParentPath: "",
                                       caller: "driveGrid onDriveSelected",
                                       operation: CloudDriveModel.Browse,
                                       localPath: "",
                                       selectedCloudType: cloudDriveType,
                                       selectedUid: cloudUid,
                                       selectedModelIndex: -1
                                   });
                } else { // Local drive is selected.
                    fsModel.currentDir = driveName;
                    pageStack.push(Qt.resolvedUrl("FolderPage.qml"));

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
                parseCloudStorage(driveGridModel);
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
