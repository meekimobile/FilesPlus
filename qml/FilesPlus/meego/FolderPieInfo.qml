import QtQuick 1.1

Column {
    id: folderPieInfo
    spacing: 10
    width: parent.width
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter: parent.verticalCenter

    property string version

    Image {
        id: icon
        width: 180
        height: 180
        anchors.horizontalCenter: parent.horizontalCenter
        source: "FolderPie256.png"

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onClicked: {
                Qt.openUrlExternally("https://sites.google.com/site/meekimobile/products/folderpie");
            }
        }
    }

    Text {
        id: title
        color: "white"
        text: qsTr("FolderPie" + ((version=="")?"":(" "+version)))
        anchors.horizontalCenter: parent.horizontalCenter
        horizontalAlignment: Text.AlignLeft
        font.bold: true
        font.pointSize: 24
    }

    Text {
        id: description
        color: "grey"
        width: 400
        text: qsTr("FolderPie helps you collect each folder actual size on your disk space.\nAnd present in Pie view for easy understanding at glance.")
        horizontalAlignment: Text.AlignHCenter
        anchors.horizontalCenter: parent.horizontalCenter
        wrapMode: Text.WordWrap
        font.pointSize: 16
    }

    Text {
        id: author
        width: parent.width
        color: "grey"
        text: "\n\n" + qsTr("Developed by") + " MeekiMobile"
        horizontalAlignment: Text.AlignHCenter
        anchors.horizontalCenter: parent.horizontalCenter
        wrapMode: Text.WordWrap
        font.pointSize: 14
    }

    Image {
        source: "MeekiMobile256.svg"
        width: 48
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
