import QtQuick 1.1
import com.nokia.meego 1.0

MenuWithIcon {
    id: chartMenu
    z: 2

    signal drives()
    signal openSettings()
    signal openMoreApps()
    signal openAbout()
    signal quit()

    content: MenuLayout {
        id: menuLayout

        // TODO Alias for fixing incorrect children.
        default property alias children: menuLayout.menuChildren

        MenuItemWithIcon {
            id: driveMenuItem
            name: "driveMenuItem"
            text: appInfo.emptyStr+qsTr("Drives")
            onClicked: {
                drives();
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
