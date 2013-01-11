import QtQuick 1.1

ListModel {
    id: messageLoggerModel
    
    property int newMessageCount: 0
    
    function log(type, titleText, message) {
        var msgObj = { "timestamp": new Date(), "meesageType": type, "isRead": false, "titleText": titleText, "message": message };
        messageLoggerModel.append(msgObj);
        messageLoggerModel.newMessageCount++;
    }
    
    function readMessage(index) {
        if (!messageLoggerModel.get(index).isRead) {
            messageLoggerModel.setProperty(index, "isRead", true);
            messageLoggerModel.newMessageCount--;
        }
    }
    
    function readAllMessage() {
        for (var i=0; i<messageLoggerModel.count; i++) {
            readMessage(i);
        }
    }
}
