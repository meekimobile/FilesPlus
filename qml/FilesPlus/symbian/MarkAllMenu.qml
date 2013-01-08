import QtQuick 1.1
import com.nokia.symbian 1.1

Menu {
    id: markAllMenu
    z: 2
    platformInverted: window.platformInverted

    property variant disabledMenus: []

    signal markAll()
    signal markAllFiles()
    signal markAllFolders()

    content: MenuLayout {
        id: markAllMenuLayout

        MenuItem {
            platformInverted: window.platformInverted
            text: appInfo.emptyStr+qsTr("Mark all")
            onClicked: {
                markAll();
            }
        }
        MenuItem {
            platformInverted: window.platformInverted
            text: appInfo.emptyStr+qsTr("Mark all files")
            onClicked: {
                markAllFiles();
            }
        }
        MenuItem {
            platformInverted: window.platformInverted
            text: appInfo.emptyStr+qsTr("Mark all folders")
            onClicked: {
                markAllFolders();
            }
        }
    }
}
