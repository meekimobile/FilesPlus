import QtQuick 1.1
import com.nokia.symbian 1.1

Menu {
    id: markMenu
    z: 2
    platformInverted: window.platformInverted

    property variant disabledMenus: []

    content: MenuLayout {
        id: menuLayout

        MenuItem {
            id: markAll

            property bool isMarkAll: true

            platformInverted: window.platformInverted
            text: (isMarkAll) ? appInfo.emptyStr+qsTr("Mark all") : appInfo.emptyStr+qsTr("Unmark all")
            onClicked: {
                if (isMarkAll) {
                    fsListView.markAll();
                } else {
                    fsListView.unmarkAll();
                }
                isMarkAll = !isMarkAll;
            }
        }

        MenuItem {
            id: copyMarked
            platformInverted: window.platformInverted
            text: appInfo.emptyStr+qsTr("Copy marked items")
            onClicked: {
                fsListView.copyMarkedItems();
                fsListView.state = "";
            }
        }

        MenuItem {
            id: cutMarked
            platformInverted: window.platformInverted
            text: appInfo.emptyStr+qsTr("Cut marked items")
            onClicked: {
                fsListView.cutMarkedItems();
                fsListView.state = "";
            }
        }

        MenuItem {
            id: deleteMarked
            platformInverted: window.platformInverted
            text: appInfo.emptyStr+qsTr("Delete marked items")
            onClicked: {
                fsListView.deleteMarkedItems();
                fsListView.state = "";
            }
        }

        MenuItem {
            id: syncMarked
            platformInverted: window.platformInverted
            text: appInfo.emptyStr+qsTr("Sync marked items")
            onClicked: {
                fsListView.syncMarkedItems();
                fsListView.state = "";
            }
        }
    }

    onStatusChanged: {
        if (status == DialogStatus.Opening) {
            var isAnyChecked = fsListView.isAnyItemChecked();
            console.debug("markMenu onStatusChanged isAnyChecked " + isAnyChecked);
            copyMarked.visible = isAnyChecked;
            cutMarked.visible = isAnyChecked;
            syncMarked.visible = isAnyChecked;
            deleteMarked.visible = isAnyChecked;
            markAll.isMarkAll = !fsListView.areAllItemChecked();
        }
    }
}
