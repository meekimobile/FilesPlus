import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

Rectangle {
    property bool forFile
    property string srcFilePath
    property string selectedFilePath
    property int selectedFileIndex
    property bool isCopy
    property string pastePath
    property alias timeout: popupTimer.interval
    property variant roots: ["C:/","D:/","E:/","F:/","G:/"]

    signal printFile(string srcFilePath, int srcItemIndex)
    signal syncFile(string srcFilePath, int srcItemIndex)

    function getPopupToolPanelWidth() {
        var w = 0;
        w += (pasteButton.visible || cutButton.visible || copyButton.visible || deleteButton.visible || syncButton.visible) ? 3 : 0;
        w += (pasteButton.visible) ? (pasteButton.width + 3) : 0;
        w += (cutButton.visible) ? (cutButton.width + 3) : 0;
        w += (copyButton.visible) ? (copyButton.width + 3) : 0;
        w += (printButton.visible) ? (printButton.width + 3) : 0;
        w += (deleteButton.visible) ? (deleteButton.width + 3) : 0;
        w += (syncButton.visible) ? (syncButton.width + 3) : 0;

        return w;
    }
    
    function getPopupToolPanelHeight() {
        var h = 0;
        h += (pasteButton.visible || cutButton.visible || copyButton.visible || printButton.visible || deleteButton.visible || syncButton.visible) ? (pasteButton.height + 6) : 0;
        
        return h;
    }
    
    function open(panelX, panelY) {
        popupToolPanel.visible = true;

        syncButton.visible = (roots.indexOf(selectedFilePath) == -1);

        popupToolPanel.width = getPopupToolPanelWidth();
        popupToolPanel.height = getPopupToolPanelHeight();

        popupToolPanel.x = panelX - 30;
        popupToolPanel.y = panelY - (popupToolPanel.height / 2);

        if (popupToolPanel.x > (parent.width - popupToolPanel.width) ) {
            popupToolPanel.x = (parent.width - popupToolPanel.width);
        } else if (popupToolPanel.x < 0) {
            popupToolPanel.x = 0;
        }

        if (popupToolPanel.y < 0) {
            popupToolPanel.y = 0;
        }
    }

    id: popupToolPanel
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
            // Disable sync if selectedFilePath is root.
//            console.debug("popupToolPanel isRoot " + roots.indexOf(selectedFilePath));
//            syncButton.visible = (roots.indexOf(selectedFilePath) == -1);

            popupTimer.restart();
//            popupToolPanel.width = getPopupToolPanelWidth();
//            popupToolPanel.height = getPopupToolPanelHeight();
//            console.debug("popupToolPanel WH " + popupToolPanel.width + ", " + popupToolPanel.height);
//            console.debug("popupToolPanel parent.width " + parent.width);
//            if (popupToolPanel.x > (parent.width - popupToolPanel.width) ) {
//                popupToolPanel.x = (parent.width - popupToolPanel.width);
//            } else if (popupToolPanel.x < 0) {
//                popupToolPanel.x = 0;
//            }
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
            visible: popupToolPanel.forFile
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
}
