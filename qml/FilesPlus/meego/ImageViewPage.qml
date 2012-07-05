import QtQuick 1.1
import com.nokia.meego 1.1
import FolderSizeItemListModel 1.0
import SystemInfoHelper 1.0
import "Utility.js" as Utility

Page {
    id: imageViewPage
    tools: null

    property string name: "imageViewPage"
    property alias model: imageGrid.model
    property string fileName
    property bool showGrid: true
    property variant supportedFileType: ["JPG", "PNG", "SVG"]

    state: "grid"
    states: [
        State {
            name: "grid"
            PropertyChanges { target: imageViewPage; showGrid: true }
            PropertyChanges { target: imageGrid; visible: true }
            PropertyChanges { target: imageFlickView
                fillMode: Image.PreserveAspectFit
                source: ""
            }
            PropertyChanges { target: imageFlick
                visible: false
            }
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
                source: "image://local/" + imageGrid.getViewFilePath()
            }
            PropertyChanges { target: imageFlick;
                visible: true
            }
        },
        State {
            name: "actual"
            PropertyChanges { target: imageViewPage; showGrid: false }
            PropertyChanges { target: imageGrid; visible: false }
            PropertyChanges { target: imageFlickView
                fillMode: Image.PreserveAspectFit
                width: 1280
                height: 1280
                sourceSize.width: 1280  // Requested size for LocalFileImageProvider
                sourceSize.height: 1280  // Requested size for LocalFileImageProvider
                source: "image://local/" + imageGrid.getViewFilePath()
            }
            PropertyChanges { target: imageFlick;
                visible: true
            }
        }
    ]

    ToolBar {
        id: toolBar
        anchors.bottom: parent.bottom
        opacity: 0.5
        z: 2
        visible: false
        tools: ToolBarLayout {
            ToolIcon {
                id: backButton
                iconId: "toolbar-back"
                onClicked: {
                    var p = pageStack.find(function (page) { return page.name == "folderPage"; });
                    if (p) p.refreshSlot();
                    pageStack.pop();
                }
            }

            ToolIcon {
                id: openButton
                iconSource: "photos.svg"
                onClicked: {
                    Qt.openUrlExternally(helper.getUrl(imageGrid.getViewFilePath()));
                }
            }

            ToolIcon {
                id: printButton
                iconSource: "print.svg"
                onClicked: {
                    var p = pageStack.find(function (page) { return page.name == "folderPage"; });
                    if (p) p.printFileSlot(imageGrid.getViewFilePath(), -1);
                }
            }
        }
    }

    Component.onCompleted: {
        console.debug("imageViewPage onCompleted imageGrid.currentIndex " + imageGrid.currentIndex);
    }

    onFileNameChanged: {
        console.debug("imageViewPage onFileNameChanged fileName " + fileName);
    }

    function orientationChangeSlot() {
        // Signal from main.qml
        imageGrid.positionViewAtIndex(imageGrid.currentIndex, GridView.Contain);
    }

    SystemInfoHelper {
        id: helper
    }

    Rectangle {
        id: imageLabel
        anchors.top: parent.top
        width: parent.width
        height: 40
        z: 2
        color: "black"
        opacity: 0.5
        visible: false

        Text {
            id: imageLabelText
            anchors.fill: parent
            anchors.margins: 5
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: "white"
            elide: Text.ElideMiddle
        }
    }

