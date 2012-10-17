import QtQuick 1.1
import com.nokia.meego 1.0
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
            text: appInfo.emptyStr+qsTr("Paste")
            onClicked: {
                paste();
            }
        }

        MenuItemWithIcon {
            id: markMenuItem
            text: appInfo.emptyStr+qsTr("Mark multiple items")
            onClicked: {
                fsListView.state = "mark";
            }
        }

        MenuItemWithIcon {
            id: clearClipboardMenuItem
            text: appInfo.emptyStr+qsTr("Clear clipboard")
            onClicked: {
                clipboard.clear();
            }
        }

        MenuItemWithIcon {
            id: newFolderMenuItem
            text: appInfo.emptyStr+qsTr("New folder / file")
            onClicked: {
                newFolderDialog.open();
            }
        }

        MenuItemWithIcon {
            id: syncItemsMenuItem
            text: appInfo.emptyStr+qsTr("Sync connected items")
            onClicked: {
                syncConnectedItemsSlot();
            }
        }

        MenuItemWithIcon {
            id: syncFolderMenuItem
            text: appInfo.emptyStr+qsTr("Sync current folder")
            onClicked: {
                console.debug("mainMenu syncFolderMenuItem fsModel.currentDir " + fsModel.currentDir);
                syncFileSlot(fsModel.currentDir, -1);
            }
        }

        MenuItemWithIcon {
            id: sortByMenuItem
            text: appInfo.emptyStr+qsTr("Sort by")
            platformSubItemIndicator: true
            onClicked: {
                sortByMenu.open();
            }
        }

        MenuItemWithIcon {
            text: appInfo.emptyStr+qsTr("Settings")
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
            text: appInfo.emptyStr+qsTr("About")
            onClicked: {
                pageStack.push(Qt.resolvedUrl("AboutPage.qml"));
            }
        }
                
//        MenuItemWithIcon {
//            text: appInfo.emptyStr+qsTr("More Apps")
//            onClicked: {
//                Qt.openUrlExternally("http://www.meeki.mobi/");
//            }
//        }
        
        MenuItemWithIcon {
            text: appInfo.emptyStr+qsTr("Exit")
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
            return !fsModel.isRoot() && cloudDriveModel.canSync(fsModel.currentDir);
        } else {
            return true;
        }
    }
}
