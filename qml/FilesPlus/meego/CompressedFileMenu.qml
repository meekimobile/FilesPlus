import QtQuick 1.1
import com.nokia.meego 1.0

MenuWithIcon {
    id: compressedFileMenu
    z: 2

    property int selectedIndex
    
    signal openCompressedFile()
    signal openExternally()
    signal extract()
    
    content: MenuLayout {
        id: menuLayout
        
        // TODO Alias for fixing incorrect children.
        default property alias children: menuLayout.menuChildren
        
        MenuItemWithIcon {
            text: appInfo.emptyStr+qsTr("Open")
            onClicked: {
                openCompressedFile();
            }
        }
        MenuItemWithIcon {
            text: appInfo.emptyStr+qsTr("Open (System)")
            onClicked: {
                openExternally();
            }
        }
        MenuItemWithIcon {
            text: appInfo.emptyStr+qsTr("Extract")
            onClicked: {
                extract();
            }
        }
    }
}
