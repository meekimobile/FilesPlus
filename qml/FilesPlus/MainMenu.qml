import QtQuick 1.1
import com.nokia.symbian 1.1
import CloudDriveModel 1.0

Menu {
    id: mainMenu
    z: 2

    property variant disabledMenus: []

    content: MenuLayout {
        id: mainMenuLayout

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
                settingMenu.open();
            }
        }

        MenuItem {
            text: "About"
            onClicked: {
                pageStack.push(Qt.resolvedUrl("AboutPage.qml"));
            }
        }
                
        MenuItem {
            text: "More Apps"
            onClicked: {
                pageStack.push(Qt.resolvedUrl("MoreApps.qml"));
            }
        }
        
        MenuItem {
            text: "Exit"
            onClicked: {
                Qt.quit();
            }
        }
    }

    onStatusChanged: {
        if (status == DialogStatus.Opening) {
            sortByMenuItem.visible = (disabledMenus.indexOf(sortByMenuItem.text) == -1);
            settingMenuItem.visible = (disabledMenus.indexOf(settingMenuItem.text) == -1);
        }
    }
}
