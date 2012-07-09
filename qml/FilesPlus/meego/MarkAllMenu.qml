import QtQuick 1.1
import com.nokia.meego 1.1

Menu {
    id: markAllMenu
    z: 2

    property variant disabledMenus: []

    content: MenuLayout {
        id: markAllMenuLayout

        MenuItem {
            text: "Mark all"
            onClicked: {
                fsListView.markAll();
            }
        }
    }
}
