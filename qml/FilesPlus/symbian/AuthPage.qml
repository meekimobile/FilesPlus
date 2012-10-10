import QtQuick 1.1
import com.nokia.symbian 1.1
import QtWebKit 1.0
import "Utility.js" as Utility

Page {
    id: authPage

    property alias url: webView.url
    property string redirectFrom

    tools: ToolBarLayout {
        id: toolBarLayout
        x: 0

        ToolButton {
            id:backButton
            iconSource: "toolbar-back"
            platformInverted: window.platformInverted

            onClicked: {
                pageStack.pop();
            }
        }

        ToolButton {
            id:okButton
            iconSource: (!window.platformInverted) ? "ok.svg" : "ok_inverted.svg"
            platformInverted: window.platformInverted

            onClicked: {
                var p = pageStack.find(function(page) {
                    return (page.name == "folderPage");
                });
                // TODO Remove dependency to make authPage reusable for other REST API.
                if (p) p.dropboxAccessTokenSlot();
                pageStack.pop();
            }
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
            width: 150
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
                }
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

        onReject: {
            pageStack.pop();
        }
    }
}
