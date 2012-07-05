import QtQuick 1.1
import com.nokia.meego 1.1

Page {
    id: moreApps
    z: 3
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
