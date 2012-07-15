import QtQuick 1.1
import com.nokia.meego 1.0
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
    
    titleText: qsTr("Downloading")
    titleIcon: "FilesPlusIcon.svg"
    buttonTexts: [qsTr("OK")]
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
            font.pointSize: 16
        }
        ProgressBar {
            id: downloadProgressBar
            width: parent.width
            
            onValueChanged: {
                downloadProgressText.text = Utility.formatFileSize(value,3) + " / " + Utility.formatFileSize(maximumValue,3);

                if (downloadProgressDialog.autoClose && value == maximumValue) {
//                    console.debug("downloadProgressDialog.autoClose " + downloadProgressDialog.autoClose + value + " / " + maximumValue);
                    downloadProgressDialog.accept();
//                    console.debug("downloadProgressDialog.close() is requested.");
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
            font.pointSize: 16
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
            font.pointSize: 16
        }
    }
    
    onButtonClicked: {
        downloadProgressDialog.close();
    }
    
    onStatusChanged: {
        console.debug("downloadProgressDialog onStatusChanged status " + status);
        if (status == DialogStatus.Closing) {
            targetFilePath = "";
            autoClose = true;
            message = "";
            indeterminate = false;
        }
    }
}
