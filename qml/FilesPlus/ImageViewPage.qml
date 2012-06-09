import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

Page {
    id: imageViewPage

    property string name: "imageViewPage"
    property alias model: imageGrid.model
    property string fileName
    property bool showGrid: true

    state: "grid"
    states: [
        State {
            name: "grid"
            when: showGrid
            PropertyChanges { target: imageGrid; visible: true }
            PropertyChanges { target: imageFlick
                visible: false
                contentWidth: width
                contentHeight: height
            }
            PropertyChanges { target: imageFlickView
                fillMode: Image.PreserveAspectFit
                width: parent.width
                height: parent.height
            }
        },
        State {
            name: "flick"
            when: !showGrid
            PropertyChanges { target: imageGrid; visible: false }
            PropertyChanges { target: imageFlick;
                visible: true
                contentWidth: imageFlickView.sourceSize.width
                contentHeight: imageFlickView.sourceSize.height
            }
            PropertyChanges { target: imageFlickView
                fillMode: Image.Null
                width: sourceSize.width
                height: sourceSize.height
            }
        }
    ]

    Component.onCompleted: {
        console.debug("imageViewPage onCompleted imageGrid.currentIndex " + imageGrid.currentIndex);
    }

    ToolBar {
        id: imageViewToolBar
        anchors.bottom: parent.bottom
        z: 2
        visible: false

        tools: ToolBarLayout {
            ToolButton {
                id: backButton
                iconSource: "toolbar-back"

                onClicked: {
                    pageStack.pop();
                }
            }

            ToolButton {
                id: switchButton
                iconSource: "toolbar-refresh"

                onClicked: {
                    imageViewPage.showGrid = !imageViewPage.showGrid;
                }
            }

            ToolButton {
                id: printButton
                iconSource: "print.svg"

                onClicked: {
                    var p = pageStack.find(function (page) { return page.name == "folderPage"; });
                    if (p) p.printFileSlot(imageGrid.getViewFilePath(), -1);
                }
            }
        }
    }

    Rectangle {
        id: imageLabel
        anchors.top: parent.top
        width: parent.width
        height: 40
        z: 2
        color: "black"
        opacity: 0.7
        visible: false

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            color: "white"
            text: imageGrid.getViewFilePath()
        }
    }

    GridView {
        id: imageGrid
        anchors.fill: parent
        cellWidth: width
        cellHeight: height
        cacheBuffer: cellWidth * 3
        flow: GridView.TopToBottom
        snapMode: GridView.SnapOneRow
        delegate: imageViewDelegate
        pressDelay: 100

        property int viewIndex: getViewIndex()

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

        Component.onCompleted: {
            console.debug("imageGrid onCompleted");
        }

        onWidthChanged: {
            console.debug("imageGrid onWidthChanged width " + width);
            console.debug("imageGrid onWidthChanged cellWidth " + cellWidth);
        }

        onHeightChanged: {
            console.debug("imageGrid onHeightChanged height " + height);
            console.debug("imageGrid onHeightChanged cellHeight " + cellHeight);
        }

        onMovementEnded: {
            imageGrid.currentIndex = viewIndex;
            console.debug("imageGrid onMovementEnded viewIndex " + viewIndex);
            console.debug("imageGrid onMovementEnded currentIndex " + currentIndex);
            console.debug("imageGrid onMovementEnded currentItem.width " + currentItem.width + " currentItem.height " + currentItem.height);
            console.debug("imageGrid onMovementEnded currentItem.sourceSize.width " + currentItem.sourceSize.width + " currentItem.sourceSize.height " + currentItem.sourceSize.height);
        }

        onFlickEnded: {
            imageGrid.currentIndex = viewIndex;
            console.debug("imageGrid onFlickEnded viewIndex " + viewIndex);
            console.debug("imageGrid onFlickEnded currentIndex " + currentIndex);
            console.debug("imageGrid onFlickEnded currentItem.width " + currentItem.width + " currentItem.height " + currentItem.height);
            console.debug("imageGrid onFlickEnded currentItem.sourceSize.width " + currentItem.sourceSize.width + " currentItem.sourceSize.height " + currentItem.sourceSize.height);
        }
    }

    Component {
        id: imageViewDelegate

        Image {
            id: imageView
            source: (imageViewPage.status == PageStatus.Active) ? ("image://local/" + absolutePath) : ""
            sourceSize.width: imageGrid.cellWidth
            sourceSize.height: imageGrid.cellHeight
            width: imageGrid.cellWidth
            height: imageGrid.cellHeight
            fillMode: Image.PreserveAspectFit

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
                if (status == Image.Ready) {
                    console.debug("imageView onStatusChanged isSelected index " + index + " Ready");
                    console.debug("imageView onStatusChanged width " + width + " height " + height);
                    console.debug("imageView onStatusChanged sourceSize.width " + sourceSize.width + " sourceSize.height " + sourceSize.height);
                    console.debug("imageView onStatusChanged paintedWidth " + paintedWidth + " paintedHeight " + paintedHeight);

                    if (isSelected) {
                        imageGrid.currentIndex = index;
                        console.debug("imageView onStatusChanged imageGrid.currentIndex " + imageGrid.currentIndex + " imageGrid.count " + imageGrid.count);
                        // *** Issue: Code below cause application crash if index is out of bound.
                        imageGrid.positionViewAtIndex(index, GridView.Contain);
                    }
                }
            }

            MouseArea {
                id: imageMouseArea
                anchors.fill: parent

                onClicked: {
                    imageViewToolBar.visible = !imageViewToolBar.visible;
                    imageLabel.visible = !imageLabel.visible;
                }

                onDoubleClicked: {
                    // TODO Send relative mouseX, mouseY, paintedWidth, paintedHeight to flick.
                    console.debug("imageView onDoubleClicked mouseX " + mouseX + " mouseY " + mouseY)
                    var left = (imageView.width / 2) - (imageView.paintedWidth / 2);
                    var right = (imageView.width / 2) + (imageView.paintedWidth / 2);
                    var top = (imageView.height / 2) - (imageView.paintedHeight / 2);
                    var bottom = (imageView.height / 2) + (imageView.paintedHeight / 2);
                    console.debug("imageView onDoubleClicked left " + left + " right " + right + " top " + top + " bottom " + bottom);
                    imageFlick.gridMouseX = Utility.limit(mouseX, left, right) - left;
                    imageFlick.gridMouseY = Utility.limit(mouseY, top, bottom) - top;
                    imageFlick.gridPaintedWidth = parent.paintedWidth;
                    imageFlick.gridPaintedHeight = parent.paintedHeight;
                    imageViewPage.showGrid = false;
                }
            }
        }
    }

    Flickable {
        id: imageFlick
        visible: false
        width: parent.width
        height: parent.height

        property int gridMouseX
        property int gridMouseY
        property int gridPaintedWidth
        property int gridPaintedHeight

        onVisibleChanged: {
            if (visible) {
                console.debug("-------------------onVisibleChanged---------------------");
                console.debug("imageFlick.width " + imageFlick.width + " imageFlick.height " + imageFlick.height);
                console.debug("imageFlick.contentWidth " + imageFlick.contentWidth + " imageFlick.contentHeight " + imageFlick.contentHeight);
                console.debug("imageFlick.contentX " + imageFlick.contentX + " imageFlick.contentY " + imageFlick.contentY);
                console.debug("imageFlickView.fillMode " + imageFlickView.fillMode);
                console.debug("imageFlickView.width " + imageFlickView.width + " imageFlickView.height " + imageFlickView.height);
                console.debug("imageFlickView.sourceSize.width " + imageFlickView.sourceSize.width + " imageFlickView.sourceSize.height " + imageFlickView.sourceSize.height);
                console.debug("imageFlickView.paintedWidth " + imageFlickView.paintedWidth + " imageFlickView.paintedHeight " + imageFlickView.paintedHeight);
            }
        }

        Image {
            id: imageFlickView
//            width: parent.width
//            height: parent.height
//            fillMode: Image.PreserveAspectFit
            source: (parent.visible) ? imageGrid.getViewFilePath() : ""

            onStatusChanged: {
                if (status == Image.Ready) {
                    console.debug("------------------onStatusChanged----------------------");
                    console.debug("imageFlick.gridMouseX " + imageFlick.gridMouseX + " imageFlick.gridMouseY " + imageFlick.gridMouseY);
                    console.debug("imageFlick.gridPaintedWidth " + imageFlick.gridPaintedWidth + " imageFlick.gridPaintedHeight " + imageFlick.gridPaintedHeight);
                    console.debug("imageFlick.width " + imageFlick.width + " imageFlick.height " + imageFlick.height);
                    console.debug("imageFlickView.fillMode " + imageFlickView.fillMode);
                    console.debug("imageFlickView.width " + imageFlickView.width + " imageFlickView.height " + imageFlickView.height);
                    console.debug("imageFlickView.sourceSize.width " + imageFlickView.sourceSize.width + " imageFlickView.sourceSize.height " + imageFlickView.sourceSize.height);
                    console.debug("imageFlickView.paintedWidth " + imageFlickView.paintedWidth + " imageFlickView.paintedHeight " + imageFlickView.paintedHeight);

                    var actualCenterX = imageFlick.gridMouseX * (imageFlickView.sourceSize.width / imageFlick.gridPaintedWidth);
                    var actualCenterY = imageFlick.gridMouseY * (imageFlickView.sourceSize.height / imageFlick.gridPaintedHeight);
                    console.debug("imageFlickView onStatusChanged actualCenterX " + actualCenterX + " actualCenterY " + actualCenterY);
                    imageFlick.contentX = Utility.limit(actualCenterX - (imageFlick.width / 2), 0, imageFlickView.sourceSize.width - imageFlick.width);
                    imageFlick.contentY = Utility.limit(actualCenterY - (imageFlick.height / 2), 0, imageFlickView.sourceSize.height - imageFlick.height);
                    console.debug("imageFlick.contentX " + imageFlick.contentX + " imageFlick.contentY " + imageFlick.contentY);
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

            PinchArea {
                id: imagePinchArea
                anchors.fill: parent
                pinch.dragAxis: Pinch.XandYAxis

                onPinchStarted: {
                    console.debug("imagePinchArea onPinchStarted");
                    imageViewPage.showGrid = false;
                }
                onPinchFinished: {
                    console.debug("imagePinchArea onPinchFinished");
                    var isPortrait = imageFlick.width < imageFlick.height;
                    var isViewPortrait = imageFlickView.width < imageFlickView.height;

                    // TODO if image size == fit size, change state to grid.
                    // Show grid if width or height of image fit to flick view.
                    imageViewPage.showGrid = (imageFlick.width == imageFlickView.width || imageFlick.height == imageFlickView.height);
                }
                onPinchUpdated: {
                    // TODO image can be shrink to smaller than fit size. it will be enlarged back to fit.
                    // TODO image can be enlarged to larger than actual size. it will be shrink back to actual.
                    console.debug("imagePinchArea onPinchUpdated pinch.scale " + pinch.scale);
                    var newWidth = Math.round(imageFlickView.width * pinch.scale);
                    newWidth = Utility.limit(newWidth, imageFlick.width, imageFlickView.sourceSize.width);
                    var newHeight = Math.round(imageFlickView.height * pinch.scale);
                    newHeight = Utility.limit(newHeight, imageFlick.height, imageFlickView.sourceSize.height);

                    imageFlickView.width = newWidth;
                    imageFlickView.height = newHeight;
                    // TODO adjust x,y to keep current center of image.

                    // TODO if image size == fit size, change state to grid.
                }

                MouseArea {
                    anchors.fill: parent

                    onClicked: {
                        imageViewToolBar.visible = !imageViewToolBar.visible;
                        imageLabel.visible = !imageLabel.visible;
                    }

                    onDoubleClicked: {
                        console.debug("imageFlick onDoubleClicked");
                        imageViewPage.showGrid = true;
                    }
                }
            }
        }
    }
}
