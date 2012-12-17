import QtQuick 1.1
import com.nokia.meego 1.0
import "Utility.js" as Utility

SelectionDialog {
    id: recipientSelectionDialog
    style: SelectionDialogStyle { dim: 0.9; pressDelay: 100; itemHeight: 80 }

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
    delegate: ListItem {
        id: recipientItem
        height: 80
        Column {
            width: parent.width - favIcon.width
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
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
                text: (shareFileCaller == "mailFileSlot" ? email : phoneNumber)
            }
        }
        Image {
            id: favIcon
            source: "image://theme/icon-m-common-favorite-mark-inverse"
            visible: favorite
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
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
