import QtQuick 1.1
import com.nokia.symbian 1.1

Page {
    id: aboutPage
    z: 3
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
        ToolButton {
            id: donate
            iconSource: (!window.platformInverted ? "love.svg" : "love_inverted.svg")
            platformInverted: window.platformInverted
            onClicked: {
                Qt.openUrlExternally("http://www.meeki.mobi/donate");
            }
        }
    }

    FilesPlusInfo {
        id: productInfo
        version: appInfo.version
    }
}
