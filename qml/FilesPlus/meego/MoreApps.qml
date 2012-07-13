import QtQuick 1.1
import com.nokia.meego 1.0

Page {
    id: moreApps
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

    FolderPieInfo {
        id: appInfo
    }
}
