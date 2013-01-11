import QtQuick 1.1
import com.nokia.symbian 1.1
import "Utility.js" as Utility

Page {
    id: messageLoggerPage

    property string name: "messageLoggerPage"
    property bool inverted: window.platformInverted

    tools: toolBarLayout

    ToolBarLayout {
        id: toolBarLayout

        ToolButton {
            id: backButton
            iconSource: "toolbar-back"
            platformInverted: window.platformInverted
            flat: true
            onClicked: {
                pageStack.pop();
            }
        }
        ToolButton {
            id: readAllButton
            iconSource: (!inverted) ? "ok.svg" : "ok_inverted.svg"
            platformInverted: window.platformInverted
            flat: true
            onClicked: {
                messageLoggerModel.readAllMessage();
            }
        }
        ToolButton {
            id: deleteAllButton
            iconSource: (!inverted) ? "delete.svg" : "delete_inverted.svg"
            platformInverted: window.platformInverted
            flat: true
            onClicked: {
                messageLoggerModel.clear();
                messageLoggerModel.newMessageCount = 0;
            }
        }
    }

    TitlePanel {
        id: titlePanel
        text: appInfo.emptyStr+qsTr("Message Logger")
    }

    ListView {
        id: logListView
        width: parent.width
        height: parent.height - titlePanel.height
        anchors.top: titlePanel.bottom
        model: messageLoggerModel
        delegate: logListItemDelegate
        clip: true
    }

    ScrollDecorator {
        id: scrollbar
        flickableItem: logListView
    }

    Component {
        id: logListItemDelegate

        ListItem {
            id: listItem

            Row {
                anchors.fill: parent
                spacing: 5
                Rectangle {
                    id: readIndicator
                    color: (!isRead) ? "#00AAFF" : "transparent"
                    width: 5
                    height: parent.height
                }
                Column {
                    width: parent.width - readIndicator.width - parent.spacing
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 5
                    Row {
                        width: parent.width
                        Text {
                            id: title
                            text: titleText
                            width: parent.width - logTimeText.width
                            font.pointSize: 8
                            elide: Text.ElideMiddle
                            color: (!inverted) ? "white" : "black"
                        }
                        Text {
                            id: logTimeText
                            text: Qt.formatDateTime(timestamp, "h:mm:ss ap")
                            font.pointSize: 6
                            horizontalAlignment: Text.Right
                            anchors.verticalCenter: parent.verticalCenter
                            color: (!inverted) ? "grey" : "#202020"
                        }
                    }
                    Text {
                        id: subtitle
                        text: message
                        width: parent.width
                        font.pointSize: 6
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        color: (!inverted) ? "grey" : "#202020"
                    }
                }
            }

            onClicked: {
                messageLoggerModel.readMessage(index);

                messageDialog.titleText = titleText;
                messageDialog.message = message;
                messageDialog.open();
            }
        }
    }
}
