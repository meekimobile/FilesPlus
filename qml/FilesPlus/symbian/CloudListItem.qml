import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

ListItem {
    id: listItem
    height: appInfo.emptySetting+(appInfo.getSettingBoolValue("listItem.compact.enabled", false) ? 60 : 70)

    property int mouseX
    property int mouseY
    property bool pressed: mode == "pressed"

    property string fileName: name
    property string filePath: absolutePath
    property string listViewState: ""
    property alias listItemIconSource: listItemIcon.source
    property alias listItemIconAsynchronous: listItemIcon.asynchronous
    property bool listItemIconBusyVisible: false
    property alias actionIconSource: cutCopyIcon.source
    property alias runningIconSource: runningIcon.source
    property alias syncIconSource: syncIcon.source
    property alias syncIconVisible: syncIcon.visible
    property bool inverted: window.platformInverted
    property bool omitShowingZeroSizeDir: false
    property variant viewableImageFileTypes: ["JPG", "PNG", "SVG"]
    property bool showPreview: (viewableImageFileTypes.indexOf(fileType.toUpperCase()) != -1)


    signal listItemIconError()

    function getIconSource(timestamp) {
        var viewableImageFileTypes = ["JPG", "PNG", "SVG"];
        var viewableTextFileTypes = ["TXT", "HTML"];
        
        if (isDir) {
            return "folder_list.svg";
        } else if (viewableImageFileTypes.indexOf(fileType.toUpperCase()) != -1) {
            var showThumbnail = appInfo.getSettingBoolValue("thumbnail.enabled", false);
            if (showThumbnail) {
                if (fileType.toUpperCase() == "SVG") {
                    return "file://" + absolutePath
                } else {
                    return "image://local/" + absolutePath;
                }
            } else {
                return "photos_list.svg";
            }
        } else {
            return "notes_list.svg";
        }
    }

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
                id: cutCopyIcon
                z: 1
                width: 32
                height: 32
                visible: (source != "")
                source: (clipboard.count > 0) ? appInfo.emptySetting+clipboard.getActionIcon(absolutePath) : ""
            }
            Image {
                id: markIcon
                z: 1
                width: 32
                height: 32
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                visible: (listViewState == "mark" && isChecked)
                source: (!inverted) ? "ok.svg" : "ok_inverted.svg"
            }
            Image {
                id: listItemIcon
                asynchronous: true
                sourceSize.width: (fileType.toUpperCase() == "SVG") ? undefined : 48
                sourceSize.height: (fileType.toUpperCase() == "SVG") ? undefined : 48
                width: 48
                height: 48
                fillMode: Image.PreserveAspectFit
                anchors.centerIn: parent
                source: appInfo.emptySetting+listItem.getIconSource((new Date()).getTime())

                BusyIndicator {
                    visible: listItemIconBusyVisible && (parent.status == Image.Loading || parent.status == Image.Error)
                    running: visible
                    width: parent.width
                    height: parent.height
                    anchors.centerIn: parent
                }

                Text {
                    id: listItemFileType
                    text: fileType.toUpperCase()
                    font.pointSize: 6
                    font.bold: true
                    color: "black"
                    style: Text.Outline
                    styleColor: "white"
                    anchors.centerIn: parent
                    visible: !showPreview && !isDir
                }

                onStatusChanged: {
                    if (status == Image.Error) {
                        listItemIconError();
                    }
                }
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
                    font.pointSize: 8
                    elide: Text.ElideMiddle
                    verticalAlignment: Text.AlignVCenter
                    color: (!inverted) ? "white" : "black"
                }
                Text {
                    id: sizeText
                    text: (omitShowingZeroSizeDir && size == 0) ? "" : Utility.formatFileSize(size, 1)
                    height: parent.height
                    font.pointSize: 6
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    color: (!inverted) ? "white" : "black"
                }
            }
            Row {
                width: parent.width
                height: parent.height / 2
                spacing: 2
                Rectangle {
                    width: parent.width - syncIcon.width - parent.spacing
                    height: parent.height
                    color: "transparent"
                    Text {
                        id: listItemSubTitle
                        text: {
                            var sub = appInfo.emptyStr+Qt.formatDateTime(lastModified, "d MMM yyyy h:mm:ss ap");
                            if (subDirCount > 0) sub += ", "+qsTr("%n dir(s)", "", subDirCount);
                            if (subFileCount > 0) sub += ", "+qsTr("%n file(s)", "", subFileCount);

                            return sub;
                        }
                        width: parent.width
                        height: parent.height
                        font.pointSize: 6
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        visible: !isRunning
                        color: (!inverted) ? "#C0C0C0" : "#202020"
                    }
                    Row {
                        width: parent.width
                        height: parent.height
                        visible: isRunning
                        anchors.verticalCenter: parent.verticalCenter
                        Image {
                            id: runningIcon
                            width: 24
                            height: 24
                            anchors.verticalCenter: parent.verticalCenter
                            source: (!inverted) ? "refresh.svg" : "refresh_inverted.svg"
                        }
                        ProgressBar {
                            id: syncProgressBar
                            width: parent.width - runningIcon.width
                            anchors.verticalCenter: parent.verticalCenter
                            indeterminate: isRunning && (isDir || (runningValue == runningMaxValue))
                            value: runningValue
                            maximumValue: runningMaxValue
                        }
                    }
                }
                Image {
                    id: syncIcon
                    width: (visible) ? 30 : 0
                    height: parent.height
                    fillMode: Image.PreserveAspectFit
                    z: 1
                    visible: cloudDriveModel.isConnected(absolutePath) || cloudDriveModel.isSyncing(absolutePath);
                    source: {
                        if (cloudDriveModel.isSyncing(absolutePath)) {
                            return "cloud_wait.svg";
                        } else if (cloudDriveModel.isDirty(absolutePath, lastModified)) {
                            return "cloud_dirty.svg";
                        } else {
                            return "cloud.svg";
                        }
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onPressed: {
            parent.mouseX = mouseX;
            parent.mouseY = mouseY;
            mouse.accepted = false;
        }
    }
}
