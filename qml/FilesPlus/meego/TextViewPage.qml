import QtQuick 1.1
import com.nokia.meego 1.0
import SystemInfoHelper 1.0
import "Utility.js" as Utility

Page {
    id: textViewPage

    property string name: "textViewPage"
    property string filePath
    property string fileName

    tools: ToolBarLayout {
        ToolIcon {
            id: backButton
            iconId: "toolbar-back"
            onClicked: {
                pageStack.pop();
            }
        }

        ToolIcon {
            id: openButton
            iconSource: (theme.inverted) ? "notes.svg" : "notes_inverted.svg"
            onClicked: {
                Qt.openUrlExternally(helper.getUrl(filePath));
            }
        }

        ToolIcon {
            id: saveButton
            iconSource: (theme.inverted) ? "save.svg" : "save_inverted.svg"
            onClicked: {
                // Save to file.
                // TODO Opt to convert new line characters to either Windows or Unix format.
                helper.saveFileContent(textViewPage.filePath, textView.text);
                fsModel.removeCache(textViewPage.filePath);
                // Reset cloudDriveModel hash on parent.
                var paths = fsModel.getPathToRoot(textViewPage.filePath);
                for (var i=0; i<paths.length; i++) {
                    console.debug("textViewPage saveButton onClicked updateItems paths[" + i + "] " + paths[i]);
                    cloudDriveModel.updateItems(paths[i], cloudDriveModel.dirtyHash);
                }
            }
        }

        ToolIcon {
            id: printButton
            iconSource: (theme.inverted) ? "print.svg" : "print_inverted.svg"
            onClicked: {
                gcpClient.printFileSlot(textViewPage.filePath, -1);
            }
        }
    }

    SystemInfoHelper {
        id: helper
    }

    TitlePanel {
        id: textLabel
        horizontalAlignment: Text.AlignHCenter
        text: textViewPage.fileName
    }

    Rectangle {
        id: background
        anchors.top: flick.top
        width: flick.width
        height: flick.height
        color: "white"
    }

    Rectangle {
        id: flickScrollbar
        anchors.right: flick.right
        anchors.rightMargin: 0
        radius: 2
        y: flick.visibleArea.yPosition * flick.height + textLabel.height
        z: 1
        width: 4
        height: flick.visibleArea.heightRatio * flick.height
        color: "grey"
        visible: flick.moving
    }

//     TextArea is a superset of TextEdit to implement the Symbian-style look-and-feel.
//     Meego needs to use Flickable + TextArea.
//     TODO Externalize to component.
    Flickable {
        id: flick
        anchors.top: textLabel.bottom
        anchors.left: parent.left
        width: parent.width
        height: parent.height - textLabel.height
        contentWidth: textView.width
        contentHeight: textView.height
        flickableDirection: Flickable.VerticalFlick
        clip: true
        pressDelay: 200

        TextArea {
            id: textView
            enabled: true
            readOnly: false
            width: textViewPage.width
            font.pointSize: 16
            wrapMode: TextEdit.WordWrap
            textFormat: TextEdit.AutoText
            text: helper.getFileContent(textViewPage.filePath)
            style: TextAreaStyle {
                backgroundError: ""
                backgroundDisabled: ""
                backgroundSelected: ""
                background: ""
            }

//            onActiveFocusChanged: {
//                console.debug("textView onActiveFocusChanged " + textView.activeFocus);
//            }

//            onHeightChanged: {
//                console.debug("textView onHeightChanged " + textView.height);
//            }

            onImplicitHeightChanged: {
                console.debug("textView onImplicitHeightChanged " + textView.implicitHeight);
                height = implicitHeight;
            }

            property int startFontSize

            states: [
                State {
                    name: "view"
                    when: !textView.activeFocus
                },
                State {
                    name: "edit"
                    when: textView.activeFocus
                }
            ]

            PinchArea {
                id: textViewPinchArea
                enabled: !textView.activeFocus
                anchors.fill: parent
                pinch.dragAxis: Pinch.XandYAxis

                onPinchStarted: {
//                    console.debug("textView onPinchStarted textView.cursorPosition " + textView.cursorPosition);
                    textView.startFontSize = textView.font.pointSize;
                }
                onPinchFinished: {
//                    console.debug("textView onPinchFinished textView.cursorPosition " + textView.cursorPosition);
                }
                onPinchUpdated: {
//                    console.debug("textView onPinchUpdated pinch.scale " + pinch.scale);
                    var newFontSize = Math.round(textView.startFontSize * pinch.scale);
                    newFontSize = Utility.limit(newFontSize, 6, 24);

                    textView.font.pointSize = newFontSize;
                }
            }
        }
    }

    Component.onCompleted: {
        console.debug("textViewPage filePath " + textViewPage.filePath);
    }
}
