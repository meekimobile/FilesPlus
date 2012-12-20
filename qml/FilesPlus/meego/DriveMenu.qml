import QtQuick 1.1
import com.nokia.meego 1.0

MenuWithIcon {
    id: driveMenu
    z: 2

    content: MenuLayout {
        id: menuLayout

        // TODO Alias for fixing incorrect children.
        default property alias children: menuLayout.menuChildren

        MenuItemWithIcon {
            text: appInfo.emptyStr+qsTr("Settings")
            platformSubItemIndicator: true
            onClicked: {
                pageStack.push(Qt.resolvedUrl("SettingPage.qml"));
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
}
