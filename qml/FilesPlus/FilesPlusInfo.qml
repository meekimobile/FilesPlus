import QtQuick 1.1

Column {
    id: filePlusInfo
    width: 270
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter: parent.verticalCenter

    property string version

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
        text: qsTr("FilesPlus" + ((version=="")?"":(" "+version)))
        anchors.horizontalCenter: parent.horizontalCenter
        horizontalAlignment: Text.AlignLeft
        font.bold: true
        font.family: "Century Gothic"
        font.pointSize: 10
    }

    Text {
        id: description
        color: "grey"
        width: 270
        text: "FilesPlus provide extended functionalities beyond file manager.\
\n • Print with Google™ Cloud Print.\
\n • Sync with Cloud Drive.\
\n • Preview images in your folder.\
\n • Present folders in Pie view."
        anchors.horizontalCenter: parent.horizontalCenter
        wrapMode: Text.WordWrap
        font.family: "Century Gothic"
        font.pointSize: 6
    }

    Row {
        height: 80
        anchors.horizontalCenter: parent.horizontalCenter
        Image {
            source: "dropbox_white.png"
            anchors.verticalCenter: parent.verticalCenter
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onClicked: {
                    Qt.openUrlExternally("https://www.dropbox.com");
                }
            }
        }
        Image {
            source: "cloudprint-ready.png"
            anchors.verticalCenter: parent.verticalCenter
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onClicked: {
                    Qt.openUrlExternally("http://www.google.com/cloudprint/learn");
                }
            }
        }
    }

    Text {
        id: author
        width: parent.width
        color: "grey"
        text: qsTr("Developed by MeekiMobile")
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
