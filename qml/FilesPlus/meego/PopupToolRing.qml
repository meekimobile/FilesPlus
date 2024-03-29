import QtQuick 1.1
import com.nokia.meego 1.0
import "Utility.js" as Utility

Rectangle {
    id: popupToolPanel
    x: 0; y: 0; z: 2;
    radius: ringRadius
    color: "transparent"
    visible: false

    state: "main"
    states: [
        State {
            name: "main"
            PropertyChanges {
                target: buttonRing
                model: mainButtonModel
            }
            PropertyChanges {
                target: toolButton
                iconSource: (theme.inverted) ? "toolbar_extension.svg" : "toolbar_extension_inverted.svg"
            }
        },
        State {
            name: "tools"
            PropertyChanges {
                target: buttonRing
                model: toolsButtonModel
            }
            PropertyChanges {
                target: toolButton
                iconSource: (theme.inverted) ? "back.svg" : "back_inverted.svg"
            }
        },
        State {
            name: "share"
            PropertyChanges {
                target: buttonRing
                model: shareButtonModel
            }
            PropertyChanges {
                target: toolButton
                iconSource: (theme.inverted) ? "back.svg" : "back_inverted.svg"
            }
        },
        State {
            name: "cloud"
            PropertyChanges {
                target: buttonRing
                model: cloudButtonModel
            }
            PropertyChanges {
                target: toolButton
                iconSource: (theme.inverted) ? "back.svg" : "back_inverted.svg"
            }
        }
    ]

    property bool isDir
    property string srcFilePath
    property string selectedFilePath
    property int selectedFileIndex
    property string selectedFileName
    property bool isCopy
    property string pastePath
    property alias timeout: popupTimer.interval

    property int ringRadius: 60
    property int buttonRadius: 25

    property alias model: buttonRing.model
    property variant disabledButtons: []

    signal opened()
    signal closed()
    signal cutClicked(string sourcePath)
    signal copyClicked(string sourcePath)
    signal pasteClicked(string targetPath)
    signal deleteFile(string sourcePath)
    signal printFile(string srcFilePath, int srcItemIndex)
    signal syncFile(string srcFilePath, int srcItemIndex)
    signal showTools(string srcFilePath, int srcItemIndex)
    signal newFolder(string srcFilePath, int srcItemIndex)
    signal markClicked(string srcFilePath, int srcItemIndex)
    signal renameFile(string srcFilePath, int srcItemIndex)
    signal uploadFile(string srcFilePath, int srcItemIndex)
    signal downloadFile(string srcFilePath, int srcItemIndex)
    signal unsyncFile(string srcFilePath, int srcItemIndex)
    signal disconnectFile(string srcFilePath, int srcItemIndex)
    signal browseRemoteFile(string srcFilePath, int srcItemIndex)
    signal scheduleSyncFile(string srcFilePath, int srcItemIndex)
    signal shareFile(string srcFilePath, int srcItemIndex)
    signal shareUrl(string srcFilePath, int srcItemIndex)
    signal mailFile(string srcFilePath, int srcItemIndex)
    signal smsFile(string srcFilePath, int srcItemIndex)
    signal bluetoothFile(string srcFilePath, int srcItemIndex)
    signal editFile(string srcFilePath, int srcItemIndex)
    signal showInfo(string srcFilePath, int srcItemIndex)
    signal compress(string srcFilePath, int srcItemIndex)

    function open(panelX, panelY) {
//        console.debug("popupToolRing open panelX " + panelX + " panelY " + panelY);

        popupToolPanel.width = popupToolPanel.ringRadius * 2;
        popupToolPanel.height = popupToolPanel.ringRadius * 2;
        popupToolPanel.x = panelX - (popupToolPanel.width / 2);
        popupToolPanel.y = panelY - (popupToolPanel.height / 2);

        if (popupToolPanel.x > (parent.width - popupToolPanel.width - popupToolPanel.buttonRadius) ) {
            popupToolPanel.x = (parent.width - popupToolPanel.width - popupToolPanel.buttonRadius);
        } else if (popupToolPanel.x < popupToolPanel.buttonRadius) {
            popupToolPanel.x = popupToolPanel.buttonRadius;
        }

        if (popupToolPanel.y > (parent.height - popupToolPanel.height - popupToolPanel.buttonRadius) ) {
            popupToolPanel.y = (parent.height - popupToolPanel.height - popupToolPanel.buttonRadius);
        } else if (popupToolPanel.y < popupToolPanel.buttonRadius) {
            popupToolPanel.y = popupToolPanel.buttonRadius;
        }

        popupToolPanel.visible = true;
    }
    
    onVisibleChanged: {
        if (visible) {
            opened();
            state = "main";
            popupTimer.restart();
        } else {
            selectedFilePath = "";
            selectedFileIndex = -1;
            srcFilePath = "";
            pastePath = "";
            closed();
        }
    }
    
    Timer {
        id: popupTimer
        interval: 2000
        running: false;
        onTriggered: parent.visible = false
    }
    
    ListModel {
        id: mainButtonModel
        ListElement { buttonName: "copy"; icon: "copy.svg" }
        ListElement { buttonName: "paste"; icon: "paste.svg" }
        ListElement { buttonName: "print"; icon: "print.svg" }
        ListElement { buttonName: "info"; icon: "info.svg" }
        ListElement { buttonName: "delete"; icon: "delete.svg" }
        ListElement { buttonName: "cut"; icon: "trim.svg" }
    }

    ListModel {
        id: toolsButtonModel
        ListElement { buttonName: "mark"; icon: "ok.svg" }
        ListElement { buttonName: "editFile"; icon: "edit.svg" }
        ListElement { buttonName: "compress"; icon: "compress.svg" }
        ListElement { buttonName: "cloud"; icon: "cloud_options.svg" }
        ListElement { buttonName: "share"; icon: "share.svg" }
        ListElement { buttonName: "rename"; icon: "rename.svg" }
    }

    ListModel {
        id: shareButtonModel
        ListElement { buttonName: "shareFile"; icon: "notes.svg" } // For Meego only.
        ListElement { buttonName: "shareUrl"; icon: "bookmark.svg" } // For Meego only.
//        ListElement { buttonName: "mail"; icon: "mail.svg" }
//        ListElement { buttonName: "sms"; icon: "messaging.svg" }
//        ListElement { buttonName: "bluetooth"; icon: "bluetooth.svg" }
    }

    ListModel {
        id: cloudButtonModel
        ListElement { buttonName: "disconnect"; icon: "cloud_disconnect.svg" }
        ListElement { buttonName: "upload"; icon: "upload.svg" }
        ListElement { buttonName: "cloudScheduler"; icon: "cloud_wait.svg" }
        ListElement { buttonName: "sync"; icon: "cloud.svg" }
//        ListElement { buttonName: "cloudSettings"; icon: "cloud_settings.svg" }
        ListElement { buttonName: "unsync"; icon: "cloud_remove.svg" }
        ListElement { buttonName: "download"; icon: "download.svg" }
    }

    Component {
        id: buttonDelegate
        Button {
            enabled: isButtonVisible(buttonName)
            width: popupToolPanel.buttonRadius * 2
            height: popupToolPanel.buttonRadius * 2
            iconSource: getIcon(icon)
            onClicked: buttonHandler(buttonName, index)
        }
    }

    function getIcon(iconSource) {
        return theme.inverted ? iconSource : Utility.replace(iconSource, ".svg", "_inverted.svg");
    }

    function isButtonVisible(buttonName) {
        if (selectedFilePath == "") return false;

        if (disabledButtons.indexOf(buttonName) != -1) return false;

        return isButtonVisibleCallback(buttonName);
    }

    // Needs to be overriden in host QML.
    function isButtonVisibleCallback(buttonName) {
        return true;
    }

    function buttonHandler(buttonName, index) {
//        console.debug("button " + index + " buttonName " + buttonName);
        if (buttonName == "tool") {
            if (popupToolPanel.state == "main") {
                popupToolPanel.state = "tools";
            } else if (popupToolPanel.state == "tools") {
                popupToolPanel.state = "main";
            } else if (popupToolPanel.state == "share") {
                popupToolPanel.state = "tools";
            } else if (popupToolPanel.state == "cloud") {
                popupToolPanel.state = "tools";
            }
            popupTimer.restart();
            return;
        } else if (buttonName == "paste") {
            pasteClicked(pastePath);
        } else if (buttonName == "cut") {
            cutClicked(selectedFilePath);
            popupToolPanel.srcFilePath = selectedFilePath;
            popupToolPanel.isCopy = false;
        } else if (buttonName == "copy") {
            copyClicked(selectedFilePath);
            popupToolPanel.srcFilePath = selectedFilePath;
            popupToolPanel.isCopy = true;
        } else if (buttonName == "print") {
            printFile(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "delete") {
            deleteFile(selectedFilePath);
        } else if (buttonName == "sync") {
            syncFile(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "newFolder") {
            newFolder(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "mark") {
            markClicked(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "rename") {
            renameFile(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "cloud") {
            popupToolPanel.state = "cloud";
            popupTimer.restart();
            return;
        } else if (buttonName == "upload") {
            uploadFile(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "download") {
            downloadFile(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "unsync") {
            unsyncFile(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "disconnect") {
            disconnectFile(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "cloudScheduler") {
            scheduleSyncFile(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "cloudSettings") {
            browseRemoteFile(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "share") {
            popupToolPanel.state = "share";
            popupTimer.restart();
            return;
        } else if (buttonName == "shareFile") {
            shareFile(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "shareUrl") {
            shareUrl(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "mail") {
            mailFile(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "sms") {
            smsFile(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "bluetooth") {
            bluetoothFile(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "editFile") {
            editFile(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "info") {
            showInfo(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "compress") {
            compress(selectedFilePath, selectedFileIndex);
        }
        popupToolPanel.visible = false;
    }

    PathView {
        id: buttonRing
        width: parent.width
        height: parent.height
        focus: true
        delegate: buttonDelegate
        visible: true
        interactive: false
        path: Path {
            startX: buttonRing.width / 2
            startY: buttonRing.y
            PathQuad { x: buttonRing.width; y: buttonRing.height / 2; controlX: buttonRing.width; controlY: buttonRing.y }
            PathQuad { x: buttonRing.width /  2; y: buttonRing.height; controlX: buttonRing.width; controlY: buttonRing.height }
            PathQuad { x: buttonRing.x; y: buttonRing.height / 2; controlX: buttonRing.x; controlY: buttonRing.height }
            PathQuad { x: buttonRing.width / 2; y: buttonRing.y; controlX: buttonRing.x; controlY: buttonRing.y }
        }
    }

    Button {
        id: toolButton
        enabled: isButtonVisible("tool")
        anchors.centerIn: parent
        width: popupToolPanel.buttonRadius * 2
        height: popupToolPanel.buttonRadius * 2


        onClicked: {
            buttonHandler("tool", -1);
        }
    }
}
