import QtQuick 1.1
import com.nokia.meego 1.0

MenuWithIcon {
    id: openMenu
    z: 2
    
    signal openMedia()
    signal openWeb()
    signal copyURL()
    
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
            name: "openAlternative"
            text: appInfo.emptyStr+qsTr("Open on web")
            onClicked: {
                openWeb();
            }
        }
    }
}
