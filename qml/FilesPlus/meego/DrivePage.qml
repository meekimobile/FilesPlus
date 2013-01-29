import QtQuick 1.1
import com.nokia.meego 1.0
import QtMobility.systeminfo 1.2
import SystemInfoHelper 1.0
import CloudDriveModel 1.0
import "Utility.js" as Utility

Page {
    id: drivePage

    property string name: "drivePage"
    property variant systemDriveNames: ["/","/home/user/.signon/signonfs-mnt"] // Meego only.
    property alias busy: driveGrid.busy

    tools: toolBarLayout

    function updateAccountInfoSlot(type, uid, name, email, shared, normal, quota) {
//        console.debug("drivePage updateAccountInfoSlot uid " + uid + " type " + type + " shared " + shared + " normal " + normal + " quota " + quota);
        for (var i=0; i<driveGrid.model.count; i++) {
            var item = driveGrid.model.get(i);
//            console.debug("drivePage updateAccountInfoSlot item i " + i + " uid " + item.uid + " driveType " + item.driveType + " cloudDriveType " + item.cloudDriveType);
            if (item.uid == uid && item.driveType == 7 && item.cloudDriveType == type) {
                console.debug("drivePage updateAccountInfoSlot found item i " + i + " uid " + item.uid + " driveType " + item.driveType + " cloudDriveType " + item.cloudDriveType);
                driveGrid.model.set(i, { availableSpace: (quota - shared - normal), totalSpace: quota });
                if (email) {
                    driveGrid.model.set(i, { logicalDrive: email, name: name, email: email });
                }
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
                // Force resume.
                cloudDriveModel.resumeNextJob();

                // Parse all storages.
                refreshSlot("drivePage refreshButton onClicked");
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

        onOpenSettings: {
            pageStack.push(Qt.resolvedUrl("SettingPage.qml"));
        }
        onOpenAbout: {
            pageStack.push(Qt.resolvedUrl("AboutPage.qml"));
        }
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
            // Workaround for QtSimulator
            var driveName = drives[i];
            if (i < simulatedDriveNames.length) {
                driveName = simulatedDriveNames[i];
            }

            // Hide system drives.
            if (!appInfo.getSettingBoolValue("drivepage.systemdrive.enabled", false)) {
                if (systemDriveNames.indexOf(driveName) != -1) continue;
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
            console.debug("parseCloudStorage type " + json.type + " " + cloudType + " i " + i + " uid " + json.uid + " email " + json.email);
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
            cloudDriveModel.quota(cloudType, json.uid);
        }
    }

    function refreshSlot(caller) {
        console.debug("drivePage refreshSlot caller " + caller);

        // TODO Parse all storages.
        driveGridModel.clear();
        parseLocalStorage(driveGridModel);
        if (appInfo.getSettingBoolValue("drivepage.clouddrive.enabled", false)) {
            parseCloudStorage(driveGridModel);
        }
    }

    DriveGrid {
        id: driveGrid
        width: parent.width
        height: parent.height - titlePanel.height
        anchors.top: titlePanel.bottom
        model: ListModel {
            id: driveGridModel

            function findIndexByCloudTypeAndUid(cloudType, uid) {
                for (var i = 0; i < driveGridModel.count; i++) {
                    if (driveGridModel.get(i).cloudDriveType == cloudType && driveGridModel.get(i).uid == uid) {
                        return i;
                    }
                }

                return -1;
            }
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

        TextIndicator {
            id: messageLoggerCounter
            text: (messageLoggerModel.newMessageCount > 0) ? (appInfo.emptyStr + qsTr("Message") + " " + messageLoggerModel.newMessageCount) : ""
            height: parent.height - 6
            anchors.right: parent.right
            anchors.rightMargin: 3
            anchors.verticalCenter: parent.verticalCenter

            onClicked: {
                pageStack.push(Qt.resolvedUrl("MessageLoggerPage.qml"));
            }
        }
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            driveGrid.currentIndex = -1;
            busy = false;

            // Stop startup logging.
            appInfo.stopLogging();

            // Show notify if logging is enabled.
            console.debug("drivePage onStatusChanged Logging.enabled " + appInfo.getSettingBoolValue("Logging.enabled", false));
            if (appInfo.getSettingBoolValue("Logging.enabled", false)) {
                logWarn(qsTr("Logging"),
                        qsTr("Logging is enabled. Log file is at %1").arg(appInfo.getLogPath()) + "\n" + qsTr("You may turn off in Settings.") );
            }

            // Show notify if monitoring is enabled.
            console.debug("drivePage onStatusChanged Monitoring.enabled " + appInfo.getSettingBoolValue("Monitoring.enabled", false));
            if (appInfo.getSettingBoolValue("Monitoring.enabled", false)) {
                logWarn(qsTr("Monitoring"),
                        qsTr("Monitoring is enabled. Monitoring file is at %1").arg(appInfo.getMonitoringFilePath()) + "\n" + qsTr("You may turn off in Settings.") );
            }
        }
    }

    Component.onCompleted: {
        console.debug(Utility.nowText() + " drivePage onCompleted");
        window.updateLoadingProgressSlot(qsTr("%1 is loaded.").arg("DrivePage"), 0.2);

        // Parse all storages.
        refreshSlot("drivePage onCompleted");
    }
}
