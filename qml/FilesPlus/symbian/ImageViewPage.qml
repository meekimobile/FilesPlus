import QtQuick 1.1
import com.nokia.symbian 1.1
import CloudDriveModel 1.0
import SystemInfoHelper 1.0
import "Utility.js" as Utility

Page {
    id: imageViewPage
    tools: null

    property string name: "imageViewPage"
    property variant model
    property int selectedCloudType: -1
    property string selectedUid: ""
    property string selectedFilePath: ""
    property int selectedModelIndex: -1
    property bool showGrid: true
    property bool inverted: window.platformInverted
    property bool showTools: true
    property real maxZoomFactor: 3
    property variant viewableImageFileTypes: ["JPG", "PNG", "GIF", "SVG"];
    property bool inPortrait

    state: "grid"
    states: [
        State {
            name: "grid"
            PropertyChanges { target: imageViewPage; showGrid: true; showTools: true }
        },
        State {
            name: "flick"
            PropertyChanges { target: imageViewPage; explicit: true; showGrid: false }
        }
    ]

    ToolBar {
        id: toolBar
        anchors.bottom: parent.bottom
        opacity: 0.5
        z: 2
        visible: showTools
        tools: ToolBarLayout {
            ToolBarButton {
                id: backButton
                buttonIconSource: "toolbar-back"
                onClicked: {
                    pageStack.pop();
                }
            }

            ToolBarButton {
                id: openButton
                buttonIconSource: (!inverted) ? "photos.svg" : "photos_inverted.svg"
                onClicked: {
                    var imageSource;
                    if (selectedCloudType != -1) {
                        imageSource = fetchCurrentPendingSourceUrl();
                        Qt.openUrlExternally(imageSource);
                    } else {
                        imageSource = "file://" + imageGridModel.get(imageGrid.currentIndex).absolutePath;
                        Qt.openUrlExternally(imageSource);
                    }
                }
            }

            ToolBarButton {
                id: deleteButton
                buttonIconSource: (!inverted) ? "delete.svg" : "delete_inverted.svg"
                onClicked: {
                    deleteItemConfirmation.open();
                }
            }

            ToolBarButton {
                id: printButton
                buttonIconSource: (!inverted) ? "print.svg" : "print_inverted.svg"
                onClicked: {
                    var imageSource;
                    if (selectedCloudType != -1) {
                        imageSource = fetchCurrentPendingSourceUrl();
                        gcpClient.printURLSlot(imageSource);
                    } else {
                        imageSource = imageGridModel.get(imageGrid.currentIndex).absolutePath;
                        gcpClient.printFileSlot(imageSource, -1);
                    }
                }
            }
        }
    }

    onStatusChanged: {
        if (status == PageStatus.Active && imageGridModel.count <= 0) {
            // Set once orientation changes is done.
            inPortrait = window.inPortrait;
            // Parse model.
            parseImageGridModel(model);
            // Position at selected image.
            imageGrid.currentIndex = (selectedModelIndex < 0) ? 0 : selectedModelIndex;
            imageGrid.positionViewAtIndex(imageGrid.currentIndex, GridView.Contain);
            // Fetch current item's media URL and pre-load current image.
//            imageFlickView.source = fetchCurrentPendingSourceUrl();
        }
    }

    function orientationChangeSlot() {
        // Signal from main.qml
        imageGrid.positionViewAtIndex(imageGrid.currentIndex, GridView.Contain);
        // Set once orientation changes is done.
        inPortrait = window.inPortrait;
    }

    function parseImageGridModel(model) {
        imageGridModel.clear();

        var timestamp = (new Date()).getTime();

        for (var i = 0; i < model.count; i++) {
            var modelItem = model.get(i);
            modelItem.timestamp = timestamp;
            if (viewableImageFileTypes.indexOf(modelItem.fileType.toUpperCase()) != -1) {
                if (selectedCloudType != -1) {
                    // Cloud image.
                    if (modelItem.source) {
                        // Remote image with source link.
                        modelItem.sourceUrl = modelItem.source;
                        modelItem.previewUrl = modelItem.preview;
                    } else if (modelItem.preview) {
                        // Remote image with preview link but without source link.
                        // NOTE Must not modify otiginal source, to keep original state.
                        // Request media to get URL.
                        modelItem.sourceUrl = ""; // Pending get media URL once pinch zoom, open URL or print URL.
                        modelItem.previewUrl = modelItem.preview;
                    } else {
                        // Remote image without preview link and source link. Ex. SVG image.
                        modelItem.sourceUrl = cloudDriveModel.media(selectedCloudType, selectedUid, modelItem.absolutePath);
                        modelItem.previewUrl = modelItem.sourceUrl;
                    }
                } else if (modelItem.fileType.toUpperCase() == "SVG") {
                    // Local SVG image.
                    modelItem.sourceUrl = helper.getUrl(modelItem.absolutePath);
                    modelItem.previewUrl = helper.getUrl(modelItem.absolutePath);
                } else {
                    // Local image.
                    modelItem.sourceUrl = helper.getUrl(modelItem.absolutePath);
//                    modelItem.previewUrl = helper.getUrl(modelItem.absolutePath); // No cache as file are local.
                    modelItem.previewUrl = "image://local/" + modelItem.absolutePath; // Cache local image.
                }
                imageGridModel.append(modelItem);

                // Find select index.
                if (modelItem.absolutePath == selectedFilePath) {
                    selectedModelIndex = (imageGridModel.count - 1);
                }
            }
        }
    }

    function fetchCurrentPendingSourceUrl() {
        // Fetch media URL.
        var imageSource = imageGridModel.get(imageGrid.currentIndex).sourceUrl;
        if (imageSource == "") {
            imageSource = cloudDriveModel.media(selectedCloudType, selectedUid, imageGridModel.get(imageGrid.currentIndex).absolutePath)
            imageGridModel.setProperty(imageGrid.currentIndex, "sourceUrl", imageSource);
        }
        return imageSource;
    }

    function isSupportedImageFormat(fileType) {
        return (viewableImageFileTypes.indexOf(fileType.toUpperCase()) != -1);
    }

    function refreshItem(absolutePath) {
        var timestamp = (new Date()).getTime();
        var i = imageGridModel.findIndexByAbsolutePath(absolutePath);
        console.debug("imageViewPage refreshItem i " + i);
        if (i >= 0) {
            imageGridModel.setProperty(i, "timestamp", timestamp);
        }
    }

    Rectangle {
        id: imageLabel
        anchors.top: parent.top
        width: parent.width
        height: 40
        z: 2
        color: "black"
        opacity: 0.5
        visible: showTools

        Text {
            id: imageLabelText
            text: (imageGrid.currentIndex >= 0) ? imageGridModel.get(imageGrid.currentIndex).name : "";
            anchors.fill: parent
            anchors.margins: 5
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: "white"
            elide: Text.ElideMiddle
            font.pointSize: 8
        }
    }

    ListModel {
        id: imageGridModel

        function findIndexByAbsolutePath(absolutePath) {
            for (var i=0; i<imageGridModel.count; i++) {
                if (imageGridModel.get(i).absolutePath == absolutePath) {
                    return i;
                }
            }

            return -1;
        }
    }

    GridView {
        id: imageGrid
        anchors.fill: parent
        cellWidth: parent.width
        cellHeight: parent.height
        cacheBuffer: cellWidth * 3
        flow: GridView.TopToBottom
        snapMode: GridView.SnapOneRow
        model: imageGridModel
        delegate: imageFlickDelegate
        pressDelay: 200
        interactive: showGrid

        function getViewIndex() {
            var cx = contentX + parent.width / 2;
            var cy = contentY + parent.height / 2;
            return indexAt(cx, cy);
        }

        onMovementEnded: {
            console.debug("imageViewPage imageGrid onMovementEnded " + getViewIndex());
            currentIndex = getViewIndex();
        }

        onCurrentIndexChanged: {
            console.debug("imageViewPage imageGrid onCurrentIndexChanged " + currentIndex);
        }
    }

    Component {
        id: imageFlickDelegate

        Rectangle {
            width: imageGrid.cellWidth
            height: imageGrid.cellHeight
            border.color: "transparent"
            color: "transparent"

            PinchArea {
                id: imageFlickPinchArea
                anchors.fill: parent
                pinch.dragAxis: Pinch.XandYAxis
                enabled: !imageGrid.moving // To avoid pinching while grid is moving.

                property real pinchScaleFactor: 1.0
                property int startX
                property int startY
                property int startWidth
                property int startHeight

                function setContentXY() {
                    // Set content X,Y after scaling.
                    var actualCenterX = imageFlickPinchArea.startX * (imageFlickView.width / imageFlickPinchArea.startWidth);
                    var actualCenterY = imageFlickPinchArea.startY * (imageFlickView.height / imageFlickPinchArea.startHeight);
                    console.debug("imageViewPage imageFlickPinchArea setContentXY imageFlick.width " + imageFlick.width + " imageFlick.height " + imageFlick.height);
                    console.debug("imageViewPage imageFlickPinchArea setContentXY imageFlick.contentX " + imageFlick.contentX + " imageFlick.contentY " + imageFlick.contentY);
                    console.debug("imageViewPage imageFlickPinchArea setContentXY imageFlickPinchArea.startX " + imageFlickPinchArea.startX + " imageFlickPinchArea.startY " + imageFlickPinchArea.startY);
                    console.debug("imageViewPage imageFlickPinchArea setContentXY actualCenterX " + actualCenterX + " actualCenterY " + actualCenterY);

                    imageFlick.contentX = actualCenterX - (imageFlick.width / 2);
                    imageFlick.contentY = actualCenterY - (imageFlick.height / 2);
                    console.debug("imageViewPage imageFlickPinchArea setContentXY after imageFlick.contentX " + imageFlick.contentX + " imageFlick.contentY " + imageFlick.contentY);
                }

                onPinchStarted: {
                    imageFlickPinchArea.startWidth = imageFlickView.width;
                    imageFlickPinchArea.startHeight = imageFlickView.height;
                    imageFlickPinchArea.startX = imageFlick.contentX + (imageFlick.width / 2);
                    imageFlickPinchArea.startY = imageFlick.contentY + (imageFlick.height / 2);
                    console.debug("imageViewPage imageFlickPinchArea onPinchStarted imageFlick.contentX " + imageFlick.contentX + " imageFlick.contentY " + imageFlick.contentY);
                    console.debug("imageViewPage imageFlickPinchArea onPinchStarted imageFlickPinchArea.startWidth " + imageFlickPinchArea.startWidth + " imageFlickPinchArea.startHeight " + imageFlickPinchArea.startHeight);
                    console.debug("imageViewPage imageFlickPinchArea onPinchStarted imageFlickPinchArea.startX " + imageFlickPinchArea.startX + " imageFlickPinchArea.startY " + imageFlickPinchArea.startY);

                    // Switch to flick mode.
                    imageViewPage.state = "flick";
                }
                onPinchUpdated: {
                    console.debug("imageViewPage imageFlickPinchArea onPinchUpdated pinch.scale " + pinch.scale);

                    // Image can be shrink to smaller than fit size. it will be enlarged back to fit.
                    // Image can be enlarged to larger than actual size. it will be shrink back to actual.
                    var newWidth = Math.round(imageFlickPinchArea.startWidth * pinch.scale * imageFlickPinchArea.pinchScaleFactor);
                    var newHeight = Math.round(imageFlickPinchArea.startHeight * pinch.scale * imageFlickPinchArea.pinchScaleFactor);
                    imageFlickView.width = newWidth;
                    imageFlickView.height = newHeight;

                    console.debug("imageViewPage imageFlickPinchArea onPinchUpdated imageFlickView.width " + imageFlickView.width + " imageFlickView.height " + imageFlickView.height);
                    console.debug("imageViewPage imageFlickPinchArea onPinchUpdated imageFlickView.paintedWidth " + imageFlickView.paintedWidth + " imageFlickView.paintedHeight " + imageFlickView.paintedHeight);
                    console.debug("imageViewPage imageFlickPinchArea onPinchUpdated imageFlick.contentWidth " + imageFlick.contentWidth + " imageFlick.contentHeight " + imageFlick.contentHeight);

                    // Set center.
                    setContentXY();
                }
                onPinchFinished: {
                    console.debug("imageViewPage imageFlickPinchArea onPinchFinished imageFlickView.width " + imageFlickView.width + " imageFlickView.height " + imageFlickView.height);
                    console.debug("imageViewPage imageFlickPinchArea onPinchFinished imageFlickView.sourceSize.width " + imageFlickView.sourceSize.width + " imageFlickView.sourceSize.height " + imageFlickView.sourceSize.height);
                    console.debug("imageViewPage imageFlickPinchArea onPinchFinished imageFlickView.paintedWidth " + imageFlickView.paintedWidth + " imageFlickView.paintedHeight " + imageFlickView.paintedHeight);
                    console.debug("imageViewPage imageFlickPinchArea onPinchFinished imageFlick.contentWidth " + imageFlick.contentWidth + " imageFlick.contentHeight " + imageFlick.contentHeight);
                    console.debug("imageViewPage imageFlickPinchArea onPinchFinished imageFlick.width " + imageFlick.width + " imageFlick.height " + imageFlick.height);

                    // Adjust size.
                    if (imageFlickView.width <= imageFlickView.minPaintedWidth && imageFlickView.height <= imageFlickView.minPaintedHeight) {
                        // Return to original cell size if scaled size is smaller than cell size.
                        imageFlickView.width = imageFlickView.minPaintedWidth;
                        imageFlickView.height = imageFlickView.minPaintedHeight;

                        // Reset center.
                        imageFlick.contentX = 0;
                        imageFlick.contentY = 0;

                        // Switch to flick mode.
                        imageViewPage.state = "grid";
                    } else {
                        // Switch to actual image.
                        imageFlickView.source = "";
                        imageFlickView.sourceSize.width = 1024; // Requires to limit cached image size for Symbian only.
                        imageFlickView.sourceSize.height = 1024; // Requires to limit cached image size for Symbian only.
                        imageFlickView.source = fetchCurrentPendingSourceUrl();
                        console.debug("imageViewPage imageFlickPinchArea onPinchFinished after imageFlickView.source " + imageFlickView.source);

                        // Set center.
                        setContentXY();

                        // Switch to flick mode.
                        imageViewPage.state = "flick";
                    }
                }

                Flickable {
                    id: imageFlick
                    width: Math.min(imageGrid.cellWidth, imageFlickView.width)
                    height: Math.min(imageGrid.cellHeight, imageFlickView.height)
                    contentWidth: imageFlickView.width
                    contentHeight: imageFlickView.height
                    anchors.centerIn: parent
//                    interactive: !showGrid

                    Image {
                        id: imageFlickView
                        source: getImageSource(previewUrl, timestamp) // NOTE It's populated while opening the page. Timestamp is used for force refreshing.
//                        fillMode: Image.PreserveAspectFit
//                        width: imageGrid.cellWidth // Undefine to get actual width.
//                        height: imageGrid.cellHeight // Undefine to get actual height.
//                        sourceSize.width: (selectedCloudType == -1 && fileType.toUpperCase() != "SVG") ? imageGrid.cellWidth : undefined // Requires for showing preview cache on local drive.
//                        sourceSize.height: (selectedCloudType == -1 && fileType.toUpperCase() != "SVG") ? imageGrid.cellHeight : undefined // Requires for showing preview cache on local drive.

                        property real minPaintedWidth
                        property real minPaintedHeight
                        property bool inPortrait: imageViewPage.inPortrait

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

                        function scaleTo(w, h) {
                            if (width == 0 || height == 0) {
                                console.debug("imageViewPage imageFlickView can't scale. width,height " + width + "," + height);
                                return;
                            }

                            var wScale = w / width;
                            var hScale = h / height;
                            console.debug("imageViewPage imageFlickView scaleTo w,h " + w + "," + h + " scale w,h " + wScale + "," + hScale);
                            var scale = Math.min(wScale, hScale);
                            width = width * scale;
                            height = height * scale;
                        }

                        onInPortraitChanged: {
                            console.debug("imageViewPage imageFlickView onInPortraitChanged name " + name + " inPortrait " + inPortrait);
                            // Scale to fit page. As cell size's updating is not done yet.
                            scaleTo(imageViewPage.width, imageViewPage.height);
                            // Save minimum size in cell.
                            minPaintedWidth = width;
                            minPaintedHeight = height;
                        }

                        onStatusChanged: {
                            if (status == Image.Ready && source != "") {
                                console.debug("imageViewPage imageFlickView onStatusChanged ready source " + source);
                                console.debug("imageViewPage imageFlickView onStatusChanged ready width " + width + " height " + height);
                                if (showGrid) {
                                    // Scale to fit cell.
                                    scaleTo(imageViewPage.width, imageViewPage.height);
                                    // Save minimum size in cell.
                                    minPaintedWidth = width;
                                    minPaintedHeight = height;
                                }
                                console.debug("imageViewPage imageFlickView onStatusChanged ready after width " + width + " height " + height);
                                console.debug("imageViewPage imageFlickView onStatusChanged ready paintedWidth " + paintedWidth + " paintedHeight " + paintedHeight);
                                console.debug("imageViewPage imageFlickView onStatusChanged ready minPaintedWidth " + minPaintedWidth + " minPaintedHeight " + minPaintedHeight);
                                console.debug("imageViewPage imageFlickView onStatusChanged ready sourceSize.width " + sourceSize.width + " sourceSize.height " + sourceSize.height);
                            } else if (status == Image.Loading) {
                                console.debug("imageViewPage imageFlickView onStatusChanged loading source " + source);
                            } else if (status == Image.Error) {
                                var imageSource = imageFlickView.source + "";
                                if (imageSource.indexOf("image://remote/") == 0) {
                                    // Cache preview image.
                                    cloudDriveModel.cacheImage(absolutePath, preview, -1, -1, imageViewPage.name); // Use original size because previewUrl is already specified with size.
                                } else if (imageSource.indexOf("image://local/") == 0) {
                                    // Cache preview image.
                                    cloudDriveModel.cacheImage(absolutePath, absolutePath, -1, -1, imageViewPage.name); // Use original file path to generate cached image.
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent

                            onClicked: {
                                showTools = !showTools;
                            }

                            onDoubleClicked: {
                                console.debug("imageViewPage imageFlick onDoubleClicked");
                                if (!showGrid) {
                                    // Return to original cell size if scaled size is smaller than cell size.
                                    imageFlickView.width = imageFlickView.minPaintedWidth;
                                    imageFlickView.height = imageFlickView.minPaintedHeight;

                                    // Reset center.
                                    imageFlick.contentX = 0;
                                    imageFlick.contentY = 0;

                                    // Switch to flick mode.
                                    imageViewPage.state = "grid";
                                }
                            }
                        }

                        BusyIndicator {
                            id: imageViewBusy
                            width: 100; height: width
                            anchors.centerIn: parent
                            visible: (parent.progress < 1 && parent.status == Image.Loading) || parent.status == Image.Error
                            running: visible

                            Text {
                                text: Math.floor(parent.parent.progress * 100) + "%"
                                font.pointSize: 6
                                color: (!inverted) ? "white" : "black"
                                anchors.centerIn: parent
                                visible: parent.visible && imageFlickView.status == Image.Loading
                            }
                        }
                    } // imageFlickView
                } // ImageFlick
            } // imageFlickPinchArea
        } // Container
    }

    ConfirmDialog {
        id: deleteItemConfirmation
        titleText: appInfo.emptyStr+qsTr("Delete")
        contentText: appInfo.emptyStr+((imageGrid.currentIndex >= 0) ? qsTr("Delete %1 ?").arg(imageGridModel.get(imageGrid.currentIndex).name) : "")
        onConfirm: {
            var imageFilePath = imageGridModel.get(imageGrid.currentIndex).absolutePath;
            if (selectedCloudType != -1) {
                cloudDriveModel.deleteFile(selectedCloudType, selectedUid, "", imageFilePath);
            } else {
                deleteProgressDialog.autoClose = true;
                fsModel.deleteFile(imageFilePath);
            }
            imageGridModel.remove(imageGrid.currentIndex);
            if (imageGrid.currentIndex < imageGridModel.count) {
                imageGrid.positionViewAtIndex(imageGrid.currentIndex, GridView.Contain);
            } else {
                imageGrid.positionViewAtEnd();
            }
        }
    }

    SystemInfoHelper {
        id: helper
    }
}
