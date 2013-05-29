import QtQuick 1.1
import com.nokia.symbian 1.1
import SystemInfoHelper 1.0
import "Utility.js" as Utility

Page {
    id: textViewPage

    property string name: "textViewPage"
    property string filePath
    property string fileName

    function saveFileSlot(useWindowsEOL) {
        // Save to file.
        // TODO Opt to convert new line characters to either Windows or Unix format.
        var r = helper.saveFileContent(textViewPage.filePath, textView.text, useWindowsEOL);
        if (r < 0) {
            window.showMessageDialogSlot(qsTr("Text Editor"), qsTr("Can not write to file."));
        } else {
            fsModel.removeCache(textViewPage.filePath);
            // Reset cloudDriveModel hash on parent.
            var paths = fsModel.getPathToRoot(textViewPage.filePath);
            for (var i=0; i<paths.length; i++) {
                console.debug("textViewPage saveFileSlot updateItems paths[" + i + "] " + paths[i]);
                cloudDriveModel.updateItems(paths[i], cloudDriveModel.dirtyHash);
            }
        }
    }

    tools: ToolBarLayout {
        ToolButton {
            id: backButton
            iconSource: "toolbar-back"
            platformInverted: window.platformInverted
            onClicked: {
                pageStack.pop();
            }
        }

        ToolButton {
            id: openButton
            iconSource: (!window.platformInverted) ? "notes.svg" : "notes_inverted.svg"
            platformInverted: window.platformInverted
            onClicked: {
                Qt.openUrlExternally(helper.getUrl(filePath));
            }
        }

        ToolButton {
            id: saveButton
            iconSource: (!window.platformInverted) ? "save.svg" : "save_inverted.svg"
            platformInverted: window.platformInverted
            onClicked: {
                saveMenu.open();
            }
        }

        ToolButton {
            id: printButton
            iconSource: (!window.platformInverted) ? "print.svg" : "print_inverted.svg"
            platformInverted: window.platformInverted
            onClicked: {
                gcpClient.printFileSlot(textViewPage.filePath, -1);
            }
        }
    }

    MenuWithIcon {
        id: saveMenu

        content: MenuLayout {
            id: menuLayout

            MenuItemWithIcon {
                name: "saveForWindows"
                text: appInfo.emptyStr+"Windows "+qsTr("format")
                onClicked: {
                    saveFileSlot(true);
                }
            }

            MenuItemWithIcon {
                name: "saveForUnix"
                text: appInfo.emptyStr+"UNIX "+qsTr("format")
                onClicked: {
                    saveFileSlot(false);
                }
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

    PinchArea {
        anchors.top: textLabel.bottom
        width: parent.width
        height: parent.height - textLabel.height - (inputContext.visible ? (inputContext.height - 50) : 0)
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
            readOnly: false
            anchors.fill: parent
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
