import QtQuick 1.1
import com.nokia.symbian 1.1

Menu {
    id: toolMenu
    z: 2

    signal newFolder()
    signal renameFile()
    signal markClicked()

    content: MenuLayout {
        MenuItem {
            id: markMenuItem
            text: "Mark"
            onClicked: {
                markClicked();
            }
        }

        MenuItem {
            id: newFolderMenuItem
            text: "New Folder"
            onClicked: {
                newFolder();
            }
        }

        MenuItem {
            id: renameMenuItem
            text: "Rename"
            onClicked: {
                renameFile();
            }
        }
    }
}
