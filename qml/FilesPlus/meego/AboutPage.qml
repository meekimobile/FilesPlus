import QtQuick 1.1
import com.nokia.meego 1.0

Page {
    id: aboutPage
    z: 3
    orientationLock: PageOrientation.LockPortrait
    tools: ToolBarLayout {
        ToolIcon {
            id: back
            iconId: "toolbar-back"
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
