import QtQuick 1.1
import com.nokia.symbian 1.1

Menu {
    id: markMenu
    z: 2

    property variant disabledMenus: []

    content: MenuLayout {
        id: menuLayout

        MenuItem {
            id: markAll
            text: "Mark all"
            onClicked: {
                fsListView.markAll();
            }
        }

        MenuItem {
            id: unmarkAll
            text: "Unmark all"
            onClicked: {
                fsListView.unmarkAll();
            }
        }

        MenuItem {
            id: copyMarked
            text: "Copy marked items"
            onClicked: {
                fsListView.copyMarkedItems();
                fsListView.state = "";
            }
        }

        MenuItem {
            id: cutMarked
            text: "Cut marked items"
            onClicked: {
                fsListView.cutMarkedItems();
                fsListView.state = "";
            }
        }

        MenuItem {
            id: syncMarked
            text: "Sync marked items"
            onClicked: {
                // TODO
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
        }
    }
}
