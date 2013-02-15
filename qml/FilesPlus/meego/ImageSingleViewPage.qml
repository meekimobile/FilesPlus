import QtQuick 1.1
import com.nokia.meego 1.0
import "Utility.js" as Utility

Page {
    id: imageViewPage
    tools: null

    property string name: "imageViewPage"
    property string imageSource: ""
    property string imageName: ""
    property bool inverted: !theme.inverted
    property bool showTools: true
    property real maxZoomFactor: 2

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
                    if (imageSource.indexOf("http") == 0) {
                        Qt.openUrlExternally(imageSource);
                    } else {
                        Qt.openUrlExternally(helper.getUrl(imageSource));
                    }
                }
            }

            ToolBarButton {
                id: printButton
                buttonIconSource: (!inverted) ? "print.svg" : "print_inverted.svg"
                onClicked: {
                    if (imageSource.indexOf("http") == 0) {
                        gcpClient.printURLSlot(imageSource);
                    } else {
                        gcpClient.printFileSlot(imageSource, -1);
                    }
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
        opacity: 0.5
        visible: showTools

        Text {
            id: imageLabelText
            text: imageName
            anchors.fill: parent
            anchors.margins: 5
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: "white"
            elide: Text.ElideMiddle
            font.pointSize: 18
        }
    }

    Flickable {
        id: imageFlick
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
//                console.debug("imageViewPage -------------------onVisibleChanged---------------------");
//                console.debug("imageViewPage imageFlick.width " + imageFlick.width + " imageFlick.height " + imageFlick.height);
//                console.debug("imageViewPage imageFlick.contentWidth " + imageFlick.contentWidth + " imageFlick.contentHeight " + imageFlick.contentHeight);
//                console.debug("imageViewPage imageFlick.contentX " + imageFlick.contentX + " imageFlick.contentY " + imageFlick.contentY);
//                console.debug("imageViewPage imageFlickView.fillMode " + imageFlickView.fillMode);
//                console.debug("imageViewPage imageFlickView.width " + imageFlickView.width + " imageFlickView.height " + imageFlickView.height);
//                console.debug("imageViewPage imageFlickView.sourceSize.width " + imageFlickView.sourceSize.width + " imageFlickView.sourceSize.height " + imageFlickView.sourceSize.height);
//                console.debug("imageViewPage imageFlickView.paintedWidth " + imageFlickView.paintedWidth + " imageFlickView.paintedHeight " + imageFlickView.paintedHeight);
//            }
//        }

        Image {
            id: imageFlickView
            source: imageSource
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
                }
            }

            BusyIndicator {
                id: imageViewBusy
                style: BusyIndicatorStyle { size: "large" }
                anchors.centerIn: parent
                visible: (imageFlickView.progress < 1 && imageFlickView.status == Image.Loading)
                running: visible
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
                    }
                }
            }
        }
    }
}
