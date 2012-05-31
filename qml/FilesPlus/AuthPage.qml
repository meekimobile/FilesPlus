import QtQuick 1.1
import com.nokia.symbian 1.1
import QtWebKit 1.0
import "Utility.js" as Utility

Page {
    id: authPage

    property alias url: webView.url

    tools: ToolBarLayout {
        id: toolBarLayout
        x: 0

        ToolButton {
            id:backButton
            iconSource: "toolbar-back"

            onClicked: {
                pageStack.pop(authPage);
            }
        }
    }

    BusyIndicator {
        id: webViewBusy
        width: 100
        height: width
        z: 2
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        visible: running
        running: false
    }

    WebView {
        id: webView
        width: parent.width
        height: parent.height
        preferredWidth: width
        preferredHeight: height
        z: 1
        visible: true

        onLoadStarted: {
            webViewBusy.running = true;
            console.debug("webView.url " + url);
        }

        onLoadFinished: {
            // Workaround for transparent background ( https://bugreports.qt-project.org/browse/QTWEBKIT-352 )
            webView.evaluateJavaScript("if (!document.body.style.backgroundColor) document.body.style.backgroundColor='white';");

            webViewBusy.running = false;

            // GCPClient handler.
            console.debug("webView.title = " + webView.title);
            if (webView.title.match("^Success")) {
                var p = pageStack.find(function(page) {
                    console.debug("pageStack.find page.name=" + page.name);
                    return (page.name == "mainPage");
                });
                // TODO Remove dependency to make authPage reusable for other REST API.
                p.setGCPClientAuthCode(webView.title);
                pageStack.pop(authPage);
            }

            if (webView.title.match("^Denied")) {
                pageStack.pop(authPage);
            }
        }
    }
}
