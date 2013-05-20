import QtQuick 1.1
import com.nokia.symbian 1.1
import CloudDriveModel 1.0
import "Utility.js" as Utility

Page {
    id: bookmarksPage

    property string name: "bookmarksPage"
    property bool inverted: window.platformInverted

    tools: toolBarLayout

    ToolBarLayout {
        id: toolBarLayout

        ToolBarButton {
            id: backButton
            buttonIconSource: "toolbar-back"
            onClicked: {
                pageStack.pop();
            }
        }
        ToolBarButton {
            id: refreshButton
            buttonIconSource: "toolbar-refresh"
            onClicked: {
                bookmarksModel.refresh();
            }
        }
    }

    TitlePanel {
        id: titlePanel
        text: appInfo.emptyStr+qsTr("Bookmarks")
    }

    MenuWithIcon {
        id: itemMenu

        property int index

        content: MenuLayout {
            id: itemMenuLayout

            MenuItemWithIcon {
                name: "rename"
                text: appInfo.emptyStr+qsTr("Rename")
                onClicked: {
                    renameDialog.index = itemMenu.index;
                    renameDialog.open();
                }
            }

            MenuItemWithIcon {
                name: "remote"
                text: appInfo.emptyStr+qsTr("Remove")
                onClicked: {
                    bookmarksModel.deleteRow(itemMenu.index);
                    bookmarksModel.save();
                }
            }
        }

        onClosed: {
            // Hide highlight.
            listView.currentIndex = -1;
            listView.highlightFollowsCurrentItem = false;
        }
    }

    ConfirmDialog {
        id: renameDialog

        property int index
        property int type: -1
        property string uid
        property string path
        property string title

        titleText: appInfo.emptyStr+qsTr("Rename")
        buttonTexts: [appInfo.emptyStr+qsTr("OK"), appInfo.emptyStr+qsTr("Cancel")]
        content: Column {
            width: parent.width - 10
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 5
            Row {
                width: parent.width
                spacing: 5
                Image {
                    id: cloudIcon
                    source: (renameDialog.type < 0) ? (!inverted ? "device.svg" : "device_inverted.svg") : cloudDriveModel.getCloudIcon(renameDialog.type)
                    width: 48
                    height: 48
                    fillMode: Image.PreserveAspectFit
                    anchors.verticalCenter: parent.verticalCenter
                }
                Column {
                    width: parent.width - cloudIcon.width
                    spacing: 5
                    Text {
                        id: titleText
                        text: renameDialog.title
                        width: parent.width
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 8
                        elide: Text.ElideMiddle
                        color: (!inverted) ? "white" : "black"
                    }
                    Text {
                        id: subtitleText
                        text: (renameDialog.uid != "" ? ("(" + renameDialog.uid + ") ") : "") + renameDialog.path
                        width: parent.width
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 6
                        elide: Text.ElideMiddle
                        color: "grey"
                    }
                }
            }
            TextField {
                id: newTitle
                width: parent.width
                placeholderText: appInfo.emptyStr+qsTr("Please input new name.")
            }
        }

        onOpened: {
            var item = bookmarksModel.get(index);
            type = item.type;
            uid = item.uid;
            path = item.path;
            title = item.title;
            newTitle.text = "";
        }
        onConfirm: {
            console.debug("bookmarksPage renameDialog onConfirm " + newTitle.text);
            if (newTitle.text.trim() == "") return;

            bookmarksModel.setProperty(index, "title", newTitle.text.trim());
            bookmarksModel.save();
        }
    }

    ListView {
        id: listView
        width: parent.width
        height: parent.height - titlePanel.height
        anchors.top: titlePanel.bottom
        delegate: listItemDelegate
        clip: true
        model: bookmarksModel
        pressDelay: 100
        highlightRangeMode: ListView.NoHighlightRange
        highlightFollowsCurrentItem: false
        highlightMoveDuration: 1
        highlightMoveSpeed: 4000
        highlight: Rectangle {
            width: listView.width
            gradient: Gradient {
                GradientStop {
                    position: 0
                    color: "#0080D0"
                }

                GradientStop {
                    position: 1
                    color: "#53A3E6"
                }
            }
        }

        onMovementStarted: {
            if (currentItem) {
                currentItem.pressed = false;
            }
        }
    }

    ScrollDecorator {
        id: scrollbar
        flickableItem: listView
    }

    Component {
        id: listItemDelegate

        ListItem {
            id: listItem
            Row {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 5

                Image {
                    id: cloudIcon
                    source: (type < 0) ? (!inverted ? "device.svg" : "device_inverted.svg") : cloudDriveModel.getCloudIcon(type)
                    width: 48
                    height: 48
                    fillMode: Image.PreserveAspectFit
                    anchors.verticalCenter: parent.verticalCenter
                }
                Column {
                    width: parent.width - cloudIcon.width
                    spacing: 5
                    Text {
                        id: titleText
                        text: title
                        width: parent.width
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 8
                        elide: Text.ElideRight
                        color: (!inverted) ? "white" : "black"
                    }
                    Text {
                        id: subtitleText
                        text: (uid != "" ? ("(" + uid + ") ") : "") + path
                        width: parent.width
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 6
                        elide: Text.ElideRight
                        color: "grey"
                    }
                }
            }

            onPressAndHold: {
                console.debug("bookmarksPage listItem onPressAndHold index " + index);
                listView.currentIndex = index;
                listView.highlightFollowsCurrentItem = true;
                itemMenu.index = index;
                itemMenu.open();
            }

            onClicked: {
                console.debug("bookmarksPage listItem onClicked index " + index);
                // Pop bookmarksPage if there are 2 pages under it. Otherwise just replace with selected bookmark target.
                if (pageStack.depth > 2) {
                    pageStack.pop();
                }
                if (type < 0) {
                    // Local bookmark.
                    pageStack.replace(Qt.resolvedUrl("FolderPage.qml"), {}, true);
                    fsModel.currentDir = path;
                } else {
                    // Cloud bookmark.
                    pageStack.replace(Qt.resolvedUrl("CloudFolderPage.qml"), {
                                       remotePath: "",
                                       remoteParentPath: path,
                                       caller: "bookmarksPage listItem onClicked",
                                       operation: CloudDriveModel.Browse,
                                       localPath: "",
                                       selectedCloudType: type,
                                       selectedUid: uid,
                                       selectedModelIndex: -1
                                   }, true);
                }
            }
        }
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            bookmarksModel.refresh();
        }
    }
}
