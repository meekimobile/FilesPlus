import QtQuick 1.1
import com.nokia.meego 1.0
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
            anchors.fill: parent
            anchors.margins: 5
            Keys.onPressed: {
                // console.debug("urlInput Keys.onPressed " + event.key + " Return " + Qt.Key_Return + " Enter " + Qt.Key_Enter);
                if (event.key == Qt.Key_Return || event.key == Qt.Key_Enter) {
                    console.debug("urlInput Keys.onPressed text " + urlInput.text);
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

            property real startContentX
            property real startContentY
            property real deltaCenterX
            property real deltaCenterY

            onPinchStarted: {
                console.debug("webPinchArea onPinchStarted webView.contentsScale " + webView.contentsScale +
                              " webView.contentsSize " + webView.contentsSize.width + "," + webView.contentsSize.height);
                startContentX = flickable.contentX;
                startContentY = flickable.contentY;
                deltaCenterX = pinch.startCenter.x - (webView.contentsSize.width / 2);
                deltaCenterY = pinch.startCenter.y - (webView.contentsSize.height / 2);
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
//                var dx = (webView.contentsSize.width / webView.contentsScale) * deltaScale / 2;
//                var dy = (webView.contentsSize.height / webView.contentsScale) * deltaScale / 2;
//                var dx = deltaCenterX * pinch.scale - deltaCenterX * pinch.previousScale;
//                var dy = deltaCenterY * pinch.scale - deltaCenterY * pinch.previousScale;
//                console.debug("webPinchArea onPinchUpdated dx,dy " + dx + "," + dy);
//                // TODO Calculate content X,Y.
//                flickable.contentX -= dx;
//                flickable.contentY -= dy;
//                flickable.contentX = startContentX - dx;
//                flickable.contentY = startContentY - dy;
//                flickable.contentX += pinch.center.x - pinch.previousCenter.x; // NOTE center always get updated once flickable.contentX,Y changed.
//                flickable.contentY += pinch.center.y - pinch.previousCenter.y; // NOTE center always get updated once flickable.contentX,Y changed.
//                flickable.contentX = pinch.center.x - deltaStartCenterX;
//                flickable.contentY = pinch.center.y - deltaStartCenterY;
//                flickable.contentX = startContentX + (pinch.center.x - pinch.startCenter.x); // NOTE center always get updated once flickable.contentX,Y changed.
//                flickable.contentY = startContentY + (pinch.center.y - pinch.startCenter.y); // NOTE center always get updated once flickable.contentX,Y changed.
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
