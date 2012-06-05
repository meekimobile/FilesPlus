import QtQuick 1.1

Column {
    id: filePlusInfo
    width: 270
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter: parent.verticalCenter

    Image {
        id: icon
        width: 180
        height: 180
        anchors.horizontalCenter: parent.horizontalCenter
        source: "FilesPlus256.png"
    }

    Text {
        id: title
        color: "white"
        text: qsTr("Files+ " + window.version)
        anchors.horizontalCenter: parent.horizontalCenter
        horizontalAlignment: Text.AlignLeft
        font.bold: true
        font.family: "Century Gothic"
        font.pointSize: 10
    }

    Text {
        id: description
        color: "grey"
        width: parent.width
        text: "File+ will provide extended functionalities beyond bundled file manager.\
\nPrint via Google® CloudPrint.\
\nSync via Dropbox®.\
\nView images in folder."
        horizontalAlignment: Text.AlignHCenter
        anchors.horizontalCenter: parent.horizontalCenter
        wrapMode: Text.WordWrap
        font.family: "Century Gothic"
        font.pointSize: 6
    }

    Text {
        id: author
        width: parent.width
        color: "grey"
        text: qsTr("\n\nDeveloped by MeekiMobile\nhttp://sites.google.com/site/meekimobile")
        horizontalAlignment: Text.AlignHCenter
        anchors.horizontalCenter: parent.horizontalCenter
        font.pointSize: 6
        wrapMode: Text.WordWrap
        font.family: "Century Gothic"

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onClicked: {
                Qt.openUrlExternally("http://sites.google.com/site/meekimobile");
            }
        }
    }

    Image {
        source: "MeekiMobile256.svg"
        width: 64
        height: width
        anchors.horizontalCenter: parent.horizontalCenter

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onClicked: {
                Qt.openUrlExternally("http://sites.google.com/site/meekimobile");
            }
        }
    }
}
