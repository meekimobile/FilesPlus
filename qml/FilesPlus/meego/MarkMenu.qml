import QtQuick 1.1
import com.nokia.meego 1.0

MenuWithIcon {
    id: markMenu
    z: 2

    property bool isMarkAll: true

    signal markAll()
    signal copyMarkedItems()
    signal cutMarkedItems()
    signal deleteMarkedItems()
    signal syncMarkedItems()

    content: MenuLayout {
        id: menuLayout

        // TODO Alias for fixing incorrect children.
        default property alias children: menuLayout.menuChildren

        MenuItemWithIcon {
            id: toggleMarkAll
            name: "toggleMarkAll"
            text: (isMarkAll) ? appInfo.emptyStr+qsTr("Mark all") : appInfo.emptyStr+qsTr("Unmark all")
            onClicked: {
                markAll();
            }
        }

        MenuItemWithIcon {
            id: copyMarked
            name: "copyMarked"
            text: appInfo.emptyStr+qsTr("Copy marked items")
            onClicked: {
                copyMarkedItems();
            }
        }

        MenuItemWithIcon {
            id: cutMarked
            name: "cutMarked"
            text: appInfo.emptyStr+qsTr("Cut marked items")
            onClicked: {
                cutMarkedItems();
            }
        }

        MenuItemWithIcon {
            id: deleteMarked
            name: "deleteMarked"
            text: appInfo.emptyStr+qsTr("Delete marked items")
            onClicked: {
                deleteMarkedItems();
            }
        }

        MenuItemWithIcon {
            id: syncMarked
            name: "syncMarked"
            text: appInfo.emptyStr+qsTr("Sync marked items")
            onClicked: {
                syncMarkedItems();
            }
        }
    }

    // Override this function with menuItem logic.
    function isMenuItemVisible(menuItem) {
        // Validate each menu logic if it's specified, otherwise it's visible.
        return false;
    }
}
