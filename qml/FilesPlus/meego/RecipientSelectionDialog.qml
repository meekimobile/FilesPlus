import QtQuick 1.1
import com.nokia.meego 1.1
import QtMobility.contacts 1.1
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
    model: ContactModel {
        id: favContactModel
        filter: DetailFilter {
            detail: ContactDetail.Favorite
            field: Favorite.favorite
            value: true
        }
        sortOrders: [
            SortOrder {
                detail: ContactDetail.Name
                field: Name.FirstName
                direction: Qt.AscendingOrder
            },
            SortOrder {
                detail: ContactDetail.Name
                field: Name.LastName
                direction: Qt.AscendingOrder
            }
        ]
    }
    delegate: Rectangle {
        id: recipientItem
        height: 60
        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 2
            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                font.pointSize: 18
                elide: Text.ElideMiddle
                color: "white"
                text: display
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                font.pointSize: 16
                elide: Text.ElideMiddle
                color: "grey"
                text: contact.email.emailAddress
            }
        }

        MouseArea {
            anchors.fill: parent
            //TODO implement onPressed, onReleased to highlight item.
            onClicked: {
                console.debug("recipientSelectionDialog recipientItem onClicked " + index + " email " + contact.email.emailAddress);
                recipientSelectionDialog.selectedIndex = index;
                recipientSelectionDialog.selectedEmail = contact.email.emailAddress;
                recipientSelectionDialog.accept();
            }
        }
    }
}
