import QtQuick 1.1
import com.nokia.symbian 1.1

CommonDialog {
    id: filePropertiesDIalog
    z: 2
    height: contentColumn.height + 120 // Workaround for Symbian only.

    property int selectedIndex
    property variant selectedItem
//    property string absolutePath
//    property string absolutePathName
    property int selectedCloudType: -1
    property string selectedUid
//    property string parentPath

    titleText: selectedItem ? (appInfo.emptyStr+qsTr("%1 properties").arg(selectedItem.name)) : ""
    titleIcon: "FilesPlusIcon.svg"
    buttonTexts: [appInfo.emptyStr+qsTr("OK"), appInfo.emptyStr+qsTr("Cancel")]
    content: Item {
        id: contentItem

        property int labelWidth: width * 0.35
        property real maxContentHeight: screen.height - (inputContext.visible ? inputContext.height : 0) - 110 // Title + Buttons height. For Symbian only.

        onHeightChanged: {
            console.debug("filePropertiesDIalog contentItem onHeightChanged " + height + " maxContentHeight " + maxContentHeight + " inputContext.height " + inputContext.height + " filePropertiesDIalog.width " + filePropertiesDIalog.width + " filePropertiesDIalog.height " + filePropertiesDIalog.height + " window.inPortrait " + window.inPortrait);
        }

        width: parent.width - 20 // For Symbian only.
        height: (window.inPortrait) ? Math.min(250, maxContentHeight) : Math.min(180, maxContentHeight) // For Symbian only.
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
                    Label {
                        text: appInfo.emptyStr+qsTr("Name")
                        width: contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Label {
                        text: selectedItem ? selectedItem.name : ""
                        width: parent.width - contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    width: parent.width
                    Label {
                        text: appInfo.emptyStr+qsTr("Path")
                        width: contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Label {
                        text: selectedItem ? selectedItem.absolutePath : ""
                        width: parent.width - contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    width: parent.width
                    Label {
                        text: appInfo.emptyStr+qsTr("Size")
                        width: contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Label {
                        text: selectedItem ? selectedItem.size : 0
                        width: parent.width - contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    width: parent.width
                    Label {
                        text: appInfo.emptyStr+qsTr("Last modified")
                        width: contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Label {
                        text: selectedItem ? Qt.formatDateTime(selectedItem.lastModified, "d MMM yyyy h:mm:ss ap") : ""
                        width: parent.width - contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    width: parent.width
                    Label {
                        text: appInfo.emptyStr+qsTr("contents")
                        width: contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Label {
                        function getText(subDirCount, subFileCount) {
                            var sub = "";
                            sub += (subDirCount > 0) ? qsTr("%n dir(s)", "", subDirCount) : "";
                            sub += (subFileCount > 0) ? (((sub == "") ? "" : " ")+qsTr("%n file(s)", "", subFileCount)) : "";
                            return sub;
                        }

                        text: selectedItem ? (appInfo.emptyStr+getText(selectedItem.subDirCount, selectedItem.subFileCount)) : "";
                        width: parent.width - contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    width: parent.width
                    Label {
                        text: appInfo.emptyStr+qsTr("connections")
                        width: contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Label {
                        text: "TBD"
                        width: parent.width - contentItem.labelWidth
                        color: "white"
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }
    }
}
