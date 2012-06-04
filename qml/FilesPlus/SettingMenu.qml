import QtQuick 1.1
import com.nokia.symbian 1.1
import FolderSizeItemListModel 1.0

ContextMenu {
    id: settingMenu
    z: 2

    signal resetCache();
    signal resetCloudPrint();
    signal resetCloudDrive();

    content: MenuLayout {
        MenuItem {
            id: menuResetCloudPrint
            text: "Reset CloudPrint"
            onClicked: {
                resetCloudPrint();
            }
        }

        MenuItem {
            id: menuResetCloudDrive
            text: "Reset CloudDrive"
            onClicked: {
                resetCloudDrive();
            }
        }

        MenuItem {
            id: menuResetCache
            text: "Reset Cache"

            onClicked: {
                resetCache();
            }
        }
    }
}
