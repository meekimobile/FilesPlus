import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

SelectionDialog {
    id: recipientSelectionDialog
    
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
    height: 280
    delegate: ListItem {
        id: recipientItem
        Row {
            anchors.fill: recipientItem.paddingItem
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

    onStatusChanged: {
        if (status == DialogStatus.Opening) {
            selectedIndex = -1;
            opening();
        }
    }
}
