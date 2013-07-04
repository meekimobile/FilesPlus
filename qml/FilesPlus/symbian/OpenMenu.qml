import QtQuick 1.1
import com.nokia.symbian 1.1

MenuWithIcon {
    id: openMenu
    z: 2

    property int selectedIndex

    signal openMedia()
    signal openSystem()

    content: MenuLayout {
        MenuItemWithIcon {
            name: "openMedia"
            text: appInfo.emptyStr+qsTr("Open")
            onClicked: {
                openMedia();
            }
        }
        
        MenuItemWithIcon {
            name: "openSystem"
            text: appInfo.emptyStr+qsTr("Open (System)")
            onClicked: {
                openSystem();
            }
        }
    }
}
