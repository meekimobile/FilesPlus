import QtQuick 1.1
import com.nokia.symbian 1.1
import CloudDriveModel 1.0

Menu {
    id: mainMenu
    z: 2

    property variant enabledMenus: []

    signal quit()

    content: MenuLayout {
        id: mainMenuLayout

        MenuItem {
            id: pasteMenuItem
            text: "Paste"
            onClicked: {
                fileActionDialog.targetPath = fsModel.currentDir;
                fileActionDialog.open();
            }
        }

        MenuItem {
            id: markMenuItem
            text: "Mark multiple items"
            onClicked: {
                fsListView.state = "mark";
            }
        }

        MenuItem {
            id: clearClipboardMenuItem
            text: "Clear clipboard"
            onClicked: {
                clipboard.clear();
            }
        }

        MenuItem {
            id: newFolderMenuItem
            text: "New folder"
            onClicked: {
                newFolderDialog.open();
            }
        }

        MenuItem {
            id: syncFolderMenuItem
            text: "Sync current folder"
            onClicked: {
                console.debug("mainMenu syncFolderMenuItem fsModel.currentDir " + fsModel.currentDir);
                syncFileSlot(fsModel.currentDir, -1);
            }
        }

        MenuItem {
            id: syncAllMenuItem
            text: "Sync all connected items"
            onClicked: {
                cloudDriveModel.syncItems();
            }
        }

        MenuItem {
            id: sortByMenuItem
            text: "Sort by"
            platformSubItemIndicator: true
            onClicked: {
                sortByMenu.open();
            }
        }

        MenuItem {
            id: settingMenuItem
            text: "Settings"
            platformSubItemIndicator: true
            onClicked: {
//                settingMenu.open();
                pageStack.push(Qt.resolvedUrl("SettingPage.qml"));
            }
        }

        MenuItem {
            text: "About"
            onClicked: {
                pageStack.push(Qt.resolvedUrl("AboutPage.qml"));
            }
        }
                
//        MenuItem {
//            text: "More Apps"
//            onClicked: {
//                pageStack.push(Qt.resolvedUrl("MoreApps.qml"));
//            }
//        }
        
        MenuItem {
            text: "Exit"
            onClicked: {
                quit();
            }
        }
    }

    function isEnabled(menuName) {
        if (enabledMenus.length > 0) {
            return (enabledMenus.indexOf(menuName) != -1);
        }
        return true;
    }

    onStatusChanged: {
        if (status == DialogStatus.Opening) {
            sortByMenuItem.visible = isEnabled(sortByMenuItem.text);
            settingMenuItem.visible = isEnabled(settingMenuItem.text);
            newFolderMenuItem.visible = isEnabled(newFolderMenuItem.text);
            pasteMenuItem.visible = isEnabled(pasteMenuItem.text) && (clipboard.count > 0);
            clearClipboardMenuItem.visible = isEnabled(clearClipboardMenuItem.text) && (clipboard.count > 0);
            markMenuItem.visible = isEnabled(markMenuItem.text) && (fsListView.state != "mark");
            syncFolderMenuItem.visible = isEnabled(syncFolderMenuItem.text) && (!fsModel.isRoot());
            syncAllMenuItem.visible = isEnabled(syncAllMenuItem.text);
        }
    }
}
