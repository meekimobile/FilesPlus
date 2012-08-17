import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

SelectionDialog {
    id: btSelectionDialog
    height: content.height + 70

    property string srcFilePath
    property bool discovery
    property alias model: btSelectionListView.model
    property alias contentHeight: content.height
    property alias delegate: btSelectionListView.delegate

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
    titleIcon: "FilesPlusIcon.svg"
    content: Item {
        id: content
        width: parent.width
        height: 180

        ListView {
            id: btSelectionListView
            anchors.fill: parent
            delegate: btItemDelegate
        }
    }

    Component {
        id: btItemDelegate

        ListItem {
            id: btItem
            height: 60
            Row {
                width: parent.width - 20
                height: parent.height
                anchors.centerIn: parent
                spacing: 5
                Text {
                    text: deviceName
                    width: parent.width - pairIcon.width - parent.spacing
                    font.pointSize: 8
                    color: "white"
                    elide: Text.ElideRight
                    anchors.verticalCenter: parent.verticalCenter
                }
                Image {
                    id: pairIcon
                    source: "lock.svg"
                    anchors.verticalCenter: parent.verticalCenter
                    visible: isTrusted
                    width: (!visible ? 0 : implicitWidth)
                }
            }
            onClicked: {
                // TODO invoke push on BluetoothClient.
                console.debug("btSelectionDialog btItem onClicked " + deviceName + " " + deviceAddress);
                if (srcFilePath != "") {
                    selected(srcFilePath, deviceAddress);
                    btSelectionDialog.close();
                }
            }
        }
    }
}
