import QtQuick 1.1
import com.nokia.symbian 1.1
import CloudDriveModel 1.0

MenuWithIcon {
    id: cloudFolderMainMenu
    z: 2

    signal paste()
    signal openMarkMenu()
    signal clearClipboard()
    signal newFolder()
    signal syncConnectedItems()
    signal syncCurrentFolder()
    signal setNameFilter()
    signal drives()
    signal openSortByMenu()
    signal openSettings()
    signal openMoreApps()
    signal openAbout()
    signal quit()

    content: MenuLayout {
        id: menuLayout

        MenuItemWithIcon {
            id: pasteMenuItem
            name: "paste"
            text: appInfo.emptyStr+qsTr("Paste")
            onClicked: {
                paste();
            }
        }

        MenuItemWithIcon {
            id: markMenuItem
            name: "markMenu"
            text: appInfo.emptyStr+qsTr("Mark multiple items")
            onClicked: {
                openMarkMenu();
            }
        }

        MenuItemWithIcon {
            id: clearClipboardMenuItem
            name: "clearClipboard"
            text: appInfo.emptyStr+qsTr("Clear clipboard")
            onClicked: {
                clearClipboard();
            }

            TextIndicator {
                id: clipboardIndicator
                color: "#00AAFF"
                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                text: clipboard.count
            }
        }

        MenuItemWithIcon {
            id: newFolderMenuItem
            name: "newFolder"
            text: appInfo.emptyStr+qsTr("New folder")
            onClicked: {
                newFolder();
            }
        }

//        MenuItemWithIcon {
//            id: syncItemsMenuItem
//            name: "syncConnectedItems"
//            text: appInfo.emptyStr+qsTr("Sync connected items")
//            onClicked: {
//                syncConnectedItems();
//            }
//        }

//        MenuItemWithIcon {
//            id: syncFolderMenuItem
//            name: "syncCurrentFolder"
//            text: appInfo.emptyStr+qsTr("Sync current folder")
//            onClicked: {
//                syncCurrentFolder();
//            }
//        }

        MenuItemWithIcon {
            id: filterMenuItem
            name: "setNameFilter"
            text: appInfo.emptyStr+qsTr("Find name")
            onClicked: {
                setNameFilter();
            }
        }

        MenuItemWithIcon {
            id: driveMenuItem
            name: "driveMenuItem"
            text: appInfo.emptyStr+qsTr("Drives")
            onClicked: {
                drives();
            }
        }

        MenuItemWithIcon {
            id: sortByMenuItem
            name: "sortByMenu"
            text: appInfo.emptyStr+qsTr("Sort by")
            platformSubItemIndicator: true
            onClicked: {
                openSortByMenu();
            }
        }

        MenuItemWithIcon {
            name: "settings"
            text: appInfo.emptyStr+qsTr("Settings")
            platformSubItemIndicator: true
            onClicked: {
                openSettings();
            }
        }

        MenuItemWithIcon {
            name: "about"
            text: appInfo.emptyStr+qsTr("About")
            onClicked: {
                openAbout();
            }
        }
                
//        MenuItemWithIcon {
//            name: "moreApps"
//            text: appInfo.emptyStr+qsTr("More Apps")
//            onClicked: {
//                openMoreApps();
//            }
//        }
        
        MenuItemWithIcon {
            name: "exit"
            text: appInfo.emptyStr+qsTr("Exit")
            onClicked: {
                quit();
            }
        }
    }
}
