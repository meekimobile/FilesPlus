import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

Rectangle {
    id: nameFilterPanel
    color: "black"
    width: parent.width
    height: 60
    z: 1
    visible: false
    
    property alias nameFilters: nameFilterInput.text

    signal requestRefresh(string caller);

    function open() {
        nameFilterInput.text = "";
        visible = true;
    }
    
    function close() {
        nameFilterInput.text = "";
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
