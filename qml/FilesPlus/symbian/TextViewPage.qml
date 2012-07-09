import QtQuick 1.1
import com.nokia.symbian 1.1
import SystemInfoHelper 1.0
import "Utility.js" as Utility

Page {
    id: textViewPage

    property string name: "textViewPage"
    property string filePath
    property string fileName

    tools: ToolBarLayout {
        ToolButton {
            id: backButton
            iconSource: "toolbar-back"

            onClicked: {
                pageStack.pop();
            }
        }

        ToolButton {
            id: openButton
            iconSource: "notes.svg"
            onClicked: {
                Qt.openUrlExternally(helper.getUrl(filePath));
            }
        }

        ToolButton {
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
        z: 1

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            color: "white"
            text: textViewPage.fileName
        }
    }

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
            newFontSize = Utility.limit(newFontSize, 4, 14);

            textView.font.pointSize = newFontSize;
        }

        TextArea {
            id: textView
            enabled: true
            readOnly: true
            anchors.fill: parent
            font.family: "Helvetica"
            font.pointSize: 6
            wrapMode: TextEdit.WordWrap
            textFormat: TextEdit.AutoText
            text: helper.getFileContent(textViewPage.filePath)

            property int startFontSize
        }
    }

    Component.onCompleted: {
        console.debug("textViewPage filePath " + textViewPage.filePath);
    }
}
