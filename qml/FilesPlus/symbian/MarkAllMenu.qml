import QtQuick 1.1
import com.nokia.symbian 1.1

Menu {
    id: markAllMenu
    z: 2
    platformInverted: window.platformInverted

    property variant disabledMenus: []

    content: MenuLayout {
        id: markAllMenuLayout

        MenuItem {
            platformInverted: window.platformInverted
            text: appInfo.emptyStr+qsTr("Mark all")
            onClicked: {
                fsListView.markAll();
            }
        }
        MenuItem {
            platformInverted: window.platformInverted
            text: appInfo.emptyStr+qsTr("Mark all files")
            onClicked: {
                fsListView.markAllFiles();
            }
        }
        MenuItem {
            platformInverted: window.platformInverted
            text: appInfo.emptyStr+qsTr("Mark all folders")
            onClicked: {
                fsListView.markAllFolders();
            }
        }
    }
}
