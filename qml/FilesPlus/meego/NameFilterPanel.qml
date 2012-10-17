import QtQuick 1.1
import com.nokia.meego 1.0
import "Utility.js" as Utility

Rectangle {
    id: nameFilterPanel
    color: "black"
    width: parent.width
    height: 80
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
        anchors.margins: 10
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
            iconSource: (theme.inverted) ? "close_stop.svg" : "close_stop_inverted.svg"
            onClicked: {
                nameFilterPanel.close();
                requestRefresh("nameFilterPanel closeButton");
            }
        }
        
        Button {
            id: searchButton
            width: parent.height
            height: parent.height
            iconSource: (theme.inverted) ? "search.svg" : "search_inverted.svg"
            onClicked: {
                requestRefresh("nameFilterPanel searchButton");
            }
        }
    }
}
