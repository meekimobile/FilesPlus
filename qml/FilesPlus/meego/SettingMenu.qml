import QtQuick 1.1
import com.nokia.meego 1.0

Menu {
    id: settingMenu
    z: 2

    signal resetCache()
    signal resetCloudPrint()
    signal resetCloudDrive()
    signal registerDropboxUser()
    signal showCloudPrintJobs()

    content: MenuLayout {
        id: menuLayout

        // TODO Alias for fixing incorrect children.
        default property alias children: menuLayout.menuChildren
        
		MenuItem {
            id: menuPrintJobs
            text: qsTr("Show CloudPrint jobs")
            onClicked: {
                showCloudPrintJobs();
            }
        }

        MenuItem {
            id: menuResetCloudPrint
            text: qsTr("Reset CloudPrint")
            onClicked: {
                resetCloudPrint();
            }
        }

        MenuItem {
            text: qsTr("Register new Dropbox account")
            onClicked: {
                registerDropboxUser();
            }
        }

        MenuItem {
            id: menuResetCache
            text: qsTr("Reset current folder cache")

            onClicked: {
                resetCache();
            }
        }
    }
}
