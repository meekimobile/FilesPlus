import QtQuick 1.1

Rectangle {
    id: background
    anchors.fill: parent
    color: "black"

    property string version

    Column {
        id: folderPieInfo
        spacing: 10
        width: parent.width
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter

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
                    Qt.openUrlExternally("http://www.meeki.mobi/products/folderpie");
                }
            }
        }

        Text {
            id: title
            color: "white"
            text: appInfo.emptyStr+qsTr("FolderPie" + ((version=="")?"":(" "+version)))
            anchors.horizontalCenter: parent.horizontalCenter
            horizontalAlignment: Text.AlignLeft
            font.bold: true
            font.pointSize: 10
        }

        Text {
            id: description
            color: "grey"
            width: 300
            text: appInfo.emptyStr+qsTr("FolderPie helps you collect each folder actual size on your disk space.\nAnd present in Pie view for easy understanding at glance.")
            horizontalAlignment: Text.AlignHCenter
            anchors.horizontalCenter: parent.horizontalCenter
            wrapMode: Text.WordWrap
            font.pointSize: 6
        }

        Text {
            id: author
            width: parent.width
            color: "grey"
            text: "\n\n" + appInfo.emptyStr+qsTr("Developed by") + " MeekiMobile"
            horizontalAlignment: Text.AlignHCenter
            anchors.horizontalCenter: parent.horizontalCenter
            wrapMode: Text.WordWrap
            font.pointSize: 6
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
                    Qt.openUrlExternally("http://www.meeki.mobi/");
                }
            }
        }
    }
}
