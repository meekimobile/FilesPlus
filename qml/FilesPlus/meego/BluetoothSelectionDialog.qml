import QtQuick 1.1
import com.nokia.meego 1.0
import "Utility.js" as Utility

SelectionDialog {
    id: btSelectionDialog
    style: SelectionDialogStyle { dim: 0.9; pressDelay: 100; itemHeight: 70 }

    property string srcFilePath
    property bool discovery

    signal selected(string localPath, string deviceAddress)

    states: [
        State {
            name: "discovery"
            when: discovery
            PropertyChanges {
                target: btSelectionDialog
                titleText: appInfo.emptyStr+qsTr("Scanning...")
            }
        },
        State {
            name: "selection"
            when: !discovery
            PropertyChanges {
                target: btSelectionDialog
                titleText: appInfo.emptyStr+qsTr("Bluetooth %1 to").arg(fsModel.getFileName(srcFilePath))
            }
        }
    ]

    onStatusChanged: {
        if (status == DialogStatus.Opening) {
            // NOTE Force content height for Meego only.
            content[0].height = 280;
        }
    }

    delegate: btItemDelegate
    Component {
        id: btItemDelegate

        ListItem {
            id: btItem
            height: 70
            Row {
                width: parent.width - 40
                height: parent.height
                anchors.centerIn: parent
                spacing: 5
                Text {
                    text: deviceName
                    width: parent.width - pairIcon.width - parent.spacing
                    font.pointSize: 18
                    color: "white"
                    elide: Text.ElideRight
                    anchors.verticalCenter: parent.verticalCenter
                }
                Image {
                    id: pairIcon
                    source: (isTrusted) ? "lock.svg" : "bluetooth_paired.svg"
                    anchors.verticalCenter: parent.verticalCenter
                    visible: (isTrusted || isPaired)
                    width: (!visible ? 0 : implicitWidth)
                }
            }
            onClicked: {
                // Invoke push on BluetoothClient.
                console.debug("btSelectionDialog btItem onClicked " + deviceName + " " + deviceAddress);
                if (srcFilePath != "") {
                    selected(srcFilePath, deviceAddress);
                    accept();
                }
            }
        }
    }
}
