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
        ToolIcon {
            id: help
            iconSource: (theme.inverted ? "question.svg" : "question_inverted.svg")
            onClicked: {
                Qt.openUrlExternally("http://www.meeki.mobi/faq");
            }
        }
    }

    FilesPlusInfo {
        id: productInfo
        version: "v1.0.4"
    }
}
