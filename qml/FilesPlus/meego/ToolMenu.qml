import QtQuick 1.1
import com.nokia.meego 1.0

Menu {
    id: toolMenu
    z: 2

    signal newFolder()
    signal renameFile()
    signal markClicked()

    content: MenuLayout {
        MenuItem {
            id: markMenuItem
            text: appInfo.emptyStr+qsTr("Mark")
            onClicked: {
                markClicked();
            }
        }

        MenuItem {
            id: newFolderMenuItem
            text: appInfo.emptyStr+qsTr("New Folder")
            onClicked: {
                newFolder();
            }
        }

        MenuItem {
            id: renameMenuItem
            text: appInfo.emptyStr+qsTr("Rename")
            onClicked: {
                renameFile();
            }
        }
    }
}
