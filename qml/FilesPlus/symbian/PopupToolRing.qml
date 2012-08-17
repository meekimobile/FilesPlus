import QtQuick 1.1
import com.nokia.symbian 1.1
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
                iconSource: (!window.platformInverted) ? "toolbar_extension.svg" : "toolbar_extension_inverted.svg"
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
                iconSource: (!window.platformInverted) ? "toolbar_extension.svg" : "toolbar_extension_inverted.svg"
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
                iconSource: (!window.platformInverted) ? "back.svg" : "back_inverted.svg"
            }
        }
    ]

    property bool isDir
    property string srcFilePath
    property string selectedFilePath
    property int selectedFileIndex
    property bool isCopy
    property string pastePath
    property int clipboardCount
    property alias timeout: popupTimer.interval

    property int ringRadius: 60
    property int buttonRadius: 25

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
    signal unsyncFile(string srcFilePath, int srcItemIndex)
    signal mailFile(string srcFilePath, int srcItemIndex)
    signal smsFile(string srcFilePath, int srcItemIndex)
    signal bluetoothFile(string srcFilePath, int srcItemIndex)

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
            popupTimer.restart();
        } else {
            selectedFilePath = "";
            selectedFileIndex = -1;
            srcFilePath = "";
            pastePath = "";
            state = "main";
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
        ListElement { buttonName: "sync"; icon: "cloud.svg" }
        ListElement { buttonName: "delete"; icon: "delete.svg" }
        ListElement { buttonName: "cut"; icon: "trim.svg" }
    }

    ListModel {
        id: toolsButtonModel
        ListElement { buttonName: "mark"; icon: "ok.svg" }
        ListElement { buttonName: "newFolder"; icon: "folder_add.svg" }
        ListElement { buttonName: "upload"; icon: "upload.svg" }
        ListElement { buttonName: "unsync"; icon: "cloud_remove.svg" }
        ListElement { buttonName: "share"; icon: "share.svg" }
        ListElement { buttonName: "rename"; icon: "rename.svg" }
    }

    ListModel {
        id: shareButtonModel
        ListElement { buttonName: "mail"; icon: "mail.svg" }
        ListElement { buttonName: "sms"; icon: "messaging.svg" }
        ListElement { buttonName: "bluetooth"; icon: "bluetooth.svg" }
    }

    Component {
        id: buttonDelegate
        Button {
            enabled: isButtonVisible(buttonName)
            width: popupToolPanel.buttonRadius * 2
            height: popupToolPanel.buttonRadius * 2
            iconSource: getIcon(icon)
            platformInverted: window.platformInverted
            onClicked: buttonHandler(buttonName, index)
        }
    }

    function getIcon(iconSource) {
        return !window.platformInverted ? iconSource : Utility.replace(iconSource, ".svg", "_inverted.svg");
    }

    function isButtonVisible(buttonName) {
        if (selectedFilePath == "") return false;

        if (buttonName === "sync") {
            return !fsModel.isRoot(selectedFilePath) && cloudDriveModel.canSync(selectedFilePath);
        } else if (buttonName === "upload") {
            return !fsModel.isRoot(selectedFilePath) && cloudDriveModel.canSync(selectedFilePath);
        } else if (buttonName === "unsync") {
            return cloudDriveModel.isConnected(selectedFilePath);
        } else if (buttonName === "paste") {
            return (clipboardCount > 0);
        } else if (buttonName == "mail") {
            return cloudDriveModel.isConnected(selectedFilePath);
        } else if (buttonName == "sms") {
            return cloudDriveModel.isConnected(selectedFilePath);
        } else if (buttonName == "bluetooth") {
            return fsModel.isFile(selectedFilePath);
        }

        return true;
    }

    function buttonHandler(buttonName, index) {
//        console.debug("button " + index + " buttonName " + buttonName);
        if (buttonName == "paste") {
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
        } else if (buttonName == "upload") {
            uploadFile(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "share") {
            popupToolPanel.state = "share";
            return;
        } else if (buttonName == "mail") {
            mailFile(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "sms") {
            smsFile(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "bluetooth") {
            bluetoothFile(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "unsync") {
            unsyncFile(selectedFilePath, selectedFileIndex);
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
        anchors.centerIn: parent
        width: popupToolPanel.buttonRadius * 2
        height: popupToolPanel.buttonRadius * 2
        platformInverted: window.platformInverted
        onClicked: {
            if (popupToolPanel.state == "main") {
                popupToolPanel.state = "tools";
            } else if (popupToolPanel.state == "tools") {
                popupToolPanel.state = "main";
            } else if (popupToolPanel.state == "share") {
                popupToolPanel.state = "tools";
            }

            // Restart timer
            popupTimer.restart();
        }
    }
}
