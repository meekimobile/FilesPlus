import QtQuick 1.1
import com.nokia.symbian 1.1
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
    titleIcon: "FilesPlusIcon.svg"
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
    delegate: ListItem {
        id: recipientItem
        Column {
            anchors.fill: recipientItem.paddingItem
            ListItemText {
                width: parent.width - 20
                text: display
                mode: recipientItem.mode
                role: "Title"
            }
            ListItemText {
                width: parent.width - 20
                text: contact.email.emailAddress
                mode: recipientItem.mode
                role: "Subtitle"
            }
        }
        onClicked: {
            console.debug("recipientSelectionDialog recipientItem onClicked " + index + " email " + contact.email.emailAddress);
            recipientSelectionDialog.selectedIndex = index;
            recipientSelectionDialog.selectedEmail = contact.email.emailAddress;
            recipientSelectionDialog.accept();
        }
    }
}
