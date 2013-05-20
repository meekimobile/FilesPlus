import QtQuick 1.1
import com.nokia.symbian 1.1

MenuWithIcon {
    id: bookmarksMenu
    z: 2
    
    signal addBookmark()
    signal openBookmarks()
    
    content: MenuLayout {
        id: menuLayout
        
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
