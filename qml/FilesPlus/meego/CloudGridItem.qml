import QtQuick 1.1
import com.nokia.meego 1.0
import "Utility.js" as Utility

Item {
    id: gridItem
    
    property string fileName: name
    property string filePath: absolutePath
    property string gridViewState: ""
    property alias gridItemIconSource: gridItemIcon.source
    property alias gridItemIconAsynchronous: gridItemIcon.asynchronous
    property bool gridItemIconBusyVisible: false
    //            property alias actionIconSource: cutCopyIcon.source
    //            property alias runningIconSource: runningIcon.source
    //            property alias syncIconSource: syncIcon.source
    //            property alias syncIconVisible: syncIcon.visible
    property bool inverted: !theme.inverted
    property bool omitShowingZeroSizeDir: false
    property variant viewableImageFileTypes: ["JPG", "PNG", "SVG"]

    signal pressAndHold(variant mouse)
    signal clicked(variant mouse)
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
    
    function getImageSource(url, timestamp) {
        if (selectedCloudType != -1) {
            if (cloudDriveModel.isImageUrlCachable(selectedCloudType) && (fileType.toUpperCase() != "SVG")) {
                // Cache only cloud, cachable and not-SVG image.
                return "image://remote/" + url + "#t=" + timestamp;
            } else {
                return url + "#t=" + timestamp;
            }
        } else {
            // Local image.
            return url + "#t=" + timestamp;
        }
    }
    
    Image {
        id: gridItemIcon
        source: getIconSource((new Date()).getTime())
        asynchronous: true
        anchors.fill: parent
        anchors.margins: 1
        fillMode: Image.PreserveAspectCrop
        smooth: true
        clip: true
        
        BusyIndicator {
            visible: gridItemIconBusyVisible && (parent.status == Image.Loading || parent.status == Image.Error)
            running: visible
            platformStyle: BusyIndicatorStyle { size: "medium" }
            anchors.centerIn: parent
        }

        onStatusChanged: {
            if (status == Image.Error) {
                gridItem.listItemIconError();
            }
        }
    }

    Text {
        id: gridItemLabel
        text: name
        font.pointSize: 18
        font.italic: true
        color: "black"
        width: parent.width
        z: 1
        anchors.fill: parent
        anchors.margins: 10
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignBottom
        elide: Text.ElideRight
        style: Text.Outline
        styleColor: "white"
        visible: (viewableImageFileTypes.indexOf(fileType.toUpperCase()) == -1)
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            gridItem.clicked(mouse);
        }
        onPressAndHold: {
            gridItem.pressAndHold(mouse);
        }
    }
}
