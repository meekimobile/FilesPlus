import QtQuick 1.1
import com.nokia.meego 1.1
import "Utility.js" as Utility

SelectionDialog {
    id: recipientSelectionDialog
    
    property string srcFilePath
    property string selectedEmail
    property string senderEmail
    property string messageSubject
    property string messageBody
    
    titleText: "Send " + fsModel.getFileName(srcFilePath) + " to favorite"
//    titleIcon: "FilesPlusIcon.svg"
    delegate: ListItem {
        id: recipientItem
        Row {
            anchors.fill: parent
            anchors.margins: 0
            spacing: 2
            Column {
                width: parent.width - favIcon.width - parent.spacing
                spacing: 2
                Text {
                    width: parent.width
                    font.pointSize: 18
                    elide: Text.ElideMiddle
                    color: "white"
                    text: displayLabel
                }
                Text {
                    width: parent.width
                    font.pointSize: 16
                    elide: Text.ElideMiddle
                    color: "grey"
                    text: email
                }
            }
            Image {
                id: favIcon
                source: "image://theme/icon-m-common-favorite-mark-inverse"
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
        }
    }
}
