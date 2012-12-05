import QtQuick 1.1
import com.nokia.meego 1.0

Dialog {
    id: commonDialog
    width: parent.width - 40
    opacity: 0.6
    style: DialogStyle {
        dim: 0.9
    }

    property alias titleText: title.text
    property alias titleIcon: titleIcon.source
    property alias contentText: content.text
    property variant buttonTexts: [appInfo.emptyStr+qsTr("OK"), appInfo.emptyStr+qsTr("Cancel")]

    signal opening()
    signal opened()
    signal closing()
    signal closed()
    signal buttonClicked(int index)

    buttons: Row {
        id: buttonRow
        width: parent.width - 20
        height: 60
        spacing: 5
        anchors.horizontalCenter: parent.horizontalCenter

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

    title: Item {
        id: titleRow
        width: parent.width
        height: 60
        Text {
            id: title
            color: "white"
            font.pointSize: 20
            elide: Text.ElideRight
            width: parent.width - titleIcon.width
            height: implicitHeight
            anchors.verticalCenter: parent.verticalCenter
        }
        Image {
            id: titleIcon
            width: 48
            height: 48
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                // To stop mouse click event to emit reject.
                console.debug("commonDialog title onClicked");
            }
        }
    }

    content: Flickable {
        width: parent.width
        height: Math.min(content.implicitHeight, commonDialog.parent.height - 160)
        contentWidth: content.width
        contentHeight: content.height
        flickableDirection: Flickable.VerticalFlick
        clip: true

        Text {
            id: content
            color: "grey"
            font.pointSize:  16
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            verticalAlignment: Text.AlignVCenter
            width: commonDialog.width - 20
            height: implicitHeight
            anchors.horizontalCenter: parent.horizontalCenter

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    // To stop mouse click event to emit reject.
                    console.debug("commonDialog content onClicked");
                }
            }
        }
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
