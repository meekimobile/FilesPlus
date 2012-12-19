import QtQuick 1.1
import com.nokia.meego 1.0

Menu {
    id: markMenu
    z: 2

    property variant disabledMenus: []
    property bool isMarkAll: true

    signal markAll()
    signal copyMarkedItems()
    signal cutMarkedItems()
    signal deleteMarkedItems()
    signal syncMarkedItems()

    content: MenuLayout {
        id: menuLayout

        MenuItem {
            id: toggleMarkAll
            text: (isMarkAll) ? appInfo.emptyStr+qsTr("Mark all") : appInfo.emptyStr+qsTr("Unmark all")
            onClicked: {
                markAll();
            }
        }

        MenuItem {
            id: copyMarked
            text: appInfo.emptyStr+qsTr("Copy marked items")
            onClicked: {
                copyMarkedItems();
            }
        }

        MenuItem {
            id: cutMarked
            text: appInfo.emptyStr+qsTr("Cut marked items")
            onClicked: {
                cutMarkedItems();
            }
        }

        MenuItem {
            id: deleteMarked
            text: appInfo.emptyStr+qsTr("Delete marked items")
            onClicked: {
                deleteMarkedItems();
            }
        }

        MenuItem {
            id: syncMarked
            text: appInfo.emptyStr+qsTr("Sync marked items")
            onClicked: {
                syncMarkedItems();
            }
        }
    }
}
