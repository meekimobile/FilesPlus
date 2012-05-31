import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: moreApps
    z: 3
    tools: ToolBarLayout {
        ToolButton {
            id: back
            iconSource: "toolbar-back"
            onClicked: {
                pageStack.pop(moreApps);
            }
        }
    }

    FolderPieInfo {
        id: appInfo
    }
}
