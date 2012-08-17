import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

CommonDialog {
    id: uploadProgressDialog
    
    property alias srcFilePath: uploadFilePath.text
    property alias message: uploadMessage.text
    property alias min: uploadProgressBar.minimumValue
    property alias max: uploadProgressBar.maximumValue
    property alias value: uploadProgressBar.value
    property alias indeterminate: uploadProgressBar.indeterminate
    property bool autoClose: true
    
    titleText: appInfo.emptyStr+qsTr("Uploading")
    titleIcon: "FilesPlusIcon.svg"
    buttonTexts: [appInfo.emptyStr+qsTr("OK")]
    content: Column {
        width: parent.width - 10
        height: implicitHeight + 20
        spacing: 4
        anchors.horizontalCenter: parent.horizontalCenter
        
        Text {
            id: uploadFilePath
            width: parent.width
            color: "white"
            wrapMode: Text.WordWrap
            elide: Text.ElideMiddle
            font.pointSize: 6
        }
        ProgressBar {
            id: uploadProgressBar
            width: parent.width
            
            onValueChanged: {
                uploadProgressText.text = Utility.formatFileSize(value,3) + " / " + Utility.formatFileSize(maximumValue,3);
                if (uploadProgressDialog.autoClose && value === maximumValue && value !== 0 && maximumValue !== 0) {
                    uploadProgressDialog.close();
                } else if (value == maximumValue) {
                    indeterminate = true;
                }
            }
        }
        Text {
            id: uploadProgressText
            width: parent.width
            color: "grey"
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignRight
            font.pointSize: 6
        }
        Rectangle {
            color: "grey"
            width: parent.width
            height: 1
            visible: (uploadMessage.text != "")
        }
        Text {
            id: uploadMessage
            width: parent.width
            color: "white"
            wrapMode: Text.WordWrap
            font.pointSize: 6
        }
    }
    
    onButtonClicked: {
        uploadProgressDialog.close();
    }
    
    onStatusChanged: {
        console.debug("uploadProgressDialog onStatusChanged status " + status);
        if (status == DialogStatus.Closing) {
            srcFilePath = "";
            autoClose = true;
            message = "";
            indeterminate = false;
        }
    }
}
