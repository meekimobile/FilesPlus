import QtQuick 1.1
import com.nokia.symbian 1.1
import CloudDriveModel 1.0
import "Utility.js" as Utility

Page {
    id: cloudDriveAccountsPage

    property string name: "cloudDriveAccountsPage"
    property bool inverted: window.platformInverted

    tools: toolBarLayout

    ToolBarLayout {
        id: toolBarLayout

        ToolBarButton {
            id: backButton
            buttonIconSource: "toolbar-back"
            onClicked: {
                pageStack.pop();
            }
        }
        ToolBarButton {
            id: refreshButton
            buttonIconSource: "toolbar-refresh"
            onClicked: {
                cloudDriveModel.refreshCloudDriveAccounts();
            }
        }
        ToolBarButton {
            id: addButton
            buttonIconSource: "toolbar-add"
            onClicked: {
                cloudTypeSelection.open();
            }
        }
    }

    TitlePanel {
        id: titlePanel
        text: appInfo.emptyStr+qsTr("Cloud Drive Accounts")
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
            ListElement {
                name: "WebDAV"
                type: CloudDriveModel.WebDAV
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
                    color: (!inverted) ? "white" : "black"
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
                addAccountDialog.show(type);
                break;
            case CloudDriveModel.WebDAV:
                addAccountDialog.show(type); // Get host name from user.
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

        property int cloudType: -1
        property alias connection: connectionName.text
        property alias host: hostname.text
        property alias user: username.text
        property alias passwd: password.text
        property alias token: tokenInput.text
        property string originalToken
        property int labelWidth: 100

        function show(type, uid, host, user, passwd, token) {
            addAccountDialog.cloudType = type;
            addAccountDialog.connection = uid ? uid : "";
            addAccountDialog.host = host ? host : "";
            addAccountDialog.user = user ? user : "";
            addAccountDialog.passwd = passwd ? passwd : "";
            addAccountDialog.token = token ? token : "";
            addAccountDialog.open();
        }

        z: 2
        titleText: appInfo.emptyStr+cloudDriveModel.getCloudName(cloudType)+" "+qsTr("account")
        buttonTexts: [appInfo.emptyStr+qsTr("Save"), appInfo.emptyStr+qsTr("Cancel")]
        content: Item {
            id: contentItem

            property real maxContentHeight: screen.height - (inputContext.visible ? inputContext.height : 0) - 110 // Title + Buttons height

            onHeightChanged: {
                console.debug("onHeightChanged " + height + " maxContentHeight " + maxContentHeight + " inputContext.height " + inputContext.height + " screen.height " + screen.height);
            }

            width: (window.inPortrait) ? (screen.width - 40) : (screen.width - 160)
            height: (window.inPortrait) ? Math.min(240, maxContentHeight) : Math.min(180, maxContentHeight)
            anchors.horizontalCenter: parent.horizontalCenter

            Flickable {
                id: contentFlick
                anchors.fill: parent
                contentWidth: contentColumn.width
                contentHeight: contentColumn.height
                flickableDirection: Flickable.VerticalFlick
                clip: true

                Column {
                    id: contentColumn
                    width: contentItem.width
                    spacing: 5

                    Row {
                        width: parent.width
                        Label {
                            text: qsTr("Name")
                            width: addAccountDialog.labelWidth
                            color: "white"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        TextField {
                            id: connectionName
                            width: parent.width - addAccountDialog.labelWidth
                            placeholderText: "Input connection name"
                            validator: RegExpValidator {
                                regExp: /[\w.-]+/
                            }
                        }
                    }
                    Row {
                        width: parent.width
                        Label {
                            text: qsTr("Host[:port]")
                            width: addAccountDialog.labelWidth
                            color: "white"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        TextField {
                            id: hostname
                            width: parent.width - addAccountDialog.labelWidth
                            placeholderText: "Input hostname"
                            validator: RegExpValidator {
                                regExp: /[\w.-:~]+/
                            }
                        }
                    }
                    Row {
                        width: parent.width
                        visible: basicAuthButton.checked
                        Label {
                            text: qsTr("Username")
                            width: addAccountDialog.labelWidth
                            color: "white"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        TextField {
                            id: username
                            width: parent.width - addAccountDialog.labelWidth
                            placeholderText: "Input username"
                            validator: RegExpValidator {
                                regExp: /[\w.@]+/
                            }
                        }
                    }
                    Row {
                        width: parent.width
                        visible: basicAuthButton.checked
                        Label {
                            text: qsTr("Password")
                            width: addAccountDialog.labelWidth
                            color: "white"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        TextField {
                            id: password
                            width: parent.width - addAccountDialog.labelWidth
                            placeholderText: "Input password"
                            echoMode: TextInput.Password
                        }
                    }
                    Row {
                        width: parent.width
                        visible: tokenAuthButton.checked
                        Label {
                            text: qsTr("Token")
                            width: addAccountDialog.labelWidth
                            color: "white"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        TextField {
                            id: tokenInput
                            width: parent.width - addAccountDialog.labelWidth
                            placeholderText: "Input token"
                        }
                    }
                    Row {
                        width: parent.width
                        visible: tokenAuthButton.checked
                        Label {
                            text: qsTr("OAuth host")
                            width: addAccountDialog.labelWidth
                            color: "white"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        TextField {
                            id: authHostname
                            width: parent.width - addAccountDialog.labelWidth
                            placeholderText: "Input auth. hostname"
                        }
                    }
                    Row {
                        width: parent.width
                        height: 60
                        spacing: 10
                        layoutDirection: Qt.RightToLeft
                        Button {
                            id: testConnectionButton
                            anchors.verticalCenter: parent.verticalCenter
                            width: (parent.width - parent.spacing) / 2
                            states: [
                                State {
                                    name: "ready"
                                    PropertyChanges {
                                        target: testConnectionButton
                                        text: (tokenAuthButton.checked && tokenInput.text == "OAuth") ? qsTr("Authorize") : qsTr("Test")
                                    }
                                },
                                State {
                                    name: "testing"
                                    PropertyChanges {
                                        target: testConnectionButton
                                        explicit: true
                                        text: qsTr("Testing")
                                    }
                                },
                                State {
                                    name: "success"
                                    PropertyChanges {
                                        target: testConnectionButton
                                        explicit: true
                                        text: qsTr("Success")
                                    }
                                },
                                State {
                                    name: "failed"
                                    PropertyChanges {
                                        target: testConnectionButton
                                        explicit: true
                                        text: qsTr("Failed")
                                    }
                                }
                            ]
                            onClicked: {
                                if (hostname.text == "" || username.text == "") {
                                    return;
                                }

                                if (tokenAuthButton.checked && tokenInput.text == "OAuth") {
                                    // Authorize.
                                    if (authHostname.text == "") {
                                        // Focus missing auth hostname.
                                        authHostname.forceActiveFocus();
                                    } else {
                                        // Get token by using auth hostname.
                                        cloudDriveModel.testConnection(addAccountDialog.cloudType, connectionName.text, hostname.text, username.text, password.text, tokenInput.text, authHostname.text);
                                        addAccountDialog.close();
                                    }
                                } else {
                                    // Test connection.
                                    testConnectionButton.state = "testing";
                                    var res = cloudDriveModel.testConnection(addAccountDialog.cloudType, connectionName.text, hostname.text, username.text, password.text, tokenInput.text);
                                    testConnectionButton.state = (res) ? "success" : "failed";
                                }
                            }
                        }
                        ButtonRow {
                            id: authSelector
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width - parent.spacing - testConnectionButton.width
                            visible: (addAccountDialog.cloudType == CloudDriveModel.WebDAV)
                            Button {
                                id: tokenAuthButton
                                text: appInfo.emptyStr+qsTr("OAuth");
                                onClicked: {
                                    // TODO Open confirmation.
                                    tokenInput.text = "OAuth";
                                    authHostname.text = "";
                                    testConnectionButton.state = "ready";
                                }
                            }
                            Button {
                                id: basicAuthButton
                                text: appInfo.emptyStr+qsTr("Basic");
                                onClicked: {
                                    // TODO Open confirmation.
                                    tokenInput.text = "Basic";
                                    testConnectionButton.state = "ready";
                                }
                            }
                        }
                    }
                }
            }
        }

        onOpened: {
            basicAuthButton.checked = (addAccountDialog.token.toLowerCase() == "basic" || addAccountDialog.token == "");
            tokenAuthButton.checked = !basicAuthButton.checked;
            originalToken = token;
            authHostname.text = "";
            testConnectionButton.state = "ready";
        }
        onClosed: {
            addAccountDialog.connection = "";
            addAccountDialog.host = "";
            addAccountDialog.user = "";
            addAccountDialog.passwd = "";
            addAccountDialog.token = "";
        }
        onConfirm: {
            connectionName.closeSoftwareInputPanel(); // For Symbian only.

            if (connectionName.text == "") {
                return;
            }

            cloudDriveModel.saveConnection(addAccountDialog.cloudType, connectionName.text, hostname.text, username.text, password.text, tokenInput.text);
            // TODO Refresh accounts.
        }
    }

    ListView {
        id: accountListView
        width: parent.width
        height: parent.height - titlePanel.height
        anchors.top: titlePanel.bottom
        delegate: accountDelegate
        clip: true
        model: cloudDriveAccountsModel
    }

    Component {
        id: accountDelegate

        ListItem {
            id: listItem

            property int mouseX
            property int mouseY

            Row {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 5

                Image {
                    id: cloudIcon
                    source: iconSource
                    width: 48
                    height: 48
                    fillMode: Image.PreserveAspectFit
                    anchors.verticalCenter: parent.verticalCenter
                }
                Column {
                    width: parent.width - cloudIcon.width
                    spacing: 5
                    Text {
                        id: titleText
                        text: email
                        width: parent.width
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 6
                        elide: Text.ElideMiddle
                        color: (!inverted) ? "white" : "black"
                    }
                    Row {
                        width: parent.width
                        Text {
                            text: "UID " + uid
                            width: parent.width - (quotaText.visible ? quotaText.width : 0) - (runningIcon.visible ? runningIcon.width : 0)
                            verticalAlignment: Text.AlignVCenter
                            font.pointSize: 6
                            elide: Text.ElideMiddle
                            color: "grey"
                        }
                        Text {
                            id: quotaText
                            text: Utility.formatFileSize(availableSpace, 1) + " / " + Utility.formatFileSize(totalSpace, 1)
                            width: 120
                            visible: (totalSpace > 0)
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                            font.pointSize: 6
                            color: "grey"
                        }
                        Image {
                            id: runningIcon
                            width: 24
                            height: 24
                            source: (!inverted ? "refresh.svg" : "refresh_inverted.svg")
                            visible: (totalSpace <= 0)
                        }
                    }
                }
            }

            onPressAndHold: {
                removeAccountConfirmation.index = index;
                removeAccountConfirmation.open();
            }

            onClicked: {
                if (cloudDriveType == CloudDriveModel.WebDAV || cloudDriveType == CloudDriveModel.Ftp) {
                    var atPos = email.lastIndexOf("@");
                    var host = email.substring(atPos+1);
                    var user = email.substring(0, atPos);
                    addAccountDialog.show(cloudDriveType, uid, host, user, secret, token);
                }
            }
        }
    }
}
