import QtQuick 1.1
import com.nokia.meego 1.0

MenuWithIcon {
    id: openMenu
    z: 2

    property int selectedIndex

    signal openMedia()
    signal openSystem()

    content: MenuLayout {
        id: menuLayout

        // TODO Alias for fixing incorrect children.
        default property alias children: menuLayout.menuChildren

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
