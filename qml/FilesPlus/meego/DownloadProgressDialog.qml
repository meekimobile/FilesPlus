import QtQuick 1.1
import com.nokia.meego 1.1
import "Utility.js" as Utility

CommonDialog {
    id: downloadProgressDialog
    
    property alias targetFilePath: targetFilePath.text
    property alias message: downloadMessage.text
    property alias min: downloadProgressBar.minimumValue
    property alias max: downloadProgressBar.maximumValue
    property alias value: downloadProgressBar.value
    property alias indeterminate: downloadProgressBar.indeterminate
    property bool autoClose: true
    
    titleText: "Downloading"
    titleIcon: "FilesPlusIcon.svg"
    buttonTexts: ["OK"]
    content: Column {
        width: parent.width - 10
        spacing: 4
        anchors.horizontalCenter: parent.horizontalCenter
        
        Text {
            id: targetFilePath
            width: parent.width
            color: "white"
            wrapMode: Text.WordWrap
            elide: Text.ElideMiddle
        }
        ProgressBar {
            id: downloadProgressBar
            width: parent.width
            
            onValueChanged: {
                downloadProgressText.text = Utility.formatFileSize(value,3) + " / " + Utility.formatFileSize(maximumValue,3);
                if (downloadProgressDialog.autoClose && value == maximumValue) {
                    downloadProgressDialog.close();
                } else if (value == maximumValue) {
                    indeterminate = true;
                }
            }
        }
        Text {
            id: downloadProgressText
            width: parent.width
            color: "grey"
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignRight
        }
        Rectangle {
            color: "grey"
            width: parent.width
            height: 1
            visible: (downloadMessage.text != "")
        }
        Text {
            id: downloadMessage
            width: parent.width
            color: "white"
            wrapMode: Text.WordWrap
        }
    }
    
    onButtonClicked: {
        downloadProgressDialog.close();
    }
    
    onStatusChanged: {
        if (status == DialogStatus.Closed) {
            targetFilePath = "";
            autoClose = true;
            message = "";
            indeterminate = false;
        }
    }
}
