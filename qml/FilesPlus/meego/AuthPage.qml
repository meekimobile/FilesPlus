import QtQuick 1.1
import com.nokia.meego 1.0
import QtWebKit 1.0
import "Utility.js" as Utility

Page {
    id: authPage

    property alias url: webView.url
    property string redirectFrom

    tools: ToolBarLayout {
        id: toolBarLayout
        x: 0

        ToolIcon {
            id:backButton
            iconId: "toolbar-back"

            onClicked: {
                pageStack.pop();
            }
        }

        ToolIcon {
            id:okButton
            iconSource: (theme.inverted) ? "ok.svg" : "ok_inverted.svg"

            onClicked: {
                var p = pageStack.find(function(page) {
                    return (page.name == "folderPage");
                });
                // TODO Remove dependency to make authPage reusable for other REST API.
                if (p) {
                    if (authPage.redirectFrom == "DropboxClient") {
                        p.dropboxAccessTokenSlot();
                    } else if (authPage.redirectFrom == "SkyDriveClient") {
                        p.skyDriveAccessTokenSlot(authPage.parseSkyDriveCode(webView.url));
                    }
                }

                pageStack.pop();
            }
        }
    }

    function parseSkyDriveCode(url) {
        console.debug("authPage parseSkyDriveCode url " + url + " typeof " + (typeof url));
        var urlStr = url + "";
        var caps = urlStr.match("\\?code=([^&]*)");
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
            style: BusyIndicatorStyle { size: "large" }
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

            onContentsSizeChanged: {
                console.debug("onContentsSizeChanged webView.contentsScale " + webView.contentsScale + " webView.contentsSize width height " + webView.contentsSize.width + " " + webView.contentsSize.height);
                height = Math.max(flickable.height, contentsSize.height);
            }

            onLoadStarted: {
                webViewBusy.visible = true;
                okButton.visible = false;
            }

            onLoadFinished: {
                // Workaround for transparent background ( https://bugreports.qt-project.org/browse/QTWEBKIT-352 )
                webView.evaluateJavaScript("if (!document.body.style.backgroundColor) document.body.style.backgroundColor='white';");

                webViewBusy.visible = false;

                if (authPage.redirectFrom == "GCPClient") {
                    // GCPClient handler.
                    console.debug("GCPClient authPage.url " + authPage.url + " authPage.redirectFrom " + authPage.redirectFrom + " title = " + title);
                    if (title.match("^Success")) {
                        var p = pageStack.find(function(page) {
                            return (page.name == "folderPage");
                        });
                        // TODO Remove dependency to make authPage reusable for other REST API.
                        if (p) p.setGCPClientAuthCode(title);
                        pageStack.pop();
                    }

                    if (title.match("^Denied")) {
                        pageStack.pop();
                    }
                } else if (authPage.redirectFrom == "GCDClient") {
                    // GCDClient handler.
                    console.debug("GCDClient authPage.url " + authPage.url + " authPage.redirectFrom " + authPage.redirectFrom + " title = " + title);
                    if (title.match("^Success")) {
                        var p = pageStack.find(function(page) {
                            return (page.name == "folderPage");
                        });
                        // TODO Remove dependency to make authPage reusable for other REST API.
                        if (p) p.setGCDClientAuthCode(title);
                        pageStack.pop();
                    }

                    if (title.match("^Denied")) {
                        pageStack.pop();
                    }
                } else if (authPage.redirectFrom == "DropboxClient") {
                    okButton.visible = true;

                    // DropboxClient handler
                    console.debug("DropboxClient authPage.url " + authPage.url + " authPage.redirectFrom " + authPage.redirectFrom);
                    console.debug("DropboxClient title " + title);
                    console.debug("DropboxClient html " + html);
                    var uidIndex = html.indexOf("uid:");
                    if (uidIndex != -1) {
                        console.debug("found uid! at " + uidIndex);
    //                    console.debug("DropboxClient html " + html);

                        var p = pageStack.find(function(page) {
                            return (page.name == "folderPage");
                        });
                        // TODO Remove dependency to make authPage reusable for other REST API.
                        if (p) p.dropboxAccessTokenSlot();
                        pageStack.pop();
                    } else if (title.match(appInfo.emptyStr+qsTr("^API Request Authorized"))) {
    //                    console.debug("DropboxClient title " + title);

                        var p = pageStack.find(function(page) {
                            return (page.name == "folderPage");
                        });
                        // TODO Remove dependency to make authPage reusable for other REST API.
                        if (p) p.dropboxAccessTokenSlot();
                        pageStack.pop();
                    } else {
    //                    console.debug("DropboxClient title " + title);
    //                    console.debug("DropboxClient html " + html);
                    }
                } else if (authPage.redirectFrom == "SkyDriveClient") {
                    // SkyDriveClient handler
                    console.debug("SkyDriveClient authPage.url " + authPage.url + " authPage.redirectFrom " + authPage.redirectFrom);
                    console.debug("SkyDriveClient title " + title);
                    console.debug("SkyDriveClient html " + html);

                    var pin = authPage.parseSkyDriveCode(url);
                    pinInputPanel.pin = pin;
                    okButton.visible = (pin != "" && pin != "PinNotFound");
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
        anchors.bottomMargin: 5
        z: 2
        visible: okButton.visible

        onVisibleChanged: {
            pinInput.closeSoftwareInputPanel();
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

            TextField {
                id: pinInput
                focus: pinInputPanel.visible
                width: parent.width
                font.pointSize: 16
                readOnly: true
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
        }

        onReject: {
            appInfo.setSettingValue("privacy.policy.accepted", false);
            privacyConsent.close();
            pageStack.pop();
        }
    }
}
