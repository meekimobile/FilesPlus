import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility
import AppInfo 1.0

PageStackWindow {
    id: window
    showStatusBar: true
    showToolBar: true

    platformInverted: true

    state: "ready"
    states: [
        State {
            name: "busy"
            PropertyChanges { target: window.pageStack; busy: true }
            PropertyChanges { target: busyPanel; visible: true }
            PropertyChanges { target: busyindicator1; running: true }
        },
        State {
            name: "ready"
            PropertyChanges { target: window.pageStack; busy: false }
            PropertyChanges { target: busyPanel; visible: false }
            PropertyChanges { target: busyindicator1; running: false }
        }
    ]

    onStateChanged: {
        console.debug(Utility.nowText() + " window onStateChanged " + state);
    }

    onOrientationChangeFinished: {
        console.debug("window onOrientationChangeFinished");
        var p = pageStack.find(function(page) {
            if (page.name == "folderPage") {
                page.orientationChangeSlot();
            } else if (page.name == "imageViewPage") {
                page.orientationChangeSlot();
            }
        });
    }

    AppInfo {
        id: appInfo
        domain: "MeekiMobile"
        app: "FilesPlus"

        Component.onCompleted: {
            appInfo.startMonitoring();
        }

        onNotifyLoggingSignal: {
            messageDialog.titleText = qsTr("Notify");
            messageDialog.message = qsTr("Logging is enabled. Log file is at ") + logFilePath + "\n" + qsTr("You may turn off in Settings.");
            messageDialog.open();
        }
    }

    MessageDialog {
        id: messageDialog
    }

    Text {
        id: statusBarTitle
        x: 4
        y: 0
        width: parent.width / 2
        height: 26
        color: "white"
        text: "FilesPlus"
        smooth: false
        verticalAlignment: Text.AlignVCenter
        font.pointSize: 6
    }

    Rectangle {
        id: busyPanel
        x: 0
        y: 0
        width: parent.width
        height: parent.height
        color: "black"
        visible: false
        smooth: false
        opacity: 0

        BusyIndicator {
            id: busyindicator1
            x: 0
            y: 0
            width: 80
            height: 80
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            running: false
            visible: true
            platformInverted: false
        }

        Text {
            id: busyText1
            width: 180
            text: qsTr("Please wait while loading.")
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pointSize: 8
            anchors.bottom: busyindicator1.top
            anchors.horizontalCenter: parent.horizontalCenter
            color: "white"
        }
    }

    transitions: [
        Transition {
            from: "ready"
            to: "busy"
            NumberAnimation {
                target: busyPanel
                property: "opacity"
                duration: 300
                to: 0.7
                easing.type: Easing.Linear
            }
        },
        Transition {
            from: "busy"
            to: "ready"
            NumberAnimation {
                target: busyPanel
                property: "opacity"
                duration: 300
                to: 0
                easing.type: Easing.Linear
            }
        }
    ]

    Splash {
        id: splashScreen
        interval: 3000

        onLoaded: {
            // Set timer to push pages later after shows splash screen.
            pushPagesTimer.start();
        }
    }

    Timer {
        id: pushPagesTimer
        interval: 200
        running: false
        onTriggered: {
            // Load folderPage then push drivePage to increase performance.
            pageStack.push(Qt.resolvedUrl("FolderPage.qml"));
            pageStack.push(Qt.resolvedUrl("DrivePage.qml"));
        }
    }

    Component.onCompleted: {
        console.debug(Utility.nowText() + " window onCompleted");

        // Set to portrait to show splash screen. Then it will set back to default once it's destroyed.
        screen.allowedOrientations = Screen.Portrait;
    }
}
