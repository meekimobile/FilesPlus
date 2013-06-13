import QtQuick 1.1
import com.nokia.meego 1.0
import "Utility.js" as Utility

Page {
    id: compressedFolderPage

    property string name: "compressedFolderPage"
    property string compressedFilePath: ""
    property string compressedFileName: ""
    property bool inverted: !theme.inverted

    Component.onCompleted: {
        compressedFolderModel.list(compressedFilePath);
    }

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
            id: actionButton
            buttonIconSource: !inverted ? "ok.svg" : "ok_inverted.svg"
            onClicked: {
                if (markMenu.isMarkAll) {
                    compressedFolderModel.unmarkAll();
                } else {
                    compressedFolderModel.markAll();
                }
                markMenu.isMarkAll = !markMenu.isMarkAll;
            }
        }

        ToolBarButton {
            id: menuButton
            buttonIconSource: "toolbar-view-menu"
            onClicked: {
                if (!compressedFolderModel.isAnyItemChecked()) {
                    markAllMenu.open();
                } else {
                    markMenu.open();
                }
            }
        }
    }

    MenuWithIcon {
        id: markMenu
        z: 2

        property bool isMarkAll: false

        onStatusChanged: {
            if (status == DialogStatus.Opening) {
                isMarkAll = !compressedFolderModel.areAllItemChecked();
            }
        }

        content: MenuLayout {
            id: markMenuLayout

            // TODO Alias for fixing incorrect children.
            default property alias children: markMenuLayout.menuChildren

            MenuItemWithIcon {
                id: toggleMarkAll
                name: "toggleMarkAll"
                text: (markMenu.isMarkAll) ? appInfo.emptyStr+qsTr("Mark all") : appInfo.emptyStr+qsTr("Unmark all")
                onClicked: {
                    if (markMenu.isMarkAll) {
                        compressedFolderModel.markAll();
                    } else {
                        compressedFolderModel.unmarkAll();
                    }
                    markMenu.isMarkAll = !markMenu.isMarkAll;
                }
            }

            MenuItemWithIcon {
                id: copyMarked
                name: "copyMarked"
                text: appInfo.emptyStr+qsTr("Copy marked items")
                onClicked: {
                    copyMarkedItems();
                }
            }
        }
    }

    MarkAllMenu {
        id: markAllMenu

        onMarkAll: {
            compressedFolderModel.markAll();
        }
        onMarkAllFiles: {
            compressedFolderModel.markAllFiles();
        }
        onMarkAllFolders: {
            compressedFolderModel.markAllFolders();
        }
    }

    function copyMarkedItems() {
        // Clear as there should be only extract action.
        clipboard.clear();

        var extractFileList = [];
        var listIndex = 0;
        for (var i=0; i<compressedFolderModel.count; i++) {
            var item = compressedFolderModel.get(i);
            if (item.isChecked) {
                console.debug("compressedFolderPage copyMarkedItems item"
                              + " compressedFilePath " + compressedFilePath
                              + " name " + item.name
                              + " isChecked " + item.isChecked);

//                extractFileList = (extractFileList == "" ? "" : ",") + "\"" + item.name + "\"";
                extractFileList[listIndex++] = item.name;
            }

            // Reset isChecked.
            compressedFolderModel.setProperty(i, "isChecked", false);
        }

        clipboard.addItemWithSuppressCountChanged({ "action": "extract",
                                                      "sourcePath": compressedFilePath,
                                                      "sourcePathName": compressedFileName,
                                                      "extractFileList": extractFileList });

        // Emit suppressed signal.
        clipboard.emitCountChanged();

        // Pop once copying is done.
        pageStack.pop();
    }

    ListView {
        id: listView
        width: parent.width
        height: parent.height - currentPath.height
        anchors.top: currentPath.bottom
        highlightRangeMode: ListView.NoHighlightRange
        highlightFollowsCurrentItem: true
        highlightMoveDuration: 1
        highlightMoveSpeed: 4000
        highlight: Rectangle {
            width: listView.width
            gradient: highlightGradient
        }
        clip: true
        focus: true
        cacheBuffer: height * 2
        pressDelay: 200
        model: compressedFolderModel
        delegate: listItemDelegate

        onMovementStarted: {
            if (currentItem) {
                currentIndex = -1;
            }
        }

        ScrollDecorator {
            id: scrollbar
            flickableItem: listView
        }
    }

    Gradient {
        id: highlightGradient
        GradientStop {
            position: 0
            color: "#0080D0"
        }

        GradientStop {
            position: 1
            color: "#53A3E6"
        }
    }

    Component {
        id: listItemDelegate

        ListItem {
            id: listItem
            height: appInfo.emptySetting+(appInfo.getSettingBoolValue("listItem.compact.enabled", false) ? 60 : 80)

            Row {
                id: listDelegateRow
                anchors.fill: parent
                anchors.margins: 4
                Rectangle {
                    id: iconRect
                    width: 60
                    height: parent.height
                    color: "transparent"
                    Image {
                        id: markIcon
                        z: 1
                        width: 32
                        height: 32
                        anchors.left: parent.left
                        anchors.bottom: parent.bottom
                        visible: isChecked
                        source: (!inverted) ? "ok.svg" : "ok_inverted.svg"
                    }
                    Image {
                        id: listItemIcon
                        width: 48
                        height: 48
                        fillMode: Image.PreserveAspectFit
                        anchors.centerIn: parent
                        source: "notes_list.svg"
                        asynchronous: true
                        smooth: false
                        clip: true
                    }
                }
                Column {
                    width: parent.width - iconRect.width
                    height: parent.height

                    Row {
                        width: parent.width
                        height: parent.height / 2
                        spacing: 2
                        Text {
                            text: name
                            width: parent.width - sizeText.width - parent.spacing
                            height: parent.height
                            font.pointSize: 18
                            elide: Text.ElideMiddle
                            verticalAlignment: Text.AlignVCenter
                            color: (!inverted) ? "white" : "black"
                        }
                        Text {
                            id: sizeText
                            text: (size == 0) ? "" : Utility.formatFileSize(size, 1)
                            height: parent.height
                            font.pointSize: 16
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                            color: (!inverted) ? "white" : "black"
                        }
                    }
                    Row {
                        width: parent.width
                        height: parent.height / 2
                        spacing: 2
                        Text {
                            id: listItemSubTitle
                            text: appInfo.emptyStr+Qt.formatDateTime(lastModified, "d MMM yyyy h:mm:ss ap")
                            width: parent.width
                            height: parent.height
                            font.pointSize: 16
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                            color: (!inverted) ? "#C0C0C0" : "#202020"
                        }
                    }
                }
            }

            onClicked: {
                compressedFolderModel.setProperty(index, "isChecked", !isChecked);
            }
        }
    }

    TitlePanel {
        id: currentPath
        text: compressedFileName
        textLeftMargin: height + 5

        Image {
            source: (!inverted) ? "compress.svg" : "compress_inverted.svg"
            sourceSize.width: parent.height
            sourceSize.height: parent.height
            width: parent.height
            height: parent.height
            fillMode: Image.PreserveAspectFit
            anchors.left: parent.left
        }
    }
}
