import QtQuick 1.1
import com.nokia.meego 1.0
import "Utility.js" as Utility

CommonDialog {
    id: progressDialog
    width: parent.width * 0.90

    property string source
    property string target
    property string message
    property alias min: progressBar.minimumValue
    property alias max: progressBar.maximumValue
    property double value
    property double newValue
    property double lastValue
    property int minCount
    property int maxCount
    property int count
    property alias indeterminate: progressBar.indeterminate
    property bool autoClose: false
    property int autoCloseInterval: 2000
    property bool formatValue: false
    property alias updateInterval: progressTimer.interval
    property int accuDeltaValue: 0

    signal ok()
    signal cancel()

    function toggleHideAction() {
        if (autoClose) {
            hideAction.restart();
        } else {
            hideAction.stop();
        }
    }

    onNewValueChanged: {
        var deltaValue = 0;
        if (newValue > 0) {
            deltaValue = newValue - lastValue;
            value += deltaValue;
            accuDeltaValue += deltaValue;
            lastValue = newValue;
        }
//        console.debug("progressDialog newValue " + newValue + " lastValue " + lastValue + " deltaValue " + deltaValue + " accuDeltaValue " + accuDeltaValue + " value " + value);
    }

    SequentialAnimation {
        id: hideAction

        PauseAnimation { duration: autoCloseInterval }
        ScriptAction {
            script: close();
        }
    }

    Timer {
        id: progressTimer
        running: (parent.status == DialogStatus.Open)
        repeat: true
        interval: 200
        onTriggered: {
//            console.debug("progressDialog timer onTriggered parent.value " + parent.value);
            progressBar.value = parent.value;
            if (parent.count > 0 || parent.maxCount > 0) {
                countText.text = parent.count + " / " + parent.maxCount;
            }
            source.text = parent.source;
            target.text = parent.target;
            message.text = parent.message;

//            console.debug("progressTimer accuDeltaValue " + accuDeltaValue + " bps " + ((1000 * accuDeltaValue) / interval) );
            parent.accuDeltaValue = 0;
        }
    }

    titleText: appInfo.emptyStr+qsTr("Progressing")
    titleIcon: "FilesPlusIcon.svg"
    buttonTexts: [appInfo.emptyStr+qsTr("OK"), appInfo.emptyStr+qsTr("Cancel")]
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

//                console.debug("ProgressDialog progressBar onValueChanged value " + value + " maximumValue " +maximumValue);
                if (value >= maximumValue) {
                    console.debug("ProgressDialog progressBar onValueChanged " + value + " >= " +maximumValue);
                    ok();
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
        if (index == 0) {
            ok();
        } else {
            cancel();
        }
    }
    
    onStatusChanged: {
        if (status == DialogStatus.Open) {
            opened();
        } else if (status == DialogStatus.Closing) {
            hideAction.stop();

            source = "";
            target = "";
            autoClose = false;
            message = "";
            indeterminate = false;
            formatValue = false;
            newValue = 0;
            progressBar.value = 0;

            closing();
        }
    }
}
