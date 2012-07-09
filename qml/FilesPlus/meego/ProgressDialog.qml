import QtQuick 1.1
import com.nokia.meego 1.1
import "Utility.js" as Utility

CommonDialog {
    id: progressDialog
    
    property alias source: source.text
    property alias target: target.text
    property alias message: message.text
    property alias min: progressBar.minimumValue
    property alias max: progressBar.maximumValue
    property alias value: progressBar.value
    property int newValue
    property int lastValue
    property int minCount
    property int maxCount
    property int count
    property alias indeterminate: progressBar.indeterminate
    property bool autoClose: false
    property int autoCloseInterval: 3000
    property bool formatValue: false

    signal cancelled()

    function toggleHideAction() {
        if (autoClose) {
            hideAction.restart();
        } else {
            hideAction.stop();
        }
    }

    onNewValueChanged: {
        var deltaValue = newValue - lastValue;
//        console.debug("progressDialog deltaValue " + deltaValue);
        value += deltaValue;
        lastValue = newValue;
    }

    onCountChanged: {
        countText.text = count + " / " + maxCount;
    }

    SequentialAnimation {
        id: hideAction

        PauseAnimation { duration: autoCloseInterval }
        ScriptAction {
            script: close();
        }
    }

    titleText: "Progressing"
    titleIcon: "FilesPlusIcon.svg"
    buttonTexts: ["OK", "Cancel"]
    content: Column {
        width: parent.width - 10
        spacing: 4
        anchors.horizontalCenter: parent.horizontalCenter

        Text {
            id: source
            width: parent.width
            height: 30
            verticalAlignment: Text.AlignBottom
            color: "white"
            font.pointSize: 16
            wrapMode: Text.WordWrap
            elide: Text.ElideMiddle
        }
        Text {
            id: target
            width: parent.width
            color: "white"
            font.pointSize: 16
            wrapMode: Text.WordWrap
            elide: Text.ElideMiddle
        }
        ProgressBar {
            id: progressBar
            width: parent.width            
            onValueChanged: {
                if (formatValue) {
                    progressText.text = Utility.formatFileSize(value,3) + " / " + Utility.formatFileSize(maximumValue,3);
                } else {
                    progressText.text = value + " / " + maximumValue;
                }

                if (value >= maximumValue) {
                    console.debug("ProgressDialog progressBar onValueChanged " + value + " >= " +maximumValue);
                    toggleHideAction();
                }
            }
        }
        Row {
            width: parent.width
            Text {
                id: countText
                width: (progressDialog.maxCount == 0) ? 0 : 120
                color: "grey"
                font.pointSize: 16
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignLeft
                text: (progressDialog.maxCount == 0) ? "" : (progressDialog.count + " / " + progressDialog.maxCount)
            }
            Text {
                id: progressText
                width: parent.width - countText.width
                color: "grey"
                font.pointSize: 16
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignRight
            }
        }
        Text {
            id: message
            width: parent.width
            color: "white"
            font.pointSize: 16
            wrapMode: Text.WordWrap
            elide: Text.ElideMiddle
        }
    }
    
    onButtonClicked: {
        var buttonName = buttonTexts[index];
        if (buttonName == "Cancel") {
            cancelled();
        }
    }
    
    onStatusChanged: {
        if (status == DialogStatus.Open) {
            open();
        } else if (status == DialogStatus.Closing) {
            hideAction.stop();

            titleText = "";
            source = "";
            target = "";
            autoClose = false;
            message = "";
            indeterminate = false;
            formatValue = false;

            closing();
        }
    }
}
