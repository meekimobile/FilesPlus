import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

Rectangle {
    id: nameFilterPanel
    gradient: Gradient {
        GradientStop {
            position: 0
            color: (!window.platformInverted) ? "#242424" : "#FFFFFF"
        }

        GradientStop {
            position: 0.790
            color: (!window.platformInverted) ? "#0F0F0F" : "#F0F0F0"
        }

        GradientStop {
            position: 1
            color: (!window.platformInverted) ? "#000000" : "#DBDBDB"
        }
    }
    width: parent.width
    height: 60
    z: 1
    visible: false
    
    property bool requestAsType: false
    property int lastFindIndex: -1
    property alias nameFilter: nameFilterInput.text

    signal previous()
    signal next()
    signal opened()
    signal closed()

    function open() {
        lastFindIndex = -1;
        nameFilterInput.focus = true;
        visible = true;
        opened();
    }
    
    function close() {
        closeButton.focus = true;
        visible = false;
        closed();
    }
    
    Row {
        anchors.fill: parent
        anchors.margins: 5
        spacing: 5

        Button {
            id: closeButton
            width: parent.height
            height: parent.height
            iconSource: (!window.platformInverted) ? "close_stop.svg" : "close_stop_inverted.svg"
            platformInverted: window.platformInverted
            onClicked: {                
                nameFilterPanel.close();
            }
        }

        TextField {
            id: nameFilterInput
            width: parent.width - (2*parent.spacing) - closeButton.width - buttons.width
            height: parent.height
            placeholderText: appInfo.emptyStr+qsTr("Please input name filter.")

            Keys.onPressed: {
                if (requestAsType || event.key == Qt.Key_Return || event.key == Qt.Key_Enter) {
                    var t = nameFilterInput.text.trim();
                    nameFilterInput.text = t;
                    next();
                }
            }
        }
        
        ButtonRow {
            id: buttons
            width: parent.height * 2
            height: parent.height
            Button {
                id: prevButton
                width: parent.height
                height: parent.height
                iconSource: (!window.platformInverted) ? "back.svg" : "back_inverted.svg"
                platformInverted: window.platformInverted
                onClicked: {
                    focus = true; // Force accept text field for Symbian only.
                    var t = nameFilterInput.text.trim();
                    nameFilterInput.text = t;
                    previous();
                }
            }

            Button {
                id: nextButton
                width: parent.height
                height: parent.height
                iconSource: (!window.platformInverted) ? "next.svg" : "next_inverted.svg"
                platformInverted: window.platformInverted
                onClicked: {
                    focus = true; // Force accept text field for Symbian only.
                    var t = nameFilterInput.text.trim();
                    nameFilterInput.text = t;
                    next();
                }
            }
        }
    }
}