//    Timer {
//        id: clickDelayTimer
//        interval: 300
//        running: false
//        onTriggered: {
//            imageLabel.visible = !imageLabel.visible;
//        }
//    }

    GridView {
        id: imageGrid
        anchors.fill: parent
        cellWidth: parent.width
        cellHeight: parent.height
        cacheBuffer: cellWidth * 3
        flow: GridView.TopToBottom
        snapMode: GridView.SnapOneRow
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
                return imageGrid.model.getProperty(i, FolderSizeItemListModel.AbsolutePathRole);
            }
            return "";
        }

        function getViewFileName() {
            var i = imageGrid.getViewIndex();
            if (i > -1) {
                return imageGrid.model.getProperty(i, FolderSizeItemListModel.NameRole);
            }
            return "";
        }

        function getImageModelIndex(fileName) {
            // Model is set before it can finish filter only images. Find selected image position needs to be done with full list.
            var imageIndex = 0;
            for (var i=0; i<imageGrid.model.count; i++) {
                var isDir = imageGrid.model.getProperty(i, FolderSizeItemListModel.IsDirRole);
                var fileType = imageGrid.model.getProperty(i, FolderSizeItemListModel.FileTypeRole);
                var name = imageGrid.model.getProperty(i, FolderSizeItemListModel.NameRole);
//                console.debug("ImageViewPage imageGrid getImageModelIndex " + i + " name " + name + " isDir " + isDir + " fileType " + fileType)

                // If item is dir or is not image, skip.
                if (isDir || supportedFileType.indexOf(fileType.toUpperCase()) == -1) {
                    continue;
                }

                if (name == fileName) {
                    console.debug("ImageViewPage imageGrid getImageModelIndex found " + i + " name " + name)
                    return imageIndex;
                }

                imageIndex++;
            }
//            console.debug("getImageModelIndex model.count " + model.count);
//            console.debug("getImageModelIndex found index " + i);
            return -1;
        }

        Component.onCompleted: {
            console.debug("imageGrid onCompleted");
            // Model is not set, positionViewAtIndex() won't work.
        }

        onModelChanged: {
            console.debug("imageGrid onModelChanged count " + model.count + " imageViewPage.fileName " + imageViewPage.fileName);
            // GridView item is not set, positionViewAtIndex() won't work.
            selectedIndex = getImageModelIndex(imageViewPage.fileName);
            console.debug("imageGrid onModelChanged selectedIndex " + selectedIndex);
        }

        onWidthChanged: {
//            console.debug("imageGrid onWidthChanged width " + width);
//            console.debug("imageGrid onWidthChanged cellWidth " + cellWidth);
        }

        onHeightChanged: {
//            console.debug("imageGrid onHeightChanged height " + height);
//            console.debug("imageGrid onHeightChanged cellHeight " + cellHeight);
        }

        onMovementEnded: {
            imageGrid.currentIndex = viewIndex;
//            console.debug("imageGrid onMovementEnded viewIndex " + viewIndex);
//            console.debug("imageGrid onMovementEnded currentIndex " + currentIndex);
//            console.debug("imageGrid onMovementEnded currentItem.width " + currentItem.width + " currentItem.height " + currentItem.height);
//            console.debug("imageGrid onMovementEnded currentItem.sourceSize.width " + currentItem.sourceSize.width + " currentItem.sourceSize.height " + currentItem.sourceSize.height);
            imageLabelText.text = getViewFileName();
        }

        onFlickEnded: {
            imageGrid.currentIndex = viewIndex;
//            console.debug("imageGrid onFlickEnded viewIndex " + viewIndex);
//            console.debug("imageGrid onFlickEnded currentIndex " + currentIndex);
//            console.debug("imageGrid onFlickEnded currentItem.width " + currentItem.width + " currentItem.height " + currentItem.height);
//            console.debug("imageGrid onFlickEnded currentItem.sourceSize.width " + currentItem.sourceSize.width + " currentItem.sourceSize.height " + currentItem.sourceSize.height);
            imageLabelText.text = getViewFileName();
        }
    }

    Component {
        id: imageViewDelegate

        Image {
            id: imageView

            property bool isImage: isSupportedImageFormat(fileType)

            source: getImageSource()
            asynchronous: true
            sourceSize.width: (fileType.toUpperCase() == "SVG") ? undefined : imageGrid.cellWidth
            sourceSize.height: (fileType.toUpperCase() == "SVG") ? undefined : imageGrid.cellHeight
            width: imageGrid.cellWidth
            height: imageGrid.cellHeight
            fillMode: Image.PreserveAspectFit

            function isSupportedImageFormat(fileType) {
                return (supportedFileType.indexOf(fileType.toUpperCase()) != -1);
            }

            function getImageSource() {
                if (imageViewPage.status == PageStatus.Active && isImage) {
                    if (fileType.toUpperCase() == "SVG") {
                        return absolutePath;
                    } else {
                        return "image://local/" + absolutePath;
                    }
                } else {
                    return "";
                }
            }

            BusyIndicator {
                id: imageViewBusy
                width: 80
                height: 80
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                visible: (parent.progress < 1 && parent.status == Image.Loading)
                running: visible
            }

            onStatusChanged: {
                if (status == Image.Null) {
//                    console.debug("imageView onStatusChanged index " + index + " status " + status + " absolutePath " + absolutePath);
//                    console.debug("imageView onStatusChanged imageGrid.currentIndex " + imageGrid.currentIndex);
//                    console.debug("imageView onStatusChanged imageGrid.selectedIndex " + imageGrid.selectedIndex);
                    // Issue: Position at index still not work if selected index is not covered by cache.

                    // Set currentIndex to skip image loading to selected image.
                    if (imageGrid.currentIndex != imageGrid.selectedIndex) {
                        imageGrid.currentIndex = imageGrid.selectedIndex;
                        console.debug("imageView onStatusChanged set imageGrid.currentIndex " + imageGrid.currentIndex);
                    }
                }

                if (status == Image.Ready) {
//                    console.debug("imageView onStatusChanged index " + index + " status " + status + " absolutePath " + absolutePath);
//                    console.debug("imageView onStatusChanged imageGrid.currentIndex " + imageGrid.currentIndex);
//                    console.debug("imageView onStatusChanged imageGrid.selectedIndex " + imageGrid.selectedIndex);
//                    console.debug("imageView onStatusChanged width " + width + " height " + height);
//                    console.debug("imageView onStatusChanged sourceSize.width " + sourceSize.width + " sourceSize.height " + sourceSize.height);
//                    console.debug("imageView onStatusChanged paintedWidth " + paintedWidth + " paintedHeight " + paintedHeight);

                    // Position selected image.
                    if (index == imageGrid.currentIndex) {
                        imageGrid.positionViewAtIndex(index, GridView.Contain);
                        imageLabelText.text = name;
                        console.debug("imageView onStatusChanged positionViewAtIndex index " + index);
                    }

                    // Show toolbar and label.
                    if (!toolBar.visible) {
                        toolBar.visible = true;
                        imageLabel.visible = true;
                    }
                }
            }

//            PinchArea {
//                anchors.fill: parent
//                pinch.dragAxis: Pinch.XandYAxis

//                onPinchStarted: {
//                    console.debug("imageView onPinchStarted imageFlick.contentX " + imageFlick.contentX + " imageFlick.contentY " + imageFlick.contentY);

//                    // TODO pinch cell image until finish, then show flick.

//                    // Send center, painted size to flick.
//                    var left = (imageView.width / 2) - (imageView.paintedWidth / 2);
//                    var right = (imageView.width / 2) + (imageView.paintedWidth / 2);
//                    var top = (imageView.height / 2) - (imageView.paintedHeight / 2);
//                    var bottom = (imageView.height / 2) + (imageView.paintedHeight / 2);
//                    imageFlick.gridMouseX = (imageView.width / 2) - left;
//                    imageFlick.gridMouseY = (imageView.height / 2) - top;
//                    imageFlick.gridPaintedWidth = imageView.paintedWidth;
//                    imageFlick.gridPaintedHeight = imageView.paintedHeight;
//                    imageViewPage.state = "flick";
//                }

//                MouseArea {
//                    anchors.fill: parent

////                    onClicked: {
////                        console.debug("imageView onClicked mouseX " + mouseX + " mouseY " + mouseY)
////                        clickDelayTimer.restart();
////                    }

//                    onDoubleClicked: {
//                        console.debug("imageView onDoubleClicked mouseX " + mouseX + " mouseY " + mouseY)
//                        clickDelayTimer.stop();

//                        // TODO Send relative mouseX, mouseY, painted size to flick.
//                        var left = (imageView.width / 2) - (imageView.paintedWidth / 2);
//                        var right = (imageView.width / 2) + (imageView.paintedWidth / 2);
//                        var top = (imageView.height / 2) - (imageView.paintedHeight / 2);
//                        var bottom = (imageView.height / 2) + (imageView.paintedHeight / 2);
//                        imageFlick.gridMouseX = mouseX - left;
//                        imageFlick.gridMouseY = mouseY - top;
//                        imageFlick.gridPaintedWidth = imageView.paintedWidth;
//                        imageFlick.gridPaintedHeight = imageView.paintedHeight;
//                        imageViewPage.state = "actual";
//                    }
//                }
//            }
        }
    }

    Flickable {
        id: imageFlick
        visible: false
        width: parent.width
        height: parent.height
        contentWidth: imageFlickView.width
        contentHeight: imageFlickView.height

        property int gridMouseX
        property int gridMouseY
        property int gridPaintedWidth
        property int gridPaintedHeight

        property real pinchScaleFactor: 1.0
        property int startX
        property int startY
        property int startWidth
        property int startHeight

//        onVisibleChanged: {
//            if (visible) {
//                console.debug("-------------------onVisibleChanged---------------------");
//                console.debug("imageFlick.width " + imageFlick.width + " imageFlick.height " + imageFlick.height);
//                console.debug("imageFlick.contentWidth " + imageFlick.contentWidth + " imageFlick.contentHeight " + imageFlick.contentHeight);
//                console.debug("imageFlick.contentX " + imageFlick.contentX + " imageFlick.contentY " + imageFlick.contentY);
//                console.debug("imageFlickView.fillMode " + imageFlickView.fillMode);
//                console.debug("imageFlickView.width " + imageFlickView.width + " imageFlickView.height " + imageFlickView.height);
//                console.debug("imageFlickView.sourceSize.width " + imageFlickView.sourceSize.width + " imageFlickView.sourceSize.height " + imageFlickView.sourceSize.height);
//                console.debug("imageFlickView.paintedWidth " + imageFlickView.paintedWidth + " imageFlickView.paintedHeight " + imageFlickView.paintedHeight);
//            }
//        }

        BusyIndicator {
            id: imageViewBusy
            width: 80
            height: 80
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            visible: (imageFlickView.progress < 1 && imageFlickView.status == Image.Loading)
            running: visible
        }

        Image {
            id: imageFlickView
            fillMode: Image.PreserveAspectFit

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

                    console.debug("------------------onStatusChanged status " + status + " source " + source);
                    console.debug("imageFlick.gridMouseX " + imageFlick.gridMouseX + " imageFlick.gridMouseY " + imageFlick.gridMouseY);
                    console.debug("imageFlick.gridPaintedWidth " + imageFlick.gridPaintedWidth + " imageFlick.gridPaintedHeight " + imageFlick.gridPaintedHeight);
                    console.debug("imageFlick.contentWidth " + imageFlick.contentWidth + " imageFlick.contentHeight " + imageFlick.contentHeight);
                    console.debug("imageFlick.width " + imageFlick.width + " imageFlick.height " + imageFlick.height);
                    console.debug("imageFlickView.fillMode " + imageFlickView.fillMode);
                    console.debug("imageFlickView.width " + imageFlickView.width + " imageFlickView.height " + imageFlickView.height);
                    console.debug("imageFlickView.sourceSize.width " + imageFlickView.sourceSize.width + " imageFlickView.sourceSize.height " + imageFlickView.sourceSize.height);
                    console.debug("imageFlickView.paintedWidth " + imageFlickView.paintedWidth + " imageFlickView.paintedHeight " + imageFlickView.paintedHeight);

                    // Set center from dblclick zoom.
                    var actualCenterX = imageFlick.gridMouseX * (imageFlick.contentWidth / imageFlick.gridPaintedWidth);
                    var actualCenterY = imageFlick.gridMouseY * (imageFlick.contentHeight / imageFlick.gridPaintedHeight);
                    console.debug("imageFlickView onStatusChanged actualCenterX " + actualCenterX + " actualCenterY " + actualCenterY);
                    imageFlick.contentX = actualCenterX - (imageFlick.width / 2);
                    imageFlick.contentY = actualCenterY - (imageFlick.height / 2);
                    console.debug("imageFlickView onStatusChanged imageFlick.contentX " + imageFlick.contentX + " imageFlick.contentY " + imageFlick.contentY);
                }
            }

            PinchArea {
                id: imagePinchArea
                anchors.fill: parent
                pinch.dragAxis: Pinch.XandYAxis

                onPinchStarted: {
                    console.debug("imagePinchArea onPinchStarted");
                    console.debug("imagePinchArea onPinchStarted imageFlick.contentX " + imageFlick.contentX + " imageFlick.contentY " + imageFlick.contentY);
                    imageViewPage.state = "flick";
                    imageFlick.startWidth = imageFlickView.width;
                    imageFlick.startHeight = imageFlickView.height;
                    imageFlick.startX = imageFlick.contentX + (imageFlick.width / 2);
                    imageFlick.startY = imageFlick.contentY + (imageFlick.height / 2);
                    console.debug("imagePinchArea onPinchStarted imageFlick.startWidth " + imageFlick.startWidth + " imageFlick.startHeight " + imageFlick.startHeight);
                    console.debug("imagePinchArea onPinchStarted imageFlick.startX " + imageFlick.startX + " imageFlick.startY " + imageFlick.startY);
                }
                onPinchFinished: {
                    console.debug("imagePinchArea onPinchFinished");
                    console.debug("imagePinchArea onPinchFinished imageFlickView.width " + imageFlickView.width + " imageFlickView.height " + imageFlickView.height);
                    console.debug("imagePinchArea onPinchFinished imageFlickView.sourceSize.width " + imageFlickView.sourceSize.width + " imageFlickView.sourceSize.height " + imageFlickView.sourceSize.height);
                    console.debug("imagePinchArea onPinchFinished imageFlickView.paintedWidth " + imageFlickView.paintedWidth + " imageFlickView.paintedHeight " + imageFlickView.paintedHeight);
                    console.debug("imagePinchArea onPinchFinished imageFlick.contentWidth " + imageFlick.contentWidth + " imageFlick.contentHeight " + imageFlick.contentHeight);
                    console.debug("imagePinchArea onPinchFinished imageFlick.gridPaintedWidth " + imageFlick.gridPaintedWidth + " imageFlick.gridPaintedHeight " + imageFlick.gridPaintedHeight);
                    console.debug("imagePinchArea onPinchFinished imageFlick.width " + imageFlick.width + " imageFlick.height " + imageFlick.height);

                    // Adjust size.
                    if (imageFlickView.width < imageFlick.gridPaintedWidth || imageFlickView.height < imageFlick.gridPaintedHeight) {
                        // Issue: gridW,H should keep original grid size.
                        imageFlickView.width = imageFlick.gridPaintedWidth;
                        imageFlickView.height = imageFlick.gridPaintedHeight
                    } else if (imageFlickView.width > imageFlickView.sourceSize.width || imageFlickView.height > imageFlickView.sourceSize.height) {
                        imageFlickView.width = imageFlickView.sourceSize.width;
                        imageFlickView.height = imageFlickView.sourceSize.height;
                    }

                    imageFlick.contentWidth = imageFlickView.width;
                    imageFlick.contentHeight = imageFlickView.height;
                    console.debug("imagePinchArea onPinchFinished after imageFlickView.width " + imageFlickView.width + " imageFlickView.height " + imageFlickView.height);
                    console.debug("imagePinchArea onPinchFinished after imageFlick.contentWidth " + imageFlick.contentWidth + " imageFlick.contentHeight " + imageFlick.contentHeight);

                    // Show grid if width or height of image fit to flick view.
                    if (imageFlickView.width == imageFlick.width || imageFlickView.height == imageFlick.height) {
                        imageViewPage.state = "grid";
                    }

                    // Set center.
                    var actualCenterX = imageFlick.startX * (imageFlickView.width / imageFlick.startWidth);
                    var actualCenterY = imageFlick.startY * (imageFlickView.height / imageFlick.startHeight);
                    console.debug("imagePinchArea onPinchFinished imageFlick.startX " + imageFlick.startX + " imageFlick.startY " + imageFlick.startY);
                    console.debug("imagePinchArea onPinchFinished actualCenterX " + actualCenterX + " actualCenterY " + actualCenterY);

                    imageFlick.contentX = actualCenterX - (imageFlick.width / 2);
                    imageFlick.contentY = actualCenterY - (imageFlick.height / 2);
                    console.debug("imagePinchArea onPinchFinished imageFlick.contentX " + imageFlick.contentX + " imageFlick.contentY " + imageFlick.contentY);
                }
                onPinchUpdated: {
                    console.debug("imagePinchArea onPinchUpdated pinch.scale " + pinch.scale);

                    // Image can be shrink to smaller than fit size. it will be enlarged back to fit.
                    // Image can be enlarged to larger than actual size. it will be shrink back to actual.
                    var newWidth = Math.round(imageFlick.startWidth * pinch.scale * imageFlick.pinchScaleFactor);
                    var newHeight = Math.round(imageFlick.startHeight * pinch.scale * imageFlick.pinchScaleFactor);
                    imageFlickView.width = newWidth;
                    imageFlickView.height = newHeight;

                    imageFlick.contentWidth = imageFlickView.width;
                    imageFlick.contentHeight = imageFlickView.height;
                    console.debug("imagePinchArea onPinchUpdated imageFlickView.width " + imageFlickView.width + " imageFlickView.height " + imageFlickView.height);
                    console.debug("imagePinchArea onPinchUpdated imageFlick.contentWidth " + imageFlick.contentWidth + " imageFlick.contentHeight " + imageFlick.contentHeight);

                    // Set center.
                    var actualCenterX = imageFlick.startX * (imageFlickView.width / imageFlick.startWidth);
                    var actualCenterY = imageFlick.startY * (imageFlickView.height / imageFlick.startHeight);
                    console.debug("imagePinchArea onPinchUpdated imageFlick.startX " + imageFlick.startX + " imageFlick.startY " + imageFlick.startY);
                    console.debug("imagePinchArea onPinchUpdated actualCenterX " + actualCenterX + " actualCenterY " + actualCenterY);

                    imageFlick.contentX = actualCenterX - (imageFlick.width / 2);
                    imageFlick.contentY = actualCenterY - (imageFlick.height / 2);
                    console.debug("imagePinchArea onPinchUpdated imageFlick.contentX " + imageFlick.contentX + " imageFlick.contentY " + imageFlick.contentY);
                }

                MouseArea {
                    anchors.fill: parent

//                    onClicked: {
//                        console.debug("imageFlick onClicked");
//                        clickDelayTimer.restart();
//                    }

                    onDoubleClicked: {
                        console.debug("imageFlick onDoubleClicked");
                        clickDelayTimer.stop();
                        imageViewPage.state = "grid";
                    }
                }
            }
        }
    }
}
