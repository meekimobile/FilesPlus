import QtQuick 1.1
import com.nokia.meego 1.1
import QtMobility.systeminfo 1.2
import SystemInfoHelper 1.0
import "Utility.js" as Utility

Page {
    id: drivePage

    property string name: "drivePage"

    tools: toolBarLayout

    function quitSlot() {
        var p = pageStack.find(function (page) { return page.name == "folderPage"; });
        if (p) p.quitSlot();
    }

    ToolBarLayout {
        id: toolBarLayout

        ToolIcon {
            id: backButton
            iconId: "toolbar-back"
            onClicked: {
                quitSlot();
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
            quitSlot();
        }
    }

    StorageInfo {
        id: storageInfo
    }

    SystemInfoHelper {
        id: systemInfoHelper
    }

    function getStorageModel() {
        var simulatedDriveNames = systemInfoHelper.getDriveList();
        var model = Qt.createQmlObject(
                    'import QtQuick 1.1; ListModel {}', drivePage);
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
                                 driveType: driveType
                });
            }
        }

        return model;
    }

    DriveGrid {
        id: driveGrid
        width: parent.width
        height: parent.height - titlePanel.height
        anchors.top: titlePanel.bottom
        model: getStorageModel()

        onDriveSelected: {
            console.debug("driveSelection onDriveSelected " + driveName);
            var p = pageStack.find(function(page) { return (page.name == "folderPage"); });
            if (p) p.currentDir = driveName;
            pageStack.pop(drivePage, true);
        }
    }

    TitlePanel {
        id: titlePanel
        text: "Drives"
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            driveGrid.currentIndex = -1;
        }
    }

    Component.onCompleted: {
        console.debug(Utility.nowText() + " drivePage onCompleted");
    }
}
