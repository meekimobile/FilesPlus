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
    
    property alias nameFilters: nameFilterInput.text

    signal requestRefresh(string caller);

    function open() {
        nameFilterInput.text = "";
        searchButton.focus = true;
        visible = true;
    }
    
    function close() {
        nameFilterInput.text = "";
        closeButton.focus = true;
        visible = false;
    }
    
    Row {
        anchors.fill: parent
        anchors.margins: 5
        spacing: 5
        
        TextField {
            id: nameFilterInput
            width: parent.width - (2*parent.spacing) - closeButton.width - searchButton.width
            height: parent.height
            placeholderText: appInfo.emptyStr+qsTr("Please input name filter.")

            Keys.onPressed: {
                if (event.key == Qt.Key_Return || event.key == Qt.Key_Enter) {
                    requestRefresh("nameFilterPanel nameFilterInput Keys.onPressed");
                }
            }
        }
        
        Button {
            id: closeButton
            width: parent.height
            height: parent.height
            iconSource: (!window.platformInverted) ? "close_stop.svg" : "close_stop_inverted.svg"
            platformInverted: window.platformInverted
            onClicked: {
                nameFilterPanel.close();
                requestRefresh("nameFilterPanel closeButton");
            }
        }
        
        Button {
            id: searchButton
            width: parent.height
            height: parent.height
            iconSource: (!window.platformInverted) ? "search.svg" : "search_inverted.svg"
            platformInverted: window.platformInverted
            onClicked: {
                requestRefresh("nameFilterPanel searchButton");
            }
        }
    }
}
