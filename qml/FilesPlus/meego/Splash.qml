import QtQuick 1.1
import "Utility.js" as Utility

Rectangle {
    id: splashScreen

    property int interval

    signal loaded()

    z: 3
    anchors.fill: parent
    color: "black"

    Component.onCompleted: {
        console.debug(Utility.nowText() + " splashScreen onCompleted");
    }

    FilesPlusInfo {
        id: appInfo
        version: "v1.0.1"
    }

    Timer {
        id: splashTimer
        interval: 100
        running: true

        onTriggered: {
            console.debug(Utility.nowText() + " splashScreen is loaded.");
            hideSplashScreen.start();
            loaded();
            console.debug(Utility.nowText() + " splashScreen hideSplashScreen.start()");
        }
    }

    SequentialAnimation {
        id: hideSplashScreen

        PauseAnimation { duration: interval-500 }
        NumberAnimation { target: splashScreen; property: "opacity"; to: 0; duration: 500; easing.type: Easing.Linear }
        ScriptAction { script: { splashScreen.destroy(); } }
    }
}
