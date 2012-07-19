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
            text: qsTr("Sort by Name")
            checked: (sortFlag == flag)
            platformInverted: window.platformInverted
            onClicked: {
                selectSort(flag);
            }
        }

        MenuItemWithCheck {
            property int flag: FolderSizeItemListModel.SortByType
            text: qsTr("Sort by Type")
            checked: (sortFlag == flag)
            platformInverted: window.platformInverted
            onClicked: {
                selectSort(flag);
            }
        }

        MenuItemWithCheck {
            property int flag: FolderSizeItemListModel.SortByTime
            text: qsTr("Sort by Time")
            checked: (sortFlag == flag)
            platformInverted: window.platformInverted
            onClicked: {
                selectSort(flag);
            }
        }

        MenuItemWithCheck {
            property int flag: FolderSizeItemListModel.SortBySize
            text: qsTr("Sort by Size")
            checked: (sortFlag == flag)
            platformInverted: window.platformInverted
            onClicked: {
                selectSort(flag);
            }
        }
    }
}
