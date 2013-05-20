import QtQuick 1.1
import com.nokia.meego 1.0

MenuWithIcon {
    id: bookmarksMenu
    z: 2
    
    signal addBookmark()
    signal openBookmarks()
    
    content: MenuLayout {
        id: menuLayout
        
        // TODO Alias for fixing incorrect children.
        default property alias children: menuLayout.menuChildren
        
        MenuItemWithIcon {
            text: appInfo.emptyStr+qsTr("Add bookmark")
            onClicked: {
                addBookmark();
            }
        }
        MenuItemWithIcon {
            text: appInfo.emptyStr+qsTr("Open bookmarks")
            onClicked: {
                openBookmarks();
            }
        }
    }
}
