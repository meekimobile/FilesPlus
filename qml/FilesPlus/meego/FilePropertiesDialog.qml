import QtQuick 1.1
import com.nokia.meego 1.0

CommonDialog {
    id: filePropertiesDIalog
    z: 2

    property int selectedIndex
    property variant selectedItem
    property alias cloudItemModel: cloudItemModel

    signal opening()
    signal opened()
    signal closing()
    signal closed()
    signal sync(int type, string uid, string absolutePath)
    signal syncAll()

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
        property real maxContentHeight: filePropertiesDIalog.parent.height - 130 // Title + Buttons height = 130. For Meego only.

        onHeightChanged: {
            console.debug("filePropertiesDIalog contentItem onHeightChanged " + height + " maxContentHeight " + maxContentHeight + " filePropertiesDIalog.width " + filePropertiesDIalog.width + " filePropertiesDIalog.height " + filePropertiesDIalog.height + " window.inPortrait " + window.inPortrait);
        }

        width: parent.width // For Meego only.
        height: Math.min(contentColumn.height, maxContentHeight) // For Meego only.
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
                        font.pointSize: 16
                        width: contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: selectedItem ? selectedItem.name : ""
                        font.pointSize: 16
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
                        font.pointSize: 16
                        width: contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: selectedItem ? selectedItem.absolutePath : ""
                        font.pointSize: 16
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
                        font.pointSize: 16
                        width: contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: selectedItem ? selectedItem.size : 0
                        font.pointSize: 16
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
                        font.pointSize: 16
                        width: contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: selectedItem ? Qt.formatDateTime(selectedItem.lastModified, "d MMM yyyy h:mm:ss ap") : ""
                        font.pointSize: 16
                        width: parent.width - contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    width: parent.width
                    Text {
                        text: appInfo.emptyStr+qsTr("Contents")
                        font.pointSize: 16
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
                        font.pointSize: 16
                        width: parent.width - contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    width: parent.width
                    visible: cloudItemModel.count > 0
                    Text {
                        text: appInfo.emptyStr+qsTr("Connected items")
                        font.pointSize: 16
                        width: parent.width - syncAllButton.width
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Button {
                        id: syncAllButton
                        text: appInfo.emptyStr+qsTr("Sync all")
                        anchors.verticalCenter: parent.verticalCenter
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
                height: 30
                source: cloudDriveModel.getCloudIcon(type)
            }
            Column {
                width: parent.width - (parent.spacing * 2)- cloudIcon.width - syncButton.width
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    text: email
                    font.pointSize: 16
                    width: parent.width
                    color: "white"
                    elide: Text.ElideRight
                }
                Text {
                    text: absolutePath
                    font.pointSize: 16
                    width: parent.width
                    color: "white"
                    elide: Text.ElideRight
                }
            }
            Button {
                id: syncButton
                iconSource: "cloud.svg"
                anchors.verticalCenter: parent.verticalCenter
                onClicked: {
                    sync(type, uid, absolutePath);
                }
            }
        }
    }
}
