// import QtQuick 1.0 // to target S60 5th Edition or Maemo 5
import QtQuick 1.1

Rectangle {
    id: splashScreen

    property alias interval: splashTimer.interval

    z: 3
    width: parent.width
    height: parent.height
    color: "black"

    FilesPlusInfo {
        id: appInfo
    }

    Timer {
        id: splashTimer
        interval: 10000
        running: true
        onTriggered: {            
            hideSplashScreen.start();

            var now = Qt.formatDateTime(new Date(), "d MMM yyyy h:mm:ss ap");
            console.debug(now + " splashScreen hideSplashScreen.start()");
        }
    }

    SequentialAnimation {
        id: hideSplashScreen

        NumberAnimation { target: splashScreen; property: "opacity"; to: 0; duration: 500; easing.type: Easing.Linear }
        ScriptAction { script: { splashScreen.destroy(); } }
    }
}
