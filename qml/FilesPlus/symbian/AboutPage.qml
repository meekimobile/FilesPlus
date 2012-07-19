import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: aboutPage
    z: 3
    orientationLock: PageOrientation.LockPortrait
    tools: ToolBarLayout {
        ToolButton {
            id: back
            iconSource: "toolbar-back"
            platformInverted: window.platformInverted
            onClicked: {
                pageStack.pop();
            }
        }
    }

    FilesPlusInfo {
        id: appInfo
        version: "v1.0.2"
    }
}
