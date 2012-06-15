import QtQuick 1.1
import com.nokia.symbian 1.1

ContextMenu {
    id: toolMenu
    z: 2

    signal newFolder();
    signal renameFile();

    content: MenuLayout {
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
