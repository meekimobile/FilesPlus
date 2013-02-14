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
                Qt.openUrlExternally(urlInput.text);
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

    Rectangle {
        id: urlPanel
        width: parent.width
        height: 60
        z: 2
        gradient: Gradient {
            GradientStop {
                position: 0
                color: (theme.inverted) ? "#242424" : "#FFFFFF"
            }

            GradientStop {
                position: 0.790
                color: (theme.inverted) ? "#0F0F0F" : "#F0F0F0"
            }

            GradientStop {
                position: 1
                color: (theme.inverted) ? "#000000" : "#DBDBDB"
            }
        }

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

        PinchArea {
            id: webPinchArea
            pinch.target: webView
            anchors.fill: parent
            pinch.dragAxis: Pinch.NoDrag

            onPinchStarted: {
                console.debug("webPinchArea onPinchStarted webView.contentsScale " + webView.contentsScale + " center " + pinch.center.x + "," + pinch.center.y);
            }
            onPinchUpdated: {
                // Update scale.
                var deltaScale = (pinch.scale - pinch.previousScale);
                console.debug("webPinchArea onPinchUpdated webView.contentsScale " + webView.contentsScale + " pinch.scale " + pinch.scale + " prev " + pinch.previousScale + " delta " + deltaScale);
                webView.contentsScale += deltaScale;
                // Center pinch.
                console.debug("webPinchArea onPinchUpdated webView.contentsScale " + webView.contentsScale + " webView.contentsSize " + webView.contentsSize.width + "," + webView.contentsSize.height + " center " + pinch.center.x + "," + pinch.center.y + " startCenter " + pinch.startCenter.x + "," + pinch.startCenter.y);
                var dx = (webView.contentsSize.width / webView.contentsScale) * deltaScale / 2;
                var dy = (webView.contentsSize.height / webView.contentsScale) * deltaScale / 2;
                console.debug("webPinchArea onPinchFinished dScale " + deltaScale + " dx,dy " + dx + "," + dy);
                flickable.contentX += dx;
                flickable.contentY += dy;
            }
            onPinchFinished: {
                var fitZoom = flickable.width / (webView.contentsSize.width / webView.contentsScale);
                console.debug("webPinchArea onPinchFinished webView.contentsScale " + webView.contentsScale + " webView.contentsSize " + webView.contentsSize.width + "," + webView.contentsSize.height);
                if (webView.contentsScale < fitZoom) {
                    // Scale too small back to fit page width.
                    webView.contentsScale = fitZoom;
                    console.debug("webPinchArea onPinchFinished scale to fit. webView.contentsScale " + webView.contentsScale);
                    // Reset to origin if it's less.
                    flickable.contentX = Math.max(0, flickable.contentX);
                    flickable.contentY = Math.max(0, flickable.contentY);
                }
            }

            WebView {
                id: webView
                preferredWidth: webViewPage.width
                preferredHeight: webViewPage.height - urlPanel.height
                visible: true
                settings.javaEnabled: false
                settings.javascriptEnabled: false
                settings.pluginsEnabled: false

                onContentsSizeChanged: {
                    console.debug("onContentsSizeChanged webView.contentsScale " + webView.contentsScale + " webView.contentsSize width height " + webView.contentsSize.width + " " + webView.contentsSize.height);
                    height = Math.max(flickable.height, contentsSize.height);
                }

                onDoubleClick: {
                    console.debug("webViewPage webView onDoubleClick contentsScale " + contentsScale + " contentsSize " + contentsSize.width + "," + contentsSize.height);
                    var zoom = flickable.width / contentsSize.width;
                    console.debug("webViewPage webView onDoubleClick zoom " + zoom);
                    var r = heuristicZoom(clickX, clickY, 2.5);
                    console.debug("webViewPage webView onDoubleClick heuristicZoom " + r);
                    if (contentsScale >= 1) {
                        contentsScale = zoom * contentsScale;
                    } else {
                        contentsScale = 1;
                    }
                    console.debug("webViewPage webView onDoubleClick done contentsScale " + contentsScale + " contentsSize " + contentsSize.width + "," + contentsSize.height);
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
}
