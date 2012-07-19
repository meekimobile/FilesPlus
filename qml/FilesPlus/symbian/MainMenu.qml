import QtQuick 1.1
import com.nokia.symbian 1.1
import CloudDriveModel 1.0

Menu {
    id: mainMenu
    z: 2

    platformInverted: window.platformInverted

    property variant enabledMenus: []
    property variant disabledMenus: []

    signal quit()
    signal paste()

    content: MenuLayout {
        id: menuLayout

        MenuItem {
            id: pasteMenuItem
            text: qsTr("Paste")
            platformInverted: window.platformInverted
            onClicked: {
                paste();
            }
        }

        MenuItem {
            id: markMenuItem
            text: qsTr("Mark multiple items")
            onClicked: {
                fsListView.state = "mark";
            }
        }

        MenuItem {
            id: clearClipboardMenuItem
            text: qsTr("Clear clipboard")
            onClicked: {
                clipboard.clear();
            }
        }

        MenuItem {
            id: newFolderMenuItem
            text: qsTr("New folder")
            onClicked: {
                newFolderDialog.open();
            }
        }

        MenuItem {
            id: syncItemsMenuItem
            text: qsTr("Sync connected items")
            onClicked: {
                syncConnectedItemsSlot();
            }
        }

        MenuItem {
            id: syncFolderMenuItem
            text: qsTr("Sync current folder")
            onClicked: {
                console.debug("mainMenu syncFolderMenuItem fsModel.currentDir " + fsModel.currentDir);
                syncFileSlot(fsModel.currentDir, -1);
            }
        }

        MenuItem {
            id: sortByMenuItem
            text: qsTr("Sort by")
            platformSubItemIndicator: true
            onClicked: {
                sortByMenu.open();
            }
        }

        MenuItem {
            text: qsTr("Settings")
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
            text: qsTr("About")
            onClicked: {
                pageStack.push(Qt.resolvedUrl("AboutPage.qml"));
            }
        }
                
//        MenuItem {
//            text: qsTr("More Apps")
//            onClicked: {
//                pageStack.push(Qt.resolvedUrl("MoreApps.qml"));
//            }
//        }
        
        MenuItem {
            text: qsTr("Exit")
            onClicked: {
                quit();
            }
        }
    }

    function isEnabled(menuName) {
        if (enabledMenus && enabledMenus.length > 0) {
            return (enabledMenus.indexOf(menuName) != -1);
        }
        return true;
    }

    function isDisabled(menuName) {
        if (disabledMenus && disabledMenus.length > 0) {
            return (disabledMenus.indexOf(menuName) != -1);
        }
        return false;
    }

    function toggleMenuItems() {
        console.debug(mainMenu.disabledMenus);
//        console.debug("mainMenu toggleMenuItems menuLayout.children.length " + menuLayout.children.length);
        for (var i=0; i<menuLayout.children.length; i++) {
            var menuItem = menuLayout.children[i];
            if (!isEnabled(menuItem.text) || isDisabled(menuItem.text)) {
                menuItem.visible = false;
//                console.debug("mainMenu toggleMenuItems menuLayout.children i " + i + " " + menuItem.toString() + " " + menuItem.text + " is hidden.");
            } else {
                // Validate each menu logic if it's specified, otherwise it's visible.
                if (menuItem == pasteMenuItem) {
                    menuItem.visible = clipboard.count > 0;
                } else if (menuItem == clearClipboardMenuItem) {
                    menuItem.visible = clipboard.count > 0;
                } else if (menuItem == markMenuItem) {
                    menuItem.visible = fsListView.state != "mark";
                } else if (menuItem == syncFolderMenuItem) {
                    menuItem.visible = !fsModel.isRoot() && cloudDriveModel.canSync(fsModel.currentDir);
                } else {
                    menuItem.visible = true;
//                    console.debug("mainMenu toggleMenuItems menuLayout.children i " + i + " " + menuItem.toString() + " " + menuItem.text + " is shown.");
                }
            }
        }
    }

    onStatusChanged: {
        if (status == DialogStatus.Opening) {
            toggleMenuItems();
            // After toggle, visible is not presented correctly. It will be updated on status=Open.
        }
    }
}
