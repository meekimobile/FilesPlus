import QtQuick 1.1
import com.nokia.meego 1.0
import FolderSizeItemListModel 1.0

MenuWithIcon {
    id: sortByMenu
    z: 2

    property int sortFlag

    signal selectSort (int flag)

    content: MenuLayout {
        id: menuLayout

        // TODO Alias for fixing incorrect children.
        default property alias children: menuLayout.menuChildren

        MenuItemWithIcon {
            property int flag: FolderSizeItemListModel.SortByName
            text: "Sort by Name"
            checked: (sortFlag == flag)
            platformLeftMargin: 60
            onClicked: {
                selectSort(flag);
            }
        }

        MenuItemWithIcon {
            property int flag: FolderSizeItemListModel.SortByType
            text: "Sort by Type"
            checked: (sortFlag == flag)
            platformLeftMargin: 60
            onClicked: {
                selectSort(flag);
            }
        }

        MenuItemWithIcon {
            property int flag: FolderSizeItemListModel.SortByTime
            text: "Sort by Time"
            checked: (sortFlag == flag)
            platformLeftMargin: 60
            onClicked: {
                selectSort(flag);
            }
        }

        MenuItemWithIcon {
            property int flag: FolderSizeItemListModel.SortBySize
            text: "Sort by Size"
            checked: (sortFlag == flag)
            platformLeftMargin: 60
            onClicked: {
                selectSort(flag);
            }
        }
    }
}
