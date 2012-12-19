import QtQuick 1.1
import com.nokia.meego 1.0

Menu {
    id: markAllMenu
    z: 2

    property variant disabledMenus: []

    signal markAll()
    signal markAllFiles()
    signal markAllFolders()

    content: MenuLayout {
        id: markAllMenuLayout

        MenuItem {
            text: appInfo.emptyStr+qsTr("Mark all")
            onClicked: {
                markAll();
            }
        }
        MenuItem {
            text: appInfo.emptyStr+qsTr("Mark all files")
            onClicked: {
                markAllFiles();
            }
        }
        MenuItem {
            text: appInfo.emptyStr+qsTr("Mark all folders")
            onClicked: {
                markAllFolders();
            }
        }
    }
}
