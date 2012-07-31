import QtQuick 1.1
import com.nokia.symbian 1.1

Menu {
    id: toolMenu
    z: 2
    platformInverted: window.platformInverted

    signal newFolder()
    signal renameFile()
    signal markClicked()

    content: MenuLayout {
        MenuItem {
            id: markMenuItem
            platformInverted: window.platformInverted
            text: appInfo.emptyStr+qsTr("Mark")
            onClicked: {
                markClicked();
            }
        }

        MenuItem {
            id: newFolderMenuItem
            platformInverted: window.platformInverted
            text: appInfo.emptyStr+qsTr("New Folder")
            onClicked: {
                newFolder();
            }
        }

        MenuItem {
            id: renameMenuItem
            platformInverted: window.platformInverted
            text: appInfo.emptyStr+qsTr("Rename")
            onClicked: {
                renameFile();
            }
        }
    }
}
