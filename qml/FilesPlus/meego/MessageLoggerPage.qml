import QtQuick 1.1
import com.nokia.meego 1.0
import "Utility.js" as Utility

Page {
    id: messageLoggerPage

    property string name: "messageLoggerPage"
    property bool inverted: !theme.inverted

    function getMessageTypeColor(messageType) {
        if (messageType == "error") return "red";
        else if (messageType == "warn") return "orange";
        else return "#00AAFF";
    }

    tools: toolBarLayout

    ToolBarLayout {
        id: toolBarLayout

        ToolIcon {
            id: backButton
            iconId: "toolbar-back"
            onClicked: {
                pageStack.pop();
            }
        }
        ToolIcon {
            id: readAllButton
            iconSource: (!inverted) ? "ok.svg" : "ok_inverted.svg"
            onClicked: {
                messageLoggerModel.readAllMessage();
            }
        }
        ToolIcon {
            id: deleteAllButton
            iconSource: (!inverted) ? "delete.svg" : "delete_inverted.svg"
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

        onMovementStarted: {
            if (currentItem) {
                currentItem.pressed = false;
            }
        }
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
                    color: (!isRead) ? getMessageTypeColor(messageType) : "transparent"
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
                            font.pointSize: 14
                            elide: Text.ElideMiddle
                            color: (!inverted) ? "white" : "black"
                        }
                        Text {
                            id: logTimeText
                            text: Qt.formatDateTime(timestamp, "h:mm:ss ap")
                            font.pointSize: 14
                            horizontalAlignment: Text.Right
                            anchors.verticalCenter: parent.verticalCenter
                            color: (!inverted) ? "grey" : "#202020"
                        }
                    }
                    Text {
                        id: subtitle
                        text: message
                        width: parent.width
                        font.pointSize: 14
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
