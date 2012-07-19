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
            id: printButton
            iconSource: (theme.inverted) ? "print.svg" : "print_inverted.svg"
            onClicked: {
                var p = pageStack.find(function (page) { return page.name == "folderPage"; });
                if (p) p.printFileSlot(textViewPage.filePath, -1);
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

//    Rectangle {
//        id: textLabel
//        anchors.top: parent.top
//        width: parent.width
//        height: 40
//        color: "black"
//        z: 1

//        Text {
//            anchors.fill: parent
//            anchors.margins: 5
//            horizontalAlignment: Text.AlignHCenter
//            verticalAlignment: Text.AlignVCenter
//            color: "white"
//            elide: Text.ElideMiddle
//            font.pointSize: 18
//            text: textViewPage.fileName
//        }
//    }

    BorderImage {
        id: background
        source: "image://theme/meegotouch-textedit-background"
        width: parent.width
        height: parent.height - textLabel.height
        anchors.top: textLabel.bottom
        border.left: 22; border.top: 22
        border.right: 22; border.bottom: 22
    }

    Rectangle {
        id: flickScrollbar
        anchors.right: flick.right
        anchors.rightMargin: -17
        radius: 2
        y: flick.visibleArea.yPosition * flick.height + textLabel.height + flick.anchors.topMargin
        z: 1
        width: 4
        height: flick.visibleArea.heightRatio * flick.height
        color: "grey"
    }

    // TextArea is a superset of TextEdit to implement the Symbian-style look-and-feel.
    // Meego needs to use Flickable + TextEdit.
    Flickable {
        id: flick
        anchors.top: textLabel.bottom
        anchors.topMargin: 22
        anchors.left: parent.left
        anchors.leftMargin: 22
        width: parent.width - 44
        height: parent.height - textLabel.height - 44
        contentWidth: textView.width
        contentHeight: textView.height
        flickableDirection: Flickable.VerticalFlick
        clip: true

        TextEdit {
            id: textView
            enabled: true
            readOnly: true
            width: textViewPage.width - 44
            font.pointSize: 16
            color: "black"
            wrapMode: TextEdit.WordWrap
            textFormat: TextEdit.AutoText
            text: helper.getFileContent(textViewPage.filePath)

            property int startFontSize

            PinchArea {
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

//            MouseArea {
//                anchors.fill: parent

//                onPressAndHold: {
//                    console.debug("textView onPressAndHold");
//                    console.debug("textView width " + textView.width + " height " + textView.height);
//                    console.debug("textViewPage " + textViewPage.width + "," + textViewPage.height);
//                    console.debug("textViewPage flick " + flick.width + "," + flick.height);
//                    console.debug("textViewPage textView " + textView.width + "," + textView.height);
//                    textView.closeSoftwareInputPanel();
//                }
//            }
        }
    }

    Component.onCompleted: {
        console.debug("textViewPage filePath " + textViewPage.filePath);
    }
}
