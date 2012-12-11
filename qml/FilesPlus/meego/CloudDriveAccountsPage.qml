import QtQuick 1.1
import com.nokia.meego 1.0
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

        ToolIcon {
            id: backButton
            iconId: "toolbar-back"
            onClicked: {
                pageStack.pop();
            }
        }
        ToolIcon {
            id: refreshButton
            iconId: "toolbar-refresh"
            onClicked: {
                refreshAccountsInfoSlot();
            }
        }
        ToolIcon {
            id: addButton
            iconId: "toolbar-add"
            onClicked: {
                cloudTypeSelection.open();
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

        iconSource: (theme.inverted) ? "delete.svg" : "delete_inverted.svg"
        visible: false
        width: 60
        height: 60
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

    SelectionDialog {
        id: cloudTypeSelection
        titleText: appInfo.emptyStr+qsTr("Select Cloud Storage")
        style: SelectionDialogStyle { dim: 0.9; pressDelay: 100; itemHeight: 80 }
        model: ListModel {
            ListElement {
                name: "Dropbox"
                type: CloudDriveModel.Dropbox
            }
            ListElement {
                name: "SkyDrive"
                type: CloudDriveModel.SkyDrive
            }
            ListElement {
                name: "FTP"
                type: CloudDriveModel.Ftp
            }
        }
        delegate: ListItem {
            height: 80
            Row {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 5

                Image {
                    id: cloudIcon
                    source: cloudDriveModel.getCloudIcon(type)
                    width: 48
                    height: 48
                    fillMode: Image.PreserveAspectFit
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: name
                    width: parent.width - cloudIcon.width - parent.spacing
                    font.pointSize: 18
                    elide: Text.ElideMiddle
                    color: (theme.inverted) ? "white" : "black"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            onClicked: {
                cloudTypeSelection.selectedIndex = index;
                cloudTypeSelection.accept();
            }
        }
        onAccepted: {
            var p = pageStack.find(function (page) { return page.name == "folderPage"; });
            if (p) {
                var type = model.get(selectedIndex).type;
                switch (type) {
                case CloudDriveModel.Dropbox:
                    p.registerDropboxUserSlot();
                    break;
                case CloudDriveModel.SkyDrive:
                    p.registerSkyDriveAccountSlot();
                    break;
                case CloudDriveModel.Ftp:
                    addAccountDialog.open();
                    break;
                }
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

    CommonDialog {
        id: addAccountDialog
        z: 2
        titleText: appInfo.emptyStr+qsTr("New FTP account")
        titleIcon: "FilesPlusIcon.svg"
        buttonTexts: [appInfo.emptyStr+qsTr("OK"), appInfo.emptyStr+qsTr("Cancel")]
        content: Column {
            width: parent.width
            spacing: 5

            TextField {
                id: connectionName
                width: parent.width
                placeholderText: "Input connection name"
                validator: RegExpValidator {
                    regExp: /[\w.-]+/
                }
            }
            TextField {
                id: hostname
                width: parent.width
                placeholderText: "Input hostname"
                validator: RegExpValidator {
                    regExp: /[\w.-]+/
                }
            }
            TextField {
                id: username
                width: parent.width
                placeholderText: "Input username"
                validator: RegExpValidator {
                    regExp: /[\w.]+/
                }
            }
            TextField {
                id: password
                width: parent.width
                placeholderText: "Input password"
                echoMode: TextInput.Password
            }
            Item {
                width: parent.width
                height: 80

                Button {
                    id: testConnectionButton
                    text: appInfo.emptyStr+qsTr("Test connection")
                    anchors.centerIn: parent
                    onClicked: {
                        console.debug("cloudDriveAccountsPage testConnectionButton.onClicked");
                        if (hostname.text == "" || username.text == "") {
                            return;
                        }

                        text = appInfo.emptyStr+qsTr("Connecting");

                        var res = cloudDriveModel.testConnection(hostname.text, 21, username.text, password.text);
                        console.debug("cloudDriveAccountsPage testConnectionButton.onClicked res " + res);
                        if (res) {
                            text = appInfo.emptyStr+qsTr("Connection success");
                        } else {
                            text = appInfo.emptyStr+qsTr("Connection failed");
                        }
                    }
                }
            }
        }

        onAccepted: {
            cloudDriveModel.saveConnection(connectionName.text, hostname.text, 21, username.text, password.text);
        }

        onButtonClicked: {
            if (index == 0) {
                accepted();
            } else {
                rejected();
            }
        }
    }

    ListView {
        id: accountListView
        width: parent.width
        height: parent.height - titlePanel.height
        anchors.top: titlePanel.bottom
        delegate: accountDelegate

        onMovementStarted: {
            if (currentItem) {
                currentItem.pressed = false;
            }
        }
    }

    Component {
        id: accountDelegate

        ListItem {
            id: listItem
            height: 80
            Row {
                anchors.fill: parent
                anchors.margins: 10
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
                    Text {
                        id: titleText
                        text: email
                        width: parent.width
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 18
                        elide: Text.ElideMiddle
                        color: (theme.inverted) ? "white" : "black"
                    }
                    Row {
                        width: parent.width
                        Text {
                            text: "UID " + uid
                            width: parent.width - quotaText.width
                            verticalAlignment: Text.AlignVCenter
                            font.pointSize: 16
                            elide: Text.ElideMiddle
                            color: "grey"
                        }
                        Text {
                            id: quotaText
                            text: Utility.formatFileSize(normal + shared) + " / " + Utility.formatFileSize(quota)
                            width: 160
                            visible: (quota > 0)
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                            font.pointSize: 16
                            elide: Text.ElideMiddle
                            color: "grey"
                        }
                        Image {
                            id: runningIcon
                            width: 24
                            height: 24
                            anchors.right: parent.right
                            source: (theme.inverted ? "refresh.svg" : "refresh_inverted.svg")
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

            Component.onCompleted: {
//                console.debug("uid " + uid + " type " + type);
                switch (type) {
                case CloudDriveModel.Dropbox:
                    cloudDriveAccountsPage.cloudDriveModel.accountInfo(type, uid);
                    break;
                case CloudDriveModel.SkyDrive:
                    if (email == "") {
                        cloudDriveAccountsPage.cloudDriveModel.accountInfo(type, uid);
                    }
                    cloudDriveAccountsPage.cloudDriveModel.quota(type, uid);
                    break;
                }
            }
        }
    }
}
