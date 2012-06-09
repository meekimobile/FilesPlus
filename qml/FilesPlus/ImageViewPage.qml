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
            PropertyChanges { target: imageFlick; visible: false; showFit: true }
        },
        State {
            name: "flick"
            when: !showGrid
            PropertyChanges { target: imageGrid; visible: false }
            PropertyChanges { target: imageFlick; visible: true; showFit: false }
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
                    console.debug("imageView mouseX " + mouseX + " mouseY " + mouseY)
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

        property bool showFit: true

        onStateChanged: {
            console.debug("------------------- " + state + "---------------------");
            console.debug("imageFlick.width " + imageFlick.width + " imageFlick.height " + imageFlick.height);
            console.debug("imageFlick.contentWidth " + imageFlick.contentWidth + " imageFlick.contentHeight " + imageFlick.contentHeight);
            console.debug("imageFlick.contentX " + imageFlick.contentX + " imageFlick.contentY " + imageFlick.contentY);
//            console.debug("imageView index " + index + " absolutePath " + absolutePath + " name " + name + " isSelected " + isSelected + " -----------------------------");
//            console.debug("imageView.fillMode " + imageView.fillMode + " mouseX " + imageMouseArea.mouseX + " mouseY " + imageMouseArea.mouseY);
//            console.debug("imageView.width " + imageView.width + " imageView.height " + imageView.height);
//            console.debug("imageView.sourceSize.width " + imageView.sourceSize.width + " imageView.sourceSize.height " + imageView.sourceSize.height);
        }

        state: "fit"
        states: [
            State {
                name: "fit"
                when: imageFlick.showFit
                PropertyChanges {
                    target: imageFlickView
                    fillMode: Image.PreserveAspectFit
                    width: parent.width
                    height: parent.height
                }
                PropertyChanges {
                    target: imageFlick
                    contentWidth: width
                    contentHeight: height
                }
            },
            State {
                name: "actual"
                when: !imageFlick.showFit
                PropertyChanges {
                    target: imageFlickView
                    fillMode: Image.Null
                    width: sourceSize.width
                    height: sourceSize.height
                }
                PropertyChanges {
                    target: imageFlick
                    contentWidth: imageFlickView.sourceSize.width
                    contentHeight: imageFlickView.sourceSize.height
                }
            }
        ]

        Image {
            id: imageFlickView
            width: parent.width
            height: parent.height
            source: (parent.visible) ? imageGrid.getViewFilePath() : ""
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

                    onDoubleClicked: {
                        console.debug("imageFlick onDoubleClicked");
                        imageFlick.showFit = !imageFlick.showFit;
                        imageViewPage.showGrid = imageFlick.showFit;
                    }
                }
            }
        }
    }
}
