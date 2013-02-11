import QtQuick 1.1
import com.nokia.symbian 1.1

MenuWithIcon {
    id: cloudMenu
    z: 2
    
    signal syncConnectedItems()
    signal syncCurrentFolder()
    signal showCloudDriveJobs()

    content: MenuLayout {
        MenuItemWithIcon {
            name: "syncConnectedItems"
            text: appInfo.emptyStr+qsTr("Sync connected items")
            onClicked: {
                syncConnectedItems();
            }
        }

        MenuItemWithIcon {
            name: "syncCurrentFolder"
            text: appInfo.emptyStr+qsTr("Sync current folder")
            onClicked: {
                syncCurrentFolder();
            }
        }

        MenuItemWithIcon {
            name: "showCloudDriveJobs"
            text: appInfo.emptyStr+qsTr("Show cloud drive jobs")
            onClicked: {
                showCloudDriveJobs();
            }
        }
    }
}
