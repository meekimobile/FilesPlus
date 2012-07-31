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
        ToolButton {
            id: help
            iconSource: (!window.platformInverted ? "question.svg" : "question_inverted.svg")
            platformInverted: window.platformInverted
            onClicked: {
                Qt.openUrlExternally("http://www.meeki.mobi/faq");
            }
        }
    }

    FilesPlusInfo {
        id: productInfo
        version: "v1.0.3"
    }
}
