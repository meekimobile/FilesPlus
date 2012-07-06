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
        opacity: 0.7
        z: 1
        visible: false

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            color: "white"
            text: textViewPage.filePath
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
            font.pointSize: 16
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
