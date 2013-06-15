import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

Rectangle {
    id: splashScreen

    property int interval

    signal loaded()

    function updateLoadingProgress(message, progressValue) {
        loadingProgressBar.value = loadingProgressBar.value + progressValue;
        loadingComponentName.text = message;
    }

    z: 3
    anchors.fill: parent
    color: "black"

    Component.onCompleted: {
        console.debug(Utility.nowText() + " splashScreen onCompleted");
    }

    FilesPlusInfo {
        id: productInfo
        version: appInfo.version
    }

    Label {
        id: loadingComponentName
        text: ""
        color: "white" // It's always in white as splash BG is black.
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: loadingProgressBar.top
    }

    ProgressBar {
        id: loadingProgressBar
        width: parent.width / 2
        z: 1
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        minimumValue: 0
        maximumValue: 1
        value: 0

        onValueChanged: {
            if (value == maximumValue) {
                hideSplashScreen.start();
            }
        }
    }

    Timer {
        id: splashTimer
        interval: 100
        running: true

        onTriggered: {
            console.debug(Utility.nowText() + " splashScreen is loaded.");
            loaded();
            console.debug(Utility.nowText() + " splashScreen hideSplashScreen.start()");
        }
    }

    SequentialAnimation {
        id: hideSplashScreen

        PauseAnimation { duration: (interval > 500 ? (interval-500) : 0) }
        NumberAnimation { target: splashScreen; property: "opacity"; to: 0; duration: 500; easing.type: Easing.Linear }
        ScriptAction {
            script: {
                splashScreen.visible = false;
                splashScreen.destroy();
            }
        }
    }

    // Manually close splash screen.
    MouseArea {
        anchors.fill: parent
        onClicked: {
            hideSplashScreen.start();
        }
    }
}
