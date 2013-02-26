import QtQuick 1.1
import com.nokia.symbian 1.1

MenuWithIcon {
    id: markAllMenu
    z: 2

    signal markAll()
    signal markAllFiles()
    signal markAllFolders()

    content: MenuLayout {
        id: menuLayout

        MenuItemWithIcon {
            text: appInfo.emptyStr+qsTr("Mark all")
            onClicked: {
                markAll();
            }
        }
        MenuItemWithIcon {
            text: appInfo.emptyStr+qsTr("Mark all files")
            onClicked: {
                markAllFiles();
            }
        }
        MenuItemWithIcon {
            text: appInfo.emptyStr+qsTr("Mark all folders")
            onClicked: {
                markAllFolders();
            }
        }
    }
}
