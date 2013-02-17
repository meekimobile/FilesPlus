import QtQuick 1.1
import com.nokia.meego 1.0
import CloudDriveModel 1.0
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
    property bool inverted: !theme.inverted
    property bool showTools: true
    property real maxZoomFactor: 2
    property variant viewableImageFileTypes: ["JPG", "PNG", "GIF", "SVG"];

    state: "grid"
    states: [
        State {
            name: "grid"
            PropertyChanges { target: imageViewPage; showGrid: true }
            PropertyChanges { target: imageViewPage; showTools: true }
            PropertyChanges { target: imageGrid; visible: true }
            PropertyChanges { target: imageFlickView
                fillMode: Image.PreserveAspectFit
                source: ""
            }
            PropertyChanges { target: imageFlick; visible: false }
        },
        State {
            name: "flick"
            PropertyChanges { target: imageViewPage; showGrid: false }
            PropertyChanges { target: imageGrid; visible: false }
            PropertyChanges { target: imageFlickView
                fillMode: Image.PreserveAspectFit
                width: imageFlick.width
                height: imageFlick.height
                sourceSize.width: 1280  // Requested size for LocalFileImageProvider
                sourceSize.height: 1280  // Requested size for LocalFileImageProvider
                source: fetchCurrentPendingSourceUrl()
            }
            PropertyChanges { target: imageFlick; visible: true }
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
                        imageSource = "file://" + imageGrid.getViewFilePath();
                        Qt.openUrlExternally(imageSource);
                    }
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
                        imageSource = imageGrid.getViewFilePath();
                        gcpClient.printFileSlot(imageSource, -1);
                    }
                }
            }
        }
    }

    onStatusChanged: {
        if (status == PageStatus.Active && imageGridModel.count <= 0) {
            parseImageGridModel(model);
            imageGrid.currentIndex = (selectedModelIndex < 0) ? 0 : selectedModelIndex;
            imageLabelText.text = imageGridModel.get(imageGrid.currentIndex).name;
            imageGrid.positionViewAtIndex(imageGrid.currentIndex, GridView.Contain);
            // Fetch current item's media URL.
            fetchCurrentPendingSourceUrl();
        }
    }

    function orientationChangeSlot() {
        // Signal from main.qml
        imageGrid.positionViewAtIndex(imageGrid.currentIndex, GridView.Contain);
    }

    function parseImageGridModel(model) {
        imageGridModel.clear();

        var timestamp = (new Date()).getTime();

        for (var i = 0; i < model.count; i++) {
            var modelItem = model.get(i);
            modelItem.timestamp = timestamp;
            if (viewableImageFileTypes.indexOf(modelItem.fileType.toUpperCase()) != -1) {
                if (modelItem.source) {
                    // Remote image with source link.
                    modelItem.sourceUrl = modelItem.source;
                    modelItem.previewUrl = modelItem.preview;
                } else if (modelItem.preview) {
                    // TODO Figure out how to detect if it's cloud item.
                    // NOTE Must not modify otiginal source, to keep original state.
                    // Request media to get URL.
                    modelItem.sourceUrl = ""; // Pending get media URL once pinch zoom, open URL or print URL.
                    modelItem.previewUrl = "image://remote/" + modelItem.preview;
                } else if (modelItem.fileType.toUpperCase() == "SVG") {
                    // Local SVG image.
                    modelItem.sourceUrl = "file://" + modelItem.absolutePath;
                    modelItem.previewUrl = "file://" + modelItem.absolutePath;
                } else {
                    // Local image.
                    modelItem.sourceUrl = "image://local/" + modelItem.absolutePath;
                    modelItem.previewUrl = "image://local/" + modelItem.absolutePath;
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
            anchors.fill: parent
            anchors.margins: 5
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: "white"
            elide: Text.ElideMiddle
            font.pointSize: 18
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
        delegate: imageViewDelegate
        pressDelay: 200

        property int viewIndex: getViewIndex()
        property int selectedIndex: -1

        function getViewIndex() {
            var cx = contentX + parent.width / 2;
            var cy = contentY + parent.height / 2;
            return indexAt(cx, cy);
        }

        function getViewFilePath() {
            var i = imageGrid.getViewIndex();
            if (i > -1) {
                return imageGrid.model.get(i).absolutePath;
            }
            return "";
        }

        function getViewFileName() {
            var i = imageGrid.getViewIndex();
            if (i > -1) {
                return imageGrid.model.get(i).name;
            }
            return "";
        }

        onMovementEnded: {
            currentIndex = viewIndex;
//            console.debug("imageViewPage imageGrid onMovementEnded viewIndex " + viewIndex);
//            console.debug("imageViewPage imageGrid onMovementEnded currentIndex " + currentIndex);
//            console.debug("imageViewPage imageGrid onMovementEnded currentItem.width " + currentItem.width + " currentItem.height " + currentItem.height);
//            console.debug("imageViewPage imageGrid onMovementEnded currentItem.sourceSize.width " + currentItem.sourceSize.width + " currentItem.sourceSize.height " + currentItem.sourceSize.height);
            imageLabelText.text = getViewFileName();
        }

        onFlickEnded: {
            currentIndex = viewIndex;
//            console.debug("imageViewPage imageGrid onFlickEnded viewIndex " + viewIndex);
//            console.debug("imageViewPage imageGrid onFlickEnded currentIndex " + currentIndex);
//            console.debug("imageViewPage imageGrid onFlickEnded currentItem.width " + currentItem.width + " currentItem.height " + currentItem.height);
//            console.debug("imageViewPage imageGrid onFlickEnded currentItem.sourceSize.width " + currentItem.sourceSize.width + " currentItem.sourceSize.height " + currentItem.sourceSize.height);
            imageLabelText.text = getViewFileName();
        }
    }

    Component {
        id: imageViewDelegate

        Image {
            id: imageView
            source: getImageSource(previewUrl, timestamp) // NOTE It's populated while opening the page. Timestamp is used for force refreshing.
            asynchronous: true
            cache: false
            sourceSize.width: (fileType.toUpperCase() == "SVG") ? undefined : imageGrid.cellWidth
            sourceSize.height: (fileType.toUpperCase() == "SVG") ? undefined : imageGrid.cellHeight
            width: imageGrid.cellWidth
            height: imageGrid.cellHeight
            fillMode: Image.PreserveAspectFit

            function getImageSource(url, timestamp) {
                if (selectedCloudType != -1) {
                    return url + "#t=" + timestamp;
                } else {
                    return url;
                }
            }

            BusyIndicator {
                id: imageViewBusy
                style: BusyIndicatorStyle { size: "large" }
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                visible: (parent.progress < 1 && parent.status == Image.Loading)
                running: visible               
            }

            Text {
                text: Math.floor(imageView.progress * 100) + "%"
                font.pointSize: 16
                color: "white"
                anchors.centerIn: parent
                visible: imageViewBusy.visible
            }

            onStatusChanged: {
                if (status == Image.Null) {
//                    console.debug("imageViewPage imageView onStatusChanged index " + index + " status " + status + " absolutePath " + absolutePath);
//                    console.debug("imageViewPage imageView onStatusChanged imageGrid.currentIndex " + imageGrid.currentIndex);
//                    console.debug("imageViewPage imageView onStatusChanged imageViewPage.selectedModelIndex " + imageViewPage.selectedModelIndex);
                    // Issue: Position at index still not work if selected index is not covered by cache.

                    // Set currentIndex to skip image loading to selected image.
//                    if (imageGrid.currentIndex != imageViewPage.selectedModelIndex) {
//                        imageGrid.currentIndex = imageViewPage.selectedModelIndex;
//                        console.debug("imageViewPage imageView onStatusChanged set imageGrid.currentIndex " + imageGrid.currentIndex);
//                    }
                } else if (status == Image.Ready) {
//                    console.debug("imageViewPage imageView onStatusChanged index " + index + " status " + status + " absolutePath " + absolutePath);
//                    console.debug("imageViewPage imageView onStatusChanged imageGrid.currentIndex " + imageGrid.currentIndex);
//                    console.debug("imageViewPage imageView onStatusChanged imageViewPage.selectedModelIndex " + imageViewPage.selectedModelIndex);
//                    console.debug("imageViewPage imageView onStatusChanged width " + width + " height " + height);
//                    console.debug("imageViewPage imageView onStatusChanged sourceSize.width " + sourceSize.width + " sourceSize.height " + sourceSize.height);
//                    console.debug("imageViewPage imageView onStatusChanged paintedWidth " + paintedWidth + " paintedHeight " + paintedHeight);

                    // Position selected image.
//                    if (index == imageGrid.currentIndex) {
//                        imageGrid.positionViewAtIndex(index, GridView.Contain);
//                        imageLabelText.text = name;
//                        console.debug("imageViewPage imageView onStatusChanged positionViewAtIndex index " + index);
//                    }
                } else if (status == Image.Error) {
                    var imageSource = imageView.source + "";
                    if (imageSource.indexOf("image://remote/") == 0) {
                        cloudDriveModel.cacheImage(absolutePath, preview, -1, -1, imageViewPage.name); // Use default preview size.
                    }
                }
            }

            function startFlickMode(centerX, centerY) {
                // Send center, painted size to flick.
                var left = (imageView.width / 2) - (imageView.paintedWidth / 2);
                var right = (imageView.width / 2) + (imageView.paintedWidth / 2);
                var top = (imageView.height / 2) - (imageView.paintedHeight / 2);
                var bottom = (imageView.height / 2) + (imageView.paintedHeight / 2);
                imageFlick.gridMouseX = centerX - left;
                imageFlick.gridMouseY = centerY - top;
                imageFlick.gridPaintedWidth = imageView.paintedWidth;
                imageFlick.gridPaintedHeight = imageView.paintedHeight;
                imageViewPage.state = "flick";
            }

            PinchArea {
                anchors.fill: parent
                pinch.dragAxis: Pinch.XandYAxis

                onPinchStarted: {
                    console.debug("imageViewPage imageView onPinchStarted imageFlick.contentX " + imageFlick.contentX + " imageFlick.contentY " + imageFlick.contentY);

                    // TODO pinch cell image until finish, then show flick.
                    // Send center, painted size to flick.
                    startFlickMode(imageView.width / 2, imageView.height / 2);
                }

                MouseArea {
                    anchors.fill: parent

                    onClicked: {
                        showTools = !showTools;
                    }

                    onDoubleClicked: {
                        console.debug("imageViewPage imageView onDoubleClicked mouseX " + mouseX + " mouseY " + mouseY)

                        // Send relative mouseX, mouseY, painted size to flick.
                        startFlickMode(mouseX, mouseY);
                    }
                }
            }
        }
    }

    Flickable {
        id: imageFlick
        visible: false
        width: parent.width
        height: parent.height
        contentWidth: imageFlickView.width
        contentHeight: imageFlickView.height
        anchors.verticalCenter: parent.verticalCenter

        property int gridMouseX
        property int gridMouseY
        property int gridPaintedWidth
        property int gridPaintedHeight

        property real pinchScaleFactor: 1.0
        property int startX
        property int startY
        property int startWidth
        property int startHeight

        BusyIndicator {
            id: imageViewBusy
            style: BusyIndicatorStyle { size: "large" }
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            visible: (imageFlickView.progress < 1 && imageFlickView.status == Image.Loading)
            running: visible
        }

        Text {
            text: Math.floor(imageFlickView.progress * 100) + "%"
            font.pointSize: 16
            color: "white"
            anchors.centerIn: parent
            visible: imageViewBusy.visible
        }

        Image {
            id: imageFlickView
            fillMode: Image.PreserveAspectFit

            function getImageSource(url, timestamp) {
                if (selectedCloudType != -1) {
                    return url + "#t=" + timestamp;
                } else {
                    return url;
                }
            }

            onStatusChanged: {
                if (status == Image.Ready && source != "") {
//                    if (imageViewPage.state == "actual") {
//                        imageFlickView.width = imageFlickView.sourceSize.width;
//                        imageFlickView.height = imageFlickView.sourceSize.height;
//                    } else {
//                        imageFlickView.width = imageFlick.gridPaintedWidth;
//                        imageFlickView.height = imageFlick.gridPaintedHeight;
//                    }

                    imageFlickView.width = imageFlickView.paintedWidth;
                    imageFlickView.height = imageFlickView.paintedHeight;

                    console.debug("imageViewPage ------------------onStatusChanged status " + status + " source " + source);
                    console.debug("imageViewPage imageFlick.gridMouseX " + imageFlick.gridMouseX + " imageFlick.gridMouseY " + imageFlick.gridMouseY);
                    console.debug("imageViewPage imageFlick.gridPaintedWidth " + imageFlick.gridPaintedWidth + " imageFlick.gridPaintedHeight " + imageFlick.gridPaintedHeight);
                    console.debug("imageViewPage imageFlick.contentWidth " + imageFlick.contentWidth + " imageFlick.contentHeight " + imageFlick.contentHeight);
                    console.debug("imageViewPage imageFlick.width " + imageFlick.width + " imageFlick.height " + imageFlick.height);
                    console.debug("imageViewPage imageFlickView.fillMode " + imageFlickView.fillMode);
                    console.debug("imageViewPage imageFlickView.width " + imageFlickView.width + " imageFlickView.height " + imageFlickView.height);
                    console.debug("imageViewPage imageFlickView.sourceSize.width " + imageFlickView.sourceSize.width + " imageFlickView.sourceSize.height " + imageFlickView.sourceSize.height);
                    console.debug("imageViewPage imageFlickView.paintedWidth " + imageFlickView.paintedWidth + " imageFlickView.paintedHeight " + imageFlickView.paintedHeight);

                    // Set center from dblclick zoom.
                    var actualCenterX = imageFlick.gridMouseX * (imageFlick.contentWidth / imageFlick.gridPaintedWidth);
                    var actualCenterY = imageFlick.gridMouseY * (imageFlick.contentHeight / imageFlick.gridPaintedHeight);
                    console.debug("imageViewPage imageFlickView onStatusChanged actualCenterX " + actualCenterX + " actualCenterY " + actualCenterY);
                    imageFlick.contentX = actualCenterX - (imageFlick.width / 2);
                    imageFlick.contentY = actualCenterY - (imageFlick.height / 2);
                    console.debug("imageViewPage imageFlickView onStatusChanged imageFlick.contentX " + imageFlick.contentX + " imageFlick.contentY " + imageFlick.contentY);
                } else if (status == Image.Error) {
                    var imageSource = imageFlickView.source + "";
                    if (imageSource.indexOf("image://remote/") == 0) {
                        var currentItem = imageGridModel.get(imageGrid.currentIndex);
                        cloudDriveModel.cacheImage(currentItem.absolutePath, currentItem.source, -1, -1, imageViewPage.name); // Use original size.
                    }
                }
            }

            PinchArea {
                id: imagePinchArea
                anchors.fill: parent
                pinch.dragAxis: Pinch.XandYAxis

                onPinchStarted: {
                    console.debug("imageViewPage imagePinchArea onPinchStarted");
                    console.debug("imageViewPage imagePinchArea onPinchStarted imageFlick.contentX " + imageFlick.contentX + " imageFlick.contentY " + imageFlick.contentY);
                    imageViewPage.state = "flick";
                    imageFlick.startWidth = imageFlickView.width;
                    imageFlick.startHeight = imageFlickView.height;
                    imageFlick.startX = imageFlick.contentX + (imageFlick.width / 2);
                    imageFlick.startY = imageFlick.contentY + (imageFlick.height / 2);
                    console.debug("imageViewPage imagePinchArea onPinchStarted imageFlick.startWidth " + imageFlick.startWidth + " imageFlick.startHeight " + imageFlick.startHeight);
                    console.debug("imageViewPage imagePinchArea onPinchStarted imageFlick.startX " + imageFlick.startX + " imageFlick.startY " + imageFlick.startY);
                }
                onPinchFinished: {
                    console.debug("imageViewPage imagePinchArea onPinchFinished");
                    console.debug("imageViewPage imagePinchArea onPinchFinished imageFlickView.width " + imageFlickView.width + " imageFlickView.height " + imageFlickView.height);
                    console.debug("imageViewPage imagePinchArea onPinchFinished imageFlickView.sourceSize.width " + imageFlickView.sourceSize.width + " imageFlickView.sourceSize.height " + imageFlickView.sourceSize.height);
                    console.debug("imageViewPage imagePinchArea onPinchFinished imageFlickView.paintedWidth " + imageFlickView.paintedWidth + " imageFlickView.paintedHeight " + imageFlickView.paintedHeight);
                    console.debug("imageViewPage imagePinchArea onPinchFinished imageFlick.contentWidth " + imageFlick.contentWidth + " imageFlick.contentHeight " + imageFlick.contentHeight);
                    console.debug("imageViewPage imagePinchArea onPinchFinished imageFlick.gridPaintedWidth " + imageFlick.gridPaintedWidth + " imageFlick.gridPaintedHeight " + imageFlick.gridPaintedHeight);
                    console.debug("imageViewPage imagePinchArea onPinchFinished imageFlick.width " + imageFlick.width + " imageFlick.height " + imageFlick.height);

                    // Adjust size.
                    if (imageFlickView.width < imageFlick.gridPaintedWidth || imageFlickView.height < imageFlick.gridPaintedHeight) {
                        // Issue: gridW,H should keep original grid size.
                        imageFlickView.width = imageFlick.gridPaintedWidth;
                        imageFlickView.height = imageFlick.gridPaintedHeight
                    } else if (imageFlickView.width > imageFlickView.sourceSize.width * maxZoomFactor || imageFlickView.height > imageFlickView.sourceSize.height * maxZoomFactor) {
                        imageFlickView.width = imageFlickView.sourceSize.width * maxZoomFactor;
                        imageFlickView.height = imageFlickView.sourceSize.height * maxZoomFactor;
                    }

                    imageFlick.contentWidth = imageFlickView.width;
                    imageFlick.contentHeight = imageFlickView.height;
                    console.debug("imageViewPage imagePinchArea onPinchFinished after imageFlickView.width " + imageFlickView.width + " imageFlickView.height " + imageFlickView.height);
                    console.debug("imageViewPage imagePinchArea onPinchFinished after imageFlick.contentWidth " + imageFlick.contentWidth + " imageFlick.contentHeight " + imageFlick.contentHeight);

                    // Show grid if width or height of image fit to flick view.
                    if (imageFlickView.width == imageFlick.width || imageFlickView.height == imageFlick.height) {
                        imageViewPage.state = "grid";
                    }

                    // Set center.
                    var actualCenterX = imageFlick.startX * (imageFlickView.width / imageFlick.startWidth);
                    var actualCenterY = imageFlick.startY * (imageFlickView.height / imageFlick.startHeight);
                    console.debug("imageViewPage imagePinchArea onPinchFinished imageFlick.startX " + imageFlick.startX + " imageFlick.startY " + imageFlick.startY);
                    console.debug("imageViewPage imagePinchArea onPinchFinished actualCenterX " + actualCenterX + " actualCenterY " + actualCenterY);

                    imageFlick.contentX = actualCenterX - (imageFlick.width / 2);
                    imageFlick.contentY = actualCenterY - (imageFlick.height / 2);
                    console.debug("imageViewPage imagePinchArea onPinchFinished imageFlick.contentX " + imageFlick.contentX + " imageFlick.contentY " + imageFlick.contentY);
                }
                onPinchUpdated: {
                    console.debug("imageViewPage imagePinchArea onPinchUpdated pinch.scale " + pinch.scale);

                    // Image can be shrink to smaller than fit size. it will be enlarged back to fit.
                    // Image can be enlarged to larger than actual size. it will be shrink back to actual.
                    var newWidth = Math.round(imageFlick.startWidth * pinch.scale * imageFlick.pinchScaleFactor);
                    var newHeight = Math.round(imageFlick.startHeight * pinch.scale * imageFlick.pinchScaleFactor);
                    imageFlickView.width = newWidth;
                    imageFlickView.height = newHeight;

                    imageFlick.contentWidth = imageFlickView.width;
                    imageFlick.contentHeight = imageFlickView.height;
                    console.debug("imageViewPage imagePinchArea onPinchUpdated imageFlickView.width " + imageFlickView.width + " imageFlickView.height " + imageFlickView.height);
                    console.debug("imageViewPage imagePinchArea onPinchUpdated imageFlick.contentWidth " + imageFlick.contentWidth + " imageFlick.contentHeight " + imageFlick.contentHeight);

                    // Set center.
                    var actualCenterX = imageFlick.startX * (imageFlickView.width / imageFlick.startWidth);
                    var actualCenterY = imageFlick.startY * (imageFlickView.height / imageFlick.startHeight);
                    console.debug("imageViewPage imagePinchArea onPinchUpdated imageFlick.startX " + imageFlick.startX + " imageFlick.startY " + imageFlick.startY);
                    console.debug("imageViewPage imagePinchArea onPinchUpdated actualCenterX " + actualCenterX + " actualCenterY " + actualCenterY);

                    imageFlick.contentX = actualCenterX - (imageFlick.width / 2);
                    imageFlick.contentY = actualCenterY - (imageFlick.height / 2);
                    console.debug("imageViewPage imagePinchArea onPinchUpdated imageFlick.contentX " + imageFlick.contentX + " imageFlick.contentY " + imageFlick.contentY);
                }

                MouseArea {
                    anchors.fill: parent

                    onClicked: {
                        showTools = !showTools;
                    }

                    onDoubleClicked: {
                        console.debug("imageViewPage imageFlick onDoubleClicked");
                        imageViewPage.state = "grid";
                    }
                }
            }
        }
    }
}
