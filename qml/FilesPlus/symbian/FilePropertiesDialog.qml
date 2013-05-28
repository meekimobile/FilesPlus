import QtQuick 1.1
import com.nokia.symbian 1.1
import FolderSizeItemListModel 1.0

CommonDialog {
    id: filePropertiesDIalog
    z: 2
    height: contentItem.height + 130 // Workaround for Symbian only.

    property int selectedIndex
    property variant selectedItem
    property alias cloudItemModel: cloudItemModel
    property bool isCloudFolder: false
    property bool showAttributes: false
    property alias isHidden: toggleHiddenCheckbox.checked
    property alias isReadOnly: toggleReadOnlyCheckbox.checked

    signal opening()
    signal opened()
    signal closing()
    signal closed()
    signal syncAll()
    signal syncAdd()
    signal disconnect(int type, string uid, string absolutePath)
    signal toggleHidden(string absolutePath, bool value)
    signal toggleReadOnly(string absolutePath, bool value)

    onStatusChanged: {
        if (status == DialogStatus.Opening) {
            opening();
        } else if (status == DialogStatus.Open) {
            opened();
        } else if (status == DialogStatus.Closing) {
            closing();
        } else if (status == DialogStatus.Closed) {
            closed();
        }
    }

    titleText: selectedItem ? (appInfo.emptyStr+selectedItem.name+" "+qsTr("Information")) : ""
    titleIcon: "FilesPlusIcon.svg"
    buttonTexts: [appInfo.emptyStr+qsTr("Close")]
    content: Item {
        id: contentItem

        property int labelWidth: width * 0.35
        property real maxContentHeight: screen.height - (inputContext.visible ? inputContext.height : 0) - 130 // Title + Buttons height = 130. For Symbian only.

        onHeightChanged: {
            console.debug("filePropertiesDIalog contentItem onHeightChanged " + height + " maxContentHeight " + maxContentHeight + " inputContext.height " + inputContext.height + " filePropertiesDIalog.width " + filePropertiesDIalog.width + " filePropertiesDIalog.height " + filePropertiesDIalog.height + " window.inPortrait " + window.inPortrait);
        }

        width: parent.width - 20 // For Symbian only.
        height: Math.min(contentColumn.height, maxContentHeight) // For Symbian only.
        anchors.horizontalCenter: parent.horizontalCenter

        Flickable {
            id: contentFlick
            anchors.fill: parent
            contentWidth: contentColumn.width
            contentHeight: contentColumn.height
            flickableDirection: Flickable.VerticalFlick
            clip: true

            Column {
                id: contentColumn
                width: contentItem.width
                spacing: 5

                Row {
                    width: parent.width
                    Text {
                        text: appInfo.emptyStr+qsTr("Name")
                        font.pointSize: 6
                        width: contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: selectedItem ? selectedItem.name : ""
                        font.pointSize: 6
                        width: parent.width - contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    width: parent.width
                    Text {
                        text: appInfo.emptyStr+qsTr("Path")
                        font.pointSize: 6
                        width: contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: selectedItem ? selectedItem.absolutePath : ""
                        font.pointSize: 6
                        width: parent.width - contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    width: parent.width
                    Text {
                        text: appInfo.emptyStr+qsTr("Size")
                        font.pointSize: 6
                        width: contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: selectedItem ? selectedItem.size : 0
                        font.pointSize: 6
                        width: parent.width - contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    width: parent.width
                    Text {
                        text: appInfo.emptyStr+qsTr("Last modified")
                        font.pointSize: 6
                        width: contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: selectedItem ? Qt.formatDateTime(selectedItem.lastModified, "d MMM yyyy h:mm:ss ap") : ""
                        font.pointSize: 6
                        width: parent.width - contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    width: parent.width
                    spacing: 3
                    visible: showAttributes
                    Text {
                        text: appInfo.emptyStr+qsTr("Attributes")
                        font.pointSize: 6
                        width: contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Button {
                        id: toggleHiddenCheckbox
                        width: (parent.width - contentItem.labelWidth - parent.spacing*2) / 2
                        text: appInfo.emptyStr+qsTr("Hidden")
                        checkable: true
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: {
                            if (selectedItem) {
                                toggleHidden(selectedItem.absolutePath, toggleHiddenCheckbox.checked);
                            }
                        }
                    }
                    Button {
                        id: toggleReadOnlyCheckbox
                        width: (parent.width - contentItem.labelWidth - parent.spacing*2) / 2
                        text: appInfo.emptyStr+qsTr("Read Only")
                        checkable: true
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: {
                            if (selectedItem) {
                                toggleReadOnly(selectedItem.absolutePath, toggleReadOnlyCheckbox.checked);
                            }
                        }
                    }
                }
                Row {
                    width: parent.width
                    visible: {
                        if (selectedItem) {
                            return (selectedItem.subDirCount + selectedItem.subFileCount) > 0;
                        } else {
                            return false;
                        }
                    }
                    Text {
                        text: appInfo.emptyStr+qsTr("Contents")
                        font.pointSize: 6
                        width: contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        function getText(subDirCount, subFileCount) {
                            var sub = "";
                            sub += (subDirCount > 0) ? qsTr("%n dir(s)", "", subDirCount) : "";
                            sub += (subFileCount > 0) ? (((sub == "") ? "" : " ")+qsTr("%n file(s)", "", subFileCount)) : "";
                            return sub;
                        }

                        text: selectedItem ? (appInfo.emptyStr+getText(selectedItem.subDirCount, selectedItem.subFileCount)) : "";
                        font.pointSize: 6
                        width: parent.width - contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    width: parent.width
                    spacing: 5
                    visible: cloudItemModel.count > 0 || !isCloudFolder
                    Text {
                        text: appInfo.emptyStr+qsTr("Connected items")
                        font.pointSize: 6
                        width: parent.width - (syncAllButton.visible ? (syncAllButton.width + parent.spacing) : 0) - (syncAddButton.visible ? (syncAddButton.width + parent.spacing) : 0)
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Button {
                        id: syncAddButton
                        width: 50
                        height: 50
                        iconSource: "cloud_add.svg"
                        anchors.verticalCenter: parent.verticalCenter
                        visible: !isCloudFolder
                        onClicked: {
                            syncAdd();
                        }
                    }
                    Button {
                        id: syncAllButton
                        width: 50
                        height: 50
                        iconSource: "cloud.svg"
                        anchors.verticalCenter: parent.verticalCenter
                        visible: cloudItemModel.count > 0
                        onClicked: {
                            syncAll();
                        }
                    }
                }
                Column {
                    width: parent.width
                    spacing: 5
                    Repeater {
                        model: ListModel { id: cloudItemModel }
                        delegate: cloudItemDelegate
                    }
                }
            }
        }
    }

    Component {
        id: cloudItemDelegate

        Row {
            width: parent.width
            spacing: 5
            Image {
                id: cloudIcon
                anchors.verticalCenter: parent.verticalCenter
                width: 30
                height: !isCloudFolder ? 30 : 0
                source: !isCloudFolder ? cloudDriveModel.getCloudIcon(type) : ""
                visible: !isCloudFolder
            }
            Column {
                width: parent.width - (cloudIcon.visible ? (cloudIcon.width + parent.spacing) : 0) - (disconnectButton.width + parent.spacing)
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    width: parent.width
                    text: !isCloudFolder ? email : ""
                    font.pointSize: 6
                    color: "white"
                    elide: Text.ElideRight
                    visible: !isCloudFolder
                }
                Text {
                    width: parent.width
                    text: absolutePath
                    font.pointSize: 6
                    color: "white"
                    elide: Text.ElideRight
                }
            }
            Button {
                id: disconnectButton
                width: 50
                height: 50
                iconSource: "cloud_disconnect.svg"
                anchors.verticalCenter: parent.verticalCenter
                onClicked: {
                    if (isCloudFolder) {
                        disconnect(selectedCloudType, selectedUid, absolutePath);
                    } else {
                        disconnect(type, uid, absolutePath);
                    }
                }
            }
        }
    }
}
