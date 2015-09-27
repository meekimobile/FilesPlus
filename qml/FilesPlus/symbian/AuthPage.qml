import QtQuick 1.1
import com.nokia.symbian 1.1
import QtWebKit 1.0
import "Utility.js" as Utility

Page {
    id: authPage

    property alias url: webView.url
    property string redirectFrom
    property bool inverted: window.platformInverted
    property bool showPinInputPanel: false

    tools: ToolBarLayout {
        id: toolBarLayout
        x: 0

        ToolBarButton {
            id: backButton
            buttonIconSource: "toolbar-back"
            onClicked: {
                pageStack.pop();
            }
        }

        ToolBarButton {
            id: openWithWebButton
            buttonIconSource: (!inverted) ? "internet.svg" : "internet_inverted.svg"
            onClicked: {
                Qt.openUrlExternally(webView.url);
            }
        }

        ToolBarButton {
            id: toggleInputPanelButton
            buttonIconSource: (!inverted) ? "edit.svg" : "edit_inverted.svg"
            onClicked: {
                showPinInputPanel = !showPinInputPanel;
            }
        }

        ToolBarButton {
            id: okButton
            buttonIconSource: (!inverted) ? "ok.svg" : "ok_inverted.svg"
            onClicked: {
                // TODO Remove dependency to make authPage reusable for other REST API.
                cloudDriveModel.accessTokenSlot(authPage.redirectFrom, pinInputPanel.pin);
                pageStack.pop();
            }
        }
    }

    function parseSkyDriveCode(url) {
        console.debug("authPage parseSkyDriveCode url " + url + " typeof " + (typeof url));
        var text = url + "";
        var caps = text.match("[\\?&]code=([^&]*)");
        if (caps) {
            console.debug("authPage parseSkyDriveCode caps " + caps + " caps.length " + caps.length);
            var code = caps[1];
            if (code && code != "") {
                console.debug("authPage parseSkyDriveCode code " + code);
                return code;
            }
        }

        return "PinNotFound";
    }

    function parseGoogleDriveCode(title) {
        console.debug("authPage parseGoogleDriveCode title " + title + " typeof " + (typeof title));
        var text = title + "";
        var caps = text.match("[\\?&]code=([^&]*)");
        if (caps) {
            console.debug("authPage parseGoogleDriveCode caps " + caps + " caps.length " + caps.length);
            var code = caps[1];
            if (code && code != "") {
                console.debug("authPage parseGoogleDriveCode code " + code);
                return code;
            }
        }

        return "PinNotFound";
    }

    function parseWebDAVCode(url) {
        console.debug("authPage parseWebDAVCode url " + url + " typeof " + (typeof url));
        var text = url + "";
        var caps = text.match("[\\?&]code=([^&]*)");
        if (caps) {
            console.debug("authPage parseWebDAVCode caps " + caps + " caps.length " + caps.length);
            var code = caps[1];
            if (code && code != "") {
                console.debug("authPage parseWebDAVCode code " + code);
                return code;
            }
        }

        return "PinNotFound";
    }

    function parseOAuth2Code(url) {
        console.debug("authPage parseOAuth2Code url " + url + " typeof " + (typeof url));
        var text = url + "";
        var caps = text.match("[\\?&]code=([^&]*)");
        if (caps) {
            console.debug("authPage parseOAuth2Code caps " + caps + " caps.length " + caps.length);
            var code = caps[1];
            if (code && code != "") {
                console.debug("authPage parseOAuth2Code code " + code);
                return code;
            }
        }

        return "PinNotFound";
    }

    function activateSlot() {
        var pin = appInfo.getFromClipboard();
        if (pin !== '') {
            pinInputPanel.pin = pin.trim();
            appInfo.clearClipboard();
        }
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            privacyConsent.open();
        }
    }

    Rectangle {
        id: webViewBusy
        anchors.fill: parent
        color: "black"
        visible: false
        smooth: false
        opacity: 0.9
        z: 2

        BusyIndicator {
            id: busyIdicator
            width: 100
            height: width
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            visible: parent.visible
            running: parent.visible
        }
    }

    Flickable {
        id: flickable
        width: parent.width
        height: parent.height
        contentWidth: webView.width
        contentHeight: webView.implicitHeight
        anchors.top: parent.top
        pressDelay: 200

        WebView {
            id: webView
            preferredWidth: authPage.width
            preferredHeight: authPage.height
            visible: true

//            onContentsSizeChanged: {
//                console.debug("onContentsSizeChanged webView.contentsScale " + webView.contentsScale + " webView.contentsSize width height " + webView.contentsSize.width + " " + webView.contentsSize.height);
//                height = Math.max(flickable.height, contentsSize.height);
//            }

            onLoadStarted: {
                webViewBusy.visible = true;
//                okButton.visible = false;

                if (appInfo.getSettingBoolValue("AuthPage.openAuthorizationWithBrowser", false)) {
                    webView.stop.trigger();
                    webViewBusy.visible = false;
                }
            }

            onLoadFailed: {
                console.debug("webView onLoadFailed authPage.url " + authPage.url + " authPage.redirectFrom " + authPage.redirectFrom + " title = " + title);

                // Reset related objects.
                webViewBusy.visible = false;
                pinInputPanel.pin = "";
            }

            onLoadFinished: {
                // Workaround for transparent background ( https://bugreports.qt-project.org/browse/QTWEBKIT-352 )
                webView.evaluateJavaScript("if (!document.body.style.backgroundColor) document.body.style.backgroundColor='white';");

                // Reset related objects.
                webViewBusy.visible = false;
                pinInputPanel.pin = "";

                if (authPage.redirectFrom == "GCPClient") {
                    // GCPClient handler.
                    console.debug("GCPClient authPage.url " + authPage.url + " authPage.redirectFrom " + authPage.redirectFrom + " title = " + title);
                    if (title.match("^Success")) {
                        // TODO Generalize as signal.
                        gcpClient.setGCPClientAuthCode(title);
                        pageStack.pop();
                    }

                    if (title.match("^Denied")) {
                        pageStack.pop();
                    }
                } else if (authPage.redirectFrom == "GCDClient") {
                    // GCDClient handler.
                    // Example title = Success state=oHlJZRooj1356574041444&code=4/yrQ-e8oo-KgtNousizC9yue_oIJ3
                    console.debug("GCDClient authPage.url " + authPage.url + " authPage.redirectFrom " + authPage.redirectFrom + " title = " + title);

                    if (title.match("^Success")) {
                        var pin = parseGoogleDriveCode(title);
                        pinInputPanel.pin = pin;
//                        okButton.visible = pinInputPanel.visible;
                    }
                } else if (authPage.redirectFrom == "DropboxClient") {
                    okButton.visible = true;

                    // DropboxClient handler
                    console.debug("DropboxClient authPage.url " + authPage.url + " authPage.redirectFrom " + authPage.redirectFrom);
                    console.debug("DropboxClient title " + title);
                    console.debug("DropboxClient html " + html);
/*
  NOTE: 2013-03-12 Dropbox has changed to include uid in first login page. It causes issue not showing login page.
                   var uidIndex = html.indexOf("uid:");
                    if (uidIndex != -1) {
                        console.debug("found uid! at " + uidIndex);
                        pinInputPanel.pin = "PinDefault";
                        cloudDriveModel.accessTokenSlot(authPage.redirectFrom, pinInputPanel.pin);
                        pageStack.pop();
                    } else
*/
                    if (title.match(appInfo.emptyStr+qsTr("^API Request Authorized"))) {
                        pinInputPanel.pin = "PinDefault";
                        cloudDriveModel.accessTokenSlot(authPage.redirectFrom, pinInputPanel.pin);
                        pageStack.pop();
                    } else {
                        var pin = authPage.parseOAuth2Code(url);
                        pinInputPanel.pin = pin;
                    }
                } else if (authPage.redirectFrom == "SkyDriveClient") {
                    // SkyDriveClient handler
                    console.debug("SkyDriveClient authPage.url " + authPage.url + " authPage.redirectFrom " + authPage.redirectFrom);
//                    console.debug("SkyDriveClient title " + title);
//                    console.debug("SkyDriveClient html " + html);

                    var pin = authPage.parseSkyDriveCode(url);
                    pinInputPanel.pin = pin;
//                    okButton.visible = pinInputPanel.visible;
                } else if (authPage.redirectFrom == "WebDavClient") {
                    // WebDavClient handler
                    console.debug("WebDavClient authPage.url " + authPage.url + " authPage.redirectFrom " + authPage.redirectFrom);

                    var pin = authPage.parseWebDAVCode(url);
                    pinInputPanel.pin = pin;
//                    okButton.visible = pinInputPanel.visible;
                } else if (authPage.redirectFrom == "BoxClient") {
                    // BoxClient handler
                    console.debug("BoxClient authPage.url " + authPage.url + " authPage.redirectFrom " + authPage.redirectFrom);
                    pinInputPanel.pin = authPage.parseOAuth2Code(url);
                }
            }
        }
    }

    Rectangle {
        id: pinInputPanel
        color: "black"
        opacity: 0.9
        width: authPage.width - 10
        height: pinInputPanelColumn.height + 20
        radius: 10
        anchors.horizontalCenter: flickable.horizontalCenter
        anchors.bottom: flickable.bottom
        anchors.bottomMargin: (inputContext.visible ? (inputContext.height - 60) : 5) // For Symbian only.
        z: 2
        visible: showPinInputPanel || (pin != "" && pin != "PinNotFound" && pin != "PinDefault") // Show PIN panel when PIN is valid and not default value for Dropbox.

        onVisibleChanged: {
            if (!visible) {
                pinInput.closeSoftwareInputPanel();
            }
        }

        property alias pin: pinInput.text

        Column {
            id: pinInputPanelColumn
            width: parent.width - 20
            anchors.centerIn: parent
            spacing: 5

            Label {
                text: appInfo.emptyStr+qsTr("Please confirm PIN.")
            }

            TextArea {
                id: pinInput
                focus: pinInputPanel.visible
                width: parent.width
                font.pointSize: 16
                readOnly: false
                wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
            }
        }
    }

    ConfirmDialog {
        id: privacyConsent
        titleText: appInfo.emptyStr+qsTr("Privacy Policy")
        contentText: appInfo.emptyStr+qsTr("+ FilesPlus stores only your email during authorization but will not share to any persons/services.\
\n+ FilesPlus never stores your password. It stores only received token which will be used for communicating only with its provider.\
\n+ FilesPlus get your language information and stores selected language internally. It will not share to any persons/services.\
\n\nPlease click 'OK' to continue.")

        onConfirm: {
            appInfo.setSettingValue("privacy.policy.accepted", true);
            privacyConsent.close();

            if (appInfo.getSettingBoolValue("AuthPage.openAuthorizationWithBrowser", false)) {
                openAuthorization.open();
            }
        }

        onReject: {
            appInfo.setSettingValue("privacy.policy.accepted", false);
            privacyConsent.close();
            pageStack.pop();
        }
    }

    ConfirmDialog {
        id: openAuthorization
        titleText: appInfo.emptyStr+qsTr("Authorization")
        contentText: appInfo.emptyStr+qsTr("FilesPlus will open authorization page with web browser.\
\n\nPlease click 'OK' to continue.")

        onConfirm: {
            Qt.openUrlExternally(webView.url);
        }

        onReject: {
            pageStack.pop();
        }
    }
}
