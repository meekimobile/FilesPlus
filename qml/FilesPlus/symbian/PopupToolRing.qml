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
        },
        State {
            name: "tools"
            PropertyChanges {
                target: buttonRing
                model: toolsButtonModel
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
    signal sendFile(string srcFilePath, int srcItemIndex)

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
        ListElement { buttonName: "mark"; icon: "check_mark.svg" }
        ListElement { buttonName: "newFolder"; icon: "folder_add.svg" }
        ListElement { buttonName: "upload"; icon: "upload.svg" }
        ListElement { buttonName: "unsync"; icon: "cloud_remove.svg" }
        ListElement { buttonName: "send"; icon: "mail.svg" }
        ListElement { buttonName: "rename"; icon: "rename.svg" }
    }

    Component {
        id: buttonDelegate
        Button {
            enabled: isButtonVisible(buttonName)
            width: popupToolPanel.buttonRadius * 2
            height: popupToolPanel.buttonRadius * 2
            iconSource: icon
            onClicked: buttonHandler(buttonName, index)
        }
    }

    function isButtonVisible(buttonName) {
        if (buttonName === "sync") {
            return !fsModel.isRoot(selectedFilePath) && cloudDriveModel.canSync(selectedFilePath);
        } else if (buttonName === "upload") {
            return !fsModel.isRoot(selectedFilePath) && cloudDriveModel.canSync(selectedFilePath);
        } else if (buttonName === "unsync") {
            return cloudDriveModel.isConnected(selectedFilePath);
        } else if (buttonName === "paste") {
            return (clipboardCount > 0);
        } else if (buttonName == "send") {
            return cloudDriveModel.isConnected(selectedFilePath);
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
        } else if (buttonName == "send") {
            sendFile(selectedFilePath, selectedFileIndex);
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
        iconSource: "toolbar_extension.svg"
        onClicked: {
            if (popupToolPanel.state == "main") {
                popupToolPanel.state = "tools";
            } else {
                popupToolPanel.state = "main";
            }

            // Restart timer
            popupTimer.restart();
        }
    }
}
