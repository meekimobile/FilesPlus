import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

Page {
    id: imageViewPage

    property string name: "imageViewPage"
    property string sources
    property string fileName

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
                id: printButton
                iconSource: "print.svg"

                onClicked: {
                    var p = pageStack.find(function (page) { return page.name == "folderPage"; });
                    if (p) p.printFileSlot(source, -1);
                }
            }
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
                    text: name
                }
            }

            BusyIndicator {
                id: imageViewBusy
                width: 80
                height: 80
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                visible: (parent.progress < 1)
                running: (parent.progress < 1)
            }

            onStatusChanged: {
                if (status == Image.Loading) {
                    if (isSelected) {
                        imageGrid.currentIndex = index;
                    }
                }
            }

//            Component.onCompleted: {
//                console.debug("imageView index " + index + " absolutePath " + absolutePath + " name " + name + " isSelected " + isSelected + " -----------------------------");
//                console.debug("imageView.fillMode " + imageView.fillMode + " mouseX " + imageMouseArea.mouseX + " mouseY " + imageMouseArea.mouseY);
//                console.debug("imageView.width " + imageView.width + " imageView.height " + imageView.height);
//                console.debug("imageView.sourceSize.width " + imageView.sourceSize.width + " imageView.sourceSize.height " + imageView.sourceSize.height);
//            }

            MouseArea {
                id: imageMouseArea
                anchors.fill: parent

                onClicked: {
                    imageViewToolBar.visible = !imageViewToolBar.visible;
                    imageLabel.visible = !imageLabel.visible;
                }

                onDoubleClicked: {
//                    if (imageFlick.state == "fit") {
//                        imageFlick.state = "actual";
//                    } else {
//                        imageFlick.state = "fit";
//                    }
                }
            }
        }
    }

//    Flickable {
//        id: imageFlick
//        visible: false
//        width: parent.width
//        height: parent.height

//        onStateChanged: {
//            console.debug("------------------- " + state + "---------------------");
//            console.debug("imageFlick.width " + imageFlick.width + " imageFlick.height " + imageFlick.height);
//            console.debug("imageFlick.contentWidth " + imageFlick.contentWidth + " imageFlick.contentHeight " + imageFlick.contentHeight);
//            console.debug("imageFlick.contentX " + imageFlick.contentX + " imageFlick.contentY " + imageFlick.contentY);
//        }

//        state: "fit"
//        states: [
//            State {
//                name: "fit"
//            },
//            State {
//                name: "actual"
//                PropertyChanges {
//                    target: imageView
//                    fillMode: Image.Null
//                    width: sourceSize.width
//                    height: sourceSize.height
//                }
//                PropertyChanges {
//                    target: imageFlick
//                    contentWidth: imageView.sourceSize.width
//                    contentHeight: imageView.sourceSize.height
//                }
//            }
//        ]
//    }
}
