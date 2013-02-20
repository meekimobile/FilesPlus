import QtQuick 1.1
import com.nokia.symbian 1.1
import QtWebKit 1.0
import "Utility.js" as Utility

Page {
    id: webViewPage

    property alias url: urlInput.text

    function pasteURL() {
        urlInput.text = appInfo.getFromClipboard();
        if (urlInput.text.trim() == "") {
            messageDialog.titleText = appInfo.emptyStr+"CloudPrint "+qsTr("Notify");
            messageDialog.message = appInfo.emptyStr+qsTr("There is no URL in clipboard. Please copy a URL with web browser.");
            messageDialog.open();
        } else {
            webView.url = decodeURI(urlInput.text);
        }
    }

    tools: ToolBarLayout {
        id: toolBarLayout
        x: 0

        ToolButton {
            id: backButton
            iconSource: "toolbar-back"
            platformInverted: window.platformInverted

            onClicked: {
                pageStack.pop();
            }
        }

        ToolButton {
            id: pasteButton
            iconSource: (!window.platformInverted) ? "paste.svg" : "paste_inverted.svg"
            platformInverted: window.platformInverted
            onClicked: {
                pasteURL();
            }
        }

        ToolButton {
            id: openButton
            iconSource: (!window.platformInverted) ? "internet.svg" : "internet_inverted.svg"
            platformInverted: window.platformInverted
            onClicked: {
                Qt.openUrlExternally(urlInput.text);
            }
        }

        ToolButton {
            id: actionButton
            iconSource: (webViewBusy.visible)
                        ? ((!window.platformInverted) ? "close_stop.svg" : "close_stop_inverted.svg")
                        : "toolbar-refresh"
            platformInverted: window.platformInverted
            onClicked: {
                if (webViewBusy.visible) {
                    webView.stop.trigger();
                } else {
                    webView.reload.trigger();
                }
            }
        }

        ToolButton {
            id: printButton
            iconSource: (!window.platformInverted) ? "print.svg" : "print_inverted.svg"
            platformInverted: window.platformInverted
            onClicked: {
                gcpClient.printURLSlot(url);

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
            width: 100
            height: width
            anchors.centerIn: parent
            visible: parent.visible
            running: parent.visible
        }

        Text {
            text: Math.floor(webView.progress * 100) + "%"
            font.pointSize: 6
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
                color: (!window.platformInverted) ? "#242424" : "#FFFFFF"
            }

            GradientStop {
                position: 0.790
                color: (!window.platformInverted) ? "#0F0F0F" : "#F0F0F0"
            }

            GradientStop {
                position: 1
                color: (!window.platformInverted) ? "#000000" : "#DBDBDB"
            }
        }

        TextField {
            id: urlInput
            anchors.fill: parent
            anchors.margins: 5
            Keys.onPressed: {
                // console.debug("urlInput Keys.onPressed " + event.key + " Return " + Qt.Key_Return + " Enter " + Qt.Key_Enter);
                if (event.key == Qt.Key_Return || event.key == Qt.Key_Enter) {
                    console.debug("urlInput Keys.onPressed text " + urlInput.text);
                    urlInput.closeSoftwareInputPanel(); // Symbian only.
                    webView.url = decodeURI(urlInput.text);
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
        contentHeight: webView.height
        anchors.top: urlPanel.bottom
        pressDelay: 200
        boundsBehavior: Flickable.StopAtBounds

//        onContentXChanged: {
//            console.debug("webViewPage flickable onContentXChanged " + contentX);
//        }

//        onContentYChanged: {
//            console.debug("webViewPage flickable onContentYChanged " + contentY);
//        }

        PinchArea {
            id: webPinchArea
            pinch.target: webView
            anchors.fill: parent
            pinch.dragAxis: Pinch.NoDrag

            property real startX
            property real startY
            property real startWidth
            property real startHeight

            onPinchStarted: {
                console.debug("webPinchArea onPinchStarted webView.contentsScale " + webView.contentsScale +
                              " webView.contentsSize " + webView.contentsSize.width + "," + webView.contentsSize.height);
                startX = flickable.contentX + (flickable.width / 2);
                startY = flickable.contentY + (flickable.height / 2);
                startWidth = webView.contentsSize.width;
                startHeight = webView.contentsSize.height;
            }
            onPinchUpdated: {
                // Update scale.
                var deltaScale = (pinch.scale - pinch.previousScale);
                webView.contentsScale += deltaScale;
                console.debug("webPinchArea onPinchUpdated webView.contentsScale " + webView.contentsScale +
                              " pinch.scale " + pinch.scale + " pinch.previousScale " + pinch.previousScale + " deltaScale " + deltaScale +
                              " webView.contentsSize " + webView.contentsSize.width + "," + webView.contentsSize.height +
                              " center " + pinch.center.x + "," + pinch.center.y +
                              " previousCenter " + pinch.previousCenter.x + "," + pinch.previousCenter.y +
                              " startCenter " + pinch.startCenter.x + "," + pinch.startCenter.y);
                // Center pinch.
                var actualCenterX = startX * (webView.contentsSize.width / startWidth);
                var actualCenterY = startY * (webView.contentsSize.height / startHeight);
                flickable.contentX = actualCenterX - (flickable.width / 2);
                flickable.contentY = actualCenterY - (flickable.height / 2);
                console.debug("webPinchArea onPinchUpdated flickable.contentX " + flickable.contentX + " flickable.contentY " + flickable.contentY);
            }
            onPinchFinished: {
                var fitZoom = flickable.width / (webView.contentsSize.width / webView.contentsScale);
                console.debug("webPinchArea onPinchFinished webView.contentsScale " + webView.contentsScale + " webView.contentsSize " + webView.contentsSize.width + "," + webView.contentsSize.height);
                if (webView.contentsScale < fitZoom) {
                    // Scale too small back to fit page width.
                    webView.contentsScale = fitZoom;
                    console.debug("webPinchArea onPinchFinished scale to fit. webView.contentsScale " + webView.contentsScale);
                    // Reset to origin.
                    flickable.contentX = 0;
                    flickable.contentY = 0;
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
                pressGrabTime: 500

                onDoubleClick: {
                    console.debug("webViewPage webView onDoubleClick contentsScale " + contentsScale + " contentsSize " + contentsSize.width + "," + contentsSize.height);
                    var fitZoom = flickable.width / (webView.contentsSize.width / webView.contentsScale);
                    console.debug("webViewPage webView onDoubleClick fitZoom " + fitZoom);
                    var r = heuristicZoom(clickX, clickY, 2.5);
                    console.debug("webViewPage webView onDoubleClick heuristicZoom " + r);
                    if (contentsScale >= 1) {
                        contentsScale = fitZoom * contentsScale;
                    } else {
                        contentsScale = 1;
                    }
                    console.debug("webViewPage webView onDoubleClick done contentsScale " + contentsScale + " contentsSize " + contentsSize.width + "," + contentsSize.height + " width,height " + width + "," + height);
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
