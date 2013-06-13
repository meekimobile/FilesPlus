import QtQuick 1.1
import com.nokia.symbian 1.1

MenuWithIcon {
    id: compressedFileMenu
    z: 2

    property int selectedIndex
    
    signal openCompressedFile()
    signal extract()
    
    content: MenuLayout {
        id: menuLayout
        
        MenuItemWithIcon {
            text: appInfo.emptyStr+qsTr("Open")
            onClicked: {
                openCompressedFile();
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
