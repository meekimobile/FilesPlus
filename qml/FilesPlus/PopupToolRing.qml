import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

Rectangle {
    id: popupToolPanel

    property bool forFile
    property string srcFilePath
    property string selectedFilePath
    property int selectedFileIndex
    property bool isCopy
    property string pastePath
    property alias timeout: popupTimer.interval
    property variant roots: ["C:/","D:/","E:/","F:/","G:/"]

    property alias ringRadius: buttonRing.radius

    signal printFile(string srcFilePath, int srcItemIndex)
    signal syncFile(string srcFilePath, int srcItemIndex)
    
    function open(panelX, panelY) {
        popupToolPanel.ringRadius = 60;

        console.debug("popupToolRing open panelX " + panelX + " panelY " + panelY);

        // Disable sync if selectedFilePath is root.
        syncButton.visible = (roots.indexOf(selectedFilePath) == -1);

        popupToolPanel.width = popupToolPanel.ringRadius + 30;
        popupToolPanel.height = popupToolPanel.ringRadius + 30;
        popupToolPanel.x = panelX - (popupToolPanel.width / 2);
        popupToolPanel.y = panelY - (popupToolPanel.height / 2);

        if (popupToolPanel.x > (parent.width - popupToolPanel.width) ) {
            popupToolPanel.x = (parent.width - popupToolPanel.width);
        } else if (popupToolPanel.x < 0) {
            popupToolPanel.x = 0;
        }

        if (popupToolPanel.y > (parent.width - popupToolPanel.height) ) {
            popupToolPanel.y = (parent.height - popupToolPanel.height);
        } else if (popupToolPanel.y < 0) {
            popupToolPanel.y = 0;
        }

        popupToolPanel.visible = true;
    }

    x: 0; y: 0; z: 2;
    radius: 9
    gradient: Gradient {
        GradientStop {
            position: 0
            color: "#808080"
        }
        
        GradientStop {
            position: 1
            color: "#505050"
        }
    }
    visible: false
    
    onVisibleChanged: {
        if (visible) {
            popupTimer.restart();
        }
    }
    
    Timer {
        id: popupTimer
        interval: 2000
        running: false;
        onTriggered: parent.visible = false
    }
    
    Row {
        id: buttonRow
        anchors.fill: parent
        anchors.margins: 3
        spacing: 3
        visible: false
        
        Button {
            id: pasteButton
            visible: (popupToolPanel.srcFilePath != "")
            width: 57
            height: 57
            iconSource: "paste.svg"
            onClicked: {
                popupToolPanel.visible = false;
                // TODO to avoid directly refer to external element.
                fileActionDialog.open();
            }
        }
        
        Button {
            id: cutButton
            visible: popupToolPanel.forFile
            width: 57
            height: 57
            iconSource: "trim.svg"
            onClicked: {
                popupToolPanel.visible = false;
                // TODO to avoid directly refer to external element.
                popupToolPanel.srcFilePath = selectedFilePath;
                popupToolPanel.isCopy = false;
            }
        }
        
        Button {
            id: copyButton
            visible: popupToolPanel.forFile
            width: 57
            height: 57
            iconSource: "copy.svg"
            onClicked: {
                popupToolPanel.visible = false;
                // TODO to avoid directly refer to external element.
                popupToolPanel.srcFilePath = selectedFilePath;
                popupToolPanel.isCopy = true;
            }
        }

        Button {
            id: printButton
            visible: popupToolPanel.forFile
            width: 57
            height: 57
            iconSource: "print.svg"
            onClicked: {
                popupToolPanel.visible = false;
                // TODO to avoid directly refer to external element.
                console.debug("popupToolPanel print " + selectedFilePath + ", " + selectedFileIndex);
                popupToolPanel.printFile(selectedFilePath, selectedFileIndex);
            }
        }

        Button {
            id: deleteButton
            visible: true
//            visible: popupToolPanel.forFile
            width: 57
            height: 57
            iconSource: "delete.svg"
            onClicked: {
                popupToolPanel.visible = false;
                // TODO to avoid directly refer to external element.
                fileDeleteDialog.open();
            }
        }

        Button {
            id: syncButton
            visible: true
            width: 57
            height: 57
            iconSource: "refresh.svg"
            onClicked: {
                popupToolPanel.visible = false;
                // TODO to avoid directly refer to external element.
                console.debug("popupToolPanel sync " + selectedFilePath + ", " + selectedFileIndex);
                popupToolPanel.syncFile(selectedFilePath, selectedFileIndex);
            }
        }
    }

    ListModel {
        id: buttonModel
        ListElement { name: "paste"; icon: "paste.svg" }
        ListElement { name: "cut"; icon: "trim.svg" }
        ListElement { name: "copy"; icon: "copy.svg" }
        ListElement { name: "print"; icon: "print.svg" }
        ListElement { name: "delete"; icon: "delete.svg" }
        ListElement { name: "sync"; icon: "refresh.svg" }
    }

    Component {
        id: buttonDelegate
        Button {
            id: name
            visible: true
            width: 57
            height: 57
            iconSource: icon
            onClicked: {
                console.debug("button " + index);
            }
        }
    }

    PathView {
        id: buttonRing

        property int radius: 60

        anchors.fill: parent
        anchors.margins: 30
        focus: true
        model: buttonModel
        delegate: buttonDelegate
        visible: false
        path: Path {
            startX: buttonRing.width / 2
            startY: buttonRing.top
            PathQuad { x: buttonRing.right; y: buttonRing.height / 2; controlX: buttonRing.right; controlY: buttonRing.top }
            PathQuad { x: buttonRing.width /  2; y: buttonRing.bottom; controlX: buttonRing.right; controlY: buttonRing.bottom }
            PathQuad { x: buttonRing.left; y: buttonRing.height / 2; controlX: buttonRing.left; controlY: buttonRing.bottom }
            PathQuad { x: buttonRing.width / 2; y: buttonRing.top; controlX: buttonRing.left; controlY: buttonRing.top }
        }
    }
}
