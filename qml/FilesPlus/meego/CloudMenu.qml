import QtQuick 1.1
import com.nokia.meego 1.0

MenuWithIcon {
    id: cloudMenu
    z: 2
    
    signal syncConnectedItems()
    signal syncCurrentFolder()
    signal showCloudDriveJobs()

    content: MenuLayout {
        id: menuLayout
        
        // TODO Alias for fixing incorrect children.
        default property alias children: menuLayout.menuChildren

        MenuItemWithIcon {
            name: "syncCurrentFolder"
            text: appInfo.emptyStr+qsTr("Sync current folder")
            onClicked: {
                syncCurrentFolder();
            }
        }

        MenuItemWithIcon {
            name: "syncConnectedItems"
            text: appInfo.emptyStr+qsTr("Sync connected items")
            onClicked: {
                syncConnectedItems();
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
