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
        var i = driveGridModel.findIndexByCloudTypeAndUid(type, uid);
        if (i > -1) {
            var item = driveGrid.model.get(i);
            console.debug("drivePage updateAccountInfoSlot found item i " + i + " uid " + item.uid + " driveType " + item.driveType + " cloudDriveType " + item.cloudDriveType);

            driveGrid.model.set(i, { availableSpace: (quota - shared - normal), totalSpace: quota });
            if (email) {
                driveGrid.model.set(i, { logicalDrive: email, email: email });
            }
            if (name) {
                driveGrid.model.set(i, { name: name });
            }
            // Return if found.
            return;
        }

        console.debug("drivePage updateAccountInfoSlot not found uid " + uid + " cloudDriveType " + type + ". Proceed refresh cloud drive from accounts.");
        // Remove cloud drive from driveGrid model.
        i = 0;
        while (i < driveGrid.model.count) {
            item = driveGrid.model.get(i);
            if (item.driveType == 7) {
                driveGrid.model.remove(i);
            } else {
                i++;
            }
        }
        // Proceed copying cloud drive items.
        for (i = 0; i < cloudDriveAccountsModel.count; i++) {
            driveGrid.model.append(cloudDriveAccountsModel.get(i));
        }
    }

    function updateLogicalDriveSlot(logicalDrive, available, total) {
        var i = driveGridModel.findIndexByLogicalDrive(logicalDrive);
        console.debug("drivePage updateLogicalDriveSlot i " + i + " logicalDrive " + logicalDrive + " available " + available + " total " + total);
        if (i > -1) {
            var item = driveGrid.model.get(i);
            driveGrid.model.set(i, { availableSpace: available, totalSpace: total });
        }
    }

    function updateJobQueueCount(runningJobCount, jobQueueCount) {
        // Update (runningJobCount + jobQueueCount) on cloudButton.
        cloudButtonIndicator.text = ((runningJobCount + jobQueueCount) > 0) ? (runningJobCount + jobQueueCount) : "";
    }

    ToolBarLayout {
        id: toolBarLayout

        ToolBarButton {
            id: backButton
            buttonIconSource: "toolbar-back"
            onClicked: {
                window.quitSlot();
            }
        }
        ToolBarButton {
            id: refreshButton
            buttonIconSource: "toolbar-refresh"
            onClicked: {
                // Force resume.
                cloudDriveModel.resumeNextJob();

                // Parse all storages.
                refreshSlot("drivePage refreshButton onClicked");
            }
        }
        ToolBarButton {
            id: cloudButton
            buttonIconSource: (theme.inverted ? "cloud.svg" : "cloud_inverted.svg")

            TextIndicator {
                id: cloudButtonIndicator
                color: "#00AAFF"
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10
                mouseAreaDisabled: true
            }

            onClicked: {
                pageStack.push(Qt.resolvedUrl("CloudDriveJobsPage.qml"));
            }
        }
        ToolBarButton {
            id: menuButton
            buttonIconSource: "toolbar-view-menu"
            onClicked: {
                driveMenu.open();
            }
        }
    }

    DriveMenu {
        id: driveMenu

        onOpenBookmarks: {
            pageStack.push(Qt.resolvedUrl("BookmarksPage.qml"));
        }
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

        // Add private folder.
        if (appInfo.getSettingBoolValue("drivepage.privatedrive.enabled", false)) {
            var privateDrive = systemInfoHelper.getPrivateDrive();
            if (privateDrive != "") {
                model.append({
                                 logicalDrive: privateDrive,
                                 availableSpace: 0,
                                 totalSpace: -1,
                                 driveType: 1, // Internal drive.
                                 email: "",
                                 uid: "",
                                 name: qsTr("Private drive"),
                                 cloudDriveType: -1,
                                 iconSource: ""
                             });
            }
        }

        // Add trash folder.
        if (appInfo.getSettingBoolValue("drivepage.trash.enabled", false)) {
            model.append({
                             logicalDrive: fsModel.getTrashPath(),
                             availableSpace: 0,
                             totalSpace: -1,
                             driveType: 1, // Internal drive.
                             email: "",
                             uid: "",
                             name: qsTr("Trash"),
                             cloudDriveType: -1,
                             iconSource: "delete_list.svg"
                         });
            fsModel.requestTrashStatus();
        }
    }

    function refreshSlot(caller) {
        console.debug("drivePage refreshSlot caller " + caller);

        // TODO Parse all storages.
        driveGridModel.clear();
        parseLocalStorage(driveGridModel);
        if (appInfo.getSettingBoolValue("drivepage.clouddrive.enabled", false)) {
            // Parse accounts into CloudDriveAccountsModel and also copy to driveGridModel.
            cloudDriveAccountsModel.parseCloudDriveAccountsModel(driveGridModel);
        } else {
            // Parse accounts into CloudDriveAccountsModel only.
            cloudDriveAccountsModel.parseCloudDriveAccountsModel();
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
                    var item = driveGridModel.get(i);
                    if (item.driveType == 7 && item.cloudDriveType == cloudType && item.uid == uid) {
                        return i;
                    }
                }

                return -1;
            }

            function findIndexByLogicalDrive(logicalDrive) {
                for (var i = 0; i < driveGridModel.count; i++) {
                    var item = driveGridModel.get(i);
                    if (item.logicalDrive == logicalDrive) {
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
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            driveGrid.currentIndex = -1;
            busy = false;

            // Stop startup logging.
            appInfo.stopLogging();

            // Request cloud job queue status.
            cloudDriveModel.requestJobQueueStatus();

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
