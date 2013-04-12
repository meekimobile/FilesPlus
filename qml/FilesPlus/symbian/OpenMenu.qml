import QtQuick 1.1
import com.nokia.symbian 1.1

MenuWithIcon {
    id: openMenu
    z: 2
    
    signal openMedia()
    signal openWeb()
    signal copyURL()
    
    content: MenuLayout {       
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
