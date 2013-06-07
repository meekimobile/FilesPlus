import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

Item {
    id: gridItem
    
    property string fileName: name
    property string filePath: absolutePath
    property string gridViewState: ""
    property alias gridItemIconSource: gridItemIcon.source
    property alias gridItemIconAsynchronous: gridItemIcon.asynchronous
    property bool gridItemIconBusyVisible: false
    property alias actionIconSource: cutCopyIcon.source
    property alias runningIconSource: runningIcon.source
    property alias runningIconVisible: runningIcon.visible
    property alias syncIconSource: syncIcon.source
    property alias syncIconVisible: syncIcon.visible
    property bool inverted: window.platformInverted
    property bool omitShowingZeroSizeDir: false
    property variant viewableImageFileTypes: ["JPG", "PNG", "SVG", "GIF"]
    property variant viewableTextFileTypes: ["TXT", "HTML", "LOG", "CSV", "CONF", "INI"]
    property bool showPreview: (viewableImageFileTypes.indexOf(fileType.toUpperCase()) != -1)
    property bool isImageUrlCachable: false
    property real subIconMargin: appInfo.emptySetting + (appInfo.getSettingBoolValue("GridView.compact.enabled", false) ? 10 : 10) // For Symbian only. 10 for 3 columns, 10 for 4 columns

    signal pressAndHold(variant mouse)
    signal clicked(variant mouse)
    signal listItemIconError()
    
    function getIconSource(timestamp) {
        if (isDir) {
            return "folder_list.svg";
        } else if (viewableImageFileTypes.indexOf(fileType.toUpperCase()) != -1) {
            if (fileType.toUpperCase() == "SVG") {
                return "file://" + absolutePath
            } else {
                return "image://local/" + absolutePath;
            }
        } else {
            return "notes_list.svg";
        }
    }
    
    states: [
        State {
            name: "preview"
            when: showPreview
            PropertyChanges {
                target: gridItemIcon
                explicit: true
                fillMode: Image.PreserveAspectCrop
                width: parent.width - 2
                height: parent.height - 2
            }
            PropertyChanges {
                target: gridItem
                explicit: true
                subIconMargin: 2
            }
        },
        State {
            name: "icon"
            when: !showPreview
            PropertyChanges {
                target: gridItemIcon
                explicit: true
                fillMode: Image.PreserveAspectFit
                width: parent.width - 30
                height: parent.width - 30
                sourceSize.width: parent.width - 30
                sourceSize.height: parent.width - 30
            }
        }
    ]

    Image {
        id: gridItemIcon
        source: appInfo.emptySetting+getIconSource((new Date()).getTime())
        sourceSize.width: isImageUrlCachable ? 128 : undefined // Expected size of thumbnail128 from cloud service.
        sourceSize.height: isImageUrlCachable ? 128 : undefined
        anchors.centerIn: parent
        asynchronous: true
        smooth: false
        clip: true
        cache: !showPreview
        
        BusyIndicator {
            visible: gridItemIconBusyVisible && (parent.status == Image.Loading || parent.status == Image.Error)
            running: visible
            anchors.centerIn: parent
        }

        Text {
            id: gridItemFileType
            text: fileType.toUpperCase()
            font.pointSize: 8
            font.bold: true
            color: "black"
            style: Text.Outline
            styleColor: "white"
            anchors.centerIn: parent
            visible: !showPreview && !isDir
        }

        onStatusChanged: {
            if (status == Image.Error) {
                gridItem.listItemIconError();
            }
        }
    }

    Image {
        id: cutCopyIcon
        z: 1
        width: 32
        height: 32
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: subIconMargin
        visible: (source != "" && !isChecked)
        source: (clipboard.count > 0) ? appInfo.emptySetting+clipboard.getActionIcon(absolutePath) : ""
    }

    Image {
        id: markIcon
        z: 1
        width: 32
        height: 32
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: subIconMargin
        visible: (gridViewState == "mark" && isChecked)
        source: (!inverted) ? "ok.svg" : "ok_inverted.svg"
    }

    Image {
        id: syncIcon
        z: 1
        width: 32
        height: 32
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: subIconMargin
        fillMode: Image.PreserveAspectFit
        visible: (source != "")
        opacity: showPreview ? 0.5 : 1
    }

    Image {
        id: runningIcon
        z: 1
        width: 32
        height: 32
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: subIconMargin
        fillMode: Image.PreserveAspectFit
        source: (!inverted) ? "refresh.svg" : "refresh_inverted.svg"
        visible: (source != "" && isRunning)
    }

    Text {
        id: gridItemLabel
        text: name
        font.pointSize: 6
        color: (!inverted) ? "white" : "black"
        width: parent.width - 2
        z: 1
        horizontalAlignment: Text.AlignHCenter
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        elide: Text.ElideRight
    }

    MouseArea {
        anchors.fill: parent
        onPressed: {
            gridItem.GridView.view.currentIndex = index;
        }
        onClicked: {
            gridItem.clicked(mouse);
        }
        onPressAndHold: {
            gridItem.pressAndHold(mouse);
        }
    }
}
