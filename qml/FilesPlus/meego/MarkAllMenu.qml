import QtQuick 1.1
import com.nokia.meego 1.0

MenuWithIcon {
    id: markAllMenu
    z: 2

    signal markAll()
    signal markAllFiles()
    signal markAllFolders()

    content: MenuLayout {
        id: menuLayout

        // TODO Alias for fixing incorrect children.
        default property alias children: menuLayout.menuChildren

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
