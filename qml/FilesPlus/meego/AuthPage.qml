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
                pageStack.pop(authPage);
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
                if (p) p.dropboxAccessTokenSlot();
                pageStack.pop();
            }
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

    WebView {
        id: webView
        width: parent.width
        height: parent.height
        preferredWidth: width
        preferredHeight: height
//        z: 1
        visible: true

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
                } else if (title.match(qsTr("^API Request Authorized"))) {
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
