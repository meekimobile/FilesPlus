import QtQuick 1.1
import com.nokia.meego 1.0
import CloudDriveModel 1.0
import BookmarksModel 1.0
import "Utility.js" as Utility

Page {
    id: bookmarksPage

    property string name: "bookmarksPage"
    property bool inverted: !theme.inverted

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
                listModel.refresh();
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

            // TODO Alias for fixing incorrect children.
            default property alias children: itemMenuLayout.menuChildren

            MenuItemWithIcon {
                name: "edit"
                text: appInfo.emptyStr+qsTr("Edit")
                onClicked: {
                    renameDialog.index = itemMenu.index;
                    renameDialog.open();
                }
            }

            MenuItemWithIcon {
                name: "remote"
                text: appInfo.emptyStr+qsTr("Remove")
                onClicked: {
                    listModel.deleteRow(itemMenu.index);
                    listModel.save();
                }
            }
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
                    source: cloudDriveModel.getCloudIcon(renameDialog.type)
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
                        font.pointSize: 18
                        elide: Text.ElideMiddle
                        color: (!inverted) ? "white" : "black"
                    }
                    Text {
                        id: subtitleText
                        text: renameDialog.path
                        width: parent.width
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 16
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
            var item = listModel.get(index);
            type = item.type;
            uid = item.uid;
            path = item.path;
            title = item.title;
        }
        onConfirm: {
            console.debug("bookmarksPage renameDialog onConfirm " + newTitle.text);
            if (newTitle.text.trim() == "") return;

            listModel.setProperty(index, "title", newTitle.text.trim());
            listModel.save();
        }
    }

    BookmarksModel {
        id: listModel
    }

    ListView {
        id: listView
        width: parent.width
        height: parent.height - titlePanel.height
        anchors.top: titlePanel.bottom
        delegate: listItemDelegate
        clip: true
        model: listModel

        onMovementStarted: {
            if (currentItem) {
                currentItem.pressed = false;
            }
        }
    }

    Component {
        id: listItemDelegate

        ListItem {
            id: listItem
            height: 80
            Row {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 5

                Image {
                    id: cloudIcon
                    source: cloudDriveModel.getCloudIcon(type)
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
                        font.pointSize: 18
                        elide: Text.ElideMiddle
                        color: (!inverted) ? "white" : "black"
                    }
                    Text {
                        id: subtitleText
                        text: path
                        width: parent.width
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 16
                        color: "grey"
                    }
                }
            }

            onPressAndHold: {
                console.debug("bookmarksPage listItem onPressAndHold index " + index);
                itemMenu.index = index;
                itemMenu.open();
            }

            onClicked: {
                console.debug("bookmarksPage listItem onClicked index " + index);
                if (type < 0) {
                    // Local bookmark.
                    // TODO pop and replace FolderPage.
                    pageStack.pop();
                    fsModel.currentDir = path;
                    pageStack.push(Qt.resolvedUrl("FolderPage.qml"));
                } else {
                    // Cloud bookmark.
                    // TODO pop and replace CloudFolderPage.
                    pageStack.pop();
                    pageStack.push(Qt.resolvedUrl("CloudFolderPage.qml"), {
                                       remotePath: "",
                                       remoteParentPath: path,
                                       caller: "bookmarksPage listItem onClicked",
                                       operation: CloudDriveModel.Browse,
                                       localPath: "",
                                       selectedCloudType: type,
                                       selectedUid: uid,
                                       selectedModelIndex: -1
                                   });
                }
            }
        }
    }
}
