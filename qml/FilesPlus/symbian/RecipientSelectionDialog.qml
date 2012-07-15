import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

SelectionDialog {
    id: recipientSelectionDialog
    
    property string srcFilePath
    property string selectedEmail
    property string senderEmail
    property string messageSubject
    property string messageBody
    
    signal opening()
    signal opened()

    titleText: qsTr("Send") + " " + fsModel.getFileName(srcFilePath) + " " + qsTr("to favorite")
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
                    text: email
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
            console.debug("recipientSelectionDialog recipientItem onClicked " + index + " email " + email);
            recipientSelectionDialog.selectedIndex = index;
            recipientSelectionDialog.selectedEmail = email;
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
