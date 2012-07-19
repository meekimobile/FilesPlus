import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: moreApps
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

    FolderPieInfo {
        id: appInfo
    }
}
