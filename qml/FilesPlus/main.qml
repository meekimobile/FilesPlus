import QtQuick 1.1
import com.nokia.symbian 1.1

PageStackWindow {
    id: window
    initialPage: MainPage { id: mainPage; tools: toolBarLayout }
    showStatusBar: true
    showToolBar: true

    property string version: "v1.0.0"

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

    onOrientationChangeFinished: {
        console.debug("window onOrientationChangeFinished");
        var p = pageStack.find(function(page) { return (page.name == "mainPage")});
        p.orientationChangeSlot();
    }

    ToolBarLayout {
        id: toolBarLayout
        x: 0

        ToolButton {
            id: refreshButton
            x: 300
            y: 0
            iconSource: "toolbar-refresh"
            flat: true

            signal refresh;
            onRefresh: {
                console.debug("Send refresh signal");
            }

            Component.onCompleted: {
                // Connect clicked signal to refresh signal.
                refreshButton.clicked.connect(refresh);
                // Connect refresh signal to window.initialPage.refreshSlot
                refreshButton.refresh.connect(window.initialPage.refreshSlot);
            }
        }

        ToolButton {
            id: upButton
            x: 150
            y: 0
            text: ""
            iconSource: "arrow-up.svg"
            flat: true

            signal goUp;
            onGoUp: {
                console.debug("Send goUp signal");
            }

            Component.onCompleted: {
                upButton.clicked.connect(goUp);
                upButton.goUp.connect(window.initialPage.goUpSlot);
            }
        }

        ToolButton {
            id: flipButton
            x: 300
            y: 8
            iconSource: (mainPage.state != "list") ? "list.svg" : "chart.svg"
            flat: true

            signal flip;
            onFlip: {
                console.debug("mainPage.state " + mainPage.state);
                console.debug("Send flip signal");
            }

            Component.onCompleted: {
                flipButton.clicked.connect(flip);
                flipButton.flip.connect(window.initialPage.flipSlot);
            }
        }

        ToolButton {
            id: menuButton
            flat: true
            iconSource: "toolbar-menu"
            onClicked: {
                mainMenu.open();
            }
        }
    }

    Text {
        id: statusBarTitle
        x: 4
        y: 0
        width: parent.width / 2
        height: 26
        color: "white"
        text: qsTr("Files+")
        smooth: false
        verticalAlignment: Text.AlignVCenter
        font.family: "MS Shell Dlg 2"
        font.pixelSize: 18
    }

    Menu {
        id: mainMenu
        z: 2

        onStatusChanged: {
//            console.debug("mainMenu onStatusChanged " + status);
        }

        content: MenuLayout {
            MenuItem {
                text: "About"
                onClicked: {
                    aboutDialog.open();
                }
            }

            MenuItem {
                text: "Help"
                onClicked: {
                    Qt.openUrlExternally("http://sites.google.com/site/meekimobile");
                }
            }

            MenuItem {
                id: menuResetCache
                text: "Reset Cache"
                onClicked: {
                    confirmDialog.open();
                }

                Component.onCompleted: {
                    confirmDialog.confirm.connect(window.initialPage.resetCacheSlot);
                }
            }

            MenuItem {
                text: "More Apps"
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("MoreApps.qml"));
                }
            }

            MenuItem {
                text: "Exit"
                onClicked: {
                    Qt.quit();
                }
            }
        }
    }

    CommonDialog {
        id: confirmDialog
        z: 2
        titleText: "Reset Cache"
        titleIcon: "FilesPlusIcon.svg"
        buttonTexts: ["OK", "Cancel"]
        content: Text {
            text: "Resetting Cache will take time depends on numbers of sub folders/files under current folder.\n\nPlease click OK to continue."
            color: "white"
            font.pixelSize: 18
            wrapMode: Text.Wrap
            width: parent.width - 8
            height: implicitHeight
            anchors.horizontalCenter: parent.horizontalCenter
        }

        signal confirm()

        onButtonClicked: {
            if (index === 0) confirm();
        }
    }    

    CommonDialog {
        id: aboutDialog
        z: 2
        titleText: "About"
        titleIcon: "FilesPlusIcon.svg"
        buttonTexts: ["OK"]
        content: Column {
            width: parent.width - 8
            spacing: 4
            anchors.horizontalCenter: parent.horizontalCenter
            Text {
                id: title
                color: "white"
                text: qsTr("Files+ " + window.version)
                font.bold: true
                font.family: "Century Gothic"
                font.pixelSize: 28
            }
            Text {
                id: description
                color: "grey"
                width: parent.width
                text: qsTr("Files+ will help you collect each folder actual size on your disk space. And present in Pie view for easy understanding at glance.")
                wrapMode: Text.WordWrap
                font.family: "Century Gothic"
                font.pixelSize: 18
            }
            Text {
                id: author
                color: "grey"
                text: qsTr("\nDeveloped by MeekiMobile\nhttp://sites.google.com/site/meekimobile")
                font.pixelSize: 18
                wrapMode: Text.WordWrap
                font.family: "Century Gothic"
            }
            Image {
                source: "MeekiMobile256.svg"
                width: 64
                height: width
            }
        }
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
            text: "Please wait while loading."
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
    }
}
