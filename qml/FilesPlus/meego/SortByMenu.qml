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
            text: appInfo.emptyStr+qsTr("Sort by Name")
            checked: (sortFlag == flag)
            platformLeftMargin: 60
            onClicked: {
                sortFlag = flag;
                selectSort(flag);
            }
        }

        MenuItemWithIcon {
            property int flag: FolderSizeItemListModel.SortByNameWithDirectoryFirst
            text: appInfo.emptyStr+qsTr("Sort by Name (dir. first)")
            checked: (sortFlag == flag)
            platformLeftMargin: 60
            onClicked: {
                sortFlag = flag;
                selectSort(flag);
            }
        }

        MenuItemWithIcon {
            property int flag: FolderSizeItemListModel.SortByType
            text: appInfo.emptyStr+qsTr("Sort by Type")
            checked: (sortFlag == flag)
            platformLeftMargin: 60
            onClicked: {
                sortFlag = flag;
                selectSort(flag);
            }
        }

        MenuItemWithIcon {
            property int flag: FolderSizeItemListModel.SortByTime
            text: appInfo.emptyStr+qsTr("Sort by Time")
            checked: (sortFlag == flag)
            platformLeftMargin: 60
            onClicked: {
                sortFlag = flag;
                selectSort(flag);
            }
        }

        MenuItemWithIcon {
            property int flag: FolderSizeItemListModel.SortBySize
            text: appInfo.emptyStr+qsTr("Sort by Size")
            checked: (sortFlag == flag)
            platformLeftMargin: 60
            onClicked: {
                sortFlag = flag;
                selectSort(flag);
            }
        }
    }
}
