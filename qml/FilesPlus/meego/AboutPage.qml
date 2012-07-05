import QtQuick 1.1
import com.nokia.meego 1.1

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

    FilesPlusInfo {
        id: appInfo
        version: "v1.0.0"
    }
}
