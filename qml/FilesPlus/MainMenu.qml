import QtQuick 1.1
import com.nokia.symbian 1.1
import CloudDriveModel 1.0

Menu {
    id: mainMenu
    z: 2

    property variant enabledMenus: []

    signal quit()
    signal paste()

    content: MenuLayout {
        id: mainMenuLayout

        MenuItem {
            id: pasteMenuItem
            text: "Paste"
            onClicked: {
                paste();
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
                pageStack.find(function (page) {
                    if (page.name == "folderPage") {
                        page.requestJobQueueStatusSlot();
                    }
                });
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
            sortByMenuItem.visible = isEnabled(sortByMenuItem.text) && (folderPage.state == "list");
            settingMenuItem.visible = isEnabled(settingMenuItem.text);
            newFolderMenuItem.visible = isEnabled(newFolderMenuItem.text) && (folderPage.state == "list");
            pasteMenuItem.visible = isEnabled(pasteMenuItem.text) && (clipboard.count > 0) && (folderPage.state == "list");
            clearClipboardMenuItem.visible = isEnabled(clearClipboardMenuItem.text) && (clipboard.count > 0) && (folderPage.state == "list");
            markMenuItem.visible = isEnabled(markMenuItem.text) && (fsListView.state != "mark") && (folderPage.state == "list");
            syncFolderMenuItem.visible = isEnabled(syncFolderMenuItem.text) && (!fsModel.isRoot()) && (folderPage.state == "list");
        }
    }
}
