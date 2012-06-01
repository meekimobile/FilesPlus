import QtQuick 1.1
import com.nokia.symbian 1.1
import FolderSizeItemListModel 1.0

ContextMenu {
    id: settingMenu
    z: 2

    signal resetCache();
    signal resetCloudPrint();

    content: MenuLayout {
        MenuItem {
            id: menuResetCloudPrint
            text: "Reset CloudPrint"
            onClicked: {
                resetCloudPrint();
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
