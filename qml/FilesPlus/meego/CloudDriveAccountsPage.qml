import QtQuick 1.1
import com.nokia.meego 1.0
import CloudDriveModel 1.0
import "Utility.js" as Utility

Page {
    id: cloudDriveAccountsPage

    property string name: "cloudDriveAccountsPage"
    property bool inverted: !theme.inverted

    onWidthChanged: {
        console.debug("cloudDriveAccountsPage onWidthChanged " + width);
    }

    onHeightChanged: {
        console.debug("cloudDriveAccountsPage onHeightChanged " + height);
    }

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
        property int selectedCloudType
        property string selectedUid

        titleText: appInfo.emptyStr+qsTr("Remove cloud drive account")
        onConfirm: {
            cloudDriveModel.removeUid(selectedCloudType, selectedUid);
            accountListView.model.remove(index);
        }
        onOpening: {
            selectedCloudType = accountListView.model.get(index).cloudDriveType;
            selectedUid = accountListView.model.get(index).uid;
            contentText = appInfo.emptyStr+qsTr("Please confirm to remove ") + cloudDriveModel.getCloudName(selectedCloudType) + " UID " + selectedUid + " ?";
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

        function show(type, uid, host, user, passwd, token) {
            addAccountDialog.cloudType = type;
            addAccountDialog.connection = uid ? uid : "";
            addAccountDialog.host = host ? host : "";
            addAccountDialog.user = user ? user : "";
            addAccountDialog.passwd = passwd ? passwd : "";
            addAccountDialog.token = token ? token : "";
            addAccountDialog.open();
        }

        function validateForm() {
            if (connectionName.text == "") {
                connectionName.forceActiveFocus();
                return false;
            }
            if (hostname.text == "") {
                hostname.forceActiveFocus();
                return false;
            }

            if (tokenAuthButton.checked && tokenInput.text == "OAuth") {
                // Authorize.
                if (authHostname.text == "") {
                    // Focus missing auth hostname.
                    authHostname.forceActiveFocus();
                    return false;
                }
            } else {
                if (username.text == "") {
                    username.forceActiveFocus();
                    return false;
                }
                if (password.text == "") {
                    password.forceActiveFocus();
                    return false;
                }
            }

            return true;
        }

        z: 2
        titleText: appInfo.emptyStr+cloudDriveModel.getCloudName(cloudType)+" "+qsTr("account")
        buttonTexts: [appInfo.emptyStr+qsTr("Save"), appInfo.emptyStr+qsTr("Cancel")]
        content: Item {
            id: contentItem

            property int labelWidth: width * 0.35
            property real maxContentHeight: cloudDriveAccountsPage.height - 110 // Title + Buttons height. For Meego only.

            onHeightChanged: {
                console.debug("addAccountDialog contentItem onHeightChanged " + height + " maxContentHeight " + maxContentHeight + " cloudDriveAccountsPage.width " + cloudDriveAccountsPage.width + " cloudDriveAccountsPage.height " + cloudDriveAccountsPage.height + " window.inPortrait " + window.inPortrait);
            }

            width: parent.width // For Meego only.
            height: (window.inPortrait) ? Math.min(contentColumn.height, maxContentHeight) : Math.min(contentColumn.height, maxContentHeight) // For Meego only.
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
                            text: appInfo.emptyStr+qsTr("Name")
                            width: contentItem.labelWidth
                            color: "white"
                            elide: Text.ElideRight
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        TextField {
                            id: connectionName
                            width: parent.width - contentItem.labelWidth
                            placeholderText: appInfo.emptyStr+qsTr("Input connection name")
                            readOnly: false
                            validator: RegExpValidator {
                                regExp: /[\w.-]+/
                            }
                        }
                    }
                    Row {
                        width: parent.width
                        Label {
                            text: appInfo.emptyStr+qsTr("Host[:port]")
                            width: contentItem.labelWidth
                            color: "white"
                            elide: Text.ElideRight
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        TextField {
                            id: hostname
                            width: parent.width - contentItem.labelWidth
                            placeholderText: appInfo.emptyStr+qsTr("Input hostname")
                            readOnly: false
                            validator: RegExpValidator {
                                regExp: /[\w.-:~]+/
                            }
                        }
                    }
                    Row {
                        width: parent.width
                        visible: basicAuthButton.checked
                        Label {
                            text: appInfo.emptyStr+qsTr("Username")
                            width: contentItem.labelWidth
                            color: "white"
                            elide: Text.ElideRight
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        TextField {
                            id: username
                            width: parent.width - contentItem.labelWidth
                            placeholderText: appInfo.emptyStr+qsTr("Input username")
                            readOnly: false
                            validator: RegExpValidator {
                                regExp: /[\w.@]+/
                            }
                        }
                    }
                    Row {
                        width: parent.width
                        visible: basicAuthButton.checked
                        Label {
                            text: appInfo.emptyStr+qsTr("Password")
                            width: contentItem.labelWidth
                            color: "white"
                            elide: Text.ElideRight
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        TextField {
                            id: password
                            width: parent.width - contentItem.labelWidth
                            placeholderText: appInfo.emptyStr+qsTr("Input password")
                            readOnly: false
                            echoMode: TextInput.Password
                        }
                    }
                    Row {
                        width: parent.width
                        visible: tokenAuthButton.checked
                        Label {
                            text: appInfo.emptyStr+qsTr("Token")
                            width: contentItem.labelWidth
                            color: "white"
                            elide: Text.ElideRight
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        TextField {
                            id: tokenInput
                            width: parent.width - contentItem.labelWidth
                            placeholderText: appInfo.emptyStr+qsTr("Input token")
                            readOnly: false
                        }
                    }
                    Row {
                        width: parent.width
                        visible: tokenAuthButton.checked
                        Label {
                            text: appInfo.emptyStr+qsTr("OAuth host")
                            width: contentItem.labelWidth
                            color: "white"
                            elide: Text.ElideRight
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        TextField {
                            id: authHostname
                            width: parent.width - contentItem.labelWidth
                            placeholderText: appInfo.emptyStr+qsTr("Input auth. hostname")
                            readOnly: false
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
                                if (!addAccountDialog.validateForm()) return;

                                if (tokenAuthButton.checked && tokenInput.text == "OAuth") {
                                    // Authorize.
                                    // Get token by using auth hostname.
                                    cloudDriveModel.testConnection(addAccountDialog.cloudType, connectionName.text, hostname.text, username.text, password.text, tokenInput.text, authHostname.text);
                                    addAccountDialog.close();
                                } else {
                                    // Set token to Basic if it's empty.
                                    tokenInput.text = (tokenInput.text == "") ? "Basic" : tokenInput.text;

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
            if (!addAccountDialog.validateForm()) return;

            cloudDriveModel.saveConnection(addAccountDialog.cloudType, connectionName.text, hostname.text, username.text, password.text, tokenInput.text);
            cloudDriveModel.refreshCloudDriveAccounts();
        }
    }

    CloudDriveSchedulerDialog {
        id: deltaSchedulerDialog
        titleText: appInfo.emptyStr+cloudDriveModel.getCloudName(selectedCloudType)+" "+qsTr("delta schedule")
        caller: "cloudDriveAccountsPage"

        function show(cloudDriveType, uid) {
            deltaSchedulerDialog.selectedCloudType = cloudDriveType;
            deltaSchedulerDialog.selectedUid = uid;
            deltaSchedulerDialog.localPathCronExp = cloudDriveModel.getDeltaCronExp(cloudDriveType, uid);
            deltaSchedulerDialog.open();
        }

        onSelectedCronExp: {
            console.debug("cloudDriveAccountsPage deltaSchedulerDialog onSelectedCronExp " + cronExp);
            cloudDriveModel.setDeltaCronExp(selectedCloudType, selectedUid, cronExp);
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
                        font.pointSize: 18
                        elide: Text.ElideMiddle
                        color: (!inverted) ? "white" : "black"
                    }
                    Row {
                        width: parent.width
                        Text {
                            text: "UID " + uid
                            width: parent.width - (quotaText.visible ? quotaText.width : 0) - (runningIcon.visible ? runningIcon.width : 0)
                            verticalAlignment: Text.AlignVCenter
                            font.pointSize: 16
                            elide: Text.ElideMiddle
                            color: "grey"
                        }
                        Text {
                            id: quotaText
                            text: appInfo.emptyStr+(totalSpace <= 0 ? qsTr("Not available") : (Utility.formatFileSize(availableSpace, 1) + " / " + Utility.formatFileSize(totalSpace, 1)))
                            width: 160
                            visible: (totalSpace >= 0)
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                            font.pointSize: 16
                            color: "grey"
                        }
                        Image {
                            id: runningIcon
                            width: 24
                            height: 24
                            source: (!inverted ? "refresh.svg" : "refresh_inverted.svg")
                            visible: (totalSpace < 0)
                        }
                    }
                }
            }

            onPressAndHold: {
                console.debug("cloudDriveAccountsPage listItem onPressAndHold index " + index);
                removeAccountConfirmation.index = index;
                removeAccountConfirmation.open();
            }

            onClicked: {
                if (cloudDriveModel.isConfigurable(cloudDriveType)) {
                    var atPos = email.lastIndexOf("@");
                    var host = email.substring(atPos+1);
                    var user = email.substring(0, atPos);
                    addAccountDialog.show(cloudDriveType, uid, host, user, secret, token);
                } else if (cloudDriveModel.isDeltaSupported(cloudDriveType)) {
                    deltaSchedulerDialog.show(cloudDriveType, uid);
                }
            }
        }
    }
}
