import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

Page {
    id: imageViewPage

    property string name: "imageViewPage"
    property string sources
    property string fileName
    property bool showGrid: true

    state: "grid"
    states: [
        State {
            name: "grid"
            when: showGrid
            PropertyChanges { target: imageGrid; visible: true }
            PropertyChanges { target: imageFlick; visible: false }
        },
        State {
            name: "flick"
            when: !showGrid
            PropertyChanges { target: imageGrid; visible: false }
            PropertyChanges { target: imageFlick; visible: true }
        }
    ]

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
                    if (p) p.printFileSlot(source, -1);
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
            text: "fileName"
        }
    }

    function getImageSourcesModel(jsonText, fileName) {
        console.debug("getImageSourcesModel jsonText " + jsonText + " fileName " + fileName);

        var supportedFileTypes = ["JPG", "PNG", "SVG"];

        // Construct model.
        var model = Qt.createQmlObject(
                    'import QtQuick 1.1; ListModel {}', imageViewPage);

        if (jsonText != "") {
            var json = JSON.parse(jsonText);

            for (var i=0; i<json.length; i++)
            {
                console.debug("i = " + i + " json[i].absolute_path " + json[i].absolute_path);
                if (json[i].is_dir) {
                    // skip dir
                } else if (supportedFileTypes.indexOf(json[i].file_type.toUpperCase()) != -1) {
                    console.debug("model.append " + json[i].absolute_path);
                    model.append({
                                     name: json[i].name,
                                     absolutePath: json[i].absolute_path,
                                     isSelected: (fileName == json[i].name)
                                 });
                }
            }
        }

        return model;
    }

    GridView {
        id: imageGrid
        width: parent.width
        height: parent.height
        model: getImageSourcesModel(sources, fileName)
        cellWidth: parent.width
        cellHeight: parent.height
        cacheBuffer: cellWidth * 3
        flow: GridView.TopToBottom
        snapMode: GridView.SnapOneRow
        delegate: imageViewDelegate
    }

    Component {
        id: imageViewDelegate

        Image {
            id: imageView
            source: absolutePath
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
                if (status == Image.Loading) {
                    if (isSelected) {
                        imageGrid.currentIndex = index;
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
            }
        }
    }

    Flickable {
        id: imageFlick
        visible: false
        width: parent.width
        height: parent.height

        Image {
            id: imageFlickView
            width: parent.width
            height: parent.height
            source: imageGrid.model.get(imageGrid.currentIndex).absolutePath
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
        }

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
            },
            State {
                name: "actual"
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

        PinchArea {
            id: imagePinchArea
            anchors.fill: parent
            pinch.dragAxis: Pinch.XandYAxis

            onPinchStarted: {
                console.debug("imagePinchArea onPinchStarted");
            }
            onPinchFinished: {
                console.debug("imagePinchArea onPinchFinished");
            }
            onPinchUpdated: {
                console.debug("imagePinchArea onPinchUpdated pinch.scale " + pinch.scale);
                var newWidth = Math.round(imageFlickView.width * pinch.scale);
                newWidth = Utility.limit(newWidth, imageFlick.width, imageFlickView.sourceSize.width);
                var newHeight = Math.round(imageFlickView.height * pinch.scale);
                newHeight = Utility.limit(newHeight, imageFlick.height, imageFlickView.sourceSize.height);

                imageFlickView.width = newWidth;
                imageFlickView.height = newHeight;
                // TODO adjust x,y to keep current center of image.
            }
        }
    }
}
