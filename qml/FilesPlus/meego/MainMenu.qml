import QtQuick 1.1
import com.nokia.meego 1.1
import CloudDriveModel 1.0

MenuWithIcon {
    id: mainMenu
    z: 2

    signal paste()

    content: MenuLayout {
        id: menuLayout

        // TODO Alias for fixing incorrect children.
        default property alias children: menuLayout.menuChildren

        MenuItemWithIcon {
            id: pasteMenuItem
            text: "Paste"
            onClicked: {
                paste();
            }
        }

        MenuItemWithIcon {
            id: markMenuItem
            text: "Mark multiple items"
            onClicked: {
                fsListView.state = "mark";
            }
        }

        MenuItemWithIcon {
            id: clearClipboardMenuItem
            text: "Clear clipboard"
            onClicked: {
                clipboard.clear();
            }
        }

        MenuItemWithIcon {
            id: newFolderMenuItem
            text: "New folder"
            onClicked: {
                newFolderDialog.open();
            }
        }

        MenuItemWithIcon {
            id: syncItemsMenuItem
            text: "Sync connected items"
            onClicked: {
                syncConnectedItemsSlot();
            }
        }

        MenuItemWithIcon {
            id: syncFolderMenuItem
            text: "Sync current folder"
            onClicked: {
                console.debug("mainMenu syncFolderMenuItem fsModel.currentDir " + fsModel.currentDir);
                syncFileSlot(fsModel.currentDir, -1);
            }
        }

        MenuItemWithIcon {
            id: sortByMenuItem
            text: "Sort by"
            platformSubItemIndicator: true
            onClicked: {
                sortByMenu.open();
            }
        }

        MenuItemWithIcon {
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

        MenuItemWithIcon {
            text: "About"
            onClicked: {
                pageStack.push(Qt.resolvedUrl("AboutPage.qml"));
            }
        }
                
//        MenuItemWithIcon {
//            text: "More Apps"
//            onClicked: {
//                pageStack.push(Qt.resolvedUrl("MoreApps.qml"));
//            }
//        }
        
        MenuItemWithIcon {
            text: "Exit"
            onClicked: {
                quit();
            }
        }
    }

    // Override this function with menuItem logic.
    function isMenuItemVisible(menuItem) {
        // Validate each menu logic if it's specified, otherwise it's visible.
        if (menuItem == pasteMenuItem) {
            return clipboard.count > 0;
        } else if (menuItem == clearClipboardMenuItem) {
            return clipboard.count > 0;
        } else if (menuItem == markMenuItem) {
            return fsListView.state != "mark";
        } else if (menuItem == syncFolderMenuItem) {
            return !fsModel.isRoot() && cloudDriveModel.canSync(fsModel.currentDir)
        } else {
            return true;
        }
    }
}
