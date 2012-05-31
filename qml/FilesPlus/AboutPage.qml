import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: aboutPage
    z: 3
    tools: ToolBarLayout {
        ToolButton {
            id: back
            iconSource: "toolbar-back"
            onClicked: {
                pageStack.pop();
            }
        }
    }

    FolderPieInfo {
        id: appInfo
    }
}
