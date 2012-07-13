import QtQuick 1.1
import com.nokia.meego 1.0

Dialog {
    id: commonDialog
    width: 400
    height: 400
    opacity: 0.6
    style: DialogStyle {
        dim: 0.9
    }

    property alias titleText: title.text
    property alias titleIcon: titleIcon.source
    property alias contentText: content.text
    property variant buttonTexts: ["OK", "Cancel"]

    signal opening()
    signal opened()
    signal closing()
    signal closed()
    signal buttonClicked(int index)

    buttons: Row {
        id: buttonRow
        width: parent.width - 20
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 5

        property int buttonWidth: (width / buttonTexts.length) - spacing

        Repeater {
            model: buttonTexts
            Button {
                text: modelData
                width: buttonRow.buttonWidth
                onClicked: {
                    buttonClicked(index);
                    close();
                }
            }
        }
    }

    title: Row {
        width: parent.width - 20
        height: implicitHeight
        anchors.horizontalCenter: parent.horizontalCenter
        Text {
            id: title
            color: "white"
            font.pointSize: 24
            wrapMode: Text.Wrap
            elide: Text.ElideRight
            width: parent.width - titleIcon.width
            height: implicitHeight
        }
        Image {
            id: titleIcon
            width: 48
            height: 48
        }
    }

    content: Text {
        id: content
        color: "grey"
        font.pointSize:  18
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        width: parent.width - 20
        height: implicitHeight
        anchors.horizontalCenter: parent.horizontalCenter
    }
    
    onStatusChanged: {
        if (status == DialogStatus.Opening) {
            opening();
        } else if (status == DialogStatus.Open) {
            opened();
        } else if (status == DialogStatus.Closing) {
            closing();
        } else if (status == DialogStatus.Closed) {
            closed();
        }
    }
}
