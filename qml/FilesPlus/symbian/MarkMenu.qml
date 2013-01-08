import QtQuick 1.1
import com.nokia.symbian 1.1

MenuWithIcon {
    id: markMenu
    z: 2
    platformInverted: window.platformInverted

    property variant disabledMenus: []
    property bool isMarkAll: true

    signal markAll()
    signal copyMarkedItems()
    signal cutMarkedItems()
    signal deleteMarkedItems()
    signal syncMarkedItems()

    content: MenuLayout {
        id: menuLayout

        MenuItemWithIcon {
            id: toggleMarkAll
            name: "toggleMarkAll"
            platformInverted: window.platformInverted
            text: (isMarkAll) ? appInfo.emptyStr+qsTr("Mark all") : appInfo.emptyStr+qsTr("Unmark all")
            onClicked: {
                markAll();
            }
        }

        MenuItemWithIcon {
            id: copyMarked
            name: "copyMarked"
            platformInverted: window.platformInverted
            text: appInfo.emptyStr+qsTr("Copy marked items")
            onClicked: {
                copyMarkedItems();
            }
        }

        MenuItemWithIcon {
            id: cutMarked
            name: "cutMarked"
            platformInverted: window.platformInverted
            text: appInfo.emptyStr+qsTr("Cut marked items")
            onClicked: {
                cutMarkedItems();
            }
        }

        MenuItemWithIcon {
            id: deleteMarked
            name: "deleteMarked"
            platformInverted: window.platformInverted
            text: appInfo.emptyStr+qsTr("Delete marked items")
            onClicked: {
                deleteMarkedItems();
            }
        }

        MenuItemWithIcon {
            id: syncMarked
            name: "syncMarked"
            platformInverted: window.platformInverted
            text: appInfo.emptyStr+qsTr("Sync marked items")
            onClicked: {
                syncMarkedItems();
            }
        }
    }
}
