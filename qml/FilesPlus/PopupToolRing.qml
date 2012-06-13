import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

Rectangle {
    id: popupToolPanel
    x: 0; y: 0; z: 2;
    radius: ringRadius
    color: "transparent"
    border.color: "grey"
    border.width: 5
    visible: false

    property bool forFile
    property string srcFilePath
    property string selectedFilePath
    property int selectedFileIndex
    property bool isCopy
    property string pastePath
    property alias timeout: popupTimer.interval
    property variant roots: ["C:/","D:/","E:/","F:/","G:/"]

    property int ringRadius: 60
    property int buttonRadius: 27

    signal opened()
    signal closed()
    signal copyFile(string sourcePath, string targetPath)
    signal moveFile(string sourcePath, string targetPath)
    signal deleteFile(string sourcePath)
    signal printFile(string srcFilePath, int srcItemIndex)
    signal syncFile(string srcFilePath, int srcItemIndex)
    
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
        id: buttonModel
        ListElement { buttonName: "copy"; icon: "copy.svg" }
        ListElement { buttonName: "paste"; icon: "paste.svg" }
        ListElement { buttonName: "print"; icon: "print.svg" }
        ListElement { buttonName: "sync"; icon: "refresh.svg" }
        ListElement { buttonName: "delete"; icon: "delete.svg" }
        ListElement { buttonName: "cut"; icon: "trim.svg" }
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
            return (roots.indexOf(selectedFilePath) == -1);
        } else if (buttonName === "copy" || buttonName === "cut") {
            return forFile;
        } else if (buttonName === "paste") {
            return (srcFilePath != "");
        }

        return true;
    }

    function buttonHandler(buttonName, index) {
        console.debug("button " + index + " buttonName " + buttonName);
        if (buttonName == "paste") {
            if (isCopy) {
                popupToolPanel.copyFile(srcFilePath, pastePath);
            } else {
                popupToolPanel.moveFile(srcFilePath, pastePath);
            }
        } else if (buttonName == "cut") {
            popupToolPanel.srcFilePath = selectedFilePath;
            popupToolPanel.isCopy = false;
        } else if (buttonName == "copy") {
            popupToolPanel.srcFilePath = selectedFilePath;
            popupToolPanel.isCopy = true;
        } else if (buttonName == "print") {
//                    console.debug("popupToolPanel print " + selectedFilePath + ", " + selectedFileIndex);
            popupToolPanel.printFile(selectedFilePath, selectedFileIndex);
        } else if (buttonName == "delete") {
            popupToolPanel.deleteFile(selectedFilePath);
        } else if (buttonName == "sync") {
//                    console.debug("popupToolPanel sync " + selectedFilePath + ", " + selectedFileIndex);
            popupToolPanel.syncFile(selectedFilePath, selectedFileIndex);
        }
        popupToolPanel.visible = false;
    }

    PathView {
        id: buttonRing
        width: parent.width
        height: parent.height
        focus: true
        model: buttonModel
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
}
