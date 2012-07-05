import QtQuick 1.1
import com.nokia.meego 1.1

Menu {
    id: settingMenu
    z: 2

    signal resetCache()
    signal resetCloudPrint()
    signal resetCloudDrive()
    signal registerDropboxUser()
    signal showCloudPrintJobs()

    content: MenuLayout {
        MenuItem {
            id: menuPrintJobs
            text: "Show CloudPrint jobs"
            onClicked: {
                showCloudPrintJobs();
            }
        }

        MenuItem {
            id: menuResetCloudPrint
            text: "Reset CloudPrint"
            onClicked: {
                resetCloudPrint();
            }
        }

        MenuItem {
            text: "Register new Dropbox account"
            onClicked: {
                registerDropboxUser();
            }
        }

        MenuItem {
            id: menuResetCache
            text: "Reset current folder cache"

            onClicked: {
                resetCache();
            }
        }
    }
}
