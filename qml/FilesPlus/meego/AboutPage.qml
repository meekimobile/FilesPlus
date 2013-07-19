import QtQuick 1.1
import com.nokia.meego 1.0

Page {
    id: aboutPage
    z: 3
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
        ToolIcon {
            id: donate
            iconSource: (theme.inverted ? "love.svg" : "love_inverted.svg")
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
