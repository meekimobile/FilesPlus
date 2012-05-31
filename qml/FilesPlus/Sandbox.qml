// import QtQuick 1.0 // to target S60 5th Edition or Maemo 5
import QtQuick 1.1
import com.nokia.symbian 1.1
import DropboxClient 1.0
import QtWebKit 1.0
import "./Utility.js" as Utility

Page {
    id: sandboxPage

    Button {
        id: requestTokenButton
        width: parent.width / 2
        height: 60
        text: "Request Token"
        onClicked: {
            dbcModel.requestToken();
        }
    }

    Button {
        id: getResourceButton
        width: parent.width / 2
        height: 60
        anchors.left: requestTokenButton.right
        text: "Get Resource"
        onClicked: {
            // TODO UID should be selected from QML before request for protected resource.
            uidDialog.open();
        }
    }

    SelectionDialog {
        id: uidDialog
        titleText: "Please select Dropbox UID"
        model: dbcModel.getStoredUidList();

        onAccepted: {
            // Item referring for QStringList model.
            var uid = uidDialog.model[uidDialog.selectedIndex];
//            dbcModel.accountInfo(uid);
            dbcModel.metadata(uid, "");
//            dbcModel.fileGet(uid, "Getting Started.pdf");
//            dbcModel.filePut(uid, "C:/dummy.txt", "sub/dummy.txt");
//            dbcModel.filePut(uid, "C:/FolderPieCache.dat", "FolderPieCache.dat");
        }
    }

    Label {
        id: replyMessage
        font.pointSize: 5
        width: parent.width
        height: parent.height - requestTokenButton.height
        anchors.top: requestTokenButton.bottom
        wrapMode: Text.WrapAnywhere
        text: "Reply message"
        font.family: "Arial"
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
        height: parent.height - requestTokenButton.height
        anchors.top: requestTokenButton.bottom
        z: 1
        visible: false
        url: ""

        onLoadStarted: {
            webViewBusy.running = true;
            console.debug("webView.url " + url);
        }

        onLoadFinished: {
            webViewBusy.running = false;
            var uidIndex = html.indexOf("uid:");
            if (uidIndex != -1) {
                console.debug("found uid! at " + uidIndex);
                console.debug(html);
                webView.visible = false;
                dbcModel.accessToken();
            }
        }
    }

    DropboxClient {
        id: dbcModel

        onRequestTokenReplySignal: {
            console.debug("sandboxPage dbcModel onRequestTokenReplySignal " + err + " " + errMsg + " " + msg);
            replyMessage.text += "\n" + msg;

            // TODO how to check if app has been authorized by user.
            dbcModel.authorize();
        }

        onAuthorizeRedirectSignal: {
            console.debug("sandboxPage dbcModel onAuthorizeRedirectSignal " + url);
            replyMessage.text += "\n" + "Redirect to authorize URL " + url;

            webView.url = url;
            webView.visible = true;
        }

        onAccessTokenReplySignal: {
            console.debug("sandboxPage dbcModel onAccessTokenReplySignal " + err + " " + errMsg + " " + msg);
            replyMessage.text += "\n" + msg;
        }

        onAccountInfoReplySignal: {
            console.debug("sandboxPage dbcModel onAccountInfoReplySignal " + err + " " + errMsg + " " + msg);
            replyMessage.text += "\n" + msg;

            var jsonObj = Utility.createJsonObj(msg);
            console.debug("jsonObj.email " + jsonObj.email);
        }

        onFileGetReplySignal: {
            console.debug("sandboxPage dbcModel onFileGetReplySignal " + err + " " + errMsg + " " + msg);
            replyMessage.text += "\n" + msg;
        }

        onFilePutReplySignal: {
            console.debug("sandboxPage dbcModel onFilePutReplySignal " + err + " " + errMsg + " " + msg);
            replyMessage.text += "\n" + msg;
        }

        onMetadataReplySignal: {
            console.debug("sandboxPage dbcModel onMetadataReplySignal " + err + " " + errMsg + " " + msg);
            replyMessage.text += "\n" + msg;
        }

        onDownloadProgress: {
            console.debug("sandBox dbcModel onDownloadProgress " + bytesReceived + " / " + bytesTotal);
        }

        onUploadProgress: {
            console.debug("sandBox dbcModel onUploadProgress " + bytesSent + " / " + bytesTotal);
        }
    }
}
