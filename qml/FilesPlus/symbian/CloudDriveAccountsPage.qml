import QtQuick 1.1
import com.nokia.symbian 1.1
import CloudDriveModel 1.0
import "Utility.js" as Utility

Page {
    id: cloudDriveAccountsPage

    property string name: "cloudDriveAccountsPage"

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

    function refreshCloudDriveAccountsSlot() {
        accountListView.model = cloudDriveModel.getUidListModel();
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            refreshCloudDriveAccountsSlot();
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
                refreshCloudDriveAccountsSlot();
            }
        }
        ToolButton {
            id: addButton
            iconSource: "toolbar-add"
            platformInverted: window.platformInverted
            flat: true
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

    SelectionDialog {
        id: cloudTypeSelection
        titleText: appInfo.emptyStr+qsTr("Select Cloud Storage")
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
                name: "GoogleDrive"
                type: CloudDriveModel.GoogleDrive
            }
            ListElement {
                name: "FTP"
                type: CloudDriveModel.Ftp
            }
        }
        delegate: ListItem {
            height: 60
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
                    font.pointSize: 8
                    elide: Text.ElideMiddle
                    color: (!window.platformInverted) ? "white" : "black"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            onClicked: {
                cloudTypeSelection.selectedIndex = index;
                cloudTypeSelection.accept();
            }
        }
        onAccepted: {
            var type = model.get(selectedIndex).type;
            switch (type) {
            case CloudDriveModel.Dropbox:
                cloudDriveModel.requestToken(CloudDriveModel.Dropbox);
                break;
            case CloudDriveModel.SkyDrive:
                cloudDriveModel.authorize(CloudDriveModel.SkyDrive);
                break;
            case CloudDriveModel.GoogleDrive:
                cloudDriveModel.authorize(CloudDriveModel.GoogleDrive);
                break;
            case CloudDriveModel.Ftp:
                addAccountDialog.open();
                break;
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

    ConfirmDialog {
        id: addAccountDialog

        property alias connection: connectionName.text
        property alias host: hostname.text
        property alias user: username.text
        property alias passwd: password.text

        z: 2
        titleText: appInfo.emptyStr+qsTr("New FTP account")
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
                    regExp: /[\w.-:]+/
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
                    text: appInfo.emptyStr+qsTr("Test connection");
                    anchors.centerIn: parent
                    onClicked: {
                        console.debug("cloudDriveAccountsPage testConnectionButton.onClicked");
                        if (hostname.text == "" || username.text == "") {
                            return;
                        }

                        text = appInfo.emptyStr+qsTr("Connecting");

                        var tokens = hostname.text.split(":");
                        var res = cloudDriveModel.testConnection(CloudDriveModel.Ftp,
                                                                 tokens[0],
                                                                 (tokens.length > 1) ? parseInt(tokens[1]) : 21,
                                                                 username.text, password.text);
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

        onClosed: {
            addAccountDialog.connection = "";
            addAccountDialog.host = "";
            addAccountDialog.user = "";
            addAccountDialog.passwd = "";
            testConnectionButton.text = appInfo.emptyStr+qsTr("Test connection");
        }
        onConfirm: {
            var tokens = hostname.text.split(":");
            cloudDriveModel.saveConnection(CloudDriveModel.Ftp,
                                           connectionName.text,
                                           tokens[0],
                                           (tokens.length > 1) ? parseInt(tokens[1]) : 21,
                                           username.text, password.text);
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
                            source: (!window.platformInverted ? "refresh.svg" : "refresh_inverted.svg")
                            visible: (quota <= 0)
                        }
                    }
                }
            }

            onPressAndHold: {
                removeAccountConfirmation.index = index;
                removeAccountConfirmation.open();
            }

            onClicked: {
                if (type == CloudDriveModel.Ftp) {
                    var tokens = email.split("@");
                    addAccountDialog.connection = uid;
                    addAccountDialog.host = tokens[1];
                    addAccountDialog.user = tokens[0];
                    addAccountDialog.passwd = "";
                    addAccountDialog.open();
                }
            }

            Component.onCompleted: {
                    if (email == "") {
                        cloudDriveModel.accountInfo(type, uid);
                    }
                    cloudDriveModel.quota(type, uid);
            }
        }
    }
}
