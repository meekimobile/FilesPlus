import QtQuick 1.1
import com.nokia.symbian 1.1
import FolderSizeItemListModel 1.0

Menu {
    id: sortByMenu
    z: 2

    property int sortFlag
    property variant disabledSorts: []

    signal selectSort (int flag)

    content: MenuLayout {
        MenuItemWithCheck {
            property int flag: FolderSizeItemListModel.SortByName
            text: "Sort by Name"
            checked: (sortFlag == flag)
            onClicked: {
                selectSort(flag);
            }
        }

        MenuItemWithCheck {
            property int flag: FolderSizeItemListModel.SortByType
            text: "Sort by Type"
            checked: (sortFlag == flag)
            onClicked: {
                selectSort(flag);
            }
        }

        MenuItemWithCheck {
            property int flag: FolderSizeItemListModel.SortByTime
            text: "Sort by Time"
            checked: (sortFlag == flag)
            onClicked: {
                selectSort(flag);
            }
        }

        MenuItemWithCheck {
            property int flag: FolderSizeItemListModel.SortBySize
            text: "Sort by Size"
            checked: (sortFlag == flag)
            onClicked: {
                selectSort(flag);
            }
        }
    }
}
