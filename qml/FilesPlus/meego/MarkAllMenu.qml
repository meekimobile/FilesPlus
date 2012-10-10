import QtQuick 1.1
import com.nokia.meego 1.0

Menu {
    id: markAllMenu
    z: 2

    property variant disabledMenus: []

    content: MenuLayout {
        id: markAllMenuLayout

        MenuItem {
            text: appInfo.emptyStr+qsTr("Mark all")
            onClicked: {
                fsListView.markAll();
            }
        }
        MenuItem {
            text: appInfo.emptyStr+qsTr("Mark all files")
            onClicked: {
                fsListView.markAllFiles();
            }
        }
        MenuItem {
            text: appInfo.emptyStr+qsTr("Mark all folders")
            onClicked: {
                fsListView.markAllFolders();
            }
        }
    }
}
