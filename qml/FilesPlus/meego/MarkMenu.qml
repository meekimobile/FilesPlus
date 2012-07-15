import QtQuick 1.1
import com.nokia.meego 1.0

Menu {
    id: markMenu
    z: 2

    property variant disabledMenus: []

    content: MenuLayout {
        id: menuLayout

        MenuItem {
            id: markAll

            property bool isMarkAll: true

            text: (isMarkAll) ? qsTr("Mark all") : qsTr("Unmark all")
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
            text: qsTr("Copy marked items")
            onClicked: {
                fsListView.copyMarkedItems();
                fsListView.state = "";
            }
        }

        MenuItem {
            id: cutMarked
            text: qsTr("Cut marked items")
            onClicked: {
                fsListView.cutMarkedItems();
                fsListView.state = "";
            }
        }

        MenuItem {
            id: deleteMarked
            text: qsTr("Delete marked items")
            onClicked: {
                fsListView.deleteMarkedItems();
                fsListView.state = "";
            }
        }

        MenuItem {
            id: syncMarked
            text: qsTr("Sync marked items")
            onClicked: {
                fsListView.syncMarkedItems();
                fsListView.state = "";
            }
        }
    }

    onStatusChanged: {
        if (status == DialogStatus.Opening) {
            markAll.isMarkAll = !fsListView.areAllItemChecked();
        }
    }
}
