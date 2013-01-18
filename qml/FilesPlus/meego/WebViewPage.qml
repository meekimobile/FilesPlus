import QtQuick 1.1
import com.nokia.meego 1.0
import QtWebKit 1.0
import "Utility.js" as Utility

Page {
    id: webViewPage

    property alias url: webView.url

    function pasteURL() {
        urlInput.text = appInfo.getFromClipboard();
        if (urlInput.text.trim() == "") {
            messageDialog.titleText = appInfo.emptyStr+"CloudPrint "+qsTr("Notify");
            messageDialog.message = appInfo.emptyStr+qsTr("There is no URL in clipboard. Please copy a URL with web browser.");
            messageDialog.open();
        } else {
            webView.url = urlInput.text;
        }
    }

    tools: ToolBarLayout {
        id: toolBarLayout
        x: 0

        ToolIcon {
            id: backButton
            iconId: "toolbar-back"

            onClicked: {
                pageStack.pop();
            }
        }

        ToolIcon {
            id: pasteButton
            iconSource: (theme.inverted) ? "paste.svg" : "paste_inverted.svg"

            onClicked: {
                pasteURL();
            }
        }

        ToolIcon {
            id: openButton
            iconSource: (theme.inverted) ? "internet.svg" : "internet_inverted.svg"
            onClicked: {
                Qt.openUrlExternally(url);
            }
        }

        ToolIcon {
            id: actionButton
            iconSource: (webViewBusy.visible)
                        ? ((theme.inverted) ? "close_stop.svg" : "close_stop_inverted.svg")
                        : ((theme.inverted) ? "refresh.svg" : "refresh_inverted.svg")

            onClicked: {
                if (webViewBusy.visible) {
                    webView.stop.trigger();
                } else {
                    webView.reload.trigger();
                }
            }
        }

        ToolIcon {
            id: printButton
            iconSource: (theme.inverted) ? "print.svg" : "print_inverted.svg"

            onClicked: {
                gcpClient.printURLSlot(webView.url);

                // Clear clipboard.
                appInfo.addToClipboard("");
            }
        }
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            pasteURL();
        }
    }

    Rectangle {
        id: webViewBusy
        width: parent.width
        height: parent.height - urlPanel.height
        anchors.top: urlPanel.bottom
        color: "black"
        visible: false
        smooth: false
        opacity: 0.9
        z: 2

        BusyIndicator {
            id: busyIdicator
            style: BusyIndicatorStyle { size: "large" }
            anchors.centerIn: parent
            visible: parent.visible
            running: parent.visible
        }

        Text {
            text: Math.floor(webView.progress * 100) + "%"
            font.pointSize: 16
            color: "white"
            anchors.centerIn: parent
        }
    }

    TitlePanel {
        id: urlPanel
        height: 60
        z: 2

        TextField {
            id: urlInput
            text: webView.url
            anchors.fill: parent
            anchors.margins: 5
            Keys.onPressed: {
                // console.debug("urlInput Keys.onPressed " + event.key + " Return " + Qt.Key_Return + " Enter " + Qt.Key_Enter);
                if (event.key == Qt.Key_Return || event.key == Qt.Key_Enter) {
                    console.debug("urlInput Keys.onPressed text " + urlInput.text);
                    webView.url = urlInput.text;
                    webView.focus = true;
                }
            }
        }
    }

    Flickable {
        id: flickable
        width: parent.width
        height: parent.height - urlPanel.height
        contentWidth: webView.width
        contentHeight: webView.implicitHeight
        anchors.top: urlPanel.bottom
        pressDelay: 200

        WebView {
            id: webView
            preferredWidth: webViewPage.width
            preferredHeight: webViewPage.height
            visible: true
            settings.javaEnabled: false
            settings.javascriptEnabled: false
            settings.pluginsEnabled: false

            onContentsSizeChanged: {
                console.debug("onContentsSizeChanged webView.contentsScale " + webView.contentsScale + " webView.contentsSize width height " + webView.contentsSize.width + " " + webView.contentsSize.height);
                height = Math.max(flickable.height, contentsSize.height);
            }

            onLoadStarted: {
                webViewBusy.visible = true;
            }

            onLoadFailed: {
                console.debug("webViewPage webView onLoadFailed " + webView.progress);
                webView.html = qsTr("Page loading failed. Please open with internet browser.");
                webViewBusy.visible = false;
            }

            onLoadFinished: {
                // Workaround for transparent background ( https://bugreports.qt-project.org/browse/QTWEBKIT-352 )
                webView.evaluateJavaScript("if (!document.body.style.backgroundColor) document.body.style.backgroundColor='white';");

                webViewBusy.visible = false;
            }
        }
    }
}
