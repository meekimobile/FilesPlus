import QtQuick 1.1

Column {
    id: folderPieInfo
    spacing: 10
    width: 270
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter: parent.verticalCenter

    property string version

    Image {
        id: icon
        width: 180
        height: 180
        anchors.horizontalCenter: parent.horizontalCenter
        source: "FolderPie256.png"
    }

    Text {
        id: title
        color: "white"
        text: qsTr("FolderPie" + ((version=="")?"":(" "+version)))
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
        text: qsTr("FolderPie will help you collect each folder actual size on your disk space. And present in Pie view for easy understanding at glance.")
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
        text: qsTr("\n\nDeveloped by MeekiMobile")
        horizontalAlignment: Text.AlignHCenter
        anchors.horizontalCenter: parent.horizontalCenter
        font.pointSize: 6
        wrapMode: Text.WordWrap
        font.family: "Century Gothic"
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
