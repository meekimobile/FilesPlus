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
            text: qsTr("Mark")
            onClicked: {
                markClicked();
            }
        }

        MenuItem {
            id: newFolderMenuItem
            text: qsTr("New Folder")
            onClicked: {
                newFolder();
            }
        }

        MenuItem {
            id: renameMenuItem
            text: qsTr("Rename")
            onClicked: {
                renameFile();
            }
        }
    }
}
