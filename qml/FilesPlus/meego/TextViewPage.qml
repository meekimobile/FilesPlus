import QtQuick 1.1
import com.nokia.meego 1.1
import SystemInfoHelper 1.0
import "Utility.js" as Utility

Page {
    id: textViewPage

    property string name: "textViewPage"
    property string filePath

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
            iconSource: "notes.svg"
            onClicked: {
                Qt.openUrlExternally(helper.getUrl(filePath));
            }
        }

        ToolIcon {
            id: printButton
            iconSource: "print.svg"
            onClicked: {
                var p = pageStack.find(function (page) { return page.name == "folderPage"; });
                if (p) p.printFileSlot(textViewPage.filePath, -1);
            }
        }
    }

    SystemInfoHelper {
        id: helper
    }

    Rectangle {
        id: textLabel
        anchors.top: parent.top
        width: parent.width
        height: 40
        color: "black"
//        opacity: 0.7
        z: 1

        Text {
            anchors.fill: parent
            anchors.margins: 5
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: "white"
            elide: Text.ElideMiddle
            font.pointSize: 18
            text: textViewPage.filePath
        }
    }

//    Flickable {
//         id: flick

//         width: parent.width
//         height: parent.height - textLabel.height
//         anchors.top: textLabel.bottom
//         contentWidth: textView.paintedWidth
//         contentHeight: textView.paintedHeight
//         clip: true

//         function ensureVisible(r)
//         {
//             if (contentX >= r.x)
//                 contentX = r.x;
//             else if (contentX+width <= r.x+r.width)
//                 contentX = r.x+r.width-width;
//             if (contentY >= r.y)
//                 contentY = r.y;
//             else if (contentY+height <= r.y+r.height)
//                 contentY = r.y+r.height-height;
//         }

    // TextArea is a superset of TextEdit to implement the Symbian-style look-and-feel.
    // TODO Change to Flickable + TextEdit.
    TextArea {
        id: textView
        enabled: true
        readOnly: true
        width: parent.width
        height: 400
//        height: parent.height
//        height: parent.height - textLabel.height
        anchors.top: textLabel.bottom
        font.pointSize: 16
        wrapMode: TextEdit.WordWrap
        textFormat: TextEdit.AutoText
        clip: true
        text: helper.getFileContent(textViewPage.filePath)

        property int startFontSize

        PinchArea {
            anchors.fill: parent
            pinch.dragAxis: Pinch.XandYAxis

            onPinchStarted: {
                console.debug("textView onPinchStarted textView.cursorPosition " + textView.cursorPosition);
                textView.startFontSize = textView.font.pointSize;
            }
            onPinchFinished: {
                console.debug("textView onPinchFinished textView.cursorPosition" + textView.cursorPosition);
            }
            onPinchUpdated: {
                console.debug("textView onPinchUpdated pinch.scale " + pinch.scale);
                var newFontSize = Math.round(textView.startFontSize * pinch.scale);
                newFontSize = Utility.limit(newFontSize, 6, 24);

                textView.font.pointSize = newFontSize;
            }
        }

        MouseArea {
            anchors.fill: parent

            onPressAndHold: {
                console.debug("textView onPressAndHold");
                console.debug("textView width " + textView.width + " height " + textView.height);
                textView.closeSoftwareInputPanel();
            }
        }
    }

//    }

    Component.onCompleted: {
        console.debug("textViewPage filePath " + textViewPage.filePath);
    }
}
