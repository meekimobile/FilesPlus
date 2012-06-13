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

    // Dummy property to be compatible with PopupToolRing.
    property int ringRadius
    property int buttonRadius

    signal copyFile(string sourcePath, string targetPath)
    signal moveFile(string sourcePath, string targetPath)
    signal deleteFile(string sourcePath)
    signal printFile(string srcFilePath, int srcItemIndex)
    signal syncFile(string srcFilePath, int srcItemIndex)

    function getPopupToolPanelWidth() {
        var w = 0;
        var spacing = buttonRow.spacing;
        w += (pasteButton.visible || cutButton.visible || copyButton.visible || printButton.visible || deleteButton.visible || syncButton.visible) ? spacing : 0;
        w += (pasteButton.visible) ? (pasteButton.width + spacing) : 0;
        w += (cutButton.visible) ? (cutButton.width + spacing) : 0;
        w += (copyButton.visible) ? (copyButton.width + spacing) : 0;
        w += (printButton.visible) ? (printButton.width + spacing) : 0;
        w += (deleteButton.visible) ? (deleteButton.width + spacing) : 0;
        w += (syncButton.visible) ? (syncButton.width + spacing) : 0;

        return w;
    }
    
    function getPopupToolPanelHeight() {
        var h = 0;
        var spacing = buttonRow.spacing;
        h += (pasteButton.visible || cutButton.visible || copyButton.visible || printButton.visible || deleteButton.visible || syncButton.visible) ? (pasteButton.height + spacing + spacing) : 0;
        
        return h;
    }
    
    function open(panelX, panelY) {
        popupToolPanel.visible = true;

        // Disable sync if selectedFilePath is root.
        syncButton.visible = (roots.indexOf(selectedFilePath) == -1);

        popupToolPanel.width = getPopupToolPanelWidth();
        popupToolPanel.height = getPopupToolPanelHeight();

        popupToolPanel.x = panelX - (popupToolPanel.width / 2);
        popupToolPanel.y = panelY - (popupToolPanel.height * 1.5);

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
        anchors.margins: 2
        spacing: 2

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
}
