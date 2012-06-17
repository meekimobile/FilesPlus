import QtQuick 1.1
import com.nokia.symbian 1.1

Menu {
    id: settingMenu
    z: 2

    signal resetCache();
    signal resetCloudPrint();
    signal resetCloudDrive();
    signal registerDropboxUser();

    content: MenuLayout {
        MenuItem {
            id: menuResetCloudPrint
            text: "Reset CloudPrint"
            onClicked: {
                resetCloudPrint();
            }
        }

        MenuItem {
            text: "Register Dropbox user"
            onClicked: {
                registerDropboxUser();
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
