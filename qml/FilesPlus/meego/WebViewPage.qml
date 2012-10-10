import QtQuick 1.1
import com.nokia.meego 1.0
import QtWebKit 1.0
import "Utility.js" as Utility

Page {
    id: webViewPage

    property alias url: webView.url

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
            id: printButton
            iconSource: (theme.inverted) ? "print.svg" : "print_inverted.svg"

            onClicked: {
                var p = pageStack.find(function (page) { return page.name == "folderPage"; });
                if (p) p.printURLSlot(webView.url);

                // Clear clipboard.
                appInfo.addToClipboard("");
            }
        }
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            webView.url = appInfo.getFromClipboard();
            if (webView.url == "") {
                messageDialog.titleText = appInfo.emptyStr+"CloudPrint "+qsTr("Notify");
                messageDialog.message = appInfo.emptyStr+qsTr("There is no URL in clipboard. Please copy a URL with web browser.");
                messageDialog.open();
            }
        }
    }

    MessageDialog {
        id: messageDialog
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
            preferredWidth: webViewPage.width
            preferredHeight: webViewPage.height
            visible: true

            onContentsSizeChanged: {
                console.debug("onContentsSizeChanged webView.contentsScale " + webView.contentsScale + " webView.contentsSize width height " + webView.contentsSize.width + " " + webView.contentsSize.height);
                height = Math.max(flickable.height, contentsSize.height);
            }

            onLoadStarted: {
                webViewBusy.visible = true;
            }

            onLoadFinished: {
                // Workaround for transparent background ( https://bugreports.qt-project.org/browse/QTWEBKIT-352 )
                webView.evaluateJavaScript("if (!document.body.style.backgroundColor) document.body.style.backgroundColor='white';");

                webViewBusy.visible = false;
            }
        }
    }
}
