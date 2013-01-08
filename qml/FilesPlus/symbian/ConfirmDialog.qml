import QtQuick 1.1
import com.nokia.symbian 1.1

CommonDialog {
    id: confirmDialog

    property alias contentText: content.text

    z: 2
    titleIcon: "FilesPlusIcon.svg"
    buttonTexts: [appInfo.emptyStr+qsTr("OK"), appInfo.emptyStr+qsTr("Cancel")]
    content: Flickable {
        id: contentFlick
        width: parent.width - 20
        height: Math.min(content.implicitHeight, screen.height - ((screen.height > screen.width) ? 260 : 180) )
        anchors.horizontalCenter: parent.horizontalCenter
        contentWidth: content.width
        contentHeight: content.height
        flickableDirection: Flickable.VerticalFlick
        clip: true

        Text {
            id: content
            color: "white"
            font.pointSize: 6
            wrapMode: Text.Wrap
            verticalAlignment: Text.AlignVCenter
            width: contentFlick.width - 20
            height: implicitHeight
        }
    }

    signal confirm()
    signal reject()
    signal closed()
    signal opening()
    signal opened()

    onButtonClicked: {
        if (index === 0) confirm();
        else reject();
    }

    onStatusChanged: {
        if (status == DialogStatus.Opening) {
            opening();
        } else if (status == DialogStatus.Open) {
            opened();
        } else if (status == DialogStatus.Closed) {
            closed();
        }
    }
}
