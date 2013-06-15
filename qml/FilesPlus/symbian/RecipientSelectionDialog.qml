import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

SelectionDialog {
    id: recipientSelectionDialog
    height: content.height + 70

    property string shareFileCaller
    property string srcFilePath
    property string srcFileName
    property string selectedEmail
    property string senderEmail
    property string messageSubject
    property string messageBody
    property string selectedNumber
    
    signal opening()
    signal opened()

    titleText: appInfo.emptyStr+qsTr("Send %1 to").arg(srcFileName)
    titleIcon: "FilesPlusIcon.svg"
    delegate: ListItem {
        id: recipientItem
        height: 60
        Column {
            width: parent.width - favIcon.width
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            ListItemText {
                width: parent.width
                text: displayLabel
                mode: recipientItem.mode
                role: "Title"
            }
            ListItemText {
                width: parent.width
                text: (shareFileCaller == "mailFileSlot" ? email : phoneNumber)
                mode: recipientItem.mode
                role: "Subtitle"
            }
        }
        Image {
            id: favIcon
            source: "favourite.svg"
            visible: favorite
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
        }

        onClicked: {
            console.debug("recipientSelectionDialog recipientItem onClicked " + index + " displayLabel " + displayLabel + " email " + email + " phoneNumber " + phoneNumber);
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
