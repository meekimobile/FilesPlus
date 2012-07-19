import QtQuick 1.1
import com.nokia.symbian 1.1

Menu {
    id: settingMenu
    z: 2
    platformInverted: window.platformInverted

    signal resetCache()
    signal resetCloudPrint()
    signal resetCloudDrive()
    signal registerDropboxUser()
    signal showCloudPrintJobs()

    content: MenuLayout {
        MenuItem {
            id: menuPrintJobs
            platformInverted: window.platformInverted
            text: qsTr("Show CloudPrint jobs")
            onClicked: {
                showCloudPrintJobs();
            }
        }

        MenuItem {
            id: menuResetCloudPrint
            platformInverted: window.platformInverted
            text: qsTr("Reset CloudPrint")
            onClicked: {
                resetCloudPrint();
            }
        }

        MenuItem {
            platformInverted: window.platformInverted
            text: qsTr("Register new Dropbox account")
            onClicked: {
                registerDropboxUser();
            }
        }

        MenuItem {
            id: menuResetCache
            platformInverted: window.platformInverted
            text: qsTr("Reset current folder cache")

            onClicked: {
                resetCache();
            }
        }
    }
}
