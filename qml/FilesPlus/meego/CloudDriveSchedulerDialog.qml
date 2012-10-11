import QtQuick 1.1
import com.nokia.meego 1.0
import "Utility.js" as Utility

SelectionDialog {
    id: cloudDriveSchedulerDialog
    style: SelectionDialogStyle { dim: 0.9; pressDelay: 100 }

    property string caller
    property int operation
    property string localPath
    property string localPathCronExp
    property int selectedCloudType
    property string selectedUid
    property int selectedModelIndex

    signal selectedCronExp(string cronExp)

    content: Item {
        id: content
        width: parent.width
        height: 350

        ListView {
            id: selectionListView
            anchors.fill: parent
            model: presetColumnModel
            delegate: itemDelegate
            pressDelay: 100
            clip: true

            onMovementStarted: {
                if (currentItem) {
                    currentItem.pressed = false;
                }
            }
        }
    }

    ListModel {
        id: presetColumnModel
        ListElement { value: "1m"; cronExp: "* * * * *" }
        ListElement { value: "2m"; cronExp: "*/2 * * * *" }
        ListElement { value: "5m"; cronExp: "*/5 * * * *" }
        ListElement { value: "10m"; cronExp: "*/10 * * * *" }
        ListElement { value: "20m"; cronExp: "*/20 * * * *" }
        ListElement { value: "30m"; cronExp: "*/30 * * * *" }
        ListElement { value: "1h"; cronExp: "0 * * * *" }
        ListElement { value: "2h"; cronExp: "0 */2 * * *" }
        ListElement { value: "4h"; cronExp: "0 */4 * * *" }
        ListElement { value: "12h"; cronExp: "0 */12 * * *" }
        ListElement { value: "1d"; cronExp: "0 0 * * *" }
        ListElement { value: "Off"; cronExp: "" }

        function getLocalizedLabel(value) {
            if (value.lastIndexOf("m") > -1) {
                return qsTr("%n minute(s)", "", parseInt(value));
            } else if (value.lastIndexOf("h") > -1) {
                return qsTr("%n hour(s)", "", parseInt(value));
            } else if (value == "1d") {
                return qsTr("Daily");
            } else {
                return value;
            }
        }

        function getIndexByCronExp(cronExp) {
            for (var i=0; i<presetColumnModel.count; i++) {
                var item = presetColumnModel.get(i);
                if (cronExp == item.cronExp) {
                    return i;
                }
            }

            return -1;
        }
    }

    Component {
        id: itemDelegate

        ListItem {
            id: selectionItem
            height: 70
            Row {
                width: parent.width - 40
                anchors.centerIn: parent
                Text {
                    text: presetColumnModel.getLocalizedLabel(value)
                    width: parent.width - parent.spacing - checkIcon.width
                    font.pointSize: 18
                    color: "white"
                    elide: Text.ElideRight
                    anchors.verticalCenter: parent.verticalCenter
                }
                Image {
                    id: checkIcon
                    visible: (cronExp == localPathCronExp)
                    source: "ok.svg"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            onClicked: {
                console.debug("cloudDriveSchedulerDialog selectionItem onClicked " + value + " " + cronExp);
                selectedCronExp(cronExp);
                cloudDriveSchedulerDialog.close();
            }
        }
    }

    onStatusChanged: {
        if (status == DialogStatus.Opening) {
            selectionListView.currentIndex = presetColumnModel.getIndexByCronExp(localPathCronExp);
            selectionListView.positionViewAtIndex(selectionListView.currentIndex, ListView.Center);
        }
    }
}
