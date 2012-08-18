import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

SelectionDialog {
    id: recipientSelectionDialog
    height: content.height + 70

    property alias model: recipientSelectionListView.model
    property alias contentHeight: content.height
    property alias delegate: recipientSelectionListView.delegate

    property string shareFileCaller
    property string srcFilePath
    property string selectedEmail
    property string senderEmail
    property string messageSubject
    property string messageBody
    property string selectedNumber
    
    signal opening()
    signal opened()

    titleText: appInfo.emptyStr+qsTr("Send %1 to").arg(fsModel.getFileName(srcFilePath))
    titleIcon: "FilesPlusIcon.svg"
    content: Item {
        id: content
        width: parent.width
        height: 180

        ListView {
            id: recipientSelectionListView
            anchors.fill: parent
            delegate: recipientItemDelegate
        }
    }

    Component {
        id: recipientItemDelegate

        ListItem {
            id: recipientItem
            height: 60
            Row {
                width: parent.width - 20
                anchors.centerIn: parent
                Column {
                    width: parent.width - favIcon.width - parent.spacing
                    spacing: 2
                    ListItemText {
                        width: parent.width - 20
                        text: displayLabel
                        mode: recipientItem.mode
                        role: "Title"
                    }
                    ListItemText {
                        width: parent.width - 20
                        text: (shareFileCaller == "mailFileSlot" ? email : phoneNumber)
                        mode: recipientItem.mode
                        role: "Subtitle"
                    }
                }
                Image {
                    id: favIcon
                    source: "favourite.svg"
                    visible: favorite
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            onClicked: {
                console.debug("recipientSelectionDialog recipientItem onClicked " + index + " email " + email + " phoneNumber " + phoneNumber);
                recipientSelectionDialog.selectedIndex = index;
                recipientSelectionDialog.selectedEmail = email;
                recipientSelectionDialog.selectedNumber = phoneNumber;
                recipientSelectionDialog.accept();
            }
        }
    }

    onStatusChanged: {
        if (status == DialogStatus.Opening) {
            selectedIndex = -1;
            opening();
        }
    }
}
