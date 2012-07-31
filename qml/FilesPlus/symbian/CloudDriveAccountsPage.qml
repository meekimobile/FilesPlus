import QtQuick 1.1
import com.nokia.symbian 1.1
import CloudDriveModel 1.0
import "Utility.js" as Utility

Page {
    id: cloudDriveAccountsPage

    property string name: "cloudDriveAccountsPage"
    property variant cloudDriveModel

    function updateAccountInfoSlot(type, uid, name, email, shared, normal, quota) {
//        console.debug("cloudDriveAccountsPage updateAccountInfoSlot uid " + uid + " type " + type + " shared " + shared + " normal " + normal + " quota " + quota);
        for (var i=0; i<accountListView.model.count; i++) {
//            console.debug("cloudDriveAccountsPage updateAccountInfoSlot accountModel i " + i + " uid " + accountListView.model.get(i).uid + " type " + accountListView.model.get(i).type);
            if (accountListView.model.get(i).uid == uid && accountListView.model.get(i).type == type) {
                console.debug("cloudDriveAccountsPage updateAccountInfoSlot i " + i + " uid " + uid + " type " + type);
                accountListView.model.set(i, { name: name, email: email, shared: shared, normal: normal, quota: quota });
            }
        }
    }

    function refreshAccountsInfoSlot() {
        // Refresh UID list.
        accountListView.model = cloudDriveAccountsPage.cloudDriveModel.getUidListModel();

        // Refresh quota.
        for (var i=0; i<accountListView.model.count; i++) {
            accountListView.model.set(i, { quota: 0 });
            cloudDriveAccountsPage.cloudDriveModel.accountInfo(accountListView.model.get(i).type, accountListView.model.get(i).uid);
        }
    }

    function refreshCloudDriveAccountsSlot() {
        accountListView.model = cloudDriveAccountsPage.cloudDriveModel.getUidListModel();
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            accountListView.model = cloudDriveAccountsPage.cloudDriveModel.getUidListModel();
        }
    }

    tools: toolBarLayout

    ToolBarLayout {
        id: toolBarLayout

        ToolButton {
            id: backButton
            iconSource: "toolbar-back"
            platformInverted: window.platformInverted
            flat: true
            onClicked: {
                pageStack.pop();
            }
        }
        ToolButton {
            id: refreshButton
            iconSource: "toolbar-refresh"
            platformInverted: window.platformInverted
            flat: true
            onClicked: {
                refreshAccountsInfoSlot();
            }
        }
        ToolButton {
            id: addButton
            iconSource: "toolbar-add"
            platformInverted: window.platformInverted
            flat: true
            onClicked: {
                var p = pageStack.find(function (page) { return page.name == "folderPage"; });
                if (p) p.registerDropboxUserSlot();
            }
        }
    }

    TitlePanel {
        id: titlePanel
        text: appInfo.emptyStr+qsTr("Cloud Drive Accounts")
    }

    Button {
        id: popupDeleteButton

        property int index

        iconSource: (!window.platformInverted) ? "delete.svg" : "delete_inverted.svg"
        platformInverted: window.platformInverted
        visible: false
        width: 50
        height: 50
        z: 2
        onClicked: {
            // Delete selected account.
            visible = false;
            removeAccountConfirmation.index = index;
            removeAccountConfirmation.open();
        }
        onVisibleChanged: {
            if (visible) popupDeleteButtonTimer.restart();
        }

        Timer {
            id: popupDeleteButtonTimer
            interval: 2000
            running: false
            onTriggered: {
                parent.visible = false;
            }
        }
    }

    ConfirmDialog {
        id: removeAccountConfirmation

        property int index

        titleText: appInfo.emptyStr+qsTr("Remove cloud drive account")
        onConfirm: {
            cloudDriveModel.removeUid(accountListView.model.get(index).type, accountListView.model.get(index).uid);
            accountListView.model.remove(index);
        }
        onOpening: {
            contentText = appInfo.emptyStr+qsTr("Please confirm to remove ") + cloudDriveModel.getCloudName(accountListView.model.get(index).type) + " UID " + accountListView.model.get(index).uid + " ?";
        }
    }

    ListView {
        id: accountListView
        width: parent.width
        height: parent.height - titlePanel.height
        anchors.top: titlePanel.bottom
        delegate: accountDelegate
    }

    Component {
        id: accountDelegate

        ListItem {
            id: listItem

            property int mouseX
            property int mouseY

            Row {
                anchors.fill: parent.paddingItem
                spacing: 5

                Image {
                    id: cloudIcon
                    source: cloudDriveModel.getCloudIcon(type)
                    width: 48
                    height: 48
                    fillMode: Image.PreserveAspectFit
                    anchors.verticalCenter: parent.verticalCenter
                }
                Column {
                    width: parent.width - cloudIcon.width
                    ListItemText {
                        id: titleText
                        mode: listItem.mode
                        role: "Title"
                        text: email
                        width: parent.width
                        verticalAlignment: Text.AlignVCenter
                        platformInverted: window.platformInverted
                    }
                    Row {
                        width: parent.width
                        ListItemText {
                            mode: listItem.mode
                            role: "SubTitle"
                            text: "UID " + uid
                            width: parent.width - quotaText.width
                            verticalAlignment: Text.AlignVCenter
                            platformInverted: window.platformInverted
                        }
                        ListItemText {
                            id: quotaText
                            mode: listItem.mode
                            role: "Subtitle"
                            text: Utility.formatFileSize(normal + shared) + " / " + Utility.formatFileSize(quota)
                            width: 120
                            visible: (quota > 0)
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                            platformInverted: window.platformInverted
                        }
                        Image {
                            id: runningIcon
                            width: 24
                            height: 24
                            anchors.right: parent.right
                            source: "refresh.svg"
                            visible: (quota <= 0)
                        }
                    }
                }
            }

            onPressAndHold: {
                var panelX = x + mouseX - accountListView.contentX;
                var panelY = y + mouseY - accountListView.contentY + accountListView.y;
                popupDeleteButton.x = panelX - (popupDeleteButton.width / 2);
                popupDeleteButton.y = panelY - (popupDeleteButton.height);
                popupDeleteButton.index = index;
                popupDeleteButton.visible = true;
            }

            MouseArea {
                anchors.fill: parent
                onPressed: {
                    parent.mouseX = mouseX;
                    parent.mouseY = mouseY;
                    mouse.accepted = false;
                }
            }

            Component.onCompleted: {
//                console.debug("uid " + uid + " type " + type);
                cloudDriveAccountsPage.cloudDriveModel.accountInfo(type, uid);
            }
        }
    }

    Component.onCompleted: {
    }
}
