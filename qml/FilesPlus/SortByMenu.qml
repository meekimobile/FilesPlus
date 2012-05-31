import QtQuick 1.1
import com.nokia.symbian 1.1
import FolderSizeItemListModel 1.0

ContextMenu {
    id: sortByMenu
    z: 2

    signal selectSort (int flag)

    content: MenuLayout {
        MenuItem {
            text: "Sort by Name"
            onClicked: {
                selectSort(FolderSizeItemListModel.SortByName);
            }
        }

        MenuItem {
            text: "Sort by Type"
            onClicked: {
                selectSort(FolderSizeItemListModel.SortByType);
            }
        }

        MenuItem {
            text: "Sort by Time"
            onClicked: {
                selectSort(FolderSizeItemListModel.SortByTime);
            }
        }

        MenuItem {
            text: "Sort by Size"
            onClicked: {
                selectSort(FolderSizeItemListModel.SortBySize);
            }
        }
    }
}
