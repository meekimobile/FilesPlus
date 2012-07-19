import QtQuick 1.1
import com.nokia.meego 1.0

MenuWithIcon {
    id: chartMenu
    z: 2

    content: MenuLayout {
        id: menuLayout

        // TODO Alias for fixing incorrect children.
        default property alias children: menuLayout.menuChildren

        MenuItemWithIcon {
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
}
